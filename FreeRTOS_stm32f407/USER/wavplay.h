#ifndef __WAVPLAY_H
#define __WAVPLAY_H
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK STM32F407������
//WAV �������	   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//��������:2014/6/29
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ������������ӿƼ����޹�˾ 2014-2024
//All rights reserved				
//********************************************************************************
//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ. 
////////////////////////////////////////////////////////////////////////////////// 	


#define WAV_I2S_TX_DMA_BUFSIZE    8192		//����WAV TX DMA �����С(����192Kbps@24bit��ʱ��,��Ҫ����8192��Ų��Ῠ)
 
//RIFF��
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"RIFF",��0X46464952
    u32 ChunkSize ;		   	//���ϴ�С;�ļ��ܴ�С-8
    u32 Format;	   			//��ʽ;WAVE,��0X45564157
}ChunkRIFF ;
//fmt��
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fmt ",��0X20746D66
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:20.
    u16 AudioFormat;	  	//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 NumOfChannels;		//ͨ������;1,��ʾ������;2,��ʾ˫����;
	u32 SampleRate;			//������;0X1F40,��ʾ8Khz
	u32 ByteRate;			//�ֽ�����; 
	u16 BlockAlign;			//�����(�ֽ�); 
	u16 BitsPerSample;		//�����������ݴ�С;4λADPCM,����Ϊ4
//	u16 ByteExtraData;		//���ӵ������ֽ�;2��; ����PCM,û���������
}ChunkFMT;	   
//fact�� 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"fact",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4.
    u32 NumOfSamples;	  	//����������; 
}ChunkFACT;
//LIST�� 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"LIST",��0X74636166;
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size);����Ϊ:4. 
}ChunkLIST;

//data�� 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;����̶�Ϊ"data",��0X5453494C
    u32 ChunkSize ;		   	//�Ӽ��ϴ�С(������ID��Size) 
}ChunkDATA;

//wavͷ
typedef __packed struct
{ 
	ChunkRIFF riff;	//riff��
	ChunkFMT fmt;  	//fmt��
//	ChunkFACT fact;	//fact�� ����PCM,û������ṹ��	 
	ChunkDATA data;	//data��		 
}__WaveHeader; 

//wav ���ſ��ƽṹ��
typedef __packed struct
{ 
    u16 audioformat;			//��Ƶ��ʽ;0X01,��ʾ����PCM;0X11��ʾIMA ADPCM
	u16 nchannels;				//ͨ������;1,��ʾ������;2,��ʾ˫����; 
	u16 blockalign;				//�����(�ֽ�);  
	u32 datasize;				//WAV���ݴ�С 

    u32 totsec ;				//���׸�ʱ��,��λ:��
    u32 cursec ;				//��ǰ����ʱ��
	
    u32 bitrate;	   			//������(λ��)
	u32 samplerate;				//������ 
	u16 bps;					//λ��,����16bit,24bit,32bit
	
	u32 datastart;				//����֡��ʼ��λ��(���ļ������ƫ��)
}__wavctrl; 


u8 wav_decode_init(u8* fname,__wavctrl* wavx);
u32 wav_buffill(u8 *buf,u16 size,u8 bits);
void wav_i2s_dma_tx_callback(void); 
u8 wav_play_song(u8* fname);
u8 mp3_play_song(u8* fname);

void MusicAddVolume( void );

void MusicDelVolume( void );

void MusicNext( void );

void MusicPrevious( void );

void MusicStop( void );

void MusicContinue( void );



#include "mp3dec.h"
//#include "stm32f10x_conf.h"

#define MP3_TITSIZE_MAX		40		//����������󳤶�
#define MP3_ARTSIZE_MAX		40		//����������󳤶�
#define MP3_FILE_BUF_SZ   4000// 2*1024	//MP3����ʱ,�ļ�buf��С

//ID3V1 ��ǩ 
typedef __packed struct 
{
    uint8_t id[3];		   	//ID,TAG������ĸ
    uint8_t title[30];		//��������
    uint8_t artist[30];		//����������
	uint8_t year[4];			//���
	uint8_t comment[30];		//��ע
	uint8_t genre;			//���� 
}ID3V1_Tag;




//ID3V2 ��ǩͷ 
typedef __packed struct 
{
    uint8_t id[3];		   	//ID
    uint8_t mversion;		//���汾��
    uint8_t sversion;		//�Ӱ汾��
    uint8_t flags;			//��ǩͷ��־
    uint8_t size[4];			//��ǩ��Ϣ��С(��������ǩͷ10�ֽ�).����,��ǩ��С=size+10.
}ID3V2_TagHead;



//ID3V2.3 �汾֡ͷ
typedef __packed struct 
{
    uint8_t id[4];		   	//֡ID
    uint8_t size[4];			//֡��С
    uint16_t flags;			//֡��־
}ID3V23_FrameHead;




//MP3 Xing֡��Ϣ(û��ȫ���г���,���г����õĲ���)
typedef __packed struct 
{
    uint8_t id[4];		   	//֡ID,ΪXing/Info
    uint8_t flags[4];		//��ű�־
    uint8_t frames[4];		//��֡��
	uint8_t fsize[4];		//�ļ��ܴ�С(������ID3)
}MP3_FrameXing;
 

//MP3 VBRI֡��Ϣ(û��ȫ���г���,���г����õĲ���)
typedef __packed struct 
{
    uint8_t id[4];		   	//֡ID,ΪXing/Info
	uint8_t version[2];		//�汾��
	uint8_t delay[2];		//�ӳ�
	uint8_t quality[2];		//��Ƶ����,0~100,Խ������Խ��
	uint8_t fsize[4];		//�ļ��ܴ�С
	uint8_t frames[4];		//�ļ���֡�� 
}MP3_FrameVBRI;




//MP3���ƽṹ��
typedef __packed struct 
{
    uint8_t title[MP3_TITSIZE_MAX];	//��������
    uint8_t artist[MP3_ARTSIZE_MAX];	//����������
    uint32_t totsec ;				//���׸�ʱ��,��λ:��
    uint32_t cursec ;				//��ǰ����ʱ��
	
    uint32_t bitrate;	   			//������
	uint32_t samplerate;				//������
	uint16_t outsamples;				//PCM�����������С(��16λΪ��λ),������MP3,�����ʵ�����*2(����DAC���)
	
	uint32_t datastart;				//����֡��ʼ��λ��(���ļ������ƫ��)
}__mp3ctrl;




extern __mp3ctrl * mp3ctrl;



void mp3_i2s_dma_tx_callback(void) ;
void mp3_fill_buffer( int step, uint16_t* buf, uint16_t size, uint8_t nch );
uint8_t mp3_id3v1_decode(uint8_t* buf,__mp3ctrl *pctrl);
uint8_t mp3_id3v2_decode(uint8_t* buf,uint32_t size,__mp3ctrl *pctrl);
uint8_t mp3_get_info(uint8_t *pname,__mp3ctrl* pctrl);
uint8_t mp3_play_song(uint8_t* fname);

#endif
















