/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka & Katsumi
   http://www.ze.em-net.ne.jp/~kenken/index.html
   http://hp.vector.co.jp/authors/VA016157/
*/

// main.c
// MachiKania BASIC System Ver Zoea
// KM-BASIC 統合開発実行環境 for PIC32MX170F256B / PIC32MX270F256B by K.Tanaka

// 利用システム
// ps2keyboard.X.a : PS/2キーボード入力システムライブラリ
// lib_colortext32.a : カラービデオ信号出力システムライブラリ（30×27テキスト版）
// libsdfsio.a ： SDカードアクセス用ライブラリ

/*
	PIC32MX ペリフェラル使用状況
	
	割り込み
		NTSC,   Timer2, vector  8, priority 5
		NTSC,   OC3,    vector 14, priority 5
		NTSC,   OC2,    vector 10, priority 5
		NTSC,   OC1,    vector  6, priority 5
		PS/2,   CNF,    vector 33, priority 6
		PS/2,   Timer5, vector 20, priority 4
		MUSIC,  CS0,    vector  1, priority 2
		TIMER,  Timer1, vector  4, priority 3
		INT,    CS1,    vector  2, priority 1
	
	タイマー
		Timer1 BASIC用タイマー
		Timer2 NTSC
		Timer3 MUSIC/PWM
		Timer4 MUSIC
		Timer5 PS/2
	
	DMA
		DMA0 未使用
		DMA1 未使用
		DMA2 MUSIC
		DMA3 PS/2
	
	Output compair
		OC1 NTSC
		OC2 NTSC
		OC3 NTSC
		OC4 MUSIC/PWM
		OC5 
		
	SPI
		SPI1 未使用
		SPI2 マルチメディアカード
	
	I2C
		I2C1 未使用
		I2C2 未使用
	
	ポート使用
		A0  MMC
		A1  PS2/button 切換え
		A2  Crystal
		A3  Crystal
		A4  MMC
		B0  NTSC
		B1  NTSC
		B2  NTSC
		B3  NTSC
		B4  NTSC
		B5  MMC
		B6  未使用
		B7  button
		B8  PS2/button
		B9  PS2/button
		B10 button
		B11 button
		B12 未使用
		B13 audio out
		B14 button
		B15 MMC
*/

#include <xc.h>
#include "api.h"
#include "compiler.h"
#include "editor.h"
#include "keyinput.h"
#include "main.h"

//外付けクリスタル with PLL (16倍)
#pragma config PMDL1WAY = OFF, IOL1WAY = OFF
#pragma config FPLLIDIV = DIV_1, FPLLMUL = MUL_16, FPLLODIV = DIV_1
#pragma config FNOSC = PRIPLL, FSOSCEN = OFF, POSCMOD = XT, OSCIOFNC = OFF
#pragma config FPBDIV = DIV_1, FWDTEN = OFF, JTAGEN = OFF, ICESEL = ICS_PGx1
#pragma config FCKSM = CSECMD

#define mBMXSetRAMKernProgOffset(offset)	(BMXDKPBA = (offset))
#define mBMXSetRAMUserDataOffset(offset)	(BMXDUDBA = (offset))
#define mBMXSetRAMUserProgOffset(offset)	(BMXDUPBA = (offset))

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

void printhex8(unsigned char d){
	printchar("0123456789ABCDEF"[d>>4]);
	printchar("0123456789ABCDEF"[d&0x0f]);	
}

void printhex16(unsigned short d){
	printhex8(d>>8);
	printhex8(d&0x00ff);
}

void printhex32(unsigned int d){
	printhex16(d>>16);
	printhex16(d&0x0000ffff);
}

int main(void){
	char *appname,*s;

	if(DEVCFG1 & 0x8000){
		// Set Clock switching enabled and reset
		NVMWriteWord(&DEVCFG1,DEVCFG1 & 0xffff7fff);
		SoftReset();
	}

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
	mBMXSetRAMKernProgOffset(PIC32MX_RAMSIZE-RAMSIZE);
	mBMXSetRAMUserDataOffset(PIC32MX_RAMSIZE);
	mBMXSetRAMUserProgOffset(PIC32MX_RAMSIZE);

	ps2mode(); //RA1オン（PS/2有効化マクロ）
	init_composite(); // ビデオメモリクリア、割り込み初期化、カラービデオ出力開始
	setcursor(0,0,COLOR_NORMALTEXT);

	// Show blue screen if exception before soft reset.
	blue_screen();

	printstr("MachiKania BASIC System\n");
	printstr(" Ver "SYSVER1" "SYSVER2" by KENKEN\n");
	printstr("BASIC Compiler "BASVER"\n");
	printstr(" by Katsumi\n\n");
	//SDカードファイルシステム初期化
	setcursorcolor(COLOR_NORMALTEXT);
	printstr("Init File System...");
	// Initialize the File System
	if(FSInit()==FALSE){ //ファイルシステム初期化
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
	while(1) asm(WAIT);
}

