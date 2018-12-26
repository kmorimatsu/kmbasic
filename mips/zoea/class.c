/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
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
			g_class_structure[i*2+1]=search_var_name(g_class_structure[i*2])+ALLOC_LNV_BLOCK
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
		if (classdata[i]==method) return (void*)classdata[i+1];
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
		if (g_source[g_srcpos]=='#' || g_source[g_srcpos]=='$') g_srcpos++;
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
	stack=12;
	g_object[g_objpos++]=0x27BD0000;             // addiu       sp,sp,-xx
	// 4(sp) is for $v0 (method name)
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
	Implementation of access to field of object.
	This feature is recursive. When an object is applied to the field of another object, 
	following expression is possible (for example):
		obj1.field1.field2
	
*/
char* integer_obj_field(){
	// $v0 contains the address of object.
	int i;
	char* err;
	do {
		i=check_var_name(); // TODO: consider accepting reserbed var names for field name
		if (i<65536) return ERR_SYNTAX;
		if (g_source[g_srcpos]=='(') {
			// This is a method
			g_srcpos++;
			return obj_method(i);
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
	return 0;
}

unsigned long long lib_obj_field(int* object, int fieldname){
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
	return (((unsigned long long)(unsigned long)(&object[1+i]))<<32) | (unsigned long long)object[1+i];
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
	}
	num+=(nums>>8)&0xff;
	for(i=i;i<num;i++){
		// Private fields
		class+=2;
		g_var_mem[class[1]]=object[i+1];
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
	}
	num+=(nums>>8)&0xff;
	for(i=i;i<num;i++){
		// Private fields
		class+=2;
		object[i+1]=g_var_mem[class[1]];
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
	int i;
	char* err;
	next_position();
	g_srcpos--;
	do{
		g_srcpos++;
		i=get_var_number();
		if (i<0) return ERR_SYNTAX;
		call_quicklib_code(lib_delete,ASM_LW_A0_XXXX_S8|(i*4));
		                                              // lw a0,xxxx(s8)
		check_obj_space(1);
		g_object[g_objpos++]=0xAFC00000|(i*4);        // sw zero,xxx(s8)
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