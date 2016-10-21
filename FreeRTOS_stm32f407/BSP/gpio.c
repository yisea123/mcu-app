#include "sys.h"
#include "gpio.h"
#include "stdint.h"
void DIO_Port_Initialize_Pin_State(const HW_DIO* hw_dio_config)
{
	GPIO_InitTypeDef	GPIO_InitStructure; 
	GPIO_TypeDef * GpioType = (GPIO_TypeDef *)(GPIOA_BASE + (hw_dio_config->Gpio * 0x400ul));
	//初始化GPIO
	
	GPIO_InitStructure.GPIO_Pin = (0x0001ul << hw_dio_config->Pin) ;
	GPIO_InitStructure.GPIO_Mode = hw_dio_config->GPIO_Mode;//功能选择
	GPIO_InitStructure.GPIO_OType = hw_dio_config->GPIO_OType;//输出方式
	GPIO_InitStructure.GPIO_Speed = hw_dio_config->GPIO_Speed;//速度
	GPIO_InitStructure.GPIO_PuPd = hw_dio_config->GPIO_PuPd;//上下拉设置
	GPIO_Init(GpioType, &GPIO_InitStructure);//初始化
}

void DIO_Port_DeInitialize(const HW_DIO* hw_dio_config)
{
	GPIO_InitTypeDef	GPIO_InitStructure; 
	GPIO_TypeDef * GpioType = (GPIO_TypeDef *)(GPIOA_BASE + (hw_dio_config->Gpio * 0x400ul));
	//初始化GPIO
	GPIO_InitStructure.GPIO_Pin = (0x0001ul << hw_dio_config->Pin) ;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;//功能选择
	GPIO_InitStructure.GPIO_OType = GPIO_OType_OD;//输出方式
	GPIO_InitStructure.GPIO_Speed = GPIO_Low_Speed;//速度
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上下拉设置
	GPIO_Init(GpioType, &GPIO_InitStructure);//初始化
}


void DIO_Port_Set_Pin_State(const HW_DIO* hw_dio_config, BOOL state)
{
	GPIO_TypeDef * GpioType = (GPIO_TypeDef *)(GPIOA_BASE + (hw_dio_config->Gpio * 0x400ul));
	if(hw_dio_config->ActiveState == ACTIVE_LOW)//低有效，需要状态变换
	{
		if(state == Bit_RESET)state = Bit_SET;
		else state = Bit_RESET;
	}		
	GPIO_WriteBit(GpioType, (0X0001<<hw_dio_config->Pin), (BitAction)state);
}


BOOL DIO_Port_Get_Pin_State(const HW_DIO* hw_dio_config)
{
	GPIO_TypeDef * GpioType = (GPIO_TypeDef *)(GPIOA_BASE + (hw_dio_config->Gpio * 0x400ul));
	uint8_t state = GPIO_ReadInputDataBit(GpioType, (uint16_t)(1<<(hw_dio_config->Pin)));
	if(hw_dio_config->ActiveState == ACTIVE_LOW)//低有效，需要状态变换
	{
		if(state == Bit_RESET)state = Bit_SET;
		else state = Bit_RESET;
	}		
	return state;
}

void DIO_Port_Toggle_Pin_State(const HW_DIO* hw_dio_config)
{
	GPIO_TypeDef * GpioType = (GPIO_TypeDef *)(GPIOA_BASE + (hw_dio_config->Gpio * 0x400ul));
	GPIO_ToggleBits(GpioType, hw_dio_config->Pin);
}
