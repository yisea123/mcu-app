#include "test.h"
#include "kfifo.h"
#include "led.h"
#include "tmodem.h"
#include "rtc.h"
#include "iwdg.h"

extern char mReportMcuStatusPending;
extern char mUart6FifoLost, mUart3FifoLost;
extern char mCan1FifoLost, mCan2FifoLost;
static char mStart = 0;
static uint32_t mId = 0, mNumEvent=0;
extern char mAndroidShutDownPending, mAndroidPower;
int num=0;

extern unsigned int flashdestination;
extern char devNum;
void update(void)
{
		flashdestination = APPLICATION_ADDRESS;
		devNum=MCU_NUM;
		num = 1;
}

void updateb(void)
{
		flashdestination = BOOTLOADER_ADDRESS;
		devNum=BOOTLOADER_NUM;
		num = 1;
}

void android_down(void)
{
		mAndroidShutDownPending = 1;
}

int send_can_mesg()
{
	u8 t, res, canbuf[8];
	static u8 cnt=0;
	
	for(t=0; t<8; t++) 
	{
		cnt+=t;
		canbuf[t]=cnt;//填充发送缓冲区
	}
	res = CAN1_Send_Msg(canbuf, 8);//发送8个字节 
	if(res) {
		//printf("send can message success.\r\n");
		return 0;
	}
	else { 
		//printf("send can message fail!");
		return -1;
	}
}

int send_can1_mesg1(uint32_t id)
{
	u8 t, res, canbuf[8];
	static u8 cnt=0;
	printf("%s\r\n", __func__);
	for(t=0; t<8; t++)
	{
		cnt+=(t%4+(t+1)*t);
		canbuf[t]=cnt;//填充发送缓冲区
	}
	res = CAN1_Send_Msg1(id, canbuf, 8);//发送8个字节 
	if(res) {
		printf("send can message fail!");
		return 0;
	}
	else { 
		printf("send can message success.\r\n");
		return -1;
	}
}

int send_can2_mesg1(uint32_t id)
{
	u8 t, res, canbuf[8];
	static u8 cnt=0;
	
	for(t=0; t<8; t++)
	{
		cnt+=(t%4+(t+1)*t);
		canbuf[t]=cnt;//填充发送缓冲区
	}
	res = CAN2_Send_Msg1(id, canbuf, 8);//发送8个字节 
	if(res) {
		printf("send can message fail!");
		//printf("send can message success.\r\n");
		return 0;
	}
	else { 
		//printf("send can message success.\r\n");
		return -1;
	}
}

/*
void report_mesg_to_t8()
{
	u8 t;
	char tmp[] = {0xAA, 0xBB, 0x03, 0x80, 0x1c, 
	0xa4, 0xf6, 0x13};
	for(t=0;t<sizeof(tmp)/sizeof(char);t++)
	{
		USART_ClearFlag(USART6,USART_FLAG_TC); 
		USART_SendData(USART6, tmp[t]);         //向串口6发送数据
		while(USART_GetFlagStatus(USART6,USART_FLAG_TC)!=SET);//等待发送结束
	}		
}
*/
char mAndroidPowerBefore = 0;
extern char mPACKETSIZE;
extern long numMcuReportToAndroid;


void test_func(void)
{
	u8 t;
	static int k=1;
	static unsigned int times=0;
	
	//handle_4g_msg();
	
	//灯的亮灭来提示程序在正常跑动
	if((++times) > 60000)
	{
		times = 0;
		LED0=!LED0;
		if((k >0) && (k++)>10) 
		{
			k = 0;
			flashdestination = APPLICATION_ADDRESS;
			devNum=MCU_NUM;
			//num = 1; 
			/*用于开机后，自动请求更新rom的功能*/
		}
	}	
		
	//aa bb 0x1 0x0f check1 check2
	if(num == 1)
	{ 
		int rand;
		num = 0;
		//printf("%s: request update rom.bin.\r\n", __func__);
		printf("%s:-\r\n", __func__);
		rand = numMcuReportToAndroid%12;
		mPACKETSIZE = 40+rand*8;		
		if(mPACKETSIZE > 128) mPACKETSIZE=128;
		printf("rand=%d, mPACKETSIZE=%d\r\n", rand, mPACKETSIZE);
		report_tmodem_packet0(UPDATE, mPACKETSIZE);
		report_tmodem_packet0(ACK, 0);
		report_tmodem_packet(CRC16);
	}		
	
	/************************************
	用于can CAN_Mode_LoopBack 模式的测试
	*************************************/
	if(mStart) {
		if(list_event() < mNumEvent)
		{
			if(mId>0x7ff) mId = 0;
			for(t=0;t<1;t++) 
			{
				send_can1_mesg1(mId++);
				/*用于往can1发送数据的功能*/
			}
		}
	}

	if(mAndroidPower != mAndroidPowerBefore) 
	{
		mAndroidPowerBefore = mAndroidPower;
		//just for printf mAndroidPower value, if change , printf it
		printf("%s: mAndroidPower=%d\r\n", __func__, mAndroidPower);
	}

}

/*用于测试往can发数据的功能*/
void set_can_num_event(uint32_t num)
{
	printf("%d\r\n", num);
	if(num == mNumEvent)
	{
		mStart = 0;
		mNumEvent=0;
	}
	else 
	{
		mStart = 1;
		mNumEvent = num;
	}
}

void report_mcu_status(void)
{
	mReportMcuStatusPending = 1;
}

extern char mDebugUartPrintfEnable;

void enable_debug_tx(uint32_t num)
{
	mDebugUartPrintfEnable = num;
}


 
