#include "wavplay.h" 
#include "audioplay.h"
#include "usart.h" 
#include "delay.h" 
#include "FreeRTOS.h"
#include "task.h"
#include "ff.h"
#include "i2s.h"
#include "wm8978.h"
#include "key.h"
#include "led.h"
#include<string.h>
	
//********************************************************************************
//V1.0 ˵��
//1,֧��16λ/24λWAV�ļ�����
//2,��߿���֧�ֵ�192K/24bit��WAV��ʽ. 
////////////////////////////////////////////////////////////////////////////////// 	
 
__wavctrl wavctrl;		//WAV���ƽṹ��
vu8 wavtransferend=0;	//i2s������ɱ�־
vu8 wavwitchbuf=0;		//i2sbufxָʾ��־

extern TaskHandle_t pxMusicPlayer;
  
u8 wav_decode_init(u8* fname,__wavctrl* wavx, void *mFile)
{
	FIL*ftemp;
	u8 *buf; 
	u32 br=0;
	u8 res=0;
	ChunkRIFF *riff;
	ChunkFMT *fmt;
	ChunkFACT *fact;
	ChunkDATA *data;
	ftemp = (FIL*) mFile;
	buf = audiodev.i2sbuf1;
	if( ftemp && buf )
	{
		res=f_open(ftemp,(TCHAR*)fname,FA_READ);//���ļ�
		if(res==FR_OK)
		{
			f_read(ftemp,buf,512,&br);	//��ȡ512�ֽ�������
			riff=(ChunkRIFF *)buf;		//��ȡRIFF��
			if(riff->Format==0X45564157)//��WAV�ļ�
			{
				fmt=(ChunkFMT *)(buf+12);	//��ȡFMT�� 
				fact=(ChunkFACT *)(buf+12+8+fmt->ChunkSize);//��ȡFACT��
				if(fact->ChunkID==0X74636166||fact->ChunkID==0X5453494C)
					wavx->datastart=12+8+fmt->ChunkSize+8+fact->ChunkSize;//����fact/LIST���ʱ��(δ����)
				else 
					wavx->datastart=12+8+fmt->ChunkSize;  
				data=(ChunkDATA *)(buf+wavx->datastart);	//��ȡDATA��
				if(data->ChunkID==0X61746164)//�����ɹ�!
				{
					wavx->audioformat=fmt->AudioFormat;		//��Ƶ��ʽ
					wavx->nchannels=fmt->NumOfChannels;		//ͨ����
					wavx->samplerate=fmt->SampleRate;		//������
					wavx->bitrate=fmt->ByteRate*8;			//�õ�λ��
					wavx->blockalign=fmt->BlockAlign;		//�����
					wavx->bps=fmt->BitsPerSample;			//λ��,16/24/32λ
					
					wavx->datasize=data->ChunkSize;			//���ݿ��С
					wavx->datastart=wavx->datastart+8;		//��������ʼ�ĵط�. 
					 
					printf("wavx->audioformat:%d\r\n",wavx->audioformat);
					printf("wavx->nchannels:%d\r\n",wavx->nchannels);
					printf("wavx->samplerate:%d\r\n",wavx->samplerate);
					printf("wavx->bitrate:%d\r\n",wavx->bitrate);
					printf("wavx->blockalign:%d\r\n",wavx->blockalign);
					printf("wavx->bps:%d\r\n",wavx->bps);
					printf("wavx->datasize:%d\r\n",wavx->datasize);
					printf("wavx->datastart:%d\r\n",wavx->datastart);  
				}
				else
				{
					res=3;//no find data
				}
			}
			else
			{
				res=2;//no wave
			}

			f_close(ftemp);		
		}
		else 
		{
			res=1;
		}
	}

	return 0;
}

u32 wav_buffill_buffer( u8 *buf, u16 size, u8 bits )
{
	u16 readlen=0;
	u32 bread;
	u16 i;
	u8 *p;

	if( wavctrl.nchannels == 2)
	{
		if( bits == 24 ) 
		{
			readlen=(size/4)*3;							//�˴�Ҫ��ȡ���ֽ���
			f_read(audiodev.file,audiodev.tbuf,readlen,(UINT*)&bread);	//��ȡ����
			p=audiodev.tbuf;
			for(i=0;i<size;)
			{
				buf[i++]=p[1];
				buf[i]=p[2]; 
				i+=2;
				buf[i++]=p[0];
				p+=3;
			} 
			bread=(bread*4)/3; 
		}
		else if( bits == 16 ) 
		{
			f_read( audiodev.file, buf, size, (UINT*)&bread );
			if( bread < size ) 
			{
				for( i=bread; i< size-bread; i++)buf[i]=0; 
			}
		}
	}
	else
	{	/*how to do in 24bit?*/
		if( bits == 24 ) 
		{
			readlen=( size/4 )*3;
			f_read( audiodev.file, audiodev.tbuf, readlen, (UINT*)&bread );
			p = audiodev.tbuf;
			for( i = 0; i< size; )
			{
				buf[i++]=p[1];
				buf[i]=p[2]; 
				i+=2;
				buf[i++]=p[0];
				p+=3;
			} 
			bread=(bread*4)/3;
		}
		else if( bits == 16 )  
		{
			short *j = (short *) buf, *k = (short *) audiodev.tbuf;
			f_read( audiodev.file, audiodev.tbuf, size, ( UINT* ) &bread );
			for( i = 0; i < bread/2; i++ )
			{
				j[ 2 * i ] = k [ i ];
				j[ 2 * i + 1] = k [ i ];
			}			
			if( bread < size )
			{
				for( i = bread/2; i < size - bread; i++ )
				{
					j[ 2 * i ] = 0;
					j[ 2 * i + 1] = 0;
				}
			}
		}

	}
	return bread;
}  

void wav_i2s_dma_tx_callback( void ) 
{   
	u16 i;
	
	if(DMA1_Stream4->CR&(1<<19))
	{
		wavwitchbuf = 0;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf1[i]=0;//���0
			}
		}
	}else 
	{
		wavwitchbuf = 1;
		if((audiodev.status&0X01)==0)
		{
			for(i=0;i<WAV_I2S_TX_DMA_BUFSIZE;i++)//��ͣ
			{
				audiodev.i2sbuf2[i]=0;//���0
			}
		}
	}
	wavtransferend=1;
	
	vTaskNotifyGiveFromISR( pxMusicPlayer, NULL);
		
} 

void wav_get_curtime(FIL*fx,__wavctrl *wavx)
{
	long long fpos;  	
 	wavx->totsec=wavx->datasize/(wavx->bitrate/8);	//�����ܳ���(��λ:��) 
	fpos=fx->fptr-wavx->datastart; 					//�õ���ǰ�ļ����ŵ��ĵط� 
	wavx->cursec=fpos*wavx->totsec/wavx->datasize;	//��ǰ���ŵ��ڶ�������?	
} 

unsigned char uMusicPlayControl = 0;
signed char xMusicVolume = 20;
signed char xSpeakerVolume = 50;

u8 wav_play_song( u8* fname )
{
	u8 t = 0; 
	u8 res;  
	u32 fillnum; 
	FIL mFilewav;

	audiodev.file = &mFilewav;	
	uMusicPlayControl = 0;
	
CIRCLEPLAY:		
	audiodev.status = 0;
	res = wav_decode_init( fname, &wavctrl, &mFilewav );
	
	if( res==0 ) 
	{
		if( wavctrl.bps == 16 )
		{
			WM8978_I2S_Cfg( 2,0 ); 
			I2S2_Init( I2S_Standard_Phillips, I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_16bextended);		
			//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,16λ��չ֡����
		}else if( wavctrl.bps == 24 )
		{
			printf("%s: 24bit wav music!!!\r\n", __func__ );
			WM8978_I2S_Cfg(2, 2); 
			I2S2_Init( I2S_Standard_Phillips, I2S_Mode_MasterTx,I2S_CPOL_Low,I2S_DataFormat_24b);		
			//�����ֱ�׼,��������,ʱ�ӵ͵�ƽ��Ч,24λ��չ֡����
		}
		I2S2_SampleRate_Set( wavctrl.samplerate );
		I2S2_TX_DMA_Init( audiodev.i2sbuf1, audiodev.i2sbuf2,
			WAV_I2S_TX_DMA_BUFSIZE/2 );
		i2s_tx_callback = wav_i2s_dma_tx_callback;
		audio_stop();
		res = f_open( audiodev.file, (TCHAR*)fname, FA_READ );
		if( res == 0 )
		{
			f_lseek( audiodev.file, wavctrl.datastart );
			fillnum = wav_buffill_buffer( audiodev.i2sbuf1,
						WAV_I2S_TX_DMA_BUFSIZE/(2/wavctrl.nchannels), wavctrl.bps );
			fillnum = wav_buffill_buffer( audiodev.i2sbuf2,
						WAV_I2S_TX_DMA_BUFSIZE/(2/wavctrl.nchannels), wavctrl.bps );
			audio_start();  
			while( res == 0 )
			{ 
				ulTaskNotifyTake( pdTRUE, 1000 / portTICK_RATE_MS );
				
				if( fillnum != WAV_I2S_TX_DMA_BUFSIZE/(2/wavctrl.nchannels) )
					//go to end of file
				{
					printf("%s: read end of file!\r\n", __func__);
					res = KEY0_PRES;
					break;
				} 
				if( wavwitchbuf )
				{
					fillnum = wav_buffill_buffer(audiodev.i2sbuf2,
								WAV_I2S_TX_DMA_BUFSIZE/(2/wavctrl.nchannels),wavctrl.bps);
				}
				else 
				{
					fillnum = wav_buffill_buffer(audiodev.i2sbuf1,
								WAV_I2S_TX_DMA_BUFSIZE/(2/wavctrl.nchannels),wavctrl.bps);
				}
				while( 1 )
				{
					if( uMusicPlayControl == STOPRESUME )
					{
						if( audiodev.status & 0X01 )
						{
							audiodev.status &= ~( 1 << 0 );
						}
						else 
						{
							audiodev.status |= 0X01;  
						}
						uMusicPlayControl = 0;
					}
					if(uMusicPlayControl == NEXT || 
						uMusicPlayControl == PREVIOUS)
					{
						res = uMusicPlayControl;
						uMusicPlayControl = 0;
						break; 
					}
					wav_get_curtime( audiodev.file, &wavctrl ); 
					audio_msg_show(wavctrl.totsec,wavctrl.cursec,wavctrl.bitrate);
					t++;
					if(t==100)
					{
						t=0;
							LED0=!LED0;
					}
					if((audiodev.status & 0X01) == 0 ) 
					{
						//stop play in this while
						vTaskDelay( 200 / portTICK_RATE_MS );
					}
					else
					{
						//next frame to play
						break;
					}
				}
			}
			audio_stop(); 
			f_close( audiodev.file );	
		}
		else 
		{
			printf("1file name error: %s\r\n", fname);
			res=0XFF;
		}
	}
	else 
	{
		printf("2file name error: %s\r\n", fname);
		res=0XFF;
	}

	f_close( audiodev.file );
	if( res != 0xff && uMusicPlayControl == CIRCLE )
	{
		vTaskDelay( 200 / portTICK_RATE_MS );
		printf("%s: circle to play %s\r\n", __func__, fname);
		goto CIRCLEPLAY;
	}		

	return res;
} 

#if 1

#include "mp3dec.h"
#include "mp3common.h"

__mp3ctrl * mp3ctrl, mMp3Contorl;
vu8 mp3transferend = 0;
vu8 mp3witchbuf = 0;

 void mp3_i2s_dma_tx_callback( void ) 
{    
	u16 i;
	
	if( DMA1_Stream4->CR & ( 1 << 19 ) )
	{
		mp3witchbuf = 0;
		if( ( audiodev.status & 0X01 ) == 0 ) 
		{
			for(i=0;i<2304*2;i++)audiodev.i2sbuf1[i]=0;
		}
	}
	else 
	{
		mp3witchbuf = 1;
		if( ( audiodev.status & 0X01 ) == 0 ) 
		{
			for(i=0;i<2304*2;i++)audiodev.i2sbuf2[i]=0;
		}
	} 
	mp3transferend=1;

	vTaskNotifyGiveFromISR( pxMusicPlayer, NULL);	
} 
 
extern void Convert_Mono( short *buffer );
extern void Convert_Stereo( short *buffer );
	
void mp3_fill_buffer( int step, u16* buf, u16 size, u8 nch )
{
	u16 *p;
	
	while( mp3transferend == 0 && step > 1 )
	{
		ulTaskNotifyTake( pdTRUE, 1000 / portTICK_RATE_MS );
	}
	mp3transferend = 0;
	
	if( mp3witchbuf == 0 || step == 0 )
	{
		p = ( u16* ) audiodev.i2sbuf1;
	}
	else if( mp3witchbuf == 1 || step == 1) 
	{
		p = ( u16* ) audiodev.i2sbuf2;
	}
	memcpy( (char *)p, (char *)buf, size * sizeof( short ) );
	
	if(	nch == 2 )
	{
		Convert_Stereo( (short *) p );
	}
	else
	{
		Convert_Mono( (short *) p );
	}
} 
 

u8 mp3_id3v1_decode( u8* buf, __mp3ctrl *pctrl )
{
	ID3V1_Tag *id3v1tag;
	id3v1tag =( ID3V1_Tag* ) buf;
	/*�ײ�����ΪTAG*/
	if ( strncmp( "TAG", (char*) id3v1tag->id, 3) == 0 )
		//��MP3 ID3V1 TAG
	{
		if(id3v1tag->title[0])strncpy((char*)pctrl->title,(char*)id3v1tag->title,30);
		if(id3v1tag->artist[0])strncpy((char*)pctrl->artist,(char*)id3v1tag->artist,30); 
		if(id3v1tag->album[0])strncpy((char*)pctrl->album,(char*)id3v1tag->album,30);
		if(id3v1tag->year[0])strncpy((char*)pctrl->year,(char*)id3v1tag->year,4);
		if(id3v1tag->comment[0])strncpy((char*)pctrl->comment,(char*)id3v1tag->comment,30);
		pctrl->genre = id3v1tag->genre ;

		printf("%s: title(%s)\r\n", __func__, pctrl->title);
		printf("%s: artist(%s)\r\n", __func__, pctrl->artist);
		printf("%s: album(%s)\r\n", __func__, pctrl->album);
		printf("%s: year(%s)\r\n", __func__, pctrl->year);
		printf("%s: comment(%s)\r\n", __func__, pctrl->comment);
		printf("%s: genre(%d)\r\n", __func__, pctrl->genre);
	}
	else 
		return 1;
	return 0;
}

u8 mp3_id3v2_decode(u8* buf, u32 size, __mp3ctrl *pctrl)
{
	ID3V2_TagHead *taghead;
	ID3V23_FrameHead *framehead; 
	u32 t;
	u32 tagsize;	//tag��С
	u32 frame_size;	//֡��С 
	taghead=(ID3V2_TagHead*)buf; 
	/*�ײ�����ΪID3�����򲻴��������Ϣ*/
	if(strncmp("ID3",(const char*)taghead->id,3)==0)//����ID3?
	{
		tagsize=((u32)taghead->size[0]<<21)|
					((u32)taghead->size[1]<<14)|
					((u16)taghead->size[2]<<7)|
					taghead->size[3];
		//�õ�tag ��С
		pctrl->datastart = tagsize;		
		//�õ�mp3���ݿ�ʼ��ƫ����
		if( tagsize > size )
			tagsize = size;	
		//tagsize��������bufsize��ʱ��,ֻ��������size��С������
		if(taghead->mversion < 3 )
		{
			printf("not supported mversion!\r\n");
			return 1;
		}
		
		t=10;
		while( t < tagsize )
		{
			framehead=(ID3V23_FrameHead*)( buf + t );
			frame_size=((u32)framehead->size[0]<<24)|
						((u32)framehead->size[1]<<16)|
						((u32)framehead->size[2]<<8)|
						framehead->size[3];
						//�õ�֡��С
 			if (strncmp("TT2",(char*)framehead->id,3)==0||
				strncmp("TIT2",(char*)framehead->id,4)==0)
				//�ҵ���������֡,��֧��unicode��ʽ!!
			{
				strncpy(( char* ) pctrl->title,
					(char*)(buf + t + sizeof( ID3V23_FrameHead ) + 1 ),
					AUDIO_MIN( frame_size - 1, MP3_TITSIZE_MAX - 1 ));
				printf("%s: title(%s)\r\n", __func__, pctrl->title);
			}
 			if (strncmp("TP1",(char*)framehead->id,3)==0||
				strncmp("TPE1",(char*)framehead->id,4)==0)
				//�ҵ�����������֡
			{
				strncpy((char*)pctrl->artist,
					(char*)(buf+t+sizeof(ID3V23_FrameHead)+1),
					AUDIO_MIN(frame_size-1,MP3_ARTSIZE_MAX-1));
				printf("%s: artist(%s)\r\n", __func__, pctrl->artist);				
			}
/*
TIT2������
TPE1������
TALB��ר��

TRCK�� ���죬��ʽ��N/M��N��ʾר���еڼ��ף�
MΪר���и�������

TYER�����
TCON������

*/
			t += frame_size + sizeof( ID3V23_FrameHead );
		} 
	}
	else 
	{
		pctrl->datastart = 0;
		//������ID3,mp3�����Ǵ�0��ʼ
	}

	return 0;
} 

//��ȡMP3������Ϣ
//pname:MP3�ļ�·��
//pctrl:MP3������Ϣ�ṹ�� 
//����ֵ:0,�ɹ�
//    ����,ʧ��
/**

1)      ����֡���Ƕ��٣�ÿ֡�Ĳ���ʱ�䶼��26ms
2)      ����֡��С��
FrameSize = 144 * Bitrate / SamplingRate + PaddingBit
��144 * Bitrate / SamplingRate���ܱ�8�������������Ӧ��paddingBit.


C,LAME��ǩ֡
���ǣ�������Ĵ�һ��MP3�ļ���ʱ����ᷢ�֣�
����֣��ܶ�ʱ���һ������֡��֡ͷ�����32
���ֽھ�Ȼ��Ϊ0������Ϊʲô�أ���ô��ֵĽ���
��Ϣ����ν��ͣ��ҵ�MP3 INFO TAG REV SPECIFICATION����վ��
�Ҳ����ף�ԭ����һ֡����������������֡������
LAME����ı�־֡��
������Ҫǣ�浽�������CBR��VBR��CBR��ʾ������
���䣬Ҳ����ÿ֡�ĳ�����һ�µģ������ַ�����
INFO��Ϊ��ǡ�VBR��Variable BitRate�ļ�ƣ�Ҳ����ÿ֡��
�����ʺ�֡�ĳ����Ǳ仯�ģ������ַ�����Xing��Ϊ
��ǡ�ͬʱ�����������MP3�ļ���֡���ܸ�������
100���ֽڵĲ�����ʱ��ֶε�֡��INDEX����������һ
Щ�������ⱻ��ΪZone A����ͳXing VBR��ǩ���ݣ���120��
�ֽڡ�
�ڶ������ı��༭�������ǻ��ɿ���һ���ַ�����
LAME�������Һ�������ظ��Ű汾�š������20����
�ڵ�Zone B��ʼLAME��Ϣ����ʾ���ļ�����LAME���뼼����
������һֱ����֡��������Zone C��LAME��ǩ��

**/
u8 mp3_get_info(u8 *pname, __mp3ctrl* pctrl)
{
    HMP3Decoder decoder;
    MP3FrameInfo frame_info;
	MP3_FrameXing* fxing;
	MP3_FrameVBRI* fvbri;
	FIL*fmp3, mFileMp3;
	u8 *buf;
	u32 br;
	u8 res;
	int offset=0;
	u32 p;
	short samples_per_frame;	//һ֡�Ĳ�������
	u32 totframes;				//��֡��
	
	fmp3= &mFileMp3;
	buf=audiodev.i2sbuf1;
	
	f_open( fmp3, (const TCHAR*)pname, FA_READ ); 
	res = f_read( fmp3, (char*)buf, 5*1024, &br );
	if( res == 0 ) 
	{  
		mp3_id3v2_decode(buf, br, pctrl);	//����ID3V2����
		f_lseek(fmp3,fmp3->fsize-128);	//ƫ�Ƶ�����128��λ��
		f_read(fmp3,(char*)buf,128,&br);//��ȡ128�ֽ�
		mp3_id3v1_decode(buf,pctrl);	//����ID3V1����  
		decoder=MP3InitDecoder(); 		//MP3���������ڴ�
		f_lseek(fmp3,pctrl->datastart);	//ƫ�Ƶ����ݿ�ʼ�ĵط�
		f_read(fmp3,(char*)buf, 5*1024, &br);	//��ȡ5K�ֽ�mp3����
			offset=MP3FindSyncWord(buf,br);	//����֡ͬ����Ϣ
		if(offset>=0 && MP3GetNextFrameInfo(decoder,&frame_info,&buf[offset]) == 0 )
		//�ҵ�֡ͬ����Ϣ��,����һ����Ϣ��ȡ����	
		{ 
			p = offset + 4 + 32;
			fvbri = (MP3_FrameVBRI*)( buf + p );
			
			if( strncmp("VBRI", (char*)fvbri->id,4) == 0 )//����VBRI֡(VBR��ʽ)
			{
				printf("%s: VBRI\r\n", __func__);
				if (frame_info.version==MPEG1)
				{
					samples_per_frame=1152;
					//MPEG1,layer3ÿ֡����������1152
				}
				else 
				{
					samples_per_frame=576;
					//MPEG2/MPEG2.5,layer3ÿ֡����������576 
					}
				totframes = ((u32)fvbri->frames[0]<<24)|
					((u32)fvbri->frames[1]<<16)|
					((u16)fvbri->frames[2]<<8)|
					fvbri->frames[3];
				//�õ���֡��
				pctrl->totsec=totframes*samples_per_frame/frame_info.samprate;
				//�õ��ļ��ܳ���
			}
			else	//����VBRI֡,�����ǲ���Xing֡(VBR��ʽ)
			{  
				if (frame_info.version==MPEG1)	
					//MPEG1 
				{
					p=frame_info.nChans==2?32:17;
					samples_per_frame = 1152;	
					//MPEG1,layer3ÿ֡����������1152
				}
				else
				{
					p=frame_info.nChans==2?17:9;
					samples_per_frame=576;		
					//MPEG2/MPEG2.5,layer3ÿ֡����������576
				}
				p+=offset+4;
				fxing=(MP3_FrameXing*)(buf+p);
				if(strncmp("Xing",(char*)fxing->id,4)==0||strncmp("Info",(char*)fxing->id,4)==0)
					//��Xng֡
				{
					printf("%s: Xing \r\n", __func__);
				
					if(fxing->flags[3]&0X01)
						//������frame�ֶ�
					{
						totframes=((u32)fxing->frames[0]<<24)|((u32)fxing->frames[1]<<16)|((u16)fxing->frames[2]<<8)|fxing->frames[3];//�õ���֡��
						pctrl->totsec=totframes*samples_per_frame/frame_info.samprate;//�õ��ļ��ܳ���
					}
					else	
						//��������frames�ֶ�
					{
						pctrl->totsec=fmp3->fsize/(frame_info.bitrate/8);
					} 
				}
				else 		
					//CBR��ʽ,ֱ�Ӽ����ܲ���ʱ��
				{
					pctrl->totsec = fmp3->fsize/(frame_info.bitrate/8);
				}
			} 
			pctrl->bitrate=frame_info.bitrate;			
			//�õ���ǰ֡������
			mp3ctrl->samplerate=frame_info.samprate; 	
			//�õ�������. 
			printf("%s:frame_info.bitsPerSample=%d\r\n", __func__, frame_info.bitsPerSample);
			printf("%s:frame_info.nChans=%d\r\n", __func__, frame_info.nChans);
			if(frame_info.nChans == 2 )
			{
				mp3ctrl->outsamples=frame_info.outputSamps; 
				//���PCM��������С 
			}
			else 
			{
				mp3ctrl->outsamples=frame_info.outputSamps * 2; 
				//���PCM��������С,���ڵ�����MP3,ֱ��*2,
				//����Ϊ˫�������
			}
		}
		else
		{
			res=0XFE;//δ�ҵ�ͬ��֡	
		}
		MP3FreeDecoder( decoder );//�ͷ��ڴ�		
	} 
	
	f_close( fmp3 );

	return res;	
}  

void mp3_get_curtime( FIL* fx, __mp3ctrl* mp3x )
{
	u32 fpos = 0;  	 
	if( fx->fptr>mp3x->datastart )fpos = fx->fptr-mp3x->datastart;	//�õ���ǰ�ļ����ŵ��ĵط� 
	mp3x->cursec = fpos*mp3x->totsec/(fx->fsize-mp3x->datastart);	//��ǰ���ŵ��ڶ�������?	
}

u32 mp3_file_seek( u32 pos )
{
	if( pos > audiodev.file->fsize )
	{
		pos = audiodev.file->fsize;
	}
	f_lseek( audiodev.file, pos );
	return audiodev.file->fptr;
}

static unsigned char pcMp3Buffer[ MP3_FILE_BUF_SZ ];

u8 mp3_play_song( u8* fname )
{ 
	u8 res = NEXT;
	u8* buffer = NULL;
	u8* readptr = NULL;
	int offset = 0;
	int outofdata = 0;
	int bytesleft = 0;
	u32 br = 0; 
	int err = 0;  
	int step = 0;
	HMP3Decoder mp3decoder = NULL;
	MP3FrameInfo mp3frameinfo;
	FIL mFileMp3;
	
	buffer = pcMp3Buffer;
 	mp3ctrl = &mMp3Contorl;
	audiodev.file = &mFileMp3;
	audiodev.file_seek = mp3_file_seek;
	uMusicPlayControl = 0;
	
CIRCLEPLAY:	
	memset( audiodev.i2sbuf1, 0, 2304 * 2 );
	memset( audiodev.i2sbuf2, 0, 2304 * 2 );
	memset( mp3ctrl, 0, sizeof( __mp3ctrl ) );
	res = mp3_get_info( fname, mp3ctrl ); 
	
	if( res == 0 )
	{ 
		printf("title:%s\r\n", mp3ctrl->title); 
		printf("artist:%s\r\n", mp3ctrl->artist); 
		printf("bitrate:%dbps\r\n", mp3ctrl->bitrate);	
		printf("samplerate:%d\r\n", mp3ctrl->samplerate);	
		printf("totalsec:%d\r\n", mp3ctrl->totsec); 
		printf("outsamples:%d\r\n", mp3ctrl->outsamples);
		
		WM8978_I2S_Cfg( 2, 0 );
		I2S2_Init( I2S_Standard_Phillips, I2S_Mode_MasterTx, I2S_CPOL_Low, I2S_DataFormat_16bextended );	
		I2S2_SampleRate_Set( mp3ctrl->samplerate );
		I2S2_TX_DMA_Init( audiodev.i2sbuf1, audiodev.i2sbuf2,  mp3ctrl->outsamples );
		i2s_tx_callback = mp3_i2s_dma_tx_callback;
		mp3decoder = MP3InitDecoder();
		res = f_open( audiodev.file, (char*)fname, FA_READ );
	}
	if( res == 0 && mp3decoder != 0 ) 
	{ 
		f_lseek( audiodev.file, mp3ctrl->datastart );
		audio_stop();
		printf("%s: name (%s), mp3ctrl->datastart (%d)\r\n", 
			__func__, fname, mp3ctrl->datastart );
		
		while( res == 0 )
		{
			readptr = buffer;	
			// mp3 read pointer
			offset = 0;	
			outofdata = 0;
			bytesleft = 0; 	
			res = f_read( audiodev.file, buffer, MP3_FILE_BUF_SZ, &br );
			//һ�ζ�ȡMP3_FILE_BUF_SZ�ֽ�
			if( res )
			{
				res = 0xff;
				break;
			}
			if( br == 0 )
			{
				res = NEXT;// finish
				break;
			}
			bytesleft += br; 
			err = 0;			
			while( !outofdata ) 
			{
				offset = MP3FindSyncWord( readptr, bytesleft ); 
				if( offset < 0 ) 
				{ 
					outofdata = 1; 
					res = NEXT;
					printf("%s: outofdata(1)\r\n", __func__);
				}
				else	 
				{
					readptr += offset;		 
					bytesleft -= offset;
					//printf("in bytesleft(%d)\r\n", bytesleft);
					err = MP3Decode( mp3decoder, &readptr, &bytesleft, (short*)audiodev.tbuf, 0 ); 
					if( err != 0 )
					{
						printf("MP3Decode error:%d\r\n",err);
						res = NEXT;
						break;
					}
					else
					{
						//printf("out bytesleft(%d)\r\n", bytesleft);
						MP3GetLastFrameInfo( mp3decoder, &mp3frameinfo );
						if(mp3ctrl->bitrate!= mp3frameinfo.bitrate)		
						{
							printf("bitrate change from %d to %d\r\n", mp3ctrl->bitrate,
								mp3frameinfo.bitrate);
							mp3ctrl->bitrate = mp3frameinfo.bitrate; 
						}
						if( step > 2 )
						{
							mp3_fill_buffer(step, (u16*)audiodev.tbuf, 
								mp3frameinfo.outputSamps, mp3frameinfo.nChans );						
						}
						else if( step == 0 || step == 1 )
						{
							mp3_fill_buffer(step, (u16*)audiodev.tbuf, 
								mp3frameinfo.outputSamps, mp3frameinfo.nChans ); 
							step++;
						}
						else if( step == 2 )
						{
							step++;
							audio_start();
						}
					}
					
					if( bytesleft < MAINBUF_SIZE * 2 )
					{ 
						memmove( buffer, readptr, bytesleft );
						f_read( audiodev.file, buffer + bytesleft, MP3_FILE_BUF_SZ - bytesleft, &br );
						if( br < MP3_FILE_BUF_SZ - bytesleft )
						{
							memset(buffer+bytesleft+br,0,MP3_FILE_BUF_SZ-bytesleft-br); 
						}
						bytesleft = MP3_FILE_BUF_SZ;  
						readptr = buffer; 
					} 	

					while( 1 )
					{
						if( uMusicPlayControl == STOPRESUME )// stop
						{
							if( audiodev.status & 0X01 )
								audiodev.status &= ~( 1 << 0 );
							else 
								audiodev.status |= 0X01;  

							uMusicPlayControl = 0;
						}
						if( uMusicPlayControl == NEXT ||
							uMusicPlayControl == PREVIOUS ) // next pre
						{
							res = uMusicPlayControl;
							uMusicPlayControl = 0;
							outofdata = 1;
							printf("next or pre! res(%d)\r\n", res);							
							break; 
						}	
						
						if( ( audiodev.status & 0X01 ) == 0 && step > 2 ) 
						{
							vTaskDelay( 200 / portTICK_RATE_MS );
						}
						else
						{
							break;
						}
					}
				}					
			}  
		}
		audio_stop();
	}
	else 
	{
		res = 0xff;
	}
	
	f_close( audiodev.file );
	if( res != 0xff && uMusicPlayControl == CIRCLE )
	{
		vTaskDelay( 200 / portTICK_RATE_MS );
		printf("%s: circle to play %s\r\n", __func__, fname);
		goto CIRCLEPLAY;
	}

	MP3FreeDecoder( mp3decoder );
	return res;
}
#endif

void MusicAddVolume( void )
{
	xMusicVolume += 5;
	if( xMusicVolume > 63 )
	{
		xMusicVolume -= 5;
		return;
	}		
	WM8978_HPvol_Set(xMusicVolume, xMusicVolume);	
}

void MusicDelVolume( void )
{
	xMusicVolume -= 5;
	if( xMusicVolume < 0 )
	{
		xMusicVolume += 5;
		return;
	}	
	WM8978_HPvol_Set(xMusicVolume, xMusicVolume);	
}

void SpeakerAddVolume( void )
{
	xSpeakerVolume += 2;
	if( xSpeakerVolume > 63 )
	{
		xSpeakerVolume -= 2;
		return;
	}	
	WM8978_SPKvol_Set(xSpeakerVolume);	
}

void SpeakerDelVolume( void )
{
	xSpeakerVolume -= 2;
	if( xSpeakerVolume < 0 )
	{
		xSpeakerVolume += 2;
		return;
	}
	WM8978_SPKvol_Set(xSpeakerVolume);	
}

void MusicNext( void )
{
	uMusicPlayControl = NEXT;
}

void MusicPrevious( void )
{
	uMusicPlayControl = PREVIOUS;
}

void MusicStop( void )
{
	uMusicPlayControl = STOPRESUME;
}

void MusicContinue( void )
{
	uMusicPlayControl = STOPRESUME;
}	

void MusicCircle( void )
{
	uMusicPlayControl = CIRCLE;
}	







