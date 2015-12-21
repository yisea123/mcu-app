#include "sys.h"
#include "led.h"
#include "key.h"
#include "can.h"
#include "rtc.h"
#include "list.h"
#include "beep.h"
#include "kfifo.h"
#include "delay.h"
#include "usart.h"
#include "ioctr.h"
#include "usmart.h"
#include "malloc.h"
#include "car_event.h"
#include "uart_command.h"
#include "tmodem.h"
#include "iwdg.h"
#include "stmflash.h"
#include "bootloader_update.h"
#include "pending.h"
#include "test.h"

#define START_LOG   \
printf(" \
\r\n/******       MCU   -   START        ******/ \
\r\n/***** LOADER FROM ANDROID RELEASE-89 *****/ \
\r\n/*****  WAIT TO UPDATE FROM ANDROID *******/ \
\r\n\r\n");

static int init_work(void);
extern uint16_t cpuGetFlashSize(void);
extern void cpuidGetId(u32 mcuID[]);
extern void handle_pending_work(void);
	
char mAndroidShutDownPending=0;
/*此变量用于请求关掉android的电源*/

char mAndroidPowerUpPending=0;
/*此变量用于请求打开android的电源*/

char mMcuJumpAppPending = 0;
/*此变量用于请求打开reset MCU*/

char mRequestSoftRestPending = 0;
/*此变量用于请求打开reset MCU*/

u32 mcuID[4];

char data0[CMD_UART_LEN], data1[CMD_LEN], 
	data2[CMD_LEN], data3[CMD_LEN];


int main(void)
{ 
	char *cmd, *ack, *cmd1, *cmd2;	

	//SCB->VTOR = FLASH_BASE | 0x40000; 
	
	cmd=data0; cmd1=data1; cmd2=data2; ack=data3;
 
	mDebugUartPrintfEnable = 1;
	mAndroidPowerUpPending = 1;
	
	init_work();

	START_LOG
	
	cpuidGetId(mcuID);
	printf("mcu ID is:[0X%X %8X %8X]\r\n", mcuID[0], mcuID[1], mcuID[2]);
	printf("Flash size is:[%d KB]\r\n",cpuGetFlashSize());
	
	while(1)
	{
		test_func();
		/*用于测试使用，在release版本中将被删除*/
		
		handle_debug_msg_report();
		/*此函数用于把printf的debug log发到android, 由android保存，并
		回传给服务器，printf 字符串中必须要以\r\n结尾！！！请注意！
		并且printf不可以中断中使用！*/
		
		handle_shutdown_android_request();
		/*处理关机请求*/
		
		handle_powerup_android_request();
		/*处理开机请求*/
		
		handle_downstream_work(cmd, ack);
		/*处理大屏到MCU的数据*/
		
		handle_upstream_work(cmd1, cmd2);		
		/*处理从CAN总线到MCU的数据*/
	
		handle_bootloader_update_work();
		/*处理BOOTLOADER的烧录请求*/
		
		handle_pending_work();
		/*遍历其它一些需要遍历的事情，以后可以往里面添加事件*/	
	}
}                                                                                                     

static int init_work(void)
{
	u8 t;
	delay_init(168);		
	/*延时初始化*/ 	 
	Init_io_ctrl(); 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	my_mem_init(SRAMIN);	
	
	INIT_LIST_HEAD(&bootloader_event_head);   
	
	INIT_LIST_HEAD(&event_head);   
	/*用于存放要的上报事件，事件从CAN总线上获得*/
	
	INIT_LIST_HEAD(&periodic_head);   	
	/*用于存放要下发的事件，事件从uart6发过来，下发到can上，为固定周期事件。*/
	
	//uart3_fifo = kfifo_alloc(512, NULL);		
	uart6_fifo = kfifo_alloc(UART_KFIFO_LEN, NULL);	
	debug_fifo = kfifo_alloc(DEBUG_KFIFO_LEN, NULL);
	/*存放从uart6中断中读到的串口数据，数据从大屏发过来*/
	if(1) {
		/*用于大屏功能板*/
		uart4_init(115200);
		uart6_init(115200);
	} else {
		/*初始化串口2跟串口6，用于开发板*/
		uart_init(115200);	
		/*串口初始化波特率为115200 230400  460800*/
	}
	/*初始化串口1跟6， uart1用于debug, 串口6用于与大屏交互数据*/
	
	usmart_dev.init(84); 
	
	LED_Init();
	
	My_RTC_Init();		 
	/*用于大屏获取RTC时间，或者设置RTC时间*/
	can1_fifo = kfifo_alloc(CAN_KFIFO_LEN, NULL);	
	can2_fifo = kfifo_alloc(CAN_KFIFO_LEN, NULL);	
	/* 用于存放从can1、can2中断中读到的数据帧，数据帧格式已完成 */
	
	Timer2_Init(1000,(u32)84*100-1); 
	/* 100MS中断一次 , 目前定时器4被usmart模块使用， 定时器5被ioctl模块使用*/	

	CAN1_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6, CAN_Mode_Normal);	
	CAN2_Mode_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6, CAN_Mode_Normal);	
			
	My_RTC_Init();	
	
	for(t=0;t<10;t++) {
		//delay_ms(60);
		LED0=!LED0;
	}
	
	IWDG_Init(6,750);

	if(can1_fifo==NULL || can2_fifo==NULL  || debug_fifo == NULL ||
		uart6_fifo == NULL/* || uart3_fifo == NULL*/) {
		printf("%s: malloc cmd or ack error!\r\n", __func__);
		return -1;
	}	else {
		return 1;
	}
	
}






