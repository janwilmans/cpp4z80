# Building MSX-DOS `.com` programs from C and C++

This guide shows how to install the cpp4z80 toolchain and use it to turn C
or C++ source files into `.com` programs for MSX-DOS. They also run under
CP/M 2.2, since they only use CP/M-compatible BDOS calls (plus MSX-DOS 2's
exit code).

```sh
z80-elf-g++ -Os -fno-exceptions -fno-rtti -specs=msxdos.specs main.cpp -o example.elf
z80-elf-objcopy -O binary example.elf example.com
```

## 1. Installing the toolchain

### Requirements

A Linux system (or WSL) with the usual tools for building GCC. On
Debian/Ubuntu:

```sh
sudo apt install build-essential libgmp-dev libmpfr-dev libmpc-dev \
                 texinfo flex bison python3 python3-venv curl git
```

Building everything takes roughly 30–60 minutes and about 6 GB of disk
space.

### Build and install

From the root of the repository:

```sh
scripts/setup-test-env.sh       # 1. binutils (assembler, linker) for z80-elf,
                                #    plus a Python venv with a Z80 emulator
                                #    for the test suite
src/gcc-16.2.0/build.sh         # 2. configure and build the compiler
scripts/build-toolchain.sh      # 3. build libgcc, newlib (C library with the
                                #    MSX-DOS support) and the C++ library,
                                #    and install everything
```

Everything is installed into the `install/` directory of the repository.
Add its `bin` directory to your `PATH`:

```sh
export PATH="$PWD/install/bin:$PATH"     # e.g. in ~/.bashrc, with the full path
z80-elf-g++ --version
```

Optionally, check the installation by running the test suite. It compiles
every test program at four optimisation levels and runs them in the
emulator, once on a bare machine and once as MSX-DOS `.com` programs:

```sh
tests/run_tests.py
tests/run_tests.py --target msxdos
```

## 2. Compiling a program

`main.cpp`:

```cpp
#include <stdio.h>

int main (int argc, char **argv)
{
  printf ("Hello from MSX-DOS!\n");
  for (int i = 1; i < argc; i++)
    printf ("argument %d: %s\n", i, argv[i]);
  return 0;
}
```

Compile and link it to an ELF file, then extract the `.com` image:

```sh
z80-elf-g++ -Os -fno-exceptions -fno-rtti -specs=msxdos.specs main.cpp -o example.elf
z80-elf-objcopy -O binary example.elf example.com
```

- `-specs=msxdos.specs` selects the MSX-DOS environment: the start-up code
  (`crt0-msxdos.o`), the linker script (`msxdos.ld`: program at 0x100) and
  the BDOS-based system calls (`libmsxdos.a`). Without it you get a program
  for a bare Z80 machine.
- `-fno-exceptions -fno-rtti` are required for C++: exceptions and RTTI are
  not supported (yet).
- `-Os` optimises for size, usually the best choice on a Z80. `-O2`
  optimises for speed.
- The ELF file keeps the symbols and debug information (useful with
  `z80-elf-objdump -d example.elf` or `z80-elf-nm`); the `.com` file is
  the raw memory image from 0x100 on.

C works the same way with `z80-elf-gcc`:

```sh
z80-elf-gcc -Os -specs=msxdos.specs main.c -o example.elf
z80-elf-objcopy -O binary example.elf example.com
```

### Projects with several files

Compile each file to an object file as usual; only the final link needs
`-specs=msxdos.specs`. A `Makefile` for all `.cpp` files in a directory:

```make
CXX      = z80-elf-g++
OBJCOPY  = z80-elf-objcopy
CXXFLAGS = -Os -std=c++20 -fno-exceptions -fno-rtti -Wall
LDFLAGS  = -specs=msxdos.specs

OBJS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

example.com: example.elf
	$(OBJCOPY) -O binary $< $@

example.elf: $(OBJS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o $@ $^

clean:
	rm -f $(OBJS) example.elf example.com
```

The object files themselves are not MSX-specific: the same `.o` files can
be linked for another environment (e.g. a ROM) with a different specs file
or linker script.

## 3. Running it

Copy `example.com` to an MSX-DOS disk (or disk image) and start it from the
MSX-DOS prompt:

```
A>EXAMPLE ONE TWO
Hello from MSX-DOS!
argument 1: ONE
argument 2: TWO
```

With the openMSX emulator, a host directory can be used as disk drive,
e.g. `openmsx -diska path/to/dir` with MSX-DOS on that disk.

## 4. What the MSX-DOS environment provides

| C / C++                        | MSX-DOS                                             |
|--------------------------------|-----------------------------------------------------|
| `stdout`, `stderr`             | console output, BDOS function 2; `\n` is written as CR LF |
| `stdin`                        | console input a line at a time, BDOS function 10 (with the usual line editing keys); a line starting with Ctrl-Z is end of file |
| `argc`, `argv`                 | the command line (`argv[0]` is `""`; MSX-DOS 1 passes the arguments in upper case) |
| return value of `main`, `exit` | MSX-DOS 2: the program's exit code (`_TERM`, function 0x62); MSX-DOS 1 / CP/M: function 0 |
| `malloc`, `new`                | heap from the end of the program up to 512 bytes below the stack |
| stack                          | starts at the top of the TPA (the BDOS address at 0x0006) |

At exit, `atexit` handlers and C++ static destructors run and stdio
buffers are flushed. That code is only linked in if the program uses it.

Available libraries: the C library (newlib, nano variant: `printf`/`scanf`
without floating point, string functions, `malloc`, `qsort`,
`setjmp`/`longjmp`, ...), the math library (`-lm`), and the freestanding
C++ standard library (`<array>`, `<algorithm>`, `<optional>`, `<variant>`,
`<span>`, `<string_view>`, `<ranges>`, ...; no iostreams or `std::string`).

### Not supported yet

- **Files.** Only the console is available: `fopen` and friends do not work
  (BDOS file functions are not connected yet).
- **BIOS calls.** Calling the MSX BIOS from MSX-DOS needs inter-slot calls
  (`CALSLT`), which are not wrapped yet. Inline assembly works.
- **Size.** The program and its data must fit in the TPA (from 0x100 to
  the BDOS, typically about 54 KB).

## 5. Code size

The C library is linked only as far as it is used. Approximate sizes of
`.com` files built with `-Os`:

| program                             | size     |
|-------------------------------------|----------|
| `int main () { return 0; }`         | 0.4 KB   |
| output with `write (1, ...)`        | 0.6 KB   |
| output with `puts`                  | 13 KB    |
| output with `printf`                | 16 KB    |

stdio (`puts`, `printf`, ...) brings in buffering and `malloc`. For small
programs, `write (1, buf, len)` from `<unistd.h>` is much smaller. To see
where the space goes, link with `-Wl,-Map,example.map`.
