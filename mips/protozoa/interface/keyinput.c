/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/
// �L�[���́A�J�[�\���\���֘A�@�\ by K.Tanaka
// PS/2�L�[�{�[�h���̓V�X�e���A�J���[�e�L�X�g�o�̓V�X�e�����p

#include "videoout.h"
#include "ps2keyboard.h"
#include "keyinput.h"
#include "plib.h"

volatile unsigned short vkey; //���z�L�[�R�[�h
unsigned short keystatus,keystatus2,oldkey; //�ŐV�̃{�^����ԂƑO��̃{�^�����

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