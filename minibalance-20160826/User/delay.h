#ifndef __DELAY_H
#define __DELAY_H 	

#include <stm32f10x.h>

extern void delay_init(uint8_t SYSCLK);
extern void delay_ms(uint16_t nms);
extern void delay_us(uint32_t nus);
extern void interrupt_priority_init(uint32_t priority);
#endif 


