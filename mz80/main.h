/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include <stdio.h>
#include <string.h>

#include "macros.h"

// Global variables.
// Note that only 56 bytes are available for global variables.
// Do not initialize global variables here but do it in init() or main() function.
// "volatile const" variables will be embed in code area after $1200,
// so these variables would be preserved in MONITOR program,
// but the other variables will be disrupted.
#ifdef MAIN
	volatile const char g_strBuff[]=
		"0---------------1---------------2---------------3---------------"
		"4---------------5---------------6---------------7---------------";
	volatile const char g_variables[]="AABBCCDDEEFFGGHHIIJJKKLLMMNNOOPPQQRRSSTTUUVVWWXXYYZZ";
	volatile const unsigned int g_firstMemory=0, g_lastMemory=0, g_nextMemory=0, g_sourceMemory=0;
	unsigned int g_objPointer, g_ifElseJump, g_seed;
	unsigned int g_temp161, g_temp162;
	char* g_tempStr;
	char* source;
	char* object;
#else
	extern const char g_strBuff[];
	extern const char g_variables[];
	extern const unsigned int g_firstMemory, g_lastMemory, g_nextMemory, g_sourceMemory;
	extern unsigned int g_objPointer, g_ifElseJump, g_seed;
	extern unsigned int g_temp161, g_temp162;
	extern char* g_tempStr;
	extern char* source;
	extern char* object;
#endif

// Macros to turn (const unsigned int) to (unsigned int)
#define g_firstMemory (((unsigned int*)(&g_firstMemory))[0])
#define g_lastMemory (((unsigned int*)(&g_lastMemory))[0])
#define g_nextMemory (((unsigned int*)(&g_nextMemory))[0])
#define g_sourceMemory (((unsigned int*)(&g_sourceMemory))[0])

// memory.c
void memoryError();
void clearMemory();
char* allocateMemory(int len);
void freeMemory(char* back);

// bios.c
void doEvents();
char shiftBreak();
char* getInt(char *source, int* result);
char* uint2dec(unsigned int value);
void printUnsignedDec(unsigned int value);
void printDec(int value);
void printStr(char* str);
void printChar(char value);
void printHex16(unsigned int value);
void printHex8(unsigned char value);
char callCode(int address);

// compiler.c
void copyCode(char* code, int len);
void copyByte(char b);
void copyInt(int i);
char command(char* str);
char skipBlank();
void* seekList(char* slist);
char compile();
char compileStr();
char compileInt();

// functions.c
char funcSubStr();
char compileIntFunc();
char compileStrFunc();

// editor.c
void newCode();
char addCode();
unsigned int getDecimal();

// libs.c
int mulInt(int b, int a);
int divInt(int b, int a);
int modInt(int b, int a);
char* initStr();
void addStr(char* str2, char* str1);
void afterStr(int* var);
void listCode(unsigned int from, unsigned int to);
void printError(char type);
void runCode();
void goTo(unsigned int addr);
void getRand();
void saveToTape();
void loadFromTape();
