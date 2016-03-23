#include "timer.h"

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
		TIM_SetAutoreload(TIM3,2000);//�ָ�ԭ��������		  
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
	static int lgperiod = 0, countsecond = 0;
	
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)//����ж�
	{
		++periodicNum;

		if(event_sending != NULL)
			event_sending->tim_count++;
		
		if(session_begin)
		{
			/**41Ҳ����4100���� = 4.1�룬��ʼ����rom��4.1����û��rom���ݣ�����tmodem״̬*/
			if(mTmodemTickCount++ > 71)	
			{
				reset_tmodem_status();
			}
		}
		
		/*40m*/
		if(++lgperiod > 400) 
		{//400 //10����һ��MCU��ʱ�ӱȽ�����
			lgperiod = 0;
			notify_longsung_period();
		}
		
		if(++countsecond > 10) 
		{
			countsecond = 0;
			notify_longsung_second();
		}
		
		TIM_SetCounter(TIM2,0);		//��ն�ʱ����CNT
		TIM_SetAutoreload(TIM2,1000);//�ָ�ԭ��������		  
		//100ms�ж�һ��		
	}			
	
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //����жϱ�־λ    
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




