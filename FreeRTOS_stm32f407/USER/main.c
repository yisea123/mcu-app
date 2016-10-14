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
#include "w25qxx.h"
#include "sdio_sdcard.h" 
#include "exfuns.h"
#include "fattester.h"
#include "wm8978.h"	 
#include "audioplay.h"	
#include "wavplay.h" 

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 21TH *****/ \
\r\n/********* %s %s *********/ \
\r\n/******************************************/ \
\r\n\r\n", __DATE__, __TIME__);

extern void Fatfs_Cat_File( char *str );
void Software_Hardware_Init( void );
void LED0_Task(void * pvParameters);
void LED1_Task(void * pvParameters);
void RTC_read_Task(void * pvParameters);
void Key_Detect_Task(void * pvParameters);
void Temprate_Task(void * pvParameters);
void Printf_Log_Task(void * pvParameters);
void Feed_Wdg_Task( void * pvParameters );
void Timer_func1( TimerHandle_t xTimer );
void Read_Fatfs(void * pvParameters);
void Music_Player(void * pvParameters);

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
TaskHandle_t pxMusicPlayer;


int main(void)
{
	FIL file;
	( void ) Software_Hardware_Init();
	if( f_open( &file, (const TCHAR*)"1:/log.txt", FA_WRITE|FA_OPEN_EXISTING) == FR_OK )
	{
		f_printf( &file ,"%s: task running...\r\n", __func__);
		f_close( &file );
	}	
	vSemaphoreCreateBinary( mLogSemaphore );
	//vSemaphoreCreateBinary( mDmaSemaphore );	

	xTaskCreate( Music_Player, (const char *)"Player", configMINIMAL_STACK_SIZE*5, NULL, tskIDLE_PRIORITY + 7, &pxMusicPlayer );
	//xTaskCreate( Read_Fatfs, (const char *)"Rfatfs", configMINIMAL_STACK_SIZE*2, NULL, tskIDLE_PRIORITY + 1, NULL );		
	xTaskCreate( usamrt_debug_task, (const char *)"Usmart", configMINIMAL_STACK_SIZE*5, NULL, tskIDLE_PRIORITY + 1, &pxUsmartTask );
	xTaskCreate( Feed_Wdg_Task, (const char *)"Wdg", configMINIMAL_STACK_SIZE*1, NULL, tskIDLE_PRIORITY + 2, NULL );	
	xTaskCreate( LED0_Task, (const char *)"LED0", configMINIMAL_STACK_SIZE*1, NULL, tskIDLE_PRIORITY + 2, NULL );
	xTaskCreate( LED1_Task, (const char *)"LED1", configMINIMAL_STACK_SIZE*1, NULL, tskIDLE_PRIORITY + 3, NULL );
	xTaskCreate( RTC_read_Task, (const char *)"RTC", configMINIMAL_STACK_SIZE*1+20, NULL, tskIDLE_PRIORITY + 4, &pxTimeTask );
	xTaskCreate( Temprate_Task, (const char *)"Temprature", configMINIMAL_STACK_SIZE*1+30, NULL, tskIDLE_PRIORITY + 5, &pxTempretureTask );		
	xTaskCreate( Key_Detect_Task, (const char *)"Key", configMINIMAL_STACK_SIZE*1, NULL, configKDETECT_TASK_PRIORITY, &pxKeyDetectTask );
	//xTaskCreate( Printf_Log_Task, (const char *)"Log", configMINIMAL_STACK_SIZE*3, NULL, configLOG_TASK_PRIORITY, &pxLogTask );	

	vTaskStartScheduler();
}

/*
*	author: yangjianzhou
* function: Software_Hardware_Init.
**/
#include "diskio.h"		/* FatFs lower layer API */
extern unsigned char xMusicVolume;
void Software_Hardware_Init( void )
{
	unsigned char res, i;
	rfifo_init( &mLogFifo );
	NVIC_PriorityGroupConfig( NVIC_PriorityGroup_2 );
	(void) uart_init( 115200 ); BOOT_LOG
	(void) RTC_INIT();
	(void) LED_Init();
	(void) EXTIX_Init();
	(void) IWDG_Init( 4, 2000 ); 
	//与分频数为64,重载值为1000,溢出时间为2s	
	(void) Adc_Init(); 
	(void) W25QXX_Init();	
	while( W25QXX_ReadID() != ( BOARD_NUM == 1 ? W25Q16 : W25Q128 ))
	{
		printf("W25Q128 Check Failed!\r\n");
		delay_ms(5000);
		printf("Please Check!      !\r\n");
		delay_ms(5000);
		LED0=!LED0;
	}	
	while( RNG_Init() )
	{
		printf("  RNG Error! RNG Trying...\r\n");	 
	}  

	/*add fatfs file system support*/
	exfuns_init();
	
 	for( i = 0; i< 3; i++ )
	{
		if( SD_Init() == 0 )
		{
			printf("SD Card Init Scueess!\r\n");
			f_mount(fs[0], "0:", 1);
			break;
		}
		printf("SD Card Error!\r\nPlease Check! \r\n");
		//delay_ms(500);					
	}	

 	res = f_mount( fs[1], "1:", 1 );
	if( res == FR_NO_FILESYSTEM )
	{
		printf("Flash Disk Formatting...\r\n");
		res = f_mkfs( "1:", 1, FLASH_BLOCK_SIZE*FLASH_SECTOR_SIZE );
		if( res == 0 )
		{
			f_setlabel( (const TCHAR *) "1:Flash" );
			printf("Flash Disk Format Finish\r\n");
			res = f_mount( fs[1], "1:", 1 );
			if( res == FR_OK )
			{
				printf("mount flash fatfs success.\r\n");
			}
			else
			{
				printf("mount flash fatfs Fail!\r\n");
			}
		}
		else 
		{
			printf("Flash Disk Format Error\r\n");
		}
	}
	
 	res = f_mount( fs[2], "2:", 1 );
	if( res == 0X0D )
	{
		printf("Ram Disk Formatting...\r\n");
		res = f_mkfs( "2:", 1, RAM_BLOCK_SIZE * RAM_SECTOR_SIZE );
		if( res == 0 )
		{
			f_setlabel( (const TCHAR *) "2:RamDisk" );
			printf("Ram Disk Format Finish\r\n");
			res = f_mount( fs[2], "2:", 1 );
			if( res == FR_OK )
			{
				printf("mount Ram fatfs success.\r\n");
			}
			else
			{
				printf("mount Ram fatfs Fail!\r\n");
			}
		}
		else 
		{
			printf("Ram Disk Format Error\r\n");
		}		
	}
	
	//mf_chdrive( "1:" );
	mf_chdrive( "2:" );	
	mf_getcwd();
	
	WM8978_Init();				//初始化WM8978
	WM8978_HPvol_Set(xMusicVolume, xMusicVolume);	//耳机音量设置
	WM8978_SPKvol_Set(50);		//喇叭音量设置	
}

void Music_Player(void * pvParameters)
{
	UBaseType_t pre;

	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );
	vSetTaskLogLevel(NULL, eLogLevel_2);
	while (1)
	{
		audio_play();
	}
}

/*
*	author: yangjianzhou
* function: Read_Fatfs.
**/
void Read_Fatfs(void * pvParameters)
{
	UBaseType_t pre;

	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("%s ...\r\n", __func__);
	vTaskPrioritySet( NULL, pre );

	/* vTaskDelay( portMAX_DELAY ) just block in max time, task will 
	 	ready to run in portMAX_DELAY tick after */
	vTaskDelay( portMAX_DELAY );
	
	while (1)
	{
		vTaskDelay( 20 / portTICK_RATE_MS );
		( void ) Fatfs_Cat_File( "0:/hello.txt" );
		printf("%s\r\n", __func__);
	}
}

/*
*	author: yangjianzhou
* function: Timer_func1. For test timers in FreeRTOS.
**/
void Timer_func1( TimerHandle_t xTimer )
{
	/*run is Timers Task context with 
		configTIMER_TASK_PRIORITY priority.*/
	printf("System timer callback in %u(ms)\r\n",
		(xTimerGetPeriod( xTimer ) / portTICK_PERIOD_MS));
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
		vTaskDelay( 8000 / portTICK_RATE_MS );
		printf("%s\r\n", __func__);
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
		vTaskDelay( 10000 / portTICK_RATE_MS );
		printf("%s\r\n", __func__);		
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
		vTaskDelay( 45000 / portTICK_RATE_MS/*portMAX_DELAY*/ );		
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
extern u8 uMusicPlayControl;
void Key_Detect_Task(void * pvParameters)
{
	int count = 0;
	unsigned int random;			
	printf("%s ...\r\n", __func__);	
	vSetTaskLogLevel(NULL, eLogLevel_0);

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
			uMusicPlayControl = 4;
		}
		
		count = 0;
		while( KEY1 == 0 )	 
		{
			count++;
			vTaskDelay(10 / portTICK_RATE_MS);	
		}		
		if( count > 80)
		{
			LED1=!LED1;
			printf("%s PE3 long press!\r\n", __func__);
			xMusicVolume -= 5;
			WM8978_HPvol_Set(xMusicVolume, xMusicVolume);			
		}
		else if( count > 0 )
		{
			LED1=!LED1;
			printf("%s PE3 press!\r\n", __func__);
			uMusicPlayControl = 1;
		}

		count = 0;
		while( KEY0 == 0 )	 
		{
			count++;
			vTaskDelay(10 / portTICK_RATE_MS);	
		}		
		if( count > 80)
		{
			LED0=!LED0;	
			LED1=!LED1;	
			printf("%s PE4 long press!\r\n", __func__);
			xMusicVolume += 5;
			WM8978_HPvol_Set(xMusicVolume, xMusicVolume);			
		}
		else if( count > 0 )
		{
			LED0=!LED0;	
			LED1=!LED1;	
			printf("%s PE4 press!\r\n", __func__);
			uMusicPlayControl = 3;
		}		
		
		random = RNG_Get_RandomNum();	
		random = random;
		//printf("random = %d\r\n", random);		
	}
}

static int xPrintfTime = 30;

/*
*	author: yangjianzhou
* function: vSetPrrintfTime.
**/
void vSetPrintfTime(int time)
{
		if( time > 0 )
		{
			xPrintfTime = time;
			printf("%s: time = %d\r\n", __func__, time);
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
/*	FIL file;
	if( f_open( &file, (const TCHAR*)"1:/log.txt", FA_WRITE|FA_OPEN_ALWAYS ) == FR_OK )
	{
		f_lseek( &file, file.fsize );
		f_printf( &file ,"%s: task running...\r\n", __func__);
		f_close( &file );
	}	
*/	printf("%s ...\r\n", __func__);	
	vSetTaskLogLevel(NULL, eLogLevel_1);	
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

const u8 TEXT_Buffer[]={"Explorer STM32F4 SPI TEST"};
#define SIZE  sizeof(TEXT_Buffer)	 
u8 datatemp[SIZE];

void saveStringToLog( const char *string )
{
	FIL file;
	if( f_open( &file, (const TCHAR*)"1:/log.txt", FA_WRITE|FA_OPEN_ALWAYS ) == FR_OK )
	{
		f_lseek( &file, file.fsize );
		f_printf( &file ,"%s", string);
		f_close( &file );
	}
	else
	{
		printf("%s: f_open file 1:/log.txt fail!\r\n", __func__ );
	}
}

void Save_String_To_Flash( unsigned int addr, const char *data )
{
	printf("Start Write W25Q16....\r\n");
	( void ) W25QXX_Write( (u8*) data, addr, strlen( data ) );
	printf("W25Q16 Write Finished!\r\n");	
}

void Read_String_From_Flash( unsigned int addr, int size )
{
	unsigned char *data = pvPortMalloc( size + 1 );
	if( data == NULL )
	{
		printf("%s: pvPortMalloc Fail!\r\n", __func__);
		return;
	}
	memset( data, '\0', size + 1);
	printf("Start Read W25Q16.... \r\n");
	W25QXX_Read( data, addr, size );
	printf("The Data Readed Is:\r\n");
	printf("%s\r\n", data);	
	vPortFree( data );
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	UBaseType_t pre;
	pre = uxTaskPriorityGet( NULL );
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("\r\n%s -> [%s] , please add stack for the Task!\r\n\r\n", __func__, pcTaskName);		
	vTaskPrioritySet( NULL, pre );
}

void Printf_Application_Version( void )
{
		BOOT_LOG
}


