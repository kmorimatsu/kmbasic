/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#include <xc.h>
#include "main.h"
#include "api.h"
#include "compiler.h"
#include "editor.h"

/* Prototypes */
char* init_file(char* buff,char* appname);
char* compile_file();
void wait60thsec(unsigned short n);
extern FSFILE* g_fhandle;

char* printdec(int num){
	char str[11];
	int i;
	if (num<0) {
		printchar('-');
		num=0-num;
	}
	for(i=10;0<i;i--){
		if (num==0 && i<10) break;
		str[i]='0'+rem10_32(num);
		num=div10_32(num);
	}
	for(i++;i<11;i++) {
		printchar(str[i]);
	}
}

int runbasic(char *appname,int test){
// BASICソースのコンパイルと実行
// appname 実行するBASICソースファイル
// test 0:コンパイルと実行、0以外:コンパイルのみで終了
//
// 戻り値
//　　0:正常終了
//　　-1:ファイルエラー
//　　-2:リンクエラー
//　　1以上:コンパイルエラーの発生行（行番号ではなくファイル上の何行目か）
	int i;
	char* buff;
	char* err;

	// Set grobal pointer
	g_gp=get_gp();
	// Set source positions
	buff=(char*)&(RAM[RAMSIZE-512]);
	g_source=(char*)(&buff[0]);
	g_srcpos=0;
	// Set object positions
	g_object=(int*)(&RAM[0]);
	g_objpos=0;
	g_objmax=g_object+(RAMSIZE-512)/4; // Buffer area excluded.
	// Initialize SD card file system
	err=init_file(buff,appname);
	if (err) {
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("Can't Open ");
		printstr(appname);
		printchar('\n');
		return -1;
	}

	// Initialize parameters
	g_pcg_font=0;
	clearscreen();
	setcursor(0,0,7);
	printstr("BASIC "BASVER"\n");
	wait60thsec(15);
	// Initialize music
	init_music();

	printstr("Compiling...");

	// Compile the file
	err=compile_file();
	FSfclose(g_fhandle);
	if (err) {
		// Compile error
		printstr(err);
		printstr("\nAround: '");
		for(i=0;i<5;i++){
			printchar(g_source[g_srcpos-2+i]);
		}
		printstr("' in line ");
		printdec(g_line);
		printstr("\n");
		for(i=g_srcpos;0x20<=g_source[i];i++);
		g_source[i]=0x00;
		for(i=g_srcpos;0x20<=g_source[i];i--);
		printstr(g_source+i);
		return g_fileline;
	}

	// Link
	err=link();
	if (err) {
		// Link error
		printstr(err);
		printstr(resolve_label(g_label));
		return -2;
	}

	// All done
	printstr("done\n");
	if(test) return 0; //コンパイルのみの場合
	wait60thsec(15);

	// Initialize the other parameters
	// Random seed
	g_rnd_seed=2463534242;
	// Clear variables
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_mem[i]=0;
		g_var_size[i]=0;
	}
	// Clear key input buffer
	for(i=0;i<256;i++){
		ps2keystatus[i]=0;
	}
	// Reset data/read.
	reset_dataread();

	// Assign memory
	set_free_area((void*)(g_object+g_objpos),(void*)(&RAM[RAMSIZE]));
	// Execute program
	// Start program from the beginning of RAM.
	// Work area (used for A-Z values) is next to the object code area.
	start_program((void*)(&(RAM[0])),(void*)(&g_var_mem[0]));
	printstr("\nOK\n");

	return 0;
}
