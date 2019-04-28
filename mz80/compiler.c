/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include "main.h"

void checkCodeMemory(int len) {
	if (g_sourceMemory<=len+(int)object) memoryError();
}

void copyCode(char* code, int len){
	checkCodeMemory(len+2);
	memcpy(object,code,len);
}

void copyByte(char b){
	checkCodeMemory(1);
	object[0]=b;
	object++;
}

void copyInt(int i){
	checkCodeMemory(1);
	((int*)object)[0]=i;
	object+=2;
}

char command(char* str){
	int len;
	for(len=0;str[len];len++);
	if (strncmp(source,str,len)) return 0;
	source+=len;
	return 1;
}
char skipBlank(){
	while (source[0]==' ') source++;
	return source[0];
}
char compileStr(){
	//Prepare a code to set the pointer to DE register.
	char b;
	b=skipBlank();
	if (b=='"') {
		source++;
		copyByte(0x11); // LD DE,XXXX
		copyInt((int)source);
		while(source[0]!='"') source++;
		source++;
	} else if (source[1]=='$') {
		source[0];// This line is required (don't know why).
		source++;
		source++;
		if (b<'A' || 'Z'<b) return 1;
		copyCode("\xED\x5B",2); // LD DE,(XXXX)
		((int*)object)[1]=(int)(&g_variables)+2*(int)(b-'A');
		object+=4;
		if (skipBlank()=='(') {
			// A$(xx) or A$(xx,yy) (substring function)
			source++;
			b=funcSubStr();
			if (b) return b;
			if (skipBlank()!=')') return 1;
			source++;
			return 0;
		}
	} else {
		// Functions
		return compileStrFunc();
	}
	return 0;
}
char compileIntSub(){
	char b;
	int i;
	// Value will be in DE.
	b=skipBlank();
	if (b=='(') {
		source++;
		b=compileInt();
		if (b) return b;
		if (skipBlank()!=')') return 1;
		source++;
	} else if (b=='-') {
		// Minus value.  Put 0 to DE.
		// Do not increment the pointer to source, as it will be used in compileInt().
		copyCode("\x11\x00\x00",3); // LD DE,0x0000
		object+=3;
	} else if (b=='$' || ('0'<=b && b<='9')) {
		// Hexadecimal or decimal
		source=getInt(source,&i);
		copyByte(0x11); // LD DE,XXXX
		copyInt(i);
	} else if ('A'<=b && b<='Z') {
		i=(int)(&g_variables)+2*(int)(b-'A');
		b=source[1];
		if (b=='(') {
			// Dimension
			source+=2;
			b=compileInt();
			if (b) return b;
			if (skipBlank()!=')') return 1;
			source++;
			// LD HL,(i); ADD HL,DE; ADD HL,DE; LD E,(HL); INC HL; LD D,(HL);
			copyCode("\x2A\x00\x00\x19\x19\x5E\x23\x56",8);
			object++;
			((int*)object)[0]=i;
			object+=7;
		} else if (b<'A' || 'Z'<b) {
			// Integer variables
			copyCode("\xED\x5B",2); // LD DE,(XXXX)
			((int*)object)[1]=i;
			object+=4;
			source++;
		} else {
			// Functions
			b=compileIntFunc();
			if (b) return b;
		}
	} else return 1;
	return 0;
}

char compileInt(){
	char b;
	char op;
	// Get left value to DE
	b=compileIntSub();
	if (b) return b;
	do {
		// Get operator
		op=skipBlank();
		switch(op){
			case '+': case '-': case '*': case '/': case '%': case '=':
				source++;
				break;
			case 'A':
				if (source[1]!='N' || source[2]!='D') return 0;
				source+=3;
				break;
			case 'O':
				if (source[1]!='R') return 0;
				source+=2;
				break;
			case 'X':
				if (source[1]!='O' || source[2]!='R') return 0;
				source+=3;
				break;
			case '!':
				if (source[1]!='=') return 0;
				source+=2;
				break;
			case '<':
				source++;
				if (source[0]=='=') {
					source++;
					op='(';
				}
				break;
			case '>':
				source++;
				if (source[0]=='=') {
					source++;
					op=')';
				}
				break;
			default: // Operator not found. Let's return the value (supporsed to be in DE)
				return 0;
		}
		// Preserve current DE in stack
		copyByte(0xD5); // PUSH DE
		// Get right value to DE
		b=compileIntSub();
		if (b) return b;
		// Caluculate
		switch(op){
			case '+':
				// POP HL; ADD HL,DE
				copyCode("\xE1\x19",2);
				object+=2;
				break;
			case '-':
				// POP HL; XOR A; SBC HL,DE
				copyCode("\xE1\xAF\xED\x52",4);
				object+=4;
				break;
			case '*':
				// PUSH DE; CALL mulInt; POP DE; POP DE
				copyCode("\xD5\xCD\x00\x00\xD1\xD1",6);
				((int*)object)[1]=(int)mulInt;
				object+=6;
				break;
			case '/':
				// PUSH DE; CALL divInt; POP DE; POP DE
				copyCode("\xD5\xCD\x00\x00\xD1\xD1",6);
				((int*)object)[1]=(int)divInt;
				object+=6;
				break;
			case '%':
				// PUSH DE; CALL ModInt; POP DE; POP DE
				copyCode("\xD5\xCD\x00\x00\xD1\xD1",6);
				((int*)object)[1]=(int)modInt;
				object+=6;
				break;
			case 'A':
				// POP HL; LD A,H; AND D; LD H,A; LD A,L; AND E; LD L,A
				copyCode("\xE1\x7C\xA2\x67\x7D\xA3\x6F",7);
				object+=7;
				break;
			case 'O':
				// POP HL; LD A,H; OR D; LD H,A; LD A,L; OR E; LD L,A
				copyCode("\xE1\x7C\xB2\x67\x7D\xB3\x6F",7); 
				object+=7;
				break;
			case 'X':
				// POP HL; LD A,H; XOR D; LD H,A; LD A,L; XOR E; LD L,A
				copyCode("\xE1\x7C\xAA\x67\x7D\xAB\x6F",7);
				object+=7;
				break;
			case '=':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JR NZ,skip:; inc HL; skip:
				copyCode("\xE1\xAF\xED\x52\x67\x6F\x20\x01\x23",9);
				object+=9;
				break;
			case '!':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JRNZ,skip:; inc HL; skip:
				copyCode("\xE1\xAF\xED\x52\x67\x6F\x28\x01\x23",9);
				object+=9;
				break;
			case '<':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JP P,skip:; INC HL; skip:
				copyCode("\xE1\xAF\xED\x52\x67\x6F\xF2\x0A\x00\x23",10);
				object+=7;
				((int*)object)[0]=(int)object+3;
				object+=3;
				break;
			case '>':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JP M,skip:; JR Z,skip:; INC HL; skip:
				copyCode("\xE1\xAF\xED\x52\x67\x6F\xFA\x0C\x00\x28\x01\x23",12);
				object+=7;
				((int*)object)[0]=(int)object+5;
				object+=5;
				break;
			case '(':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JR Z,skip1: JP P,skip:2; skip1: INC HL; skip:2
				copyCode("\xE1\xAF\xED\x52\x67\x6F\x28\x03\xF2\x0A\x00\x23",12);
				object+=9;
				((int*)object)[0]=(int)object+3;
				object+=3;
				break;
			case ')':
				// POP HL; XOR A; SBC HL,DE; LD H,A; LD L,A; JP M,skip:; INC HL; skip:
				copyCode("\xE1\xAF\xED\x52\x67\x6F\xFA\x0A\x00\x23",10);
				object+=7;
				((int*)object)[0]=(int)object+3;
				object+=3;
				break;
			default:
				return 127;
		}
		copyByte(0xEB); // EX HL,DE
		// Seek the next operator
	} while(1);
}
char compilePrint(){
	char b;
	char cr=1;
	do {
		switch ( skipBlank() ) {
			case ':':
			case 0x00:
				break;
			case '"':
				cr=1;
				source++;
				copyCode("\x11\x00\x00\xCD\x09\x12",6); // LD DE,nn; CALL 0x1209
				object++;
				((int*)object)[0]=(int)source;
				object+=5;
				while (source[0]!='"') {
					if (source[0]==0x00) return 1;
					source++;
				}
				source++;
				continue;
			case ';':
				cr=0;
				source++;
				continue;
			case ',':
				cr=0;
				source++;
				copyCode("\xCD\x0F\x00",3); // CALL 0x000F
				object+=3;
				continue;
			case 'E':
				if (!strncmp(source,"ELSE ",5)) break;
			default:
				cr=1;
				if ('A'<=source[0] && source[0]<='Z') {
					for (b=1;source[b]!='$';b++) {
						if ('A'<=source[b] && source[b]<='Z') continue;
						b=0;
						break;
					}
				} else b=0;
				if (b) {
					// String
					b=compileStr();
					if (b) return b;
					copyCode("\xCD\x09\x12",3); // CALL 0x1209
					object+=3;
				} else {
					// Integer
					b=compileInt();
					if (b) return b;
					copyCode("\xD5\xCD\x00\x00\xD1",5); // PUSH DE; CALL printDec; POP DE
					((int*)object)[1]=(int)printDec;
					object+=5;
				}
				continue;
		}
		if (cr) {
			copyCode("\xCD\x06\x00",3); // CALL 0x0006
			object+=3;
		}
		return 0;
	} while (1);
}
char compileBye(){
	copyCode(
		"\x31\xF0\x11" // LD SP,0x11F0
		"\xC3\x82\x00" // JP 0x0082
		,6);
	object+=6;
	return 0;
}
char compileEnd(){
	copyCode(
		"\xC3\x03\x12" // JP 0x1203
		,3);
	object+=3;
	return 0;
}
char compileNew(){
	copyCode(
		"\xC3\x00\x12" // JP 0x1200
		,3);
	object+=3;
	return 0;
}

char compileLet(){
	char b,e;
	int variableAddress;
	// Seek the name of variable (either A, B, ... or Z)
	e=skipBlank();
	if (e<'A' || 'Z'<e) return 1;
	source++;
	// Determine the address of variable
	variableAddress=(int)(&g_variables[(e-'A')*2]);
	// Determine if string
	b=source[0];
	if (b=='$') {
		source++;
	} else if (b=='(') {
		source++;
		// Dimension
		e=compileInt();
		if (e) return e;
		if (skipBlank()!=')') return 1;
		source++;
		copyByte(0xD5); // PUSH DE
	} else {
		b=0;
	}
	// Check "="
	if (skipBlank()!='=') return 1;
	source++;
	if (b=='$') {
		// String
		// LD HL,variableAddress; PUSH HL; CALL initStr; PUSH HL
		copyCode("\x21\x34\x12\xE5\xCD\x34\x12\xE5",8);
		object++;
		((int*)object)[0]=variableAddress;
		((int*)object)[2]=(int)initStr;
		object+=7;
		while(1) {
			e=compileStr();
			if (e) return e;
			// PUSH DE; CALL addStr; POP DE
			copyCode("\xD5\xCD\x34\x12\xD1",5);
			((int*)object)[1]=(int)addStr;
			object+=5;
			if (skipBlank()!='+') break;
			source++;
		}
		// POP HL; CALL afterStr; POP HL
		copyCode("\xE1\xCD\x34\x12\xE1",5);
		((int*)object)[1]=(int)afterStr;
		object+=5;
	} else if (b=='(') {
		// Dimension
		e=compileInt();
		if (e) return e;
		// POP BC; LD HL,(variableAddress); ADD HL,BC; ADD HL,BC; LD (HL),E; INC HL; LD (HL),D;
		copyCode("\xC1\x2A\x00\x00\x09\x09\x73\x23\x72",9);
		((int*)object)[1]=variableAddress;
		object+=9;
	} else {
		// Integer
		// Get value in DE register
		e=compileInt();
		if (e) return e;
		// Put value to memory
		copyCode("\xED\x53",2); // LD (XXXX),DE
		((int*)object)[1]=variableAddress;
		object+=4;
	}
	return 0;
}

void setLineNum(){
	// Set g_objPointer
	copyCode("\x21\x34\x12\x22",4);
	((unsigned int*)object)[2]=(unsigned int)(&g_objPointer);
	object++;
	((unsigned int*)object)[0]=g_objPointer;
	object+=5;
}
/*	FOR A=1 TO XX STEP YY
	, where "A=1" is any initialization statement, XX and YY are any values.
	"STEP YY" may be omitted, meaning "STEP 1"
	Object code will be:
	(initialization)
	(put XX values to DE)
	PUSH DE
	(put YY values to DE) // Maybe omitted.
	LD (yyyy),DE // See 7 lines below. Maybe omitted.
	POP HL
	ADD HL,DE
	LD (xxxx),HL // See 8 lines below
	JR skip:
	// Process comes here when "NEXT" statement is executed.
	LD HL,(nn) // initialized variable
	LD DE,0x0001 or LD DE,YY (modified above)
	ADD HL,DE
	LD (nn),HL // initialized variable
	XOR A // reset C flag
	LD DE,XX (modified above)
	SBC HL,DE
	RET Z
	POP HL
	skip:
	LD HL,nn // The address of 10 lines above
	PUSH HL
*/
char compileFor(){
	char e;
	int variableAddress, *XX, *YY;
	// init statement
	e=skipBlank();
	variableAddress=(int)(&g_variables[(e-'A')*2]);
	e=compileLet();
	if (e) return e;
	// TO statement
	if (strncmp(source,"TO ",3)) return 1;
	source+=3;
	e=compileInt();
	if (e) return e;
	// STEP statement
	skipBlank();
	if (!strncmp(source,"STEP ",5)) {
		source+=5;
		copyByte(0xD5); // PUSH DE
		e=compileInt();
		if (e) return e;
		copyCode("\xED\x53\x00\x00\xE1\x19",6); // LD (yyyy),DE; POP HL; ADD HL,DE
		YY=(int*)(object+2); // See below
		object+=6;
	} else {
		// STEP 1
		YY=0;
		copyCode("\xEB\x23",2); // EX DE,HL; INC HL
		object+=2;
	}
	copyByte(0x22); // LD (xxxx),HL
	XX=(int*)object; // See below
	object+=2;
	// FOR statement main routine follows.
	copyCode(
		"\x18\x12"     // JR skip:
		"\x2A\x34\x12" // LD HL,(1234)
		"\x11\x01\x00" // LD DE,0001
		"\x19"         // ADD HL,DE
		"\x22\x34\x12" // LD (1234),HL
		"\xAF"         // XOR A
		"\x11\x34\x12" // LD DE,XXXX
		"\xED\x52"     // SBC HL,DE
		"\xC8"         // RET Z
		"\xE1"         // POP HL
		"\x21\x34\x12" // skip: LD HL,1234
		"\xE5"         // PUSH HL
		,24);
	*XX=(int)object+14;
	if (YY) *YY=(int)object+6;
	object+=3;
	((int*)object)[0]=variableAddress;
	object+=7;
	((int*)object)[0]=variableAddress;
	object+=11;
	((int*)object)[0]=(int)object-19;
	object+=3;
	// g_objPointer will back to current line.
	setLineNum();
	return 0;
}
/*	NEXT statement
	POP HL
	JR skip:
	back:
	JP (HL)
	skip:
	CALL back:
*/
char compileNext(){
	copyCode(
		"\xE1"         // POP HL
		"\x18\x01"     // JR skip:
		"\xE9"         // back: JP (HL)
		"\xCD\x34\x12" // skip: CALL back:
		,7);
	object+=5;
	((int*)object)[0]=(int)object-2;
	object+=2;
	return 0;
}
/*
char compileDebug(){
	int objaddr;
	// Print object address
	objaddr=(int)object;
	copyCode("\x11\x00\x00\xCD\x15\x00",6); // LD DE,XXXX; CALL 0x0015
	object++;
	((int*)object)[0]=(int)"OBJECTS:\x0D";
	object+=5;
	copyCode("\x21\x00\x00\xCD\xBA\x03\xCD\x09\x00",9); // LD HL,XXXX; CALL 0x03BA; CALL 0x0009
	object++;
	((int*)object)[0]=objaddr;
	object+=8;
	// Print g_variables
	copyCode("\x11\x00\x00\xCD\x15\x00",6); // LD DE,XXXX; CALL 0x0015
	object++;
	((int*)object)[0]=(int)"VARIABLES:\x0D";
	object+=5;
	copyCode("\x21\x00\x00\xCD\xBA\x03\xCD\x09\x00",9); // LD HL,XXXX; CALL 0x03BA; CALL 0x0009
	object++;
	((int*)object)[0]=(int)(&g_variables[0]);
	object+=8;
	// Print stack pointer
	copyCode("\x11\x00\x00\xCD\x15\x00",6); // LD DE,XXXX; CALL 0x0015
	object++;
	((int*)object)[0]=(int)"STACK:\x0D";
	object+=5;
	copyCode("\xED\x73\x34\x12\x2A\x34\x12\xCD\xBA\x03\xCD\x09\x00",13); // LD (XXXX),SP; LD HL,(XXXX); CALL 0x03BA; CALL 0x0009
	((int*)object)[1]=(int)(&g_temp161);
	object++;
	((int*)object)[2]=(int)(&g_temp161);
	object+=12;
	return 0;
}//*/
char compileList(){
	char b;
	unsigned int from,to;
	from=0;
	to=65535;
	b=skipBlank();
	if ('0'<=b && b<='9') from=to=getDecimal();
	b=skipBlank();
	if (b=='-') {
		source++;
		b=skipBlank();
		if ('0'<=b && b<='9') to=getDecimal();
		else to=65535;
	}
	copyCode("\x21\x34\x12\xE5\x21\x34\x12\xE5\xCD\x34\x12\xE1\xE1",13); // LD HL,XXXX; PUSH HL; LD HL,XXXX; PUSH HL; CALL XXXX; POP HL; POP HL;
	object++;
	((unsigned int*)object)[0]=to;
	((unsigned int*)object)[2]=from;
	((unsigned int*)object)[4]=(unsigned int)listCode;
	object+=12;
	return 0;
}
char compileClear(){
	copyByte(0xCD);
	copyInt((int)clearMemory);
	return 0;
}
char compileGoto(){
	char b;
	unsigned int i, sourcePos;
	b=skipBlank();
	if (b<'0' || '9'<b) {
		i=0;
	} else {
		i=getDecimal();
	}
	sourcePos=g_sourceMemory;
	while (sourcePos<g_lastMemory) {
		if (i<=((unsigned int*)sourcePos)[0]) {
			sourcePos+=3;
			break;
		}
		sourcePos+=((unsigned char*)sourcePos)[2]+5;
	}
	if (g_lastMemory<=sourcePos) sourcePos=0;
	copyCode("\x21\x34\x12\xE5\xCD",5); // LD HL,XXXX; PUSH HL; CALL XXXX;
	object++;
	((unsigned int*)object)[0]=(unsigned int)sourcePos;
	((unsigned int*)object)[2]=(unsigned int)goTo;
	object+=6;
	return 0;
}
char compileGosub(){
	char e;
	copyCode("\xCD\x34\x12\x18\x07",5); // CALL skip:; JR ret:; skip:
	object++;
	((unsigned int*)object)[0]=(unsigned int)object+4;
	object+=4;
	e=compileGoto();
	if (e) return e;
	// g_objPointer will back to current line.
	setLineNum();
	return 0;
}
char compileRet(){
	copyByte(0xC9); // RET
	return 0;
}
char compileRun(){
	copyByte(0xCD); // CALL XXXX
	copyInt((int)clearMemory);
	return compileGoto();
}
char compileIf(){
	char e;
	e=compileInt();
	if (e) return e;
	skipBlank();
	if (strncmp(source,"THEN ",5)) return 1;
	source+=5;
	copyCode("\x7A\xB3\x20\x03\xC3",5);
	object+=7;
	g_ifElseJump=(unsigned int)object-2;
	return 0;
}
char compileElse(){
	if (!g_ifElseJump) return 1;
	((unsigned int*)g_ifElseJump)[0]=(unsigned int)object+3;
	copyByte(0xC3);
	g_ifElseJump=(unsigned int)object;
	object+=2;
	return 0;
}
char compilePoke(){
	char e;
	e=compileInt();
	if (e) return e;
	if (skipBlank()!=',') return 1;
	source++;
	copyByte(0xD5); // PUSH DE
	e=compileInt();
	if (e) return e;
	copyCode("\xE1\x73",2); // POP HL; LD (HL),E
	object+=2;
	return 0;
}
char compileCursor(){
	char e;
	e=compileInt();
	if (e) return e;
	// LD A,E; CP 0x28; JR NC,skip; LD (1171),A; skip: 
	copyCode("\x7B\xFE\x28\x30\x03\x32\x71\x11",8);
	object+=8;
	if (skipBlank()!=',') return 1;
	source++;
	e=compileInt();
	if (e) return e;
	// LD A,E; CP 0x19; JR NC,skip; LD (1171),A; skip: 
	copyCode("\x7B\xFE\x19\x30\x03\x32\x72\x11",8);
	object+=8;
	return 0;
}
char compileDim(){
	char b;
	int variableAddress;
	while(1){
		b=skipBlank();
		if (b<'A'||'Z'<b) return 1;
		if (source[1]!='(') return 1;
		source+=2;
		variableAddress=(int)(&g_variables[(b-'A')*2]);
		b=compileInt();
		if (b) return b;
		if (skipBlank()!=')') return 1;
		source++;
		// INC DE; LD H,D; LD L,E; ADD HL,DE; PUSH HL; CALL allocateMemory; POP DE;
		// LD D,H; LD E,L; LD HL,variableAddress; LD (HL),E; INC HL; LD (HL),D; 
		copyCode("\x13\x62\x6B\x19\xE5\xCD\x00\x00\xD1\x54\x5D\x21\x00\x00\x73\x23\x72",17);
		((unsigned int*)object)[3]=(unsigned int)allocateMemory;
		((unsigned int*)object)[6]=(unsigned int)variableAddress;
		object+=17;
		if (skipBlank()!=',') return 0;
		source++;
	}
}
char compileExec(){
	char b1,b2;
	while(1){
		b1=skipBlank();
		if ('0'<=b1 && b1<='9') {
			b1=b1-'0';
		} else if ('A'<=b1 && b1<='F') {
			b1=b1-'A'+10;
		} else {
			return 0;
		}
		b1<<=4;
		b2=source[1];
		if ('0'<=b2 && b2<='9') {
			b2=b2-'0';
		} else if ('A'<=b2 && b2<='F') {
			b2=b2-'A'+10;
		} else {
			return 0;
		}
		copyByte(b1+b2);
		source+=2;
	}
}
char compileSave(){
	copyByte(0xCD);
	copyInt((int)saveToTape);
	return 0;
}
char compileLoad(){
	copyByte(0xCD);
	copyInt((int)loadFromTape);
	return 0;
}

char* statementList(){
	__asm
		ld hl,#list
		ret
		list:
//		.dw #_compileDebug
//		.ascii "DEBUG"
//		.db 0x00
		.dw #_compileBye
		.ascii "BYE"
		.db 0x00
		.dw #_compileEnd
		.ascii "END"
		.db 0x00
		.dw #_compileNew
		.ascii "NEW"
		.db 0x00
		.dw #_compileRun
		.ascii "RUN"
		.db 0x00
		.dw #_compileLet
		.ascii "LET "
		.db 0x00
		.dw #_compileFor
		.ascii "FOR "
		.db 0x00
		.dw #_compileDim
		.ascii "DIM "
		.db 0x00
		.dw #_compileList
		.ascii "LIST"
		.db 0x00
		.dw #_compileNext
		.ascii "NEXT"
		.db 0x00
		.dw #_compileLoad
		.ascii "LOAD"
		.db 0x00
		.dw #_compileSave
		.ascii "SAVE"
		.db 0x00
		.dw #_compileGoto
		.ascii "GOTO "
		.db 0x00
		.dw #_compilePoke
		.ascii "POKE "
		.db 0x00
		.dw #_compileExec
		.ascii "EXEC "
		.db 0x00
		.dw #_compilePrint
		.ascii "PRINT"
		.db 0x00
		.dw #_compileClear
		.ascii "CLEAR"
		.db 0x00
		.dw #_compileRet
		.ascii "RETURN"
		.db 0x00
		.dw #_compileGosub
		.ascii "GOSUB "
		.db 0x00
		.dw #_compileCursor
		.ascii "CURSOR "
		.db 0x00
		.dw 0x0000
	__endasm;
	return 0;
}

void* seekList(char* slist){
	void* ret;
	while(1){
		// Fetch pointer to function
		ret=(void*) (((int*)slist)[0]);
		if (!ret) {
			// End of statement list (not found)
			return 0;
		}
		slist++;
		slist++;
		// slist is now pointer to statement string to check
		if (command(slist)) {
			// Statement/function found
			return ret;
		}
		// Skip current statement to check
		while(slist[0]!=0x00) slist++;
		slist++;
	}
	
}

char compile (){
	char* slist;
	char (*sfunc)();
	char e=0;
	g_ifElseJump=0;
	slist=statementList();
	while (skipBlank()!=0x00) {
		if (command("IF ")) {
			e=compileIf();
			continue;
		} else if (command("REM")) {
			// Skip until 0x00
			while(source[0]) source++;
			break;
		} else {
			sfunc=(char(*)())seekList(slist);
			if (sfunc) {
				// Statement found
				e=sfunc();
			} else {
				// Statement not found
				// Must be LET statement
				e=compileLet();
			}
		}
		if (e) return e;
		e=skipBlank();
		if (e==':') {
			source++;
			continue;
		} else if (e==0x00) {
			break;
		} else if (command("ELSE ")) {
			e=compileElse();
			if (e) return e;
		} else {
			return 1;
		}
	}
	if (g_ifElseJump) ((unsigned int*)g_ifElseJump)[0]=(unsigned int)object;
	return 0;
}
