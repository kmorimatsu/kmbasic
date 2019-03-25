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

#define INTERRUPT_TIMER 1

// Timer value that increments every timer event
static int g_timer;

// Enable or disable interrupt
static char g_interrupt_enabled;
// Jump address when interrupt
static int g_int_vector;

void init_timer(){
	// Stop timer, first
	T1CON=0x0000;
	IEC0bits.T1IE=0;
	TMR1=0;
	PR1=0xffff;
	g_timer=0;
	// Disable interrupt
	IEC0bits.CS1IE=0;
	g_interrupt_enabled=0;
}

void stop_timer(){
	// Stop timer
	T1CON=0x0000;
	IEC0bits.T1IE=0;
	// Disable interrupt
	IEC0bits.CS1IE=0;
	g_interrupt_enabled=0;
}

// Interrupt handler
#ifndef __DEBUG
	// Timer1 is also used for debug mode
	#pragma interrupt T1Handler IPL2SOFT vector 4
#endif
void T1Handler(void){
	g_timer++;
	// Clear Timer1 interrupt flag
	IFS0bits.T1IF=0;
	// Raise CS1 interrupt flag if required
	IFS0bits.CS1IF=g_interrupt_enabled;
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

//CS1 ? Core Software Interrupt 1 2 2 IFS0<2> IEC0<2> IPC0<20:18> IPC0<17:16>
void BasicInt(int addr){
//TODO: here
	// $a0 is the address in BASIC code
	asm volatile(".set noreorder");
	asm volatile("jr $a0");
	asm volatile("nop");
}
#pragma interrupt CS1Handler IPL1SOFT vector 2
void CS1Handler(void){
	IFS0bits.CS1IF=0;
	BasicInt(g_int_vector);
}

void _lib_interrupt(int itype, int address){
	// Ignore itype in the first imprementation
	g_int_vector=address;
	g_interrupt_enabled=1;
	// CS1 interrupt: priority 1
	IPC0bits.CS1IP=1;
	IPC0bits.CS1IS=0;
	IEC0bits.CS1IE=1;
}

void lib_interrupt(int itype){
	// $ra is 2 word before the interrupt address
	asm volatile("addiu $a1,$ra,8");
	asm volatile("j _lib_interrupt");
}

char* interrupt_statement(){
	int itype;
	int i,opos;
	char* err;
	next_position();
	if (nextCodeIs("TIMER")) {		itype=INTERRUPT_TIMER;
	} else {
		return ERR_SYNTAX;
	}
	// Detect ','
	next_position();
	if (g_source[g_srcpos]!=',') return ERR_SYNTAX;	g_srcpos++;
	// Store address to call
	call_quicklib_code(lib_interrupt,ASM_ORI_A0_ZERO_|itype);
	check_obj_space(2);
	opos=g_objpos;
	g_object[g_objpos++]=0x10000000; // b   xxxxx
	g_object[g_objpos++]=0x00000000; // nop
	// Store $ra
	check_obj_space(2);
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
	// Complete B assembly
	g_object[opos]|=g_objpos-opos-1;
	return 0;
}