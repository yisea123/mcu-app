#include "ioctr.h"
#include "sys.h"
#include "car_event.h"
#include "timer.h"

extern char mAndroidShutDownPending;
extern char mAndroidPowerUpPending;
extern char mDebugUartPrintfEnable;

char mAndroidPower = 0;
//此变量用于标示android电源是否供给

char mAndroidRunning = 0;
//此变量用于标示android系统是否跑起来了

/**************************************************
init all io which control t8 power
****************************************************/
void Init_io_ctrl(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure, GPIO_InitStructure1;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);//使能GPIOC时钟
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);//使能GPIOD时钟
	
	GPIO_InitStructure1.GPIO_Pin =GPIO_Pin_9|GPIO_Pin_12;
  GPIO_InitStructure1.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure1.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure1.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure1.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
  GPIO_Init(GPIOC, &GPIO_InitStructure1);//初始化
	
	UP2PS_ENABLE = 0;
	MAIN_PWR_EN = 0;
	
  GPIO_InitStructure.GPIO_Pin =GPIO_Pin_1|GPIO_Pin_4|GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_10|GPIO_Pin_11;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;//普通输出模式
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;//上拉
  GPIO_Init(GPIOD, &GPIO_InitStructure);//初始化
	
	UP2USB_PWR = 0;
	UP2PS_USB_SYNC = 0;
	UP2PS_8V_SW_EN = 0;
	UP2PS_5V_SW_EN = 0;
	UP2PS_3V3_SW_EN = 0;
	UP2PS_SYNC = 0;
	UP2TUNER_ANT_ON_OFF = 0;
}

/***************************************************
power on or power off t8
on=1 power on
on=0 power off
use power_android(1) in interrupt or recv CAN msg
****************************************************/

void power_android(uint32_t on)
{
	//printf("%s: on=%d\r\n", __func__, on);
	mAndroidPower = on;
	
	if(on) //power on android
	{
		UP2USB_PWR = 1;
		UP2PS_USB_SYNC = 1;
		UP2PS_8V_SW_EN = 1;
		UP2PS_5V_SW_EN = 1;
		UP2PS_3V3_SW_EN = 1;	
		UP2TUNER_ANT_ON_OFF = 1;
		
		UP2PS_ENABLE = 1;	
	}
	else   //power off android
	{
		UP2PS_ENABLE = 0;	
		
		UP2USB_PWR = 0;
		UP2PS_USB_SYNC = 0;
		UP2PS_8V_SW_EN = 0;
		UP2PS_5V_SW_EN = 0;
		UP2PS_3V3_SW_EN = 0;	
		UP2TUNER_ANT_ON_OFF = 0;	
		
		mAndroidRunning = 0;//power off 	
		
		/*Android关机之后，log从调试串口中输出*/
		mDebugUartPrintfEnable = 1;
	}
}

void handle_powerup_android_request(void)
{
	if(mAndroidPowerUpPending) 
	{
		mAndroidPowerUpPending = 0;
		power_android(1);
	}	
}

void handle_shutdown_android_request(void)
{
	char cmd[7];
	unsigned short checkValue;
	
	if(mAndroidShutDownPending) 
	//set in interrupt or check shutdown ID comes from CAN 
	{
		mAndroidShutDownPending = 0;
		printf("mAndroidShutDownPending\r\n");
		cmd[0] = 0xaa;
		cmd[1] = 0xbb;
		cmd[2] = 1;
		cmd[3] = 0x0f;
		checkValue = calculate_crc((uint8*)cmd+LEN, 2);
		cmd[4] = (unsigned char)(checkValue & 0xff);
		cmd[5] = (unsigned char)((checkValue & 0xff00) >> 8);	
		
		make_event_to_list(cmd, 6, CAN_EVENT, 0);		
		//插入关机命令
		//如果关机命令在2.2秒内无作用，则强制断电。
		Timer5_Init(22000, (u32)84*100-1); 
	}		
}



















