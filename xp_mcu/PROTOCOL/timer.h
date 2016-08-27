#ifndef __TIMER_H
#define __TIMER_H	 

#include "sys.h"
#include "uart_command.h"
#include "tmodem.h"
#include "longsung.h"
#include "ioctr.h"

#define NUMTIMER				10

#define TIMER2PERIOD		1/*20=20ms 10=10ms 1=1ms*/
#define TIMER2SECOND 		1000

#define REPEAT					1
#define REPEATNOT				0

#define ENABLE_INT()	__set_PRIMASK(0)
#define DISABLE_INT()	__set_PRIMASK(1)

extern void Timer2_Init(u16 arr,u16 psc);
extern void Timer3_Init(u16 arr,u16 psc);
extern void Timer5_Init(u16 arr,u16 psc);

/*unsigned int times ms.*/
typedef void (* timer_callback)(void *argc);

typedef struct Timer2 {
	char name[25];
	char isUse;	
	char repeat;
	signed char mReady;/*set mReady to -1, will delete timer!*/
	unsigned int times;
  unsigned int timesSave;
	timer_callback callback;
	void *argc;
	struct Timer2 *next;
}TIMER2;

extern void handle_timer_callback(void);
extern TIMER2* register_timer2(const char name[], unsigned int times, 
		timer_callback callback, char repeat, void *argc);
extern int unregister_timer2(TIMER2* timer);

extern void poll_system_timer(void);
extern void init_system_timer(void);
static void clean_system_timer(TIMER2 *timer);
#endif
