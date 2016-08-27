#ifndef __TIMER_H
#define __TIMER_H
#include <sys.h>	 

void Timer1_Init(u16 arr,u16 psc);  

void TIM3_Cap_Init(u16 arr,u16 psc);
void Read_Distane(void);
void TIM3_IRQHandler(void);
int Get_battery_volt(void);  
void  Adc_Init(void);
#endif
