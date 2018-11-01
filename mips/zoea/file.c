/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"

static FSFILE* g_fhandle;
static char* g_fbuff;
static int g_size;

char* init_file(char* buff,char* appname){
	// Open file
	g_fhandle=FSfopen(appname,"r");
	if (!g_fhandle) {
		return ERR_UNKNOWN;
	}
	g_fbuff=buff;
	g_line=0;
	g_fileline=0;
	return 0;
}

void close_file(){
	FSfclose(g_fhandle);
}

void read_file(int blocklen){
	int i;
	static char in_string, escape;
	// blocklen is either 512 or 256.
	if (blocklen==512) {
		// This is first read. Initialize parameter(s).
		in_string=0;
		escape=0;
	} else if (g_size<512) {
		// Already reached the end of file.
		return;
	} else {
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
	}
	// Read 512 or 256 bytes from SD card.
	g_size=512-blocklen+FSfread((void*)&g_fbuff[512-blocklen],1,blocklen,g_fhandle);
	// Some modifications of text for easy compiling.
	for(i=512-blocklen;i<512;i++){
		if (in_string) {
			if (g_fbuff[i]=='\\' && !escape) {
				escape=1;
			} else {
				escape=0;
				if (g_fbuff[i]=='"') in_string=0;
			}
		} else {
			// If not in string, all upper cases.
			if (g_fbuff[i]=='"') in_string=1;
			else if ('a'<=g_fbuff[i] && g_fbuff[i]<='z') g_fbuff[i]+='A'-'a';
			// If not in string, tabs will be spaces.
			else if ('\t'==g_fbuff[i]) g_fbuff[i]=' ';
		}
		if (g_fbuff[i]==0x0a || g_fbuff[i]==0x0d) in_string=escape=0;
	}
	return;
}

char* compile_file(){
	int i;
	char* err;
	// Read first 512 bytes
	read_file(512);
	// Compile line by line
	while (g_size==512) {
		err=compile_line();
		if (err) return err;
		// Maintain at least 256 characters in cache.
		if (256<=g_srcpos) read_file(256);
	}
	// Return code at the end
	g_source[g_size]=0x0d;
	// Compile last few lines.
	while(g_srcpos<g_size-1){
		err=compile_line();
		if (err) return err;
	}
	// Add "DATA 0" and "END" statements.
	g_source="DATA 0:END\n";
	g_srcpos=0;
	err=compile_line();
	if (err) return err;
	g_srcpos=-1;
	// No error occured
	return 0;
}

