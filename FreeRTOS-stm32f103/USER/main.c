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
   
   /* �����Դ��룬ȷ���ַ����к���Ľ���*/  
   *pcWriteBuffer = 0x00;  
   
   /* ��ȡ��������Ŀ*/  
   uxArraySize = uxTaskGetNumberOfTasks();  
/*   if (uxArraySize > 6) {
   		printf("uxArraySize > 6 error !!!\r\n");
		return;
   	}
*/   /*Ϊÿ�������TaskStatus_t�ṹ������ڴ棬Ҳ���Ծ�̬�ķ���һ���㹻������� */  
   if (!pxTaskStatusArray)
   		pxTaskStatusArray = xmalloc(0, uxArraySize * sizeof( TaskStatus_t ));  

   printf("pxTaskStatusArray=%p, xmem_used_pecent=%d%%\r\n", pxTaskStatusArray, xmem_used_pecent(0));
   if(pxTaskStatusArray != NULL )  
   {  
      /*��ȡÿ�������״̬��Ϣ */  
     uxArraySize = uxTaskGetSystemState( pxTaskStatusArray, uxArraySize, &ulTotalRunTime );  
   
      /* �ٷֱȼ��� */  
     ulTotalRunTime /= 100UL;  
   
      /* ���������� */  
      if(ulTotalRunTime > 0 )  
      {  
         /* ����õ�ÿһ������״̬��Ϣ���ֵ�ת��Ϊ����Ա����ʶ����ַ�����ʽ*/  
        for( x = 0; x < uxArraySize; x++ )  
         {  
           /* ������������ʱ����������ʱ��İٷֱȡ�*/  
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
              /* ��������ʱ�䲻��������ʱ���1%*/  
              sprintf(pcWriteBuffer, "%s\t\t%lu\t\t< 1%%\r\n",  
                                pxTaskStatusArray[ x ].pcTaskName,  
                                 pxTaskStatusArray[x ].ulRunTimeCounter );  
           }  
   
           pcWriteBuffer += strlen(pcWriteBuffer);  
         }  
      }  
   
      /* �ͷ�֮ǰ������ڴ�*/  
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
 	/* ����2ʲôҲû����ֻ��ɾ���Լ���
 	ɾ���Լ����Դ���NULLֵ������Ϊ����ʾ��
 	���Ǵ������Լ��ľ���� */ 
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
		/* ��������2Ϊ�����ȼ��� */
		xTaskCreate( vTask2, "Task 2", 1000, NULL, 2, NULL ); 
		/* The task handle is the last parameter _____^^^^^^^^^^^^^ */	
		/* ��Ϊ����2���и����ȼ�����������1���е���
		��ʱ������2�Ѿ����ִ�У�ɾ�����Լ���
		����1���� ִ�У��ӳ�100ms */ 
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








