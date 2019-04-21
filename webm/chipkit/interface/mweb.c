#include <xc.h>
#include "../api.h"

/*
	TODO:
		set_videomode()の際の、ビデオモード切替。
			フォント切換え：lib_video_megalopa内で成される。fontdataを表示に使うこと。
			表示モード：videomodeを参照。videomode>=16でグラフィック、micdemode<16でキャラクター
		タイマー２の割り込み処理
			TMR=0とし、割り込み用の関数を呼び出す。表示は、OC2, OC5の割り込みで成されるが、こちらは無視する。
		ソフトウェア割り込み処理
			音楽再生など
		


*/

int html5data[4];
void html5returnValue();

// Construct jump assembly in boot area.
const unsigned int _debug_boot[] __attribute__((address(0xBFC00000))) ={
	0x0B401C00,//   j           0x9d007000
	0x00000000,//   nop         
};

// Data area for html5
const unsigned int _html5_data[] __attribute__((address(0x9D006000))) ={
	(unsigned int)(&TVRAM[0]),
	(unsigned int)(&FontData[0]),
	(unsigned int)(&html5data[0]),
	(unsigned int)(&ps2keystatus[0]),
	(unsigned int)(&vkey),
};

const unsigned int _html5_func[] __attribute__((address(0x9D006080))) ={
	(unsigned int)html5returnValue,
	(unsigned int)shiftkeys,
	(unsigned int)ps2readkey,
};

// For returning value from HTML5
void html5returnValue(){
	// $v0=html5data[0];
	asm volatile("la $v0,%0"::"i"(&html5data[0]));
	asm volatile("lw $v0,0($v0)");
}

/*

	Implementation of file library

*/

//void FSGetDiskProperties(FS_DISK_PROPERTIES* properties){}
//int FSCreateMBR (unsigned long firstSector, unsigned long numSectors){}
//int FSerror (void){}
//int SetClockVars (unsigned int year, unsigned char month, unsigned char day, unsigned char hour, unsigned char minute, unsigned char second){}
//int FSrmdir (char * path, unsigned char rmsubdirs){}
//int FSattrib (FSFILE * file, unsigned char attributes){}
//int FSformat (char mode, long int serialNumber, char * volumeID){}
int FindFirst (const char * fileName, unsigned int attr, SearchRec * rec){}
int FindNext (SearchRec * rec){}
int FSmkdir (char * path){}
char * FSgetcwd (char * path, int numbchars){}
int FSchdir (char * path){}
size_t FSfwrite(const void *data_to_write, size_t size, size_t n, FSFILE *stream){}
int FSremove (const char * fileName){}
int FSrename (const char * fileName, FSFILE * fo){}
int FSfeof( FSFILE * stream ){}
long FSftell(FSFILE *fo){}
int FSfseek(FSFILE *stream, long offset, int whence){}
size_t FSfread(void *ptr, size_t size, size_t n, FSFILE *stream){}
void FSrewind (FSFILE *fo){}
int FSfclose(FSFILE *fo){}
FSFILE * FSfopen(const char * fileName, const char *mode){}
int FSInit(void){}

/*
	Implementation of PS/2 keyboard library
*/

volatile unsigned char ps2keystatus[256]; // 仮想コードに相当するキーの状態（Onの時1）
volatile unsigned short vkey; //仮想キーコード
unsigned char lockkey; // 初期化時にLockキーの状態指定。下位3ビットが<CAPSLK><NUMLK><SCRLK>
unsigned char keytype; // キーボードの種類。0：日本語109キー、1：英語104キー

int ps2init(){ return 0; } // PS/2ライブラリ関連初期化。正常終了0、エラーで-1を返す
unsigned char shiftkeys(){ return 0; } // SHIFT関連キーの押下状態を返す
unsigned char ps2readkey(){ return 0; }
// 入力された1つのキーのキーコードをグローバル変数vkeyに格納（押されていなければ0を返す）
// 下位8ビット：キーコード
// 上位8ビット：シフト状態（押下：1）、上位から<0><CAPSLK><NUMLK><SCRLK><Win><ALT><CTRL><SHIFT>
// 英数・記号文字の場合、戻り値としてASCIIコード（それ以外は0を返す）
