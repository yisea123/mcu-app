#ifndef __TIMER_H
#define __TIMER_H	 

#include <string.h>
#include "Protocol.h"

#define NUMTIMER				15

#define TIMERPERIOD		1
#define TIMERSECOND 		1000

#define REPEAT					1
#define REPEATNOT				0

#define ENABLE_INT()	__set_PRIMASK(0)
#define DISABLE_INT()	__set_PRIMASK(1)

/* 
* void* timer is TIMER2* timer! so we can get all info about TIMER2
* in timer_callback we are not allowed to call 
* register_system_timer and unregister_system_timer function!
*/
typedef void (* timer_callback)(void *timer);

typedef struct sysTimer {
	char name[25];
	char isUse;	
	char repeat;
	signed char destory;
	/*set destory to -1, will delete timer!*/
	unsigned int times;
  unsigned int timesSave;
	timer_callback callback;
	void *argc;
	struct sysTimer *next;
} SYSTIMER;

extern SYSTIMER* register_system_timer(const char name[], unsigned int times, 
		timer_callback callback, char repeat, void *argc);
extern int unregister_system_timer(SYSTIMER* timer);

extern void poll_system_timer(void);
extern void init_system_timer(void);
extern void timer1_int_init(u16 arr,u16 psc);
extern void timer2_int_init(u16 arr,u16 psc);
extern void timer4_int_init(u16 arr,u16 psc);
extern void timer3_int_init(u16 arr,u16 psc);
#endif
