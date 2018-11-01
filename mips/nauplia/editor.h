/*
   This file is provided under the LGPL license ver 2.1.
   Written by K.Tanaka
   http://www.ze.em-net.ne.jp/~kenken/index.html
*/

#define TBUFMAXLINE 101 //�e�L�X�g�o�b�t�@��
#define TBUFSIZE 200 //�e�L�X�g�o�b�t�@1�̃T�C�Y
#define TBUFMAXSIZE (TBUFSIZE*(TBUFMAXLINE-1)) //�ő�o�b�t�@�e�ʁi�o�b�t�@1�s���󂯂�j
#define EDITWIDTHX 30 //�G�f�B�^��ʉ���
#define EDITWIDTHY 26 //�G�f�B�^��ʏc��
#define COLOR_NORMALTEXT 7 //�ʏ�e�L�X�g�F
#define COLOR_ERRORTEXT 4 //�G���[���b�Z�[�W�e�L�X�g�F
#define COLOR_AREASELECTTEXT 4 //�͈͑I���e�L�X�g�F
#define COLOR_BOTTOMLINE 5 //��ʍŉ��s�̐F
#define FILEBUFSIZE 256 //�t�@�C���A�N�Z�X�p�o�b�t�@�T�C�Y
#define MAXFILENUM 200 //���p�\�t�@�C���ő吔

#define ERR_FILETOOBIG -1
#define ERR_CANTFILEOPEN -2
#define ERR_CANTWRITEFILE -3

#define TEMPFILENAME "~TEMP.BAS"
#define INIFILE "MACHIKAN.INI" // �����ݒ�t�@�C��
#define HEXFILE "MACHIKAN.HEX" // ���s��HEX�t�@�C����������ƈ�v�����ꍇ�̓G�f�B�^�N��

void texteditor(void); //�e�L�X�g�G�f�B�^�{��
int runbasic(char *s,int test); //�R���p�C�����Ď��s
extern unsigned char tempfile[13];
