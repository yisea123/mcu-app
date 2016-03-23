#include "timer.h"

/********************************************************************
给上报给android中的数据包struct car_event* event_sending中的tim_count
字段计数，计数的值代表这个数据有多长时间没有得到android的回复包 ，
如果超过0.5秒还没回复，就要重新发送这个包

此部分代码的功能已集成到定时器2上，所以在main中不初始化Timer3_Init
*********************************************************************/
void TIM3_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)//溢出中断
	{
		//if(event_sending != NULL)
		//	event_sending->tim_count++;
		
		TIM_SetCounter(TIM3,0);		//清空定时器的CNT
		TIM_SetAutoreload(TIM3,2000);//恢复原来的设置		  
		//2000=200ms中断一次	
	}			
	
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //清除中断标志位    
}

void Timer3_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///使能TIM3时钟

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//初始化定时器3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM3,ENABLE); //使能定时器4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;//外部中断3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置NVIC
	 							 
}

//定时器3中断服务程序	 
void TIM5_IRQHandler(void)
{ 		    		  	
	TIM_Cmd(TIM5, DISABLE);		
	
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)==SET)//溢出中断
	{
		//printf("T5\r\n");
		if(mAndroidPower != 0) 
			power_android(0);
	}			
	
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);  //清除中断标志位   
}

//使能定时器5,使能中断.
//如果2.2S内android没有回复关机成功的命令，则定时器强制关机
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

/********************************************************************
1、用于周期性命令包的时间记数，所有周期性命令包中的一个字段都指向了
periodicNum全局变量，这个变量会每100MS自加加一次。
2、给event_sending的tim_count字段计数
3、给下载固件中的mTickCount计数
*********************************************************************/

void TIM2_IRQHandler(void)
{ 
	static int lgperiod = 0, countsecond = 0;
	
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)//溢出中断
	{
		++periodicNum;

		if(event_sending != NULL)
			event_sending->tim_count++;
		
		if(session_begin)
		{
			/**41也就是4100毫秒 = 4.1秒，开始传输rom后，4.1秒内没有rom数据，重置tmodem状态*/
			if(mTmodemTickCount++ > 71)	
			{
				reset_tmodem_status();
			}
		}
		
		/*40m*/
		if(++lgperiod > 400) 
		{//400 //10其中一个MCU的时钟比较慢！
			lgperiod = 0;
			notify_longsung_period();
		}
		
		if(++countsecond > 10) 
		{
			countsecond = 0;
			notify_longsung_second();
		}
		
		TIM_SetCounter(TIM2,0);		//清空定时器的CNT
		TIM_SetAutoreload(TIM2,1000);//恢复原来的设置		  
		//100ms中断一次		
	}			
	
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //清除中断标志位    
}

/*********************************************************************
使能定时器4,使能中断.
Timer3_Init(1000,(u32)sysclk*100-1);//分频,时钟为10K ,100ms中断一次
用于给全局变量 periodicNum 每100毫秒加一。
***********************************************************************/
void Timer2_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///使能TIM3时钟

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//初始化定时器3
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM2,ENABLE); //使能定时器4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//外部中断3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//子优先级0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置NVIC 							 
}




