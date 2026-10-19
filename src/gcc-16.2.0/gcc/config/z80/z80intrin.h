/* Z80 intrinsics.
   Copyright (C) 2026 Free Software Foundation, Inc.

   This file is part of GCC.

   GCC is free software; you can redistribute it and/or modify it
   under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   Under Section 7 of GPL version 3, you are granted additional
   permissions described in the GCC Runtime Library Exception, version
   3.1, as published by the Free Software Foundation.  */

/* Access to the Z80 operations that C cannot express: I/O ports,
   interrupt control and HALT.  */

#ifndef _Z80INTRIN_H
#define _Z80INTRIN_H

/* Read from / write to the I/O port PORT.  The whole 16-bit port number is
   put on the address bus ("in r,(c)" / "out (c),r" with PORT in BC).  */

static __inline__ __attribute__ ((__always_inline__)) unsigned char
z80_in (unsigned int __port)
{
  unsigned char __value;
  __asm__ __volatile__ ("in\t%0,(c)" : "=r" (__value) : "b" (__port));
  return __value;
}

static __inline__ __attribute__ ((__always_inline__)) void
z80_out (unsigned int __port, unsigned char __value)
{
  __asm__ __volatile__ ("out\t(c),%1" : : "b" (__port), "r" (__value));
}

/* The same for a constant 8-bit port number ("in a,(n)" / "out (n),a",
   which put A on the high half of the address bus).  */

#define Z80_IN(PORT)							\
  (__extension__ ({							\
    unsigned char __z80_value;						\
    __asm__ __volatile__ ("in\ta,(%1)" : "=a" (__z80_value)		\
			  : "n" ((unsigned char) (PORT)));		\
    __z80_value;							\
  }))

#define Z80_OUT(PORT, VALUE)						\
  __asm__ __volatile__ ("out\t(%0),a" : : "n" ((unsigned char) (PORT)),	\
			"a" ((unsigned char) (VALUE)))

/* Interrupt control.  These are also compiler memory barriers.  */

static __inline__ __attribute__ ((__always_inline__)) void
z80_di (void)
{
  __asm__ __volatile__ ("di" : : : "memory");
}

static __inline__ __attribute__ ((__always_inline__)) void
z80_ei (void)
{
  __asm__ __volatile__ ("ei" : : : "memory");
}

/* Select interrupt mode 0, 1 or 2.  */
#define z80_im(MODE) __asm__ __volatile__ ("im\t%0" : : "n" (MODE))

static __inline__ __attribute__ ((__always_inline__)) void
z80_halt (void)
{
  __asm__ __volatile__ ("halt" : : : "memory");
}

static __inline__ __attribute__ ((__always_inline__)) void
z80_nop (void)
{
  __asm__ __volatile__ ("nop");
}

#endif /* _Z80INTRIN_H */
