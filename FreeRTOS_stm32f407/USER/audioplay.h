#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H
#include "sys.h"
#include "ff.h"
#include "wavplay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//���ֲ����� Ӧ�ô���	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/5/24
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
     

						  
#define I2S_TX_DMA_BUF_SIZE    8192		//����TX DMA �����С(����192Kbps@24bit��ʱ��,��Ҫ����8192��Ų��Ῠ)


//���ֲ��ſ�����
/*
typedef __packed struct
{  
	//2��I2S�����BUF
	u8 *i2sbuf1;
	u8 *i2sbuf2; 
	u8 *tbuf;				//��ʱ����,����24bit�����ʱ����Ҫ�õ�
	FIL *file;				//��Ƶ�ļ�ָ��
	
	u8 status;				//bit0:0,��ͣ����;1,��������
							//bit1:0,��������;1,�������� 
}__audiodev; 
extern __audiodev audiodev;	//���ֲ��ſ�����
*/
//���ֲ��ſ�����
typedef __packed struct
{  
	//2��I2S�����BUF
	uint8_t *i2sbuf1;
	uint8_t *i2sbuf2; 
	uint8_t *tbuf;			//��ʱ����
	FIL *file;			//��Ƶ�ļ�ָ�� 	
	vu8 status;			//bit0:0,��ͣ����;1,��������
						//bit1:0,��������;1,��������  
						//bit2~3:����
						//bit4:0,�����ֲ���;1,���ֲ����� (������)		
						//bit5:0,�޶���;1,ִ����һ���и����(������)
						//bit6:0,�޶���;1,������ֹ����(���ǲ�ɾ����Ƶ��������),������ɺ�,���������Զ������λ
 						//bit7:0,��Ƶ����������ɾ��/����ɾ��;1,��Ƶ����������������(�������ִ��)
	uint8_t mode;			//����ģʽ
						//0,ȫ��ѭ��;1,����ѭ��;2,�������;
	uint8_t *path;			//��ǰ�ļ���·��
	uint8_t *name;			//��ǰ���ŵ�MP3��������
	uint16_t namelen;		//name��ռ�ĵ���.
	uint16_t curnamepos;		//��ǰ��ƫ��
    uint32_t totsec ;		//���׸�ʱ��,��λ:��
    uint32_t cursec ;		//��ǰ����ʱ�� 
    uint32_t bitrate;	   	//������(λ��)
	uint32_t samplerate;		//������ 
	uint16_t bps;			//λ��,����16bit,24bit,32bit
	
	uint16_t curindex;		//��ǰ���ŵ���Ƶ�ļ�����
	uint16_t mfilenum;		//�����ļ���Ŀ	    
	uint16_t *mfindextbl;	//��Ƶ�ļ�������
	uint32_t(*file_seek)(uint32_t);//�ļ�������˺��� 
	
}__audiodev; 
extern __audiodev audiodev;	//���ֲ��ſ�����

//ȡ2��ֵ����Ľ�Сֵ.
#ifndef AUDIO_MIN			
#define AUDIO_MIN(x,y)	((x)<(y)? (x):(y))
#endif


void wav_i2s_dma_callback(void);

void audio_start(void);
void audio_stop(void);
u16 audio_get_tnum(u8 *path);
void audio_index_show(u16 index,u16 total);
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate);
void audio_play(void);
u8 audio_play_song(u8* fname);

 
#endif












