/* Program termination for MSX-DOS (and CP/M 2.2) programs, called by
   crt0-msxdos.S with main's result.

   This does what exit () does - run the atexit handlers (and C++ static
   destructors) and flush stdio - but refers to those parts of newlib
   weakly, so that a program that does not use them does not pull in
   stdio, malloc and so on just to be able to exit.  */

typedef void (*__msxdos_handler) (void);

extern void __call_exitprocs (int, void *) __attribute__ ((weak));
extern __msxdos_handler __stdio_exit_handler __attribute__ ((weak));
extern void _exit (int status) __attribute__ ((noreturn));

void
__msxdos_exit (int status)
{
  if (__call_exitprocs)
    __call_exitprocs (status, 0);
  if (&__stdio_exit_handler && __stdio_exit_handler)
    __stdio_exit_handler ();
  _exit (status);
}
