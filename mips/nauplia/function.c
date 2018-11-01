/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include "compiler.h"
#include "api.h"

char* music_function(){
	call_lib_code(LIB_MUSICFUNC);
	return 0;
}

char* read_function(){
	call_lib_code(LIB_READ);
	return 0;
}

char* gosub_function(){
	return gosub_statement();
}
char* strncmp_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF8; // addiu       sp,sp,-8
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	err=get_string();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA40008; // lw          a0,8(sp)
	g_object[g_objpos++]=0x8FA50004; // lw          a1,4(sp)
	g_object[g_objpos++]=0x27BD0008; // addiu       sp,sp,8
	call_lib_code(LIB_STRNCMP);
	return 0;
}
char* len_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(5);
	g_object[g_objpos++]=0x2443FFFF; // addiu       v1,v0,-1
	                                 // loop:
	g_object[g_objpos++]=0x80640001; // lb          a0,1(v1)
	g_object[g_objpos++]=0x1480FFFE; // bne         a0,zero,loop
	g_object[g_objpos++]=0x24630001; // addiu       v1,v1,1
	g_object[g_objpos++]=0x00621023; // subu        v0,v1,v0
	return 0;
}

char* asc_function(){
	char* err;
	err=get_string();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0x90420000; // lbu         v0,0(v0)
	return 0;
}

char* val_function(){
	char* err;
	err=get_string();
	if (err) return err;
	call_lib_code(LIB_VAL);
	return 0;	
}

char* peek_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0x90420000; // lbu         v0,0(v0)
	return 0;
}

char* sgn_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(5);
	g_object[g_objpos++]=0x10400004; // beq         v0,zero,end
	g_object[g_objpos++]=0x24030001; // addiu       v1,zero,1
	g_object[g_objpos++]=0x1C400002; // bgtz        v0,end
	g_object[g_objpos++]=0x00601021; // addu        v0,v1,zero
	g_object[g_objpos++]=0x00031023; // subu        v0,zero,v1
	                                 // end:
	return 0;
}

char* abs_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x00021FC3; //sra         v1,v0,0x1f
	g_object[g_objpos++]=0x00621026; //xor         v0,v1,v0
	g_object[g_objpos++]=0x00431023; //subu        v0,v0,v1
	return 0;
}

char* not_function(){
	char* err;
	err=get_value();
	if (err) return err;
	check_obj_space(1);
	g_object[g_objpos++]=0x2C420001; //sltiu       v0,v0,1
	return 0;
}

char* rnd_function(){
	call_lib_code(LIB_RND);
	return 0;
}


char* chr_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_CHR);
	return 0;
}
char* hex_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]==',') {
		// Second argument found.
		// Get is as $a0.
		g_srcpos++;
		check_obj_space(2);
		g_object[g_objpos++]=0x27BDFFFC; //addiu       sp,sp,-4
		g_object[g_objpos++]=0xAFA20004; //sw          v0,4(sp)
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x00022021; //a0,zero,v0
		g_object[g_objpos++]=0x8FA20004; //lw          v0,4(sp)
		g_object[g_objpos++]=0x27BD0004; //addiu       sp,sp,4
	} else {
		// Second argument not found.
		// Set $a0 to 0.
		check_obj_space(1);
		g_object[g_objpos++]=0x24040000; //addiu       a0,zero,0
	}
	call_lib_code(LIB_HEX);
	return 0;
}

char* dec_function(void){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_DEC);
	return 0;
}

char* keys_function(void){
	char* err;
	next_position();
	if (g_source[g_srcpos]==')') {
		check_obj_space(1);
		g_object[g_objpos++]=0x3402003F; //ori         v0,zero,0x3f
	} else {
		err=get_value();
		if (err) return err;
	}	
	call_lib_code(LIB_KEYS);
	return 0;
}

char* tvram_function(void){
	char* err;
	int i;
	next_position();
	if (g_source[g_srcpos]==')') {
		i=(int)(&TVRAM[0]);
		i-=g_gp;
		check_obj_space(1);
		g_object[g_objpos++]=0x27820000|(i&0x0000FFFF);       // addiu       v0,gp,xxxx
	} else {
		err=get_value();
		if (err) return err;
		i=(int)(&TVRAM[0]);
		i-=g_gp;
		check_obj_space(3);
		g_object[g_objpos++]=0x27830000|(i&0x0000FFFF);       // addiu       v1,gp,xxxx
		g_object[g_objpos++]=0x00621821;                      // addu        v1,v1,v0
		g_object[g_objpos++]=0x90620000;                      // lbu         v0,0(v1)
	}	
	return 0;
}

char* drawcount_function(void){
	call_lib_code(LIB_DRAWCOUNT);
	return 0;
}

char* input_function(void){
	call_lib_code(LIB_INPUT);
	return 0;
}

char* inkey_function(void){
	char* err;
	next_position();
	if (g_source[g_srcpos]==')') {
		check_obj_space(1);
		g_object[g_objpos++]=0x34020000; //ori         v0,zero,0x00
	} else {
		err=get_value();
		if (err) return err;
	}	
	call_lib_code(LIB_INKEY);
	return 0;
}

char* str_function(void){
	char* err;
	if (nextCodeIs("CHR$(")) {
		err=chr_function();
	} else if (nextCodeIs("HEX$(")) {
		err=hex_function();
	} else if (nextCodeIs("DEC$(")) {
		err=dec_function();
	} else if (nextCodeIs("INPUT$(")) {
		err=input_function();
	} else {
		return ERR_SYNTAX;
	}
	if (err) return err;
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return 0;
}

char* function(void){
	char* err;
	if (nextCodeIs("NOT(")) {
		err=not_function();
	} else if (nextCodeIs("DRAWCOUNT(")) {
		err=drawcount_function();
	} else if (nextCodeIs("MUSIC(")) {
		err=music_function();
	} else if (nextCodeIs("TVRAM(")) {
		err=tvram_function();
	} else if (nextCodeIs("KEYS(")) {
		err=keys_function();
	} else if (nextCodeIs("READ(")) {
		err=read_function();
	} else if (nextCodeIs("GOSUB(")) {
		err=gosub_function();
	} else if (nextCodeIs("STRNCMP(")) {
		err=strncmp_function();
	} else if (nextCodeIs("PEEK(")) {
		err=peek_function();
	} else if (nextCodeIs("LEN(")) {
		err=len_function();
	} else if (nextCodeIs("ASC(")) {
		err=asc_function();
	} else if (nextCodeIs("SGN(")) {
		err=sgn_function();
	} else if (nextCodeIs("ABS(")) {
		err=abs_function();
	} else if (nextCodeIs("RND(")) {
		err=rnd_function();
	} else if (nextCodeIs("VAL(")) {
		err=val_function();
	} else if (nextCodeIs("INKEY(")) {
		err=inkey_function();
	} else {
		return ERR_SYNTAX;
	}
	if (err) return err;
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	return err;
}

/*
int g_v0;
int g_v1;
int test(int v0,int v1){
	g_v1=g_v0>0?g_v0:-g_v0;
	asm volatile("addiu $v1,$v0,-1");
	asm volatile("loop:");
	asm volatile("lb $a0,1($v1)");
	asm volatile("addiu $v1,$v1,1");
	asm volatile("bne $a0,$zero,loop");
	asm volatile("subu $v0,$v1,$v0");
	return v0*v1;
}
//*/
