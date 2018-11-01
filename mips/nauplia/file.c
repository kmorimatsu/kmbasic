/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"

FSFILE* g_fhandle;
char* g_fbuff;

int init_file(char* buff,char* appname){
	// Open file
	g_fhandle=FSfopen(appname,"r");
	if (!g_fhandle) {
		return -1;
	}
	g_fbuff=buff;
	g_line=0;
	g_fileline=0;
	return 0;
}

char* compile_file(){
	int i,size;
	int in_string;
	char* err;
	
	in_string=0;

	// Read first 512 bytes (all uppercase).
	size=FSfread((void*)&g_fbuff[0],1,512,g_fhandle);
	for(i=0;i<512;i++){
		if ('a'<=g_fbuff[i] && g_fbuff[i]<='z' && !in_string) g_fbuff[i]+='A'-'a';
		if (g_fbuff[i]=='"') in_string=1-in_string;
		if (g_fbuff[i]==0x0a || g_fbuff[i]==0x0d) in_string=0;
	}
	while (size==512) {
		while(g_srcpos<256){
			err=compile_line();
			if (err) return err;
		}
		// Shift buffer and source position 256 bytes.
		for(i=0;i<256;i++) g_fbuff[i]=g_fbuff[i+256];
		g_srcpos-=256;
		// Read next 256 line (all uppercase).
		size=256+FSfread((void*)&g_fbuff[256],1,256,g_fhandle);
		for(i=256;i<512;i++){
			if ('a'<=g_fbuff[i] && g_fbuff[i]<='z' && !in_string) g_fbuff[i]+='A'-'a';
			if (g_fbuff[i]=='"') in_string=1-in_string;
			if (g_fbuff[i]==0x0a || g_fbuff[i]==0x0d) in_string=0;
		}
	}
	// Return code at the end
	g_source[size]=0x0d;
	// Compile last few lines.
	while(g_srcpos<size-1){
		err=compile_line();
		if (err) return err;
	}
	// Add "END" statement.
	g_source="END\n";
	g_srcpos=0;
	err=compile_line();
	if (err) return err;
	g_srcpos=-1;
	// No error occured
	return 0;
}

