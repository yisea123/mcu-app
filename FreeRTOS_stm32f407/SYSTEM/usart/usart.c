#include "sys.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "rfifo.h"
#include "usart.h"
#include "semphr.h"
#include "w25qxx.h" 


//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
//#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 
}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
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

//重定义fputc函数 
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
		while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
		USART1->DR = (u8) ch;  
#else
	while((UART4->SR&0X40)==0);//循环发送,直到发送完毕   
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
 
#if 1   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
//u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
//u16 USART_RX_STA=0;       //接收状态标记	

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
   //GPIO端口设置
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE); //使能GPIOA时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);//使能USART1时钟
 
	//串口1对应引脚复用映射
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1); //GPIOA9复用为USART1
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1); //GPIOA10复用为USART1
	
	//USART1端口配置
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOA,&GPIO_InitStructure); //初始化PA9，PA10

   //USART1 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
  USART_Init(USART1, &USART_InitStructure); //初始化串口1
	
  USART_Cmd(USART1, ENABLE);  //使能串口1 
	
	USART_ClearFlag(USART1, USART_FLAG_TC);
	
#if 1	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
  NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、

#endif
	
}

void uart_deinit( void )
{
	USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);//开启相关中断	
  USART_Cmd(USART1, DISABLE);  //使能串口1 
	USART_DeInit(USART1);
}

extern Ringfifo uart1fifo;
extern QueueHandle_t xDebugQueue;
extern char isDma;

void USART1_IRQHandler(void)                	//串口1中断服务程序
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

 

//uart3 PD8 PD9 	用于与4G通讯
void uart3_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;	
	
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE); //使能GPIOG时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);//使能USART3时钟	
	
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource8, GPIO_AF_USART3); //PD8复用为USART3
	GPIO_PinAFConfig(GPIOD,GPIO_PinSource9, GPIO_AF_USART3); //PD9复用为USART3		
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9; //PD8 PD9 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOD, &GPIO_InitStructure); //初始化PD8 PD9	
	
	USART_InitStructure.USART_BaudRate = bound/*bound*/;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(USART3, &USART_InitStructure); //初始化串口6
	
	USART_Cmd(USART3, ENABLE);  //使能串口3 
	USART_ClearFlag(USART3, USART_FLAG_TC);		
	
	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//抢占优先级2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、	
}

//uart4 pc10 pc11	用于调试用
void uart4_init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE); //使能GPIOA时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4,ENABLE);//使能USART1时钟
	
	//串口1对应引脚复用映射
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource10,GPIO_AF_UART4); //GPIOA9复用为UART4
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource11,GPIO_AF_UART4); //GPIOA10复用为UART4
	
	//USART1端口配置
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11; //GPIOA9与GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//速度50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //推挽复用输出
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //上拉
	GPIO_Init(GPIOC,&GPIO_InitStructure); //初始化PA9，PA10	
	
   //USART4 初始化设置
	USART_InitStructure.USART_BaudRate = bound;//波特率设置
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
    USART_Init(UART4, &USART_InitStructure); //初始化串口1	
	
	USART_Cmd(UART4, ENABLE);
	USART_ClearFlag(UART4, USART_FLAG_TC);
	
	USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//开启相关中断

	//Usart1 NVIC 配置
    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;//串口1中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器、
	
}

//uart6 pc6 pc7 	ó?óúó?androidí¨??
void uart6_init(u32 bound)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;	
	
 	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART6,ENABLE);//ê1?üUSART6ê±?ó	
	
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource6,GPIO_AF_USART6); //GPIOA9?′ó??aUSART6
	GPIO_PinAFConfig(GPIOC,GPIO_PinSource7,GPIO_AF_USART6); //GPIOA14?′ó??aUSART6		
	
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; //GPIOA9ó?GPIOA10
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?′ó?1|?ü
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;	//?ù?è50MHz
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP; //í?íì?′ó?ê?3?
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP; //é?à-
	GPIO_Init(GPIOC, &GPIO_InitStructure); //3?ê??ˉPG9￡?PG14	
	
	USART_InitStructure.USART_BaudRate = bound/*bound*/;//2¨ì??êéè??
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//×?3¤?a8??êy?Y??ê?
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//ò???í￡?1??
	USART_InitStructure.USART_Parity = USART_Parity_No;//?T????D￡?é??
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//?Tó2?têy?Yá÷????
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//ê?·￠?￡ê?
    USART_Init(USART6, &USART_InitStructure); //3?ê??ˉ′??ú6
	
	USART_Cmd(USART6, ENABLE);  //ê1?ü′??ú6 
	USART_ClearFlag(USART6, USART_FLAG_TC);		
	
	USART_ITConfig(USART6, USART_IT_RXNE, ENABLE);//?a???à1??D??

	//Usart1 NVIC ????
  NVIC_InitStructure.NVIC_IRQChannel = USART6_IRQn;//′??ú1?D??í¨μà

/*0，3  的中断优先级导致pxDownStreamTask任务没有被唤醒
改成2， 2
*/  
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2;//?à??ó??è??3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority =2;		//×óó??è??3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQí¨μàê1?ü
	NVIC_Init(&NVIC_InitStructure);	//?ù?Y???¨μ?2?êy3?ê??ˉVIC??′??÷?￠
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

void USART6_IRQHandler(void)                	//′??ú1?D??·t??3ìDò
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




