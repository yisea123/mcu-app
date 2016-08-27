#ifndef __KEYDETECT_H
#define __KEYDETECT_H	 

#include <string.h>
#include "Protocol.h"
#include <stdio.h>
#include <stm32f10x.h>

#define KEY(port, pin) 					GPIO_ReadInputDataBit(port, pin)

typedef enum
{
    KEY_SINGLE 				= 1,
    KEY_DOUBLE				= 2,
    KEY_LONG_PRESS		= 3,
} KEY_VALUE;

typedef void (* keyevent_callback)(KEY_VALUE value);

typedef struct KeyEvent {
	char isUse;	
	GPIO_TypeDef* port;
	uint16_t pin;
	keyevent_callback callback;
	
	u8 flag_key;
	u8 count_key;
	u8 double_key;	
	u16 count_single;
	u16 Forever_count;	
	
	u16 long_press_count;
	u16 long_press;	
}KEYEVENT;

extern void key_scan(void);
void register_key_event(GPIO_TypeDef* port, uint16_t pin, keyevent_callback callback);
#endif
