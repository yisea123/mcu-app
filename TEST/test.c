#include "test.h"
#include "kfifo.h"

extern char mUart6FifoLost, mUart3FifoLost;
extern char mCan1FifoLost, mCan2FifoLost;
static char mStart = 0;
static uint32_t mId = 0, mNumEvent=0;
extern char mAndroidShutDownPending, mAndroidPower;

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
		//printf("send can message success.\r\n");
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

char mCopyAndroidPower = 0;

void test_func(void)
{
	u8 t;
	
	//aa bb 0x1 0x0f check1 check2
	
	/************************************
	用于can CAN_Mode_LoopBack 模式的测试
	*************************************/
	if(mStart) {
		if(list_event() < mNumEvent){
			if(mId>0x7ff) mId = 0;
			for(t=0;t<1;t++) {
				send_can1_mesg1(mId++);
			}
		}
	}

	if(mAndroidPower != mCopyAndroidPower) {
			mCopyAndroidPower = mAndroidPower;
		  printf("mAndroidPower=%d\r\n", mAndroidPower);
	}
		
	if(mUart3FifoLost) {
		mUart3FifoLost = 0;
		printf("uart3_fifo->lostBytes=%d\r\n", uart3_fifo->lostBytes);
	}	
	
	if(mUart6FifoLost) {
		mUart6FifoLost = 0;
		printf("uart6_fifo->lostBytes=%d\r\n", uart6_fifo->lostBytes);
	}	
	
	if(mCan1FifoLost) {
		mCan1FifoLost = 0;
		printf("can1_fifo->lostBytes=%d\r\n", can1_fifo->lostBytes);
	}	
	
	if(mCan2FifoLost) {
		mCan2FifoLost = 0;
		printf("can2_fifo->lostBytes=%d\r\n", can2_fifo->lostBytes);
	}				
}

void set_start(void)
{
	mStart = !mStart;
}

void set_num_event(uint32_t num)
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


