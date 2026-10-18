# Z80 backend tests

Every file in `c/` and `cpp/` is a complete program. It is compiled with the
freshly built cross compiler at `-O0`, `-O1`, `-O2` and `-Os`, assembled and
linked with the z80-elf binutils, and executed in an emulator (the `z80`
Python package). The value returned by `main()` (read from HL after the
program halts) must equal the value in the file's `// expect:` line.
Optional `// output: text` lines give the expected console output (one line
each), e.g. from `printf`.

```sh
scripts/setup-test-env.sh      # once: binutils + emulator venv
scripts/make-gcc.sh            # rebuild the compiler after backend changes
scripts/make-libgcc.sh         # (re)build libgcc with that compiler
scripts/install-gcc.sh         # install compiler + libgcc into install/
scripts/build-newlib.sh        # build and install newlib (C library)
scripts/make-libstdcxx.sh      # build the freestanding C++ library
scripts/build-toolchain.sh     # all of the above, in order
tests/run_tests.py             # run everything
tests/run_tests.py -v -O 0 tests/c/007_stack_args.c   # one test, show asm
```

Build products (assembly, map files, binaries) go to `work/test-out/`.

Programs are linked through the compiler driver, exactly as a user would
link them:

- `crt0.o` (libgcc, `src/gcc-16.2.0/libgcc/config/z80/crt0.S`) - sets SP,
  clears `.bss`, runs constructors (`.init_array`), calls `main` and halts
  in `_exit`.
- `libgcc.a` - built by `scripts/make-libgcc.sh`: the assembly helpers from
  `lib1funcs.S` (`__mulhi3`, `__divsi3`, ...) plus the generic C routines
  (64-bit integers, soft float).
- newlib's `libc.a` / `libm.a` (built by `scripts/build-newlib.sh` with this
  compiler), with `support/sys.c` as system layer: console output goes to
  I/O port 1, which the emulator captures; the heap starts at `_end`.
- for C++: the freestanding libstdc++ (`<array>`, `<algorithm>`,
  `<optional>`, `<variant>`, `<span>`, ...; no iostreams), compiled with
  `-fno-exceptions -fno-rtti -std=gnu++23`.
- the default z80-elf linker script (program at 0x100).

The emulator fills memory with 0xA5 before loading the program, so a missing
`.bss` clear or a read of uninitialised memory shows up as a failure.

Functions under test are marked `__attribute__ ((noipa))` so that GCC really
calls them instead of folding the whole program into a constant.

## Calling convention (as implemented by the backend)

| what                          | where                                   |
|-------------------------------|-----------------------------------------|
| 1st 8/16-bit argument         | `L` / `HL`                              |
| 2nd 8/16-bit argument         | `E` / `DE`                              |
| other arguments, varargs      | pushed right to left, 2-byte slots      |
| stack cleanup                 | caller                                  |
| 8 / 16 / 32-bit return value  | `L` / `HL` / `DE:HL` (DE = low word)    |
| preserved by callee           | `IX` (frame pointer)                    |
