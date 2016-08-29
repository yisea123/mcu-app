#ifndef __OSTIMER_H
#define __OSTIMER_H	 
#include <string.h>

typedef unsigned char  boolean; 

#define  OS_TMR_TYPE                  100u

#define  OS_FALSE                       0u
#define  OS_TRUE                        1u


#define OS_TMR_EN                 1u   /* Enable (1) or Disable (0) code generation for TIMERS         */
#define OS_TMR_CFG_MAX           16u   /*     Maximum number of timers                                 */
#define OS_TMR_CFG_NAME_EN        1u   /*     Determine timer names                                    */
#define OS_TMR_CFG_WHEEL_SIZE     8u   /*     Size of timer wheel (#Spokes)                            */
#define OS_TMR_CFG_TICKS_PER_SEC 100u   /*     Rate at which timer management task runs (Hz)            */

/*
*********************************************************************************************************
*                            TIMER OPTIONS (see OSTmrStart() and OSTmrStop())
*********************************************************************************************************
*/
#define  OS_TMR_OPT_NONE                0u  /* No option selected                                      */

#define  OS_TMR_OPT_ONE_SHOT            1u  /* Timer will not automatically restart when it expires    */
#define  OS_TMR_OPT_PERIODIC            2u  /* Timer will     automatically restart when it expires    */

#define  OS_TMR_OPT_CALLBACK            3u  /* OSTmrStop() option to call 'callback' w/ timer arg.     */
#define  OS_TMR_OPT_CALLBACK_ARG        4u  /* OSTmrStop() option to call 'callback' w/ new   arg.     */

/*
*********************************************************************************************************
*                                            TIMER STATES
*********************************************************************************************************
*/
#define  OS_TMR_STATE_UNUSED            0u
#define  OS_TMR_STATE_STOPPED           1u
#define  OS_TMR_STATE_COMPLETED         2u
#define  OS_TMR_STATE_RUNNING           3u

/*
*********************************************************************************************************
*                                             ERROR CODES
*********************************************************************************************************
*/
#define OS_ERR_NONE                     0u
#define OS_ERR_TMR_INVALID_DLY        130u
#define OS_ERR_TMR_INVALID_PERIOD     131u
#define OS_ERR_TMR_INVALID_OPT        132u
#define OS_ERR_TMR_INVALID_NAME       133u
#define OS_ERR_TMR_NON_AVAIL          134u
#define OS_ERR_TMR_INACTIVE           135u
#define OS_ERR_TMR_INVALID_DEST       136u
#define OS_ERR_TMR_INVALID_TYPE       137u
#define OS_ERR_TMR_INVALID            138u
#define OS_ERR_TMR_ISR                139u
#define OS_ERR_TMR_NAME_TOO_LONG      140u
#define OS_ERR_TMR_INVALID_STATE      141u
#define OS_ERR_TMR_STOPPED            142u
#define OS_ERR_TMR_NO_CALLBACK        143u

#define  OS_TMR_LINK_DLY       0u
#define  OS_TMR_LINK_PERIODIC  1u

typedef  void (*OS_TMR_CALLBACK)(void *ptmr, void *parg);

typedef  struct  os_tmr {
    unsigned char    OSTmrType;                       /* Should be set to OS_TMR_TYPE                                  */
    OS_TMR_CALLBACK  OSTmrCallback;                   /* Function to call when timer expires                           */
    void            *OSTmrCallbackArg;                /* Argument to pass to function when timer expires               */
    void            *OSTmrNext;                       /* Double link list pointers                                     */
    void            *OSTmrPrev;
    unsigned int     OSTmrMatch;                      /* Timer expires when OSTmrTime == OSTmrMatch                    */
    unsigned int     OSTmrDly;                        /* Delay time before periodic update starts                      */
    unsigned int     OSTmrPeriod;                     /* Period to repeat timer                                        */
    unsigned char   *OSTmrName;                       /* Name to give the timer                                        */
    unsigned char    OSTmrOpt;                        /* Options (see OS_TMR_OPT_xxx)                                  */
    unsigned char    OSTmrState;                      /* Indicates the state of the timer:                             */
                                                      /*     OS_TMR_STATE_UNUSED                                       */
                                                      /*     OS_TMR_STATE_RUNNING                                      */
                                                      /*     OS_TMR_STATE_STOPPED                                      */
} OS_TMR;

typedef  struct  os_tmr_wheel {
    OS_TMR          *OSTmrFirst;                      /* Pointer to first timer in linked list                         */
    unsigned short   OSTmrEntries;
} OS_TMR_WHEEL;


extern void init_os_timer(void);
extern void poll_os_timer(void);

extern OS_TMR  *create_os_timer(unsigned int dly,
                      unsigned int period,
                      unsigned char opt,
                      OS_TMR_CALLBACK  callback,
                      void *callback_arg,
                      unsigned char *pname,
                      unsigned char *perr);
extern boolean  del_os_timer(OS_TMR  *ptmr,
                   unsigned char   *perr);

extern boolean  start_os_timer(OS_TMR   *ptmr,
                     unsigned char    *perr);
extern boolean  stop_os_timer(OS_TMR  *ptmr,
                    unsigned char    opt,
                    void    *callback_arg,
                    unsigned char   *perr);
extern unsigned int get_os_timer_remain(OS_TMR *ptmr, unsigned char *perr);
										
#endif



