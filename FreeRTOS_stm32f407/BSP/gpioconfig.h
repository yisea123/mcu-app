#ifndef _GPIO_CONFIG_H
#define _GPIO_CONFIG_H
#include "gpio.h"

extern const HW_DIO DIO_SET[];

#define ACC_SWITCH  &DIO_SET[0] 

#define uP2PS_ENABLE   &DIO_SET[1] 
#define uP24G_BAT_ON   &DIO_SET[2]
#define uP24G_PWR_ON   &DIO_SET[3]
#define uP24G_RST   	&DIO_SET[4]
#define uP24G_WAKE_AP   &DIO_SET[5]
#define uP24G_W_DISABLE   &DIO_SET[6]
#define uP2BL_PWR_EN   &DIO_SET[7]
#define uP2PS_5V_SW_EN   &DIO_SET[8]
#define uP2EXTAMP_EN   &DIO_SET[9]
#define uP2TUNER_ANT_ON   &DIO_SET[10]
#define uP25V25_SW_EN   &DIO_SET[11]
#define uP2PS_3V3_SW_EN   &DIO_SET[12]
#define uP2PS_1V8_LR_EN   &DIO_SET[13]
#define uP2MUSB_PWR_EN   &DIO_SET[14]
#define uP2FRUSB_PWR_EN   &DIO_SET[15]
#define uP2DVRUSB_PWR_EN   &DIO_SET[16]
#define uP2CTP_PWR_EN   &DIO_SET[17]
#define uP2GNSS_PWR_EN   &DIO_SET[18]
#define uMAIN_PWR_EN			&DIO_SET[19]
#define uMCU2CORE_RESET	&DIO_SET[20]
#define uMCU2CORE_UBOOT	&DIO_SET[21]
#define uMCU2CORE_BLU_EN		&DIO_SET[22]

extern void gpio_initialize( void );
extern void android_power_init( void );
extern void android_power_reset( void );
#endif
