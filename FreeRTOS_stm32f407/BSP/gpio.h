
#ifndef _GPIO_H
#define _GPIO_H
#include "stdint.h"
#include "stm32f4xx_gpio.h"
typedef enum 
{
	GPIO_A,
	GPIO_B,
	GPIO_C,
	GPIO_D,
	GPIO_E,
	GPIO_F,
	GPIO_G,
	GPIO_H,
	GPIO_I,
	GPIO_J,
	GPIO_K,
	GPIO_MAX
}HW_GPIO_INDEX,*pHW_GPIO_INDEX;

typedef enum 
{
	PIN_0,
	PIN_1,
	PIN_2,
	PIN_3,
	PIN_4,
	PIN_5,
	PIN_6,
	PIN_7,
	PIN_8,
	PIN_9,
	PIN_10,
	PIN_11,
	PIN_12,
	PIN_13,
	PIN_14,
	PIN_15,
	PIN_MAX
}HW_PIN_INDEX,*pHW_PIN_INDEX;

typedef enum
{
	ACTIVE_LOW,
	ACTIVE_HIGH
}ACTIVE_STATE,*pACTIVE_STATE;

typedef struct 
{
	HW_GPIO_INDEX Gpio;
	HW_PIN_INDEX Pin;
	ACTIVE_STATE ActiveState;
	GPIOMode_TypeDef GPIO_Mode;
  GPIOSpeed_TypeDef GPIO_Speed;
  GPIOOType_TypeDef GPIO_OType; 
  GPIOPuPd_TypeDef GPIO_PuPd;   
}HW_DIO,*pHW_DIO;

void DIO_Port_Initialize_Pin_State(const HW_DIO* hw_dio_config);
void DIO_Port_Set_Pin_State(const HW_DIO* hw_dio_config, BOOL state);
BOOL DIO_Port_Get_Pin_State(const HW_DIO* hw_dio_config);
void DIO_Port_DeInitialize(const HW_DIO* hw_dio_config);

void DIO_Port_Toggle_Pin_State(const HW_DIO* hw_dio_config);
#endif
