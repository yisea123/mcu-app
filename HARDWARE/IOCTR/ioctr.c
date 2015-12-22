#include "ioctr.h"
#include "sys.h"
#include "car_event.h"

extern char mAndroidShutDownPending;
extern char mAndroidPowerUpPending;
extern char mDebugUartPrintfEnable;

char mAndroidPower = 0;

char mAndroidRunning = 0;

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

void power_android(uint32_t on)
{
	mAndroidPower = on;
	
	if(on)
	{
		UP2USB_PWR = 1;
		UP2PS_USB_SYNC = 1;
		UP2PS_8V_SW_EN = 1;
		UP2PS_5V_SW_EN = 1;
		UP2PS_3V3_SW_EN = 1;	

		UP2PS_ENABLE = 1;	
	}
	else
	{
		UP2PS_ENABLE = 0;	
		
		UP2USB_PWR = 0;
		UP2PS_USB_SYNC = 0;
		UP2PS_8V_SW_EN = 0;
		UP2PS_5V_SW_EN = 0;
		UP2PS_3V3_SW_EN = 0;	
		
		mAndroidRunning = 0;//power off 	
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
		Timer5_Init(22000, (u32)84*100-1); 
	}		
}

void TIM5_IRQHandler(void)
{ 		    		  	
	TIM_Cmd(TIM5, DISABLE);		
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)==SET)//溢出中断
	{
		//printf("T5\r\n");
		if(mAndroidPower != 0) power_android(0);
	}			
	
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);  //清除中断标志位   
}

void Timer5_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseInitStructure);//初始化定时器5
	
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);  //清除中断标志位   
	
	TIM_ITConfig(TIM5,TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM5, ENABLE);
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;//外部中断3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置NVIC					 
}


















