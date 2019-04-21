#include <xc.h>
#include "../api.h"

/*
	TODO:
		set_videomode()�̍ۂ́A�r�f�I���[�h�ؑցB
			�t�H���g�؊����Flib_video_megalopa���Ő������Bfontdata��\���Ɏg�����ƁB
			�\�����[�h�Fvideomode���Q�ƁBvideomode>=16�ŃO���t�B�b�N�Amicdemode<16�ŃL�����N�^�[
		�^�C�}�[�Q�̊��荞�ݏ���
			TMR=0�Ƃ��A���荞�ݗp�̊֐����Ăяo���B�\���́AOC2, OC5�̊��荞�݂Ő�����邪�A������͖�������B
		�\�t�g�E�F�A���荞�ݏ���
			���y�Đ��Ȃ�
		


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

volatile unsigned char ps2keystatus[256]; // ���z�R�[�h�ɑ�������L�[�̏�ԁiOn�̎�1�j
volatile unsigned short vkey; //���z�L�[�R�[�h
unsigned char lockkey; // ����������Lock�L�[�̏�Ԏw��B����3�r�b�g��<CAPSLK><NUMLK><SCRLK>
unsigned char keytype; // �L�[�{�[�h�̎�ށB0�F���{��109�L�[�A1�F�p��104�L�[

int ps2init(){ return 0; } // PS/2���C�u�����֘A�������B����I��0�A�G���[��-1��Ԃ�
unsigned char shiftkeys(){ return 0; } // SHIFT�֘A�L�[�̉�����Ԃ�Ԃ�
unsigned char ps2readkey(){ return 0; }
// ���͂��ꂽ1�̃L�[�̃L�[�R�[�h���O���[�o���ϐ�vkey�Ɋi�[�i������Ă��Ȃ����0��Ԃ��j
// ����8�r�b�g�F�L�[�R�[�h
// ���8�r�b�g�F�V�t�g��ԁi�����F1�j�A��ʂ���<0><CAPSLK><NUMLK><SCRLK><Win><ALT><CTRL><SHIFT>
// �p���E�L�������̏ꍇ�A�߂�l�Ƃ���ASCII�R�[�h�i����ȊO��0��Ԃ��j
