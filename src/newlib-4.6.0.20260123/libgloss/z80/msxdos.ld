/* Linker script for MSX-DOS (and CP/M 2.2) .com programs: everything in
   one block from 0x100, the load address of a .com file.  Turn the ELF
   output into a .com file with "z80-elf-objcopy -O binary".  */

OUTPUT_FORMAT ("elf32-z80")
ENTRY (_start)

SECTIONS
{
  . = 0x100;
  .text : {
    KEEP (*(.init))		/* crt0-msxdos.o: must be first */
    *(.text.unlikely .text.*_unlikely .text.unlikely.*)
    *(.text.startup .text.startup.*)
    *(.text .text.*)
  }
  .rodata : { *(.rodata .rodata.*) }
  .init_array : {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT_BY_INIT_PRIORITY (.init_array.*)))
    KEEP (*(.init_array))
    PROVIDE_HIDDEN (__init_array_end = .);
  }
  .data : { *(.data .data.*) }
  __bss_start = .;
  .bss : { *(.bss .bss.*) *(COMMON) }
  _end = .;
  PROVIDE (end = .);
  /DISCARD/ : {
    *(.comment) *(.note .note.*) *(.fini_array .fini_array.*)
    *(.dtors .dtors.*) *(.eh_frame .eh_frame.*)
  }
}
