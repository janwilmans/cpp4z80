/* hello_write.com: 619 bytes.

   The program itself is 19 bytes of code and 21 bytes of text.  The rest
   is the standard start-up and system call code that -specs=msxdos.specs
   links in:
     233  __msxdos_args: builds argc / argv from the command line
     186  write (): the BDOS console output, "\n" -> CR LF conversion
      67  _start (crt0): sets the stack, clears .bss, runs constructors
      66  __msxdos_exit, _exit
      16  __msxdos_bdos
   (plus .data: the stdio structures write () does not use but the
   start-up code still references.)  The tiny/ hello.com needs 35.  */

#include <unistd.h>

int main ()
{
  static const char msg[] = "Hello from MSX-DOS!\n";
  write (1, msg, sizeof msg - 1);
  return 0;
}
