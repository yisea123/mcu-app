#ifndef __TEST_H
#define __TEST_H
#include "timer.h"
#include "led.h"

extern SYSTIMER *timerTest;
extern void usmart_test(void);
extern void test_task_callback(void *timer);
extern void test_put_msg(char cmd, char *text);
extern void printf_systemrccClocks(void);
#endif


