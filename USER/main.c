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
/*�˱�����������ص�android�ĵ�Դ*/

char mAndroidPowerUpPending=0;
/*�˱������������android�ĵ�Դ*/

char mMcuJumpAppPending = 0;
/*�˱������������reset MCU*/

char mRequestSoftRestPending = 0;
/*�˱������������reset MCU*/

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
		/*���ڲ���ʹ�ã���release�汾�н���ɾ��*/
		
		handle_debug_msg_report();
		/*�˺������ڰ�printf��debug log����android, ��android���棬��
		�ش�����������printf �ַ����б���Ҫ��\r\n��β��������ע�⣡
		����printf�������ж���ʹ�ã�*/
		
		handle_shutdown_android_request();
		/*����ػ�����*/
		
		handle_powerup_android_request();
		/*����������*/
		
		handle_downstream_work(cmd, ack);
		/*���������MCU������*/
		
		handle_upstream_work(cmd1, cmd2);		
		/*�����CAN���ߵ�MCU������*/
	
		handle_bootloader_update_work();
		/*����BOOTLOADER����¼����*/
		
		handle_pending_work();
		/*��������һЩ��Ҫ���������飬�Ժ��������������¼�*/	
	}
}                                                                                                     

static int init_work(void)
{
	u8 t;
	delay_init(168);		
	/*��ʱ��ʼ��*/ 	 
	Init_io_ctrl(); 
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

	my_mem_init(SRAMIN);	
	
	INIT_LIST_HEAD(&bootloader_event_head);   
	
	INIT_LIST_HEAD(&event_head);   
	/*���ڴ��Ҫ���ϱ��¼����¼���CAN�����ϻ��*/
	
	INIT_LIST_HEAD(&periodic_head);   	
	/*���ڴ��Ҫ�·����¼����¼���uart6���������·���can�ϣ�Ϊ�̶������¼���*/
	
	//uart3_fifo = kfifo_alloc(512, NULL);		
	uart6_fifo = kfifo_alloc(UART_KFIFO_LEN, NULL);	
	debug_fifo = kfifo_alloc(DEBUG_KFIFO_LEN, NULL);
	/*��Ŵ�uart6�ж��ж����Ĵ������ݣ����ݴӴ���������*/
	if(1) {
		/*���ڴ������ܰ�*/
		uart4_init(115200);
		uart6_init(115200);
	} else {
		/*��ʼ������2������6�����ڿ�����*/
		uart_init(115200);	
		/*���ڳ�ʼ��������Ϊ115200 230400  460800*/
	}
	/*��ʼ������1��6�� uart1����debug, ����6�����������������*/
	
	usmart_dev.init(84); 
	
	LED_Init();
	
	My_RTC_Init();		 
	/*���ڴ�����ȡRTCʱ�䣬��������RTCʱ��*/
	can1_fifo = kfifo_alloc(CAN_KFIFO_LEN, NULL);	
	can2_fifo = kfifo_alloc(CAN_KFIFO_LEN, NULL);	
	/* ���ڴ�Ŵ�can1��can2�ж��ж���������֡������֡��ʽ����� */
	
	Timer2_Init(1000,(u32)84*100-1); 
	/* 100MS�ж�һ�� , Ŀǰ��ʱ��4��usmartģ��ʹ�ã� ��ʱ��5��ioctlģ��ʹ��*/	

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






