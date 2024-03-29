/*
*****************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
*****************************************************************************
*
*      File             : sp_dec.c
*      Purpose          : Decoding and post filtering of one speech frame.
*
*****************************************************************************
*/
 
/*
*****************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*****************************************************************************
*/
#include "sp_dec.h"
const char sp_dec_id[] = "@(#)$Id $" sp_dec_h;
 
/*
*****************************************************************************
*                         INCLUDE FILES
*****************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include "typedef.h"
#include "basic_op.h"
#include "count.h"
#include "cnst.h"
#include "set_zero.h"
#include "dec_amr.h"
#include "pstfilt.h"
#include "bits2prm.h"
#include "mode.h"
#include "post_pro.h"

#ifdef MMS_IO
#include "bitno.tab"
#endif

#if(STATIC_MEMORY_ALLOCK == 1)
	Speech_Decode_FrameState mStaticSpeech;
#endif

/*
*****************************************************************************
*                         LOCAL VARIABLES AND TABLES
*****************************************************************************
*/
/*---------------------------------------------------------------*
 *    Constants (defined in "cnst.h")                            *
 *---------------------------------------------------------------*
 * L_FRAME     :
 * M           :
 * PRM_SIZE    :
 * AZ_SIZE     :
 * SERIAL_SIZE :
 *---------------------------------------------------------------*/

/*
*****************************************************************************
*                         PUBLIC PROGRAM CODE
*****************************************************************************
*/
 
/*************************************************************************
*
*  Function:   Speech_Decode_Frame_init
*  Purpose:    Allocates memory for filter structure and initializes
*              state memory
*
**************************************************************************
*/
int Speech_Decode_Frame_init (Speech_Decode_FrameState **state,
                              char *id)
{
  Speech_Decode_FrameState* s;
 
  if (state == (Speech_Decode_FrameState **) NULL){
      printf("Speech_Decode_Frame_init: invalid parameter\n");
      return -1;
  }
  *state = NULL;

#if(STATIC_MEMORY_ALLOCK == 1)
	s = &mStaticSpeech;
	//memset((char *)s, 0, sizeof(Speech_Decode_FrameState));
#else 
  /* allocate memory */
  if ((s= (Speech_Decode_FrameState *)
          pvPortMalloc(sizeof(Speech_Decode_FrameState))) == NULL) {
      printf("Speech_Decode_Frame_init: can not malloc state "
              "structure\n");
      return -1;
  }
#endif
  
  s->decoder_amrState = NULL;
  s->post_state = NULL;
  s->postHP_state = NULL;

  if (Decoder_amr_init(&s->decoder_amrState) ||
      Post_Filter_init(&s->post_state) ||
      Post_Process_init(&s->postHP_state) ) {
      Speech_Decode_Frame_exit(&s);
      return -1;
  }

  s->complexityCounter = getCounterId(id);
  
  Speech_Decode_Frame_reset(s);
  *state = s;
  
  return 0;
}
 
/*************************************************************************
*
*  Function:   Speech_Decode_Frame_reset
*  Purpose:    Resetses state memory
*
**************************************************************************
*/
int Speech_Decode_Frame_reset (Speech_Decode_FrameState *state)
{
  if (state == (Speech_Decode_FrameState *) NULL){
      printf("Speech_Decode_Frame_reset: invalid parameter\n");
      return -1;
  }
  
  Decoder_amr_reset(state->decoder_amrState, (enum Mode)0);
  Post_Filter_reset(state->post_state);
  Post_Process_reset(state->postHP_state);

  state->prev_mode = (enum Mode)0;

  setCounter(state->complexityCounter);
  Init_WMOPS_counter();
  setCounter(0); /* set counter to global counter */

  return 0;
}
 
/*
**************************************************************************
*
*  Function:   Speech_Decode_Frame_exit
*  Purpose:    The memory used for state memory is freed
*
**************************************************************************
*/
void Speech_Decode_Frame_exit (Speech_Decode_FrameState **state)
{
  if (state == NULL || *state == NULL)
      return;
 
  Decoder_amr_exit(&(*state)->decoder_amrState);
  Post_Filter_exit(&(*state)->post_state);
  Post_Process_exit(&(*state)->postHP_state);

  setCounter((*state)->complexityCounter);
  WMOPS_output(0);
  setCounter(0); /* set counter to global counter */

 
#if(STATIC_MEMORY_ALLOCK == 1)
#else
  /* deallocate memory */
  vPortFree(*state);
#endif
  *state = NULL;
  
  return;
}
 
int Speech_Decode_Frame (
    Speech_Decode_FrameState *st, /* io: post filter states                */
    enum Mode mode,               /* i : AMR mode                          */
    Word16 *serial,               /* i : serial bit stream                 */
    enum RXFrameType frame_type,    /* i : Frame type                        */
    Word16 *synth                 /* o : synthesis speech (postfiltered    */
                                  /*     output)                           */
)
{
  Word16 parm[MAX_PRM_SIZE + 1];  /* Synthesis parameters                */
  Word16 Az_dec[AZ_SIZE];         /* Decoded Az for post-filter          */
                                  /* in 4 subframes                      */

#if !defined(NO13BIT)
  Word16 i;
#endif

  setCounter(st->complexityCounter);
  Reset_WMOPS_counter ();          /* reset WMOPS counter for the new frame */


  /* Serial to parameters   */
  test(); test(); sub(0,0); sub(0,0); logic16();
  if ((frame_type == RX_SID_BAD) ||
      (frame_type == RX_SID_UPDATE)) {
    /* Override mode to MRDTX */
    Bits2prm (MRDTX, serial, parm);
  } else {
    Bits2prm (mode, serial, parm);
  }

  /* Synthesis */
  Decoder_amr(st->decoder_amrState, mode, parm, frame_type,
              synth, Az_dec);

  Post_Filter(st->post_state, mode, synth, Az_dec);   /* Post-filter */

  /* post HP filter, and 15->16 bits */
  Post_Process(st->postHP_state, synth, L_FRAME);  
  
#if !defined(NO13BIT)
  /* Truncate to 13 bits */
  for (i = 0; i < L_FRAME; i++) 
  {
     synth[i] = synth[i] & 0xfff8;    logic16 (); move16 ();
  }
#endif
  
  fwc();          /* function worst case */
  setCounter(0); /* set counter to global counter */
  return 0;
}

#ifdef MMS_IO

/*
**************************************************************************
*
*  Function    : UnpackBits
*  Purpose     : Unpack and re-arrange bits from file storage format to the
*                format required by speech decoder.
*
**************************************************************************
*/
enum RXFrameType UnpackBits (
    Word8  q,              /* i : Q-bit (i.e. BFI)        */
	Word16 ft,             /* i : frame type (i.e. mode)  */
    UWord8 packed_bits[],  /* i : sorted & packed bits    */
	enum Mode *mode,       /* o : mode information        */
    Word16 bits[]          /* o : serial bits             */
)
{
	Word16 i, sid_type;
	UWord8 *pack_ptr, temp;

	pack_ptr = (UWord8*)packed_bits;

	/* real NO_DATA frame or unspecified frame type */
	if (ft == 15 || (ft > 8 && ft < 15))
	{
		*mode = (enum Mode)-1;
		return RX_NO_DATA;
	}

	temp = *pack_ptr;
	pack_ptr++;

/*
static Word16 unpacked_size[16] = {95, 103, 118, 134, 148, 159, 204, 244,
                                   35,   0,   0,   0,   0,   0,   0,   0};

*/
	for (i = 1; i < unpacked_size[ft] + 1; i++)
	{
		if (temp & 0x80)	bits[sort_ptr[ft][i-1]] = BIT_1; 			 // sort_ptr in bitno.tab
		else				bits[sort_ptr[ft][i-1]] = BIT_0;			 // sort_ptr in bitno.tab

		if (i % 8)
		{
			temp <<= 1;
		}
		else
		{
			temp = *pack_ptr;
			pack_ptr++;
		}
	}

	/* SID frame */
	if (ft == MRDTX)
	{
		if (temp & 0x80)	sid_type = 1;
		else				sid_type = 0;

		*mode = (enum Mode)((temp >> 4) & 0x07);
		*mode = ((*mode & 0x4) >> 2) | (*mode & 0x2) | ((*mode & 0x1) << 2); 

		if (q)
		{
			if (sid_type)	return  RX_SID_UPDATE;
			else			return	RX_SID_FIRST;
		}
		else
		{
			return	RX_SID_BAD;
		}
	}
	/* speech frame */
	else
	{
		*mode = (enum Mode)ft;

		if (q)	return RX_SPEECH_GOOD;
		else	return RX_SPEECH_BAD;    //��֡
	}
}

#endif

