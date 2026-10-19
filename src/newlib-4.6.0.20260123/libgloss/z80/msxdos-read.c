/* MSX-DOS (and CP/M 2.2) system call for newlib: read.

   Console input is read a line at a time with BDOS function 10 (buffered
   line input, with the usual editing keys); a line starting with Ctrl-Z
   is end of file.  */

#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#undef errno
extern int errno;

/* BDOS function numbers.  */
#define BDOS_CONOUT 2
#define BDOS_BUFIN 10

extern int __msxdos_bdos (int function, int de);

static void
console_out (char c)
{
  __msxdos_bdos (BDOS_CONOUT, (unsigned char) c);
}

/* The current input line, returned by read () until it is used up.  */
static unsigned char line[130];
static unsigned char line_pos, line_len;

int
read (int fd, void *buf, int len)
{
  char *p = (char *) buf;
  int n = 0;

  if (fd != 0)
    {
      errno = EBADF;
      return -1;
    }

  if (line_pos == line_len)
    {
      /* BDOS buffered input: byte 0 = buffer size, byte 1 = characters
	 read, then the characters (without the final CR).  */
      line[0] = 127;
      line[1] = 0;
      __msxdos_bdos (BDOS_BUFIN, (int) line);
      console_out ('\r');
      console_out ('\n');
      if (line[1] > 0 && line[2] == 0x1a)
	return 0;		/* Ctrl-Z: end of file */
      line_len = line[1] + 1;
      line[2 + line[1]] = '\n';
      line_pos = 0;
    }

  while (n < len && line_pos < line_len)
    p[n++] = line[2 + line_pos++];
  return n;
}

/* newlib calls the names without underscore (MISSING_SYSCALL_NAMES);
   provide the underscore variants too.  */
__typeof__ (read) _read __attribute__ ((alias ("read")));
