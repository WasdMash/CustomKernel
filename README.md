## Requirements
To run this, we will need
* NASM
* A C/C++ compiler (like cc or GCC - I'm using GCC)
* GRUB (comes with basically every Linux distibution)

## Building source code
We will now create object files from kernel.asm and kernel.c and then link it using our linker script.

* nasm -f elf32 kernel.asm -o kasm.o

will run the assembler to create the object file kasm.o in ELF-32 bit format.

* gcc -m32 -c kernel.c -o kc.o

The ’-c ’ option makes sure that after compiling, linking doesn’t implicitly happen.

* ld -m elf_i386 -T link.ld -o kernel-<version> kasm.o kc.o

will run the linker with our linker script and generate the executable named kernel.

### Setup
After building the source code, you need to move the final executable (preferable naming convention be `kernel-<version>`) into your /boot folder
* Easily done on Linux by doing `sudo mv kernel-<version> /boot`

Then, open the grub.cfg in /boot/grub and add the following at the bottom of the file:

menuentry '<whatever name you want>' {
	set root='hd0,msdos1'
	multiboot /boot/kernel-<version> ro
}

By convention, I named my first iteration `kernel-001` where `001` is the `<version>`

To run this kernel, simply reboot your PC and choose `<whatever name you want>` in the dropdown