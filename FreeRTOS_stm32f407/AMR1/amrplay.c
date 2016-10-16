
/*************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
********************************************************************************
*
*      File             : decoder.c
*      Purpose          : Speech decoder main program.
*
********************************************************************************
*
*         Usage : decoder  bitstream_file  synth_file
*
*
*         Format for bitstream_file:
*             1 word (2-byte) for the frame type
*               (see frame.h for possible values)
*               Normally, the TX frame type is expected.
*               RX frame type can be forced with "-rxframetype"
*           244 words (2-byte) containing 244 bits.
*               Bit 0 = 0x0000 and Bit 1 = 0x0001
*             1 word (2-byte) for the mode indication
*               (see mode.h for possible values)
*             4 words for future use, currently unused
*
*         Format for synth_file:
*           Synthesis is written to a binary file of 16 bits data.
*
********************************************************************************
*/

/*
********************************************************************************
*                         INCLUDE FILES
********************************************************************************
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "typedef.h"
#include "n_proc.h"
#include "cnst.h"
#include "mode.h"
#include "frame.h"
#include "strfunc.h"
#include "sp_dec.h"
#include "d_homing.h"
#include "ff.h"
#include "audioplay.h"
#include "usart.h" 
#include "delay.h" 
#include "ff.h"
#include "i2s.h"
#include "wm8978.h"
#include "led.h"
#include "iwdg.h"

extern TaskHandle_t pxMusicPlayer;
extern unsigned char uMusicPlayControl;
#define AMR_MAGIC_NUMBER "#!AMR\n"
#define MAX_PACKED_SIZE (MAX_SERIAL_SIZE / 8 + 2)
const char decoder_id[] = "@(#)$Id $";
/* frame size in serial bitstream file (frame type + serial stream + flags) */
#define SERIAL_FRAMESIZE (1+MAX_SERIAL_SIZE+5)

/*
********************************************************************************
*                             MAIN PROGRAM 
********************************************************************************
*/

typedef enum 
{
	STEP0 = 0,
	STEP1,
	STEP2,
	STEP3,
} INITSTEP;

unsigned char amrwitchbuf = 0;
unsigned char amrtransferend = 0;
unsigned int amrsamplerate = 8000;

void amr_set_sample_rate( unsigned int rate )
{
	amrsamplerate = rate;
	printf("%s: amrsamplerate = %d\r\n", __func__, amrsamplerate);
}

 void amr_i2s_dma_tx_callback( void ) 
{    
	int i;
	
	if( DMA1_Stream4->CR & ( 1 << 19 ) )
	{
		amrwitchbuf = 0;
		if( ( audiodev.status & 0X01 ) == 0 ) 
		{
			for( i = 0; i < 20 * L_FRAME * 2; i++ )
				audiodev.i2sbuf1[i] = 0;
		}
	}
	else 
	{
		amrwitchbuf = 1;
		if( ( audiodev.status & 0X01 ) == 0 ) 
		{
			for( i = 0; i < 20 * L_FRAME * 2; i++ )
				audiodev.i2sbuf2[i] = 0;
		}
	} 
	
	amrtransferend = 1;
	vTaskNotifyGiveFromISR( pxMusicPlayer, NULL);	
} 


void amr_fill_buffer(short *p, Word16 *synth, int channel)
{
	int i, size = L_FRAME;

	
	if( channel == 2 )//Stereo
	{
		for(i=0; i < size; i++)
		{
			p[i] = synth[i];
		}
	}
	else//mono
	{
		for( i=0; i<size; i++)
		{
			p[ 2*i ] = synth[i];
			p[ 2*i + 1] = synth[i];
		}
	}

}

#define N_L_FRAME 12

int amr_play ( char *serialFileName )
{
  int res = NEXT, times = 0;
  unsigned char *p = NULL;
  INITSTEP step = 0;
  unsigned int br;
  Speech_Decode_FrameState *speech_decoder_state = NULL;
  Word16 serial[SERIAL_FRAMESIZE];   /* coded bits                    */
  Word16 synth[L_FRAME];             /* Synthesis                     */
  FIL *file_serial = NULL, mFileAmr;
  enum Mode mode = (enum Mode)0;
  enum RXFrameType rx_type = (enum RXFrameType)0;
  Word16 reset_flag = 0;
  Word16 reset_flag_old = 1;
  Word16 i;
  
  UWord8 toc, q, ft;
  Word8 magic[8];
  UWord8 packed_bits[MAX_PACKED_SIZE];
  Word16 packed_size[16] = {12, 13, 15, 17, 19, 20, 26, 31, 5, 0, 0, 0, 0, 0, 0, 0};

  //audiodev.i2sbuf1 = pvPortMalloc( 2 * L_FRAME * 20 );
  //audiodev.i2sbuf2 = pvPortMalloc( 2 * L_FRAME * 20 );
  file_serial = &mFileAmr;
  //( FIL* ) pvPortMalloc( sizeof( FIL ) ); 
  if( !file_serial )
  {
  	printf("%s: pvPortMalloc fail!\r\n", __func__);
  	goto FAIL1;
  }
  proc_head ("Decoder");

  printf("start f_open %s\r\n", serialFileName);  
  if ((f_open (file_serial, (const TCHAR*)serialFileName, FA_READ )) != FR_OK)
  {
      printf("Input file '%s' does not exist !!\n", serialFileName);
      goto FAIL1;
  }
  printf("Input bitstream file:   %s\r\n", serialFileName);

  memset(magic, 0, 8);
   /* read and verify magic number */
  f_read(file_serial, magic, 1 * strlen(AMR_MAGIC_NUMBER), &br);
  if (
  	/*br != sizeof(Word8) * strlen(AMR_MAGIC_NUMBER) ||*/
  	strncmp((const char *)magic, AMR_MAGIC_NUMBER, strlen(AMR_MAGIC_NUMBER)))
  {
	   printf("%s%s\n", "Invalid magic number: \r\n", magic);
	   goto FAIL2;
  }
  printf("magic=%s\r\n", magic);

  /*-----------------------------------------------------------------------*
   * Initialization of decoder                                             *
   *-----------------------------------------------------------------------*/
  if (Speech_Decode_Frame_init(&speech_decoder_state, "Decoder"))
  {
      goto FAIL2;
  }
  else
  {
	printf("%s: Speech_Decode_Frame_init ok!\r\n", __func__);
  }

  uMusicPlayControl = 0;
  WM8978_I2S_Cfg( 2, 0 );
  I2S2_Init( I2S_Standard_Phillips, I2S_Mode_MasterTx, I2S_CPOL_Low, I2S_DataFormat_16b );	  
  if( 0 == I2S2_SampleRate_Set( amrsamplerate ) )
  {
  	printf("%s: set sample rate to %d scueess!\r\n", __func__, amrsamplerate);
  }
  else
  {
  	printf("%s: set sample rate to %d fail!!!\r\n", __func__, amrsamplerate);
	goto FAIL2;
  }
  I2S2_TX_DMA_Init( audiodev.i2sbuf1, audiodev.i2sbuf2,  L_FRAME * N_L_FRAME * 2 );
  //L_FRAME * N_L_FRAME * 2 for mono
  //L_FRAME * N_L_FRAME  for stero
  i2s_tx_callback = amr_i2s_dma_tx_callback;
  audio_stop();
  
  /*-----------------------------------------------------------------------*
   * process serial bitstream frame by frame                               *
   *-----------------------------------------------------------------------*/
  res = 0;
  while ( res == 0 && f_read (file_serial, &toc, 1, &br ) == FR_OK)
  {
  	  if( br == 0 )
  	  {
  	    res = NEXT;
	  	break;
  	  }
	  /* read rest of the frame based on ToC byte */
	  q  = (toc >> 2) & 0x01;
	  ft = (toc >> 3) & 0x0F;
	  if( f_read (file_serial, packed_bits, packed_size[ft], &br) != FR_OK )
	  {
	  	printf("%s: f_read error!\r\n", __func__);
		res = NEXT;
		goto FAIL3;
	  }

	  rx_type = UnpackBits(q, ft, packed_bits, &mode, &serial[1]);

     if (rx_type == RX_NO_DATA) {
       mode = speech_decoder_state->prev_mode;
     }
     else {
       speech_decoder_state->prev_mode = mode;
     }

     /* if homed: check if this frame is another homing frame */
     if (reset_flag_old == 1)
     {
         /* only check until end of first subframe */
         reset_flag = decoder_homing_frame_test_first( &serial[1], mode );
     }
     /* produce encoder homing frame if homed & input=decoder homing frame */
     if ((reset_flag != 0) && (reset_flag_old != 0))
     {
         for (i = 0; i < L_FRAME; i++)
         {
             synth[i] = EHF_MASK;
         }
     }
     else
     {     
         /* decode frame */
         Speech_Decode_Frame(speech_decoder_state, mode, &serial[1],
                             rx_type, synth);
     }

     if( step == STEP3 )
     {	 		
		while( amrtransferend == 0 && times == 0 )
		{
			ulTaskNotifyTake( pdTRUE, 800 / portTICK_RATE_MS );
		}
		amrtransferend = 0;

		if( amrwitchbuf == 0 )
		{
			p =  audiodev.i2sbuf1;
		}
		else 
		{
			p =  audiodev.i2sbuf2;
		}
		
		if( times >= N_L_FRAME-1 )
		{
			//memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) );
			amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
			times = 0;
		}
		else
		{
			//memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) );
			amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
     		times++;		
		}
     }	 
     else if( step == STEP0 )
     {
		 p =  audiodev.i2sbuf1;
		if( times >= N_L_FRAME - 1 )
		{
     		step++;
			//memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) ); 
			amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
			times = 0;
		}
		else
		{
			//memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) ); 
			amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
     		times++;		
		}
	}
	 else if( step == STEP1 )
	 {
	 
		 p =  audiodev.i2sbuf2;
		 if( times >= N_L_FRAME - 1 )
		 {
			 step++;
			 //memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) );	
			 //for mono we need to  p + (2*times * sizeof( synth ))
			 //for Stereo we set p + (times * sizeof( synth ))
			 amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
			 times = 0;
			 audio_start();
			 step++;
		 }
		 else
		 {
			 //memcpy( p + times * sizeof( synth ), synth, sizeof( synth ) );	
			 amr_fill_buffer((short *)(p + 2*times * sizeof( synth )), synth, 1);
			 times++;		 
		 }
	 }

     /* if not homed: check whether current frame is a homing frame */
     if (reset_flag_old == 0)
     {
         /* check whole frame */
         reset_flag = decoder_homing_frame_test(&serial[1], mode);
     }
     /* reset decoder if current frame is a homing frame */
     if (reset_flag != 0)
     {
     	 printf("reset decoder if current frame is a homing frame!\r\n");
         Speech_Decode_Frame_reset(speech_decoder_state);
     }
     reset_flag_old = reset_flag;
	 //( void ) IWDG_Feed();

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
		if( uMusicPlayControl == NEXT || uMusicPlayControl == PREVIOUS ) // next pre
		{
			res = uMusicPlayControl;
			uMusicPlayControl = 0;
			printf("next or pre! res(%d)\r\n", res);							
			break; 
		}	
		
		if( ( audiodev.status & 0X01 ) == 0 &&  step == STEP3 ) 
		{
			vTaskDelay( 200 / portTICK_RATE_MS );
		}
		else
		{
			break;
		}
	}	 
  }
  printf("all AMR frame processed\r\n");
  
  /*-----------------------------------------------------------------------*
   * Close down speech decoder                                             *
   *-----------------------------------------------------------------------*/
  
  audio_stop();
FAIL3:
	Speech_Decode_Frame_exit(&speech_decoder_state);
FAIL2:
  	f_close(file_serial);
FAIL1:
	//vPortFree( audiodev.i2sbuf1 );
	//vPortFree( audiodev.i2sbuf2 );
	//vPortFree( file_serial );	
	//audiodev.i2sbuf1 = NULL;
	//audiodev.i2sbuf2 = NULL;
	
  return res;
}
