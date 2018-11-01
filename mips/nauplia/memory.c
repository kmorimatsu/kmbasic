/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include "compiler.h"

/*
	ALLOC_VAR_NUM:   # of variables used for allocation of memory (string/dimension).
	                 0 is for A, 1 is for B, ... , and 25 is for Z.
	                 This number also includes temporary area used for string construction etc.
	                 Temporary area is cleared every line of BASIC code in alloc_memory().
	ALLOC_BLOCK_NUM: # of blocks that can be used for memory allocation.
	                 This # also includes the ones for ALLOC_VAR_NUM.
	                 After ALLOC_VAR_NUM area, dedicated memory area information follows.
	                 Currently, only PCG is used in this block.
	                 Therefore, ALLOC_VAR_NUM+1 == ALLOC_BLOCK_NUM
*/

void set_free_area(void* begin, void* end){
	int i;
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		g_var_size[i]=0;
	}
	g_heap_mem=(int*)begin;
	g_max_mem=(int)((end-begin)/4);
}

void* calloc_memory(int size, int var_num){
	int i;
	void* ret;
	// Allocate memory
	ret=alloc_memory(size,var_num);
	// Fill zero in allocated memory
	for(i=0;i<size;i++){
		((int*)ret)[i]=0;
	}
	// return pointer to allocated memory
	return ret;
}
void* alloc_memory(int size, int var_num){
	// Remove temporary blocks once a line.
	asm volatile("nop");
	asm volatile("bltz $s6,_alloc_memory_main"); // Skip if $s6<0
	// Following code will be activated after setting $s6 register
	// every line and after label statement.
	asm volatile("subu $s6,$zero,$s6");          // $s6=0-$s6
	// Remove all temporary blocks
	// Note that g_var_size is short integer.
	// Note that ALLOC_VAR_NUM is used here (but not ALLOC_BLOC_NUM)
	//	for(i=26;i<ALLOC_VAR_NUM;i++)g_var_size[i]=0;
	asm volatile("addiu $v0,$zero,%0"::"n"((ALLOC_VAR_NUM-26)/2)); // $v0=(ALLOC_VAR_NUM-26)/2
	asm volatile("la $v1,%0"::"i"(&g_var_size[0]));                // $v1=g_var_size
	asm volatile("loop:");
	asm volatile("sw $zero,(26*2)($v1)");                          // $v1[26]=0, $v1[27]=0
	asm volatile("addiu $v0,$v0,-1");                              // $v0--
	asm volatile("addiu $v1,$v1,4");                               // $v1+=2
	asm volatile("bne $v0,$zero,loop");                            // loop if 0<$v0
	asm volatile("b _alloc_memory_main");
}
void* _alloc_memory_main(int size, int var_num){
	int i,j,candidate,after_this;
	// Assign temp var number
	if (var_num<0) {
		// Use ALLOC_VAR_NUM here but not ALLOC_BLOCK_NUM
		for(i=26;i<ALLOC_VAR_NUM;i++){
			if (g_var_size[i]==0) {
				var_num=i;
				break;
			}
		}
		if (var_num<0) {
			err_str_complex();
			return 0;
		}
	}
	while(1){
		// Try the block after last block
		candidate=0;
		for(i=0;i<ALLOC_BLOCK_NUM;i++){
			if (g_var_size[i]==0) continue;
			if (candidate<=g_var_pointer[i]) {
				candidate=g_var_pointer[i]+g_var_size[i];
			}
		}
		if (candidate+size<=g_max_mem) break;
		// Check between blocks
		candidate=-1;
		for(i=0;i<ALLOC_BLOCK_NUM;i++){
			after_this=g_max_mem;
			for(j=0;j<ALLOC_BLOCK_NUM;j++){
				if (g_var_pointer[i]<g_var_pointer[j] && g_var_pointer[j]<after_this) {
					after_this=g_var_pointer[j];
				}
			}
			if (g_var_pointer[i]+g_var_size[i]+size<=after_this) {
				// Possible block found.
				candidate=g_var_pointer[i]+g_var_size[i];
				// Check if this area is not used by others.
				for(j=0;j<ALLOC_BLOCK_NUM;j++){
					if (candidate+size<g_var_pointer[j]) continue;
					if (g_var_pointer[j]+g_var_size[j]<=candidate) continue;
					// This block is at least partly taken by another block.
					candidate=-1;
					break;
				}
				if (0<=candidate) break;
			}
		}
		if (0<=candidate) break;
		// New memory block cannot be allocated.
		err_no_mem();
		return 0;
	}
	// Available block found.
	g_var_pointer[var_num]=candidate;
	g_var_size[var_num]=size;
	g_var_mem[var_num]=(int)(&(g_heap_mem[candidate]));
	return (void*)g_var_mem[var_num];
}

void free_temp_str(char* str){
	int i,pointer;
	if (!str) return;
	pointer=(int)str-(int)g_heap_mem;
	pointer>>=2;
	for(i=0;i<ALLOC_BLOCK_NUM;i++){
		if (g_var_pointer[i]==pointer) {
			g_var_size[i]=0;
			break;
		}
	}
}