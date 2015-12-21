#ifndef __IOCTR_H
#define __IOCTR_H	 
#include "sys.h" 

/******************OUTPUT********************/

#define RCC_ALL_PORT 	(RCC_AHB1Periph_GPIOC | RCC_AHB1Periph_GPIOD)

#define UP2USB_PWR_BASE       		GPIOD
#define UP2USB_PWR 								GPIO_Pin_10
#define UP2PS_USB_SYNC_BASE       GPIOD
#define UP2PS_USB_SYNC 						GPIO_Pin_11
#define UP2PS_8V_SW_EN_BASE       GPIOD
#define UP2PS_8V_SW_EN   					GPIO_Pin_6
#define UP2PS_5V_SW_EN_BASE       GPIOD
#define UP2PS_5V_SW_EN   					GPIO_Pin_5
#define UP2PS_3V3_SW_EN_BASE      GPIOD
#define UP2PS_3V3_SW_EN   				GPIO_Pin_7
#define UP2PS_SYNC_BASE       		GPIOD
#define UP2PS_SYNC   							GPIO_Pin_4
#define UP2TUNER_ANT_ON_OFF_BASE  GPIOD
#define UP2TUNER_ANT_ON_OFF 			GPIO_Pin_1


#define UP2PS_ENABLE_BASE  				GPIOC
#define UP2PS_ENABLE							GPIO_Pin_12

#define MAIN_PWR_EN_BASE					GPIOC
#define MAIN_PWR_EN								GPIO_Pin_9
/******************OUTPUT********************/


/******************INPUT********************/
#define CORE2UP_POWER_OFF 		GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_9) //PA9

/******************INPUT********************/

void Init_io_ctrl(void);	
void power_android(uint32_t on);
void bsp_io_low(char port, uint16_t num);
void bsp_io_heigh(char port, uint16_t num);
extern char mAndroidPower;
extern char mAndroidRunning;

#endif
