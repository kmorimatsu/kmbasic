/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka
   http://www.ze.em-net.ne.jp/~kenken/index.html
*/

#define TBUFMAXLINE 101 //テキストバッファ数
#define TBUFSIZE 200 //テキストバッファ1つのサイズ
#define TBUFMAXSIZE (TBUFSIZE*(TBUFMAXLINE-1)) //最大バッファ容量（バッファ1行分空ける）
#define EDITWIDTHX 30 //エディタ画面横幅
#define EDITWIDTHY 26 //エディタ画面縦幅
#define COLOR_NORMALTEXT 7 //通常テキスト色
#define COLOR_ERRORTEXT 4 //エラーメッセージテキスト色
#define COLOR_AREASELECTTEXT 4 //範囲選択テキスト色
#define COLOR_BOTTOMLINE 5 //画面最下行の色
#define FILEBUFSIZE 256 //ファイルアクセス用バッファサイズ
#define MAXFILENUM 200 //利用可能ファイル最大数

#define ERR_FILETOOBIG -1
#define ERR_CANTFILEOPEN -2
#define ERR_CANTWRITEFILE -3

#define TEMPFILENAME "~TEMP.BAS"
#define INIFILE "MACHIKAN.INI" // 初期設定ファイル
#define HEXFILE "MACHIKAN.HEX" // 実行中HEXファイル名がこれと一致した場合はエディタ起動

void texteditor(void); //テキストエディタ本体
int runbasic(char *s,int test); //コンパイルして実行
extern unsigned char tempfile[13];
