/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

/*
	Use following command to build:
	sdcc *.rel -mz80 --code-loc 0x1200 --data-loc 0x1000 --no-std-crt0 -Wlcrt\crt.asm.o

	Note that only 56 bytes are available in data area, starting from 0x1000.
	For char[], use constant as it will be embed in code area.
*/
#define MAIN
#include "main.h"

int lastMem(){
	__asm
		push af
		push bc
		ld hl,#0xd000
	loop_lastmem:
		dec hl
		ld a,(hl)
		ld b,a
		cpl
		ld (hl),a
		ld a,(hl)
		ld (hl),b
		cp a,b
		jr z,loop_lastmem
		pop bc
		pop af
		ret
	__endasm;
	return 0xcfff;
}

void init(void){
	// Starting message
	newLine();
	printStr("BASIC KM-1013\x0D");
	newLine();
	// Determine the first address of available area.
	if (!g_firstMemory) {
		// The address 0x1102 contains total file size loaded from CMT
		g_temp161=0x1102;
		g_firstMemory=0x1200+((unsigned int*)g_temp161)[0];
	}
	// Determine the last address (+1) of memory.
	g_lastMemory=lastMem();
	printHex16(g_firstMemory);
	printConstChar(0x2D); //'-'
	printHex16(g_lastMemory);
	newLine();
	g_lastMemory++;
	printUnsignedDec(g_lastMemory-g_firstMemory);
	printStr(" BYTES FREE\x0D");
	newLine();
	// Destroy program and clear all variables
	newCode();
	return;
}

char inputLine(){
	char* buff=source;
	char pos,len,temp;
	newLine();
	__asm
		LD DE,(#_source)
		CALL #0x0003
	__endasm;
	// "***" -> "***\x0D", "\x0D" -> "\x00" conversion
	for (len=0;buff[len]!=0x0D;len++);
	buff[len]=0x00;
	if (80<len) return 2;
	for (pos=0;pos<len;pos++) {
		if (buff[pos]!='"') continue;
		if (79<len) return 2;
		pos++;
		while (buff[pos]!='"') {
			pos++;
			if (len<=pos) return 1;
		}
		for (temp=len+1;pos<temp;temp--) buff[temp]=buff[temp-1];
		buff[pos]=0x0D;
		pos+=2;
		len++;
	}
	return 0;
}
void main(void){
	char e;
	char* tempObject;
	while(1){
		while(1){
			source=0x11A3;
			e=inputLine();
			if (e) break;
			if ( shiftBreak() ) continue;
			e=skipBlank();
			if ('0'<=e && e<='9') {
				// Editing mode
				e=addCode();
				if (e) break;
			} else {
				// Direct execution of a statement
				g_objPointer=0;
				object=tempObject=g_nextMemory;
				e=compile();
				if (e) break;
				copyByte(0xC9); // ret
				g_nextMemory=(int)object;
				e=callCode((int)tempObject);
				if (e) break;
				if (g_nextMemory==(int)object) {
					// Destroy compiled code though new object is not attached.
					// Otherwise, memory is allocated to string etc.
					// You can go back to original position using clearMemory() function.
					g_nextMemory=(int)tempObject;
				}
			}
		}
		printError(e);
	}
}

