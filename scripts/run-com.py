#!/usr/bin/env python3
"""Run an MSX-DOS / CP/M .com program on the PC, in a Z80 emulator.

    scripts/run-com.py example.com [ARGUMENTS...]

The program is loaded at 0x100 and runs under a minimal emulated MSX-DOS:
the BDOS calls for console output and input (functions 1, 2, 6, 9, 10, 11,
12) go to this terminal, and the program ends with function 0 or 0x62
(_TERM, the MSX-DOS 2 exit code becomes the exit status of this script).
Everything else (files, BIOS calls, ...) is not emulated.

Uses the `z80` Python package from work/venv (scripts/setup-test-env.sh).
"""

import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

BDOS_ENTRY = 0xda00		# also the top of the TPA (stored at 0x0006)
WBOOT_TRAP = 0xda03


def ensure_emulator():
    """Re-run this script with the venv's Python if `z80` is missing."""
    try:
        import z80  # noqa: F401
    except ImportError:
        venv_python = os.path.join(ROOT, "work", "venv", "bin", "python")
        if os.path.exists(venv_python) and sys.executable != venv_python:
            os.execv(venv_python, [venv_python] + sys.argv)
        sys.exit("the `z80` package is missing: run scripts/setup-test-env.sh")


class ProgramExit(Exception):
    def __init__(self, code):
        super().__init__(code)
        self.code = code


# After BDOS line input, MSX-DOS leaves the cursor behind the typed text
# and the program writes CR LF itself; on this terminal Enter already
# started a new line, so that one CR LF is dropped.
swallow_newline = False


def out(byte):
    global swallow_newline
    if swallow_newline:
        if byte == 0x0d:
            return
        swallow_newline = False
        if byte == 0x0a:
            return
    # The program writes CR LF; a terminal only needs the LF.
    if byte != 0x0d:
        sys.stdout.buffer.write(bytes([byte]))
        sys.stdout.buffer.flush()


def read_line():
    """A line from the terminal (without the newline), or None at EOF."""
    line = sys.stdin.buffer.readline()
    if not line:
        return None
    return line.rstrip(b"\r\n")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__.strip().splitlines()[2].strip())
    ensure_emulator()
    import z80

    program = open(sys.argv[1], "rb").read()
    if len(program) > BDOS_ENTRY - 0x100:
        sys.exit("program too large for the TPA")

    memory = bytearray(0x10000)
    memory[0:3] = bytes([0xc3, WBOOT_TRAP & 0xff, WBOOT_TRAP >> 8])
    memory[5:8] = bytes([0xc3, BDOS_ENTRY & 0xff, BDOS_ENTRY >> 8])
    tail = "".join(" " + a for a in sys.argv[2:]).encode("latin-1")[:126]
    memory[0x80] = len(tail)
    memory[0x81:0x81 + len(tail)] = tail
    memory[0x81 + len(tail)] = 0x0d
    memory[0x100:0x100 + len(program)] = program

    machine = z80.Z80Machine()
    machine.set_memory_block(0, bytes(memory))
    machine.pc = 0x100
    machine.sp = BDOS_ENTRY
    machine.set_breakpoint(BDOS_ENTRY)
    machine.set_breakpoint(WBOOT_TRAP)
    mem = machine.memory
    pending = bytearray()		# typed characters for functions 1 / 6

    def bdos():
        f, de = machine.c, machine.de
        if f == 0:
            raise ProgramExit(0)
        if f == 0x62:			# MSX-DOS 2 _TERM
            raise ProgramExit(machine.b)
        if f == 1:			# console input (the terminal echoes)
            if not pending:
                line = read_line()
                pending.extend(b"\x1a" if line is None else line + b"\r")
            machine.a = machine.l = pending.pop(0)
        elif f == 2:			# console output
            out(machine.e)
        elif f == 6:			# direct console I/O
            if machine.e == 0xff:
                machine.a = pending.pop(0) if pending else 0
            else:
                out(machine.e)
        elif f == 9:			# output string until '$'
            while mem[de] != ord("$"):
                out(mem[de])
                de = (de + 1) & 0xffff
        elif f == 10:			# buffered line input
            global swallow_newline
            swallow_newline = True
            line = read_line()
            line = b"\x1a" if line is None else line[:mem[de]]
            mem[de + 1] = len(line)
            for i, c in enumerate(line):
                mem[de + 2 + i] = c
        elif f == 11:			# console status
            machine.a = 0xff if pending else 0
        elif f == 12:			# version: CP/M 2.2
            machine.a = machine.l = 0x22
            machine.b = machine.h = 0
        else:
            sys.exit("\nrun-com: unsupported BDOS function 0x%02x at 0x%04x"
                     % (f, mem[machine.sp] | (mem[machine.sp + 1] << 8)))
        sp = machine.sp
        machine.pc = mem[sp] | (mem[sp + 1] << 8)
        machine.sp = (sp + 2) & 0xffff

    try:
        while True:
            machine.ticks_to_stop = 1_000_000
            machine.run()
            if machine.pc == BDOS_ENTRY:
                bdos()
            elif machine.pc == WBOOT_TRAP:
                raise ProgramExit(0)
            elif machine.halted:
                sys.exit("\nrun-com: HALT at 0x%04x" % machine.pc)
    except ProgramExit as e:
        return e.code
    except KeyboardInterrupt:
        return 130


if __name__ == "__main__":
    sys.exit(main())
