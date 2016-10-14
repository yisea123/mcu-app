#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H
#include "sys.h"
#include "ff.h"
#include "wavplay.h"
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32F407开发板
//音乐播放器 应用代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//创建日期:2014/5/24
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2014-2024
//All rights reserved									  
////////////////////////////////////////////////////////////////////////////////// 	
     

						  
#define I2S_TX_DMA_BUF_SIZE    8192		//定义TX DMA 数组大小(播放192Kbps@24bit的时候,需要设置8192大才不会卡)


//音乐播放控制器
/*
typedef __packed struct
{  
	//2个I2S解码的BUF
	u8 *i2sbuf1;
	u8 *i2sbuf2; 
	u8 *tbuf;				//零时数组,仅在24bit解码的时候需要用到
	FIL *file;				//音频文件指针
	
	u8 status;				//bit0:0,暂停播放;1,继续播放
							//bit1:0,结束播放;1,开启播放 
}__audiodev; 
extern __audiodev audiodev;	//音乐播放控制器
*/
//音乐播放控制器
typedef __packed struct
{  
	//2个I2S解码的BUF
	uint8_t *i2sbuf1;
	uint8_t *i2sbuf2; 
	uint8_t *tbuf;			//零时数组
	FIL *file;			//音频文件指针 	
	vu8 status;			//bit0:0,暂停播放;1,继续播放
						//bit1:0,结束播放;1,开启播放  
						//bit2~3:保留
						//bit4:0,无音乐播放;1,音乐播放中 (对外标记)		
						//bit5:0,无动作;1,执行了一次切歌操作(对外标记)
						//bit6:0,无动作;1,请求终止播放(但是不删除音频播放任务),处理完成后,播放任务自动清零该位
 						//bit7:0,音频播放任务已删除/请求删除;1,音频播放任务正在运行(允许继续执行)
	uint8_t mode;			//播放模式
						//0,全部循环;1,单曲循环;2,随机播放;
	uint8_t *path;			//当前文件夹路径
	uint8_t *name;			//当前播放的MP3歌曲名字
	uint16_t namelen;		//name所占的点数.
	uint16_t curnamepos;		//当前的偏移
    uint32_t totsec ;		//整首歌时长,单位:秒
    uint32_t cursec ;		//当前播放时长 
    uint32_t bitrate;	   	//比特率(位速)
	uint32_t samplerate;		//采样率 
	uint16_t bps;			//位数,比如16bit,24bit,32bit
	
	uint16_t curindex;		//当前播放的音频文件索引
	uint16_t mfilenum;		//音乐文件数目	    
	uint16_t *mfindextbl;	//音频文件索引表
	uint32_t(*file_seek)(uint32_t);//文件快进快退函数 
	
}__audiodev; 
extern __audiodev audiodev;	//音乐播放控制器

//取2个值里面的较小值.
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












