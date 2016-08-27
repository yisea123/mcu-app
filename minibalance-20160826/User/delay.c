#include "delay.h"

static uint8_t  fac_us=0;																		//us延时倍乘数
static uint16_t fac_ms=0;																		//ms延时倍乘数

void interrupt_priority_init(uint32_t priority)
{
		NVIC_PriorityGroupConfig(priority);//0-3 0-3	
}

void delay_init(uint8_t SYSCLK)
{
		SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟,HCLK/8
		fac_us=SYSCLK/8; //硬件分频,fac_us得出的值要给下面的时钟函数使用
		fac_ms =(u16)fac_us*1000; 	
}			  

void delay_us(uint32_t nus)
{		
		uint32_t temp;  
		SysTick->LOAD = nus*fac_us;  //延时10us,10*9 = 90,装到load寄存器中
		SysTick->VAL=0x00;//计数器清0
		SysTick->CTRL = 0x01;//配置异常生效,也就是计数器倒数到0时发出异常通知
		do  
		{  
			 temp = SysTick->CTRL;//时间到,该位将被硬件置1,但被查询后自动清0
		}  
		while(temp & 0x01 && !(temp &(1<<16)));//查询
		SysTick->CTRL = 0x00;//关闭定时器
		SysTick->VAL = 0x00;//清空val,清空定时器
}


void delay_ms(uint16_t nms)
{	 		  	  
		uint32_t temp;  
		SysTick->LOAD = nms*fac_ms;//延时10ms,10*9 = 90,装到load寄存器中
		SysTick->VAL=0x00;//计数器清0
		SysTick->CTRL = 0x01;//配置异常生效,也就是计数器倒数到0时发出异常通知
		do  
		{  
			 temp = SysTick->CTRL;//时间到,该位将被硬件置1,但被查询后自动清0
		}  
		while(temp & 0x01 && !(temp &(1<<16)));//查询
	
		SysTick->CTRL = 0x00;//关闭定时器
		SysTick->VAL = 0x00;//清空val,清空定时器
} 
