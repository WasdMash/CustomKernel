;;kernel.asm

;This is not an x86 assembly instruction. It’s a directive to the NASM assembler 
;that specifies it should generate code to run on a processor operating in 32
;bit mode. It is not mandatorily required in our example, however is
;included here as it’s good practice to be explicit.

; We start our code now
bits 32			;nasm directive - 32 bit
section .text ; Just as in our linker file, this will be compiled under .text
        ;multiboot spec - dd defines adouble word size of 4 bytes
        align 4
        dd 0x1BADB002            ;magic - identifies the header
        dd 0x00                  ;flags
        dd - (0x1BADB002 + 0x00) ;checksum. magic+flangs+checksum should be zero

;These are gonna get exported into my C file to be used
global start
global keyboard_handler
global read_port
global write_port
global load_idt

; global is another NASM directive to set symbols from source code as global.
; By doing so, the linker knows where the symbol start is; which happens to be our entry point.
extern kmain	        ;kmain, our function will be defined in a kernel.c file externally

;Gotta handle read/write operations now in order for keyboard to be functional now
;Gonna pass in the port number as an argument somewhere
read_port:
	mov edx, [esp + 4] ;edx = lower 16 bits from data register
	in al, dx	         ;al = lower 8 bits of accumulator register
	ret

write_port:
	mov   edx, [esp + 4]    
	mov   al, [esp + 4 + 4]  
	out   dx, al  
	ret


load_idt:
	mov edx, [esp + 4]
	lidt [edx]
	sti 				;turn on interrupts
	ret

keyboard_handler:                 
	call    keyboard_handler_main
	iretd

; Start function - gonna use it to call kmain here
start:
  cli 			;block interrupts
  mov esp, stack_space	;set stack pointer (esp)
  call kmain
  hlt		 	;halt the CPU

section .bss ;Should probably look back to my SCC.131 notes to understand stack and heap memory and all that
resb 8192		;8KB for stack
stack_space: