/* MSX-DOS (and CP/M 2.2) system call for newlib: sbrk.  The heap starts
   at the end of the program and grows towards the stack.  */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;

extern char __msxdos_end[] __asm__ ("_end");

void *
sbrk (ptrdiff_t increment)
{
  static char *brk = __msxdos_end;
  char *old = brk;
  char sp_probe;

  /* Keep at least 512 bytes between the heap and the stack.  */
  if (brk + increment > &sp_probe - 512 || brk + increment < __msxdos_end)
    {
      errno = ENOMEM;
      return (void *) -1;
    }
  brk += increment;
  return old;
}

/* newlib calls the names without underscore (MISSING_SYSCALL_NAMES);
   provide the underscore variants too.  */
__typeof__ (sbrk) _sbrk __attribute__ ((alias ("sbrk")));
