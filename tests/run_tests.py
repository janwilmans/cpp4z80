#!/usr/bin/env python3
"""Compile, link and execute the Z80 test programs.

Every test is a C or C++ file containing a line

    // expect: <value>

where <value> is the value that main() must return (it is read from HL
after the program halts).  Optionally a test can restrict the optimisation
levels it is run at:

    // opts: -O0 -O2

Each test is compiled with the freshly built cross compiler, assembled and
linked with the z80-elf binutils together with tests/support/crt0.s (and
the runtime library, if present), and run in the `z80` Python emulator.

Usage:  tests/run_tests.py [-v] [-O LEVEL ...] [test files...]
"""

import argparse
import glob
import os
import re
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
LD = os.path.join(BINUTILS, "z80-elf-ld")
OBJCOPY = os.path.join(BINUTILS, "z80-elf-objcopy")

DEFAULT_OPTS = ["-O0", "-O1", "-O2", "-Os"]
MAX_TICKS = 20_000_000


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


def support_objects(outdir):
    """Assemble crt0.s and runtime.s (if present) once."""
    objs = []
    for name in ("crt0.s", "runtime.s"):
        src = os.path.join(SUPPORT, name)
        if not os.path.exists(src):
            continue
        obj = os.path.join(outdir, name.replace(".s", ".o"))
        run([AS, "-o", obj, src], "assemble " + name)
        objs.append(obj)
    return objs


def parse_spec(path):
    text = open(path).read()
    m = re.search(r"//\s*expect:\s*(-?(0x)?[0-9a-fA-F]+)", text)
    if not m:
        raise TestError("spec", "no '// expect:' line")
    expect = int(m.group(1), 0)
    m = re.search(r"//\s*opts:\s*(.*)", text)
    opts = m.group(1).split() if m else None
    return expect, opts


def execute(binary):
    import z80

    machine = z80.Z80Machine()
    machine.set_memory_block(0, binary)
    machine.ticks_to_stop = MAX_TICKS
    machine.run()
    if not machine.halted:
        raise TestError("run", "did not halt within %d ticks (pc=0x%04x)"
                        % (MAX_TICKS, machine.pc))
    return machine.hl


def run_test(path, opt, support):
    name = os.path.splitext(os.path.basename(path))[0]
    lang = "c++" if path.endswith(".cpp") else "c"
    workdir = os.path.join(OUT, "%s-%s%s" % (lang, name, opt))
    os.makedirs(workdir, exist_ok=True)
    asm = os.path.join(workdir, name + ".s")
    obj = os.path.join(workdir, name + ".o")
    elf = os.path.join(workdir, name + ".elf")
    binfile = os.path.join(workdir, name + ".bin")

    cflags = [opt, "-ffreestanding", "-fno-builtin"]
    if lang == "c++":
        cflags += ["-fno-exceptions", "-fno-rtti"]
    run(XGCC + ["-x", lang, "-S", "-o", asm] + cflags + [path], "compile")
    run([AS, "-o", obj, asm], "assemble")
    run([LD, "-T", os.path.join(SUPPORT, "z80.ld"), "-o", elf,
         "-Map", os.path.join(workdir, name + ".map")] + support[:1]
        + [obj] + support[1:], "link")
    run([OBJCOPY, "-O", "binary", elf, binfile], "objcopy")
    with open(binfile, "rb") as f:
        return execute(f.read()), asm


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
    parser.add_argument("-v", "--verbose", action="store_true",
                        help="print the generated assembly of failing tests")
    args = parser.parse_args()

    tests = args.tests or sorted(glob.glob(os.path.join(TESTS, "c", "*.c"))
                                 + glob.glob(os.path.join(TESTS, "cpp",
                                                          "*.cpp")))
    levels = ["-O" + o for o in args.opts] if args.opts else None

    os.makedirs(OUT, exist_ok=True)
    support = support_objects(OUT)

    passed = failed = 0
    for path in tests:
        rel = os.path.relpath(path, TESTS)
        try:
            expect, test_opts = parse_spec(path)
        except TestError as e:
            print("FAIL %-32s %s" % (rel, e))
            failed += 1
            continue
        for opt in levels or test_opts or DEFAULT_OPTS:
            label = "%-32s %-4s" % (rel, opt)
            asm = None
            try:
                result, asm = run_test(path, opt, support)
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
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
