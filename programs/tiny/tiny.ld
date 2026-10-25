/* Linker script for tiny .com programs without start-up code: main () is
   placed first, at 0x100, which is where a .com file is entered. */
OUTPUT_FORMAT ("elf32-z80")
ENTRY (_main)

SECTIONS
{
  . = 0x100;
  .text : {
    *(.text.startup .text.startup.*)	/* main */
    *(.text .text.*)
    *(.rodata .rodata.*)
    *(.data .data.*)
  }
  /* No .bss: nothing clears it.  Use initialised or stack data only. */
  /DISCARD/ : {
    *(.comment) *(.note .note.*) *(.eh_frame .eh_frame.*)
    *(.bss .bss.*) *(COMMON)
  }
}
