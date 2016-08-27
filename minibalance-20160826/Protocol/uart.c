#include "uart.h"

#ifdef __GNUC__
// With GCC/RAISONANCE, small printf (option LD Linker->Libraries->Small printf set to 'Yes') calls __io_putchar()
#define UART_PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define UART_PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif /* __GNUC__ */

/* Private typedef -----------------------------------------------------------*/
typedef enum {FAILED = 0, PASSED = !FAILED} TestStatus;

void uart_gpio_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

#if defined (OPEN_UART1)
    USART1_GPIO_Cmd(USART1_GPIO_CLK, ENABLE);
    USART1_AFIO_Cmd(USART1_AFIO_CLK, ENABLE);
    USART1_CLK_Cmd(USART1_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = USART1_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART1_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USART1_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART1_GPIO_PORT, &GPIO_InitStructure);
#endif

#if defined (OPEN_UART2)
    USART2_GPIO_Cmd(USART2_GPIO_CLK, ENABLE);
    USART2_CLK_Cmd(USART2_CLK, ENABLE);
    USART2_AFIO_Cmd(USART2_AFIO_CLK, ENABLE);

    GPIO_InitStructure.GPIO_Pin = USART2_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART2_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USART2_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART2_GPIO_PORT, &GPIO_InitStructure);
#endif
#if defined (OPEN_UART3)
    USART3_GPIO_Cmd(USART3_GPIO_CLK, ENABLE);
    USART3_CLK_Cmd(USART3_CLK, ENABLE);
    USART3_AFIO_Cmd(USART3_AFIO_CLK, ENABLE);
    //GPIO_PinRemapConfig(GPIO_PartialRemap_USART3,ENABLE);

    GPIO_InitStructure.GPIO_Pin = USART3_TxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(USART3_GPIO_PORT, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = USART3_RxPin;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(USART3_GPIO_PORT, &GPIO_InitStructure);
#endif
}

void uart_config(void)
{
    USART_InitTypeDef USART_InitStructure;
#if defined (OPEN_UART1)
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART1, &USART_InitStructure);

//	USART_Cmd(USART1, ENABLE);
    USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART1, ENABLE);
    /* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
    如下语句解决第1个字节无法正确发送出去的问题 */
    USART_ClearFlag(USART1, USART_FLAG_TC); /* 清发送完成标志，Transmission Complete flag */
#endif

#if defined (OPEN_UART2)
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART2, &USART_InitStructure);

    USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART2, ENABLE);
    /* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
    如下语句解决第1个字节无法正确发送出去的问题 */
    USART_ClearFlag(USART2, USART_FLAG_TC); /* 清发送完成标志，Transmission Complete flag */
#endif

#if defined (OPEN_UART3)
    USART_InitStructure.USART_BaudRate = 115200;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);
		
    USART_ITConfig(USART3,USART_IT_RXNE,ENABLE);
    USART_Cmd(USART3, ENABLE);
		
    /* CPU的小缺陷：串口配置好，如果直接Send，则第1个字节发送不出去
    如下语句解决第1个字节无法正确发送出去的问题 */
    USART_ClearFlag(USART3, USART_FLAG_TC); /* 清发送完成标志，Transmission Complete flag */
#endif

}

void nvic_configuration(void)
{
    NVIC_InitTypeDef NVIC_InitStructure;
#if defined (OPEN_UART1)
    //使能串口中断，并设置优先级
    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif

#if defined (OPEN_UART2)
    //使能串口中断，并设置优先级
    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif

#if defined (OPEN_UART3)
    //使能串口中断，并设置优先级
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
#endif
}

void uart2_send_char(uint8_t data)
{
    USART_SendData(USART2, data);
    //Loop until the end of transmission
    while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
}

void uart1_send_char(uint8_t data)
{
    USART_SendData(USART1,data);
    //Loop until the end of transmission
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
}

void uart3_send_char(uint8_t data)
{
		//USART_ClearFlag(USART3,USART_FLAG_TC);
    USART_SendData(USART3,data);
    //Loop until the end of transmission
    while (USART_GetFlagStatus(USART3, USART_FLAG_TXE) == RESET);
}

void uart3_send_string(const char *data)
{
		int i, len;
		len = strlen(data);
	
		for (i=0; i<len; i++) {
				uart3_send_char(data[i]);
		}
}

void uart2_send_string(const char *data)
{
		int i, len;
		len = strlen(data);
	
		for (i=0; i<len; i++) {
				uart2_send_char(data[i]);
		}
}

void uart1_send_string(const char *data)
{
		int i, len;
		len = strlen(data);
	
		for (i=0; i<len; i++) {
				uart1_send_char(data[i]);
		}
}

#include "usmart.h"
void USART1_IRQHandler(void)
{
	int ret;
	uint8_t ch = 0;
  
	if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) { 		
			USART_ClearITPendingBit(USART1,USART_IT_RXNE);
			ch = USART_ReceiveData(USART1);
			if ((USART_RX_STA & 0x8000) == 0) {
					if (USART_RX_STA & 0x4000) {
							if (ch != 0x0a) {
								USART_RX_STA=0;
							} else {
								USART_RX_STA|=0x8000;
							}
					} else {	
							if (ch == 0x0d) {
								USART_RX_STA |= 0x4000;
							} else {
								USART_RX_BUF[USART_RX_STA&0X3FFF] = ch;
								USART_RX_STA++;
								if (USART_RX_STA > (USART_REC_LEN-1))
										USART_RX_STA=0;
						}		 
					}
			} 
	}
}

void uarts_init(void)
{
    uart_gpio_init();
    uart_config();
    nvic_configuration();
}


//不使用半主机模式, 如果没有这段，则需要在target选项中选择使用USE microLIB
// #if 1
// #pragma import(__use_no_semihosting)
// struct __FILE
// {
// 	int handle;
// };
// FILE __stdout;

// _sys_exit(int x)
// {
// 	x = x;
// }
// #endif

UART_PUTCHAR_PROTOTYPE
{
    //Place your implementation of fputc here , e.g. write a character to the USART
    USART_SendData(USART1, (u8)ch);
    //Loop until the end of transmission
    while (USART_GetFlagStatus(USART1, USART_FLAG_TXE) == RESET);
    return ch;
}

