#include "sys.h" 	
#include "led.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "timer.h"
#include "queue.h"
#include "xmalloc.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 2016 *****/ \
\r\n/******************************************/ \
\r\n");

TimerHandle_t handleT, handleT1;
QueueHandle_t xQueue;
xTaskHandle  pTaskHandler;

void vTaskGetRunTimeStats(char *pcWriteBuffer )  
{  
   TaskStatus_t *pxTaskStatusArray = NULL;  
   volatile UBaseType_t uxArraySize, x;  
   uint32_t ulTotalRunTime, ulStatsAsPercentage;  
   
   /* 防御性代码，确保字符串有合理的结束*/  
   *pcWriteBuffer = 0x00;  
   
   /* 获取任务总数目*/  
   uxArraySize = uxTaskGetNumberOfTasks();  
/*   if (uxArraySize > 6) {
   		printf("uxArraySize > 6 error !!!\r\n");
		return;
   	}
*/   /*为每个任务的TaskStatus_t结构体分配内存，也可以静态的分配一个足够大的数组 */  
   if (!pxTaskStatusArray)
   		pxTaskStatusArray = xmalloc(0, uxArraySize * sizeof( TaskStatus_t ));  

   printf("pxTaskStatusArray=%p, xmem_used_pecent=%d%%\r\n", pxTaskStatusArray, xmem_used_pecent(0));
   if(pxTaskStatusArray != NULL )  
   {  
      /*获取每个任务的状态信息 */  
     uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );  
   
      /* 百分比计算 */  
     ulTotalRunTime /= 100UL;  
   
      /* 避免除零错误 */  
      if(ulTotalRunTime > 0 )  
      {  
         /* 将获得的每一个任务状态信息部分的转化为程序员容易识别的字符串格式*/  
        for( x = 0; x < uxArraySize; x++ )  
         {  
           /* 计算任务运行时间与总运行时间的百分比。*/  
           ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter /ulTotalRunTime;  
   
           if( ulStatsAsPercentage > 0UL )  
           {  
              sprintf(pcWriteBuffer, "%s\t\t%lu\t\t%lu%%\r\n",  
                                pxTaskStatusArray[ x ].pcTaskName,  
                                pxTaskStatusArray[ x ].ulRunTimeCounter,  
                                ulStatsAsPercentage );  
           }  
           else  
           {  
              /* 任务运行时间不足总运行时间的1%*/  
              sprintf(pcWriteBuffer, "%s\t\t%lu\t\t< 1%%\r\n",  
                                pxTaskStatusArray[ x ].pcTaskName,  
                                 pxTaskStatusArray[x ].ulRunTimeCounter );  
           }  
   
           pcWriteBuffer += strlen(pcWriteBuffer);  
         }  
      }  
   
      /* 释放之前申请的内存*/  
     //vPortFree( pxTaskStatusArray );  
     xfree(0, pxTaskStatusArray);
   }  
}  

void xTimerFunc0( TimerHandle_t xTimer )
{
	UBaseType_t xFreeStack;
	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();  
	char *pcWriteBuffer = (char *) xmalloc(0, uxArraySize*90);
	printf("%s\r\n", __func__);

	xFreeStack = uxTaskGetStackHighWaterMark(NULL);
	printf("%s xFreeStack = %ld", __func__, xFreeStack);

	if (pcWriteBuffer) {
		vTaskGetRunTimeStats(pcWriteBuffer);
		printf("pcWriteBuffer\r\n%s", pcWriteBuffer);
		xfree(0, pcWriteBuffer);
		printf("xmem_used_pecent=%d%%\r\n", xmem_used_pecent(0));
	} else {
		printf("xmalloc pcWriteBuffer fail!\r\n");
	}
		//vTaskDelay(500);
}

void xTimerFunc1( TimerHandle_t xTimer )
{
	UBaseType_t xFreeStack;

	printf("%s\r\n", __func__);
	xFreeStack = uxTaskGetStackHighWaterMark(NULL);
	printf("%s xFreeStack = %ld", __func__, xFreeStack);		
	//vTaskDelay(500);
}

void xFirstTask(void *argc)
{
	UBaseType_t xFreeStack;
	UBaseType_t num;
	portTickType xLastWakeTime;
	argc = argc;

	printf("%s enter\r\n", __func__);
	xLastWakeTime = xTaskGetTickCount();
	while(1) {
		//vTaskDelay(5000/portTICK_PERIOD_MS);
		xFreeStack = uxTaskGetStackHighWaterMark(NULL);
		printf("%s xFreeStack = %ld", __func__, xFreeStack);		
		vTaskDelayUntil(&xLastWakeTime, 15000/portTICK_PERIOD_MS);
		printf("%s, hi... xPortGetFreeHeapPercent=%f%%\r\n",
			__func__, xPortGetFreeHeapPercent());
		//num = uxTaskGetNumberOfTasks();
	}
}

 void vApplicationIdleHook(void)
{
	UBaseType_t xFreeStack;
	static UBaseType_t xIdleCountTotal = 0;
	xIdleCountTotal++;
	if (xIdleCountTotal%1500000 == 0) {
		xFreeStack = uxTaskGetStackHighWaterMark(NULL);
		printf("%s xFreeStack = %ld", __func__, xFreeStack);
		
		printf("%s xIdleCountTotal=%ld, xTaskGetTickCount=%d\r\n", 
			__func__, xIdleCountTotal, xTaskGetTickCount());
	}
}

static void vSenderTask( void *pvParameters ) { 
	long lValueToSend; 
	portBASE_TYPE xStatus;  
	UBaseType_t xFreeStack;

	lValueToSend = ( long ) pvParameters;  

	for( ;; ) { 		
		vTaskDelay(100*lValueToSend/portTICK_PERIOD_MS);
		xFreeStack = uxTaskGetStackHighWaterMark(NULL);
		printf("%s xFreeStack = %ld", __func__, xFreeStack);
		
		xStatus = xQueueSendToBack(xQueue, &lValueToSend, 0); 
		if( xStatus != pdPASS ) 
		{
			printf( "Could not send to the queue.\r\n" ); 
		}  
		//taskYIELD(); 
	} 
		
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

static void vReceiverTask( void *pvParameters ) {
	UBaseType_t xFreeStack;
	long lReceivedValue; 
	portBASE_TYPE xStatus; 
	const portTickType xTicksToWait = portMAX_DELAY;//100 / portTICK_RATE_MS;  

	for( ;; ) { 
		if( uxQueueMessagesWaiting( xQueue ) != 0 ) { 
			printf( "Queue should have been empty!\r\n" ); 
		}  
		xFreeStack = uxTaskGetStackHighWaterMark(NULL);
		printf("%s xFreeStack = %ld", __func__, xFreeStack);		
			//uxQueueMessagesWaiting();
			//xQueuePeek( xQueue, &lReceivedValue, xTicksToWait);
			// Receive an item from a queue without removing the item from the queue.			
			xStatus = xQueueReceive( xQueue, &lReceivedValue, xTicksToWait); 
			if( xStatus == pdPASS ) {
				printf( "Received = %ld\r\n", lReceivedValue ); 
			} else {
				printf( "Could not receive from the queue.\r\n" ); } 
		} 
} 

 void vTask2( void *pvParameters ) { 
 	/* 任务2什么也没做，只是删除自己。
 	删除自己可以传入NULL值，这里为了演示，
 	还是传入其自己的句柄。 */ 
 	printf( "Task2 is running and about to delete itself\r\n" ); 
	//vTaskDelete( xTask2Handle ); 
	
	vTaskDelete( NULL ); 
 } 

 void vTask1( void *pvParameters ) 
 { 
 	const portTickType xDelay100ms = 10000 / portTICK_RATE_MS; 
	
	for( ;; ) { 
		/* Print out the name of this task. */ 
		printf( "Task1 is running\r\n" );  
		/* 创建任务2为高优先级。 */
		xTaskCreate( vTask2, "Task 2", 1000, NULL, 2, NULL ); 
		/* The task handle is the last parameter _____^^^^^^^^^^^^^ */	
		/* 因为任务2具有高优先级，所以任务1运行到这
		里时，任务2已经完成执行，删除了自己。
		任务1得以 执行，延迟100ms */ 
		vTaskDelay( xDelay100ms ); 
 	} 
 }

 int main(void)
 {  
		BaseType_t xReturn;
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	    timer3_int_init(10-1, 720-1);
		uarts_init();
		BOOT_LOG
		printf("FreeRTOS start...\r\n");


	 	//vTaskDelete( TaskHandle_t xTask);
	 	//vTaskPrioritySet( TaskHandle_t xTask, UBaseType_t uxNewPriority );
	 	//unsigned portBASE_TYPE uxTaskPriorityGet( xTaskHandle pxTask )
		xReturn = xTaskCreate(xFirstTask,
								"fTask", configMINIMAL_STACK_SIZE,
								( void * ) NULL,
								1,
								&pTaskHandler);
		/* Pass the handle out in an anonymous way.  pTaskHandler handle can be used to
		change the created task's priority, delete the created task, etc.*/

		
		if (xReturn == pdPASS) {
				printf("xFirstTask create  success...\r\n");			
		} else {
				printf("xFirstTask create fail.\r\n");
		}
		
		handleT =  xTimerCreate("timer0", 13000/portTICK_PERIOD_MS, pdTRUE, "hh", xTimerFunc0);
		if (handleT != NULL) {
				printf("xTimerCreate timer0 success...\r\n");
				xTimerStart(handleT, 20000/portTICK_PERIOD_MS);			
		} else {
				printf("xTimerCreate fail...\r\n");
		}
		
		handleT1 =  xTimerCreate("timer1", 11000/portTICK_PERIOD_MS, pdTRUE, "dd", xTimerFunc1);
		if (handleT1 != NULL) {
				printf("xTimerCreate timer1 success...\r\n");
				xTimerStart(handleT1, 20000/portTICK_PERIOD_MS);			
		} else {
				printf("xTimerCreate fail...\r\n");
		}
		xQueue = xQueueCreate(5, sizeof(long)); 
		if( xQueue != NULL ) { 
			xTaskCreate( vSenderTask, "Send1", configMINIMAL_STACK_SIZE, ( void * ) 100, 3, NULL ); 
			xTaskCreate( vSenderTask, "Send2", configMINIMAL_STACK_SIZE, ( void * ) 200, 2, NULL );  
			xTaskCreate( vReceiverTask, "Receiver", configMINIMAL_STACK_SIZE, NULL, 4, NULL );  
			vTaskStartScheduler(); 
		} else {
		}
		
		/* Start the scheduler. */
		vTaskStartScheduler();
}








