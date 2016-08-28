#ifndef __WDG_H
#define __WDG_H
#include "stm32f10x_iwdg.h"
#include "timer.h"

extern SYSTIMER *wdTimer;
extern void feed_watchdog_task(void *timer);
extern void watchdog_init(int second);
extern void watchdog_feed(void);
#endif


