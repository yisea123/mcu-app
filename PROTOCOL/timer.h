#ifndef __TIMER_H
#define __TIMER_H	 

#include "sys.h"
#include "uart_command.h"
#include "tmodem.h"
#include "longsung.h"
#include "ioctr.h"

#define TIMER2PERIOD		20/*20ms*/
#define TIMER2SECOND 		50*TIMER2PERIOD

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
	char repeat;
	signed char mReady;
	int times;
  int timesSave;
	timer_callback callback;
	void *argc;
	struct Timer2 *next;
}TIMER2;

TIMER2* register_timer2(const char name[], unsigned int times, timer_callback callback, char repeat, void *argc);
int unregister_timer2(TIMER2* timer);
void handle_timer_callback(void);
#endif
