#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H

#include "sys.h"
#include "ff.h"
#include "wavplay.h"

						  
#define I2S_TX_DMA_BUF_SIZE    8192		
//����TX DMA �����С(����192Kbps@24bit��ʱ��,��Ҫ����
//8192��Ų��Ῠ)


//���ֲ��ſ�����
/*
typedef __packed struct
{  
	//2��I2S�����BUF
	u8 *i2sbuf1;
	u8 *i2sbuf2; 
	u8 *tbuf;				
	//��ʱ����,����24bit�����ʱ����Ҫ�õ�
	FIL *file;				
	//��Ƶ�ļ�ָ��
	
	u8 status;				
	//bit0:0,��ͣ����;1,��������
	//bit1:0,��������;1,�������� 
}__audiodev; 

extern __audiodev audiodev;	
//���ֲ��ſ�����
*/

typedef struct
{  
	//2��I2S�����BUF
	//���ﻨ��3*8 = 24KB!
	unsigned char i2sbuf1[ WAV_I2S_TX_DMA_BUFSIZE ];
	unsigned char i2sbuf2[ WAV_I2S_TX_DMA_BUFSIZE ]; 
	unsigned char tbuf[ WAV_I2S_TX_DMA_BUFSIZE ];			//��ʱ����
	FIL *file;			//��Ƶ�ļ�ָ�� 	
	vu8 status;			//bit0:0,��ͣ����;1,��������
						//bit1:0,��������;1,��������  
						//bit2~3:����
						//bit4:0,�����ֲ���;1,���ֲ����� (������)		
						//bit5:0,�޶���;1,ִ����һ���и����(������)
						//bit6:0,�޶���;1,������ֹ����(���ǲ�ɾ����Ƶ��������),������ɺ�,���������Զ������λ
 						//bit7:0,��Ƶ����������ɾ��/����ɾ��;1,��Ƶ����������������(�������ִ��)
	unsigned char mode;			//����ģʽ
						//0,ȫ��ѭ��;1,����ѭ��;2,�������;
	unsigned char *path;			//��ǰ�ļ���·��
	unsigned char *name;			//��ǰ���ŵ�MP3��������
	unsigned short namelen;		//name��ռ�ĵ���.
	unsigned short curnamepos;		//��ǰ��ƫ��
    unsigned int totsec ;		//���׸�ʱ��,��λ:��
    unsigned int cursec ;		//��ǰ����ʱ�� 
    unsigned int bitrate;	   	//������(λ��)
	unsigned int samplerate;		//������ 
	unsigned short bps;			//λ��,����16bit,24bit,32bit	
	unsigned short curindex;		//��ǰ���ŵ���Ƶ�ļ�����
	unsigned short mfilenum;		//�����ļ���Ŀ	    
	unsigned short *mfindextbl;	//��Ƶ�ļ�������
	unsigned int( *file_seek )( unsigned int );//�ļ�������˺��� 
	
}__audiodev; 

extern __audiodev audiodev;	//���ֲ��ſ�����

//ȡ2��ֵ����Ľ�Сֵ.
#ifndef AUDIO_MIN			
#define AUDIO_MIN(x,y)	((x)<(y)? (x):(y))
#endif


extern void wav_i2s_dma_callback( void );
extern void audio_start( void );
extern void audio_stop( void );
extern unsigned short audio_get_tnum(unsigned char *path);
extern void audio_index_show( unsigned short index, unsigned short total);
extern void audio_msg_show(unsigned int totsec,unsigned int cursec,unsigned int bitrate);
extern void audio_play(void);
extern unsigned char audio_play_song( unsigned char* fname );
 
#endif












