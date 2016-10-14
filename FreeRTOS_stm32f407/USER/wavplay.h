#ifndef __WAVPLAY_H
#define __WAVPLAY_H
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//WAV 解码代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/6/29
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved				
//********************************************************************************
//V1.0 说明
//1,支持16位/24位WAV文件播放
//2,最高可以支持到192K/24bit的WAV格式. 
////////////////////////////////////////////////////////////////////////////////// 	


#define WAV_I2S_TX_DMA_BUFSIZE    8192		//定义WAV TX DMA 数组大小(播放192Kbps@24bit的时候,需要设置8192大才不会卡)
 
//RIFF块
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;这里固定为"RIFF",即0X46464952
    u32 ChunkSize ;		   	//集合大小;文件总大小-8
    u32 Format;	   			//格式;WAVE,即0X45564157
}ChunkRIFF ;
//fmt块
typedef __packed struct
{
    u32 ChunkID;		   	//chunk id;这里固定为"fmt ",即0X20746D66
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size);这里为:20.
    u16 AudioFormat;	  	//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	u16 NumOfChannels;		//通道数量;1,表示单声道;2,表示双声道;
	u32 SampleRate;			//采样率;0X1F40,表示8Khz
	u32 ByteRate;			//字节速率; 
	u16 BlockAlign;			//块对齐(字节); 
	u16 BitsPerSample;		//单个采样数据大小;4位ADPCM,设置为4
//	u16 ByteExtraData;		//附加的数据字节;2个; 线性PCM,没有这个参数
}ChunkFMT;	   
//fact块 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;这里固定为"fact",即0X74636166;
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size);这里为:4.
    u32 NumOfSamples;	  	//采样的数量; 
}ChunkFACT;
//LIST块 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;这里固定为"LIST",即0X74636166;
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size);这里为:4. 
}ChunkLIST;

//data块 
typedef __packed struct 
{
    u32 ChunkID;		   	//chunk id;这里固定为"data",即0X5453494C
    u32 ChunkSize ;		   	//子集合大小(不包括ID和Size) 
}ChunkDATA;

//wav头
typedef __packed struct
{ 
	ChunkRIFF riff;	//riff块
	ChunkFMT fmt;  	//fmt块
//	ChunkFACT fact;	//fact块 线性PCM,没有这个结构体	 
	ChunkDATA data;	//data块		 
}__WaveHeader; 

//wav 播放控制结构体
typedef __packed struct
{ 
    u16 audioformat;			//音频格式;0X01,表示线性PCM;0X11表示IMA ADPCM
	u16 nchannels;				//通道数量;1,表示单声道;2,表示双声道; 
	u16 blockalign;				//块对齐(字节);  
	u32 datasize;				//WAV数据大小 

    u32 totsec ;				//整首歌时长,单位:秒
    u32 cursec ;				//当前播放时长
	
    u32 bitrate;	   			//比特率(位速)
	u32 samplerate;				//采样率 
	u16 bps;					//位数,比如16bit,24bit,32bit
	
	u32 datastart;				//数据帧开始的位置(在文件里面的偏移)
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

#define MP3_TITSIZE_MAX		40		//歌曲名字最大长度
#define MP3_ARTSIZE_MAX		40		//歌曲名字最大长度
#define MP3_FILE_BUF_SZ   4000// 2*1024	//MP3解码时,文件buf大小

//ID3V1 标签 
typedef __packed struct 
{
    uint8_t id[3];		   	//ID,TAG三个字母
    uint8_t title[30];		//歌曲名字
    uint8_t artist[30];		//艺术家名字
	uint8_t year[4];			//年代
	uint8_t comment[30];		//备注
	uint8_t genre;			//流派 
}ID3V1_Tag;




//ID3V2 标签头 
typedef __packed struct 
{
    uint8_t id[3];		   	//ID
    uint8_t mversion;		//主版本号
    uint8_t sversion;		//子版本号
    uint8_t flags;			//标签头标志
    uint8_t size[4];			//标签信息大小(不包含标签头10字节).所以,标签大小=size+10.
}ID3V2_TagHead;



//ID3V2.3 版本帧头
typedef __packed struct 
{
    uint8_t id[4];		   	//帧ID
    uint8_t size[4];			//帧大小
    uint16_t flags;			//帧标志
}ID3V23_FrameHead;




//MP3 Xing帧信息(没有全部列出来,仅列出有用的部分)
typedef __packed struct 
{
    uint8_t id[4];		   	//帧ID,为Xing/Info
    uint8_t flags[4];		//存放标志
    uint8_t frames[4];		//总帧数
	uint8_t fsize[4];		//文件总大小(不包含ID3)
}MP3_FrameXing;
 

//MP3 VBRI帧信息(没有全部列出来,仅列出有用的部分)
typedef __packed struct 
{
    uint8_t id[4];		   	//帧ID,为Xing/Info
	uint8_t version[2];		//版本号
	uint8_t delay[2];		//延迟
	uint8_t quality[2];		//音频质量,0~100,越大质量越好
	uint8_t fsize[4];		//文件总大小
	uint8_t frames[4];		//文件总帧数 
}MP3_FrameVBRI;




//MP3控制结构体
typedef __packed struct 
{
    uint8_t title[MP3_TITSIZE_MAX];	//歌曲名字
    uint8_t artist[MP3_ARTSIZE_MAX];	//艺术家名字
    uint32_t totsec ;				//整首歌时长,单位:秒
    uint32_t cursec ;				//当前播放时长
	
    uint32_t bitrate;	   			//比特率
	uint32_t samplerate;				//采样率
	uint16_t outsamples;				//PCM输出数据量大小(以16位为单位),单声道MP3,则等于实际输出*2(方便DAC输出)
	
	uint32_t datastart;				//数据帧开始的位置(在文件里面的偏移)
}__mp3ctrl;




extern __mp3ctrl * mp3ctrl;



void mp3_i2s_dma_tx_callback(void) ;
void mp3_fill_buffer( int step, uint16_t* buf, uint16_t size, uint8_t nch );
uint8_t mp3_id3v1_decode(uint8_t* buf,__mp3ctrl *pctrl);
uint8_t mp3_id3v2_decode(uint8_t* buf,uint32_t size,__mp3ctrl *pctrl);
uint8_t mp3_get_info(uint8_t *pname,__mp3ctrl* pctrl);
uint8_t mp3_play_song(uint8_t* fname);

#endif
















