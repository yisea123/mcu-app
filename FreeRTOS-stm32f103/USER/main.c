#include "sys.h" 	
#include "led.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include <stm32f10x.h>
#include "timer.h"
#include "queue.h"
#include "xmalloc.h"
#include "semphr.h"
#include "keydetect.h"
#include "usmart.h"
#include <stdio.h>
#include "rfifo.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 2016 *****/ \
\r\n/******************************************/ \
\r\n\r\n");

TimerHandle_t handleT, handleT1, keyTimer;
QueueHandle_t xQueue;
xSemaphoreHandle xSemaphore;
xSemaphoreHandle xSemaphoreC;
QueueHandle_t xSeamaphoreMutex;
xTaskHandle  pFirstTask;
xTaskHandle pxUsmartTask;
xTaskHandle pxModuleTask;

extern void HandleModuleTask( void * pvParameters );
extern unsigned char xmem_used_pecent(unsigned char memx);

void xTimerFunc( TimerHandle_t xTimer )
{
 	printf("%s  enter\r\n", __func__);
}

 void vApplicationIdleHook(void)
{

}

 int main(void)
 {  
	BaseType_t xReturn;
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	uarts_init();
	BOOT_LOG
	printf("\r\nFreeRTOS start...\r\n");

	xReturn = xTaskCreate(usamrt_debug_task,
							"Usmar", configMINIMAL_STACK_SIZE*2,
							( void * ) NULL,
							4,
							&pxUsmartTask);
	
	if (xReturn == pdPASS) {
		printf("debug_task create  success...\r\n");			
	} else {
		printf("debug_task create fail.\r\n");
	}

	xReturn = xTaskCreate(HandleModuleTask,
							"longsung", configMINIMAL_STACK_SIZE*4,
							( void * ) NULL,
							3,
							&pxModuleTask);
	
	if (xReturn == pdPASS) {
		printf("longsung create  success...\r\n");			
	} else {
		printf("longsung create fail.\r\n");
	}

/*	xReturn = xTaskCreate(led_flash_task,
							"led", configMINIMAL_STACK_SIZE,
							( void * ) NULL,
							tskIDLE_PRIORITY + 2,
							NULL);
	
	if (xReturn == pdPASS) {
		printf("led_flash_task create  success...\r\n");			
	} else {
		printf("led_flash_task create fail.\r\n");
	}
*/
	/*
	xReturn = xTaskCreate(keyevent_detect_task,
							"key", configMINIMAL_STACK_SIZE,
							( void * ) NULL,
							tskIDLE_PRIORITY + 1,
							NULL);
	
	if (xReturn == pdPASS) {
		printf("keyevent_detect_task create  success...\r\n");			
	} else {
		printf("keyevent_detect_task create fail.\r\n");
	}
	*/
	
/*	handleT =  xTimerCreate("timer0", 
							  5000/portTICK_PERIOD_MS, 
							  pdTRUE, 
							  "hh", 
							  xTimerFunc);
	if (handleT != NULL) {
		printf("xTimerCreate timer0 success...\r\n");
		xTimerStart(handleT, 2000/portTICK_PERIOD_MS);			
	} else {
		printf("xTimerCreate fail...\r\n");
	}
*/		
	/* Start the scheduler. */
	vTaskStartScheduler();
}

void vApplicationStackOverflowHook( TaskHandle_t xTask, signed char *pcTaskName )
{
	UBaseType_t pre;
	pre = uxTaskPriorityGet( NULL );
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
	printf("\r\n%s -> [%s] , please add stack for the Task!\r\n\r\n", __func__, pcTaskName);		
	vTaskPrioritySet( NULL, pre );
}

























/* 允许其它发送任务执行。 
taskYIELD()通知调度器现在就切换到其它任务，
而不必等到本任务的时 间片耗尽

某个任务调用 taskYIELD()等效于其自愿
放弃运行态。由于本例中两个写队列任务具有相同的
任务优先级，所以一旦其中一个任
务调用了 taskYIELD()，另一个任务将会得到执行 — 调用
taskYIELD()的任务转移到
就绪态，同时另一个任务进入运行态。



uxTaskGetStackHighWaterMark()



xSemaphoreHandle xSemaphore ;
把二值信号量

void vSemaphoreCreateBinary( xSemaphoreHandle xSemaphore ); 

xSemaphoreGive( xSemaphore ); 如果队列满了，返回errQUEUE_FULL

但 xSemaphoreTake(xSemaphore, 1000/portTICK_PERIOD_MS)
不能在中断服务例程中调用。

portBASE_TYPE pxHigherPriorityTaskWoken;
xSemaphoreGiveFromISR(xSemaphore, &pxHigherPriorityTaskWoken)
如果调用 xSemaphoreGiveFromISR()使得一个任
务解除阻塞，并且这个任务的优先级高于当前任务
(也就是被中断的任务)，那么 xSemaphoreGiveFromISR()会
在函数内部将*pxHigherPriorityTaskWoken 设为
pdTRUE。 

如果 xSemaphoreGiveFromISR() 将此值设为
pdTRUE，则在中断退出前应当进行一次上下文切换。
这样才能保证中断直接返回到就绪态任务中优先级
高的任务中






信号量在使用前必须先被创建。使用 
xSemaphoreCreateCounting( unsigned portBASE_TYPE uxMaxCount,
unsigned portBASE_TYPE uxInitialCount) API 函数来创
建一个计数信号量。 

 在信号量使用之前必须先创建。本例中创建了一个计
数信号量。此信号量的大计数值为10，初始计数值为0 
xCountingSemaphore = xSemaphoreCreateCounting( 10, 0 )



xSemaphoreCreateMutex
用于互斥的信号量必须归还。 
用于同步的信号量通常是完成同步

之后便丢弃，不再归还

 信号量使用前必须先创建。本例创建了一个互斥量
类型的信号量。  
xMutex = xSemaphoreCreateMutex();

试图获得互斥量。如果互斥量无效，则将阻塞，
进入无超时等待。xSemaphoreTake()只可能在成功获得
互 斥量后返回，所以无需检测返回值。如果指定
了等待超时时间，则代码必须检测到xSemaphoreTake()
返回 pdTRUE后，才能访问共享资源(此处是指标准输出)。

 xSemaphoreTake( xMutex, portMAX_DELAY ); 
 {
  程序执行到这里表示已经成功持有互斥量。
 现在可以自由访问标准输出，因为任意时刻只会
 有一个任 务能持有互斥量。 
 printf( "%s", pcString ); 
 互斥量必须归还! 
} 
xSemaphoreGive( xMutex ); 

*/ 


/*
static void vSenderTask( void *pvParameters ) 
{ 
	long lValueToSend; 
	portBASE_TYPE xStatus;  

	lValueToSend = ( long ) pvParameters;  

	for( ;; ) { 
	
		vTaskDelay(1000*(lValueToSend-lValueToSend/20)/portTICK_PERIOD_MS);
		
		xStatus = xQueueSendToBack(xQueue, &lValueToSend, 0); 
		if( xStatus != pdPASS ) 
		{
			printf( "Could not send to the queue.\r\n" ); 
		} else {
			printf("%s %ld send ok\r\n", __func__, lValueToSend); 

			}
		//taskYIELD(); 
	} 
		
}

static void vReceiverTask( void *pvParameters ) {
	long lReceivedValue; 
	portBASE_TYPE xStatus; 
	const portTickType xTicksToWait = portMAX_DELAY;//100 / portTICK_RATE_MS;  

	for( ;; ) { 
		if( uxQueueMessagesWaiting( xQueue ) != 0 ) { 
			printf( "Queue should have been empty!\r\n" ); 
		}  
			//uxQueueMessagesWaiting();
			//xQueuePeek( xQueue, &lReceivedValue, xTicksToWait);
			// Receive an item from a queue without removing the item from the queue.			
			xStatus = xQueueReceive( xQueue, &lReceivedValue, xTicksToWait); 
			if( xStatus == pdPASS ) {
				printf( "Received = %ld\r\n", lReceivedValue ); 
			} else {
				printf( "Could not receive from the queue.\r\n" ); 
			} 
		} 
} 

*/





