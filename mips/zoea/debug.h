/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#ifdef __DEBUG

// Pseudo reading config setting for debug mode
extern unsigned int g_DEVCFG1;
#define DEVCFG1 g_DEVCFG1

// Do not use PS/2 keyboard
#define ps2init() not_ps2init_but_init_Timer1()
int not_ps2init_but_init_Timer1();

// Do not use set_graphmode()
#define set_graphmode(m) (0)

// Do not use asm("wait") but use asm("nop")
#undef WAIT
#define WAIT "nop"

#endif // __DEBUG
