#include "sys.h"
#include "gpio.h"
#include "gpioconfig.h"
#include "stdint.h"
//=============================================================================
//   General purpose IO configuration
//=============================================================================

//
const HW_DIO DIO_SET[] = 
{
  
		//const HW_DIO_T DIO_IN_PE_4 =   //0
		{
		   GPIO_E,
		   PIN_4,
		   ACTIVE_HIGH,
		   GPIO_Mode_IN,
		   GPIO_Fast_Speed,
		   GPIO_OType_PP,
		   GPIO_PuPd_UP
		},

		//const HW_DIO_T DIO_OUT_PC_12 =   //1
		{
		   GPIO_C,
		   PIN_12,
		   ACTIVE_HIGH,
		   GPIO_Mode_OUT,
		   GPIO_Fast_Speed,
		   GPIO_OType_PP,
		   GPIO_PuPd_UP
		},

		//const HW_DIO_T DIO_IN_PD_10 =   //2
		{
		   GPIO_D,
		   PIN_10,
		   ACTIVE_HIGH,
		   GPIO_Mode_OUT,
		   GPIO_Fast_Speed,
		   GPIO_OType_PP,
		   GPIO_PuPd_UP
		},

		//const HW_DIO_T DIO_IN_PA_04 =   //3
		{
		   GPIO_A,
		   PIN_4,
		   ACTIVE_LOW,
		   GPIO_Mode_OUT,
		   GPIO_Fast_Speed,
		   GPIO_OType_PP,
		   GPIO_PuPd_UP
		},
	   //const HW_DIO_T DIO_IN_PA_06 =	 //4
	   {
		  GPIO_A,
		  PIN_6,
		  ACTIVE_LOW,
		  GPIO_Mode_OUT,
		  GPIO_Fast_Speed,
		  GPIO_OType_PP,
		  GPIO_PuPd_UP
	   },
	   
	   //const HW_DIO_T DIO_IN_PC_01 =	 //5
	   {
		  GPIO_C,
		  PIN_1,
		  ACTIVE_LOW,
		  GPIO_Mode_OUT,
		  GPIO_Fast_Speed,
		  GPIO_OType_PP,
		  GPIO_PuPd_UP
	   },
	   
	   //const HW_DIO_T DIO_IN_PA_05 =	 //6
	   {
		  GPIO_A,
		  PIN_5,
		  ACTIVE_LOW,
	      GPIO_Mode_OUT,
		  GPIO_Fast_Speed,
		  GPIO_OType_PP,
		  GPIO_PuPd_UP
	   },
        //const HW_DIO_T DIO_IN_PA_01 =	//7
		  {
			 GPIO_A,
			 PIN_1,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PD_05 =	//8
		  {
			 GPIO_D,
			 PIN_5,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PC_02 =	//9
		  {
			 GPIO_C,
			 PIN_2,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PD_01 =	//10
		  {
			 GPIO_D,
			 PIN_1,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
		 //const HW_DIO_T DIO_IN_PD_03 =   //11
		 {
			GPIO_D,
			PIN_3,
			ACTIVE_HIGH,
			GPIO_Mode_OUT,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 },
		 
		 //const HW_DIO_T DIO_IN_PD_07 =   //12
		 {
			GPIO_D,
			PIN_7,
			ACTIVE_HIGH,
			GPIO_Mode_OUT,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 },
		 
		 //const HW_DIO_T DIO_IN_PD_06 =   //13
		 {
			GPIO_D,
			PIN_6,
			ACTIVE_HIGH,
			GPIO_Mode_OUT,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 },

          //const HW_DIO_T DIO_IN_PA_07 = //14
		  {
			 GPIO_A,
			 PIN_7,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PC_03 =	//15
		  {
			 GPIO_C,
			 PIN_3,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PB_01 =	//16
		  {
			 GPIO_B,
			 PIN_1,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PA_02 =	//17
		  {
			 GPIO_A,
			 PIN_2,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
		 //const HW_DIO_T DIO_IN_PA_08 =   //18
		 {
			GPIO_A,
			PIN_8,
			ACTIVE_HIGH,
			GPIO_Mode_OUT,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 },
		 //const HW_DIO_T DIO_IN_PC_09 =	//19
		  {
			 GPIO_C,
			 PIN_9,
			 ACTIVE_HIGH,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PE_02 =	//20
		  {
			 GPIO_E,
			 PIN_2,
			 ACTIVE_LOW,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
  
		  //const HW_DIO_T DIO_IN_PE_07 =	//21
		  {
			 GPIO_E,
			 PIN_7,
			 ACTIVE_LOW,
			 GPIO_Mode_OUT,
			 GPIO_Fast_Speed,
			 GPIO_OType_PP,
			 GPIO_PuPd_UP
		  },
		 //const HW_DIO_T DIO_IN_PE_08 =   //22
		 {
			GPIO_E,
			PIN_8,
			ACTIVE_HIGH,
			GPIO_Mode_OUT,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 },
		 //const HW_DIO_T DIO_IN_PD_14 =   //23
         {
			GPIO_D,
			PIN_14,
			ACTIVE_HIGH,
			GPIO_Mode_IN,
			GPIO_Fast_Speed,
			GPIO_OType_PP,
			GPIO_PuPd_UP
		 }
};

//=============================================================================
// PORT configuration function according to app
//since no HAL layer is implemented, all the app related configuration are done in this file
//=============================================================================
void gpio_initialize( void )
{
  unsigned char i;
  HW_DIO * pDioSet;
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOA, ENABLE );
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOB, ENABLE );
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOC, ENABLE );
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOD, ENABLE );
  RCC_AHB1PeriphClockCmd( RCC_AHB1Periph_GPIOE, ENABLE );
	
  for( i = 0; i < sizeof( DIO_SET )/sizeof( HW_DIO ); i++ )
  {
    pDioSet = ( HW_DIO* )&DIO_SET[i];
    DIO_Port_Initialize_Pin_State( pDioSet );
    if( pDioSet->GPIO_Mode == GPIO_Mode_OUT )
    {
      DIO_Port_Set_Pin_State( pDioSet, 0 );
    }
  }
}


void android_power_init( void )
{
	DIO_Port_Set_Pin_State(uP2PS_ENABLE,1);// 1 12最先开启
	DIO_Port_Set_Pin_State(uP24G_BAT_ON,1);// 2 打开4G电源
	DIO_Port_Set_Pin_State(uP24G_PWR_ON,1);// 3
	DIO_Port_Set_Pin_State(uP24G_RST,1);// 4
	DIO_Port_Set_Pin_State(uP24G_WAKE_AP,1);// 5
	DIO_Port_Set_Pin_State(uP24G_W_DISABLE,1);// 6 
	DIO_Port_Set_Pin_State(uP2BL_PWR_EN,1);// 7 开启显示屏背光
	DIO_Port_Set_Pin_State(uP2EXTAMP_EN,1);// 8
	DIO_Port_Set_Pin_State(uP2TUNER_ANT_ON,1);// 9
	DIO_Port_Set_Pin_State(uP25V25_SW_EN,1);// 10
	DIO_Port_Set_Pin_State(uP2PS_3V3_SW_EN,1);// 11
	DIO_Port_Set_Pin_State(uP2PS_1V8_LR_EN,1);// 12
	DIO_Port_Set_Pin_State(uP2MUSB_PWR_EN,1);// 13
	DIO_Port_Set_Pin_State(uP2FRUSB_PWR_EN,1);// 14
	DIO_Port_Set_Pin_State(uP2DVRUSB_PWR_EN,1);// 15
	DIO_Port_Set_Pin_State(uP2CTP_PWR_EN,1);// 16
	DIO_Port_Set_Pin_State(uP2GNSS_PWR_EN,1);// 17
	DIO_Port_Set_Pin_State(uMAIN_PWR_EN,1);// 18
	DIO_Port_Set_Pin_State(uMCU2CORE_UBOOT,1);// 19
	DIO_Port_Set_Pin_State(uMCU2CORE_BLU_EN,1);// 20正常模式静默脚为高
	//DIO_Port_Set_Pin_State(uMCU2CORE_RESET,1);// 21    
	DIO_Port_Set_Pin_State(uP2PS_5V_SW_EN,1);// 22
	
}

#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>

void android_power_reset( void )
{
	printf("%s \r\n", __func__);
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 0 );
	vTaskDelay( 500 / portTICK_RATE_MS );	
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 1 );
	vTaskDelay( 500 / portTICK_RATE_MS );
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 0 );
	vTaskDelay( 500 / portTICK_RATE_MS );
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 1 );	
}

void android_power_on( void )
{
	printf("%s \r\n", __func__);
	android_power_reset();
}

void android_power_off( void )
{
	printf("%s \r\n", __func__);
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 0 );
	DIO_Port_Set_Pin_State( uMCU2CORE_RESET, 0 );
}

void longsung_power_reset( void )
{
	printf("%s \r\n", __func__);

	DIO_Port_Set_Pin_State(uP24G_PWR_ON,0);// 3
	vTaskDelay( 2000 / portTICK_RATE_MS );	
	DIO_Port_Set_Pin_State(uP24G_PWR_ON,1);// 3	
	//vTaskDelay( 100 / portTICK_RATE_MS );
	DIO_Port_Set_Pin_State(uP24G_RST,0);// 4	
	vTaskDelay( 100 / portTICK_RATE_MS );
	DIO_Port_Set_Pin_State(uP24G_RST,1);// 4		
}



