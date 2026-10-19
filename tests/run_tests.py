#!/usr/bin/env python3
"""Compile, link and execute the Z80 test programs.

Every test is a C or C++ file containing a line

    // expect: <value>

where <value> is the value that main() must return (it is read from HL
after the program halts).  Optionally a test can restrict the optimisation
levels it is run at:

    // opts: -O0 -O2

Each test is compiled with the freshly built cross compiler, assembled with
the z80-elf binutils and linked through the compiler driver (crt0.o and
libgcc.a from the libgcc build, the default linker script) together with
tests/support/libc.s, and run in the `z80` Python emulator.

Usage:  tests/run_tests.py [-v] [-O LEVEL ...] [test files...]
"""

import argparse
import glob
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TESTS = os.path.join(ROOT, "tests")
SUPPORT = os.path.join(TESTS, "support")
GCC_BUILD = os.path.join(ROOT, "src", "build-z80", "gcc")
BINUTILS = os.path.join(ROOT, "install", "bin")
OUT = os.path.join(ROOT, "work", "test-out")

XGCC = [os.path.join(GCC_BUILD, "xgcc"), "-B" + GCC_BUILD + "/"]
AS = os.path.join(BINUTILS, "z80-elf-as")
NM = os.path.join(BINUTILS, "z80-elf-nm")

DEFAULT_OPTS = ["-O0", "-O1", "-O2", "-Os"]
# The emulator is fast (10^9 ticks take about a second), so be generous.
MAX_TICKS = int(os.environ.get("Z80_MAX_TICKS", 1_000_000_000))


class TestError(Exception):
    def __init__(self, stage, message):
        super().__init__(message)
        self.stage = stage


def run(cmd, stage):
    try:
        proc = subprocess.run(cmd, capture_output=True, text=True,
                              timeout=60)
    except subprocess.TimeoutExpired:
        raise TestError(stage, "timeout")
    if proc.returncode != 0:
        raise TestError(stage, (proc.stdout + proc.stderr).strip())
    return proc


LIBGCC_DIR = os.path.join(ROOT, "src", "build-z80", "z80-elf", "libgcc")


NEWLIB = os.path.join(ROOT, "install", "z80-elf")
NEWLIB_FLAGS = ["-isystem", os.path.join(NEWLIB, "include")]
# The freestanding libstdc++ headers (installed by scripts/make-libstdcxx.sh).
CXX_INCLUDE = os.path.join(NEWLIB, "include", "c++", "16.2.0")
CXX_FLAGS = ["-isystem", CXX_INCLUDE,
             "-isystem", os.path.join(CXX_INCLUDE, "z80-elf")]


def support_objects(outdir):
    """Compile the system layer for newlib once."""
    if not os.path.exists(os.path.join(LIBGCC_DIR, "libgcc.a")):
        sys.exit("libgcc.a not found: run scripts/make-libgcc.sh")
    if not os.path.exists(os.path.join(NEWLIB, "lib", "libc.a")):
        sys.exit("newlib not found: run scripts/install-gcc.sh and "
                 "scripts/build-newlib.sh")
    obj = os.path.join(outdir, "sys.o")
    run(XGCC + NEWLIB_FLAGS + ["-Os", "-c", "-o", obj,
                               os.path.join(SUPPORT, "sys.c")],
        "compile sys.c")
    return [obj]


def load_elf(path):
    """Return (memory image, entry point) from the PT_LOAD segments."""
    data = open(path, "rb").read()
    entry, phoff = struct.unpack_from("<II", data, 0x18)
    phentsize, phnum = struct.unpack_from("<HH", data, 0x2a)
    # Fill memory with garbage so that a missing .bss clear or a read of
    # uninitialised memory does not go unnoticed.
    memory = bytearray(b"\xa5" * 0x10000)
    for i in range(phnum):
        (p_type, p_offset, _vaddr, p_paddr, p_filesz, _memsz, _flags,
         _align) = struct.unpack_from("<8I", data, phoff + i * phentsize)
        if p_type == 1 and p_filesz:
            memory[p_paddr:p_paddr + p_filesz] = \
                data[p_offset:p_offset + p_filesz]
    return bytes(memory), entry


def parse_spec(path):
    text = open(path).read()
    m = re.search(r"//\s*expect:\s*(-?(0x)?[0-9a-fA-F]+)", text)
    if not m:
        raise TestError("spec", "no '// expect:' line")
    expect = int(m.group(1), 0)
    m = re.search(r"//\s*opts:\s*(.*)", text)
    opts = m.group(1).split() if m else None
    # "// output: text" lines give the expected console output, one line
    # of output each.
    lines = re.findall(r"^//\s*output:\s?(.*)$", text, re.M)
    output = "".join(line + "\n" for line in lines) if lines else None
    return expect, opts, output


def exit_address(elf):
    """Address of _exit (assembly name __exit), where crt0's HALT is."""
    proc = run([NM, elf], "nm")
    for line in proc.stdout.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == "__exit":
            return int(parts[0], 16)
    return None


def execute(elf):
    import z80

    memory, entry = load_elf(elf)
    machine = z80.Z80Machine()
    machine.set_memory_block(0, memory)
    machine.pc = entry
    # Stop exactly at _exit: after a HALT the emulator keeps spending
    # T-states until the end of its frame, which would distort the count.
    stop = exit_address(elf)
    if stop is not None:
        machine.set_breakpoint(stop)
    output = bytearray()

    def on_output(port, value):
        if port & 0xff == 1:
            output.append(value)

    machine.set_output_callback(on_output)

    # Input: port 2 returns 0x42, any other port the high byte of the
    # 16-bit port address (to check what was put on the address bus).
    def on_input(port):
        return 0x42 if port & 0xff == 2 else port >> 8

    machine.set_input_callback(on_input)
    # run() also returns on internal events (e.g. the end of a video
    # frame), so keep going until HALT or until the tick budget is spent.
    machine.ticks_to_stop = MAX_TICKS
    while (not machine.halted and machine.ticks_to_stop > 0
           and machine.pc != stop):
        machine.run()
    if not machine.halted and machine.pc != stop:
        raise TestError("run", "did not halt within %d ticks (pc=0x%04x)"
                        % (MAX_TICKS, machine.pc))
    ticks = MAX_TICKS - machine.ticks_to_stop
    return machine.hl, output.decode("latin-1"), ticks


def run_test(path, opt, support):
    name = os.path.splitext(os.path.basename(path))[0]
    lang = "c++" if path.endswith(".cpp") else "c"
    workdir = os.path.join(OUT, "%s-%s%s" % (lang, name, opt))
    os.makedirs(workdir, exist_ok=True)
    asm = os.path.join(workdir, name + ".s")
    obj = os.path.join(workdir, name + ".o")
    elf = os.path.join(workdir, name + ".elf")

    cflags = [opt, "-ffreestanding", "-fno-builtin"]
    libs = []
    incs = NEWLIB_FLAGS
    if lang == "c++":
        cflags += ["-fno-exceptions", "-fno-rtti", "-std=gnu++23"]
        incs = CXX_FLAGS + NEWLIB_FLAGS
        libs = ["-lstdc++"]
    run(XGCC + incs + ["-x", lang, "-S", "-o", asm] + cflags + [path],
        "compile")
    run([AS, "-o", obj, asm], "assemble")
    # Link through the compiler driver: crt0.o and libgcc.a come from the
    # libgcc build directory, the default linker script is used.
    run(XGCC + ["-B" + LIBGCC_DIR + "/", "-L" + LIBGCC_DIR,
                "-L" + os.path.join(NEWLIB, "lib"),
                "-Wl,--no-warn-rwx-segments",
                "-Wl,-Map," + os.path.join(workdir, name + ".map"),
                "-o", elf, obj] + support + libs, "link")
    return execute(elf), asm


def ensure_emulator():
    """Re-run this script with the venv's Python if `z80` is missing."""
    try:
        import z80  # noqa: F401
    except ImportError:
        venv_python = os.path.join(ROOT, "work", "venv", "bin", "python")
        if os.path.exists(venv_python) and sys.executable != venv_python:
            os.execv(venv_python, [venv_python] + sys.argv)
        sys.exit("the `z80` package is missing: run scripts/setup-test-env.sh")


def main():
    ensure_emulator()
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("tests", nargs="*", help="test files (default: all)")
    parser.add_argument("-O", dest="opts", action="append",
                        help="optimisation level(s) to test, e.g. -O 0")
    parser.add_argument("--ticks", action="store_true",
                        help="report the total executed T-states per level")
    parser.add_argument("--benchmark", action="store_true",
                        help="run the tests listed in tests/benchmark.txt")
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the generated assembly of failing tests")
    args = parser.parse_args()

    if args.benchmark:
        with open(os.path.join(TESTS, "benchmark.txt")) as f:
            args.tests = [os.path.join(TESTS, line.strip()) for line in f
                          if line.strip()]
    tests = args.tests or sorted(glob.glob(os.path.join(TESTS, "c", "*.c"))
                                 + glob.glob(os.path.join(TESTS, "cpp",
                                                          "*.cpp")))
    levels = ["-O" + o for o in args.opts] if args.opts else None

    os.makedirs(OUT, exist_ok=True)
    support = support_objects(OUT)

    passed = failed = 0
    total_ticks = {}
    for path in tests:
        rel = os.path.relpath(path, TESTS)
        try:
            expect, test_opts, expect_output = parse_spec(path)
        except TestError as e:
            print("FAIL %-32s %s" % (rel, e))
            failed += 1
            continue
        for opt in levels or test_opts or DEFAULT_OPTS:
            label = "%-32s %-4s" % (rel, opt)
            asm = None
            try:
                (result, output, ticks), asm = run_test(path, opt, support)
                total_ticks[opt] = total_ticks.get(opt, 0) + ticks
                if expect_output is not None and output != expect_output:
                    msg = "output %r, expected %r" % (output, expect_output)
                    raise TestError("output", msg)
                if result == expect & 0xffff:
                    print("PASS " + label)
                    passed += 1
                    continue
                msg = "returned %d (0x%04x), expected %d" % (
                    result, result, expect)
                stage = "result"
            except TestError as e:
                msg, stage = str(e), e.stage
            failed += 1
            first = msg.splitlines()[0] if msg else ""
            print("FAIL %s [%s] %s" % (label, stage, first))
            if args.verbose:
                if len(msg.splitlines()) > 1:
                    print(msg)
                if asm and os.path.exists(asm):
                    print(open(asm).read())

    print("\n%d passed, %d failed" % (passed, failed))
    if args.ticks:
        for opt in sorted(total_ticks):
            print("%s: %d ticks" % (opt, total_ticks[opt]))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
