/*
*  link.ld
    We need an explicit linker to link our assembly and C program together, for what runs?
    I tried running gcc compilation of kernel.c but there's no main function to start the compilationfrom
*/
OUTPUT_FORMAT(elf32-i386)
/*
output format of our output executable to be 32 bit Executable and Linkable Format (ELF).
 ELF is the standard binary file format for Unix-like systems on x86 architecture.
*/
ENTRY(start) /* symbol name which is the start of my executable - written in assembly */
/* Sections here defines the layout of our executable */
SECTIONS
 {
   . = 0x100000; /* location counter - our executable's memory starts from here*/
   .text : { *(.text) } /* Any text/input section from all input files - probably source code? (ends in .txt) */
   .data : { *(.data) }
   .bss  : { *(.bss)  }
 }