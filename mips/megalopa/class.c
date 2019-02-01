/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include "compiler.h"

static int* g_class_structure;

/*
	CMPDATA_CLASS structure
		type:      CMPDATA_CLASS (2)
		len:       3
		data16:    n/a (0)
		record[1]: class name as integer
		record[2]: pointer to class structure
*/

/*
	CMPDATA_FIELD structure
		type:      CMPDATA_FIELD (3)
		len:       2 or 3 (2: field; 3: method)
		data16:    field or method
		             0: public field
		             1: private field
		             2: public method
		             3: reserved
		record[1]: field/method name as integer
		record[2]: pointer to method
*/

/*
	CMPDATA_STATIC structure
		type:      CMPDATA_STATIC (4)
		len:       3
		data16:    variable number; add ALLOC_LNV_BLOCK when using
		record[1]: class name as integer
		record[2]: variable name as integer
*/


#define PUBLIC_FIELD 0
#define PRIVATE_FIELD 1
#define PUBLIC_METHOD 2

/*
	Local prototyping
*/
char* obj_method(int method);

char* update_class_info(int class){
	int* record;
	int data[2];
	// Update record if exist.
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_CLASS)){
		if (record[1]==class) {
			record[2]=(int)g_class_structure;
			return 0;
		}
	}
	// No record of this class yet. Insert a record.
	data[0]=class;
	data[1]=(int)g_class_structure;
	return cmpdata_insert(CMPDATA_CLASS,0,&data[0],2);
}

/*
	Class structure:
		cstruct[0]:   class name as integer
		cstruct[1]:   number of fields and methods:
		                bit 0-7:   # of public fields
		                bit 8-15:  # of private fields
		                bit 16-23: # of public methods
		                bit 24-31: reserved
		cstruct[x]:   public field name
		cstruct[x+1]: public field var number
		cstruct[y]:   private field name
		cstruct[y+1]: private field var number
		cstruct[z]:   public method name
		cstruct[z+1]: public method pointer
*/

char* construct_class_structure(int class){
	int* record;
	int i;
	int num=0;
	// Register current address to global var
	g_class_structure=&g_object[g_objpos];
	// Construct a class structure in object area in following lines
	// Class name
	check_obj_space(2);
	g_object[g_objpos++]=class; // Class name
	g_objpos++;                 // Number of fields/methods
	// Public fields
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if ((record[0]&0xffff)==PUBLIC_FIELD) {
			num+=1<<0;
			check_obj_space(2);
			g_object[g_objpos++]=record[1]; // Field name
			g_objpos++;                     // Var number (see below)
		}
	}
	// Private fields
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if ((record[0]&0xffff)==PRIVATE_FIELD) {
			num+=1<<8;
			check_obj_space(2);
			g_object[g_objpos++]=record[1]; // Field name
			g_objpos++;                     // Var number (see below)
		}
	}
	// Public methods
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if ((record[0]&0xffff)==PUBLIC_METHOD) {
			num+=1<<16;
			check_obj_space(2);
			g_object[g_objpos++]=record[1]; // Method name
			g_object[g_objpos++]=record[2]; // pointer
		}
	}
	// Update number info
	g_class_structure[1]=num;
	// Update var numbers of fields
	num=((num>>8)&0xff)+(num&0xff);
	for(i=1;i<=num;i++){
		if ((
			g_class_structure[i*2+1]=search_var_name(0x7FFFFFFF & g_class_structure[i*2])+ALLOC_LNV_BLOCK
			)<ALLOC_LNV_BLOCK) return ERR_UNKNOWN;
	}
	return 0;
}

void delete_cmpdata_for_class(){
	int* record;
	// Delete field/method data
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		cmpdata_delete(record);
		cmpdata_reset();
	}	
	// Delete longvar data
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_USEVAR)){
		cmpdata_delete(record);
		cmpdata_reset();
	}
}

void* search_method(int* classdata,int method){
	int pos,i;
	int nums=classdata[1];

	classdata+=2;                  // exclude first 2 words
	classdata+=2*(nums&0xff);      // exclude public field
	classdata+=2*((nums>>8)&0xff); // exclude private field
	nums=(nums>>16)&0xff;          // number of methods
	for(i=0;i<nums;i++){
		if (classdata[0]==method) return (void*)classdata[1];
		classdata+=2;
	}
	return 0; // not found
}

int object_size(int* classdata){
	int nums=classdata[1];
	int size=nums&0xff;   // public field
	size+=(nums>>8)&0xff; // private
	return size;	
}

char* new_function(){
	char* err;
	int class,size;
	int i,stack, opos;
	int* data;
	int* classdata;
	void* init_method;
	// Resolve class name
	err=get_label();
	if (err) return err;
	if (!g_label) return ERR_SYNTAX;
	class=g_label;
	next_position();
	// Get class data from cmpdata
	// Note that the address of class structure can be resolved
	// by using cmpdata when compiling NEW function but not running. 
	// Therefore, class table is not requred when running.
	cmpdata_reset();
	while(data=cmpdata_find(CMPDATA_CLASS)){
		if (data[1]==class) break;
	}
	if (!data) return ERR_NO_CLASS;
	classdata=(int*)data[2];
	size=object_size(classdata);
	// Create object
	call_quicklib_code(lib_calloc_memory,ASM_ORI_A0_ZERO_|(size+1));
	// First word of object is pointer to classdata
	check_obj_space(3);
	g_object[g_objpos++]=0x3C080000|(((unsigned int)classdata)>>16);        // lui         t0,xxxx
	g_object[g_objpos++]=0x35080000|(((unsigned int)classdata)&0x0000FFFF); // ori         t0,t0,xxxx
	g_object[g_objpos++]=0xAC480000; // sw          t0,0(v0)
	// Check if INIT method exists
	init_method=search_method(classdata,LABEL_INIT);
	if (!init_method) {
		// All done
		// Note that $v0 is address of object here.
		// There should not be parameter(s).
		if (g_source[g_srcpos]==',') return ERR_NO_INIT;
		return 0;
	}
	// INIT method exists. Note that $v0 is address of object here.
	if (g_source[g_srcpos]==',') g_srcpos++;
	else if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	check_obj_space(2);
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFA20004; // sw          v0,4(sp)
	err=obj_method(LABEL_INIT);
	if (err) return err;
	g_srcpos--; // Leave ')' character for detecting end of "new" function
	check_obj_space(2);
	g_object[g_objpos++]=0x8FA20004; //lw          v0,4(sp)
	g_object[g_objpos++]=0x27BD0004; //addiu       sp,sp,4
	// All done
	// Note that $v0 is address of object here.
	return 0;
}

char* field_statement(){
	char* err;
	int i;
	int data[1];
	int is_private=0;
	// This statement is valid only in class file.
	if (!g_compiling_class) return ERR_INVALID_NON_CLASS;
	// Check which private or public
	next_position();
	if (nextCodeIs("PRIVATE ")) {
		is_private=1;
	} else if (nextCodeIs("PUBLIC ")) {
		is_private=0;
	}
	do {
		next_position();
		i=check_var_name();
		if (i<65536) return ERR_SYNTAX;
		// Register varname
		err=register_var_name(i);
		if (err) return err;
		if (g_source[g_srcpos]=='#') {
			g_srcpos++;
		} else if (g_source[g_srcpos]=='$') {
			// String field. Raise 31st bit.
			g_srcpos++;
			i|=0x80000000;
		} else if (g_source[g_srcpos]=='(' && g_source[g_srcpos+1]==')' && is_private) {
			// Dimension field (private only). Raise 31st bit.
			g_srcpos++;
			g_srcpos++;
			i|=0x80000000;
		}
		// Register field
		data[0]=i;
		if (is_private) {
			err=cmpdata_insert(CMPDATA_FIELD,PRIVATE_FIELD,(int*)&data[0],1);
		} else {
			err=cmpdata_insert(CMPDATA_FIELD,PUBLIC_FIELD,(int*)&data[0],1);
		}
		next_position();
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	} while(1);
	return 0;
}

/*
	char* obj_method(int method);
	Implementation of access to method of object.
*/
char* obj_method(int method){
	// $v0 contains the address of object.
	char* err;
	int stack,opos;
	// Parameters preparation (to $s5) here.
	next_position();
	opos=g_objpos;

	// Begin parameter(s) construction routine
	// Note that this comment must be copied
	// when inserting simiar routine to source

	stack=12;
	g_object[g_objpos++]=0x27BD0000;             // addiu       sp,sp,-xx
	// 4(sp) is for $v0 (pointer to object)
	g_object[g_objpos++]=0xAFA20004;             // sw          v0,4(sp)
	if (g_source[g_srcpos]!=')') {
		g_srcpos--;
		do {
			g_srcpos++;
			stack+=4;
			err=get_stringFloatOrValue();
			if (err) return err;
			check_obj_space(1);
			g_object[g_objpos++]=0xAFA20000|stack;   // sw          v0,xx(sp)
			next_position();
		} while(g_source[g_srcpos]==',');
	}
	if (g_source[g_srcpos]!=')') return ERR_SYNTAX;
	g_srcpos++;
	// 8(sp) is for $s5, 12(sp) is for # of arguments
	check_obj_space(4);
	g_object[g_objpos++]=0xAFB50008;             // sw          s5,8(sp)
	g_object[g_objpos++]=0x34020000|(stack/4-3); // ori         v0,zero,xx
	g_object[g_objpos++]=0xAFA2000C;             // sw          v0,12(sp)
	g_object[g_objpos++]=0x27B50008;             // addiu       s5,sp,8
	g_object[opos]|=((0-stack)&0xFFFF);          // addiu       sp,sp,-xx (See above)

	// End parameter(s) construction routine
	// Note that this comment must be copied
	// when inserting simiar routine to source

	// Determine address of method and store fields to local variables.
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA20004;                           // lw          v0,4(sp)
	g_object[g_objpos++]=0x3C050000|((method>>16)&0x0000FFFF); // lui   a1,xxxx
	g_object[g_objpos++]=0x34A50000|(method&0x0000FFFF);       // ori a1,a1,xxxx
	call_quicklib_code(lib_pre_method,ASM_ADDU_A0_V0_ZERO);
	// Call method address here. Same routine for GOSUB statement with integer value is used.
	check_obj_space(6);
	g_object[g_objpos++]=0x04130003;                           // bgezall     zero,label1
	g_object[g_objpos++]=0x27BDFFFC;                           // addiu       sp,sp,-4
	g_object[g_objpos++]=0x10000003;                           // beq         zero,zero,label2
	g_object[g_objpos++]=0x00000000;                           // nop         
	                                                           // label1:
	g_object[g_objpos++]=0x00400008;                           // jr          v0
	g_object[g_objpos++]=0xAFBF0004;                           // sw          ra,4(sp)	                                                           // label2:
	// Restore fields from local variables.
	check_obj_space(3);
	g_object[g_objpos++]=0x8FA40004;                           // lw          a0,4(sp)
	g_object[g_objpos++]=0x3C050000|((method>>16)&0x0000FFFF); // lui   a1,xxxx
	g_object[g_objpos++]=0x34A50000|(method&0x0000FFFF);       // ori a1,a1,xxxx
	call_quicklib_code(lib_post_method,ASM_ADDU_A2_V0_ZERO);
	// Remove stack
	check_obj_space(2);
	g_object[g_objpos++]=0x8FB50008;                           // lw          s5,8(sp)
	g_object[g_objpos++]=0x27BD0000|stack;                     // addiu       sp,sp,xx	
	return 0;
}

/*
	char* integer_obj_field();
	char* string_obj_field();
	char* float_obj_field();
	Implementation of access to field of object.
	This feature is recursive. When an object is applied to the field of another object, 
	following expression is possible (for example):
		obj1.field1.field2
	
*/

#define OBJ_FIELD_INTEGER 0
#define OBJ_FIELD_STRING  '$'
#define OBJ_FIELD_FLOAT   '#'

char* _obj_field(char mode){
	// $v0 contains the address of object.
	int i;
	char* err;
	do {
		i=check_var_name(); // TODO: consider accepting reserbed var names for field name
		if (i<65536) return ERR_SYNTAX;
		if (g_source[g_srcpos]=='(' && mode==OBJ_FIELD_INTEGER) {
			// This is a method
			g_srcpos++;
			return obj_method(i);
		} else if (g_source[g_srcpos+1]=='(') {
			if (g_source[g_srcpos]==mode) {
				// This is a string/float method
				g_srcpos++;
				g_srcpos++;
				return obj_method(i);
			}
		} else if (g_source[g_srcpos]==mode && mode==OBJ_FIELD_STRING) {
			// This is a string field. Raise 31st bit.
			i|=0x80000000;
		}
		check_obj_space(2);
		g_object[g_objpos++]=0x3C050000|((i>>16)&0x0000FFFF); // lui   a1,xxxx
		g_object[g_objpos++]=0x34A50000|(i&0x0000FFFF);       // ori a1,a1,xxxx
		// First and second arguments are address of object and field name, respectively.
		call_quicklib_code(lib_obj_field,ASM_ADDU_A0_V0_ZERO);
		// Check if "." follows
		if (g_source[g_srcpos]=='.') {
			// "." found. $v0 is adress of an object. See the field.
			g_srcpos++;
			continue;
		}
	} while(0);
	// All done. Check variable type
	if (mode==OBJ_FIELD_INTEGER) return 0;
	else if (g_source[g_srcpos]==mode) {
		g_srcpos++;
		return 0;
	} else return ERR_SYNTAX;
}

char* integer_obj_field(){
	return _obj_field(OBJ_FIELD_INTEGER);
}

char* string_obj_field(){
	return _obj_field(OBJ_FIELD_STRING);
}

char* float_obj_field(){
	return _obj_field(OBJ_FIELD_FLOAT);
}

int lib_obj_field(int* object, int fieldname){
	int* class;
	int i,numfield;
	// Check if this is an object (if within the RAM).
	if (!withinRAM(object)) err_not_obj();
	class=(int*)object[0];
	if (!withinRAM(class)) err_not_obj();
	// Obtain # of public field
	numfield=class[1]&0xff;
	for(i=0;i<numfield;i++){
		if (class[2+i*2]==fieldname) break;
	}
	if (i==numfield) err_not_field(fieldname,class[0]);
	// Got address of field. Return value as $v0 and address as $v1.
	g_temp=(int)(&object[1+i]);
	asm volatile("la $v1,%0"::"i"(&g_temp));
	asm volatile("lw $v1,0($v1)");
	return object[1+i];
}

/*
	Library for letting string field 
*/

void lib_let_str_field(char* prev_str, char* new_str){
	int var_num=get_permanent_var_num();
	free_perm_str(prev_str);
	lib_let_str(new_str,var_num);
	return;
}

/*
	Library for calling method statement
*/

int lib_pre_method(int* object, int methodname){
	int i,num,nums;
	int* class;
	// Check if this is an object (if within the RAM).
	if (!withinRAM(object)) err_not_obj();
	class=(int*)object[0];
	if (!withinRAM(class)) err_not_obj();
	// Save object field values in local variables in class
	nums=class[1];
	num=nums&0xff;
	for(i=0;i<num;i++){
		// Public fields
		class+=2;
		g_var_mem[class[1]]=object[i+1];
		// When string, move from permanent block
		if (0x80000000&class[0]) move_from_perm_block_if_exists(class[1]);
	}
	num+=(nums>>8)&0xff;
	for(i=i;i<num;i++){
		// Private fields
		class+=2;
		g_var_mem[class[1]]=object[i+1];
		// When string/dimension, move from permanent block
		if (0x80000000&class[0]) move_from_perm_block_if_exists(class[1]);
	}
	// Seek method
	num+=(nums>>16)&0xff;
	for(i=i;i<num;i++){
		class+=2;
		if (class[0]==methodname) break;
	}
	if (i==num) {
		// Method not found
		class=(int*)object[0];
		err_not_field(methodname,class[0]);
	}
	// Method found. return it.
	return class[1];
}

int lib_post_method(int* object, int methodname, int v0){
	// Note that existence of the method was checked in above function before reaching this function.
	int i,num,nums;
	int* class;
	// Restore local variables to object field values
	class=(int*)object[0];
	nums=class[1];
	num=nums&0xff;
	for(i=0;i<num;i++){
		// Public fields
		class+=2;
		object[i+1]=g_var_mem[class[1]];
		// When string, move to permanent block
		if (0x80000000&class[0]) {
			if (g_var_size[class[1]]) move_to_perm_block(class[1]);
		}
	}
	num+=(nums>>8)&0xff;
	for(i=i;i<num;i++){
		// Private fields
		class+=2;
		object[i+1]=g_var_mem[class[1]];
		// When string/dimension, move to permanent block
		if (0x80000000&class[0]) {
			if (g_var_size[class[1]]) move_to_perm_block(class[1]);
		}
	}
	// all done
	return v0;
}

/*
	Method statement
*/

char* method_statement(){
	char* err;
	int data[2];
	int opos=g_objpos;
	// This statement is valid only in class file.
	if (!g_compiling_class) return ERR_INVALID_NON_CLASS;
	// Insert label for setting $s6
	err=label_statement();
	if (err) return err;
	// Register cmpdata
	data[0]=g_label;
	data[1]=(int)(&g_object[opos]);
	return cmpdata_insert(CMPDATA_FIELD,PUBLIC_METHOD,(int*)&data[0],2);
}

/*
	Delete statement
*/

char* delete_statement(){
	char* err;
	next_position();
	g_srcpos--;
	do{
		g_srcpos++;
		err=get_value();
		if (err) return err;
		call_quicklib_code(lib_delete,ASM_ADDU_A0_V0_ZERO);
		next_position();
	} while (g_source[g_srcpos]==',');
	return 0;
}

/*
	Call statement
*/

char* call_statement(){
	// Just get an integer value. That is it.
	return get_value();
}

/*
	Static statement
*/

char* static_statement(){
	char* err;
	int data[2];
	int i;
	int is_private=0;
	// This statement is valid only in class file.
	if (!g_compiling_class) return ERR_INVALID_NON_CLASS;
	// Check which private or public
	next_position();
	if (nextCodeIs("PRIVATE ")) {
		is_private=1;
	} else if (nextCodeIs("PUBLIC ")) {
		is_private=0;
	}
	do {
		next_position();
		i=check_var_name();
		if (i<65536) return ERR_SYNTAX;
		// Register varname
		err=register_var_name(i);
		if (err) return err;
		if (g_source[g_srcpos]=='#' || g_source[g_srcpos]=='$') {
			g_srcpos++;
		}
		// Register public static field
		if (!is_private) {
			data[0]=g_compiling_class; // class name as integer
			data[1]=i;                 // static var name as integer
			i=search_var_name(i);      // var number of this static field
			if (i<0) return ERR_UNKNOWN;
			err=cmpdata_insert(CMPDATA_STATIC,i,(int*)&data[0],2);
			if (err) return err;
		}
		next_position();
		if (g_source[g_srcpos]==',') {
			g_srcpos++;
		} else {
			break;
		}
	} while(1);
	return 0;
	
}

/*
	Static method
		Type is either 0, '$', or '#', 
		for integer, string, or float
*/

char* static_method(char type){
	char* err;
	int* data;
	int i,opos,method,stack;
	next_position();
	// Check class name
	i=check_var_name();
	if (i<65536) return ERR_SYNTAX;
	// Check if the class exists
	cmpdata_reset();
	while(data=cmpdata_find(CMPDATA_CLASS)){
		if (data[1]==i) {
			// The class was already defined.
			i=0;
			break;
		}
	}
	// Check '::'
	if (g_source[g_srcpos]!=':') return ERR_SYNTAX;
	g_srcpos++;
	if (g_source[g_srcpos]!=':') return ERR_SYNTAX;
	g_srcpos++;
	if (i) return ERR_NO_CLASS;
	data=(int*)data[2];
	// Check method
	i=check_var_name();
	if (i<65536) return ERR_SYNTAX;
	method=(int)search_method(data,i);
	if (!method) return ERR_NOT_FIELD;
	// Check type and '('
	if (type) {// Either 0, '$', or '#'
		if (g_source[g_srcpos]!=type) return ERR_SYNTAX;
		g_srcpos++;

	}
	if (g_source[g_srcpos]!='(') return ERR_SYNTAX;
	g_srcpos++;

	// Begin parameter(s) construction routine
	// Note that this comment must be copied
	// when inserting simiar routine to source

	stack=8;
	opos=g_objpos;
	g_object[g_objpos++]=0x27BD0000;           // addiu       sp,sp,-xx
	while(g_source[g_srcpos]==',') {
		g_srcpos++;
		stack+=4;
		err=get_stringFloatOrValue();
		if (err) return err;
		check_obj_space(1);
		g_object[g_objpos++]=0xAFA20000|stack; // sw          v0,xx(sp)
		next_position();
	}
	// 4(sp) is for $s5, 8(sp) is for # of parameters
	check_obj_space(5);
	g_object[g_objpos++]=0xAFB50004;             // sw          s5,4(sp)
	g_object[g_objpos++]=0x34020000|(stack/4-2); // ori         v0,zero,xx
	g_object[g_objpos++]=0xAFA20008;             // sw          v0,8(sp)
	g_object[g_objpos++]=0x27B50004;             // addiu       s5,sp,4
	g_object[opos]|=((0-stack)&0xFFFF);          // addiu       sp,sp,-xx (See above)

	// End parameter(s) construction routine
	// Note that this comment must be copied
	// when inserting simiar routine to source

	// Calling subroutine, which is static method of class
	check_obj_space(6);
	g_object[g_objpos++]=0x04130003;                            // bgezall     zero,label1
	g_object[g_objpos++]=0x27BDFFFC;                            // addiu       sp,sp,-4
	g_object[g_objpos++]=0x10000003;                            // beq         zero,zero,label2
	g_object[g_objpos++]=0x00000000;                            // nop         
	                                                            // label1:
	g_object[g_objpos++]=0x08000000|((method&0x0FFFFFFF)>>2);   // j           xxxx
	g_object[g_objpos++]=0xAFBF0004;                            // sw          ra,4(sp)
		                                                            // label2:	
	// Remove stack
	check_obj_space(2);
	g_object[g_objpos++]=0x8FB50004;           // lw          s5,4(sp)
	g_object[g_objpos++]=0x27BD0000|stack;     // addiu       sp,sp,xx

	return 0;
}

/*
	Let object.field statement
*/

char* let_object_field(){
	char* err;
	char b3;
	int spos,opos;
	// $v0 contains the pointer to object
	spos=g_srcpos;
	opos=g_objpos;
	// Try string field, first
	err=string_obj_field();
	if (err) {
		// Integer or float field
		g_srcpos=spos;
		g_objpos=opos;
		err=integer_obj_field();
		if (err) return err;
		b3=g_source[g_srcpos];
		if (b3=='#') g_srcpos++;
	} else {
		// String field
		b3='$';
	}
	if (g_source[g_srcpos-1]==')') {
		// This is a CALL statement
		return 0;
	}
	// $v1 is address to store value. Save it in stack.
	check_obj_space(1);
	g_object[g_objpos++]=0x27BDFFFC;              // addiu sp,sp,-4
	g_object[g_objpos++]=0xAFA30004;              // sw    v1,4(sp)
	if (b3=='$') {
		// String field
		// Get value
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_string();
	} else if (b3=='#') {
		// Float field
		// Get value
		next_position();
		if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
		g_srcpos++;
		err=get_float();
	} else {
			// Integer field
			// Get value
			next_position();
			if (g_source[g_srcpos]!='=') return ERR_SYNTAX;
			g_srcpos++;
			err=get_value();
	}
	if (err) return err;
	// Store in field of object
	check_obj_space(4);
	g_object[g_objpos++]=0x8FA30004;              // lw    v1,4(sp)
	g_object[g_objpos++]=0x27BD0004;              // addiu sp,sp,4
	g_object[g_objpos++]=0x8C640000;              // lw    a0,0(v1)
	g_object[g_objpos++]=0xAC620000;              // sw    v0,0(v1)
	// Handle permanent block for string field
	if (b3=='$') call_quicklib_code(lib_let_str_field,ASM_ADDU_A1_V0_ZERO);
	return 0;
}