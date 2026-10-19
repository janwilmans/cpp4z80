/* MSX-DOS (and CP/M 2.2) system call for newlib: write.

   stdin, stdout and stderr are the console.  Output turns "\n" into
   "\r\n".  Other files are not supported (yet).  */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;

/* BDOS function numbers.  */
#define BDOS_CONOUT 2

extern int __msxdos_bdos (int function, int de);

static void
console_out (char c)
{
  __msxdos_bdos (BDOS_CONOUT, (unsigned char) c);
}

int
write (int fd, const void *buf, int len)
{
  const char *p = (const char *) buf;

  if (fd != 1 && fd != 2)
    {
      errno = EBADF;
      return -1;
    }
  for (int i = 0; i < len; i++)
    {
      if (p[i] == '\n')
	console_out ('\r');
      console_out (p[i]);
    }
  return len;
}

/* newlib calls the names without underscore (MISSING_SYSCALL_NAMES);
   provide the underscore variants too.  */
__typeof__ (write) _write __attribute__ ((alias ("write")));
