#include "sys.h" 	
#include "led.h"
#include "uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "timer.h"
#include "queue.h"
#include "xmalloc.h"
#include "semphr.h"

#define BOOT_LOG   \
printf(" \
\r\n\r\n/******       MCU   -   START        ******/ \
\r\n/***** YANGJIANZHOU AT GUANGZHOU 2016 *****/ \
\r\n/******************************************/ \
\r\n\r\n");

TimerHandle_t handleT, handleT1;
QueueHandle_t xQueue;
xSemaphoreHandle xSemaphore;
xSemaphoreHandle xSemaphoreC;

QueueHandle_t xSeamaphoreMutex;

xTaskHandle  pTaskHandler;

char *getTaskStatus(eTaskState status)
{
	switch (status) {
		case eRunning :	return "Running";/* A task is querying the state of itself, so must be running. */
		case eReady: return "Ready"; 		/* The task being queried is in a read or pending ready list. */
		case eBlocked: return "Block";		/* The task being queried is in the Blocked state. */
		case eSuspended: return "Suspend"; 	/* The task being queried is in the Suspended state, or is in the Blocked state with an infinite time out. */
		case eDeleted: return "Delete";		/* The task being queried has been deleted, but its TCB has not yet been freed. */
		case eInvalid: return "Invalid";			/* Used as an 'invalid state' value. */

		default: return "Unknow";
	}
}

void vTaskGetRunTimeStats(char *pcWriteBuffer )  
{  
   TaskStatus_t *pxTaskStatusArray = NULL;  
   volatile UBaseType_t uxArraySize, x;  
   uint32_t ulTotalRunTime, ulStatsAsPercentage;  
   
   *pcWriteBuffer = 0x00;  
   
   uxArraySize = uxTaskGetNumberOfTasks();  
 
   if (!pxTaskStatusArray)
   		pxTaskStatusArray = xmalloc(0, uxArraySize * sizeof( TaskStatus_t ));  

   printf("pxTaskStatusArray=%p, xmem_used_pecent=%d%%\r\n", pxTaskStatusArray, xmem_used_pecent(0));
   if(pxTaskStatusArray != NULL )  
   {  
     uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );  
   
     ulTotalRunTime /= 100UL;  
   
      if(ulTotalRunTime > 0 )  
      {  
         
        for( x = 0; x < uxArraySize; x++ )  
         {  
           ulStatsAsPercentage = pxTaskStatusArray[ x ].ulRunTimeCounter /ulTotalRunTime;  
   
           if( ulStatsAsPercentage > 0UL )  
           {  
              sprintf(pcWriteBuffer, "[%s\t%s\t%01d\t%.01f%%\t%d\t%lu\t%lu%%]\r\n",  
                                pxTaskStatusArray[ x ].pcTaskName, 
                                getTaskStatus(pxTaskStatusArray[ x ].eCurrentState),
                                pxTaskStatusArray[ x ].uxCurrentPriority,
                                pxTaskStatusArray[ x ].uStackFreePer,
								pxTaskStatusArray[ x ].xStackSize/4,
                                pxTaskStatusArray[ x ].ulRunTimeCounter,  
                                ulStatsAsPercentage );  
           }  
           else  
           {  
              sprintf(pcWriteBuffer, "[%s\t%s\t%01d\t%.01f%%\t%d\t%lu\t< 1%%]\r\n",  
                                pxTaskStatusArray[ x ].pcTaskName,  
                                getTaskStatus(pxTaskStatusArray[ x ].eCurrentState),
                                pxTaskStatusArray[ x ].uxCurrentPriority,
                                pxTaskStatusArray[ x ].uStackFreePer,  
                                pxTaskStatusArray[ x ].xStackSize/4,
                                pxTaskStatusArray[x ].ulRunTimeCounter );  
           }  
   
           pcWriteBuffer += strlen(pcWriteBuffer);  
         }  
      }  
   
     //vPortFree( pxTaskStatusArray );  
     xfree(0, pxTaskStatusArray);
   }  
}  

void xTimerFunc0( TimerHandle_t xTimer )
{
	UBaseType_t uxArraySize = uxTaskGetNumberOfTasks();  
	char *pcWriteBuffer = (char *) xmalloc(0, uxArraySize*90);
	printf("$$$$$$$$$$%s$$$$$$$$$$\r\n", __func__);

	if (pcWriteBuffer) {
		xSemaphoreGive(xSemaphore);	
		xSemaphoreGive(xSemaphore);
		xSemaphoreGive(xSemaphore);
		xSemaphoreGive(xSemaphoreC);	
		xSemaphoreGive(xSemaphoreC);
		xSemaphoreGive(xSemaphoreC);		
		vTaskGetRunTimeStats(pcWriteBuffer);
		printf("System Task Infomation:\r\n%s", pcWriteBuffer);
		xfree(0, pcWriteBuffer);
		printf("xmem_used_pecent=%d%%\r\n", xmem_used_pecent(0));
	} else {
		printf("xmalloc pcWriteBuffer fail!\r\n");
	}
		//vTaskDelay(500);
}

void vApplicationIdleHook(void);

void xTimerFunc1( TimerHandle_t xTimer )
{
	char rr[100];

	sprintf(rr, " %s ", __func__);
	printf("****************%s*************\r\n", __func__, rr);
	//xTimerFunc0(NULL);
	//vApplicationIdleHook();
	//vTaskDelay(500);
}

void xFirstTask(void *argc)
{
	UBaseType_t num;
	portTickType xLastWakeTime;
	argc = argc;

 	printf("%s  enter\r\n", __func__);
	
	xLastWakeTime = xTaskGetTickCount();
	while(1) {
		//vTaskDelay(5000/portTICK_PERIOD_MS);
		vTaskDelayUntil(&xLastWakeTime, 5000/portTICK_PERIOD_MS);
		printf("%s, hi... xPortGetFreeHeapPercent=%f%%\r\n",
			__func__, xPortGetFreeHeapPercent());
		//num = uxTaskGetNumberOfTasks();
	}
}

 void vApplicationIdleHook(void)
{
	char uStack[60];
	char *p;
	static UBaseType_t xIdleCountTotal = 0;
	UBaseType_t t;
	TickType_t c;
	
	xIdleCountTotal++;
	t = xIdleCountTotal*(xIdleCountTotal) + xIdleCountTotal%3;

	c = xTaskGetTickCount();

	if (c%1500 == 0)
		printf("OOOOOOOOOOOOOOOOOOOOOOOOOOOO tick count = %ld, t3=%ld\r\n",
			c, portGET_RUN_TIME_COUNTER_VALUE());
	
	p = xmalloc(0, (xIdleCountTotal%(1024*5)+1));
	if (!p) {
		printf("%s: xmalloc %d bytes fail!\r\n", __func__, 
			xIdleCountTotal%(1024*5)+1);
		while (1) ;
	} else {
		xfree(0, p);
	}
	
	if (xIdleCountTotal%2000000 == 0) {
		sprintf(uStack, "%s:%ld", __func__, xIdleCountTotal);
		printf("%s\r\n", uStack);
		//printf("%s xIdleCountTotal=%ld, xTaskGetTickCount=%d\r\n", 
		//	__func__, xIdleCountTotal, xTaskGetTickCount());
	}

	//vTaskDelay(100/portTICK_PERIOD_MS);
}

static void vSenderTask( void *pvParameters ) { 
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


 void vTask2( void *pvParameters );
 void vTask1( void *pvParameters );
 int main(void)
 {  
		BaseType_t xReturn;
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	  timer3_int_init(5-1, 3600);//0.25ms   250us
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
		
		handleT =  xTimerCreate("timer0", 5200/portTICK_PERIOD_MS, pdTRUE, "hh", xTimerFunc0);
		if (handleT != NULL) {
				printf("xTimerCreate timer0 success...\r\n");
				xTimerStart(handleT, 20000/portTICK_PERIOD_MS);			
		} else {
				printf("xTimerCreate fail...\r\n");
		}
		
/*		handleT1 =  xTimerCreate("timer1", 10000/portTICK_PERIOD_MS, pdTRUE, "dd", xTimerFunc1);
		if (handleT1 != NULL) {
				printf("xTimerCreate timer1 success...\r\n");
				xTimerStart(handleT1, 40000/portTICK_PERIOD_MS);			
		} else {
				printf("xTimerCreate fail...\r\n");
		}
*/		xQueue = xQueueCreate(5, sizeof(long)); 
		if( xQueue != NULL ) { 
			//xTaskCreate( vSenderTask, "Send1", configMINIMAL_STACK_SIZE, ( void * ) 100, 3, NULL ); 
			xTaskCreate( vSenderTask, "Send2", configMINIMAL_STACK_SIZE, ( void * ) 200, 2, NULL );  
			xTaskCreate( vReceiverTask, "Recv", configMINIMAL_STACK_SIZE, NULL, 4, NULL );  
			//vTaskStartScheduler(); 
		} else {
		}

		vSemaphoreCreateBinary(xSemaphore);
		if (xSemaphore == NULL) {
			printf("xSemaphore create error!\r\n");
		} else {
			xTaskCreate( vTask2, "vTask2", configMINIMAL_STACK_SIZE, 
				NULL, 3, NULL );  
		}

		xSemaphoreC = xSemaphoreCreateCounting(10, 0);
		if (xSemaphoreC == NULL) {
			printf("xSemaphoreC create error!\r\n");
		} else {
			xTaskCreate( vTask1, "vTask1", configMINIMAL_STACK_SIZE, 
				NULL, 5, NULL );  
		}		

		xSeamaphoreMutex = xSemaphoreCreateMutex();
		if (xSeamaphoreMutex == NULL) {
			printf("xSeamaphoreMutex create error!\r\n");
		} else {
			printf("xSeamaphoreMutex create scueess!\r\n");
			}
		/* Start the scheduler. */
		vTaskStartScheduler();
}


void vTask2( void *pvParameters ) { 
   /* ����2ʲôҲû����ֻ��ɾ���Լ���
   ɾ���Լ����Դ���NULLֵ������Ϊ����ʾ��
   ���Ǵ������Լ��ľ���� */ 
   static int i = 0;

   while (1) {
	   i++;
	   xSemaphoreTake(xSeamaphoreMutex, portMAX_DELAY);
	   printf( "Task2 xSeamaphoreMutex wake up , i = %d\r\n", i); 
	   vTaskDelay(1000/portTICK_PERIOD_MS);
	   xSemaphoreGive(xSeamaphoreMutex);
	   //vTaskDelete( xTask2Handle ); 
   }
   //vTaskDelete( NULL ); 
} 

void vTask1( void *pvParameters ) 
{ 
   const portTickType xDelay100ms = 10000 / portTICK_RATE_MS; 
   
   for( ;; ) { 
   		static int i = 0;
		i++;
		xSemaphoreTake(xSeamaphoreMutex, portMAX_DELAY);
		printf( "Task1 xSeamaphoreMutex wake up , i = %d\r\n", i); 
		vTaskDelay(1000/portTICK_PERIOD_MS);
		xSemaphoreGive(xSeamaphoreMutex);
		vTaskDelay(200/portTICK_PERIOD_MS);

	   /* Print out the name of this task. */ 
	  // printf( "Task1 is running\r\n" );  
	   /* ��������2Ϊ�����ȼ��� */
	  // xTaskCreate( vTask2, "Task 2", 1000, NULL, 2, NULL ); 
	   /* The task handle is the last parameter _____^^^^^^^^^^^^^ */  
	   /* ��Ϊ����2���и����ȼ�����������1���е���
	   ��ʱ������2�Ѿ����ִ�У�ɾ�����Լ���
	   ����1���� ִ�У��ӳ�100ms */ 
	  // vTaskDelay( xDelay100ms ); 
   } 
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






