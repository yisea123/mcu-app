#include "exti.h"
#include "sys.h"
  /**************************************************************************
���ߣ�ƽ��С��֮��
�ҵ��Ա�С�꣺http://shop114407458.taobao.com/
**************************************************************************/
/**************************************************************************
�������ܣ��ⲿ�жϳ�ʼ��
��ڲ�������
����  ֵ���� 
**************************************************************************/
void EXTI_Init(void)
{
  	//GPIO_InitTypeDef  GPIO_InitStructure, GPIO_InitStructure1;

	RCC->APB2ENR|=1<<3;    //ʹ��PORTBʱ��	   	 
//	GPIOB->CRL&=0XFF0FFFFF; 
//	GPIOB->CRL|=0X00800000;//PB5 ��������
 //   GPIOB->ODR|=1<<5; //PB5 ����	
//	Ex_NVIC_Config(GPIO_B,5,FTIR);		//�½��ش���
//	MY_NVIC_Init(2,1,EXTI9_5_IRQn,2);  	//��ռ2�������ȼ�1����2

	GPIOB->CRL&=0XF0FFFFFF; 
	GPIOB->CRL|=0X08000000;//PB5 ��������
  	GPIOB->ODR|=1<<6; //PB5 ����	
	Ex_NVIC_Config(GPIO_B, 6, FTIR);		//�½��ش���
	MY_NVIC_Init(2,1, EXTI9_5_IRQn, 2);  	//��ռ2�������ȼ�1����2
}









