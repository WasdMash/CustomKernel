#include "keyboard_map.h"

/* there are 25 lines each of 80 columns; each element takes 2 bytes */
#define LINES 25
#define COLUMNS_IN_LINE 80
#define BYTES_FOR_EACH_ELEMENT 2
#define SCREENSIZE BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE * LINES

/*
A device or a chip called Programmable Interrupt Controller (PIC) is responsible for x86 being an interrupt driven architecture.
It manages hardware interrupts and sends them to the appropriate system interrupt.
Without this, we'd have to keep manually checking the computer to see if something happened - polling.
*/

/*
The keyboard works through the I/O ports 0x60 and 0x64. 
Port 0x60 gives the data (pressed key) and port 0x64 gives the status. 
*/
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define IDT_SIZE 256
#define INTERRUPT_GATE 0x8e
#define KERNEL_CODE_SEGMENT_OFFSET 0x10 //upon testing in my VM, 0x10 is better. This could be hardware-specific.
//Could be 0x08 on different architecture

#define ENTER_KEY_CODE 0x1C

extern unsigned char keyboard_map[128]; //comes from my header file
//These are coming from my assembly file
extern void keyboard_handler(void);
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern void load_idt(unsigned long *idt_ptr);

/* current cursor location */
unsigned int current_loc = 0;
/* video memory begins at address 0xb8000 */
char *vidptr = (char*)0xb8000;

// Interrupt Descriptor Table (IDT) - structure that gives the interrupt handler address corresponding to the interrupt number.
struct IDT_entry{
	//The PIC has an offset value stored during initialisation of the PIC. It adds the input line number to this offset to form the Interrupt number.
	unsigned short int offset_lowerbits;
	unsigned short int selector;
	unsigned char zero;
	unsigned char type_attr;
	unsigned short int offset_higherbits;
};

struct IDT_entry IDT[IDT_SIZE];

void idt_init(void)
{
	unsigned long keyboard_address;
	unsigned long idt_address;
	unsigned long idt_ptr[2];

	//Modern x86 systems have 2 PIC chips each having 8 input lines. Let’s call them PIC1 and PIC2
	/* populate IDT entry of keyboard's interrupt */
	keyboard_address = (unsigned long)keyboard_handler; 
	IDT[0x21].offset_lowerbits = keyboard_address & 0xffff;
	IDT[0x21].selector = KERNEL_CODE_SEGMENT_OFFSET; /* KERNEL_CODE_SEGMENT_OFFSET - defined above*/
	IDT[0x21].zero = 0;
	IDT[0x21].type_attr = INTERRUPT_GATE; /* INTERRUPT_GATE - defined constant above*/
	IDT[0x21].offset_higherbits = (keyboard_address & 0xffff0000) >> 16; //bitshifts up by 16 bits by the looks of
	
	//PIC1 receives IRQ0 to IRQ7 and PIC2 receives IRQ8 to IRQ15 - IRQ means interrupt request
	/*     Ports
	*	 PIC1	PIC2
	*Command 0x20	0xA0
	*Data	 0x21	0xA1
	*/

	//The PICs are initialised using 8-bit command words known as Initialication command words (ICW). 

	/* ICW1 - begin initialisation on command ports of both PIC chips*/
	write_port(0x20 , 0x11);
	write_port(0xA0 , 0x11);

	/* ICW2 - remap offset address of IDT on data ports of both PIC chips*/
	/*
	* In x86 protected mode, we have to remap the PICs beyond 0x20 because
	* Intel have designated the first 32 interrupts as "reserved" for cpu exceptions
	*/
	//It sets the PIC’s offset value. This is the value to which we add the input line number to form the Interrupt number.
	write_port(0x21 , 0x20); //We have initialised PIC1 to an offset 0x20
	// To find interrupt number, add 1 + 0x20 ie. 0x21. So, keyboard handler address has to be mapped against interrupt 0x21 in the IDT.
	write_port(0xA1 , 0x28); //We have initialised PIC2 to an offset 0x28

	/* ICW3 - setup cascading (supposedly how PICs are wired as masters/slaves)*/
		/*
		Think of each PIC as a doorbell box with several buttons (IRQs).
		If one box doesn’t have enough buttons, you can connect a smaller box to one of the first box’s buttons. That’s cascading: the smaller box’s signals pass through the larger box.
		ICW3 tells the boxes how they’re connected:
			On the main PIC, each bit says whether a smaller PIC is connected to that IRQ line.
			On a smaller PIC, ICW3 says which connection number it uses.
		So the main PIC can tell, “That signal came from the extra box; ask it which button was pressed.”
		*/
	write_port(0x21 , 0x04); //Master: slave is connected to IRQ2
	write_port(0xA1 , 0x02); //Slave: its cascade identity is IRQ2 

	/* ICW4 - environment info (gives additional information about the environment)*/
	write_port(0x21 , 0x01);
	write_port(0xA1 , 0x01);
	/* Initialisation finished */

	//Each PIC has an internal 8 bit register named Interrupt Mask Register (IMR). 
	//This register stores a bitmap of the IRQ lines going into the PIC. When a bit is set (=1), the PIC ignores the request.
	/* mask interrupts */
	write_port(0x21 , 0xff);
	write_port(0xA1 , 0xff);

	/* fill the IDT descriptor */
	idt_address = (unsigned long)IDT ;
	idt_ptr[0] = (sizeof (struct IDT_entry) * IDT_SIZE - 1) + ((idt_address & 0xffff) << 16);
	idt_ptr[1] = idt_address >> 16 ;

	load_idt(idt_ptr);
}

void kb_init(void)
{
	/* 0xFD is 11111101 - enables only IRQ1 (keyboard)*/
	write_port(0x21 , 0xFD);
}

void kprint(const char *str)
{
	unsigned int i = 0;
	while (str[i] != '\0') {
		vidptr[current_loc++] = str[i++];
		/*
		0 - Black, 1 - Blue, 2 - Green, 3 - Cyan, 4 - Red, 5 - Magenta, 6 - Brown, 7 - Light Grey, 8 - Dark Grey, 9 - Light Blue,
		10/a - Light Green, 11/b - Light Cyan, 12/c - Light Red, 13/d - Light Magenta, 14/e - Light Brown, 15/f – White.
		*/
		vidptr[current_loc++] = 0x0b;
	}
}

void kprint_newline(void)
{
	unsigned int line_size = BYTES_FOR_EACH_ELEMENT * COLUMNS_IN_LINE;
	current_loc = current_loc + (line_size - current_loc % (line_size));
}

void clear_screen(void)
{
	unsigned int i = 0;
	while (i < SCREENSIZE) {
		vidptr[i++] = ' ';
		vidptr[i++] = 0x07; //Light grey foreground on black background
	}
}

void keyboard_handler_main(void)
{
	unsigned char status;
	char keycode;

	/* write EOI */
	write_port(0x20, 0x20);

	status = read_port(KEYBOARD_STATUS_PORT);
	/* Lowest bit of status will be set if buffer is not empty */
	if (status & 0x01) {
		keycode = read_port(KEYBOARD_DATA_PORT);
		if(keycode < 0)
			return;

		if(keycode == ENTER_KEY_CODE) {
			kprint_newline();
			return;
		}

		vidptr[current_loc++] = keyboard_map[(unsigned char) keycode];
		vidptr[current_loc++] = 0x0b; //should print a new character to screen in light cyan
	}
}

void kmain(void)
{
	const char *str = "Welcome to my first kernel with keyboard support";
	clear_screen(); //Don't want to be dealing with leftovers on the screen - get rid of that nonsense immediately
	kprint(str); //prints string and then two newlines/ENTERs
	kprint_newline();
	kprint_newline();

	idt_init();
	kb_init(); //now we are ready to start using my keyboard for basic input

	while(1); //just keeps going really
}