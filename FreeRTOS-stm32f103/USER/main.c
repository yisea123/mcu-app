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

























/* ����������������ִ�С� 
taskYIELD()֪ͨ���������ھ��л�����������
�����صȵ��������ʱ ��Ƭ�ľ�

ĳ��������� taskYIELD()��Ч������Ը
��������̬�����ڱ���������д�������������ͬ��
�������ȼ�������һ������һ����
������� taskYIELD()����һ�����񽫻�õ�ִ�� �� ����
taskYIELD()������ת�Ƶ�
����̬��ͬʱ��һ�������������̬��



uxTaskGetStackHighWaterMark()



xSemaphoreHandle xSemaphore ;
�Ѷ�ֵ�ź���

void vSemaphoreCreateBinary( xSemaphoreHandle xSemaphore ); 

xSemaphoreGive( xSemaphore ); ����������ˣ�����errQUEUE_FULL

�� xSemaphoreTake(xSemaphore, 1000/portTICK_PERIOD_MS)
�������жϷ��������е��á�

portBASE_TYPE pxHigherPriorityTaskWoken;
xSemaphoreGiveFromISR(xSemaphore, &pxHigherPriorityTaskWoken)
������� xSemaphoreGiveFromISR()ʹ��һ����
�������������������������ȼ����ڵ�ǰ����
(Ҳ���Ǳ��жϵ�����)����ô xSemaphoreGiveFromISR()��
�ں����ڲ���*pxHigherPriorityTaskWoken ��Ϊ
pdTRUE�� 

��� xSemaphoreGiveFromISR() ����ֵ��Ϊ
pdTRUE�������ж��˳�ǰӦ������һ���������л���
�������ܱ�֤�ж�ֱ�ӷ��ص�����̬���������ȼ�
�ߵ�������






�ź�����ʹ��ǰ�����ȱ�������ʹ�� 
xSemaphoreCreateCounting( unsigned portBASE_TYPE uxMaxCount,
unsigned portBASE_TYPE uxInitialCount) API ��������
��һ�������ź����� 

 ���ź���ʹ��֮ǰ�����ȴ����������д�����һ����
���ź��������ź����Ĵ����ֵΪ10����ʼ����ֵΪ0 
xCountingSemaphore = xSemaphoreCreateCounting( 10, 0 )



xSemaphoreCreateMutex
���ڻ�����ź�������黹�� 
����ͬ�����ź���ͨ�������ͬ��

֮��㶪�������ٹ黹

 �ź���ʹ��ǰ�����ȴ���������������һ��������
���͵��ź�����  
xMutex = xSemaphoreCreateMutex();

��ͼ��û������������������Ч����������
�����޳�ʱ�ȴ���xSemaphoreTake()ֻ�����ڳɹ����
�� �����󷵻أ����������ⷵ��ֵ�����ָ��
�˵ȴ���ʱʱ�䣬���������⵽xSemaphoreTake()
���� pdTRUE�󣬲��ܷ��ʹ�����Դ(�˴���ָ��׼���)��

 xSemaphoreTake( xMutex, portMAX_DELAY ); 
 {
  ����ִ�е������ʾ�Ѿ��ɹ����л�������
 ���ڿ������ɷ��ʱ�׼�������Ϊ����ʱ��ֻ��
 ��һ���� ���ܳ��л������� 
 printf( "%s", pcString ); 
 ����������黹! 
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





