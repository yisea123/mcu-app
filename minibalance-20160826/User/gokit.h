#ifndef _GOKIT_H
#define _GOKIT_H

#include "stm32f10x.h"
#include "Hal_key/Hal_key.h"
#include "Hal_led/Hal_led.h"
#include "Hal_motor/Hal_motor.h"
#include "Hal_rgb_led/Hal_rgb_led.h"
#include "Hal_temp_hum/Hal_temp_hum.h"
#include "Hal_infrared/Hal_infrared.h"
#include "Hal_Usart/hal_uart.h"
#include "ringbuffer.h"
#include "timer.h"
#include "rfifo.h"
#include "keydetect.h"
#include "uart.h"
#include "wdg.h"
#include "usmart.h"
#include "pwm.h"
#include "rtc.h"
#include "Protocol.h"
#include <string.h>
#include "MPU6050.h"
#include "IOI2C.h"
#include "madc.h"
#include "encoder.h"
#include "oled.h"
#include "malloc.h"
#include "sys.h"
//#include "oled.h"
#include "minibalance.h"

__packed	typedef struct	
{
	uint8_t				LED_Cmd;
	uint8_t				LED_R;
	uint8_t				LED_G;  
	uint8_t				LED_B;	
	MOTOR_T				Motor;
	uint8_t       		Infrared;
	uint8_t       		Temperature;
	uint8_t       		Humidity;
	uint8_t				Alert;
	uint8_t				Fault;
}ReadTypeDef_t; 

__packed typedef struct	
{
	uint8_t							Attr_Flags;
	uint8_t							LED_Cmd;
	uint8_t							LED_R;
	uint8_t							LED_G;  
	uint8_t							LED_B;
	MOTOR_T							Motor;		
	
}WirteTypeDef_t;

void HW_Init(void);
void Printf_SystemRccClocks(void);
void GizWits_GatherSensorData(void);
void GizWits_ControlDeviceHandle(void);
void SW_Init(void);
void KEY_Handle(void);
//void GizWits_WiFiStatueHandle(uint16_t wifiStatue);

#endif
