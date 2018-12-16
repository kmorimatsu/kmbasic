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
		cstruct[0]: class name as integer
		cstruct[1]: number of fields and methods:
		              bit 0-7:   # of public fields
		              bit 8-15:  # of private fields
		              bit 16-23: # of public methods
		              bit 24-31: reserved
		cstruct[x]: public fields name
		cstruct[y]: private fields name
		cstruct[z], cstruct[z+1]: public methods name and pointer
*/

char* construct_class_structure(int class){
	int* record;
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
			check_obj_space(1);
			g_object[g_objpos++]=record[1];
		}
	}
	// Private fields
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if ((record[0]&0xffff)==PRIVATE_FIELD) {
			num+=1<<8;
			check_obj_space(1);
			g_object[g_objpos++]=record[1];
		}
	}
	// Public methods
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if ((record[0]&0xffff)==PUBLIC_METHOD) {
			num+=1<<16;
			check_obj_space(2);
			g_object[g_objpos++]=record[1];
			g_object[g_objpos++]=record[2];
		}
	}
	// Update number info
	g_class_structure[1]=num;
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

void construct_class_list(){
	int* record;
	int pclass;
	int num;
	// Initialize
	cmpdata_reset();
	num=0;
	g_classlist=&g_object[g_objpos];
	// List up classes
	while(record=cmpdata_find(CMPDATA_CLASS)){
		num++;
		pclass=record[2];
		// Delete CMPDATA, first
		cmpdata_delete(record);
		// There must be area because a CMPDATA was deleted above
		g_object[g_objpos++]=pclass;
	}
	if (num) {
		// Show the number of classes used.
		printstr(" Using ");
		printnum(num);
		printstr(" classes. ");
	} else {
		// Disable class list
		g_classlist=0;
	}
}

void* search_method(int* classdata,int method){
	int pos,i;
	int nums=classdata[1];
	pos=1;
	pos+=nums&0xff;      // exclude public field
	pos+=(nums>>8)&0xff; // exclude private field
	for(i=2*((nums>>16)&0xff-1);i>=0;i=i-2){ // Start loop at the last of method
		if (classdata[pos+i]==method) return (void*)classdata[pos+i+1];
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
	// Get class data from cmpdata
	cmpdata_reset();
	while(data=cmpdata_find(CMPDATA_CLASS)){
		if (data[1]==class) break;
	}
	if (!data) return ERR_NO_CLASS;
	classdata=(int*)data[2];
	size=object_size(classdata);
	// Check if INIT method exists
	init_method=search_method(classdata,LABEL_INIT);
	if (!init_method) {
		// INIT method does not exist
		// Create object
		call_quicklib_code(lib_calloc_memory,ASM_ORI_A0_ZERO_|(size+1));
		// First word of object is pointer to classdata
		check_obj_space(3);
		g_object[g_objpos++]=0x3C080000|(((unsigned int)classdata)>>16);        // lui         t0,0x1234
		g_object[g_objpos++]=0x35080000|(((unsigned int)classdata)&0x0000FFFF); // ori         t0,t0,0x5678
		g_object[g_objpos++]=0xAC480000; // sw          t0,0(v0)
		// All done
		return 0;
	}
	// INIT method exists
	// Construct stacks for parameters
	stack=0;
	opos=g_objpos;
	check_obj_space(1);
	g_object[g_objpos++]=0x27BD0000;               // addiu       sp,sp,-xx
	// Check if there is/are parameter(s)
	next_position();
	if (g_source[g_srcpos]==',') {
		// Parameter(s) exist(s)
		stack=+4;
		while(g_source[g_srcpos]==','){
			g_srcpos++;
			stack+=4;
			err=get_stringFloatOrValue();
			if (err) return err;
			check_obj_space(1);
			g_object[g_objpos++]=0xAFA20000|stack; // sw          v0,xx(sp)
			next_position();
		}
		check_obj_space(2);
		g_object[g_objpos++]=0xAFB50004;           // sw          s5,4(sp)
		g_object[g_objpos++]=0x03A0A821;           // addu        s5,sp,zero
	}
	stack+=4; // For $v0 (see below)
	g_object[opos]|=((0-stack)&0xFFFF);            // addiu       sp,sp,-xx (See above)
	// Create object
	// Note that $v0 will contain the address of object
	call_quicklib_code(lib_calloc_memory,ASM_ORI_A0_ZERO_|size);
	check_obj_space(1);
	g_object[g_objpos++]=0xAFA20000;               // sw          v0,0(sp)
	// Call INIT method
	check_obj_space(6);
	g_object[g_objpos++]=0x04130001;               // bgezall     zero,label1
	g_object[g_objpos++]=0x27BDFFFC;               // addiu       sp,sp,-4
	                                               //label1:
	g_object[g_objpos++]=0x27FF000C;               // addiu       ra,ra,12
	g_object[g_objpos++]=0x08000000|
		((((int)init_method)>>2)&0x03ffffff);      // j           init_method
	g_object[g_objpos++]=0xAFBF0004;               // sw          ra,4(sp)
	// Reload #v0
	g_object[g_objpos++]=0x8FA20000|stack;         // lw          v0,xx(sp)
	// Remove stack
	check_obj_space(2);
	if (4<stack) {
		g_object[g_objpos++]=0x8FB50008;           // lw          s5,4(sp)
	}
	g_object[g_objpos++]=0x27BD0000|stack;         // addiu       sp,sp,xx
	// All done. $v0 will be
	return 0;

}

char* field_statement(){
// TODO: This statement is only valid in class definition code.
	char* err;
	int i;
	int data[1];
	int is_private=0;
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
	TODO:
	1. Improve gosub statement and args() function for args(0) being # of arguments.

*/