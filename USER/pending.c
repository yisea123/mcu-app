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

extern long num_can1_IRQ, num_can2_IRQ, mMcuReportRepeatNum,  
	numRecvAndroidCanCmd, numMcuReportToAndroid, numMcuSendedCmdToAndroid;
extern char mMcuJumpAppPending;
extern char mRequestSoftRestPending;
extern char mReportMcuStatusPending;
extern unsigned int FLASH_If_Erase_Sector(unsigned int StartSector);
extern void handle_debug_msg_report(void);
extern void list_event_to_android(void);
extern char data0[CMD_UART_LEN], data3[CMD_LEN];
extern long numRecvAndroidCanCmd;
extern unsigned char md5[32];

/*Ӳ�����⵼���޷�����*/
__asm void SCU_RESET_Force(void)
{
 MOV R0, #1           //; 
 MSR FAULTMASK, R0    //; Clear FAULTMASK flag to disable all interrupts
 LDR R0, =0xE000ED0C  //; address of Application Interrupt and Reset Control Register
 LDR R1, =0x05FA0004  //; value of Application Interrupt and Reset Control Register
 STR R1, [R0]         //; system reset by soft ware   
 
deadloop
		B deadloop        //; deadloop
}

/*���ÿ��Ź����ܽ�������*/
void enter_loop_module(void)
{
		while(1);
}

uint16_t cpuGetFlashSize(void)
{

   return (*(__IO u16*)(0x1FFF7A22));
   // return (*(__IO u32*)(0x1FFF7A20))>>16;
}

/*RTC_Format_BCD RTC_Format_BIN*/
void get_sys_time(uint32_t format, RTC_DateTypeDef* date, RTC_TimeTypeDef* time)
{
	RTC_GetDate(format, date);
	RTC_GetTime(format, time);
}

void cpuidGetId(u32 mcuID[])
{
    mcuID[0] = *(__IO u32*)(0x1FFF7A10);
    mcuID[1] = *(__IO u32*)(0x1FFF7A14);
    mcuID[2] = *(__IO u32*)(0x1FFF7A18);
}

void report_debug_to_android(void)
{
	int tmp;
	for(tmp=0; tmp <14000; tmp++)
	/*Ϊ�˰����е�printf���ϴ���android*/
	{
		handle_debug_msg_report();
		list_event_to_android();
		handle_downstream_work(data0, data3);				
	}				
}

void handle_pending_work(void)
{
	static long wd=0, level=0, lastNum=0;
	static unsigned int mMcuBootTimes = 0;
	static char mWriteBootTimesFlag = 1;
	
	if(((wd++)%40000) == 0)
		/*��40000�Ƕ�Ӧ���Ź���6�룬���������޸ģ�*/
	{
		IWDG_Feed();
		LED1 =!LED1;
		//printf("IWDG_Feed(), wd=%d\r\n", wd);
	}	
	
	/*Android �·�����������������ϱ�MCU���ݽṹ������*/
	if((lastNum != numRecvAndroidCanCmd) && ((numRecvAndroidCanCmd%500000) == 0)) 
	{
		lastNum = numRecvAndroidCanCmd;
		mReportMcuStatusPending=1;
	}
	
	if(mReportMcuStatusPending)
	{
		switch(level++)
		{
			case 1:
				printf("*******************************MCU Running Status Start*******************************\r\n");
				printf("can1_fifo [size=%d, in=%d, out=%d, acceptCmds=%ld, errorCmds=%d, lostBytes=%d]\r\n", 
					can1_fifo->size, can1_fifo->in, can1_fifo->out, can1_fifo->acceptCmds,
					can1_fifo->errorCmds, can1_fifo->lostBytes);
			  break;
			
			case 100:
				printf("can2_fifo [size=%d, in=%d, out=%d, acceptCmds=%ld, errorCmds=%d, lostBytes=%d]\r\n", 
					can2_fifo->size, can2_fifo->in, can2_fifo->out, can2_fifo->acceptCmds,
					can2_fifo->errorCmds, can2_fifo->lostBytes);			
				break;
			
			case 200:
				printf("uart6_fifo [size=%d, in=%d, out=%d, acceptCmds=%ld, errorCmds=%d, lostBytes=%d]\r\n", 
					uart6_fifo->size, uart6_fifo->in, uart6_fifo->out, uart6_fifo->acceptCmds,
					uart6_fifo->errorCmds, uart6_fifo->lostBytes);					
				break;
			
			case 300:
				printf("uart3_fifo [size=%d, in=%d, out=%d, lostBytes=%d]\r\n", 
					uart3_fifo->size, uart3_fifo->in, uart3_fifo->out, uart3_fifo->lostBytes);						
				break;
			
			case 400:
				break;
			
			case 500:
				printf("mMcuBootTimes=%d\r\n", mMcuBootTimes);
				printf("num_can1_IRQ=%ld\r\n", num_can1_IRQ);
				printf("num_can2_IRQ=%ld\r\n", num_can2_IRQ);
				printf("numRecvAndroidCanCmd=%ld\r\n", numRecvAndroidCanCmd);
				printf("numMcuReportToAndroid=%ld\r\n", numMcuReportToAndroid);
				printf("numMcuSendedCmdToAndroid=%ld\r\n", numMcuSendedCmdToAndroid);
			  printf("mMcuReportRepeatNum=%ld\r\n", mMcuReportRepeatNum);				
				break;
			
			case 600:
				printf("********************************MCU Running Status End********************************\r\n");
				mReportMcuStatusPending = 0;
				level = 0;
				/*stop report mcu running status infomation*/
				break;
			
			default:
				break;
		}
	}
	
	if(mWriteBootTimesFlag)
		/*�˴�ֻ����һ��*/
	{
		unsigned int numBoot=0;
		RTC_DateTypeDef date;
		RTC_TimeTypeDef time;

		/*RTC_Format_BCD RTC_Format_BIN*/
    get_sys_time(RTC_Format_BIN, &date, &time);
		
		printf("%02d-%02d-%02d [WEEK=%d]\r\n", date.RTC_Year, date.RTC_Month,
						date.RTC_Date, date.RTC_WeekDay);
		printf("%02d:%02d:%02d [%s]\r\n", time.RTC_Hours, time.RTC_Minutes,
			time.RTC_Seconds, time.RTC_H12==RTC_H12_AM?"AM":"PM");		
		
		if(RTC_ReadBackupRegister(RTC_BKP_DR1)!=0xABCD)
		{
			numBoot = 1;
			RTC_WriteBackupRegister(RTC_BKP_DR2,numBoot);
			RTC_WriteBackupRegister(RTC_BKP_DR1,0xABCD);
		}	
		else  
		{
			numBoot = RTC_ReadBackupRegister(RTC_BKP_DR2);
			numBoot++;
			RTC_WriteBackupRegister(RTC_BKP_DR2,numBoot);
		}
		mMcuBootTimes = numBoot;
		printf("McuBootTimes = [%d]\r\n", mMcuBootTimes);		
		mWriteBootTimesFlag = 0;

	}
	
	if(mRequestSoftRestPending && list_event() == 0 && kfifo_len(uart6_fifo) == 0 && 
		event_sending == NULL && kfifo_len(can1_fifo) == 0 && kfifo_len(can2_fifo) == 0) 
	{
		mRequestSoftRestPending=0;
		printf("soft reset...\r\n");
		//delay_ms(100);
		report_debug_to_android();
		//SCU_RESET_Force();
		enter_loop_module();
	}
	
	if( mMcuJumpAppPending == 1 && list_event() == 0 && kfifo_len(uart6_fifo) == 0 && 
		event_sending == NULL && kfifo_len(can1_fifo) == 0 && kfifo_len(can2_fifo) == 0 
		&& mAndroidPower ==0 ) 
	{
		char tag = 0;
		u32 *pData = (u32 *)md5;
		unsigned char pRead[16];
		
		unsigned int preFlashdestination, mUpdate, tmp;
	  static unsigned int flashdestination = 0x0800C000;
		/*0x0800C000+800 ��¼mcu�����Ĵ���*/
		
		/* ����3��ʼ��ַ, 16 Kbytes */
		FLASH_If_Erase_Sector(flashdestination);
		
		mUpdate = 0x5a5a;
		preFlashdestination = flashdestination;
		
		/* ǰ8��BYTES �����Ƿ����ROM�ı�־�� Ϊ0x5a5a�� 0xa5a5ʱ����*/
		flashdestination = STMFLASH_Write(flashdestination, &mUpdate, 1);
		if(flashdestination == preFlashdestination+sizeof(unsigned int)) 
		{
			;//printf("%s: STMFLASH_Write sector 3 mUpdate0 ok.\r\n", __func__);
		} 
		else 
		{
			tag = 1;
			printf("%s: STMFLASH_Write sector 3 mUpdate0 fail !\r\n", __func__);
		}	
		
		mUpdate = 0xa5a5;
		preFlashdestination = flashdestination;
		flashdestination = STMFLASH_Write(flashdestination, &mUpdate, 1);
		if(flashdestination == preFlashdestination+sizeof(unsigned int)) 
		{
			;//printf("%s: STMFLASH_Write sector 3 mUpdate1 ok.\r\n", __func__);
		} 
		else 
		{
			tag = 1;
			printf("%s: STMFLASH_Write sector 3 mUpdate1 fail !\r\n", __func__);
		}	
		
		/* ������4��bytes ����rom���Ĵ�С */
		preFlashdestination = flashdestination;
		flashdestination = STMFLASH_Write(flashdestination, &romSize, 1);
		if(flashdestination == preFlashdestination+sizeof(unsigned int)) 
		{
			;//printf("%s: STMFLASH_Write sector 3 rom_size = %d ok.\r\n", __func__, romSize);
		}
		else 
		{
			tag = 1;
			printf("%s: STMFLASH_Write sector 3 romSize = %d fail !\r\n", __func__, romSize);
		}		
		
		/* ������4��bytes �������android download������rom���� ����flash�е��ĸ�λ��*/
		tmp = APPLICATION_ADDRESS;
		preFlashdestination = flashdestination;
		flashdestination = STMFLASH_Write(flashdestination, &tmp, 1);
		if(flashdestination == preFlashdestination+sizeof(unsigned int)) 
		{
			;//printf("%s: STMFLASH_Write sector 3 rom addr = %X ok.\r\n", __func__, tmp);
		} 
		else 
		{
			tag = 1;
			printf("%s: STMFLASH_Write sector 3 rom addr = %X fail !\r\n", __func__, tmp);
		}		
		
		/*��������16��bytes������MD5ֵ*/
		preFlashdestination = flashdestination;
		flashdestination = STMFLASH_Write(flashdestination, pData, 4);
		if(flashdestination == preFlashdestination + 4*sizeof(unsigned int)) 
		{
			;//printf("%s: STMFLASH_Write sector 3 md5 ok.\r\n", __func__);
		}
		else 
		{
			tag = 1;
			printf("%s: STMFLASH_Write sector 3 md5 fail !\r\n", __func__);
		}		

		
		flashdestination = 0x0800C000;
		STMFLASH_Read(flashdestination, &tmp, 1);
		printf("%s: addr=0X%X, value=0X%X \r\n", __func__, flashdestination, tmp);
		STMFLASH_Read(flashdestination+4, &tmp, 1);
		printf("%s: addr=0X%X, value=0X%X \r\n", __func__, flashdestination+4, tmp);

		STMFLASH_Read(flashdestination+8, &tmp, 1);
		printf("%s: addr=0X%X, romSize=%d \r\n", __func__, flashdestination+8, tmp);
		STMFLASH_Read(flashdestination+12, &tmp, 1);
		printf("%s: addr=0X%X, target addr=0X%X \r\n", __func__, flashdestination+12, tmp);
	
		STMFLASH_Read(flashdestination+16, (u32 *)pRead, 4);		
		printf("%s: addr=0X%X, md5=[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]\r\n", 
				__func__,	flashdestination+16,
				pRead[0], pRead[1],pRead[2], pRead[3],pRead[4], pRead[5],pRead[6], pRead[7],
				pRead[8], pRead[9],pRead[10], pRead[11],pRead[12], pRead[13],pRead[14], pRead[15]); 	
		
		if(tag == 0) 
		{
			mMcuJumpAppPending = 0;
			printf("%s:SCU_RESET_Force()\r\n", __func__);
			report_debug_to_android();	
			//SCU_RESET_Force();
			enter_loop_module();
			/* ����֮ǰ�޸ı�־��rom��С ��*/
		}
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

************************************************************************************/

/*****************************************		
		printf("tmp=%8X\r\n", tmp);
		if(flag == tmp) 
		{
			memcpy(data, &flag, 4);
			STMFLASH_Read(des+4, &numBoot, 1);
			numBoot++;
			memcpy(data+4, &numBoot, 4);
			STMFLASH_Write(des, (u32*)data, 2);
		} 
		else 
		{
			memcpy(data, &flag, 4);
			numBoot = 1;
			memcpy(data+4, &numBoot, 4);
			STMFLASH_Write(des, (u32*)data, 2);
		}
********************************************/		










