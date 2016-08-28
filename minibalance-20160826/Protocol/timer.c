#include "timer.h"
#include "minibalance.h"

extern uint32_t SystemTimeCount;

uint32_t testCount = 0;
unsigned int sysTimerTick = 0;
/*sysTimerTick++ in every ms*/
static SYSTIMER *sysTimerHead = NULL;
static SYSTIMER sysTimer[NUMTIMER];
static int sysTimerTotal = 0;
static void clean_system_timer(SYSTIMER *timer);

void timer1_int_init(u16 arr,u16 psc)
{
		TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
	
		TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1,ENABLE);

		//TIM_DeInit(TIM1); 

		TIM_TimeBaseStructure.TIM_Period = arr;
		TIM_TimeBaseStructure.TIM_Prescaler = psc;
		TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
		TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
		TIM_TimeBaseInit(TIM1,&TIM_TimeBaseStructure);

		//TIM_ClearFlag(TIM1, TIM_FLAG_Update); 
		TIM_ITConfig(TIM1,TIM_IT_Update,ENABLE);
	
		NVIC_InitStructure.NVIC_IRQChannel = TIM1_UP_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStructure);

		//TIM_Cmd(TIM1, ENABLE);
}

void TIM1_UP_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM1,TIM_IT_Update) != RESET)
	{ 
			TIM_ClearITPendingBit(TIM1,TIM_IT_Update);
			//printf("%s: SystemTimeCount=%d\r\n", __func__, SystemTimeCount);
	}
}

//timer2_int_init(10000-1, 7199)  10000-1=1s
void timer2_int_init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM2, ENABLE);  //使能TIMx					 
}

/**
  * TIM2_IRQHandler loop be call in peroid
  */
void TIM2_IRQHandler(void)   				
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)  
    {
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
				//printf("%s: SystemTimeCount=%d\r\n", __func__, SystemTimeCount);	
    }
}  

void timer3_int_init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能

    //定时器TIM3初始化
    TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
    TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位

    TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

    //中断优先级NVIC设置
    NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
		/*系统时钟，优先级最高*/
		/* will not be interrupt by same NVIC_IRQChannelPreemptionPriority interrupt*/
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //从优先级3级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
    NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
    TIM_Cmd(TIM3, ENABLE);  //使能TIMx
}

/**
  * TIM3_IRQHandler
  */
void TIM3_IRQHandler(void)   				
{
    if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  
    {
        TIM_ClearITPendingBit(TIM3, TIM_IT_Update);
        SystemTimeCount++;
				sysTimerTick++;		
				//if (SystemTimeCount%5 == 0) {
				//		poll_minibalance_core(NULL);
				//}
    }
}  

//timer4_int_init(50000-1, 7199); 50000-1=5s is max value
void timer4_int_init(u16 arr,u16 psc)
{
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

	TIM_Cmd(TIM4, ENABLE);  //使能TIMx					 
}

/**
  * TIM4_IRQHandler loop be call in peroid
  */
void TIM4_IRQHandler(void)   				
{
    if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  
    {
        TIM_ClearITPendingBit(TIM4, TIM_IT_Update);		
    }
}  

/*
*	author: yangjianzhou
* function: clean sturct sysTimer[10].
**/
static void clean_system_timer(SYSTIMER *timer)
{
		timer->isUse = 0;
		timer->destory = 0;
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
SYSTIMER* register_system_timer(const char name[], unsigned int times, 
		timer_callback callback, char repeat, void *argc)
{
		int i;
		SYSTIMER *timer = NULL;
	
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
						timer->destory = 0;
						timer->times = times/TIMERPERIOD;
						timer->timesSave = times/TIMERPERIOD;
						timer->repeat = repeat;
						memset(timer->name, '\0', sizeof(timer->name));
						memcpy(timer->name, name, strlen(name));
						timer->callback = callback;
						//DISABLE_INT();
						timer->next = sysTimerHead;
						sysTimerHead = timer;
						INIT_LIST_HEAD(&(timer->msgHead)); 
						//ENABLE_INT();		
						sysTimerTotal++;	
						printf("%s: add timer[%s] success. sysTimerTotal=%d, i=%d\r\n",
							__func__, timer->name, sysTimerTotal, i);					
				}
		}
		if (timer == NULL) {
				printf("%s: %s Fail!!! sysTimerTotal = %d\r\n", __func__, name, sysTimerTotal);
		}
		
		return timer;
}

/*
*	author: yangjianzhou
* function: delete timer from list, return 0 success, return -1 fail.
* must be not call in interrupt!
**/
int unregister_system_timer(SYSTIMER* timer)
{
		int ret = -1;
		SYSTIMER **mPtr = &sysTimerHead, *cur = NULL;
		
		//DISABLE_INT();
		while ((*mPtr) != NULL) {
				if ((*mPtr) == timer) {
						cur = *mPtr;
						(*mPtr) = (*mPtr)->next;
						sysTimerTotal--;
						printf("%s: delete timer [%s]. sysTimerTotal=%d\r\n",
							__func__, cur->name, sysTimerTotal);
						clean_system_timer(cur);
						clean_message(cur);					
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
* modify in 2016.8.27
**/
void poll_system_timer(void)
{
		unsigned int t;
		MESSAGE* msg = NULL;	
		SYSTIMER **mPtr = &sysTimerHead, *cur = NULL;	
	  static unsigned int systimes = 0;
		unsigned int temp = sysTimerTick;
		const unsigned int count = temp - systimes;
		systimes = temp;
	
		if (count > 0) {
			t = count;
			while ((*mPtr) != NULL) {
					(*mPtr)->message = NULL;
					while((msg = get_message(*mPtr))) {
							(*mPtr)->isMessage = 1;
							(*mPtr)->message = msg;
							(*mPtr)->callback(*mPtr);	
					}			
					if ((*mPtr)->times <= t) {
							(*mPtr)->isMessage = 0;
							(*mPtr)->callback(*mPtr);	
							t = t - (*mPtr)->times;
							(*mPtr)->times = (*mPtr)->timesSave;						
							if (t > 0) {
									continue;
							}						
							if ((*mPtr)->repeat == REPEAT) {
									if (((*mPtr)->destory) == 1) {
										cur = *mPtr;
										(*mPtr) = (*mPtr)->next;
										t = count;
										sysTimerTotal--;												
										clean_system_timer(cur);
										clean_message(cur);
										continue;
									} else {
											mPtr = &((*mPtr)->next);
											t = count;
									}										
							} else {
									cur = *mPtr;
									(*mPtr) = (*mPtr)->next;
									t = count;
									sysTimerTotal--;											
									clean_system_timer(cur);
									clean_message(cur);
									continue;										
							}									
					} else {
							(*mPtr)->times -= t;
							mPtr = &((*mPtr)->next);
							t = count;
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



