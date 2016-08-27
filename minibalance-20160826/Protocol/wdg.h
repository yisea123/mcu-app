#ifndef __WDG_H
#define __WDG_H
#include "stm32f10x_iwdg.h"

void watchdog_init(int second);
void watchdog_feed(void);

#endif
