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
	int num;
	int* record;
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
		if (record[0]&0xffff==PUBLIC_FIELD) {
			num+=1<<0;
			check_obj_space(1);
			g_object[g_objpos++]=record[1];
		}
	}
	// Private fields
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if (record[0]&0xffff==PRIVATE_FIELD) {
			num+=1<<8;
			check_obj_space(1);
			g_object[g_objpos++]=record[1];
		}
	}
	// Public methods
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_FIELD)){
		if (record[0]&0xffff==PUBLIC_METHOD) {
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
	}	
	// Delete longvar data
	cmpdata_reset();
	while(record=cmpdata_find(CMPDATA_USEVAR)){
		cmpdata_delete(record);
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
