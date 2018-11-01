/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include "api.h"
#include "compiler.h"

char* sound_statement(){
	char *err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0814xxxx and 0x0815xxxx are specific codes for these.
		check_obj_space(2);
		g_object[g_objpos++]=0x08140000|((g_label>>16)&0x0000FFFF); // lui   v0,xxxx
		g_object[g_objpos++]=0x08150000|(g_label&0x0000FFFF);       // ori v0,v0,xxxx
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
	}
	call_lib_code(LIB_SOUND);
	return 0;
}
char* music_statement(){
	char *err;
	err=get_string();
	call_lib_code(LIB_MUSIC);
	return 0;
}

char* exec_statement(){
	char *err;
	char b1;
	int i,prevpos;
	b1=g_source[g_srcpos];
	while('0'<=b1 && b1<='9' || b1=='-' || b1=='$'){
		prevpos=g_objpos;
		err=get_simple_value();
		if (err) return err;
		if (g_objpos==prevpos+1) {
			// 16 bit value was taken
			i=g_object[g_objpos-1]&0x0000FFFF;
		} else if (g_objpos==prevpos+2) {
			// 32 bit value was taken
			i=g_object[g_objpos-2]&0x0000FFFF;
			i<<=16;
			i|=g_object[g_objpos-1]&0x0000FFFF;
		} else {
			return ERR_SYNTAX;
		}
		g_objpos=prevpos;
		g_object[g_objpos++]=i;
		next_position();
		b1=g_source[g_srcpos];
		if (b1!=',') break;
		g_srcpos++;
		next_position();
		b1=g_source[g_srcpos];
	}
	return 0;
}

char* data_statement(){
	// 0x00000021(addu zero,zero,zero) is the sign of data region
	int i,prevpos;
	char* err;
	prevpos=g_objpos;
	check_obj_space(2);
	g_object[g_objpos++]=0x04110000; // bgezal      zero,xxxx
	g_object[g_objpos++]=0x00000021; // addu        zero,zero,zero
	err=exec_statement();
	if (err) return err;
	// Determine the size of data
	i=g_objpos-prevpos-1;
	g_object[prevpos]=0x04110000|i; // bgezal zero,xxxx
	return 0;
}

char* clear_statement(){
	call_lib_code(LIB_CLEAR);
	return 0;
}

char* poke_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA30004; // lw          v1,4(sp)
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	g_object[g_objpos++]=0xA0620000; // sb          v0,0(v1)
	return 0;
}

char* dim_statement(){
	char* err;
	char b1;
	while(1){
		next_position();
		b1=g_source[g_srcpos];
		if (b1<'A' || 'Z'<b1) return ERR_SYNTAX;
		g_srcpos++;
		next_position();
		if (g_source[g_srcpos]!='(') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
		g_srcpos++;
		check_obj_space(2);
		g_object[g_objpos++]=0x24040000|(b1-'A'); //addiu       a0,zero,xx
		call_lib_code(LIB_DIM);
		next_position();
		if (g_source[g_srcpos]!=',') break;
		g_srcpos++;
	}
	return 0;	
}

char* label_statement(){
	char* err;
	char b1;
	b1=g_source[g_srcpos];
	if (b1<'A' || 'Z'<b1) return ERR_SYNTAX; // Number is not allowed here.
	err=get_label();
	if (err) return err;
	// Check existing label with the same name here.
	if (search_label(g_label)) {
		// Error: duplicate labels
		printstr("Label ");
		printstr(resolve_label(g_label));
		return ERR_MULTIPLE_LABEL;
	}
	check_obj_space(2);
	g_object[g_objpos++]=0x3C160000|((g_label>>16)&0x0000FFFF); //lui s6,yyyy;
	g_object[g_objpos++]=0x36D60000|(g_label&0x0000FFFF);       //ori s6,s6,zzzz;
	return 0;
}

char* restore_statement(){
	char* err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Constant label/number
		if (g_label<65536) {
			// 16 bit
			check_obj_space(1);
			g_object[g_objpos++]=0x34020000|g_label;              // ori         v0,zero,xxxx
		} else {
			// 32 bit
			check_obj_space(2);
			g_object[g_objpos++]=0x3C020000|(g_label>>16);        // lui         v0,xxxx
			g_object[g_objpos++]=0x34420000|(g_label&0x0000FFFF); // ori         v0,v0,xxxx
		}
	} else {
		// Dynamic number
		err=get_value();
		if (err) return err;
	}
	call_lib_code(LIB_RESTORE);
	return 0;
}

char* gosub_statement(){
	char* err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0812xxxx and 0x0813xxxx are specific codes for these.
		check_obj_space(6);
		g_object[g_objpos++]=0x04110003;                            // bgezal      zero,label1
		g_object[g_objpos++]=0x27BDFFFC;                            // addiu       sp,sp,-4
		g_object[g_objpos++]=0x10000003;                            // beq         zero,zero,label2
		g_object[g_objpos++]=0x08120000|((g_label>>16)&0x0000FFFF); // nop         
		                                                            // label1:
		g_object[g_objpos++]=0x08130000|(g_label&0x0000FFFF);       // j           xxxx
		g_object[g_objpos++]=0xAFBF0004;                            // sw          ra,4(sp)
		                                                            // label2:
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
		check_obj_space(6);
		g_object[g_objpos++]=0x04110003; // bgezal      zero,label1
		g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
		g_object[g_objpos++]=0x10000003; // beq         zero,zero,label2
		g_object[g_objpos++]=0x00000000; // nop         
		                                 // label1:
		g_object[g_objpos++]=0x00400008; // jr          v0
		g_object[g_objpos++]=0xAFBF0004; // sw          ra,4(sp)
		                                 // label2:
	}
	return 0;
}

char* return_statement(){
	char* err;
	char b1;
	next_position();
	b1=g_source[g_srcpos];
	if (0x20<b1 && b1!=':') {
		// There is a return value.
		err=get_value();
		if (err) return err;
	}
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA30004; // lw          v1,4(sp)
	g_object[g_objpos++]=0x00600008; // jr          v1
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	return 0;
}

char* goto_statement(){
	char* err;
	err=get_label();
	if (err) return err;
	if (g_label) {
		// Label/number is constant.
		// Linker will change following codes later.
		// Note that 0x0810xxxx and 0x0811xxxx are specific codes for these.
		check_obj_space(2);
		g_object[g_objpos++]=0x08100000|((g_label>>16)&0x0000FFFF); // j xxxx
		g_object[g_objpos++]=0x08110000|(g_label&0x0000FFFF);       // nop
	} else {
		// Label/number will be dynamically set when executing code.
		err=get_value();
		if (err) return err;
		call_lib_code(LIB_LABEL);
		check_obj_space(2);
		g_object[g_objpos++]=0x00400008; // jr          v0
		g_object[g_objpos++]=0x00000000; // nop
	}
	return 0;
}

char* if_statement(){
	char* err;
	int prevpos,bpos;
	// Get value.
	err=get_value();
	if (err) return err;
	// Check "THEN"
	if (!nextCodeIs("THEN ")) return ERR_SYNTAX;
	// If $v0=0 then skip.
	bpos=g_objpos;
	check_obj_space(2);
	g_object[g_objpos++]=0x10400000; // beq         v0,zero,xxxx
	g_object[g_objpos++]=0x00000000; // nop
	// Next statement is either label or general statement
	prevpos=g_srcpos;
	if (statement()) {
		// May be label
		g_srcpos=prevpos;
		err=goto_statement();
		if (err) return err;
	} else {
		// Must be statement(s)
		while(1) {
			if (g_source[g_srcpos]!=':') break;
			g_srcpos++;
			err=statement();
			if (err) return err;
		}
	}
	// Check if "ELSE" exists.
	if (!nextCodeIs("ELSE ")) {
		// "ELSE" not found. This is the end of "IF" statement.
		// Previous branch command must jump to this position.
		g_object[bpos]=0x10400000|(g_objpos-bpos-1); // beq         v0,zero,xxxx	
		return 0;
	}
	// Skip after ELSE if required.
	check_obj_space(2);
	g_object[g_objpos++]=0x10000000; // beq         zero,zero,xxxx
	g_object[g_objpos++]=0x00000000; // nop
	// Previous branch command must jump to this position.
	g_object[bpos]=0x10400000|(g_objpos-bpos-1); // beq         v0,zero,xxxx	
	bpos=g_objpos-2;
	// Next statement is either label or general statement
	prevpos=g_srcpos;
	if (statement()) {
		// May be label
		g_srcpos=prevpos;
		err=goto_statement();
		if (err) return err;
	} else {
		// Must be statement(s)
		while(1) {
			if (g_source[g_srcpos]!=':') break;
			g_srcpos++;
			err=statement();
			if (err) return err;
		}
	}
	// Previous branch command must jump to this position.
	g_object[bpos]=0x10000000|(g_objpos-bpos-1); // beq         zero,zero,xxxx	
	return 0;
}



char* end_statement(void){
	int i;
	i=(int)&g_end_addr;
	i-=g_gp;
	check_obj_space(3);
	g_object[g_objpos++]=0x8F820000|(i&0x0000FFFF);       // lw v0,xxxx(gp)
	g_object[g_objpos++]=0x00400008;                      // jr v0
	g_object[g_objpos++]=0x00000000;                      // nop
	return 0;
}


char* let_statement(){
	char* err;
	char b1,b2;
	int i;
	next_position();
	b1=g_source[g_srcpos];
	b2=g_source[g_srcpos+1];
	if (b1<'A' || 'Z'<b1) return ERR_SYNTAX;
	g_srcpos++;
	if (b2=='$') {
		// String
		g_srcpos++;
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_string();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0x24040000|(b1-'A'); //addiu       a0,zero,xx
		call_lib_code(LIB_LETSTR);
		return 0;
	} else if (b2=='(') {
		// Dimension
		g_srcpos++;
		err=get_value();
		if (err) return err;
		if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
		g_srcpos++;
		check_obj_space(5);
		g_object[g_objpos++]=0x00021080;              // sll v0,v0,0x2
		g_object[g_objpos++]=0x8FC30000|((b1-'A')*4); // lw    v1,xx(s8)
		g_object[g_objpos++]=0x00621821;              // addu  v1,v1,v0
		g_object[g_objpos++]=0x27BDFFFC;              // addiu sp,sp,-4
		g_object[g_objpos++]=0xAFA30004;              // sw    v1,4(sp)
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(3);
		g_object[g_objpos++]=0x8FA30004;              // lw    v1,4(sp)
		g_object[g_objpos++]=0x27BD0004;              // addiu sp,sp,4
		g_object[g_objpos++]=0xAC620000;              // sw    v0,0(v1)
		return 0;
	} else {
		// Integer A-Z
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_value();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFC20000|((b1-'A')*4); // sw v0,xxx(s8)
	}
	return 0;
}

char* print_statement(){
	char* err;
	char b1;
	int i,prevpos;
	int status=0;// 1:',' 2:';' 0:none
	while(1){
		next_position();
		b1=g_source[g_srcpos];
		if (b1<0x20 || b1==':') break;
		if (!strncmp(g_source+g_srcpos,"ELSE "  ,5)) break;	
		prevpos=g_objpos;
		err=get_string();
		if (err) {
			// Restore changes caused by trying get_string().
			g_objpos=prevpos;
			g_sdepth=0;
			// May be integer
			err=get_value();
			if (err) return err;
			// Use DEC$() function.
			call_lib_code(LIB_DEC);
		}
		// Call printstr() function
		// First argument is the pointer to string
		call_lib_code(LIB_PRINTSTR);
		next_position();
		b1=g_source[g_srcpos];
		if (b1==',') {
			status=1;
			g_srcpos++;
			// Call lib_string() function for comma (,)
			check_obj_space(1);
			g_object[g_objpos++]=0x34040001;                      // ori   a0,zero,1
			call_lib_code(LIB_STRING);
		} else if (b1==';') {
			status=2;
			g_srcpos++;
		} else {
			status=0;
		}
	}
	if (status==0) {
		// Call lib_string() function for CR (\n)
		check_obj_space(1);
		g_object[g_objpos++]=0x34040000;                      // ori   a0,zero,0
		call_lib_code(LIB_STRING);
	}
	return 0;
}
char* for_statement(){
	char* err;
	char b1;
	// Initialization of variable
	next_position();
	b1=g_source[g_srcpos];
	if (b1<'A' || 'Z'<b1) return ERR_SYNTAX;
	err=let_statement();
	if (err) return err;
	// Check if "TO" exists
	next_position();
	if (!nextCodeIs("TO ")) return ERR_SYNTAX;
	err=get_value();
	if (err) return err;
	// Usage of stack:
	//   12(sp): "TO" value
	//    8(sp): "STEP" value
	//    4(sp): Address to return to in "NEXT" statement.
	// Store "TO" value in stack
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF4; // addiu sp,sp,-12
	g_object[g_objpos++]=0xAFA2000C; // sw v0,12(sp)
	// Check if "STEP" exists
	next_position();
	if (nextCodeIs("STEP ")) {
		// "STEP" exists. Get value
		err=get_value();
		if (err) return err;
	} else {
		// "STEP" not exist. Use "1".	
		check_obj_space(1);
		g_object[g_objpos++]=0x24020001; // addiu v0,zero,1
	}
	// Store "STEP" value in stack and jump to start address 
	// while store return address to $ra.
	check_obj_space(11);
	g_object[g_objpos++]=0x04110009;              // bgezal     zero,label1
	g_object[g_objpos++]=0xAFA20008;              // sw         v0,8(sp)
	// After executing "NEXT" statement, process reaches following line.
	// Go to next step and check if variable reachs "TO" value.
	// Note that $v1 is set to 12($sp) in NEXT statement.
	// If yes, exit FOR-NEXT loop while restore stack pointer.
	// Note that $ra contain the address to go to.
	g_object[g_objpos++]=0x8FC20000|((b1-'A')*4); // lw          v0,xx(s8)
	g_object[g_objpos++]=0x14430003;              // bne         v0,v1,label0
	g_object[g_objpos++]=0x8FA30008;              // lw          v1,8(sp)
	g_object[g_objpos++]=0x03E00008;              // jr          ra
	g_object[g_objpos++]=0x27BD000C;              // addiu       sp,sp,12
	                                              // label0
	g_object[g_objpos++]=0x00431021;              // addu        v0,v0,v1
	g_object[g_objpos++]=0x10000002;              // beq         zero,zero,label2 
	g_object[g_objpos++]=0xAFC20000|((b1-'A')*4); // sw          v0,xx(s8)
	                                              // label1:
	g_object[g_objpos++]=0xAFBF0004;              // sw          ra,4(sp)
	                                              // label2:
	return 0;
}

char* next_statement(){
	// Return to address stored in 4($sp)
	// while set $v1 to 8($sp) (see for_statement) 
	// and store return address to exit FOR-NEXT loop.
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA20004; // lw          v0,4(sp)
	g_object[g_objpos++]=0x0040F809; // jalr        ra,v0
	g_object[g_objpos++]=0x8FA3000C; // lw          v1,12(sp)
	return 0;
}

char* rem_statement(){
	while(0x20<=g_source[g_srcpos]){
		g_srcpos++;
	}
	return 0;
}

char* palette_statement(){
	// PALETTE N,R,G,B
	char* err;
	// Get N
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF4; // addiu       sp,sp,-12
	g_object[g_objpos++]=0xAFA2000C; // sw          v0,12(sp)
	// Get R
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	// Get G
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get B
	err=get_value();
	if (err) return err;
	check_obj_space(6);
	g_object[g_objpos++]=0x8FA40008; // lw          a0,8(sp)
	g_object[g_objpos++]=0x00042400; // sll         a0,a0,0x10
	g_object[g_objpos++]=0x8FA50004; // lw          a1,4(sp)
	g_object[g_objpos++]=0x00A42825; // or          a1,a1,a0
	g_object[g_objpos++]=0x8FA4000C; // lw          a0,12(sp)
	g_object[g_objpos++]=0x27BD000C; // addiu       sp,sp,12
	// a0=N, a1=(R<<16)|G, v0=B
	call_lib_code(LIB_PALETTE);
	return 0;
}

char* param3_statement(enum libs lib){
	char* err;
	// Get 1st parameter
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFF8; // addiu       sp,sp,-8
	g_object[g_objpos++]=0xAFA20008; // sw          v0,8(sp)
	// Get 2nd parameter
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 3rd parameter
	err=get_value();
	if (err) return err;
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA40008; // lw          a0,8(sp)
	g_object[g_objpos++]=0x8FA50004; // lw          a1,4(sp)
	g_object[g_objpos++]=0x27BD0008; // addiu       sp,sp,8
	// a0=1st, a1=2nd, v0=3rd
	call_lib_code(lib);
	return 0;
}

char* bgcolor_statement(){
	// BGCOLOR R,G,B
	return param3_statement(LIB_BGCOLOR);
}

char* pcg_statement(){
	// PCG ASCII,D1,D2
	return param3_statement(LIB_PCG);
}

char* usepcg_statement(){
	int objpos=g_objpos;
	if (get_value()) {
		// Getting integer failed.
		// It supporsed to be not parameter
		// and same as parameter=1.
		g_objpos=objpos;
		check_obj_space(1);
		g_object[g_objpos++]=0x34020001; //ori         v0,zero,0x01
	}
	call_lib_code(LIB_USEPCG);
	return 0;
}

char* cls_statement(){
	call_lib_code(LIB_CLS);
	return 0;
}

char* color_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_COLOR);
	return 0;
}

char* param2_statement(enum libs lib){
	char* err;
	// Get 1st
	err=get_value();
	if (err) return err;
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	// Get 2nd
	err=get_value();
	if (err) return err;
	check_obj_space(2);
	g_object[g_objpos++]=0x8FA40004; // lw          a0,4(sp)
	g_object[g_objpos++]=0x27BD0004; // addiu       sp,sp,4
	call_lib_code(lib);
	return 0;
}

char* cursor_statement(){
	// CURSOR X,Y
	return param2_statement(LIB_CURSOR);
}

char* scroll_statement(){
	// CURSOR X,Y
	return param2_statement(LIB_SCROLL);
}

char* drawcount_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_SETDRAWCOUNT);
	return 0;
}

char* wait_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_lib_code(LIB_WAIT);
	return 0;
}

char* statement(void){
	char* err;
	int prevpos;
	next_position();
	// Initialize stack handler used for value
	g_sdepth=g_maxsdepth=0;
	if (nextCodeIs("REM")) {
		err=rem_statement();
	} else if (nextCodeIs("SOUND ")) {
		err=sound_statement();
	} else if (nextCodeIs("MUSIC ")) {
		err=music_statement();
	} else if (nextCodeIs("DRAWCOUNT ")) {
		err=drawcount_statement();
	} else if (nextCodeIs("CURSOR ")) {
		err=cursor_statement();
	} else if (nextCodeIs("PALETTE ")) {
		err=palette_statement();
	} else if (nextCodeIs("BGCOLOR ")) {
		err=bgcolor_statement();
	} else if (nextCodeIs("CLS")) {
		err=cls_statement();
	} else if (nextCodeIs("COLOR ")) {
		err=color_statement();
	} else if (nextCodeIs("RESTORE ")) {
		err=restore_statement();
	} else if (nextCodeIs("DATA ")) {
		err=data_statement();
	} else if (nextCodeIs("LABEL ")) {
		err=label_statement();
	} else if (nextCodeIs("DIM ")) {
		err=dim_statement();
	} else if (nextCodeIs("CLEAR")) {
		err=clear_statement();
	} else if (nextCodeIs("PRINT")) {
		err=print_statement();
	} else if (nextCodeIs("IF ")) {
		err=if_statement();
	} else if (nextCodeIs("END")) {
		err=end_statement();
	} else if (nextCodeIs("EXEC ")) {
		err=exec_statement();
	} else if (nextCodeIs("GOTO ")) {
		err=goto_statement();
	} else if (nextCodeIs("GOSUB ")) {
		err=gosub_statement();
	} else if (nextCodeIs("RETURN")) {
		err=return_statement();
	} else if (nextCodeIs("POKE ")) {
		err=poke_statement();
	} else if (nextCodeIs("FOR ")) {
		err=for_statement();
	} else if (nextCodeIs("NEXT")) {
		err=next_statement();
	} else if (nextCodeIs("LET ")) {
		err=let_statement();
	} else if (nextCodeIs("PCG ")) {
		err=pcg_statement();
	} else if (nextCodeIs("USEPCG")) {
		err=usepcg_statement();
	} else if (nextCodeIs("SCROLL ")) {
		err=scroll_statement();
	} else if (nextCodeIs("WAIT ")) {
		err=wait_statement();
	} else {
		err=let_statement();
	}
	if (err) return err;
	// Stack handler must be zero here.
	if (g_sdepth!=0) return ERR_UNKNOWN;
	return 0;
}
