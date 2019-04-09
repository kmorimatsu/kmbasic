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

unsigned char currentfile[13],tempfile[13]; //編集中のファイル名、一時ファイル名

void wait60thsec(unsigned short n){
	// 60分のn秒ウェイト（ビデオ画面の最下行信号出力終了まで待つ）
	n+=drawcount;
	while(drawcount!=n) asm(WAIT);
}
