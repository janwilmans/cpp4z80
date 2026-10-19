/* MSX-DOS (and CP/M 2.2) system calls for newlib: close, lseek, fstat,
   isatty, getpid, kill.  Only the console (file descriptors 0, 1 and 2)
   is supported.  */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;

extern void _exit (int status);

static int
is_console (int fd)
{
  return fd >= 0 && fd <= 2;
}

int
close (int fd)
{
  return is_console (fd) ? 0 : (errno = EBADF, -1);
}

_off_t
lseek (int fd, _off_t offset, int whence)
{
  (void) fd, (void) offset, (void) whence;
  errno = ESPIPE;
  return -1;
}

int
fstat (int fd, struct stat *st)
{
  if (!is_console (fd))
    {
      errno = EBADF;
      return -1;
    }
  st->st_mode = S_IFCHR;
  return 0;
}

int
isatty (int fd)
{
  return is_console (fd);
}

int
getpid (void)
{
  return 1;
}

int
kill (int pid, int sig)
{
  if (pid == 1)
    _exit (128 + sig);
  errno = EINVAL;
  return -1;
}

/* newlib calls the names without underscore (MISSING_SYSCALL_NAMES);
   provide the underscore variants too.  */
__typeof__ (close) _close __attribute__ ((alias ("close")));
__typeof__ (lseek) _lseek __attribute__ ((alias ("lseek")));
__typeof__ (fstat) _fstat __attribute__ ((alias ("fstat")));
__typeof__ (isatty) _isatty __attribute__ ((alias ("isatty")));
__typeof__ (getpid) _getpid __attribute__ ((alias ("getpid")));
__typeof__ (kill) _kill __attribute__ ((alias ("kill")));
