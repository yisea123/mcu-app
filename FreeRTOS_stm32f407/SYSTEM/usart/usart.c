#include "sys.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rfifo.h"
#include "usart.h"
#include "semphr.h"
#include "w25qxx.h" 


//�������´���,֧��printf����,������Ҫѡ��use MicroLIB	  
#if 1
//#pragma import(__use_no_semihosting)             
//��׼����Ҫ��֧�ֺ���                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//����_sys_exit()�Ա���ʹ�ð�����ģʽ    
_sys_exit(int x) 
{ 
	x = x; 
} 

extern Ringfifo mLogFifo;
extern QueueHandle_t mLogSemaphore;
extern TaskHandle_t pxUsmartTask;

#if( INCLUDE_xTaskLogLevel == 1 )
/*0~8*/
eLogLevel ucOsLogLevel = eLogLevel_4;
#endif

//�ض���fputc���� 
int fputc(int ch, FILE *f)
{ 	
	static unsigned char pre = 0;

#if( INCLUDE_xTaskLogLevel == 1 )
	if( ucGetTaskLogLevel( NULL ) > ucOsLogLevel )
	{
		return ch;
	}
#endif

	if( 1 /*xTaskGetSchedulerState() != taskSCHEDULER_RUNNING*/ )
	{
#if( BOARD_NUM != 3)	
		while((USART1->SR&0X40)==0);//ѭ������,ֱ���������   
		USART1->DR = (u8) ch;  
#else
	while((UART4->SR&0X40)==0);//ѭ������,ֱ���������   
	UART4->DR = (u8) ch;  
#endif
	} 
	else if( portIsInInterrupt() ) 
	{
		/*in interrupt context*/
		if( rfifo_put( &mLogFifo, &ch, 1 ) != 1 ) 
		{
			/*we may lost byte printf in interrupt*/
			mLogFifo.lostBytes++;
			xSemaphoreGiveFromISR( mLogSemaphore, NULL );	
			pre = 0;
		} 
		else 
		{
			if( pre == 0x0d && ch == 0x0a ) 
			{
				///* for debug interrupt
				int i;
				char mSendBuffer[30];
				mSendBuffer[0] = 'i';
				mSendBuffer[1] = 'n';
				mSendBuffer[2] = 't';
				mSendBuffer[3] = ':';
				mSendBuffer[4] = ' ';
				for( i = 0; i<5; i++ ) 
				{
#if( BOARD_NUM != 3)				
					while((USART1->SR&0X40)==0);   
					USART1->DR = mSendBuffer[i];  	
#else
					while((UART4->SR&0X40)==0);   
					UART4->DR = mSendBuffer[i];	
#endif
				}
				//*/					
				xSemaphoreGiveFromISR( mLogSemaphore, NULL );		
				pre = 0;
			} 
			else 
			{
				pre = ch;
			}
		}			
	} 
	else 
	{
		/*in task context*/
		
TRYAGAIN:
		
		if( rfifo_put( &mLogFifo, &ch, 1 ) != 1 ) 
		{
			/*we will never switch out*/
			vTaskDelay( 2 / portTICK_RATE_MS );
			
			/*we will never lost a byte to printf*/
			goto TRYAGAIN;
			/*
			mLogFifo.lostBytes++;
			xSemaphoreGive( mLogSemaphore );	
			pre = 0;
			*/
		} 
		else 
		{
			if( pre == 0x0d && ch == 0x0a ) 
			{
				xSemaphoreGive( mLogSemaphore );	
				pre = 0;
			} 
			else 
			{
				pre = ch;
			}
		}		
	}
	
	return ch;
}
#endif
 
#if 1   //���ʹ���˽���
//����1�жϷ������
//ע��,��ȡUSARTx->SR�ܱ���Ī������Ĵ���   	
//u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
//u16 USART_RX_STA=0;       //����״̬���	

//��ʼ��IO ����1 
//bound:������
void uart_init(u32 bound){
   //GPIO�˿�����
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //ʹ��GPIOAʱ��
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//ʹ��USART1ʱ��
 
	//����1��Ӧ���Ÿ���ӳ��
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9����ΪUSART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10����ΪUSART1
	
	//USART1�˿�����
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9��GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOA,&GPIO_InitStructure); //��ʼ��PA9��PA10

   //USART1 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
  USART_Init(USART1, &USART_InitStructure); //��ʼ������1
	
  USART_Cmd(USART1, ENABLE);  //ʹ�ܴ���1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	
#if 1	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart1 NVIC ����
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����

#endif
	
}

void uart_deinit( void )
{
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);//��������ж�	
  USART_Cmd(USART1, DISABLE);  //ʹ�ܴ���1 
	USART_DeInit(USART1);
}

extern Ringfifo uart1fifo;
extern QueueHandle_t xDebugQueue;
extern char isDma;

void USART1_IRQHandler(void)                	//����1�жϷ������
{
	//int len = 0;	
	uint8_t ch = 0;
	
	
	if( USART_GetITStatus( USART1, USART_IT_RXNE ) != RESET ) 
	{ 		
		ch = USART_ReceiveData( USART1 );
		
		(void) vPortEnterCritical();
		if( rfifo_put( &uart1fifo, &ch, 1 ) != 1 ) 
		{
				uart1fifo.lostBytes++;
		} 
		else 
		{

		}
		vTaskNotifyGiveFromISR( pxUsmartTask, NULL);
		(void) vPortExitCritical();
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	} 
} 
#endif	

 

//uart3 PD8 PD9 	������4GͨѶ
void uart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;	
	
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //ʹ��GPIOGʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);//ʹ��USART3ʱ��	
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8, GPIO_AF_USART3); //PD8����ΪUSART3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9, GPIO_AF_USART3); //PD9����ΪUSART3		
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; //PD8 PD9 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOD, &GPIO_InitStructure); //��ʼ��PD8 PD9	
	
	USART_InitStructure.USART_BaudRate = bound/*bound*/;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(USART3, &USART_InitStructure); //��ʼ������6
	
	USART_Cmd(USART3, ENABLE);  //ʹ�ܴ���3 
	USART_ClearFlag(USART3, USART_FLAG_TC);		
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart1 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����	
}

//uart4 pc10 pc11	���ڵ�����
void uart4_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //ʹ��GPIOAʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//ʹ��USART1ʱ��
	
	//����1��Ӧ���Ÿ���ӳ��
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //GPIOA9����ΪUART4
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //GPIOA10����ΪUART4
	
	//USART1�˿�����
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOA9��GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//�ٶ�50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //���츴�����
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //����
	GPIO_Init(GPIOC,&GPIO_InitStructure); //��ʼ��PA9��PA10	
	
   //USART4 ��ʼ������
	USART_InitStructure.USART_BaudRate = bound;//����������
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ
    USART_Init(UART4, &USART_InitStructure); //��ʼ������1	
	
	USART_Cmd(UART4, ENABLE);
	USART_ClearFlag(UART4, USART_FLAG_TC);
	
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//��������ж�

	//Usart1 NVIC ����
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//����1�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//��ռ���ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//�����ȼ�3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ�����
	
}

//uart6 pc6 pc7 	��?������?android����??
void uart6_init(u32 bound)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;	
	
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6,ENABLE);//��1?��USART6����?��	
	
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_USART6); //GPIOA9?�䨮??aUSART6
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_USART6); //GPIOA14?�䨮??aUSART6		
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; //GPIOA9��?GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?�䨮?1|?��
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//?��?��50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //��?����?�䨮?��?3?
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //��?��-
	GPIO_Init(GPIOC, &GPIO_InitStructure); //3?��??��PG9��?PG14	
	
	USART_InitStructure.USART_BaudRate = bound/*bound*/;//2����??������??
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//��?3��?a8??��y?Y??��?
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//��???����?1??
	USART_InitStructure.USART_Parity = USART_Parity_No;//?T????D��?��??
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//?T��2?t��y?Y����????
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//��?����?�꨺?
    USART_Init(USART6, &USART_InitStructure); //3?��??����??��6
	
	USART_Cmd(USART6, ENABLE);  //��1?����??��6 
	USART_ClearFlag(USART6, USART_FLAG_TC);		
	
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);//?a???��1??D??

	//Usart1 NVIC ????
  NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;//��??��1?D??�����̨�

/*0��3  ���ж����ȼ�����pxDownStreamTask����û�б�����
�ĳ�2�� 2
*/  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//?��??��??��??3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//������??��??3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ�����̨���1?��
	NVIC_Init(&NVIC_InitStructure);	//?��?Y???����?2?��y3?��??��VIC??��??��?��
}



extern Ringfifo uart6fifo;
extern TaskHandle_t pxDownStreamTask;

void USART3_IRQHandler( void )
{
	u8 Res;
	(void) vPortEnterCritical();		

	if( USART_GetITStatus(USART3, USART_IT_RXNE ) != RESET)
	{
		USART_ClearITPendingBit(USART3, USART_IT_RXNE); 	
		
		Res =USART_ReceiveData( USART3 );
		if( rfifo_put( &uart6fifo, &Res, 1 ) != 1 ) 
		{
			uart6fifo.lostBytes++; 
		} 

		vTaskNotifyGiveFromISR( pxDownStreamTask, NULL );
    } 
	
	(void) vPortExitCritical();
} 


void UART4_IRQHandler(void)
{
	u8 Res;
	
#if( BOARD_NUM == 3 )		
	(void) vPortEnterCritical();

	if( USART_GetITStatus( UART4, USART_IT_RXNE ) != RESET ) 
	{ 		
		USART_ClearITPendingBit( UART4, USART_IT_RXNE );
	
		Res = USART_ReceiveData( UART4 );
		
		if( rfifo_put( &uart1fifo, &Res, 1 ) != 1 ) 
		{
				uart1fifo.lostBytes++;
		} 

		vTaskNotifyGiveFromISR( pxUsmartTask, NULL);
	} 

	(void) vPortExitCritical();

#else

	(void) vPortEnterCritical();
	if( USART_GetITStatus( UART4, USART_IT_RXNE ) != RESET )
	{
		USART_ClearITPendingBit(UART4,USART_IT_RXNE);

		Res = USART_ReceiveData( UART4 );
		if( rfifo_put( &uart6fifo, &Res, 1 ) != 1 ) 
		{
				uart6fifo.lostBytes++;
		} 

		vTaskNotifyGiveFromISR( pxDownStreamTask, NULL );
    } 
	(void) vPortExitCritical();
	
#endif

} 

void USART6_IRQHandler(void)                	//��??��1?D??��t??3��D��
{
	u8 Res;
	unsigned int ret;

	(void) vPortEnterCritical();

	if(USART_GetITStatus(USART6, USART_IT_RXNE) != RESET)
	{
		//USART_ClearFlag(USART6, USART_IT_RXNE);
		USART_ClearITPendingBit(USART6, USART_IT_RXNE);		
		Res = USART_ReceiveData( USART6 );
	    ret = rfifo_put( &uart6fifo, &Res, 1 );		
    	if( ret != 1 ) 
		{
			uart6fifo.lostBytes++;
		}
		if( pxDownStreamTask )
		{
			vTaskNotifyGiveFromISR( pxDownStreamTask, NULL );		
		}
	} 
	(void) vPortExitCritical();	
} 




