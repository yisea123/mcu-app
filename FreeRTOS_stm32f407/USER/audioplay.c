#include "audioplay.h"
#include "ff.h"
#include "FreeRTOS.h"
#include "task.h"
#include "usart.h"
#include "wm8978.h"
#include "i2s.h"
#include "led.h"
#include "delay.h"
#include "key.h"
#include "exfuns.h"  
#include "string.h"  

//音乐播放控制器
__audiodev audiodev;	  
 

//开始音频播放
void audio_start(void)
{
	audiodev.status = 3 << 0;//开始播放+非暂停
	I2S_Play_Start();
} 
//关闭音频播放
void audio_stop(void)
{
	audiodev.status = 0;
	I2S_Play_Stop();
}  
//得到path路径下,目标文件的总个数
//path:路径		    
//返回值:总有效文件数
u16 audio_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//临时目录
	FILINFO tfileinfo;	//临时文件信息		
	u8 *fn; 			 			   			     
    res=f_opendir(&tdir,(const TCHAR*)path); //打开目录
  	tfileinfo.lfsize=_MAX_LFN*2+1;						//长文件名最大长度
	tfileinfo.lfname=pvPortMalloc(  tfileinfo.lfsize);	//为长文件缓存区分配内存
	if(res==FR_OK)
	{
		if(tfileinfo.lfname!=NULL)
		{
			while(1)//查询总的有效文件数
			{
		        res=f_readdir(&tdir,&tfileinfo);       		//读取目录下的一个文件
		        if(res!=FR_OK||tfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出		  
	     		fn=(u8*)(*tfileinfo.lfname?tfileinfo.lfname:tfileinfo.fname);			 
				res=f_typetell(fn);	
				if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
				{
					rval++;//有效文件数增加1
				}	    
			} 
		}
		f_closedir( &tdir );
	} 
	vPortFree(  tfileinfo.lfname);
	return rval;
}
//显示曲目索引
//index:当前索引
//total:总文件数
void audio_index_show(u16 index,u16 total)
{
	//显示当前曲目的索引,及总曲目数
		  	  
}
 
//显示播放时间,比特率 信息  
//totsec;音频文件总时间长度
//cursec:当前播放时间
//bitrate:比特率(位速)
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate)
{	
	static u16 playtime=0XFFFF;//播放时间标记	      
	if(playtime!=cursec)					//需要更新显示时间
	{
		playtime=cursec;

	} 		 
}
//播放音乐
void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 		//目录
	FILINFO wavfileinfo;//文件信息
	u8 *fn;   			//长文件名
	u8 *pname;			//带路径的文件名
	u16 totwavnum; 		//音乐文件总数
	u16 curindex;		//图片当前索引
	u8 key;				//键值		  
 	u16 temp;
	u16 *wavindextbl;	//音乐索引表
	
	WM8978_ADDA_Cfg(1,0);	//开启DAC
	WM8978_Input_Cfg(0,0,0);//关闭输入通道
	WM8978_Output_Cfg(1,0);	//开启DAC输出   
	
 	while( f_opendir(&wavdir,"0:/MUSIC") )//打开音乐文件夹
 	{	    
		//vTaskDelay( 2000000 / portTICK_RATE_MS );
		printf("Have not 0:/MUSIC/\r\n");
		vTaskDelay( 2000000 / portTICK_RATE_MS );				  
	} 	
	f_closedir( &wavdir );
	
	totwavnum=audio_get_tnum("0:/MUSIC"); //得到总有效文件数
  	while(totwavnum==NULL)//音乐文件总数为0		
 	{	    
 		printf("no music!!! in 0:/MUSIC/\r\n");
		vTaskDelay( 20000 / portTICK_RATE_MS );				   				  
	}										   
  	wavfileinfo.lfsize=_MAX_LFN*2+1;						//长文件名最大长度
	wavfileinfo.lfname=pvPortMalloc(  wavfileinfo.lfsize );	//为长文件缓存区分配内存
 	pname=pvPortMalloc( wavfileinfo.lfsize );				//为带路径的文件名分配内存
 	wavindextbl=pvPortMalloc(  2*totwavnum );				//申请2*totwavnum个字节的内存,用于存放音乐文件索引
 	while(wavfileinfo.lfname==NULL||pname==NULL||wavindextbl==NULL)//内存分配出错
 	{	    
		vTaskDelay( 2000000 / portTICK_RATE_MS );			  
	}  	 
 	//记录索引
    res=f_opendir(&wavdir,"0:/MUSIC"); //打开目录
	if(res==FR_OK)
	{
		curindex=0;//当前索引为0
		while(1)//全部查询一遍
		{
			temp=wavdir.index;								//记录当前index
	        res=f_readdir(&wavdir,&wavfileinfo);       		//读取目录下的一个文件
	        if(res!=FR_OK||wavfileinfo.fname[0]==0)break;	//错误了/到末尾了,退出		  
     		fn=(u8*)(*wavfileinfo.lfname?wavfileinfo.lfname:wavfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X40)//取高四位,看看是不是音乐文件	
			{
				wavindextbl[curindex]=temp;//记录索引
				curindex++;
			}	    
		} 
		f_closedir( &wavdir );		
	}   
   	curindex = 0;											//从0开始显示
   	res = f_opendir( &wavdir, (const TCHAR*)"0:/MUSIC" ); 	//打开目录
	while(res==FR_OK)//打开成功
	{	
		dir_sdi( &wavdir, wavindextbl[ curindex ] );			//改变当前目录索引	   
        res = f_readdir( &wavdir, &wavfileinfo );       		//读取目录下的一个文件
        if( res != FR_OK || wavfileinfo.fname[0] == 0 )
		{
		
			f_closedir( &wavdir );		
			break;	//错误了/到末尾了,退出
     	}
		fn = (u8*)( *wavfileinfo.lfname?wavfileinfo.lfname:wavfileinfo.fname );			 
		strcpy(( char* ) pname, "0:/MUSIC/" );
		strcat(( char* ) pname, ( const char* ) fn ); 
		audio_index_show( curindex + 1, totwavnum );
		key = audio_play_song( pname ); 
		if( key == KEY2_PRES )
		{
			if(curindex)curindex--;
			else curindex=totwavnum-1;
 		}
		else if(key==KEY0_PRES)
		{
			curindex++;		   	
			if( curindex >= totwavnum )
				curindex=0; 
 		}
		else { 			
			
			printf("%s: error happend\r\n", __func__ );
			curindex++;		   	
			if( curindex >= totwavnum )
				curindex=0; 
			continue;
			//f_closedir( &wavdir );		
			//break;	// error happend 	 
		}
	} 	
	
	vPortFree(  wavfileinfo.lfname);			    
	vPortFree(  pname);			    
	vPortFree( wavindextbl);
} 

u8 audio_play_song(u8* fname)
{
	u8 res;  
	res = f_typetell( fname ); 
	
	switch( res )
	{
		case T_WAV:
			res = wav_play_song( fname );
			printf("%s: 1 res = %d\r\n", __func__, res );			
			break;
		case T_MP3:
			res = mp3_play_song( fname );
			printf("%s: 2 res = %d\r\n", __func__, res );		
			break;
		default://其他文件,自动跳转到下一曲
			printf("can't play:%s\r\n",fname);
			res=KEY0_PRES;
			break;
	}
	return res;
}



























