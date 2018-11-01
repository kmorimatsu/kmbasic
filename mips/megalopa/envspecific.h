/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

// Megalopa uses I/O statements/functions
#include "io.h"

#define PERSISTENT_RAM_SIZE (1024*100)

int readbuttons();
void scroll(int x, int y);
void usegraphic(int mode);
void videowidth(int width);
int lib_system(int a0, int a1 ,int v0, int a3, int g_gcolor, int g_prev_x, int g_prev_y);
void init_env(void);
void pre_run(void);
void post_run(void);

// 30 or 40 characters per line for Zoea
#define printcomma() printstr("          "+rem10_32((unsigned int)(cursor-TVRAM)))

// Check break key or buttons when executing BASIC code.
// In PS/2 mode, detect ctrl-break.
// In button mode, detect pushing four buttons are pushed simultaneously.
#define check_break() \
	if (g_disable_break==0) {\
		if (ps2keystatus[0x03]) err_break();\
	}

// Megalopa specific lists of statements and functions
#define ADDITIONAL_STATEMENTS \
	"OUT "      ,out_statement,\
	"OUT8H "    ,out8h_statement,\
	"OUT8L "    ,out8l_statement,\
	"OUT16 "    ,out16_statement,\
	"PWM "      ,pwm_statement,\
	"SERIAL "   ,serial_statement,\
	"SERIALOUT ",serialout_statement,

#define ADDITIONAL_INT_FUNCTIONS \
	"IN("  ,    in_function,\
	"IN8H("  ,  in8h_function,\
	"IN8L("  ,  in8l_function,\
	"IN16("  ,  in16_function,\
	"ANALOG("  ,analog_function,\
	"SERIALIN(",serialin_function,

#define ADDITIONAL_STR_FUNCTIONS
#define ADDITIONAL_RESERVED_VAR_NAMES \
	0x00015045, /*OUT*/ \
	0x01975e81, /*OUT8H*/ \
	0x01975e85, /*OUT8L*/ \
	0x01975d7a, /*OUT16*/ \
	0x0001015b, /*IN*/ \
	0x0007dde1, /*IN8H*/ \
	0x0007dde5, /*IN8L*/ \
	0x0007dcda, /*IN16*/ \
	0x05f0a740, /*ANALOG*/ \
	0x00015596, /*PWM*/ \
	0x45f58f5d, /*SERIAL*/ \
	0x000163c6, /*SPI*/ \
	0x47093355, /*SPIOUT*/ \
	0x01fa1cff, /*SPIIN*/


#define EXTRA_MASK 0x003F
#define EXTRA_STEP 0x0001
enum extra{
	EXTRA_SYSTEM     =EXTRA_STEP*0,
	EXTRA_OUT        =EXTRA_STEP*1,
	EXTRA_OUT8H      =EXTRA_STEP*2,
	EXTRA_OUT8L      =EXTRA_STEP*3,
	EXTRA_OUT16      =EXTRA_STEP*4,
	EXTRA_IN         =EXTRA_STEP*5,
	EXTRA_IN8H       =EXTRA_STEP*6,
	EXTRA_IN8L       =EXTRA_STEP*7,
	EXTRA_IN16       =EXTRA_STEP*8,
	EXTRA_ANALOG     =EXTRA_STEP*9,
	EXTRA_PWM        =EXTRA_STEP*10,
	EXTRA_SERIALOUT  =EXTRA_STEP*11,
	EXTRA_SERIALIN   =EXTRA_STEP*12,
	EXTRA_SERIAL     =EXTRA_STEP*13,
	EXTRA_SPIOUT     =EXTRA_STEP*14,
	EXTRA_SPIIN      =EXTRA_STEP*15,
	EXTRA_SPI        =EXTRA_STEP*16,
	// MAX 63
};
