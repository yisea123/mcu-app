/*
*****************************************************************************
*
*      GSM AMR-NB speech codec   R98   Version 7.6.0   December 12, 2001
*                                R99   Version 3.3.0                
*                                REL-4 Version 4.1.0                
*
*****************************************************************************
*
*      File             : lsp_avg.c
*      Purpose:         : LSP averaging and history
*
*****************************************************************************
*/
 
 
/*
*****************************************************************************
*                         MODULE INCLUDE FILE AND VERSION ID
*****************************************************************************
*/
#include "lsp_avg.h"
const char lsp_avg_id[] = "@(#)$Id $" lsp_avg_h;
 
/*
*****************************************************************************
*                         INCLUDE FILES
*****************************************************************************
*/
#include <stdlib.h>
#include <stdio.h>
#include "basic_op.h"
#include "oper_32b.h"
#include "count.h"
#include "q_plsf_5.tab"
#include "copy.h"

#if(STATIC_MEMORY_ALLOCK == 1)
	lsp_avgState mLsp_avgState;
#endif

/*
*****************************************************************************
*                         LOCAL VARIABLES AND TABLES
*****************************************************************************
*/
/*
*****************************************************************************
*                         PUBLIC PROGRAM CODE
*****************************************************************************
*/
/*
**************************************************************************
*
*  Function    : lsp_avg_init
*  Purpose     : Allocates memory and initializes state variables
*
**************************************************************************
*/
int lsp_avg_init (lsp_avgState **state)
{
  lsp_avgState* s;
 
  if (state == (lsp_avgState **) NULL){
      printf("lsp_avg_init: invalid parameter\n");
      return -1;
  }
  *state = NULL;

#if(STATIC_MEMORY_ALLOCK == 1)
	s = &mLsp_avgState;
#else
  /* allocate memory */
  if ((s = (lsp_avgState *) pvPortMalloc(sizeof(lsp_avgState))) == NULL){
      printf("lsp_avg_init: can not malloc state structure\n");
      return -1;
  }
#endif
  lsp_avg_reset(s);
  *state = s;
  
  return 0;
}
 
/*
**************************************************************************
*
*  Function    : lsp_avg_reset
*  Purpose     : Resets state memory
*
**************************************************************************
*/
int lsp_avg_reset (lsp_avgState *st)
{ 
  if (st == (lsp_avgState *) NULL){
      printf("lsp_avg_reset: invalid parameter\n");
      return -1;
  }

  Copy(mean_lsf, &st->lsp_meanSave[0], M);
  
  return 0;
}
 
/*
**************************************************************************
*
*  Function    : lsp_avg_exit
*  Purpose     : The memory used for state memory is freed
*
**************************************************************************
*/
void lsp_avg_exit (lsp_avgState **state)
{
  if (state == NULL || *state == NULL)
      return;

#if(STATIC_MEMORY_ALLOCK == 1)

#else
  /* deallocate memory */
  vPortFree(*state);
#endif
  *state = NULL;
  
  return;
}

/*
**************************************************************************
*
*  Function    : lsp_avg
*  Purpose     : Calculate the LSP averages
*
**************************************************************************
*/

void lsp_avg (
    lsp_avgState *st,         /* i/o : State struct                 Q15 */
    Word16 *lsp               /* i   : state of the state machine   Q15 */
)
{
    Word16 i;
    Word32 L_tmp;            /* Q31 */

    for (i = 0; i < M; i++) {

       /* mean = 0.84*mean */
       L_tmp = L_deposit_h(st->lsp_meanSave[i]);
       L_tmp = L_msu(L_tmp, EXPCONST, st->lsp_meanSave[i]);

       /* Add 0.16 of newest LSPs to mean */
       L_tmp = L_mac(L_tmp, EXPCONST, lsp[i]);

       /* Save means */
       st->lsp_meanSave[i] = round(L_tmp);          move16();   /* Q15 */
    }

    return;
}
