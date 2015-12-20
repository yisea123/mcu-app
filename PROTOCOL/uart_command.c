#include "uart_command.h"
#include "tmodem.h"
#include "ioctr.h"

//#include "dma.h"

uint32_t periodicNum = 0;
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
int phase_uart6_fifo(void *fifo, char cmd[], int *cmd_len, char ack[], int *ack_len, int fifoClean)
{
	static unsigned short recvCheck;
	static char data_len, isAck;
	static int state = HEAD1, i=0, n=0;	
	
	char data,tmp;
	unsigned short calCheck, ackCheckValue;
	int ret, tag, t;
	
	struct kfifo *mfifo = (struct kfifo *)fifo;
	if (fifoClean) state = HEAD1;

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

long numRecvT8Cmd = 0;

void get_t8(void)
{
	printf("numRecvT8Cmd=%ld\r\n", numRecvT8Cmd);
}

/***************************************************************
收到android回复的反馈包，从链表中删除这个反馈中对应的命令包，释放
内存，同时把event_sending置空。
****************************************************************/
void phase_cmd_ack(const char* ack, int ack_len)
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
				printf("->e\r\n");						
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
	
	for(t=0;t<ack_len;t++) {
		USART_ClearFlag(USART6,USART_FLAG_TC); 
		//add for fix bug.
		//USART_GetFlagStatus(USART6, USART_FLAG_TC);
		USART_SendData(USART6, ack[t]);         //向串口6发送数据
		while(USART_GetFlagStatus(USART6,USART_FLAG_TC)!=SET);//等待发送结束
	}			
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

	//大屏的命令包, 下发信息到can 上，或者读取rtc时间，或者设置时间。
	mCmd = *(cmd+CAN_CMD) & 0x0f;  //bit0~bit3 -> (0~15)  
	
	//0xaa 0xbb len mCmd d0~d3(id) ide rtr data0 data1 ... check01 check02
	switch(mCmd)
	{
		case CMD_CAN_EVENT:
			numRecvT8Cmd++;
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
			numRecvT8Cmd++;
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
				u8 hour, min, sec, ampm, year, month, date, week;

				case 0x01: //RTC_Set_Time RTC_H12_AM RTC_H12_PM
					printf("RTC_Set_Time.\r\n");
					hour = cmd[5]; min = cmd[6]; sec = cmd[7]; ampm = cmd[8];
					RTC_Set_Time(hour, min, sec, ampm);
					break;
				case 0x02://RTC_Set_Date
					printf("RTC_Set_Date.\r\n");
					year = cmd[5]; month = cmd[5]; date = cmd[7]; week = cmd[8];
					RTC_Set_Date(year, month, date, week);
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
				printf("%s: android start!\r\n", __func__);
				//AA BB 02 04 01 85 B2
				mAndroidRunning = 1;
				mAndroidPower=1;//for test , del after...
			} else {
				printf("%s: android stop!\r\n", __func__);
				//AA BB 02 04 00 A4 A2 
				mAndroidRunning = 0;
				//关机命令起作用，android已关机，切断android电源。
				power_android(0); // cut the power
			}		
			break;

		case CMD_UPDATE_BIN:
			
			return handle_update_bin(cmd, len);
		
		case CMD_OTHER: 
		break;
	
		default:
			break;
	}

	return 0;
}

/********************************************************************
用于周期性命令包的时间记数，所有周期性命令包中的一个字段都指向了
periodicNum全局变量，这个变量会每100MS自加加一次。
*********************************************************************/
void TIM2_IRQHandler(void)
{ 		    		  			    
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)//溢出中断
	{
		++periodicNum;
		TIM_SetCounter(TIM2,0);		//清空定时器的CNT
		TIM_SetAutoreload(TIM2,1000);//恢复原来的设置		  
		//100ms中断一次			
	}			
	
	TIM_ClearITPendingBit(TIM2,TIM_IT_Update);  //清除中断标志位    
}

/*********************************************************************
使能定时器4,使能中断.
Timer3_Init(1000,(u32)sysclk*100-1);//分频,时钟为10K ,100ms中断一次
用于给全局变量 periodicNum 每100毫秒加一。
***********************************************************************/
void Timer2_Init(u16 arr,u16 psc)
{
	NVIC_InitTypeDef   NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);  ///使能TIM3时钟

	TIM_TimeBaseInitStructure.TIM_Prescaler=psc;  //定时器分频
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up; //向上计数模式
	TIM_TimeBaseInitStructure.TIM_Period=arr;   //自动重装载值
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1; 
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//初始化定时器3
	
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE); //允许定时器3更新中断
	TIM_Cmd(TIM2,ENABLE); //使能定时器4
 
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//外部中断3
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//抢占优先级3
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//子优先级3
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//使能外部中断通道
  NVIC_Init(&NVIC_InitStructure);//配置NVIC
	 							 
}

/*****************************************************************

		if(event_sending->data_len == ack_len) {
			for(t=0; t< (ack_len -2); t++) {
				if(t == ACK_CMD) {
					if(event_sending->data[t] != (ack[t]&0x7f)) {
						printf("%s->data=%02x, ack=%02x, event_sending->data[%d] != ack[%d]\r\n", 
							__func__, event_sending->data[t], ack[t], t, t);
				
						printf("%s->event_sending:\r\n", __func__);
						for(t=0; t<event_sending->data_len; t++) {
							printf("0X%02X ", *(event_sending->data+t));
						}
						printf("\r\n");
						printf("%s->ack:\r\n", __func__);
						for(t=0; t<event_sending->data_len; t++) {
							printf("0X%02X ", *(ack+t));
						}
						printf("\r\n");					
					
						report_car_event(event_sending->state, 
								event_sending->data, event_sending->data_len);	
						//report again right now!
						printf("->e1\r\n");					
						return;								
					}
				} else {
					if(event_sending->data[t] != ack[t]) {
						printf("%s->data=%02x, ack=%02x, event_sending->data[%d] != ack[%d]\r\n", 
							__func__, event_sending->data[t], ack[t], t, t);
			
						printf("%s->event_sending:\r\n", __func__);
						for(t=0; t<event_sending->data_len; t++) {
							printf("0X%02X ", *(event_sending->data+t));
						}
						printf("\r\n");
						printf("%s->ack:\r\n", __func__);
						for(t=0; t<event_sending->data_len; t++) {
							printf("0X%02X ", *(ack+t));
						}
						printf("\r\n");					
						
						report_car_event(event_sending->state, 
								event_sending->data, event_sending->data_len);	
						report again right now!
						printf("->e2\r\n");					
						return;
					}
				}
			}
			
			del_event(event_sending);
			if(event_sending->data != NULL) {
				myfree(0, event_sending->data);
				myfree(0, event_sending);
			}
			event_sending = NULL;
			//list_event();
		} else {
			printf("%s->data_len(%d) != ack_len(%d)\r\n", __func__, event_sending->data_len, ack_len);
		}	




*******************************************************************/









