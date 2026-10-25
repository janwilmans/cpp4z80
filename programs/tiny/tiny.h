/* Minimal MSX-DOS / CP/M runtime for freestanding programs: no C library,
   no start-up code.  A .com file is loaded at 0x100 and entered there, so
   main () is the entry point (tiny.ld puts it first) and "return" is a
   plain RET to the warm-boot address the CCP left on the stack.

   Nothing is cleared or initialised: no .bss clearing, no constructors, no
   argc/argv.  Use only what main () itself needs.  The BDOS is assumed to
   preserve IX and IY (true for CP/M and MSX-DOS 1; the programs here never
   keep anything in them across a call).  */

#ifndef TINY_H
#define TINY_H

#ifdef __cplusplus
extern "C" {
#endif

/* BDOS function 2: console output of one character. */
static inline void putc_ (char c)
{
  int de = (unsigned char) c;
  __asm__ volatile ("ld c,2\n\tcall 5"
		    : "+d" (de) : : "a", "b", "c", "h", "l", "memory", "cc");
}

/* BDOS function 9: print a '$'-terminated string. */
static inline void puts_ (const char *s)
{
  __asm__ volatile ("ld c,9\n\tcall 5"
		    : "+d" (s) : : "a", "b", "c", "h", "l", "memory", "cc");
}

/* BDOS function 10: read a line into BUF (buf[0] = capacity, buf[1] = the
   length read, text from buf[2]). */
static inline void readline_ (unsigned char *buf)
{
  __asm__ volatile ("ld c,10\n\tcall 5"
		    : "+d" (buf) : : "a", "b", "c", "h", "l", "memory", "cc");
}

/* Print VALUE in decimal.  The quotient by 10 is found by repeated
   subtraction (slow for big numbers, but libgcc's divide is much larger). */
static void putu_ (unsigned value)
{
  unsigned quotient = 0;
  while (value >= 10)
    {
      value -= 10;
      quotient++;
    }
  if (quotient)
    putu_ (quotient);
  putc_ ('0' + value);
}

/* The command line tail the CCP leaves at 0x80: length, then the text
   (starting with a space), terminated by CR. */
#define CMDLINE_LEN (*(volatile unsigned char *) 0x80)
#define CMDLINE_TEXT ((const char *) 0x81)

#ifdef __cplusplus
}
#endif

#endif
