#include "timer.h"

static TIMER2 *mTimerHead2 = NULL;
static unsigned int mTimerCount = 0;

static unsigned int sysTimerTick = 0;
static TIMER2 *sysTimerHead = NULL;
static TIMER2 sysTimer[NUMTIMER];
static int sysTimerTotal = 0;

/********************************************************************
���ϱ���android�е����ݰ�struct car_event* event_sending�е�tim_count
�ֶμ�����������ֵ������������ж೤ʱ��û�еõ�android�Ļظ��� ��
�������0.5�뻹û�ظ�����Ҫ���·��������

�˲��ִ���Ĺ����Ѽ��ɵ���ʱ��2�ϣ�������main�в���ʼ��Timer3_Init
*********************************************************************/
void TIM3_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetITStatus(TIM3,TIM_IT_Update)==SET)//����ж�
	{
		//if(event_sending != NULL)
		//	event_sending->tim_count++;
		
		TIM_SetCounter(TIM3,0);		//��ն�ʱ����CNT
		TIM_SetAutoreload(TIM3,2000-1);//�ָ�ԭ��������		  
		//2000=200ms�ж�һ��	
	}			
	
	TIM_ClearITPendingBit(TIM3,TIM_IT_Update);  //����жϱ�־λ    
}

void Timer3_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);  ///ʹ��TIM3ʱ��

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM3,&TIM_TimeBaseInitStructure);//��ʼ����ʱ��3
	
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM3,ENABLE); //ʹ�ܶ�ʱ��4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;//�ⲿ�ж�3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����NVIC
	 							 
}

//��ʱ��3�жϷ������	 
void TIM5_IRQHandler(void)
{ 		    		  	
	TIM_Cmd(TIM5, DISABLE);		
	
	if(TIM_GetITStatus(TIM5,TIM_IT_Update)==SET)//����ж�
	{
		//printf("T5\r\n");
		if(mAndroidPower != 0) 
			power_android(0);
	}			
	
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);  //����жϱ�־λ   
}

//ʹ�ܶ�ʱ��5,ʹ���ж�.
//���2.2S��androidû�лظ��ػ��ɹ��������ʱ��ǿ�ƹػ�
void Timer5_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseInitStructure);//��ʼ����ʱ��5
	
	TIM_ClearITPendingBit(TIM5,TIM_IT_Update);  //����жϱ�־λ   
	
	TIM_ITConfig(TIM5,TIM_IT_Update, ENABLE);

	TIM_Cmd(TIM5, ENABLE);
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;//�ⲿ�ж�3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;//�����ȼ�3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����NVIC					 
}

/********************************************************************
1�������������������ʱ�����������������������е�һ���ֶζ�ָ����
periodicNumȫ�ֱ��������������ÿ100MS�ԼӼ�һ�Ρ�
2����event_sending��tim_count�ֶμ���
3�������ع̼��е�mTickCount����
*********************************************************************/

void TIM2_IRQHandler(void)
{ 	
	if(TIM_GetITStatus(TIM2, TIM_IT_Update)==SET)//����ж�
	{
		/*�жϺ�벿����*/
		mTimerCount++;
		sysTimerTick++;
		
		TIM_SetCounter(TIM2, 0);		//��ն�ʱ����CNT
		TIM_SetAutoreload(TIM2, /*1000*/ TIMER2PERIOD*10-1);//�ָ�ԭ��������		  
		//100ms�ж�һ��		
	}			
	
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //����жϱ�־λ    
}

/*********************************************************************
ʹ�ܶ�ʱ��4,ʹ���ж�.
Timer3_Init(1000,(u32)sysclk*100-1);//��Ƶ,ʱ��Ϊ10K ,100ms�ж�һ��
���ڸ�ȫ�ֱ��� periodicNum ÿ100�����һ��
***********************************************************************/
void Timer2_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///ʹ��TIM3ʱ��

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //��ʱ����Ƶ
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //���ϼ���ģʽ
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //�Զ���װ��ֵ
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//��ʼ����ʱ��3
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //����ʱ��3�����ж�
	TIM_Cmd(TIM2,ENABLE); //ʹ�ܶ�ʱ��4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//�ⲿ�ж�3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//��ռ���ȼ�2
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;//�����ȼ�0
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//ʹ���ⲿ�ж�ͨ��
  NVIC_Init(&NVIC_InitStructure);//����NVIC 							 
}

/*��ʱ������ΪTIMER2PERIOD��������*/
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
			
			/*�ر��жϣ���������*/
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
	
	/*�ر��жϣ���������*/
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



