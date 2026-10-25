/* hello_puts.com: 13232 bytes.  Only ~20 bytes of it is this program.

   puts () goes through newlib's stdio, which buffers its output and
   allocates the buffer with malloc.  The big pieces:
    2.7 KB  __sfvwrite_r: the buffered write
    2.0 KB  __free_r
    1.5 KB  __sflush_r
    1.1 KB  __malloc_r
    0.7 KB  __swsetup_r, __smakebuf_r: set up the stdout buffer
    0.6 KB  __swrite, __sread, __read, __fwalk_sglue (flush at exit)
    0.7 KB  start-up and exit code (see hello_write.cpp)
   plus smaller pieces (memmove, sbrk, realloc, ...).  The ELF's debug
   sections are not part of the .com file.

   Writing the same text with write () (hello_write.cpp) takes 619 bytes,
   with the BDOS directly (tiny/hello.c) 35.  */

#include <stdio.h>

int main ()
{
  puts ("Hello from MSX-DOS!");
  return 0;
}
