#include "timer.h"

static TIMER2 *mTimerHead2 = NULL;
static unsigned int mTimerCount = 0;

static unsigned int sysTimerTick = 0;
static TIMER2 *sysTimerHead = NULL;
static TIMER2 sysTimer[NUMTIMER];
static int sysTimerTotal = 0;

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
		TIM_SetAutoreload(TIM3,2000-1);//恢复原来的设置		  
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
	if(TIM_GetITStatus(TIM2, TIM_IT_Update)==SET)//溢出中断
	{
		/*中断后半部处理*/
		mTimerCount++;
		sysTimerTick++;
		
		TIM_SetCounter(TIM2, 0);		//清空定时器的CNT
		TIM_SetAutoreload(TIM2, /*1000*/ TIMER2PERIOD*10-1);//恢复原来的设置		  
		//100ms中断一次		
	}			
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //清除中断标志位    
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

/*定时器周期为TIMER2PERIOD的整数倍*/
TIMER2* register_timer2(const char name[], unsigned int times, timer_callback callback, char repeat, void *argc)
{
	TIMER2 *timer = NULL;
	
	if(times < TIMER2PERIOD)
	{
			printf("%s: times < TIMER2PERIOD error!", __func__);
			return NULL;
	}
	
	timer = (TIMER2 *) mymalloc(0, sizeof(TIMER2));
	
	if(timer) 
	{
			timer->next = NULL;
			timer->argc = argc;
			timer->mReady = 1;
			timer->times = times/TIMER2PERIOD;
			timer->timesSave = times/TIMER2PERIOD;
			timer->repeat = repeat;
			memset(timer->name, '\0', sizeof(timer->name));
			memcpy(timer->name, name, strlen(name));
			timer->callback = callback;
			
			/*关闭中断，保护链表*/
			//DISABLE_INT();
			timer->next = mTimerHead2;
			mTimerHead2 = timer;
			//ENABLE_INT();
			//printf("%s: name = [%s] OK!\r\n", __func__, name);
	} 
	else 
	{
			printf("ERROR: %s mymalloc timer %s fail!\r\n", __func__, name);
	}

	return timer;
}

int unregister_timer2(TIMER2* timer)
{
	int ret = -1;
	TIMER2 **tPtr = &mTimerHead2, *cur = NULL;
	
	/*关闭中断，保护链表*/
	//DISABLE_INT();
	while((*tPtr) != NULL) 
	{
			if((*tPtr) == timer) 
			{
					cur = *tPtr;
					(*tPtr) = (*tPtr)->next;
					//printf("%s: delete timer [%s] success.\r\n", __func__, cur->name);
					myfree(0, cur);
					ret = 0;
					break;
			}
			else
			{
					tPtr = &((*tPtr)->next);
			}
	}
	//ENABLE_INT();
	return ret;
}

void handle_timer_callback(void)
{
	while(mTimerCount > 0)
	{
			TIMER2 **tPtr = &mTimerHead2, *cur = NULL;	
		
			mTimerCount--;
		
			while((*tPtr) != NULL) 
			{
					if(((*tPtr)->mReady) == -1)
					{
							cur = *tPtr;
							(*tPtr) = (*tPtr)->next;
							printf("%s: delete timer %s. mReady=%d\r\n", __func__, cur->name, cur->mReady);
							myfree(0, cur);
							continue;
					}
					
					if(--((*tPtr)->times) <= 0) 
					{
							(*tPtr)->callback((*tPtr)->argc);
							
							if((*tPtr)->repeat == REPEAT)
							{
									(*tPtr)->times = (*tPtr)->timesSave;
									tPtr = &((*tPtr)->next);					
							}
							else 
							{
									cur = *tPtr;
									(*tPtr) = (*tPtr)->next;
									//printf("%s: delete timer %s. repeat=%d\r\n", __func__, cur->name, cur->repeat);
									myfree(0, cur);
							}				
					}
					else
					{
							tPtr = &((*tPtr)->next);
					}
			}		
	}
}

/*
*	author: yangjianzhou
* function: clean sturct sysTimer[10].
**/
static void clean_system_timer(TIMER2 *timer)
{
		timer->isUse = 0;
		timer->mReady = 0;
		timer->repeat = 0;
		timer->times = 0;
		timer->timesSave = 0;
		timer->argc = NULL;
		timer->next = NULL;
		timer->callback = NULL;
		memset(timer->name, '\0', sizeof(timer->name));	
}

/*
*	author: yangjianzhou
* function: add timer to list, return NULL fail.
**/
TIMER2* register_system_timer(const char name[], unsigned int times, timer_callback callback, char repeat, void *argc)
{
		int i;
		TIMER2 *timer = NULL;
	
		if (sysTimerTotal < sizeof(sysTimer)/sizeof(sysTimer[0])) {
				for (i=0; i<sizeof(sysTimer)/sizeof(sysTimer[0]); i++) {
						if (sysTimer[i].isUse == 0) {
								break;
						}
				}
				if (i < sizeof(sysTimer)/sizeof(sysTimer[0])) {
						timer = &(sysTimer[i]);
						timer->isUse = 1;			
						timer->next = NULL;
						timer->argc = argc;
						timer->mReady = 1;
						timer->times = times/TIMER2PERIOD;
						timer->timesSave = times/TIMER2PERIOD;
						timer->repeat = repeat;
						memset(timer->name, '\0', sizeof(timer->name));
						memcpy(timer->name, name, strlen(name));
						timer->callback = callback;
						//DISABLE_INT();
						timer->next = sysTimerHead;
						sysTimerHead = timer;
						//ENABLE_INT();		
						sysTimerTotal++;					
				}
		}
		if (timer == NULL) {
				printf("%s: Fail!!! sysTimerTotal = %d", __func__, sysTimerTotal);
		}
		
		return timer;
}

/*
*	author: yangjianzhou
* function: delete timer from list, return 0 success, return -1 fail.
**/
int unregister_system_timer(TIMER2* timer)
{
		int ret = -1;
		TIMER2 **mPtr = &sysTimerHead, *cur = NULL;
		
		//DISABLE_INT();
		while ((*mPtr) != NULL) 
		{
				if ((*mPtr) == timer) {
						cur = *mPtr;
						(*mPtr) = (*mPtr)->next;
						//printf("%s: delete timer [%s] success.\r\n", __func__, cur->name);
						clean_system_timer(cur);				
						sysTimerTotal--;
						ret = 0;
						break;
				} else {
						mPtr = &((*mPtr)->next);
				}
		}
		//ENABLE_INT();
		
		return ret;	
}

/*
*	author: yangjianzhou
* function: check if timer expire, callback for the timer.
**/
void poll_system_timer(void)
{
		unsigned int count;
		TIMER2 **tPtr = &sysTimerHead, *cur = NULL;	
		
		count = sysTimerTick;
		sysTimerTick = 0;/*set sysTimerTick to zero maybe lost counter*/
	
		if (count > 0) {
				while ((*tPtr) != NULL) {
						if (((*tPtr)->mReady) == -1) {
								cur = *tPtr;
								(*tPtr) = (*tPtr)->next;
								printf("%s: delete timer %s. mReady=%d\r\n", __func__, cur->name, cur->mReady);
								sysTimerTotal--;
								clean_system_timer(cur);				
								continue;
						}
						if ((*tPtr)->times <= count) {
								(*tPtr)->callback((*tPtr)->argc);
								if ((*tPtr)->repeat == REPEAT) {
										(*tPtr)->times = (*tPtr)->timesSave;
										tPtr = &((*tPtr)->next);					
								} else {
										cur = *tPtr;
										(*tPtr) = (*tPtr)->next;
										sysTimerTotal--;
										clean_system_timer(cur);
										//printf("%s: delete timer %s. repeat=%d\r\n", __func__, cur->name, cur->repeat);									
								}				
						} else {
								(*tPtr)->times -= count;
								tPtr = &((*tPtr)->next);
						}
				}		
		}		
}

/*
*	author: yangjianzhou
* function: init sysTimer[10].
**/
void init_system_timer(void)
{
		int i = 0;
		
		sysTimerTotal = 0;
		for (i=0; i<(sizeof(sysTimer)/sizeof(sysTimer[0])); i++) {
				clean_system_timer(&(sysTimer[i]));
		}
}



