/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include "main.h"

void doEvents(){
	// Check BREAK key
	__asm
		CALL #0x001E
		JR NZ,skip1
		JP 0x1203
		skip1:
	__endasm;
}

char shiftBreak(){
	__asm
		CALL #0x001E
		LD L,#0x00
		RET NZ
	__endasm;
	return 1;
}

char* getInt(char *source, int* result){
	int i;
	char b;
	if (source[0]=='$') {
		// Hexadecimal
		i=0;
		while(1){
			source++;
			b=source[0];
			if ('0'<=b && b<='9') {
				b=b-'0';
			} else if ('A'<=b && b<='F') {
				b=b-'A'+10;
			} else {
				break;;
			}
			i<<=4;
			i+=b;
		}
	} else {
		// Decimal
		for (i=0;'0'<=source[0] && source[0]<='9';source++) {
			i=i*10+(int)(source[0]-'0');
		}
	}
	result[0]=i;
	return source;
}

char* uint2dec(unsigned int value){
	unsigned char* buff=&g_strBuff[1];
	unsigned char b;
	for (b=0x30;9999<value;b++) value-=10000;
	buff[0]=b;
	for (b=0x30;999<value;b++) value-=1000;
	buff[1]=b;
	for (b=0x30;99<value;b++) value-=100;
	buff[2]=b;
	for (b=0x30;9<value;b++) value-=10;
	buff[3]=b;
	buff[4]=0x30+value;
	buff[5]=0x0D;
	for(b=0;b<4;b++){
		if (0x30<buff[b]) break;
	}
	return (char*)(&buff[b]);
}

void printUnsignedDec(unsigned int value){
	printStr(uint2dec(value));
}
void printDec(int value){
	if (value<0) {
		printConstChar(0x2D); //'-'
		value=-value;
	}
	printStr(uint2dec(value));
}
void printStr(char* str){
	__asm
		LD D,5(ix)
		LD E,4(ix)
		CALL #0x0015
		CALL #0x001E
		CALL Z,#0x1203
	__endasm;
}
void printChar(char value){
	__asm
		LD A,4(ix)
		CALL #0x0012
	__endasm;
}
void printHex16(unsigned int value){
	__asm
		LD H,5(ix)
		LD L,4(ix)
		CALL #0x03BA
	__endasm;
}
void printHex8(unsigned char value){
	__asm
		LD A,4(ix)
		CALL #0x03C3
	__endasm;
}

char callCode(int address){
	__asm
		jr skip2
		jump:
		jp (HL)
		skip2:
		LD H,5(ix)
		LD L,4(ix)
		call jump
	__endasm;
	return 0;
}
