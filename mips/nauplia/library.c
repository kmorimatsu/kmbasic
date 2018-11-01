/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include <xc.h>
#include "compiler.h"
#include "api.h"
#include "keyinput.h"

int lib_read(int mode,unsigned int label){
	unsigned int i,code;
	static unsigned int pos=0;
	static unsigned int in_data=0;
	if (mode==2) {
		// Reset data/read
		pos=0;
		in_data=0;
		return 0;
	}
	if (label) {
		// RESTORE function
		i=(int)search_label(label);
		if (!i) {
			err_data_not_found();
			return 0;
		}
		i-=(int)(&g_object[0]);
		pos=i/4;
		in_data=0;
		return 0;
	} else {
		// Get data
		if (in_data==0) {
			for(i=pos+1;i<g_objpos;i++){
				code=g_object[i];
				if ((code&0xFFFF0000)!=0x04110000) continue;
				// "bgezal zero," assembly found.
				// Check if 0x00000021 follows
				if (g_object[i+1]!=0x00000021) {// addu        zero,zero,zero
					// If not, skip following block (it's strig).
					i+=code&0x0000FFFF;
					i--;
					continue;
				}
				// DATA region found.
				in_data=(code&0x0000FFFF)-1;
				pos=i+2;
				break;
			}
			if (g_objpos<=i) {
				err_data_not_found();
				return 0;
			}
		}
		in_data--;
		return g_object[pos++];
	}	
}

void reset_dataread(){
	lib_read(2,1);
}

char* lib_midstr(int var_num, int pos, int len){
	int i;
	char* str;
	char* ret;
	if (0<=pos) {
		// String after "pos" position.
		str=(char*)(g_var_mem[var_num]+pos);
	} else {
		// String right "pos" characters.
		// Determine length
		str=(char*)g_var_mem[var_num];
		for(i=0;str[i];i++);
		if (0<=(i+pos)) {
			str=(char*)(g_var_mem[var_num]+i+pos);
		}
	}
	if (len<0) {
		// Length is not specified.
		// Return the string to the end.
		return str;
	}
	// Length is specified.
	// Construct temporary string containing specified number of characters.
	ret=alloc_memory((len+1+3)/4,-1);
	// Copy string.
	for(i=0;(ret[i]=str[i])&&(i<len);i++);
	ret[len]=0x00;
	return ret;
}

void lib_clear(void){
	int i;
	// All variables will be integer 0
	for(i=0;i<26;i++){
		g_var_mem[i]=0;
	}
	// Clear memory allocation area
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_size[i]=0;
	}
	// Cancel PCG
	stopPCG();
	g_pcg_font=0;
}

void lib_let_str(char* str, int var_num){
	int begin,end,size;
	// Save pointer
	g_var_mem[var_num]=(int)str;
	// Determine size
	for(size=0;str[size];size++);
	// Check if str is in heap area.
	begin=(int)str;
	end=(int)(&str[size]);
	if (begin<(int)(&g_heap_mem[0]) || (int)(&g_heap_mem[g_max_mem])<=end) {
		// String is not within allcated block
		return;
	}
	// Str is in heap area. Calculate values stored in heap data dimension
	begin-=(int)(&g_heap_mem[0]);
	begin>>=2;
	end-=(int)(&g_heap_mem[0]);
	end>>=2;
	size=end-begin+1;
	g_var_pointer[var_num]=begin;
	g_var_size[var_num]=size;
}

int lib_rnd(){
	int y;
	y=g_rnd_seed;
	y = y ^ (y << 13);
	y = y ^ (y >> 17);
	y = y ^ (y << 5);
	g_rnd_seed=y;
	return y&0x7fff;
}

char* lib_chr(int num){
	char* str;
	str=alloc_memory(1,-1);
	str[0]=num&0x000000FF;
	str[1]=0x00;
	return str;
}

char* lib_dec(int num){
	char* str;
	int i,j,minus;
	char b[12];
	b[11]=0x00;
	if (num<0) {
		minus=1;
		num=0-num;
	} else {
		minus=0;
	}
	for (i=10;0<i;i--) {
		if (num==0 && i<10) break; 
		b[i]='0'+rem10_32(num);
		num=div10_32(num);
	}
	if (minus) {
		b[i]='-';
	} else {
		i++;
	}
	str=alloc_memory(3,-1);
	for(j=0;str[j]=b[i++];j++);
	return str;
}

char* lib_hex(int num, int width){
	char* str;
	int i,j,minus;
	char b[8];
	str=alloc_memory(3,-1);
	for(i=0;i<8;i++){
		b[i]="0123456789ABCDEF"[(num>>(i<<2))&0x0F];
	}
	// Width must be between 0 and 8;
	if (width<0||8<width) width=8;
	if (width==0) {
		// Width not asigned. Use minimum width.
		for(i=7;0<i;i--){
			if ('0'<b[i]) break;
		}
	} else {
		// Constant width
		i=width-1;
	}
	// Copy string to allocated block.
	for(j=0;0<=i;i--){
		str[j++]=b[i];
	}
	str[j]=0x00;
	return str;
}

char* lib_connect_string(char* str1, char* str2){
	int i,j;
	char b;
	char* result;
	// Determine total length
	for(i=0;str1[i];i++);
	for(j=0;str2[j];j++);
	// Allocate a block for new string
	result=alloc_memory((i+j+1+3)/4,-1);
	// Create connected strings 
	for(i=0;b=str1[i];i++) result[i]=b;
	for(j=0;b=str2[j];j++) result[i+j]=b;
	result[i+j]=0x00;
	free_temp_str(str1);
	free_temp_str(str2);
	return result;
}

void lib_string(int mode){
	int i;
	switch(mode){
		case 0:
			// CR
			printchar('\n');
			return;
		case 1:
			// ,
			i=rem10_32((unsigned int)(cursor-TVRAM));
			printstr("          "+i);
			return;
		default:
			return;
	}
}

void* lib_label(unsigned int label){
	// This routine is used to jump to address dynamically determined
	// in the code; for example: "GOTO 100+I"
	unsigned int i,code,search;
	void* ret;  
	if (label&0xFFFF0000) {
		// Label is not supported.
		// Line number must bs less than 65536.
		err_label_not_found();
	} else {
		// Line number
		ret=search_label(label);
		if (ret) return ret;
		// Line number not found.
		err_label_not_found();
	}
}

int lib_keys(int mask){
	int keys;
	// Enable tact switches
	if (inPS2MODE()) {
		buttonmode();
	}

	keys=KEYPORT;
	keys=
		((keys&KEYUP)?    0:1)|
		((keys&KEYDOWN)?  0:2)|
		((keys&KEYLEFT)?  0:4)|
		((keys&KEYRIGHT)? 0:8)|
		((keys&KEYSTART)? 0:16)|
		((keys&KEYFIRE)?  0:32);
	return mask&keys;
}

int lib_val(char* str){
	int i;
	int val=0;
	int sign=1;
	char b;
	// Skip blanc
	for(i=0;0<=str[i] && str[i]<0x21;i++);
	// Skip '+'
	if (str[i]=='+') i++;
	// Check '-'
	if (str[i]=='-') {
		sign=-1;
		i++;
	}
	// Check '0x' or '$'
	if (str[i]=='$' || str[i]=='0' && (str[i+1]=='x' || str[i+1]=='X')) {
		// Hexadecimal
		if (str[i++]=='0') i++;
		while(1) {
			b=str[i++];
			if ('0'<=b && b<='9') {
				val<<=4;
				val+=b-'0';
			} else if ('a'<=b && b<='f') {
				val<<=4;
				val+=b-'a'+10;
			} else if ('A'<=b && b<='F') {
				val<<=4;
				val+=b-'A'+10;
			} else {
				break;
			}
		}
	} else {
		// Decimal
		while(1) {
			b=str[i++];
			if ('0'<=b && b<='9') {
				val*=10;
				val+=b-'0';
			} else {
				break;
			}
		}
	}
	return val*sign;
}

char* lib_input(){
	// Allocate memory for strings with 63 characters
	char *str=calloc_memory((63+1)/4,-1);
	// Enable PS/2 keyboard
	if (!inPS2MODE()) {
		ps2mode();
		ps2init();
	}
	// Clear key buffer
	do ps2readkey();
	while(vkey!=0);
	// Get string as a line
	lineinput(str,63);
	check_break();
	return str;
}

unsigned char lib_inkey(int key){
	int i;
	// Enable PS/2 keyboard
	if (!inPS2MODE()) {
		ps2mode();
		ps2init();
	}
	if (key) {
		return ps2keystatus[key&0xff];
	} else {
		for(i=0;i<256;i++){
			if (ps2keystatus[i]) return i;
		}
		return 0;
	}
}

void lib_usepcg(int mode){
	// Modes; 0: stop PCG, 1: use PCG, 2: reset PCG and use it
	switch(mode){
		case 0:
			// Stop PCG
			stopPCG();
			break;
		case 2:
			// Reset PCG and use it
			if (g_pcg_font) {
				free_temp_str(g_pcg_font);
				g_pcg_font=0;
			}
			// Continue to case 1:
		case 1:
		default:
			// Use PCG
			if (g_pcg_font) {
				startPCG(g_pcg_font,0);
			} else {
				g_pcg_font=alloc_memory(256*8/4,ALLOC_PCG_BLOCK);
				startPCG(g_pcg_font,1);
			}
			break;
	}
}

void lib_pcg(unsigned int ascii,unsigned int fontdata1,unsigned int fontdata2){
	unsigned int* pcg;
	// If USEPCG has not yet executed, do now.
	if (!g_pcg_font) lib_usepcg(1);
	pcg=(unsigned int*)g_pcg_font;
	// 0 <= ascii <= 0xff
	ascii&=0xff;
	// Update font data
	ascii<<=1;
	pcg[ascii]=(fontdata1>>24)|((fontdata1&0xff0000)>>8)|((fontdata1&0xff00)<<8)|(fontdata1<<24);
	pcg[ascii+1]=(fontdata2>>24)|((fontdata2&0xff0000)>>8)|((fontdata2&0xff00)<<8)|(fontdata2<<24);
}

void lib_scroll(int x,int y){
	int i,j;
	int vector=y*WIDTH_X+x;
	if (vector<0) {
		// Copy data from upper address to lower address
		for(i=0-vector;i<WIDTH_X*WIDTH_Y;i++){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X*WIDTH_Y+i+vector]=TVRAM[WIDTH_X*WIDTH_Y+i];
		}
	} else if (0<vector) {
		// Copy data from lower address to upper address
		for(i=WIDTH_X*WIDTH_Y-vector-1;0<=i;i--){
			TVRAM[i+vector]=TVRAM[i];
			TVRAM[WIDTH_X*WIDTH_Y+i+vector]=TVRAM[WIDTH_X*WIDTH_Y+i];
		}
	} else {
		return;
	}
	if (x<0) {
		// Fill blanc at right
		for(i=x;i<0;i++){
			for(j=WIDTH_X+i;j<WIDTH_X*WIDTH_Y;j+=WIDTH_X){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X*WIDTH_Y+j]=cursorcolor;
			}
		}
	} else if (0<x) {
		// Fill blanc at left
		for(i=0;i<x;i++){
			for(j=i;j<WIDTH_X*WIDTH_Y;j+=WIDTH_X){
				TVRAM[j]=0x00;
				TVRAM[WIDTH_X*WIDTH_Y+j]=cursorcolor;
			}
		}
	}
	if (y<0) {
		// Fill blanc at bottom
		for(i=WIDTH_X*(WIDTH_Y+y);i<WIDTH_X*WIDTH_Y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X*WIDTH_Y+i]=cursorcolor;
		}
	} else if (0<y) {
		// Fill blanc at top
		for(i=0;i<WIDTH_X*y;i++){
				TVRAM[i]=0x00;
				TVRAM[WIDTH_X*WIDTH_Y+i]=cursorcolor;
		}
	}
}

void lib_wait(int period){
	int i;
	unsigned short dcount;
	for(i=0;i<period;i++){
		dcount=drawcount;
		while(dcount==drawcount){
			asm ("wait");
			check_break();
		}
	}
}

int _call_library(int a0,int a1,int a2,enum libs a3);

void call_library(void){
	asm volatile("addu $a2,$v0,$zero");
	asm volatile("j _call_library");
}

int _call_library(int a0,int a1,int v0,enum libs a3){
	// usage: call_lib_code(LIB_XXXX);
	// Above code takes 2 words.
	check_break();
	switch(a3){
		case LIB_DIV0:
			err_div_zero();
			return;
		case LIB_STRNCMP:
			return strncmp((char*)a0,(char*)a1,v0);
		case LIB_MIDSTR:
			return (int)lib_midstr(a1,v0,a0);
		case LIB_RND:
			return (int)lib_rnd();
		case LIB_DEC:
			return (int)lib_dec(v0);
		case LIB_HEX:
			return (int)lib_hex(v0,a0);
		case LIB_CHR:
			return (int)lib_chr(v0);
		case LIB_VAL:
			return lib_val((char*)v0);
		case LIB_LETSTR:
			lib_let_str((char*)v0,a0);
			return;
		case LIB_CONNECT_STRING:
			return (int)lib_connect_string((char*)a0, (char*)v0);
		case LIB_STRING:
			lib_string(a0);
			return;
		case LIB_PRINTSTR:
			printstr((char*)v0);
			return;
		case LIB_SCROLL:
			lib_scroll(a0,v0);
			return;
		case LIB_KEYS:
			return lib_keys(v0);
		case LIB_INKEY:
			return (int)lib_inkey(v0);
		case LIB_CURSOR:
			setcursor(a0,v0,cursorcolor);
			return;
		case LIB_SOUND:
			set_sound((unsigned long*)v0);
			return;
		case LIB_MUSICFUNC:
			return musicRemaining();
		case LIB_MUSIC:
			set_music((char*)v0);
			return;
		case LIB_SETDRAWCOUNT:
			drawcount=(v0&0x0000FFFF);
			return;
		case LIB_DRAWCOUNT:
			return drawcount;
		case LIB_RESTORE:
			lib_read(0,v0);
			return;
		case LIB_READ:
			return lib_read(0,0);
		case LIB_LABEL:
			return (int)lib_label(v0);
		case LIB_INPUT:
			return (int)lib_input();
		case LIB_USEPCG:
			lib_usepcg(v0);
			return;
		case LIB_PCG:
			lib_pcg(a0,a1,v0);
			return;
		case LIB_BGCOLOR: // BGCOLOR R,G,B
			set_bgcolor(v0,a0,a1); //set_bgcolor(b,r,g);
			return;
		case LIB_PALETTE: // PALETTE N,R,G,B
			set_palette(a0,v0,(a1>>16)&255,a1&255); // set_palette(n,b,r,g);
			return;
		case LIB_CLS:
			clearscreen();
			return;
		case LIB_COLOR:
			setcursorcolor(v0);
			return;
		case LIB_WAIT:
			lib_wait(v0);
			return;
		case LIB_CLEAR:
			lib_clear();
			return;
		case LIB_DIM:
			return (int)calloc_memory(v0+1, a0);
		default:
			break;
	}
}