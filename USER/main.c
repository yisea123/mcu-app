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
 ����������Ĵ���ͨѶ���ڴ�ѭ���У����ϵı�������Ҫ��������ݣ�
 ���Բ����з������ݰ������������
****************************************************************/
	
	while(1)
	{
		test_func();
		/*���ڲ���ʹ�ã���release�汾�н���ɾ��*/
		
		handle_debug_msg_report();
		/*�˺������ڰ�printf��debug log����android, ��android���棬��
		�ش�����������printf �ַ����б���Ҫ��\r\n��β��������ע�⣡
		����printf���������ж���ʹ�ã�*/
		
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
		
		handle_longsung_uart_msg();
		/*��androidϵͳ�ػ�ʱ����ʼ����4G�Ĵ�����Ϣ*/
		
		handle_longsung_setting();
		/*����4G�����硢���ӷ�������״̬*/
		
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
	/*���ÿ���androidϵͳ�ĸ�·DCDC*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	/***************************************************************		
		Ŀǰϵͳ�ж����ȼ����·��䣺
	
	  UART3     	��ռ���ȼ�2 �����ȼ�0		 ���ڽ���4G����
		��ʱ��2 		��ռ���ȼ�2 �����ȼ�1 	 ���ڼ�������������ļ���
		��ʱ��5 		��ռ���ȼ�2 �����ȼ�3		 ���ڶ�ʱ�ص�

		UART1/2/4   ��ռ���ȼ�3 �����ȼ�2	   ���ڵ���
	  ��ʱ��4 		��ռ���ȼ�3 �����ȼ�3		 ����usmartģ��

	
		UART6   		��ռ���ȼ�0 �����ȼ�3	   android��MCU����ͨ��
    CAN2    		��ռ���ȼ�0 �����ȼ�2		 can2��mcu������ͨ��
		CAN1    		��ռ���ȼ�0 �����ȼ�1	   can1��MCU������ͨ��
	******************************************************************/
	my_mem_init(SRAMIN);	
	
	INIT_LIST_HEAD(&bootloader_event_head);   
	
	INIT_LIST_HEAD(&event_head);   
	/*���ڴ��Ҫ���ϱ��¼����¼���CAN�����ϻ��*/
	
	INIT_LIST_HEAD(&periodic_head);   	
	/*���ڴ��Ҫ�·����¼����¼���uart6���������·���can�ϣ�Ϊ�̶������¼���*/
	
	uart3_fifo = kfifo_alloc(UART_4G_KFIFO_LEN, NULL);		
	uart6_fifo = kfifo_alloc(UART_KFIFO_LEN, NULL);	
	debug_fifo = kfifo_alloc(DEBUG_KFIFO_LEN, NULL);
	/*��Ŵ�uart6�ж��ж����Ĵ������ݣ����ݴӴ���������*/
	if(1) {
		/*���ڴ������ܰ�*/
		uart3_init(115200);
		uart4_init(115200);
		uart6_init(503000); 
		//t8 230400 �Ĳ����ʲ�׼��MCU��ߵ���Ϊ214550
		//t8 460800 �Ĳ����ʲ�׼�� MCU��ߵ���Ϊ503000
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
		
	/*CAN_Mode_LoopBack CAN_Mode_Normal*/
	
	My_RTC_Init();	
	/* ��ʼ��RTC��֮��Ϳ��Խ��ж�д RTC */
	
	for(t=0;t<10;t++) {
		//delay_ms(60);
		LED0=!LED0;
	}
	
	//Tout=((4*2^prer)*rlr)/32 (ms).  4*16*500
	IWDG_Init(6,750);
	/* ���ʱ��Ϊ6s	������Ҫ��ʱι���� ��6S�Ǿ������Ե����ݣ������������
	����ᵼ�¿��Ź�����������������ʱ*/
	
	longsung_init();
	/*��ʼ��4G������ص����ݽṹ*/
	
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
�˴����������ƿ������MCU��T8����ͨѶ�����ʣ�Ŀǰ��115200
ϣ����ߵ�230400,Ŀǰ����һ��230400��ͨѶ���ϣ�����Ӳ������
***********************************************************/

/************************************************************************************
��һ����¼������¼bootlader�� sector0, ��¼rom������6

#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//����0��ʼ��ַ, 16 Kbytes    ------->���bootloader�������������rom����־��
���true���������9 copy������6��ɾ�����±�־��Ȼ����������6���У�����ֱ�Ӵ�����6���С�

#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//����1��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//����2��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//����3��ʼ��ַ, 16 Kbytes    ------->����Ƿ������rom���ı�־�� �Լ�rom����С��

#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//����4��ʼ��ַ, 64 Kbytes    ------->BOOTLOADER_ADDRESS ���bootloader��bin�̼�

#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//����5��ʼ��ַ, 128 Kbytes  

#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//����6��ʼ��ַ, 128 Kbytes   ------->���mcu��rom���� android�и���rom����ʱ����
rom copy�� sector9, ������ɺ��޸�rom���±�־�� Ȼ�������reset->SCU_RESET_Force();
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//����7��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//����8��ʼ��ַ, 128 Kbytes  

#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//����9��ʼ��ַ, 128 Kbytes   ------->��sector9-sector11����Ŵ�android���µ�rom��
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//����10��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//����11��ʼ��ַ,128 Kbytes  




RTC   RTC_BKP_DR0~RTC_BKP_DR19

#define RTC_BKP_DR0                       ((uint32_t)0x00000000)  
--->RTC��ʼ��ʹ�ã������ڱ�ĵط�ʹ��

#define RTC_BKP_DR1                       ((uint32_t)0x00000001)
#define RTC_BKP_DR2                       ((uint32_t)0x00000002)
��������block�����洢MCU�����Ĵ���������ʹ��

#define RTC_BKP_DR3                       ((uint32_t)0x00000003)
����BLOCK������ʾROM�ļ��ݴ��flash��ַ�Ƿ��ѱ�����������ʹ��

************************************************************************************/








