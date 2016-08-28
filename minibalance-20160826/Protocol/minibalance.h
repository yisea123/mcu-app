#ifndef __MINIBALANCE_H
#define __MINIBALANCE_H

#include "stdlib.h"
#include "delay.h"
#include "timer.h"

#define CMD_REMOTE_CONTROL    99
#define CMD_RUNNING_CONTROL		100

extern SYSTIMER *miniTimer;
extern SYSTIMER *lcdTimer;
void lcd_display_task(void *timer);
extern void MiniBalance_PWM_Init(u16 arr,u16 psc);
extern void minibalance_core_task(void *timer);
extern void handle_remote_control(int uart_receive);
#endif
