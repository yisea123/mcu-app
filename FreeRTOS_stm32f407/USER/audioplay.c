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

extern unsigned char amr_play_song ( unsigned char *serialFileName );

__audiodev audiodev;	  
 

void audio_start(void)
{
	audiodev.status = 3 << 0;
	I2S_Play_Start();
} 

void audio_stop(void)
{
	audiodev.status = 0;
	I2S_Play_Stop();
}  

u16 audio_get_tnum(u8 *path)
{	  
	u8 res;
	u16 rval=0;
 	DIR tdir;	 		//��ʱĿ¼
	FILINFO tfileinfo;	//��ʱ�ļ���Ϣ		
	u8 *fn;
	char namebuf[_MAX_LFN*2+1];
    res=f_opendir(&tdir,(const TCHAR*)path); //��Ŀ¼
  	tfileinfo.lfsize=_MAX_LFN*2+1;						//���ļ�����󳤶�
	//tfileinfo.lfname = pvPortMalloc( tfileinfo.lfsize );	//Ϊ���ļ������������ڴ�
	tfileinfo.lfname = ( TCHAR* ) namebuf;
	if(res==FR_OK)
	{
		if(tfileinfo.lfname!=NULL)
		{
			while(1)//��ѯ�ܵ���Ч�ļ���
			{
		        res=f_readdir(&tdir,&tfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
		        if(res!=FR_OK||tfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�		  
	     		fn=(u8*)(*tfileinfo.lfname?tfileinfo.lfname:tfileinfo.fname);			 
				res=f_typetell(fn);	
				if((res&0XF0)==0X40)//ȡ����λ,�����ǲ��������ļ�	
				{
					rval++;//��Ч�ļ�������1
				}	    
			} 
		}
		f_closedir( &tdir );
	} 
	//vPortFree(  tfileinfo.lfname);
	return rval;
}

void audio_index_show(u16 index,u16 total)
{
	//��ʾ��ǰ��Ŀ������,������Ŀ��
		  	  
}
 
void audio_msg_show(u32 totsec,u32 cursec,u32 bitrate)
{	
	static u16 playtime=0XFFFF;//����ʱ����	      
	if(playtime!=cursec)					//��Ҫ������ʾʱ��
	{
		playtime=cursec;

	} 		 
}

char amrPlayRequest = 0;

void audio_play(void)
{
	u8 res;
 	DIR wavdir;	 		//Ŀ¼
	FILINFO wavfileinfo;//�ļ���Ϣ
	u8 *fn;   			//���ļ���
	u8 *pname;			//��·�����ļ���
	u16 totwavnum; 		//�����ļ�����
	u16 curindex;		//ͼƬ��ǰ����
	u8 key;				//��ֵ		  
 	u16 temp;
	u16 *wavindextbl;	//����������
	
	WM8978_ADDA_Cfg(1,0);	//����DAC
	WM8978_Input_Cfg(0,0,0);//�ر�����ͨ��
	WM8978_Output_Cfg(1,0);	//����DAC���   
	
 	while( f_opendir(&wavdir,"0:/MUSIC") )//�������ļ���
 	{	    
		vTaskDelay( 2000000 / portTICK_RATE_MS );
		//printf("Have not 0:/MUSIC/\r\n");
		//vTaskDelay( 300 / portTICK_RATE_MS );
		if( amrPlayRequest )
		{
			amrPlayRequest = 0;
			amr_play_song( "0:/voice/1.amr" );
		}
	} 	
	f_closedir( &wavdir );
	
	totwavnum=audio_get_tnum("0:/MUSIC"); //�õ�����Ч�ļ���
  	while(totwavnum==NULL)//�����ļ�����Ϊ0		
 	{	    
 		printf("no music!!! in 0:/MUSIC/\r\n");
		vTaskDelay( 20000 / portTICK_RATE_MS );				   				  
	}										   
  	wavfileinfo.lfsize=_MAX_LFN*2 + 1;						//���ļ�����󳤶�
	wavfileinfo.lfname = pvPortMalloc( wavfileinfo.lfsize );	//Ϊ���ļ������������ڴ�
 	pname = pvPortMalloc( wavfileinfo.lfsize );				//Ϊ��·�����ļ��������ڴ�
 	wavindextbl = pvPortMalloc( 2 * totwavnum );				//����2*totwavnum���ֽڵ��ڴ�,���ڴ�������ļ�����
 	while(wavfileinfo.lfname==NULL||pname==NULL||wavindextbl==NULL)//�ڴ�������
 	{	    
		vTaskDelay( 2000000 / portTICK_RATE_MS );			  
	}  	 
 	//��¼����
    res=f_opendir(&wavdir,"0:/MUSIC"); //��Ŀ¼
	if(res==FR_OK)
	{
		curindex=0;//��ǰ����Ϊ0
		while(1)//ȫ����ѯһ��
		{
			temp=wavdir.index;								//��¼��ǰindex
	        res=f_readdir(&wavdir,&wavfileinfo);       		//��ȡĿ¼�µ�һ���ļ�
	        if(res!=FR_OK||wavfileinfo.fname[0]==0)break;	//������/��ĩβ��,�˳�		  
     		fn=(u8*)(*wavfileinfo.lfname?wavfileinfo.lfname:wavfileinfo.fname);			 
			res=f_typetell(fn);	
			if((res&0XF0)==0X40)//ȡ����λ,�����ǲ��������ļ�	
			{
				wavindextbl[curindex]=temp;//��¼����
				curindex++;
			}	    
		} 
		f_closedir( &wavdir );		
	}   
   	curindex = 0;											//��0��ʼ��ʾ
   	res = f_opendir( &wavdir, (const TCHAR*)"0:/MUSIC" ); 	//��Ŀ¼
	while(res==FR_OK)//�򿪳ɹ�
	{	
		dir_sdi( &wavdir, wavindextbl[ curindex ] );			//�ı䵱ǰĿ¼����	   
        res = f_readdir( &wavdir, &wavfileinfo );       		//��ȡĿ¼�µ�һ���ļ�
        if( res != FR_OK || wavfileinfo.fname[0] == 0 )
		{
		
			f_closedir( &wavdir );		
			break;	//������/��ĩβ��,�˳�
     	}
		fn = (u8*)( *wavfileinfo.lfname?wavfileinfo.lfname:wavfileinfo.fname );			 
		strcpy(( char* ) pname, "0:/MUSIC/" );
		strcat(( char* ) pname, ( const char* ) fn ); 
		audio_index_show( curindex + 1, totwavnum );
		key = audio_play_song( pname ); 
		if( key == PREVIOUS )
		{
			if( curindex ) curindex--;
			else curindex = totwavnum - 1;
 		}
		else if( key == NEXT )
		{
			curindex++;		   	
			if( curindex >= totwavnum )
				curindex = 0; 
 		}
		else 
		{ 				
			printf("%s: error happend\r\n", __func__ );
			curindex++;		   	
			if( curindex >= totwavnum )
				curindex=0; 
			continue;
			//f_closedir( &wavdir );		
			//break;	
			// error happend 	 
		}
	} 	
	
	vPortFree( wavfileinfo.lfname );			    
	vPortFree( pname );			    
	vPortFree( wavindextbl );
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
		case T_APE:
			res = amr_play_song( fname );
			printf("%s: 3 res = %d\r\n", __func__, res );		
			break;
		default:  
			printf("can't play:%s\r\n",fname);
			res=KEY0_PRES;
			break;
	}
	return res;
}


























