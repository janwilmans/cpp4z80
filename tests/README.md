# Z80 backend tests

Every file in `c/` and `cpp/` is a complete program. It is compiled with the
freshly built cross compiler at `-O0`, `-O1`, `-O2` and `-Os`, assembled and
linked with the z80-elf binutils, and executed in an emulator (the `z80`
Python package). The value returned by `main()` (read from HL after the
program halts) must equal the value in the file's `// expect:` line.

```sh
scripts/setup-test-env.sh      # once: binutils + emulator venv
scripts/make-gcc.sh            # rebuild the compiler after backend changes
tests/run_tests.py             # run everything
tests/run_tests.py -v -O 0 tests/c/007_stack_args.c   # one test, show asm
```

Build products (assembly, map files, binaries) go to `work/test-out/`.

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
