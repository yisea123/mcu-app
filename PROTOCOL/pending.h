#ifndef __PENDING_H
#define __PENDING_H	 

#include "sys.h"
#include "led.h"
#include "key.h"
#include "can.h"
#include "rtc.h"
#include "list.h"
#include "beep.h"
#include "kfifo.h"
#include "delay.h"
#include "usart.h"
#include "ioctr.h"
#include "usmart.h"
#include "malloc.h"
#include "car_event.h"
#include "uart_command.h"
#include "tmodem.h"
#include "iwdg.h"
#include "stmflash.h"
#include "longsung.h"

extern uint16_t cpuGetFlashSize(void);
extern void cpuidGetId(u32 mcuID[]);
extern void handle_pending_work(void);
extern void report_debug_to_android(void);
extern void get_sys_time(uint32_t format, RTC_DateTypeDef* date, RTC_TimeTypeDef* time);
extern char mMcuJumpAppPending, mRequestSoftRestPending;
#endif



