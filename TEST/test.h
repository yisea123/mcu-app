#ifndef __TEST_H
#define __TEST_H	 

#include "can.h"
#include "usart.h"
#include "kfifo.h"
#include "led.h"
#include "tmodem.h"
#include "rtc.h"
#include "iwdg.h"
#include "pending.h"
#include "ioctr.h"

extern int send_can_mesg(void);
extern int send_can_mesg1(uint32_t id);
//extern void report_mesg_to_t8(void);
extern void test_func(void);

#endif

