/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

#include <xc.h>
#include "api.h"
#include "editor.h"
#include "compiler.h"
#include "main.h"

unsigned char currentfile[13],tempfile[13]; //�ҏW���̃t�@�C�����A�ꎞ�t�@�C����

void wait60thsec(unsigned short n){
	// 60����n�b�E�F�C�g�i�r�f�I��ʂ̍ŉ��s�M���o�͏I���܂ő҂j
	n+=drawcount;
	while(drawcount!=n) asm(WAIT);
}
