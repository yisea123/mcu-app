#ifndef __TIMER_H
#define __TIMER_H	 

#include "sys.h"
#include "uart_command.h"
#include "tmodem.h"
#include "longsung.h"
#include "ioctr.h"

extern void Timer2_Init(u16 arr,u16 psc);
extern void Timer3_Init(u16 arr,u16 psc);
extern void Timer5_Init(u16 arr,u16 psc);

#endif
