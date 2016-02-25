#ifndef __IOCTR_H
#define __IOCTR_H

#include "sys.h" 
#include "car_event.h"
#include "timer.h"
#include "usart.h"

/******************OUTPUT********************/

#define UP2USB_PWR 						PDout(10)
#define UP2PS_USB_SYNC 				PDout(11)
#define UP2PS_8V_SW_EN   			PDout(6)
#define UP2PS_5V_SW_EN   			PDout(5)
#define UP2PS_3V3_SW_EN   		PDout(7)
#define UP2PS_SYNC   					PDout(4)
#define UP2TUNER_ANT_ON_OFF 	PDout(1)


#define UP2PS_ENABLE					PCout(12)
#define MAIN_PWR_EN						PCout(9)
/******************OUTPUT********************/


/******************INPUT********************/
#define CORE2UP_POWER_OFF 		GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_9) //PA9

/******************INPUT********************/

extern void Init_io_ctrl(void);	
extern void power_android(uint32_t on);
extern void handle_powerup_android_request(void);
extern void handle_shutdown_android_request(void);

extern char mAndroidPower;
extern char mAndroidRunning;
extern char mAndroidShutDownPending;
#endif
