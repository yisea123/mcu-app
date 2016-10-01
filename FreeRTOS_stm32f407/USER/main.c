
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"
#include "misc.h"
#include "usart.h"

#include "delay.h"
#include "usmart.h"
#include "led.h"
#include "rtc.h"
#include "exti.h"
#include "key.h"
#include "rng.h"
#include "iwdg.h"
#include "adc.h"
#include "dma.h"
#include "rfifo.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 21TH *****/ \
\r\n/********* %s %s *********/ \
\r\n/******************************************/ \
\r\n\r\n", __DATE__, __TIME__);

void LED0_Task(void * pvParameters);
void LED1_Task(void * pvParameters);
void RTC_read_Task(void * pvParameters);
void Key_Detect_Task(void * pvParameters);
void Temprate_Task(void * pvParameters);
void Printf_Log_Task(void * pvParameters);
void Feed_Wdg_Task( void * pvParameters );
void Timer_func1( TimerHandle_t xTimer );

Ringfifo mLogFifo;
QueueHandle_t mLogSemaphore =  NULL;
//QueueHandle_t mDmaSemaphore =  NULL;
unsigned char mSendBuffer[512];
TimerHandle_t xTimer1 = NULL;
TaskHandle_t pxKeyDetectTask;
TaskHandle_t pxLogTask;
TaskHandle_t pxTimeTask;
TaskHandle_t pxTempretureTask;
TaskHandle_t pxUsmartTask;

int main(void)
{
	rfifo_init( &mLogFifo );
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_2 );
	(void) uart_init( 115200 ); BOOT_LOG
	(void) RTC_INIT();
	(void) LED_Init();
	(void) EXTIX_Init();
	(void) IWDG_Init( 4,2000 ); //Óë·ÖÆµÊýÎª64,ÖØÔØÖµÎª1000,Òç³öÊ±¼äÎª2s	
	(void) Adc_Init();         //ÄÚ²¿ÎÂ¶È´«¸ÐÆ÷ADC³õÊ¼»
	while( RNG_Init() )
	{
		printf("  RNG Error! RNG Trying...\r\n");	 
	}   
	
	vSemaphoreCreateBinary( mLogSemaphore );
	//vSemaphoreCreateBinary( mDmaSemaphore );	
	
	xTaskCreate( usamrt_debug_task, (const char *)"Usmart", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY + 2, &pxUsmartTask );
	xTaskCreate( Feed_Wdg_Task, (const char *)"Wdg", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );	
	xTaskCreate( LED0_Task, (const char *)"LED0", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL );
	xTaskCreate( LED1_Task, (const char *)"LED1", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 2, NULL );
	xTaskCreate( RTC_read_Task, (const char *)"RTC", configMINIMAL_STACK_SIZE+20, NULL, tskIDLE_PRIORITY + 3, &pxTimeTask );
	xTaskCreate( Temprate_Task, (const char *)"Temprature", configMINIMAL_STACK_SIZE+30, NULL, tskIDLE_PRIORITY + 3, &pxTempretureTask );		
	xTaskCreate( Key_Detect_Task, (const char *)"Key", configMINIMAL_STACK_SIZE, NULL, configKDETECT_TASK_PRIORITY, &pxKeyDetectTask );
	xTaskCreate( Printf_Log_Task, (const char *)"Log", configMINIMAL_STACK_SIZE, NULL, configLOG_TASK_PRIORITY, &pxLogTask );	

	vTaskStartScheduler();
}

/*
*	author: yangjianzhou
* function: Timer_func1. For test timers in FreeRTOS.
**/
void Timer_func1( TimerHandle_t xTimer )
{
	/*run is Timers Task context with 
		configTIMER_TASK_PRIORITY priority.*/
	//printf("System timer callback in %u(ms)\r\n",
	//	(xTimerGetPeriod( xTimer ) / portTICK_PERIOD_MS));
}

/*
*	author: yangjianzhou
* function: LED0_Task.
**/
void LED0_Task(void * pvParameters)
{
	UBaseType_t pre;
	
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );

	/*create and start a timer in FreeRTOS*/
	xTimer1 = xTimerCreate("timer1", 20000, pdTRUE, "T1", Timer_func1);
	xTimerStart(xTimer1, 0);
	
	while (1)
	{
		LED0 = !LED0;
		vTaskDelay( 5000 / portTICK_RATE_MS );
	}
}

/*
*	author: yangjianzhou
* function: LED1_Task.
**/
void LED1_Task( void * pvParameters )
{
	UBaseType_t pre;
	
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );
	
	while( 1 )
	{
		LED1 = !LED1;
		vTaskDelay( 4000 / portTICK_RATE_MS );
	}
}

/*
*	author: yangjianzhou
* function: Feed_Wdg_Task. Feed watch dog in every 500ms.
**/
void Feed_Wdg_Task( void * pvParameters )
{
	TickType_t pxPreviousWakeTime;
	UBaseType_t pre;
	
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );
	
	pxPreviousWakeTime = xTaskGetTickCount();
	
	while( 1 )
	{
		( void ) IWDG_Feed();
		vTaskDelayUntil( &pxPreviousWakeTime, 2000 / portTICK_RATE_MS );
	}
}

/*
*	author: yangjianzhou
* function: Temprate_Task.
**/
void Temprate_Task(void * pvParameters)
{
	short temp;
	UBaseType_t pre;
	
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );
	
	while (1)
	{
		vTaskDelay( /*45000 / portTICK_RATE_MS*/portMAX_DELAY );		
		temp = Get_Temprate();
		if( temp < 0 )
		{
			temp = -temp;
			vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
			printf("temprature: -%d.%d\r\n", temp/100, temp%100);	
			vTaskPrioritySet( NULL, pre );			
		}
		else 
		{
			vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );
			printf("temprature: %d.%d\r\n", temp/100, temp%100);
			vTaskPrioritySet( NULL, pre );	
		}
	}
}

char isDma = 0;
/*
*	author: yangjianzhou
* function: Printf_Log_Task run in most hight Priority. Useing DMA to printf log, interrupt happend when 
						DMA transfer finish. 
**/
void Printf_Log_Task(void * pvParameters)
{	
	int len;

	UBaseType_t pre;
	
	pre = uxTaskPriorityGet( NULL );
	//MYDMA_Config( DMA2_Stream7, DMA_Channel_4, (u32)&USART1->DR, (u32)mSendBuffer, 512 );
	/*mLogSemaphore create as has value, take it.*/
	xSemaphoreTake( mLogSemaphore, 0 );	
	//xSemaphoreTake( mDmaSemaphore, 0 );

	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );		
	printf("%s ...\r\n", __func__);	
	vTaskPrioritySet( NULL, pre );
	
	while( 1 )
	{
		/*sleep here to wait for Log come*/		
		xSemaphoreTake( mLogSemaphore, portMAX_DELAY );
		
		while( ( len = rfifo_len( &mLogFifo ) ) > 0 ) 
		{
			rfifo_get( &mLogFifo, mSendBuffer, len );
			isDma = 1;
      USART_DMACmd( USART1, USART_DMAReq_Tx, ENABLE );  
			MYDMA_Enable( DMA2_Stream7, len );
			/*we sleep 2000ms to wait DMA finish*/
			//xSemaphoreTake( mDmaSemaphore, 2000 / portTICK_PERIOD_MS );	
			//use ulTaskNotifyTake instead of xSemaphoreTake 
			ulTaskNotifyTake( pdTRUE, 2000 / portTICK_PERIOD_MS );
			isDma = 0;
		}
	}
}

//xSemaphoreHandle xKeySemaphore = NULL ;

/*
*	author: yangjianzhou
* function: Key_Detect_Task. Wake up by key interrupt, we delay 
						10ms then check which interrupt had happened.
**/
void Key_Detect_Task(void * pvParameters)
{
	int count = 0;
	unsigned int random;	
	printf("%s ...\r\n", __func__);	

	//vSemaphoreCreateBinary( xKeySemaphore); 	
	//xSemaphoreTake( xKeySemaphore, 0 );

	while (1)
	{
		//xSemaphoreTake( xKeySemaphore, portMAX_DELAY );
		/*pdTRUE make ulTaskNotifyTake Binary
			pdFALSE make ulTaskNotifyTake Counting*/
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		vTaskDelay(10 / portTICK_RATE_MS);
		
		count = 0;
		while( WK_UP == 1 )	 
		{
			count++;
			vTaskDelay(10 / portTICK_RATE_MS);
		}		 		
		if( count > 200)
		{
			printf("%s PA0 long press!\r\n", __func__);
		}
		else if( count > 0 )
		{
			printf("%s PA0 press!\r\n", __func__);
		}
		
		count = 0;
		while( KEY1 == 0 )	 
		{
			count++;
			vTaskDelay(10 / portTICK_RATE_MS);	
		}		
		if( count > 200)
		{
			LED1=!LED1;
			printf("%s PE3 long press!\r\n", __func__);
		}
		else if( count > 0 )
		{
			LED1=!LED1;
			printf("%s PE3 press!\r\n", __func__);
		}

		count = 0;
		while( KEY0 == 0 )	 
		{
			count++;
			vTaskDelay(10 / portTICK_RATE_MS);	
		}		
		if( count > 200)
		{
			LED0=!LED0;	
			LED1=!LED1;	
			printf("%s PE4 long press!\r\n", __func__);
		}
		else if( count > 0 )
		{
			LED0=!LED0;	
			LED1=!LED1;	
			printf("%s PE4 press!\r\n", __func__);
		}		
		
		random = RNG_Get_RandomNum();	
		printf("random = %d\r\n", random);		
	}
}

static int xPrintfTime = 30;

/*
*	author: yangjianzhou
* function: vSetPrrintfTime.
**/
void vSetPrrintfTime(int time)
{
		if( time > 0 )
		{
			xPrintfTime = time;
		}
}

/*
*	author: yangjianzhou
* function: RTC_read_Task.
**/
void RTC_read_Task(void * pvParameters)
{
	unsigned char hour,min,sec,ampm;
	unsigned char year,month,date,week;
	unsigned char tbuf[40];
	TickType_t pxPreviousWakeTime;
	
	printf("%s ...\r\n", __func__);	
	pxPreviousWakeTime = xTaskGetTickCount();
	
	xPrintfTime = xPrintfTime;
	
	while (1)
	{
	  vTaskDelayUntil( &pxPreviousWakeTime, portMAX_DELAY );
		//printf("%s	1\r\n", __func__);
		RTC_Get_Time( &hour, &min, &sec, &ampm );
		//printf("%s	2\r\n", __func__);		
		if( 1 /*( sec % xPrintfTime ) == 0*/ ) 
		{
			sprintf((char*)tbuf, "Time:%02d:%02d:%02d\r\n", hour, min, sec);
			printf("%s", tbuf);
			RTC_Get_Date(&year,&month,&date,&week);
			sprintf((char*)tbuf, "Date:20%02d-%02d-%02d\r\n", year, month, date);
			printf("%s", tbuf);			
		}
	}
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	printf("\r\n%s -> [%s] , please add stack for the Task!\r\n\r\n", __func__, pcTaskName);		
}

void Printf_Application_Version( void )
{
		BOOT_LOG
}


