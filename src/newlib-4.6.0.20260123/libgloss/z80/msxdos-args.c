/* Command line arguments for MSX-DOS (and CP/M 2.2) programs.  The
   command tail is at 0x80: a length byte followed by the characters (CP/M
   and MSX-DOS 1 convert them to upper case).  argv[0] is "" (the program
   name is not available).  Called by crt0-msxdos.S.  */

#define MAX_ARGS 16

char **__msxdos_argv;

int
__msxdos_args (void)
{
  static char tail[128];
  static char *argv[MAX_ARGS + 1];
  static char empty[] = "";
  const volatile unsigned char *cmd = (const volatile unsigned char *) 0x80;
  unsigned char len = cmd[0];
  int argc = 0;

  if (len > 127)
    len = 127;
  for (unsigned char i = 0; i < len; i++)
    tail[i] = cmd[1 + i];
  tail[len] = 0;

  argv[argc++] = empty;
  for (char *p = tail; *p && argc < MAX_ARGS;)
    {
      while (*p == ' ' || *p == '\t')
	*p++ = 0;
      if (!*p)
	break;
      argv[argc++] = p;
      while (*p && *p != ' ' && *p != '\t')
	p++;
    }
  argv[argc] = 0;
  __msxdos_argv = argv;
  return argc;
}
