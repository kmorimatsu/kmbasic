/*
   This file is provided under the LGPL license ver 2.1.
   Written by Katsumi.
   http://hp.vector.co.jp/authors/VA016157/
   kmorimatsu@users.sourceforge.jp
*/

/*
	This file is shared by Megalopa and Zoea
*/

#include <xc.h>
#include "compiler.h"
#include "api.h"

/*
	32 different type interruptions are possible.
	See definition in compiler.h like:
		#define INTERRUPT_TIMER 0
		extern int g_interrupt_flags;
		extern int g_int_vector[];
		#define raise_interrupt_flag(x) do {\
*/

// Timer value that increments every timer event
static int g_timer;

// Interrupt types
static const void* interrupt_list[]={
	"TIMER",    (void*)INTERRUPT_TIMER,
	"DRAWCOUNT",(void*)INTERRUPT_DRAWCOUNT,
	"KEYS",     (void*)INTERRUPT_KEYS,
	"INKEY",    (void*)INTERRUPT_INKEY,
	"MUSIC",    (void*)INTERRUPT_MUSIC,
	"WAVE",     (void*)INTERRUPT_WAVE,
	ADDITIONAL_INTERRUPT_FUNCTIONS
};
#define NUM_INTERRUPT_TYPES ((sizeof(interrupt_list)/sizeof(interrupt_list[0]))/2)
// Flags for interrupt
int g_interrupt_flags;
// Jump address when interrupt
int g_int_vector[NUM_INTERRUPT_TYPES];

/*
	Initialize and termination
*/

void init_timer(){
	int i;
	// Stop timer, first
	T1CON=0x0000;
	IEC0bits.T1IE=0;
	TMR1=0;
	PR1=0xffff;
	g_timer=0;
	// Disable interrupt
	IEC0bits.CS1IE=0;
	for(i=0;i<NUM_INTERRUPT_TYPES;i++) g_int_vector[i]=0;
	// CS0 interrupt every 1/60 sec (triggered by Timer2)
	IPC0bits.CS0IP=3;
	IPC0bits.CS0IS=0;
	IFS0bits.CS0IF=0;
	IEC0bits.CS0IE=1;	
}

void stop_timer(){
	// Stop timer
	T1CON=0x0000;
	IEC0bits.T1IE=0;
	// Disable interrupt
	IEC0bits.CS1IE=0;
}

/*
	Timer interprtation
*/

// Interrupt handler
#ifndef __DEBUG
	// Timer1 is also used for debug mode
	#pragma interrupt T1Handler IPL2SOFT vector 4
#endif
void T1Handler(void){
	g_timer++;
	// Clear Timer1 interrupt flag
	IFS0bits.T1IF=0;
	// Raise TIMER interrupt flag
	raise_interrupt_flag(INTERRUPT_TIMER);
}

void lib_usetimer(int hz){
	int temppr1;
	// Stop timer, first
	T1CON=0x0000;
	IEC0bits.T1IE=0;
	TMR1=0;
	PR1=0xffff;
	if (!hz) {
		return;
	}
	// PR1 setting
	temppr1=CPU_CLOCK_HZ/hz;
	if (temppr1<=65536) {
		// no prescaler
		T1CON=0x0000;
		PR1=temppr1-1;
	} else if ((temppr1>>3)<=65536) {
		// 1/8 prescaler
		T1CON=0x0010;
		PR1=(temppr1>>3)-1;
	} else if ((temppr1>>6)<=65536) {
		// 1/64 prescaler
		T1CON=0x0020;
		PR1=(temppr1>>6)-1;
	} else if ((temppr1>>8)<=65536) {
		// 1/256 prescaler
		T1CON=0x0030;
		PR1=(temppr1>>8)-1;
	} else {
		err_invalid_param();
	}
	// Timer1 interrupt: priority 2
	IPC1bits.T1IP=2;
	IPC1bits.T1IS=0;
	IEC0bits.T1IE=1;
	// Start timer
	T1CONbits.ON=1;
}

char* usetimer_statement(){
	char* err;
	err=get_value();
	if (err) return err;
	call_quicklib_code(lib_usetimer,ASM_ADDU_A0_V0_ZERO);
	return 0;
}

char* timer_statement(){
	int i;
	char* err;
	err=get_value();
	if (err) return err;
	i=(int)(&g_timer);
	check_obj_space(3);
	g_object[g_objpos++]=0x3C030000|((i>>16)&0x0000FFFF); // lui v1,xxxx
	g_object[g_objpos++]=0x34630000|(i&0x0000FFFF);       // ori v1,v1,xxxx
	g_object[g_objpos++]=0xAC620000;                      // sw  v0,0(v1)
	return 0;
}

char* timer_function(){
	int i;
	i=(int)(&g_timer);
	check_obj_space(3);
	g_object[g_objpos++]=0x3C020000|((i>>16)&0x0000FFFF); // lui v0,xxxx
	g_object[g_objpos++]=0x34420000|(i&0x0000FFFF);       // ori v0,v0,xxxx
	g_object[g_objpos++]=0x8C420000;                      // lw  v0,0(v0)
	return 0;
}

/*
	Interrupt interprtation
	To cause interruption, use raise_interrupt_flag() macro
	For example,
		raise_interrupt_flag(INTERRUPT_TIMER);

*/

void BasicInt(int addr){
	// Note that $s0-$s7 values must be set again here.
	asm volatile(".set noreorder");
	// Set s5 for initial_s5_stack
	asm volatile("la $s5,%0"::"i"(&g_initial_s5_stack[2]));
	// Set s7 for easy calling call_library()
	asm volatile("la $s7,%0"::"i"(&call_library));
	// $a0 is the address in BASIC code
	asm volatile("jr $a0");
	asm volatile("nop");
}
#pragma interrupt CS1Handler IPL1SOFT vector 2
void CS1Handler(void){
	int i;
	// Store s0-s7, fp, and ra in stacks
	asm volatile("#":::"s0");
	asm volatile("#":::"s1");
	asm volatile("#":::"s2");
	asm volatile("#":::"s3");
	asm volatile("#":::"s4");
	asm volatile("#":::"s5");
	asm volatile("#":::"s6");
	asm volatile("#":::"s7");
	asm volatile("#":::"fp");
	asm volatile("#":::"ra");
	while(g_interrupt_flags){
		for(i=0;i<NUM_INTERRUPT_TYPES;i++){
			if (g_interrupt_flags & (1<<i)) {
				if (g_int_vector[i]) BasicInt(g_int_vector[i]);
				g_interrupt_flags &= (1<<i)^0xffff;
			}
		}
	}
	IFS0bits.CS1IF=0;
}

void lib_interrupt_main(int itype, int address){
	// Set address
	g_int_vector[itype]=address;
	// CS1 interrupt: priority 1
	IPC0bits.CS1IP=1;
	IPC0bits.CS1IS=0;
	IEC0bits.CS1IE=1;
}

void lib_interrupt(int itype){
	// $ra contains the address for interrupt
	// $ra is 2 word before the interrupt address
	asm volatile("addiu $a1,$ra,8");
	asm volatile("j lib_interrupt_main");
}

char* interrupt_statement(){
	int itype;
	int i,opos;
	char* err;
	int stop=0;
	// Check if STOP
	stop=nextCodeIs("STOP ");
	// Seek the interrupt
	for (i=0;i<NUM_INTERRUPT_TYPES;i++){
		if (nextCodeIs((char*)interrupt_list[i*2])) break;
	}
	if (i<NUM_INTERRUPT_TYPES) {
		// Interrupt found
		itype=(int)interrupt_list[i*2+1];
	} else {
		// Interrupt not found
		return ERR_SYNTAX;
	}
	// Compile INTERRUPT STOP
	if (stop) {
		// g_int_vector[itype]=0;
		i=(int)(&g_int_vector[itype]);
		i-=g_gp;
		check_obj_space(1);
		g_object[g_objpos++]=0xAF800000|(i&0x0000FFFF); // sw          zero,xxxx(gp)
		return 0;
	}
	// Detect ','
	next_position();
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;
	g_srcpos++;
	// Store address to call
	call_quicklib_code(lib_interrupt,ASM_ORI_A0_ZERO_|itype);
	check_obj_space(4);
	// Skip the region used for interrupt-calling
	opos=g_objpos;
	g_object[g_objpos++]=0x10000000; // b   xxxxx
	g_object[g_objpos++]=0x00000000; // nop
	// Begin interrupt-calling region
	// Store $ra
	g_object[g_objpos++]=0x27BDFFFC; // addiu       sp,sp,-4
	g_object[g_objpos++]=0xAFBF0004; // sw          ra,4(sp)
	// Compile as GOSUB statement
	err=gosub_statement();
	if (err) return err;
	// Retore $ra and return
	check_obj_space(3);
	g_object[g_objpos++]=0x8FBF0004; // lw          ra,4(sp)
	g_object[g_objpos++]=0x03E00008; // jr          ra
	g_object[g_objpos++]=0x08320004; // addiu       sp,sp,4
	// End interrupt-calling region
	// Complete B assembly
	g_object[opos]|=g_objpos-opos-1;
	return 0;
}

/*
	CS0 interrupt
	IPL3SOFT vector 1

	This interrupt is always active. Therefore, Do things as few as possible.
	1) Call music function if needed.
		MUSIC interrupt is taken by music.c
	2) Check buttons for KEYS interrupt
	3) Check PS/2 for INKEY interrupt
	4) DRAWCOUNT interrupt
*/

const int* keystatus=(int*)&ps2keystatus[0];

#pragma interrupt CS0Handler IPL3SOFT vector 1
void CS0Handler(void){
	static int s_keys=-1;
	static char s_inkey=0;
	int i;
	IFS0bits.CS0IF=0;
	// Call music function
	if (g_music_active) musicint();
	// The interrupts are valid only when CS1 is active
	if (IEC0bits.CS1IE) {
		// Raise DRAWCOUNT interrupt flag
		raise_interrupt_flag(INTERRUPT_DRAWCOUNT);
		// Check buttons
		if (0<=s_keys && s_keys!=(KEYPORT&(KEYUP|KEYDOWN|KEYLEFT|KEYRIGHT|KEYSTART|KEYFIRE))) {
			// Raise KEYS interrupt flag
			raise_interrupt_flag(INTERRUPT_KEYS);
		}
		s_keys=KEYPORT&(KEYUP|KEYDOWN|KEYLEFT|KEYRIGHT|KEYSTART|KEYFIRE);
		// Check PS/2 keyboard down
		if (g_int_vector[INTERRUPT_INKEY]) {
			for(i=0;i<64;i++){
				if (keystatus[i]) {
					// Raise INKEY interrupt flag
					if (!s_inkey) raise_interrupt_flag(INTERRUPT_INKEY);
					break;
				}
			}
			s_inkey=(i==64) ? 0:1;
		}
	}
}
/*
		for(i=0;i<256;i++){
			if (ps2keystatus[i]) return i;
		}
*/
