/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include "main.h"

int mulInt(int b, int a){
	return a*b;
}

int divInt(int b, int a){
	return a/b;
}

int modInt(int b, int a){
	return a%b;
}
// g_tempStr and g_temp161 are used here.
char* initStr(){
	g_tempStr=allocateMemory(81);
	g_tempStr[0]=0x0D;
	g_temp161=80;
	return g_tempStr;
}
void addStr(char* str2, char* str1){
	while (str1[0]!=0x0D) str1++;
	while((str1[0]=str2[0])!=0x0D) {
		if (!g_temp161) {
			// String too long.
			str1[0]=0x0D;
			break;
		}
		g_temp161--;
		str1++;
		str2++;
	}
}
void afterStr(int* var){
	char* dest;
	if (*var==0) {
		*var=(int)g_tempStr;
	} else {
		freeMemory(g_tempStr);
		dest=(char*)(*var);
		while((dest[0]=g_tempStr[0])!=0x0D){
			dest++;
			g_tempStr++;
		}
	}
}

void listCode(unsigned int from, unsigned int to){
	unsigned int sourcePos, lineNum;
	unsigned char i,b;
	sourcePos=g_sourceMemory;
	while (sourcePos<g_lastMemory) {
		lineNum=((unsigned int*)sourcePos)[0];
		if (from<=lineNum && lineNum<=to) {
			printUnsignedDec(lineNum);
			printChar(' ');
			i=0;
			sourcePos+=5;
			while (b=((char*)sourcePos++)[0]) {
				if (b!=0x0D) ((char*)g_strBuff)[i++]=b;
			}
			((char*)g_strBuff)[i]=0x0D;
			printStr(g_strBuff);
			newLine();
		} else {
			sourcePos+=((char*)sourcePos)[2]+5;
		}
	}
}

void printError(char type){
	newLine();
	switch(type){
		case 0:
			return;
		case 1:
			printStr("SYNTAX ERROR (\x0D");
			printChar(source[0]);
			printChar(source[1]);
			printChar(source[2]);
			printChar(')');
			break;
		case 2:
			printStr("MEMORY FULL\x0D");
			break;
		case 3:
			printStr("NO SUCH LINE\x0D");
			break;			
		case 127:
			printStr("NOT IMPLEMENTED YET\x0D");
			break;
		default:
			printStr("ERROR \x0D");
			printUnsignedDec(type);
	}
	if (g_objPointer) {
		printStr(" IN \x0D");
		printUnsignedDec(((unsigned int*)(g_objPointer-3))[0]);
	}
	newLine();
	bell();
}

void errorAndEnd(char type) {
	printError(type);
	__asm
		JP 0x1203
	__endasm;
}

void runNext(){
	g_objPointer--;
	g_objPointer+=((unsigned char*)g_objPointer)[0]+6;
	if (g_objPointer<g_lastMemory) {
		__asm
			JP _runCode
		__endasm;
	} else {
		__asm
			JP 0x1203
		__endasm;
	}
}

void runCode(){
	char e;
	// Check if compiled
	if (((unsigned int*)g_objPointer)[0]==0) {
		// Compile it.
		source=g_objPointer+2;
		object=g_nextMemory;
		e=compile();
		if (e) errorAndEnd(e);
		copyByte(0xC3); // JP XXXX
		copyInt((int)runNext);
		((unsigned int*)g_objPointer)[0]=g_nextMemory;
		g_nextMemory=(unsigned int)object;
	}
	__asm
		LD HL,#_g_objPointer
		LD E,(HL)
		INC HL
		LD H,(HL)
		LD L,E
		LD E,(HL)
		INC HL
		LD D,(HL)
		PUSH DE
		RET
	__endasm;
}

void goTo(unsigned int addr){
	if (addr==0) errorAndEnd(3);
	g_objPointer=addr;
	__asm
		POP IX // IX is pushed at the beginning
		POP HL // Remove return address that won't be used
		POP HL // Remove stack used for addr
		JP _runCode
	__endasm;
}

/*int getRand(){
	static long seed;
	seed=seed*1103515245+12345;
	return ((int)seed)>>1;
}*/
void getRand(){
	__asm
		LD HL,(#_g_seed)
		LD B,#15
		
	loop:
		LD A,H
		RLA    // bit 14 of HL will be in bit 7 of A
		XOR H  // XOR of bits 15 and 14 of HL will be in bit 7 of A
		RLA    // XOR of bits 15 and 14 of HL will be in carry bit
		RLA    // XOR of bits 15 and 14 of HL will be in bit 0 of A
		LD D,A // Store XOR value in D register
		LD A,H // bit 12 of HL will be in bit 4 of A
		RRA    // bit 12 of HL will be in bit 3 of A
		XOR L  // XOR of bits 12 and 3 of HL will be in bit 3 of A
		RRA    // XOR of bits 12 and 3 of HL will be in bit 2 of A
		RRA    // XOR of bits 12 and 3 of HL will be in bit 1 of A
		RRA    // XOR of bits 12 and 3 of HL will be in bit 0 of A
		XOR D
		AND #0x01
		ADD HL,HL
		OR L
		LD L,A
		DJNZ loop
		
		LD (#_g_seed),HL
		LD A,H
		AND #0x7F
		LD D,A
		LD E,L
	__endasm;
}

void saveToTape(){
	char* atrb=0x10f0;
	unsigned int* info=0x1102;
	unsigned char i;
	memcpy(atrb,"\x02KMB-FILE VER 1.0\x0D",18);
	info[0]=g_lastMemory-g_sourceMemory; // size
	info[1]=g_sourceMemory;              // data address
	info[2]=0;                           // execution address
	for(i=0x18;i<0x80;i++){
		atrb[i]=0;
	}
	__asm
		CALL #0x0021
		CALL #0x0024
	__endasm;
}

void loadFromTape(){
	char* atrb=0x10f0;
	unsigned int* info=0x1102;
	unsigned char i;
	while(1) {
		__asm
			CALL #0x0027
		__endasm;
		for(i=0;i<9;i++){
			if (atrb[i]!="\x02KMB-FILE"[i]) break;
		}
		doEvents();
		if (8<i) break;
	}
	// Show info
	newLine();
	printStr("LOADING \x0D");
	printUnsignedDec(info[0]);
	printStr(" BYTES \x0D");
	newLine();
	// Prepare to load
	clearMemory();
	g_sourceMemory=g_lastMemory;
	if (g_lastMemory-g_firstMemory<info[0]) errorAndEnd(2); // Not enough memory to load
	g_sourceMemory=g_lastMemory-info[0];
	info[1]=g_sourceMemory;
	// Load data
	__asm
		CALL #0x002A
		JP 0x1203
	__endasm;
}
