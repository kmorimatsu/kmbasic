
#define def_sharp #

#define newLine() __asm CALL 0x0009 __endasm

#define printConstChar(x)\
	__asm\
		LD A,def_sharp x\
		CALL 0x0012\
	__endasm

#define bell() __asm CALL 0x003E __endasm

