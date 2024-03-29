#include "exti.h"
#include "sys.h"
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/
/**************************************************************************
函数功能：外部中断初始化
入口参数：无
返回  值：无 
**************************************************************************/
void EXTI_Init(void)
{
  	//GPIO_InitTypeDef  GPIO_InitStructure, GPIO_InitStructure1;

	RCC->APB2ENR|=1<<3;    //使能PORTB时钟	   	 
//	GPIOB->CRL&=0XFF0FFFFF; 
//	GPIOB->CRL|=0X00800000;//PB5 上拉输入
 //   GPIOB->ODR|=1<<5; //PB5 上拉	
//	Ex_NVIC_Config(GPIO_B,5,FTIR);		//下降沿触发
//	MY_NVIC_Init(2,1,EXTI9_5_IRQn,2);  	//抢占2，子优先级1，组2

	GPIOB->CRL&=0XF0FFFFFF; 
	GPIOB->CRL|=0X08000000;//PB5 上拉输入
  	GPIOB->ODR|=1<<6; //PB5 上拉	
	Ex_NVIC_Config(GPIO_B, 6, FTIR);		//下降沿触发
	MY_NVIC_Init(2,1, EXTI9_5_IRQn, 2);  	//抢占2，子优先级1，组2
}









