#ifndef __TIMER_H
#define __TIMER_H	 

#include <stm32f10x.h>
#include <string.h>
#include "xlist.h"
//#include "message.h"

//#define MINIBALANCE			1		

#define NUMTIMER				15

#define TIMERPERIOD		1
#define TIMERSECOND 		1000

#define REPEAT					1
#define REPEATNOT				0

#define ENABLE_INT()	__set_PRIMASK(0)
#define DISABLE_INT()	__set_PRIMASK(1)

#endif
