# Sample MSX-DOS programs

```sh
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=z80-toolchain.cmake
cmake --build build
cmake --build build --target sizes      # size of every .com
cmake --build build --target run-hello  # run one in the emulator
```

| program | what | bytes |
|---|---|---|
| `tiny/nop.c` | does nothing | 4 |
| `tiny/hello.c` | hello world, BDOS 9 | 35 |
| `tiny-cpp/hello.cpp` | hello with `std_print ("...")`, `$` added at compile time | 34 |
| `tiny-cpp/hello_udl.cpp` | the same with `std_print ("..."_s)`, no macro | 34 |
| `tiny/echo.c` | prints the command line | 83 |
| `tiny/upper.c` | reads a line, prints it in upper case | 153 |
| `tiny/countdown.cpp` | C++ template, decimal output | 153 |
| `tiny/fib.c` | Fibonacci numbers up to 16 bits | 175 |
| `newlib/hello_write.cpp` | hello via `write()` | 619 |
| `newlib/hello_puts.cpp` | hello via `puts()` | 13232 |
| `newlib/hello_printf.cpp` | hello via `printf()` | 15866 |

`tiny/` programs are freestanding: no C library and no start-up code
(`-nostdlib -nostartfiles`, linker script `tiny/tiny.ld`, BDOS calls in
`tiny/tiny.h`). `main` sits at 0x100 and returns with a plain `ret`.
There is no `.bss` clearing, no constructors and no argc/argv.
The `newlib/` programs use `-specs=msxdos.specs` and are here for comparison.

Size flags: `-Os -flto -fomit-frame-pointer`, no unwind tables, no RTTI,
`--gc-sections`.
