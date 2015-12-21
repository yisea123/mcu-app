#include "ioctr.h"
#include "delay.h" 
#include "sys.h"

char mAndroidPower = 0;
char mAndroidRunning = 0;

/**************************************************
init all io which control t8 power
****************************************************/
void Init_io_ctrl(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure, GPIO_InitStructure1;
	RCC_AHB1PeriphClockCmd(RCC_ALL_PORT, ENABLE);
	
	GPIO_InitStructure1.GPIO_Pin =UP2PS_ENABLE|MAIN_PWR_EN;
  GPIO_InitStructure1.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure1.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure1.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure1.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
  GPIO_Init(GPIOC, &GPIO_InitStructure1);//初始化
	
	bsp_io_low('C', UP2PS_ENABLE);
	bsp_io_low('C', MAIN_PWR_EN);
	
  GPIO_InitStructure.GPIO_Pin =UP2USB_PWR|UP2PS_USB_SYNC|UP2PS_8V_SW_EN|UP2PS_5V_SW_EN|UP2PS_3V3_SW_EN|UP2PS_SYNC|UP2TUNER_ANT_ON_OFF;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
  GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化
	
	bsp_io_low('D', UP2USB_PWR);
	bsp_io_low('D', UP2PS_USB_SYNC);
	bsp_io_low('D', UP2PS_8V_SW_EN);
	bsp_io_low('D', UP2PS_5V_SW_EN);
	bsp_io_low('D', UP2PS_3V3_SW_EN);
	bsp_io_low('D', UP2PS_SYNC);
	bsp_io_low('D', UP2TUNER_ANT_ON_OFF);
	
}

/***************************************************
power on or power off t8
****************************************************/

void power_android(uint32_t on)
{
	//printf("%s->%s\r\n", __func__, on!=0?"on":"off");
	printf("%s: on=%d\r\n", __func__, on);
	mAndroidPower = on;
	if(on) //power on android
	{
		bsp_io_heigh('D', UP2USB_PWR);
		bsp_io_heigh('D', UP2PS_USB_SYNC);
		bsp_io_heigh('D', UP2PS_8V_SW_EN);
		bsp_io_heigh('D', UP2PS_5V_SW_EN);
		bsp_io_heigh('D', UP2PS_3V3_SW_EN);	

		bsp_io_heigh('C', UP2PS_ENABLE);	
	}
	else   //power off android
	{
		bsp_io_low('C', UP2PS_ENABLE);	
		
		bsp_io_low('D', UP2USB_PWR);
		bsp_io_low('D', UP2PS_USB_SYNC);
		bsp_io_low('D', UP2PS_8V_SW_EN);
		bsp_io_low('D', UP2PS_5V_SW_EN);
		bsp_io_low('D', UP2PS_3V3_SW_EN);	
	}
}

void bsp_io_low(char port, uint16_t num)
{
	
	switch(port)
	{
		case 'C':
			UP2PS_ENABLE_BASE->BSRRL = num;
			break;
		
		case 'D':
			UP2USB_PWR_BASE->BSRRL = num;
			break;
		
		default:
			break;
	}
}

void bsp_io_heigh(char port, uint16_t num)
{
	
		switch(port)
	{
		case 'C':
			UP2PS_ENABLE_BASE->BSRRH = num;
			break;
		
		case 'D':
			UP2USB_PWR_BASE->BSRRH = num;
			break;
		
		default:
			break;
	}
}
















