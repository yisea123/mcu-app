#include "uart_command.h"
#include "tmodem.h"
#include "ioctr.h"
#include "rtc.h"
#include <string.h>

//#include "dma.h"
extern char mDebugUartPrintfEnable;
extern void report_debug_to_android(void);
extern void get_sys_time(uint32_t format, RTC_DateTypeDef* date, RTC_TimeTypeDef* time);
static int report_rtc_msg(unsigned char *msg, unsigned char len);
static int report_mcu_version_msg(unsigned char *msg, unsigned char len);
void report_mcu_software_version(void);
void report_mcu_id_version(void);

long numRecvAndroidCanCmd = 0;
uint32_t periodicNum = 0;
char mReportMcuStatusPending=0;
struct list_head periodic_head;

static const unsigned short crctable[256]=
{0x0000 , 0x1021 , 0x2042 , 0x3063 , 0x4084 , 0x50a5 , 0x60c6 , 0x70e7 , 0x8108 , 0x9129 ,
0xa14a , 0xb16b , 0xc18c , 0xd1ad , 0xe1ce , 0xf1ef , 0x1231 , 0x0210 , 0x3273 , 0x2252 ,
0x52b5 , 0x4294 , 0x72f7 , 0x62d6 , 0x9339 , 0x8318 , 0xb37b , 0xa35a , 0xd3bd , 0xc39c , 
0xf3ff , 0xe3de , 0x2462 , 0x3443 , 0x0420 , 0x1401 , 0x64e6 , 0x74c7 , 0x44a4 , 0x5485 , 
0xa56a , 0xb54b , 0x8528 , 0x9509 , 0xe5ee , 0xf5cf , 0xc5ac , 0xd58d , 0x3653 , 0x2672 , 
0x1611 , 0x0630 , 0x76d7 , 0x66f6 , 0x5695 , 0x46b4 , 0xb75b , 0xa77a , 0x9719 , 0x8738 , 
0xf7df , 0xe7fe , 0xd79d , 0xc7bc , 0x48c4 , 0x58e5 , 0x6886 , 0x78a7 , 0x0840 , 0x1861 , 
0x2802 , 0x3823 , 0xc9cc , 0xd9ed , 0xe98e , 0xf9af , 0x8948 , 0x9969 , 0xa90a , 0xb92b , 
0x5af5 , 0x4ad4 , 0x7ab7 , 0x6a96 , 0x1a71 , 0x0a50 , 0x3a33 , 0x2a12 , 0xdbfd , 0xcbdc , 
0xfbbf , 0xeb9e , 0x9b79 , 0x8b58 , 0xbb3b , 0xab1a , 0x6ca6 , 0x7c87 , 0x4ce4 , 0x5cc5 , 
0x2c22 , 0x3c03 , 0x0c60 , 0x1c41 , 0xedae , 0xfd8f , 0xcdec , 0xddcd , 0xad2a , 0xbd0b , 
0x8d68 , 0x9d49 , 0x7e97 , 0x6eb6 , 0x5ed5 , 0x4ef4 , 0x3e13 , 0x2e32 , 0x1e51 , 0x0e70 , 
0xff9f , 0xefbe , 0xdfdd , 0xcffc , 0xbf1b , 0xaf3a , 0x9f59 , 0x8f78 , 0x9188 , 0x81a9 , 
0xb1ca , 0xa1eb , 0xd10c , 0xc12d , 0xf14e , 0xe16f , 0x1080 , 0x00a1 , 0x30c2 , 0x20e3 , 
0x5004 , 0x4025 , 0x7046 , 0x6067 , 0x83b9 , 0x9398 , 0xa3fb , 0xb3da , 0xc33d , 0xd31c , 
0xe37f , 0xf35e , 0x02b1 , 0x1290 , 0x22f3 , 0x32d2 , 0x4235 , 0x5214 , 0x6277 , 0x7256 , 
0xb5ea , 0xa5cb , 0x95a8 , 0x8589 , 0xf56e , 0xe54f , 0xd52c , 0xc50d , 0x34e2 , 0x24c3 , 
0x14a0 , 0x0481 , 0x7466 , 0x6447 , 0x5424 , 0x4405 , 0xa7db , 0xb7fa , 0x8799 , 0x97b8 , 
0xe75f , 0xf77e , 0xc71d , 0xd73c , 0x26d3 , 0x36f2 , 0x0691 , 0x16b0 , 0x6657 , 0x7676 , 
0x4615 , 0x5634 , 0xd94c , 0xc96d , 0xf90e , 0xe92f , 0x99c8 , 0x89e9 , 0xb98a , 0xa9ab , 
0x5844 , 0x4865 , 0x7806 , 0x6827 , 0x18c0 , 0x08e1 , 0x3882 , 0x28a3 , 0xcb7d , 0xdb5c , 
0xeb3f , 0xfb1e , 0x8bf9 , 0x9bd8 , 0xabbb , 0xbb9a , 0x4a75 , 0x5a54 , 0x6a37 , 0x7a16 , 
0x0af1 , 0x1ad0 , 0x2ab3 , 0x3a92 , 0xfd2e , 0xed0f , 0xdd6c , 0xcd4d , 0xbdaa , 0xad8b , 
0x9de8 , 0x8dc9 , 0x7c26 , 0x6c07 , 0x5c64 , 0x4c45 , 0x3ca2 , 0x2c83 , 0x1ce0 , 0x0cc1 , 
0xef1f , 0xff3e , 0xcf5d , 0xdf7c , 0xaf9b , 0xbfba , 0x8fd9 , 0x9ff8 , 0x6e17 , 0x7e36 , 
0x4e55 , 0x5e74 , 0x2e93 , 0x3eb2 , 0x0ed1 , 0x1ef0  
};

void add_msg_to_list(struct msg_periodic *msg)
{
	//list_add(&event->list, &periodic_head);
	//count_cmd++;
	if(msg)
		list_add_tail(&msg->list, &periodic_head); //add to tail
}

void del_msg(struct msg_periodic *msg)
{
	struct msg_periodic *msg0;
	struct list_head *pos, *n;
	
	if(msg == NULL) return;
	
	list_for_each_safe(pos, n, &periodic_head) 
	{ 
		msg0 = list_entry(pos, struct msg_periodic, list); 
		if(msg0 == msg) 
		{ 
				list_del(pos); 
		} 
	 } 
}

struct msg_periodic * get_msg()
{
	struct msg_periodic *msg=NULL;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &periodic_head)
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		break;
	}	

	return msg;
}

/*******************************************************************
遍历周期消息的链表，获得每个消息的结构体，周期发送消息。
********************************************************************/
void list_periodic_msg()
{
	u8 res;
	struct msg_periodic *msg=NULL;
	struct list_head *pos=NULL, *n=NULL;
	
//	printf("{\r\n");
	list_for_each_safe(pos, n, &periodic_head)
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		
		if(*(msg->periodic) < msg->update) {//msg->periodic有可能溢出
			msg->update = *(msg->periodic);
		}
		
		if((*(msg->periodic)- msg->update) >= msg->n) 
		//周期由n指定，1为100MS
		{
			msg->update = *(msg->periodic);
			if( msg->canx == 1) {
				res = CAN1_Send_Msg2(msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
			} else if( msg->canx == 2 ){
				res = CAN2_Send_Msg2(msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
			} else {
					printf("can num error in periodic! num = %d\r\n", msg->canx);
			}
			
			if(res);
			else ;//printf("CAN1_Send_Msg2 error!\r\n");
/*			
			printf("send periodic msg to can.\r\n");
			printf("msg->id=0x%04X, msg->update=%d:\r\n", msg->id, msg->update);
			for(res=0; res<msg->len; res++) {
				printf("0X%02X ", *(msg->msg+res));
			}
			printf("\r\n");			
*/			
		}
//		printf("[event->id=%x, event->tim_count=%d]\r\n", event->id, event->tim_count);
//		for(t=0; t<event->data_len; t++) {
//			printf("0X%02X ", *(event->data+t));
//		}		
//		printf("\r\n");			
	}	
	//printf("car event count = %d, count_cmd=%ld\r\n", count, count_cmd);
}

/*******************************************************************
根据数据的ID来确定数据是否需要被覆盖
********************************************************************/
struct msg_periodic * check_periodic_msg(uint32_t id)
{
	struct msg_periodic *msg0=NULL;
	struct list_head *pos=NULL, *n=NULL;
	
//	printf("{\r\n");
	list_for_each_safe(pos, n, &periodic_head) {
		msg0 = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		if(msg0->id == id) {
			//printf("%s: msg0->id = 0x%04x\r\n", __func__, msg0->id);
			return msg0;
		}
	}		
	
	return NULL;
}

unsigned short calculate_crc(unsigned char *p, unsigned short n)
{
	unsigned short crc = 0;
	unsigned char da;
	while(n--)
	{
		da = crc >> 8;		//
		crc <<= 8;
		crc ^= crctable[da^*p++];
	}
	return crc;
}

/********************************************************************
解析与android系统通讯的串口所收到的数据，如果收到反馈包，返回UART_ACK
如果收到命令包返回UART_CMD，数据分别保存在ack 和cmd 中。
*********************************************************************/
int parse_uart6_fifo(void *fifo, char cmd[], int *cmd_len, char ack[], int *ack_len)
{
	static unsigned short recvCheck;
	static char data_len, isAck;
	static int state = HEAD1, i=0, n=0;	
	
	char data,tmp;
	unsigned short calCheck, ackCheckValue;
	int ret, tag, t;
	
	struct kfifo *mfifo = (struct kfifo *)fifo;
	//if (fifoClean) state = HEAD1;

LOOP:	
	switch(state) {
		
		case HEAD1:
			i = 0;
			n = 0;
			data_len = 0;
			recvCheck = 0;
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xaa) {
				*cmd = data;
				state++;
			} else if(ret == 1) {
				do{		   	
					ret = kfifo_get(mfifo, &data, 1);
			  }while(ret == 1 && data != 0xaa);
				
				if(ret == 0) {
					break;
				} else {
					*cmd = data;
					state++;
				}
			} else if(ret == 0) {
				break;
			} 
			
		case HEAD2:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xbb) {
				*(cmd+state) = data;			
				state++;
			} else if (ret == 0) {
				break;
			} else if (ret == 1) {
				state = HEAD1;
				goto LOOP;
			}
			
		case LEN:
			ret = kfifo_get(mfifo, &data, 1);
			if((ret == 1) && (data > 0) && (data < DATA_MAX_LEN)) {
				data_len = data;
				*(cmd+state) = data;			
				state++;	
			} else if (ret == 0){
				break;
			} else {
				state = HEAD1;
				goto LOOP;
			}
			
		case CMD:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1) {				
				*(cmd+state) = data;
#if ACK2				
				isAck = (data==0x80?1:0);
#else
				isAck = (data>>7) && 0x01;
#endif
				state++;
			} else {
				break;
			}
			
		case DATA:
			tag = 0;
			for( ; i<data_len-1; i++) {
				ret = kfifo_get(mfifo, &data, 1);
				if(ret == 1) {
					*(cmd+state+i) = data;
				} else {
					tag = 1;
					break;
				}
			}
			if(tag) {
				break;
			}
			state++;
			
		case CHECK:
			tag = 0;
			for(; n<2; n++) {
				ret = kfifo_get(mfifo, &data, 1);
				if(ret == 1) {
					if(n == 0) {						
						*(cmd+state+data_len-2) = data;
						recvCheck = data;
					} else if(n == 1) {
						*(cmd+state+data_len-1) = data;
						recvCheck = ((data << 8)& 0xff00) | recvCheck; 
					}
				} else {
					tag = 1;
					break;
				}		
			}
			if(tag) {
				break;
			}
			calCheck = calculate_crc((uint8*)cmd+LEN, data_len+1);		
		
			if(calCheck == recvCheck) {
				if(isAck) 
				{
					*ack_len = state+data_len;
					for(t=0; t<*ack_len; t++){
						*(ack+t) = *(cmd+t);
					}						
					state = HEAD1;	
					return UART_ACK;							
				} 
				else 
				{
					mfifo->acceptCmds++;
					*cmd_len = state+data_len;
#if ACK2	
					//0xaa 0xbb 0x03 0x80 c1 c2 check1 check2 -> ack message
					*(ack+ACK_HEAD1) = 0xaa;
					*(ack+ACK_HEAD2) = 0xbb;
					*(ack+ACK_DATA_LEN) = 0x03;
					*(ack+ACK_CMD) = 0x80;
					*(ack+ACK_D0) = *(cmd+*cmd_len-2);
					*(ack+ACK_D1) = *(cmd+*cmd_len-1);
					ackCheckValue = calculate_crc((uint8*)(ack+ACK_DATA_LEN), ACK_D1-ACK_HEAD2);							
					tmp = (unsigned char)(ackCheckValue & 0xff);		
					*(ack+ACK_CHECK_0) = tmp;
					tmp = (unsigned char)((ackCheckValue & 0xff00) >> 8);		
					*(ack+ACK_CHECK_1) = tmp;
					*ack_len = ACK_LEN;
#else
					for(t=0; t<*cmd_len; t++) {
						*(ack+t) = *(cmd+t);
					}
					
					*(ack+CMD) |= 0x80; 
					ackCheckValue = calculate_crc((uint8*)(ack+2), *cmd_len-4);							
					tmp = (unsigned char)(ackCheckValue & 0xff);			
					*(ack+*cmd_len-2) = tmp;
					tmp = (unsigned char)((ackCheckValue & 0xff00) >> 8);		
					*(ack+*cmd_len-1) = tmp;		
					*ack_len = *cmd_len;
#endif					
					state = HEAD1;
					return UART_CMD;
				}
			} else {			
				mfifo->errorCmds++;			
				printf("%s:errCmds=%d, isAck =%d, calCheck = %04x, recvCheck = %04x\r\n", 
				__func__, mfifo->errorCmds, isAck, calCheck, recvCheck);		
				
				//printf("len = %d\r\n", (state+data_len));
				for(t=0; t<(state+data_len); t++) {
					printf("0X%02X ", *(cmd+t));
				}				
				printf("\r\n");	
				
				state = HEAD1;	
				goto LOOP;
			}

		default:
			state = HEAD1;			
			goto LOOP;
		}

		return UART_NULL;
}

/***************************************************************
收到android回复的反馈包，从链表中删除这个反馈中对应的命令包，释放
内存，同时把event_sending置空。
****************************************************************/
void parse_cmd_ack(const char* ack, int ack_len)
{

	if(event_sending != NULL) {
#if ACK2	
		//0xaa 0xbb 0x03(LEN) 0x80(CMD) c1 c2 check1 check2 -> ack message
		if(ack_len == ACK_LEN) {
			if(*(ack+ACK_D0) != event_sending->data[event_sending->data_len-2]
					|| *(ack+ACK_D1) != event_sending->data[event_sending->data_len-1]) {
				report_car_event(event_sending->state, 
						event_sending->data, event_sending->data_len);	
				//report again right now!
						printf("%s: Get Error Ack From Android!\r\n", __func__);						
				return;
			}
			del_event_from_list(event_sending);
			if(event_sending->data != NULL) {
				myfree(0, event_sending->data);
				myfree(0, event_sending);
			}
			event_sending = NULL;
		} else {
			printf("%s->ack_len(%d) != ACK_LEN\r\n", __func__, ack_len);
		}		
#else
			
#endif				
	}	else {
		printf("%s: event_sending=null\r\n", __func__);
	}
}

/***************************************************************
								发送反馈包给android
****************************************************************/
void send_cmd_ack(const char* ack, int ack_len)
{
	u8 t;
	//printf("%s->start\r\n", __func__); 
	for(t=0;t<ack_len;t++) {
		USART_ClearFlag(USART6,USART_FLAG_TC); 
		//add for fix bug.
		//USART_GetFlagStatus(USART6, USART_FLAG_TC);
		USART_SendData(USART6, ack[t]);         //向串口6发送数据
		while(USART_GetFlagStatus(USART6,USART_FLAG_TC)!=SET);//等待发送结束
	}		
	//printf("%s->end\r\n", __func__);	
}

/***************************************************************
收到android下发的命令包，对命令包进行解析，处理各种请求
****************************************************************/
int do_uart_cmd(int result, const char* cmd, int cmd_len)
{
	u8 res, len, mCmd, n, canx;
	uint32_t id=0;
	uint8_t  ide, rtr;
	struct msg_periodic *msg = NULL;

/*****************************************************************
	BYTE:  cmd + PACKET_ITEM （低4位 用于区别android下发的命令包）
	CMD_CAN_EVENT  			0x01
	CMD_CAN_PERIODIC  	0x02
	CMD_RTC  						0x03
	CMD_ANDROID_STATE   0x04
	CMD_UPDATE_BIN      0x05	
	
	1、添加发到can上的单次包
	2、添加发到can上的（周期性动作）包
	3、添加操作RTC的命令包，包括读取rtc跟设置rtc
	4、添加android开机后，下发android完成开机的数据包，关机包
	5、添加android 更新MCU 固件的数据包
******************************************************************/
	mCmd = *(cmd+PACKET_ITEM) & 0x0f;
	
	//0xaa 0xbb len mCmd d0~d3(id) ide rtr data0 data1 ... check01 check02
	switch(mCmd)
	{
		case CMD_CAN_EVENT:
			numRecvAndroidCanCmd++;
			canx = *(cmd+CAN_CMD) >> 4 & 0x03;  //bit4-bit5=canx
			len = *(cmd+CAN_DATA_LEN)- (CAN_D0-CAN_CMD);	
			id |= ((*(cmd+CAN_ID_0)) << 24); 
			id |= ((*(cmd+CAN_ID_1)) << 16);
			id |= ((*(cmd+CAN_ID_2)) << 8); 
			id |=  (*(cmd+CAN_ID_3));	
			ide = *(cmd+CAN_IDE); 
			rtr = *(cmd+CAN_RTR);				
			//printf("can cmd. canx=%d\r\n", canx);
			if(canx == 1) {
				res = CAN1_Send_Msg2(id, ide, rtr, (u8 *)cmd+CAN_D0, len);
			} else if(canx == 2) {
				res = CAN2_Send_Msg2(id, ide, rtr, (u8 *)cmd+CAN_D0, len);
			} else {
				printf("can num error! num = %d\r\n", canx);
			}
			//printf("mCmd == 0x01->can num = %d\r\n", canx);
			if(res);
			else ;		
		break;

/*****************************************************
android系统下发的周期性命令包
0xaa 0xbb len mCmd periodic d0~d3(id) ide rtr data0 data1 .... check01 check02 
******************************************************/							
		case CMD_CAN_PERIODIC:
			numRecvAndroidCanCmd++;
			canx = *(cmd+CAN_CMD_P) >> 4 & 0x03;  //bit4~bit5   (bit6 not use) (bit7 不能使用！)						
			len = *(cmd+CAN_DATA_LEN_P)- (CAN_D0_P-CAN_CMD_P);
			n = *(cmd+CAN_PERIODIC_P);
			id |= ((*(cmd+CAN_ID_0_P)) << 24); 
			id |= ((*(cmd+CAN_ID_1_P)) << 16);
			id |= ((*(cmd+CAN_ID_2_P)) << 8); 
			id |=  (*(cmd+CAN_ID_3_P));		
			ide = *(cmd+CAN_IDE_P); 
			rtr = *(cmd+CAN_RTR_P);	
			//printf("mCmd == 0x02 ->can num = %d\r\n", canx);
			msg = check_periodic_msg(id);
			if(msg != NULL) {
				if(msg->len == len && msg->msg != NULL) {
					memcpy(msg->msg, cmd+CAN_D0_P, len);					
				} else if(msg->len != len && msg->msg != NULL) {
					myfree(0, msg->msg);
					msg->msg = mymalloc(0, len);
					if(msg->msg == NULL) {
						//printf("%s: *** msg->msg mymalloc fail!\r\n", __func__);
						del_msg(msg);
						myfree(0, msg);
					} else {					
						msg->len = len;
						memcpy(msg->msg, cmd+CAN_D0_P, len);							
					}
				}
			} else {
				msg = (struct msg_periodic*)mymalloc(0, sizeof(struct msg_periodic));
				if(msg == NULL) {
					printf("%s:msg mymalloc fail.\r\n", __func__);
				} else {
					msg->msg = (u8*)mymalloc(0, len);
					if(msg->msg == NULL) {
						printf("%s:msg->msg mymalloc fail.\r\n", __func__);
					} else {
						memcpy(msg->msg, cmd+11, len);
						msg->len = len; msg->id = id; msg->canx = canx;// can1 or can2;
						msg->ide = ide; msg->rtr = rtr; msg->periodic = &periodicNum;
						msg->update = periodicNum; msg->n = n; //n*100ms send one package
						add_msg_to_list(msg);						
					}
				}
			}				
		break;

/*****************************************************
android系统可设置 读取RTC的时间
0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02
******************************************************/				
		case CMD_RTC:
			switch(*(cmd+4))
			{
				RTC_DateTypeDef mDate;
				RTC_TimeTypeDef mTime;					
				unsigned char timeData[32], len;
				u8 hour, min, sec, ampm, year, month, date, week;

				case 0x01: 
					/* RTC_H12_AM RTC_H12_PM */
					/*RTC_Format_BCD RTC_Format_BIN*/
					get_sys_time(RTC_Format_BIN, &mDate, &mTime);
					timeData[0] = 0x01;//time message
				  timeData[1] = mDate.RTC_Year;
					timeData[2] = mDate.RTC_Month;
					timeData[3] = mDate.RTC_Date;
					timeData[4] = mDate.RTC_WeekDay;
					timeData[5] = mTime.RTC_Hours;
					timeData[6] = mTime.RTC_Minutes;
					timeData[7] = mTime.RTC_Seconds;
					timeData[8] = mTime.RTC_H12;
					len = 9;
					report_rtc_msg(timeData, len);
				  /*
					printf("%02d-%02d-%02d [WEEK=%d]\r\n", mDate.RTC_Year, mDate.RTC_Month,
									mDate.RTC_Date, mDate.RTC_WeekDay);
					printf("%02d:%02d:%02d [%s]\r\n", mTime.RTC_Hours, mTime.RTC_Minutes,
						mTime.RTC_Seconds, mTime.RTC_H12==RTC_H12_AM?"AM":"PM");						
					*/
					break;
				
				case 0x02:
					year = cmd[5]; month = cmd[6]; date = cmd[7]; week = cmd[8];
					hour = cmd[9]; min = cmd[10]; sec = cmd[11]; ampm = cmd[12];				
					RTC_Set_Date(year, month, date, week);
					RTC_Set_Time(hour, min, sec, ampm);	
					/*
					printf("%s: RTC_Set_Date. Y=%d, M=%d, D=%d, W=%d\r\n", 
									__func__, year, month, date, week);		
					printf("%s: RTC_Set_Time. h=%d, m=%d, s=%d, a=%d\r\n",
									__func__, hour, min, sec, ampm);				
					*/
					break;
				
				case 0x03://RTC_Set_AlarmA
					printf("RTC_Set_AlarmA.\r\n");
					week = cmd[5]; hour = cmd[6]; min = cmd[7]; sec = cmd[8];
					RTC_Set_AlarmA(week, hour, min, sec);
					break;
				
				case 0x04:
					break;
				//... add if necessary.
				default:
					printf("uart6 error command!!!\r\n");
					break;
			}		
			
		break;
			
/*****************************************************
android在开机成功后，会下发指令：AA BB 02 04 01 85 B2
在关机成功后，会下发指令：AA BB 02 04 00 A4 A2
******************************************************/
		case CMD_ANDROID_STATE:
			if(*(cmd+4)) {
				printf("%s: android start success!\r\n", __func__);
				//AA BB 02 04 01 85 B2
				mAndroidRunning = 1;
				/*开机之后log默认关闭调试串口的输出*/
//				mDebugUartPrintfEnable = 0;				
				
				/*Android运行后，上传当前MCU ID 及软件版本*/
				report_mcu_id_version();
				report_mcu_software_version();
			} else {
				printf("%s: android stop!\r\n", __func__);
				/*在关电前把log都上传到ANDROID*/
				report_debug_to_android();
				//AA BB 02 04 00 A4 A2 
				mAndroidRunning = 0;
				//关机命令起作用，android已关机，切断android电源。
				power_android(0); // cut the power
			}		
			break;

/***********************************************************
0xaa 0xbb len 0x05 d0 d1 d2 d3... check01 check02		
固件更新状态：在更新固件过程中返回TMODEM协议所要求的反馈			
************************************************************/			
		case CMD_UPDATE_BIN:
			
			return handle_update_bin(cmd, len);

/*用于上传MCU各个数据结构的数据到android中去，便于跟踪代码运行状态，
		从而实现mcu的log通过android的中转，可上传到服务器的功能*/		
		case CMD_DEBUG:		
			if(*(cmd+4) == 0x00) {
				/*调试信息上传给Android*/
				mDebugUartPrintfEnable = 0;
			}
			else if(*(cmd+4) == 0x01) {
				/*调试信息输出到调试串口*/
				mDebugUartPrintfEnable = 1;
			}	
			else if(*(cmd+4) == 0x02) {
				/*上报MCU运行的数据状态*/
				mReportMcuStatusPending = 1;
			}
				
			break;
		
		case CMD_OTHER: 
		break;
	
		default:
			break;
	}
/***********************************************************
非固件更新状态返回NOMAL 正常模式下
************************************************************/	
	return NOMAL;
}

void handle_downstream_work(char* cmd, char *ack)
{
	int result, cmd_len, ack_len, ret;

	//do{
	result = 0;
	if(kfifo_len(uart6_fifo) > 0) {
		result = parse_uart6_fifo(uart6_fifo, cmd, &cmd_len, ack, &ack_len);
		//解析uart6 fifo的数据
		if(result == UART_CMD) {
			send_cmd_ack(ack, ack_len);
			ret = do_uart_cmd(result, cmd, cmd_len);
			if(ret != NOMAL) 
				handle_tmodem_result(ret, ack, ack_len);
		} else if(result == UART_ACK) {
			parse_cmd_ack(ack, ack_len);
		}
	}
	//}while(result == UART_CMD || result == UART_ACK);
	//处理大屏send过来的数据，命令包或者应答包
	
	//处理定期下发到can的链表  periodic head.
	list_periodic_msg();
}

static int report_rtc_msg(unsigned char *msg, unsigned char len)
{
	char cmd[64], t;
	unsigned short checkValue;	

	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	cmd[2] = len+1;
	cmd[3] = 0x03;
	for(t=0; t<len; t++)
	{
		cmd[4+t] = msg[t];	
	}

	checkValue = calculate_crc((uint8*)cmd+LEN, len+2);
	cmd[4+t] = (unsigned char)(checkValue & 0xff);
	cmd[5+t] = (unsigned char)((checkValue & 0xff00) >> 8);	
	
	make_event_to_list(cmd, 6+t, CAN_EVENT, 0);
  
  return 0;	
}

static int report_mcu_version_msg(unsigned char *msg, unsigned char len)
{
	char cmd[128], t;
	unsigned short checkValue;	

	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	cmd[2] = len+1;
	cmd[3] = 0x0c;
	for(t=0; t<len; t++)
	{
		cmd[4+t] = msg[t];	
	}

	checkValue = calculate_crc((uint8*)cmd+LEN, len+2);
	cmd[4+t] = (unsigned char)(checkValue & 0xff);
	cmd[5+t] = (unsigned char)((checkValue & 0xff00) >> 8);	
	
	make_event_to_list(cmd, 6+t, CAN_EVENT, 0);
  
  return 0;	
}

const char mSoftVer[] = {'X', 'i', 'a', 'o', 'P','e', 'n', 'g', ' ', '2', '0', '1', '5', '-', '0', '1', '-', '0', '6'};
	
void report_mcu_software_version(void)
{
	unsigned char cmd[32], t;
	/*软件版本*/
	cmd[0] = 0x02;
	
	for(t=0; t<(sizeof(mSoftVer)/sizeof(char)); t++)
		cmd[t+1] = mSoftVer[t];
	
	cmd[t+1] = '\0';
	printf("%s: mcu software version [%s]\r\n", __func__, cmd+1);
	
	report_mcu_version_msg(cmd, t+1);
}

extern u32 mcuID[4];
void report_mcu_id_version(void)
{
	unsigned char cmd[32];
	/*硬件ID*/
	cmd[0] = 0x01;
	
	memcpy(&cmd[1], &mcuID[0], 4);
	memcpy(&cmd[5], &mcuID[1], 4);
	memcpy(&cmd[9], &mcuID[2], 4);	
	printf("%s: mcu ID [0X%X %8X %8X]\r\n", __func__, *((int*)(&cmd[1])), 
		*((int*)(&cmd[5])), *((int*)(&cmd[9])));
	report_mcu_version_msg(cmd, 13);
}





