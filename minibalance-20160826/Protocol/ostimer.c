/*时间轮算法定时器*/

#include "ostimer.h"

extern 	unsigned int sysTimerTick;

static  OS_TMR  *alloc_os_timer(void);
static  void  free_os_timer(OS_TMR *ptmr);
static  void  os_timer_unlink(OS_TMR *ptmr);
static  void  os_timer_link(OS_TMR *ptmr, unsigned char type);


static OS_TMR            OSTmrTbl[OS_TMR_CFG_MAX];
static OS_TMR           *OSTmrFreeList;
static OS_TMR_WHEEL      OSTmrWheelTbl[OS_TMR_CFG_WHEEL_SIZE];

static  unsigned short  OSTmrFree;
static  unsigned short  OSTmrUsed;
static  unsigned int  OSTmrTime;

OS_TMR  *create_os_timer(unsigned int dly,
                      unsigned int period,
                      unsigned char opt,
                      OS_TMR_CALLBACK  callback,
                      void *callback_arg,
                      unsigned char *pname,
                      unsigned char *perr)
{
    OS_TMR   *ptmr;

    switch (opt) {
        case OS_TMR_OPT_PERIODIC:
             if (period == 0u) {
                 *perr = OS_ERR_TMR_INVALID_PERIOD;
                 return ((OS_TMR *)0);
             }
             break;

        case OS_TMR_OPT_ONE_SHOT:
             if (dly == 0u) {
                 *perr = OS_ERR_TMR_INVALID_DLY;
                 return ((OS_TMR *)0);
             }
             break;

        default:
             *perr = OS_ERR_TMR_INVALID_OPT;
             return ((OS_TMR *)0);
    }

    ptmr = alloc_os_timer();
    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_NON_AVAIL;
        return ((OS_TMR *)0);
    }
    ptmr->OSTmrState       = OS_TMR_STATE_STOPPED;
    ptmr->OSTmrDly         = dly;
    ptmr->OSTmrPeriod      = period;
    ptmr->OSTmrOpt         = opt;
    ptmr->OSTmrCallback    = callback;
    ptmr->OSTmrCallbackArg = callback_arg;
    ptmr->OSTmrName        = pname;
    *perr = OS_ERR_NONE;
    return (ptmr);
}

boolean  del_os_timer(OS_TMR  *ptmr,
                   unsigned char   *perr)
{
    if (ptmr->OSTmrType != OS_TMR_TYPE) {
        *perr = OS_ERR_TMR_INVALID_TYPE;
        return (OS_FALSE);
    }

    switch (ptmr->OSTmrState) {
        case OS_TMR_STATE_RUNNING:
             os_timer_unlink(ptmr);
             free_os_timer(ptmr);
             *perr = OS_ERR_NONE;
             return (OS_TRUE);

        case OS_TMR_STATE_STOPPED:
        case OS_TMR_STATE_COMPLETED:
             free_os_timer(ptmr);
             *perr = OS_ERR_NONE;
             return (OS_TRUE);

        case OS_TMR_STATE_UNUSED:
             *perr = OS_ERR_TMR_INACTIVE;
             return (OS_FALSE);

        default:
             *perr = OS_ERR_TMR_INVALID_STATE;
             return (OS_FALSE);
    }
}

static OS_TMR* alloc_os_timer(void)
{
    OS_TMR *ptmr;


    if (OSTmrFreeList == (OS_TMR *)0) {
        return ((OS_TMR *)0);
    }
    ptmr            = (OS_TMR *)OSTmrFreeList;
    OSTmrFreeList   = (OS_TMR *)ptmr->OSTmrNext;
    ptmr->OSTmrNext = (OS_TMR *)0;
    ptmr->OSTmrPrev = (OS_TMR *)0;
    OSTmrUsed++;
    OSTmrFree--;
    return (ptmr);
}

static  void  free_os_timer(OS_TMR *ptmr)
{
    ptmr->OSTmrState       = OS_TMR_STATE_UNUSED;
    ptmr->OSTmrOpt         = OS_TMR_OPT_NONE;
    ptmr->OSTmrPeriod      = 0u;
    ptmr->OSTmrMatch       = 0u;
    ptmr->OSTmrCallback    = (OS_TMR_CALLBACK)0;
    ptmr->OSTmrCallbackArg = (void *)0;
    ptmr->OSTmrName        = (unsigned char *)(void *)"?";

    ptmr->OSTmrPrev        = (OS_TMR *)0;
    ptmr->OSTmrNext        = OSTmrFreeList;
    OSTmrFreeList          = ptmr;

    OSTmrUsed--;
    OSTmrFree++;
}

static  void  os_timer_unlink (OS_TMR *ptmr)
{
    OS_TMR        *ptmr1;
    OS_TMR        *ptmr2;
    OS_TMR_WHEEL  *pspoke;
    unsigned short spoke;

    spoke  = (unsigned short)(ptmr->OSTmrMatch % OS_TMR_CFG_WHEEL_SIZE);
    pspoke = &OSTmrWheelTbl[spoke];

    if (pspoke->OSTmrFirst == ptmr) {
        ptmr1              = (OS_TMR *)ptmr->OSTmrNext;
        pspoke->OSTmrFirst = (OS_TMR *)ptmr1;
        if (ptmr1 != (OS_TMR *)0) {
            ptmr1->OSTmrPrev = (void *)0;
        }
    } else {
        ptmr1            = (OS_TMR *)ptmr->OSTmrPrev;
        ptmr2            = (OS_TMR *)ptmr->OSTmrNext;
        ptmr1->OSTmrNext = ptmr2;
        if (ptmr2 != (OS_TMR *)0) {
            ptmr2->OSTmrPrev = (void *)ptmr1;
        }
    }
    ptmr->OSTmrState = OS_TMR_STATE_STOPPED;
    ptmr->OSTmrNext  = (void *)0;
    ptmr->OSTmrPrev  = (void *)0;
    pspoke->OSTmrEntries--;
}

static void os_timer_link(OS_TMR  *ptmr,
                          unsigned char    type)
{
    OS_TMR       *ptmr1;
    OS_TMR_WHEEL *pspoke;
    unsigned short spoke;

    ptmr->OSTmrState = OS_TMR_STATE_RUNNING;
    if (type == OS_TMR_LINK_PERIODIC) {
        ptmr->OSTmrMatch = ptmr->OSTmrPeriod + OSTmrTime;
    } else {
        if (ptmr->OSTmrDly == 0u) {
            ptmr->OSTmrMatch = ptmr->OSTmrPeriod + OSTmrTime;
        } else {
            ptmr->OSTmrMatch = ptmr->OSTmrDly    + OSTmrTime;
        }
    }
    spoke  = (unsigned short)(ptmr->OSTmrMatch % OS_TMR_CFG_WHEEL_SIZE);
    pspoke = &OSTmrWheelTbl[spoke];

    if (pspoke->OSTmrFirst == (OS_TMR *)0) {                       
		/* Link into timer wheel */
        pspoke->OSTmrFirst   = ptmr;
        ptmr->OSTmrNext      = (OS_TMR *)0;
        pspoke->OSTmrEntries = 1u;
    } else {
        ptmr1                = pspoke->OSTmrFirst;                 
		/* Point to first timer in the spoke */
        pspoke->OSTmrFirst   = ptmr;
        ptmr->OSTmrNext      = (void *)ptmr1;
        ptmr1->OSTmrPrev     = (void *)ptmr;
        pspoke->OSTmrEntries++;
    }
    ptmr->OSTmrPrev = (void *)0;                                   
	/* Timer always inserted as first node in list */
}

boolean  start_os_timer(OS_TMR   *ptmr,
                     unsigned char    *perr)
{
    if (perr == (unsigned char *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }

    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }

    if (ptmr->OSTmrType != OS_TMR_TYPE) { 
        *perr = OS_ERR_TMR_INVALID_TYPE;
        return (OS_FALSE);
    }

    switch (ptmr->OSTmrState) {
        case OS_TMR_STATE_RUNNING: 
             os_timer_unlink(ptmr); 
             os_timer_link(ptmr, OS_TMR_LINK_DLY); 
             *perr = OS_ERR_NONE;
             return (OS_TRUE);

        case OS_TMR_STATE_STOPPED: 
        case OS_TMR_STATE_COMPLETED:
             os_timer_link(ptmr, OS_TMR_LINK_DLY); 
             *perr = OS_ERR_NONE;
             return (OS_TRUE);

        case OS_TMR_STATE_UNUSED: 
             *perr = OS_ERR_TMR_INACTIVE;
             return (OS_FALSE);

        default:
             *perr = OS_ERR_TMR_INVALID_STATE;
             return (OS_FALSE);
    }
}

boolean  stop_os_timer(OS_TMR  *ptmr,
                    unsigned char    opt,
                    void    *callback_arg,
                    unsigned char   *perr)
{
    OS_TMR_CALLBACK  pfnct;

    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (OS_FALSE);
    }
    if (ptmr->OSTmrType != OS_TMR_TYPE) { 
        *perr = OS_ERR_TMR_INVALID_TYPE;
        return (OS_FALSE);
    }
    switch (ptmr->OSTmrState) {
        case OS_TMR_STATE_RUNNING:
             os_timer_unlink(ptmr); 
             *perr = OS_ERR_NONE;
             switch (opt) {
                 case OS_TMR_OPT_CALLBACK:
                      pfnct = ptmr->OSTmrCallback; 
                      if (pfnct != (OS_TMR_CALLBACK)0) {
                          (*pfnct)((void *)ptmr, ptmr->OSTmrCallbackArg); 
                      } else {
                          *perr = OS_ERR_TMR_NO_CALLBACK;
                      }
                      break;

                 case OS_TMR_OPT_CALLBACK_ARG:
                      pfnct = ptmr->OSTmrCallback; 
                      if (pfnct != (OS_TMR_CALLBACK)0) {
                          (*pfnct)((void *)ptmr, callback_arg); 
                      } else {
                          *perr = OS_ERR_TMR_NO_CALLBACK;
                      }
                      break;

                 case OS_TMR_OPT_NONE:
                      break;

                 default:
                     *perr = OS_ERR_TMR_INVALID_OPT;
                     break;
             }
             return (OS_TRUE);

        case OS_TMR_STATE_COMPLETED: 
        case OS_TMR_STATE_STOPPED: 
             *perr = OS_ERR_TMR_STOPPED;
             return (OS_TRUE);

        case OS_TMR_STATE_UNUSED: 
             *perr = OS_ERR_TMR_INACTIVE;
             return (OS_FALSE);

        default:
             *perr = OS_ERR_TMR_INVALID_STATE;
             return (OS_FALSE);
    }
}

unsigned int get_os_timer_remain(OS_TMR *ptmr, unsigned char *perr)
{
    unsigned int  remain;

    if (perr == (unsigned char *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (0u);
    }

    if (ptmr == (OS_TMR *)0) {
        *perr = OS_ERR_TMR_INVALID;
        return (0u);
    }
    if (ptmr->OSTmrType != OS_TMR_TYPE) { 
        *perr = OS_ERR_TMR_INVALID_TYPE;
        return (0u);
    }
    switch (ptmr->OSTmrState) {
        case OS_TMR_STATE_RUNNING:
             remain = ptmr->OSTmrMatch - OSTmrTime; 
             *perr  = OS_ERR_NONE;
             return (remain);

        case OS_TMR_STATE_STOPPED: 
             switch (ptmr->OSTmrOpt) {
                 case OS_TMR_OPT_PERIODIC:
                      if (ptmr->OSTmrDly == 0u) {
                          remain = ptmr->OSTmrPeriod;
                      } else {
                          remain = ptmr->OSTmrDly;
                      }
                      *perr  = OS_ERR_NONE;
                      break;

                 case OS_TMR_OPT_ONE_SHOT:
                 default:
                      remain = ptmr->OSTmrDly;
                      *perr  = OS_ERR_NONE;
                      break;
             }
             return (remain);

        case OS_TMR_STATE_COMPLETED: 
             *perr = OS_ERR_NONE;
             return (0u);

        case OS_TMR_STATE_UNUSED:
             *perr = OS_ERR_TMR_INACTIVE;
             return (0u);

        default:
             *perr = OS_ERR_TMR_INVALID_STATE;
             return (0u);
    }
}

void init_os_timer(void)
{
    unsigned char err;
    unsigned short   ix;
    unsigned short   ix_next;
    OS_TMR  *ptmr1;
    OS_TMR  *ptmr2;

    memset((unsigned char *)&OSTmrTbl[0], '\0', sizeof(OSTmrTbl));
    memset((unsigned char *)&OSTmrWheelTbl[0], '\0', sizeof(OSTmrWheelTbl));

    for (ix = 0u; ix < (OS_TMR_CFG_MAX - 1u); ix++) {
        ix_next = ix + 1u;
        ptmr1 = &OSTmrTbl[ix];
        ptmr2 = &OSTmrTbl[ix_next];
        ptmr1->OSTmrType    = OS_TMR_TYPE;
        ptmr1->OSTmrState   = OS_TMR_STATE_UNUSED;
        ptmr1->OSTmrNext    = (void *)ptmr2;

        ptmr1->OSTmrName    = (unsigned char *)(void *)"?";

    }
    ptmr1               = &OSTmrTbl[ix];
    ptmr1->OSTmrType    = OS_TMR_TYPE;
    ptmr1->OSTmrState   = OS_TMR_STATE_UNUSED;
    ptmr1->OSTmrNext    = (void *)0;

    ptmr1->OSTmrName    = (unsigned char *)(void *)"?";

    OSTmrTime           = 0u;
    OSTmrUsed           = 0u;
    OSTmrFree           = OS_TMR_CFG_MAX;
    OSTmrFreeList       = &OSTmrTbl[0];
}

void poll_os_timer(void)
{			
	unsigned char    err;
	OS_TMR          *ptmr;
	OS_TMR          *ptmr_next;
	OS_TMR_CALLBACK  pfnct;
	OS_TMR_WHEEL    *pspoke;
	unsigned short   spoke;
	static unsigned int systimes = 0;
	unsigned int        temp = sysTimerTick;
	unsigned int  count = temp - systimes;
	systimes = temp;
	
	while (count--> 0)	{
		OSTmrTime++;
		spoke  = (unsigned short)(OSTmrTime % OS_TMR_CFG_WHEEL_SIZE);
		pspoke = &OSTmrWheelTbl[spoke];
		ptmr   = pspoke->OSTmrFirst;
		while (ptmr != (OS_TMR *)0) {
				ptmr_next = (OS_TMR *)ptmr->OSTmrNext;
														
				if (OSTmrTime == ptmr->OSTmrMatch) {
						os_timer_unlink(ptmr);
						if (ptmr->OSTmrOpt == OS_TMR_OPT_PERIODIC) {
								os_timer_link(ptmr, OS_TMR_LINK_PERIODIC);
						} else {
								ptmr->OSTmrState = OS_TMR_STATE_COMPLETED;
						}
						pfnct = ptmr->OSTmrCallback;
						if (pfnct != (OS_TMR_CALLBACK)0) {
								(*pfnct)((void *)ptmr, ptmr->OSTmrCallbackArg);
						}
				}
				ptmr = ptmr_next;
		}
	}
}

