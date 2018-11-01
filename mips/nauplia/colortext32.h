#define WIDTH_X	30 // ������������
#define WIDTH_Y	27 // �c����������
#define ATTROFFSET	(WIDTH_X*WIDTH_Y) // VRAM��̃J���[�p���b�g�i�[�ʒu

// ���̓{�^���̃|�[�g�A�r�b�g��`
#define KEYPORT PORTB
#define KEYUP 0x0400
#define KEYDOWN 0x0080
#define KEYLEFT 0x0100
#define KEYRIGHT 0x0200
#define KEYSTART 0x0800
#define KEYFIRE 0x4000

extern volatile char drawing;		//�@�\�����Ԓ���-1
extern volatile unsigned short drawcount;		//�@1��ʕ\���I�����Ƃ�1�����B�A�v������0�ɂ���B
							// �Œ�1��͉�ʕ\���������Ƃ̃`�F�b�N�ƁA�A�v���̏���������ʊ��ԕK�v���̊m�F�ɗ��p�B
extern unsigned char TVRAM[]; //�e�L�X�g�r�f�I������

extern const unsigned char FontData[]; //�t�H���g�p�^�[����`
extern unsigned char *cursor;
extern unsigned char cursorcolor;
extern unsigned char *fontp;

void start_composite(void); //�J���[�R���|�W�b�g�o�͊J�n
void stop_composite(void); //�J���[�R���|�W�b�g�o�͒�~
void init_composite(void); //�J���[�R���|�W�b�g�o�͏�����
void clearscreen(void); //��ʃN���A
void set_palette(unsigned char n,unsigned char b,unsigned char r,unsigned char g); //�p���b�g�ݒ�
void set_bgcolor(unsigned char b,unsigned char r,unsigned char g); //�o�b�N�O�����h�J���[�ݒ�

void vramscroll(void);
	//1�s�X�N���[��
void setcursor(unsigned char x,unsigned char y,unsigned char c);
	//�J�[�\���ʒu�ƃJ���[��ݒ�
void setcursorcolor(unsigned char c);
	//�J�[�\���ʒu���̂܂܂ŃJ���[�ԍ���c�ɐݒ�
void printchar(unsigned char n);
	//�J�[�\���ʒu�Ƀe�L�X�g�R�[�hn��1�����\�����A�J�[�\����1�����i�߂�
void printstr(unsigned char *s);
	//�J�[�\���ʒu�ɕ�����s��\��
void printnum(unsigned int n);
	//�J�[�\���ʒu�ɕ����Ȃ�����n��10�i���\��
void printnum2(unsigned int n,unsigned char e);
	//�J�[�\���ʒu�ɕ����Ȃ�����n��e����10�i���\���i�O�̋󂫌������̓X�y�[�X�Ŗ��߂�j
void cls(void);
	//��ʏ������A�J�[�\����擪�Ɉړ�
void startPCG(unsigned char *p,int a);
	// RAM�t�H���g�iPCG�j�̗��p�J�n�Ap���t�H���g�i�[�ꏊ�Aa��0�ȊO�ŃV�X�e���t�H���g���R�s�[
void stopPCG(void);
	// RAM�t�H���g�iPCG�j�̗��p��~
