/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/
// キー入力、カーソル表示関連機能 by K.Tanaka
// PS/2キーボード入力システム、カラーテキスト出力システム利用

#include "videoout.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "plib.h"

volatile unsigned short vkey; //仮想キーコード
unsigned short keystatus,keystatus2,oldkey; //最新のボタン状態と前回のボタン状態

int lineinput(char *s,int n){
	return 0;
}

unsigned short readButton(void){
	unsigned short k;
	LCD_CS_HI;
	asm("nop");
	KEY_EN;
	asm("nop");
	TRISBSET=KEYMASK;
	asm("nop");
	k=~KEYPORT & KEYMASK;
	KEY_DS;
	asm("nop");
	TRISBCLR=KEYMASK;
	asm("nop");
	LCD_CS_LO;
	return k;
}

unsigned char ps2readkey(){
	return 0;
}