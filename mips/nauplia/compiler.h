/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/* Definitions */
// Number of variables (including temporary ones)
#define ALLOC_VAR_NUM 36
// Number of blocks that can be assigned for memory allocation (including ALLOC_VAR_NUM)
#define ALLOC_BLOCK_NUM 37
// Block # dedicated for PCG
#define ALLOC_PCG_BLOCK 36

// RAM size used for object and heap
#define RAMSIZE (1024*25)

/* Enums */
enum operator{
	OP_VOID=0,
	OP_OR  =1,
	OP_AND =2,
	OP_XOR =3,
	OP_EQ  =4,
	OP_NEQ =5,
	OP_LT  =6,
	OP_LTE =7,
	OP_MT  =8,
	OP_MTE =9,
	OP_ADD =10,
	OP_SUB =11,
	OP_MUL =12,
	OP_DIV =13,
	OP_REM =14
};
enum libs{
	LIB_SOUND,
	LIB_MUSICFUNC,
	LIB_MUSIC,
	LIB_SETDRAWCOUNT,
	LIB_DRAWCOUNT,
	LIB_PALETTE,
	LIB_BGCOLOR,
	LIB_CURSOR,
	LIB_CLS,
	LIB_COLOR,
	LIB_KEYS,
	LIB_RESTORE,
	LIB_READ,
	LIB_MIDSTR,
	LIB_CLEAR,
	LIB_DIV0,
	LIB_LETSTR,
	LIB_STRNCMP,
	LIB_RND,
	LIB_DEC,
	LIB_HEX,
	LIB_CHR,
	LIB_CONNECT_STRING,
	LIB_STRING,
	LIB_PRINTSTR,
	LIB_LABEL,
	LIB_DIM,
	LIB_VAL,
	LIB_INPUT,
	LIB_INKEY,
	LIB_USEPCG,
	LIB_PCG,
	LIB_SCROLL,
	LIB_WAIT,
};

/* Global vars */
extern int g_rnd_seed;
extern unsigned int g_label;
extern int g_sdepth;
extern int g_maxsdepth;
extern char* g_source;
extern int g_srcpos;
extern int g_line;
extern int g_fileline;
extern int* g_object;
extern int g_objpos;
extern int* g_objmax;
extern const char* g_err_str[];
extern const unsigned char g_priority[];
extern enum operator g_last_op;
extern int g_end_addr;
extern int g_gp;
extern char RAM[RAMSIZE];
extern int g_var_mem[ALLOC_BLOCK_NUM];
extern unsigned short g_var_pointer[ALLOC_BLOCK_NUM];
extern unsigned short g_var_size[ALLOC_BLOCK_NUM];
extern int* g_heap_mem;
extern int g_max_mem;
extern char g_disable_break;
extern unsigned char* g_pcg_font;

/* Prototypes */
int get_gp(void);
int get_fp(void);
void start_program(void* addr, void* memory);
void shift_obj(int* src, int* dst, int len);
char* compile_line(void);
int nextCodeIs(char* str);

void err_break(void);
void err_music(char* str);
void err_data_not_found(void);
void err_str_complex(void);
void err_label_not_found(void);
void err_no_mem(void);
void err_div_zero(void);
void err_unkonwn(void);
void err_unexp_next(void);
char* resolve_label(int s6);

void set_sound(unsigned long* data);
int musicRemaining(void);
void set_music(char* str);
void init_music(void);

char* statement(void);
char* gosub_statement();

char* function(void);
char* str_function(void);

void call_library(void);
void reset_dataread();

void free_temp_str(char* str);
void* alloc_memory(int size, int var_num);
void* calloc_memory(int size, int var_num);

char* link(void);
char* get_label(void);
void* search_label(unsigned int label);

char* get_string();

char* get_operator(void);
char* calculation(enum operator op);

char* get_simple_value(void);
char* get_value();

/* Error messages */
#define ERR_SYNTAX (char*)(g_err_str[0])
#define ERR_NE_BINARY (char*)(g_err_str[1])
#define ERR_NE_MEMORY (char*)(g_err_str[2])
#define ERR_DIV_0 (char*)(g_err_str[3])
#define ERR_NY_I (char*)(g_err_str[4])
#define ERR_LABEL_NF (char*)(g_err_str[5])
#define ERR_LABEL_LONG (char*)(g_err_str[6])
#define ERR_STR_COMPLEX (char*)(g_err_str[7])
#define ERR_DATA_NF (char*)(g_err_str[8])
#define ERR_UNKNOWN (char*)(g_err_str[9])
#define ERR_MUSIC (char*)(g_err_str[10])
#define ERR_MULTIPLE_LABEL (char*)(g_err_str[11])
#define ERR_BREAK (char*)(g_err_str[12])
#define ERR_UNEXP_NEXT (char*)(g_err_str[13])

/* Macros */

// Skip blanc(s) in source code
#define next_position() while(g_source[g_srcpos]==' ') {g_srcpos++;}

// Check if object area is not full.
#define check_obj_space(x) if (g_objmax<g_object+g_objpos+x) return ERR_NE_BINARY

// Returns priority of operator
#define priority(x) (int)g_priority[(int)x]

// Insert code for calling library
//02E0F809   jalr        ra,s7
//24070000   addiu       a3,zero,0000
#define call_lib_code(x) \
	check_obj_space(2);\
	g_object[g_objpos++]=0x02E0F809;\
	g_object[g_objpos++]=0x24070000|(x&0x0000FFFF)

// Division macro for unsigned long
// Valid for 31 bits for all cases and 32 bits for some cases
#define div32(x,y,z) ((((unsigned long long)((unsigned long)x))*((unsigned long long)((unsigned long)y)))>>z)

// Divide by 10 (valid for 32 bits)
#define div10_32(x) div32(x,0xcccccccd,35)
#define rem10_32(x) (x-10*div10_32(x))

// Divide by 36 (valid for 32 bits)
#define div36_32(x) div32(x,0xe38e38e4,37)
#define rem36_32(x) (x-36*div36_32(x))

// Check break key or buttons when executing BASIC code.
// In PS/2 mode, detect ctrl-break.
// In button mode, detect pushing four buttons are pushed simultaneously.
#define check_break() \
	if (g_disable_break==0) {\
		if (inPS2MODE()) {\
			if (ps2keystatus[0x03]) err_break();\
		} else {\
			if ((PORTB&0x4c80)==0) err_break();\
		}\
	}
