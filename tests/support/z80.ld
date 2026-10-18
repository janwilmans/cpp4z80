/* Flat 64K memory image for the test programs: code from address 0,
   followed by read-only data, data and bss.  The stack starts at the top
   of memory (see crt0.s).  */

ENTRY(_start)

SECTIONS
{
  . = 0x0000;
  .text   : { *(.text.crt0) *(.text .text.*) }
  .rodata : { *(.rodata .rodata.*) }
  .data   : { *(.data .data.*) }
  .bss    : { *(.bss .bss.*) *(COMMON) }
  /DISCARD/ : { *(.comment) *(.note.*) }
}
