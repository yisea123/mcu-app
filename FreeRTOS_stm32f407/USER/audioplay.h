#ifndef __AUDIOPLAY_H
#define __AUDIOPLAY_H

#include "sys.h"
#include "ff.h"
#include "wavplay.h"

						  
#define I2S_TX_DMA_BUF_SIZE    8192		
//定义TX DMA 数组大小(播放192Kbps@24bit的时候,需要设置
//8192大才不会卡)


//音乐播放控制器
/*
typedef __packed struct
{  
	//2个I2S解码的BUF
	u8 *i2sbuf1;
	u8 *i2sbuf2; 
	u8 *tbuf;				
	//零时数组,仅在24bit解码的时候需要用到
	FIL *file;				
	//音频文件指针
	
	u8 status;				
	//bit0:0,暂停播放;1,继续播放
	//bit1:0,结束播放;1,开启播放 
}__audiodev; 

extern __audiodev audiodev;	
//音乐播放控制器
*/

typedef struct
{  
	//2个I2S解码的BUF
	//这里花了3*8 = 24KB!
	unsigned char i2sbuf1[ WAV_I2S_TX_DMA_BUFSIZE ];
	unsigned char i2sbuf2[ WAV_I2S_TX_DMA_BUFSIZE ]; 
	unsigned char tbuf[ WAV_I2S_TX_DMA_BUFSIZE ];			//零时数组
	FIL *file;			//音频文件指针 	
	vu8 status;			//bit0:0,暂停播放;1,继续播放
						//bit1:0,结束播放;1,开启播放  
						//bit2~3:保留
						//bit4:0,无音乐播放;1,音乐播放中 (对外标记)		
						//bit5:0,无动作;1,执行了一次切歌操作(对外标记)
						//bit6:0,无动作;1,请求终止播放(但是不删除音频播放任务),处理完成后,播放任务自动清零该位
 						//bit7:0,音频播放任务已删除/请求删除;1,音频播放任务正在运行(允许继续执行)
	unsigned char mode;			//播放模式
						//0,全部循环;1,单曲循环;2,随机播放;
	unsigned char *path;			//当前文件夹路径
	unsigned char *name;			//当前播放的MP3歌曲名字
	unsigned short namelen;		//name所占的点数.
	unsigned short curnamepos;		//当前的偏移
    unsigned int totsec ;		//整首歌时长,单位:秒
    unsigned int cursec ;		//当前播放时长 
    unsigned int bitrate;	   	//比特率(位速)
	unsigned int samplerate;		//采样率 
	unsigned short bps;			//位数,比如16bit,24bit,32bit	
	unsigned short curindex;		//当前播放的音频文件索引
	unsigned short mfilenum;		//音乐文件数目	    
	unsigned short *mfindextbl;	//音频文件索引表
	unsigned int( *file_seek )( unsigned int );//文件快进快退函数 
	
}__audiodev; 

extern __audiodev audiodev;	//音乐播放控制器

//取2个值里面的较小值.
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












