/************************
*  KM-BASIC for KMZ-80  *
*       Katsumi         *
* License: LGPL ver 2.1 *
*************************/

#include "main.h"

void newCode(){
	// Set start point of source to last of memory
	g_sourceMemory=g_lastMemory;
	// Clear all variables
	clearMemory();
}

unsigned int getDecimal(){
	char b;
	int i=0;
	while(1) {
		b=source[0];
		if (b<'0' || '9'<b) break;
		source++;
		i*=10;
		i+=b-'0';
	}
	return i;
}

/*	Format of a line.
+0     LSB address in source
+1     MSB address in source
+2     source length (n)
+3     LSB address of object code
+4     MSB address of object code
+5     source begin

+n+4   source end (must end with '0x00')

+n+5  next line
*/
char addCode(){
	char e;
	unsigned int lineNum, sourceStart, sourceLen, lineLen, lastSourceStart;
	// Clear memory to forget everything with allocated memory regions.
	clearMemory();
	// Determine line number
	e=skipBlank();
	if (e<'0' || '9'<e) return 1;
	lineNum=getDecimal();
	skipBlank();
	// Check syntax
	sourceStart=(unsigned int)source;
	object=g_nextMemory;
	e=compile();
	if (e) return e;
	// Determine source length and go back to original source position.
	sourceLen=(unsigned int)source-sourceStart+1;
	source=(char*)sourceStart;
	// Check if available memory.
	if (g_sourceMemory-g_firstMemory<sourceLen+5) return 2;
	// Delete the line with same number
	sourceStart=g_sourceMemory;
	while (sourceStart<g_lastMemory) {
		lineLen=((unsigned char*)sourceStart)[2]+5;
		if (((unsigned int*)sourceStart)[0]==lineNum) {
			memmove((void*)(g_sourceMemory+lineLen), (void*)g_sourceMemory, sourceStart-g_sourceMemory);
			g_sourceMemory+=lineLen;
			break;
		}
		sourceStart+=lineLen;
	}
	// Return if source code is null (the command is "delete a line").
	if (sourceLen<2) return 0;
	// Alocate area for a line
	if (g_sourceMemory==g_lastMemory) {
		// The first line of code.
		g_sourceMemory-=sourceLen+5;
		sourceStart=g_sourceMemory;
	} else {
		// Make a space for new line
		sourceStart=g_sourceMemory;
		while (sourceStart<g_lastMemory) {
			lineLen=((unsigned char*)sourceStart)[2]+5;
			if (lineNum<((unsigned int*)sourceStart)[0]) break;
			sourceStart+=lineLen;
		}
		memmove((void*)(g_sourceMemory-sourceLen-5), (void*)g_sourceMemory, sourceStart-g_sourceMemory);
		g_sourceMemory-=sourceLen+5;
		sourceStart-=sourceLen+5;
	}
	// Insert a line
	((unsigned int*)sourceStart)[0]=lineNum;
	sourceStart+=2;
	((char*)sourceStart)[0]=(char)sourceLen;
	((char*)sourceStart)[1]=0;
	((char*)sourceStart)[2]=0;
	sourceStart+=3;
	memcpy((char*)sourceStart,source,sourceLen);
	return 0;
}


