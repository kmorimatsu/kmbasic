/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

#include "compiler.h"

// Contain the valus of $gp (GPR of MIPS32)
int g_gp;

// Line data when compiling
int g_line;
int g_fileline;

// Contain the address to which return in "END" statement.
int g_end_addr;

// Following vars are used in value.c and string.c.
// These define the depth of stack pointer used for
// handling values and strings.
int g_sdepth;
int g_maxsdepth;

// Global vars associated to RAM
char* g_source;
int g_srcpos;
int* g_object;
int g_objpos;
int* g_objmax;
char RAM[RAMSIZE] __attribute__((persistent,address(0xA0008000-RAMSIZE)));

// Global area for vars A-Z and three temporary string pointers
int g_var_mem[ALLOC_BLOCK_NUM];
unsigned short g_var_pointer[ALLOC_BLOCK_NUM];
unsigned short g_var_size[ALLOC_BLOCK_NUM];

// Heap area
int* g_heap_mem;
int g_max_mem;

// Random seed
int g_rnd_seed;

// Enable/disable Break keys
char g_disable_break;

// Font data used for PCG
unsigned char* g_pcg_font;
