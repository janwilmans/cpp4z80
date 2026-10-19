/* Minimal system layer for newlib on the test machine.

   The emulator in tests/run_tests.py captures every byte written to I/O
   port 1 as program output.  The heap starts at the end of the program
   (_end) and grows towards the stack.

   newlib is configured with MISSING_SYSCALL_NAMES, so it calls write(),
   sbrk() etc.; the underscore names are provided as aliases.  */

#ifdef __cplusplus
extern "C" {
#endif

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;

#define CONSOLE_PORT 1

static void
console_putc (char c)
{
  __asm__ volatile ("out (%0),a" : : "i" (CONSOLE_PORT), "a" (c));
}

int
write (int fd, const void *buf, int len)
{
  const char *p = (const char *) buf;
  (void) fd;
  for (int i = 0; i < len; i++)
    console_putc (p[i]);
  return len;
}

int
read (int fd, void *buf, int len)
{
  (void) fd, (void) buf, (void) len;
  return 0;
}

int
close (int fd)
{
  (void) fd;
  return -1;
}

_off_t
lseek (int fd, _off_t offset, int whence)
{
  (void) fd, (void) offset, (void) whence;
  return 0;
}

int
fstat (int fd, struct stat *st)
{
  (void) fd;
  st->st_mode = S_IFCHR;
  return 0;
}

int
isatty (int fd)
{
  (void) fd;
  return 1;
}

int
getpid (void)
{
  return 1;
}

int
kill (int pid, int sig)
{
  (void) pid, (void) sig;
  errno = EINVAL;
  return -1;
}

/* The linker script symbol _end (C symbols get a "_" prefix).  */
extern char program_end[] __asm__ ("_end");

void *
sbrk (ptrdiff_t increment)
{
  static char *brk = program_end;
  char *old = brk;
  char sp_probe;

  /* Keep at least 256 bytes between the heap and the stack.  */
  if (brk + increment > &sp_probe - 256)
    {
      errno = ENOMEM;
      return (void *) -1;
    }
  brk += increment;
  return old;
}

#ifdef __cplusplus
#define ALIAS(name)
#else
#define ALIAS(name) __typeof__ (name) _##name __attribute__ ((alias (#name)))
#endif
ALIAS (write);
ALIAS (read);
ALIAS (close);
ALIAS (lseek);
ALIAS (fstat);
ALIAS (isatty);
ALIAS (getpid);
ALIAS (kill);
ALIAS (sbrk);

#ifdef __cplusplus
}
#endif
