/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include "main.h"

// Library follows
int valInt(char* str){
	int i;
	if (str[0]=='-') {
		str++;
		getInt(str, &i);
		return 0-i;
	} else {
		getInt(str, &i);
		return i;
	}
}
char* decStr(int num){
	char* str;
	if (0<=num) {
		str=uint2dec((unsigned int)num);
	} else {
		num=0-num;
		str=uint2dec((unsigned int)num);
		str--;
		str[0]='-';
	}
	return str;
}
char* hexStr(unsigned int num){
	char* str;
	memcpy(g_strBuff,"0000\x0D",5);
	str=&g_strBuff[4];
	do {
		str--;
		str[0]="0123456789ABCDEF"[(num & 0x0F)];
		num=num>>4;
	} while(num);
	return str;
}
char* substr1(int pos, char* str){
	if (pos<0) {
		while (str[0]!=0x0D) str++;
	}
	return &str[pos];
}
char* substr2(int len, int pos, char* str){
	memcpy(g_strBuff, str, 81);
	str=g_strBuff;
	if (pos<0) {
		while (str[0]!=0x0D) str++;
	}
	str=&str[pos];
	pos=0;
	if (len<0) {
		while (str[pos]!=0x0D) pos++;
	}
	len+=pos;
	if (len<0 || (unsigned int)(&str[len])<(unsigned int)(&g_strBuff[0])) {
		return "\x0D";
	} else if ((unsigned int)(&str[len])<=(unsigned int)(&g_strBuff[81])) {
		str[len]=0x0D;
	}
	return str;
}

// Function construction routines follow

char funcVal(){
	compileStr();
	// PUSH DE; CALL valInt; LD D,H; LD E,L; POP HL;
	copyCode("\xD5\xCD\x34\x12\x54\x5D\xE1",7);
	((int*)object)[1]=(int)valInt;
	object+=7;
	return 0;
}

char funcInput(){
	// Call $0003 and remove left part (determined from X cursor position)
	// of return value
	// LD A,(1171); LD DE,g_strBuff; LD L,A; LD H,0;
	// ADD HL,DE; CALL $0003; LD D,H; LD E,L;
	copyCode("\x3A\x71\x11\x11\x34\x12\x6F\x26\x00\x19\xCD\x03\x00\x54\x5D",15);
	((int*)object)[2]=(int)(&g_strBuff[0]);
	object+=15;
	return 0;
}

char funcInkey(){
	// CALL $001B; LD E,A; LD D,0x00
	copyCode("\xCD\x1b\x00\x5F\x16\x00",7);
	object+=7;
	return 0;
}

char funcDec(){
	compileInt();
	// DEC$(num)
	// PUSH DE; CALL decStr; LD D,H; LD E,L; POP HL;
	copyCode("\xD5\xCD\x34\x12\x54\x5D\xE1",7);
	((int*)object)[1]=(int)decStr;
	object+=7;
	return 0;
}

char funcChr(){
	compileInt();
	// LD A,E; LD DE,g_tempShortStr; LD (DE),A; LD A,0x0D; INC DE; LD (DE),A; DEC DE;
	copyCode("\x7B\x11\x34\x12\x12\x3E\x0D\x13\x12\x1B",10);
	((int*)object)[1]=(int)(&g_strBuff[0]);
	object+=10;
	return 0;
}
char funcHex(){
	compileInt();
	if (skipBlank()==',') {
		source++;
		// HEX$(num,len)
		// PUSH DE; CALL hexStr; POP HL;
		copyCode("\xD5\xCD\x34\x12\xE1",5);
		((int*)object)[1]=(int)hexStr;
		object+=5;
		compileInt();
		// XOR A; LD HL,g_strShortBuff+4; SBC HL,DE; LD D,H; LD E,L;
		copyCode("\xAF\x21\x34\x12\xED\x52\x54\x5D",8);
		((int*)object)[1]=(int)(&g_strBuff[4]);
		object+=8;
	} else {
		// HEX$(num)
		// PUSH DE; CALL hexStr; LD D,H; LD E,L; POP HL;
		copyCode("\xD5\xCD\x34\x12\x54\x5D\xE1",7);
		((int*)object)[1]=(int)hexStr;
		object+=7;
	}
	return 0;
}

char funcSubStr(){
	char e;
	copyByte(0xD5); // PUSH DE
	e=compileInt();
	if (e) return e;
	if (skipBlank()==',') {
		// A$(xx,yy)
		source++;
		copyByte(0xD5); // PUSH DE
		e=compileInt();
		if (e) return e;
		// PUSH DE; CALL substr2; POP DE; POP DE; POP DE; LD D,H; LD E,L;
		copyCode("\xD5\xCD\x34\x12\xD1\xD1\xD1\x54\x5D",9);
		((int*)object)[1]=(int)substr2;
		object+=9;
	} else {
		// A$(xx)
		// PUSH DE; CALL substr1; POP DE; POP DE; LD D,H; LD E,L;
		copyCode("\xD5\xCD\x34\x12\xD1\xD1\x54\x5D",8);
		((int*)object)[1]=(int)substr1;
		object+=8;
	}
	return 0;
}
char funcPeek(){
	char e;
	e=compileInt();
	if (e) return e;
	copyCode("\x62\x6B\x5E\x16\x00",5); // LD H,D; LD L,E; LD E,(HL) LD D,00;
	object+=5;
	return 0;
}
char funcRnd(){
	copyByte(0xCD); // CALL XXXX
	copyInt((int)getRand);
	return 0;
}
char funcAbs(){
	char e;
	e=compileInt();
	if (e) return e;
	// LD A,80; AND D; JR Z,end:; XOR A; LD H,A; LD L,A; SBC HL,DE; LD D,H; LD E,L; end:
	copyCode("\x3E\x80\xA2\x28\x07\xAF\x67\x6F\xED\x52\x54\x5D",12);
	object+=12;
	return 0;
}
char funcAsc(){
	char e;
	e=compileStr();
	if (e) return e;
	// LD A,(DE); LD E,A; LD D,00
	copyCode("\x1A\x5F\x16\x00",4);
	object+=4;
	return 0;
}
char funcLen(){
	char e;
	e=compileStr();
	if (e) return e;
	// LD L,00; loop: LD A,(DE); CP 0D; JR Z,end:; INC DE; INC L; JR loop:; end: LD E,L; LD D,00;
	copyCode("\x2E\x00\x1A\xFE\x0D\x28\x04\x13\x2C\x18\xF7\x5D\x16\x00",14);
	object+=14;
	return 0;
}
char funcSgn(){
	char e;
	e=compileInt();
	if (e) return e;
	// LD A,D; OR E; JR Z,end:; LD A,80; AND D; LD DE,0001; JR Z,end:; DEC DE; DEC DE; end:
	copyCode("\x7A\xB3\x28\x0A\x3E\x80\xA2\x11\x01\x00\x28\x02\x1B\x1B",14);
	object+=14;
	return 0;
}
char funcNot(){
	char e;
	e=compileInt();
	if (e) return e;
	// LD A,D; OR E; LD DE,0001; JR Z,end:; DEC DE; end:
	copyCode("\x7A\xB3\x11\x01\x00\x28\x01\x1B",8);
	object+=8;
	return 0;
}
char funcStrncmp(){
	char e;
	e=compileStr();
	if (e) return e;
	if (skipBlank()!=',') return 1;
	source++;
	//push de;LD BC,0051;CALL 120f(allocateMemory with BC);pop hl;push de;ldir
	copyCode("\xD5\x01\x51\x00\xCD\x0F\x12\xE1\xD5\xED\xB0",11);
	object+=11;
	e=compileStr();
	if (e) return e;
	if (skipBlank()!=',') return 1;
	source++;
	//push de
	copyByte(0xD5);
	e=compileInt();
	if (e) return e;
	//pop bc;pop hl;push de;push bc;push hl;CALL 1215(freeMemory with HL);ld d,h;lde,l;pop hl;push de;CALL 1212;pop de;pop hl;pop hl
	copyCode("\xC1\xE1\xD5\xC5\xE5\xCD\x15\x12\x54\x5D\xE1\xD5\xCD\x12\x12\xD1\xE1\xE1",18);
	object+=18;
	return 0;
}

char* funcList(){
	__asm
		ld hl,#flist
		ret
		flist:
		.dw #_funcPeek
		.ascii "PEEK"
		.db 0x00
		.dw #_funcRnd
		.ascii "RND"
		.db 0x00
		.dw #_funcVal
		.ascii "VAL"
		.db 0x00
		.dw #_funcAbs
		.ascii "ABS"
		.db 0x00
		.dw #_funcAsc
		.ascii "ASC"
		.db 0x00
		.dw #_funcLen
		.ascii "LEN"
		.db 0x00
		.dw #_funcSgn
		.ascii "SGN"
		.db 0x00
		.dw #_funcNot
		.ascii "NOT"
		.db 0x00
		.dw #_funcInkey
		.ascii "INKEY"
		.db 0x00
		.dw #_funcStrncmp
		.ascii "STRNCMP"
		.db 0x00
		.dw 0x0000
	__endasm;
	return 0;
}

char* strFuncList(){
	__asm
		ld hl,#sflist
		ret
		sflist:
		.dw #_funcChr
		.ascii "CHR$"
		.db 0x00
		.dw #_funcHex
		.ascii "HEX$"
		.db 0x00
		.dw #_funcDec
		.ascii "DEC$"
		.db 0x00
		.dw #_funcInput
		.ascii "INPUT$"
		.db 0x00
		.dw 0x0000
	__endasm;
	return 0;
}
char compileFuncSub(char* slist){
	char e;
	char (*sfunc)();
	sfunc=seekList(slist);
	if (!sfunc) return 1;
	if (skipBlank()!='(') return 1;\
	source++;
	e=sfunc();
	if (e) return e;
	if (skipBlank()!=')') return 1;
	source++;
	return 0;
}
char compileIntFunc(){
	return compileFuncSub(funcList());
}
char compileStrFunc(){
	return compileFuncSub(strFuncList());
}
