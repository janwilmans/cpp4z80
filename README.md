# cpp4z80 — a GCC backend for the Z80

A Z80 port of GCC 16.2 (`z80-elf`), with libgcc, newlib (C library) and a
freestanding libstdc++ (C++ library), so that C and modern C++ programs can
be compiled for the Z80.

```c
#include <stdio.h>
int main (void)
{
  printf ("Hello from the Z80!\n");
  return 0;
}
```

```sh
export PATH=$PWD/install/bin:$PATH
z80-elf-gcc -Os -c tests/support/sys.c          # system layer, see below
z80-elf-gcc -Os -o hello.elf hello.c sys.o
z80-elf-objcopy -O binary hello.elf hello.bin   # raw image, loads at 0x100
```

## Status

What works (each item is covered by the test suite, which runs every test
program at `-O0`, `-O1`, `-O2` and `-Os` in a Z80 emulator):

- C: `char`/`int` (16-bit)/`long` (32-bit)/`long long` (64-bit) arithmetic,
  pointers, arrays, structs (copy, pass, return), bitfields, unions,
  `switch` (jump tables), recursion, function pointers, varargs, large stack
  frames, `float` (`double` and `long double` are 32-bit as well).
- C++ (`-fno-exceptions -fno-rtti`): classes, templates, virtual functions,
  global constructors, `new`/`delete`, and the freestanding standard
  library: `<array>`, `<algorithm>`, `<numeric>`, `<optional>`,
  `<variant>`, `<tuple>`, `<span>`, `<string_view>`, `<ranges>`, `<bit>`,
  `<limits>`, ... (no iostreams, strings or containers that need a heap
  allocator from the library).
- C library (newlib, nano variant): `printf` family, string functions,
  `malloc`, `qsort`, `setjmp`/`longjmp`, ...; the math library builds too.
- `__attribute__ ((interrupt))` (saves the registers it uses, returns with
  `ei` / `reti`), `__attribute__ ((nmi))` (`retn`) and
  `__attribute__ ((naked))`; inline assembly with register constraints.
- `<z80intrin.h>`: `z80_in`/`z80_out` (16-bit port in BC),
  `Z80_IN`/`Z80_OUT` (constant 8-bit port), `z80_di`, `z80_ei`,
  `z80_im`, `z80_halt`, `z80_nop`.

Not (yet) supported: C++ exceptions and RTTI, the hosted C++ library,
code banking, the undocumented IXH/IXL/IYH/IYL instructions, Z80 variants
(Z180, eZ80, R800, ...).

## Calling convention

| what                              | where                                     |
|-----------------------------------|-------------------------------------------|
| 1st 8/16-bit argument             | `L` / `HL`                                |
| 2nd 8/16-bit argument             | `E` / `DE`                                |
| other arguments, 32/64-bit values, varargs | stack, pushed right to left, 2-byte slots |
| stack cleanup                     | caller                                    |
| 8 / 16 / 32-bit return value      | `L` / `HL` / `DE:HL` (DE = low word)      |
| larger return values              | memory (hidden pointer)                   |
| preserved by the callee           | `IX` (frame pointer)                      |
| symbols                           | C names get a `_` prefix (`main` is `_main`) |

`IY` is a scratch register used inside single instructions (stack
adjustments, frame accesses beyond `(ix-128)`); it is never live across
instructions.

## Running programs

`crt0.o` (from libgcc) sets SP to `__stack` (default 0: top of memory),
clears `.bss`, runs constructors, calls `main` and passes the result to
`_exit`, which by default executes `HALT`. The default linker script
places the program at 0x100 (as on CP/M); use `-T` with your own script for
other layouts (e.g. ROM at 0).

newlib needs a few system functions for I/O and memory: `write`, `read`,
`sbrk`, `close`, `lseek`, `fstat`, `isatty`, ... `tests/support/sys.c` is a
small example that sends console output to I/O port 1 and puts the heap
after the program; adapt it to your hardware.

## Building

The sources of binutils 2.45, GCC 16.2.0 and newlib 4.6.0 are in `src/`.

```sh
scripts/setup-test-env.sh              # binutils (z80-elf) + emulator venv
src/gcc-16.2.0/build.sh                # configure + build the compiler
scripts/build-toolchain.sh             # libgcc, newlib, libstdc++, install
tests/run_tests.py                     # run the test suite
```

Everything is installed into `install/`. During backend development:

```sh
scripts/make-gcc.sh                    # incremental compiler rebuild
scripts/make-libgcc.sh                 # rebuild libgcc with it
tests/run_tests.py -v -O s tests/c/021_loops.c   # one test, show the asm
tests/run_tests.py --ticks             # also report executed T-states
scripts/code-size.sh                   # code size of a fixed test set
```

See [tests/README.md](tests/README.md) for how the tests work.

## Where things are

| path | contents |
|------|----------|
| `src/gcc-16.2.0/gcc/config/z80/` | the backend: `z80.h` (registers, ABI, layout), `z80.md` (instruction patterns), `z80.cc` (output code, costs, prologue/epilogue, ...), `constraints.md` |
| `src/gcc-16.2.0/gcc/config/z80/z80intrin.h` | intrinsics header (`#include <z80intrin.h>`) |
| `src/gcc-16.2.0/libgcc/config/z80/` | `lib1funcs.S` (multiply, divide, shifts, bit operations), `crt0.S`, `t-z80` |
| `src/newlib-4.6.0.20260123/newlib/libc/machine/z80/` | `setjmp.S` |
| `tests/` | test programs and the test harness |
| `scripts/` | build and measurement helpers |

## Design notes

- **Registers.** The 8-bit registers are hard registers; 16-bit values live
  in pairs, numbered low byte first (C,B / E,D / L,H) because GCC maps the
  first register of a group to the lowest byte. `UNITS_PER_WORD` is 1, like
  on AVR.
- **No flags register in RTL.** Every comparison is emitted together with
  its conditional jump as a single instruction, so no pass can separate
  them.
- **Pointers.** Only HL (and IX/IY with a displacement) can address memory
  for every instruction. BC and DE are base registers too: `(bc)`/`(de)`
  accesses are emitted as `ex de,hl ... ex de,hl` or
  `push hl / ld h,b / ld l,c ... pop hl`, which keeps the register
  allocator from running out of pointer registers.
- **Frame.** IX is the frame pointer; locals are at `(ix-d)`, stack
  arguments at `(ix+4+d)`. Offsets beyond -128..127 are reached by pointing
  IY at the slot first.
- **Moves never clobber registers** (they use `push`/`pop`/`ex` for
  temporaries), so the register allocator can insert them anywhere.
