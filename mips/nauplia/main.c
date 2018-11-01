/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka
   http://www.ze.em-net.ne.jp/~kenken/index.html
*/

// main.c
// MachiKania BASIC System
// KM-BASIC 統合開発実行環境 for PIC32MX150F128B / PIC32MX250F128B by K.Tanaka

// 利用システム
// ps2keyboard.X.a : PS/2キーボード入力システムライブラリ
// lib_colortext32.a : カラービデオ信号出力システムライブラリ（30×27テキスト版）
// libsdfsio.a ： SDカードアクセス用ライブラリ

#include <xc.h>
#include "main.h"
#include "api.h"
#include "compiler.h"
#include "editor.h"
#include "keyinput.h"

//外付けクリスタル with PLL (16倍)
#pragma config PMDL1WAY = OFF, IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_1, FPLLMUL = MUL_16, FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL, FSOSCEN = OFF, POSCMOD = XT, OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1, FWDTEN = OFF, JTAGEN = OFF, ICESEL = ICS_PGx1

#define mBMXSetRAMKernProgOffset(offset)	(BMXDKPBA = (offset))
#define mBMXSetRAMUserDataOffset(offset)	(BMXDUDBA = (offset))
#define mBMXSetRAMUserProgOffset(offset)	(BMXDUPBA = (offset))

#define FILENAME_FLASH_ADDRESS 0x9D005800

// INIファイル指定キーワード（8文字以内）
const char InitKeywords[][9]={
	"106KEY","101KEY","NUMLOCK","CAPSLOCK","SCRLLOCK"
};

void freadline(char *s,FSFILE *fp){
// ファイルから1行読み込み、配列sに返す
// 最大8文字まで。9文字以上の場合無効
// #または0x20以下のコードを見つけた場合、以降は無視
// s:9バイト以上の配列
// fp:ファイルポインタ
	int n;
	char c,*p;
	n=0;
	p=s;
	*p=0;
	while(n<=8){
		if(FSfread(p,1,1,fp)==0 || *p=='\n'){
			*p=0;
			return;
		}
		if(*p=='#'){
			*p=0;
			break;
		}
		if(*p<=' '){
			if(n>0){
				*p=0;
				break;
			}
			continue;
		}
		p++;
		n++;
	}
	if(n>8) *s=0; //9文字以上の文字列の場合は無効
	//以降の文字は無視
	while(FSfread(&c,1,1,fp) && c!='\n') ;
}
int searchinittext(char *s){
// InitKeywords配列の中から文字列sを探し、位置した場合何番目かを返す
// 見つからなかった場合-1を返す
	int i;
	char *p1;
	const char *p2;
	for(i=0;i<sizeof(InitKeywords)/sizeof(InitKeywords[0]);i++){
		p1=s;
		p2=InitKeywords[i];
		while(*p1==*p2){
			if(*p1==0) return i;
			p1++;
			p2++;
		}
	}
	return -1;
}
void readinifile(void){
	FSFILE *fp;
	char inittext[9];

	fp=FSfopen(INIFILE,"r");
	if(fp==NULL) return;
	printstr("Initialization File Found\n");
	lockkey=0; //INIファイルが存在する場合、Lock関連キーはINIファイルに従う
	while(1){
		if(FSfeof(fp)) break;
		freadline(inittext,fp);
		switch(searchinittext(inittext)){
			case 0:
				keytype=0;//日本語キーボード
				break;
			case 1:
				keytype=1;//英語キーボード
				break;
			case 2:
				lockkey|=2;//Num Lock
				break;
			case 3:
				lockkey|=4;//CAPS Lock
				break;
			case 4:
				lockkey|=1;//Scroll Lock
				break;
		}
	}
	FSfclose(fp);
}
int main(void){
	char *appname,*s;

	/* ポートの初期設定 */
	TRISA = 0x0000; // PORTA全て出力
	ANSELA = 0x0000; // 全てデジタル
	TRISB = KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// ボタン接続ポート入力設定
	ANSELB = 0x0000; // 全てデジタル
	CNPUBSET=KEYSTART | KEYFIRE | KEYUP | KEYDOWN | KEYLEFT | KEYRIGHT;// プルアップ設定
	ODCB = 0x0300;	//RB8,RB9はオープンドレイン

	// 周辺機能ピン割り当て
	SDI2R=2; //RPA4:SDI2
	RPB5R=4; //RPB5:SDO2

	// Make RAM executable. See also "char RAM[RAMSIZE]" in globalvars.c
	mBMXSetRAMKernProgOffset(0x8000-RAMSIZE);
	mBMXSetRAMUserDataOffset(0x8000);
	mBMXSetRAMUserProgOffset(0x8000);

	ps2mode(); //RA1オン（PS/2有効化マクロ）
	init_composite(); // ビデオメモリクリア、割り込み初期化、カラービデオ出力開始
	setcursor(0,0,COLOR_NORMALTEXT);

	printstr("MachiKania BASIC System\n");
	printstr(" Ver "SYSVER1" "SYSVER2" by KENKEN\n");
	printstr("BASIC Compiler "BASVER"\n");
	printstr(" by Katsumi\n\n");
	//SDカードファイルシステム初期化
	setcursorcolor(COLOR_NORMALTEXT);
	printstr("Init File System...");
	// Initialize the File System
	if(FSInit()==NULL){ //ファイルシステム初期化
		//エラーの場合停止
		setcursorcolor(COLOR_ERRORTEXT);
		printstr("\nFile System Error\n");
		printstr("Insert Correct Card\n");
		printstr("And Reset\n");
		while(1) asm("wait");
	}
	printstr("OK\n");
	lockkey=2; // NumLockキーオン
	keytype=0; // 日本語キーボード
	readinifile(); //INIファイル読み込み
	printstr("Init PS/2...");
	if(ps2init()){ //PS/2初期化
		//キーボードが見つからない場合
		printstr("Keyboard Not Found\n");
	}
	else printstr("OK\n");

	wait60thsec(60); //1秒待ち

	// 実行中HEXファイル名がHEXFILEと一致した場合はエディタ起動
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=HEXFILE;
	while(*s++==*appname++) if(*s==0) texteditor(); //テキストエディター呼び出し

	// 実行中HEXファイル名の「.HEX」を「.BAS」に置き換えてBASファイルを実行
	appname=(char*)FILENAME_FLASH_ADDRESS;
	s=tempfile;
	while(*appname!='.') *s++=*appname++;
	appname=".BAS";
	while(*appname!=0) *s++=*appname++;
	*s=0;
	// buttonmode(); //ボタン有効化
	g_disable_break=1; // Breakキー無効化
	runbasic(tempfile,0);
	while(1) asm("wait");
}
