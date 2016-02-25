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
#include "longsung.h"
#include "timer.h"
#include "test.h"

#define START_LOG   \
printf(" \
\r\n/******       MCU   -   START        ******/ \
\r\n/***** LOADER FROM ANDROID RELEASE-12 *****/ \
\r\n/*****  WAIT TO UPDATE FROM ANDROID *******/ \
\r\n\r\n");

static int init_work(void);

u32 mcuID[4];

char data0[CMD_UART_LEN], data1[CMD_LEN], 
	data2[CMD_LEN], data3[CMD_LEN];


int main(void)
{ 
	char *cmd, *ack, *cmd1, *cmd2;	

	SCB->VTOR = FLASH_BASE | 0x40000; 
	
	cmd=data0; cmd1=data1; cmd2=data2; ack=data3;
 
	mDebugUartPrintfEnable = 1;
	//mAndroidPowerUpPending = 1;
	
	init_work();

	START_LOG
	
	cpuidGetId(mcuID);
	printf("mcu ID is:[0X%X %8X %8X]\r\n", mcuID[0], mcuID[1], mcuID[2]);
	printf("Flash size is:[%d KB]\r\n",cpuGetFlashSize());
/***************************************************************
 所有与大屏的串口通讯都在此循环中，不断的遍历所有要处理的数据，
 所以不会有发送数据包不完整的情况
****************************************************************/
	
	while(1)
	{
		test_func();
		/*用于测试使用，在release版本中将被删除*/
		
		handle_debug_msg_report();
		/*此函数用于把printf的debug log发到android, 由android保存，并
		回传给服务器，printf 字符串中必须要以\r\n结尾！！！请注意！
		并且printf不可以在中断中使用！*/
		
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
		
		handle_longsung_uart_msg();
		/*当android系统关机时，开始遍历4G的串口消息*/
		
		handle_longsung_setting();
		/*设置4G的网络、连接服务器等状态*/
		
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
	/*设置控制android系统的各路DCDC*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	/***************************************************************		
		目前系统中断优先级如下分配：
	
	  UART3     	抢占优先级2 子优先级0		 用于接收4G数据
		定时器2 		抢占优先级2 子优先级1 	 用于计数，多个变量的计数
		定时器5 		抢占优先级2 子优先级3		 用于定时关电

		UART1/2/4   抢占优先级3 子优先级2	   用于调试
	  定时器4 		抢占优先级3 子优先级3		 用于usmart模块

	
		UART6   		抢占优先级0 子优先级3	   android与MCU数据通道
    CAN2    		抢占优先级0 子优先级2		 can2与mcu的数据通道
		CAN1    		抢占优先级0 子优先级1	   can1与MCU的数据通道
	******************************************************************/
	my_mem_init(SRAMIN);	
	
	INIT_LIST_HEAD(&bootloader_event_head);   
	
	INIT_LIST_HEAD(&event_head);   
	/*用于存放要的上报事件，事件从CAN总线上获得*/
	
	INIT_LIST_HEAD(&periodic_head);   	
	/*用于存放要下发的事件，事件从uart6发过来，下发到can上，为固定周期事件。*/
	
	uart3_fifo = kfifo_alloc(UART_4G_KFIFO_LEN, NULL);		
	uart6_fifo = kfifo_alloc(UART_KFIFO_LEN, NULL);	
	debug_fifo = kfifo_alloc(DEBUG_KFIFO_LEN, NULL);
	/*存放从uart6中断中读到的串口数据，数据从大屏发过来*/
	if(1) {
		/*用于大屏功能板*/
		uart3_init(115200);
		uart4_init(115200);
		uart6_init(503000); 
		//t8 230400 的波特率不准，MCU这边调整为214550
		//t8 460800 的波特率不准， MCU这边调整为503000
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
		
	/*CAN_Mode_LoopBack CAN_Mode_Normal*/
	
	My_RTC_Init();	
	/* 初始化RTC，之后就可以进行读写 RTC */
	
	for(t=0;t<10;t++) {
		//delay_ms(60);
		LED0=!LED0;
	}
	
	//Tout=((4*2^prer)*rlr)/32 (ms).  4*16*500
	IWDG_Init(6,750);
	/* 溢出时间为6s	代码需要定时喂狗！ 此6S是经过测试的数据，请勿随意更改
	否则会导致看门狗重启，当数据量大时*/
	
	longsung_init();
	/*初始化4G串口相关的数据结构*/
	
	if(can1_fifo==NULL || can2_fifo==NULL  || debug_fifo == NULL ||
		uart6_fifo == NULL || uart3_fifo == NULL) 
	{
		printf("%s: malloc cmd or ack error!\r\n", __func__);
		return -1;
	}
	else 
	{
		return 1;
	}
	
}


/***********************************************************
此代码的吞吐量瓶颈在于MCU与T8串口通讯的速率，目前是115200
希望提高到230400,目前速率一到230400就通讯不上，加入硬件流控
***********************************************************/

/************************************************************************************
第一次烧录必须烧录bootlader到 sector0, 烧录rom到扇区6

#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes    ------->存放bootloader，起来后检查更新rom包标志，
如果true，则从扇区9 copy到扇区6，删除更新标志，然后跳到扇区6运行，否则直接从扇区6运行。

#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes    ------->存放是否更新了rom包的标志， 以及rom包大小。

#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes    ------->BOOTLOADER_ADDRESS 存放bootloader的bin固件

#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  

#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//扇区6起始地址, 128 Kbytes   ------->存放mcu的rom包， android有更新rom请求时，把
rom copy到 sector9, 更新完成后，修改rom更新标志， 然后调用软reset->SCU_RESET_Force();
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//扇区8起始地址, 128 Kbytes  

#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//扇区9起始地址, 128 Kbytes   ------->（sector9-sector11）存放从android更新的rom包
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//扇区11起始地址,128 Kbytes  




RTC   RTC_BKP_DR0~RTC_BKP_DR19

#define RTC_BKP_DR0                       ((uint32_t)0x00000000)  
--->RTC初始化使用，请勿在别的地方使用

#define RTC_BKP_DR1                       ((uint32_t)0x00000001)
#define RTC_BKP_DR2                       ((uint32_t)0x00000002)
以上两个block用来存储MCU重启的次数，请勿使用

#define RTC_BKP_DR3                       ((uint32_t)0x00000003)
以上BLOCK用来标示ROM文件暂存的flash地址是否已被擦除，请勿使用

************************************************************************************/








