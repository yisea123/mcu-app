#ifndef __MINIBALANCE_H
#define __MINIBALANCE_H

#include "stdlib.h"
#include "delay.h"

extern void poll_led_display(void *timer);
extern void MiniBalance_PWM_Init(u16 arr,u16 psc);
extern void poll_minibalance_core(void *timer);
extern void handle_remote_control(int uart_receive);
#endif
