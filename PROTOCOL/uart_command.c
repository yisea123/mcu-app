#include "uart_command.h"
#include "tmodem.h"
#include "ioctr.h"
#include "rtc.h"
#include <string.h>

static int report_rtc_msg(unsigned char *msg, unsigned char len);
static int report_mcu_version_msg(unsigned char *msg, unsigned char len);
void report_mcu_software_version(void);
void report_mcu_id_version(void);

long numRecvAndroidCanCmd = 0;

static uint32_t periodicNum = 0;
static uint32_t mEventCount	 = 0;
static TIMER2* mEventTimer = NULL;

char mReportMcuStatusPending=0;

struct list_head periodic_head;
struct list_head mEventHead;
struct list_head periodic_wait_head;

static const unsigned short crctable[256] =
{
	0x0000 , 0x1021 , 0x2042 , 0x3063 , 0x4084 , 0x50a5 , 0x60c6 , 0x70e7 , 0x8108 , 0x9129 ,
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

static u8 send_can_message(u8 canx, uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{
	u8 res = 0;
	
	if( canx == 1) 
	{
		res = CAN1_Send_Msg2(id, ide, rtr, msg, len);
	} 
	else if( canx == 2 )
	{
		res = CAN2_Send_Msg2(id, ide, rtr, msg, len);
	} 
	else 
	{
		printf("can num error! num = %d\r\n", canx);
	}	
	
	return res;
}

static void add_msg_to_list(struct list_head* list, struct list_head *head)
{
	list_add_tail(list, head);
}

static void del_msg_from_list(struct msg_periodic *msg, struct list_head *head)
{
	struct msg_periodic *msg0;
	struct list_head *pos, *n;
	
	if(msg == NULL) return;
	
	list_for_each_safe(pos, n, head) 
	{ 
		msg0 = list_entry(pos, struct msg_periodic, list); 
		
		if(msg0 == msg) 
		{ 
				list_del(pos); 
		} 
	 } 
}

/************************************************************************
static struct msg_periodic * get_msg_from_list(struct list_head *head)
{
	struct list_head *pos, *n;	
	struct msg_periodic *msg=NULL;
	
	list_for_each_safe(pos, n, head)
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		break;
	}	

	return msg;
}
************************************************************************/

static void list_periodic_wait(struct msg_periodic *msg)
{
	struct msg_periodic *msg_wait = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe(pos, n, &periodic_wait_head)
	{
		msg_wait = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		
		if(msg->id == msg_wait->id)
		{
			memcpy(msg->msg, msg_wait->msg, msg->len);
			msg->is_idle = msg_wait->is_idle;
			msg->send_status = msg_wait->send_status;
			msg->try_count = msg_wait->try_count;
	
			list_del(pos);			
//			del_msg_from_list(msg_wait, &periodic_wait_head);
/*			printf("%s: del msg from periodic_wait_head id=0x%04x. [%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x]\r\n", 
								__func__, msg->id, 
								*(msg->msg),*(msg->msg+1),*(msg->msg+2),*(msg->msg+3),
								*(msg->msg+4),*(msg->msg+5),*(msg->msg+6),*(msg->msg+7));		*/	
			myfree(0, msg_wait->msg);
			myfree(0, msg_wait);
			break;
		}
	}	
}

/*******************************************************************
����������Ϣ���������ÿ����Ϣ�Ľṹ�壬���ڷ�����Ϣ��
********************************************************************/

static void list_periodic_msg()
{
	u8 res;
	struct msg_periodic *msg = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe(pos, n, &periodic_head)
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		
		if((*(msg->periodic)- msg->update) >= msg->n)
		{
			msg->update = *(msg->periodic);
			
			send_can_message(msg->canx, msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
			
			if(!msg->is_idle || (msg->is_idle && !msg->send_status))
				printf("%s: **** id=0x%03x: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x.\r\n", 
								__func__, msg->id, 
								*(msg->msg),*(msg->msg+1),*(msg->msg+2),*(msg->msg+3),
								*(msg->msg+4),*(msg->msg+5),*(msg->msg+6),*(msg->msg+7));
			
			if(msg->is_idle)
			{
				if(msg->try_count > 0)
					msg->try_count--;

				if(msg->try_count <= 0)
				{
					msg->send_status = 1;	
					list_periodic_wait(msg);
				}	
			}
			else if(!msg->is_idle && !msg->send_status)
			{
				if((--msg->try_count) <= 0)
				{
					memset(msg->msg, 0, msg->len);
					msg->is_idle = 1;
					msg->send_status = 0;
				}
			}
			
			if(res);
			else ;
		}	
	}	
}

static void list_periodic_message()
{
	u8 res;
	struct msg_periodic *msg = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe(pos, n, &periodic_head)
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		
		if((*(msg->periodic)- msg->update) >= msg->n)
		{
			msg->update = *(msg->periodic);
			send_can_message(msg->canx, msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
			
			if(!msg->send_status)
				printf("%s: **** id=0x%03x: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x.\r\n", 
								__func__, msg->id, 
								*(msg->msg),*(msg->msg+1),*(msg->msg+2),*(msg->msg+3),
								*(msg->msg+4),*(msg->msg+5),*(msg->msg+6),*(msg->msg+7));			
			msg->send_status = 1;
			
			if(res);
			else ;
		}	
	}	
}

/*base on xiaopeng DOC*/
static void list_event_message(void)
{
	u8 res;
	MSGEVENT* msg = NULL;	
	unsigned int total = 0;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe(pos, n, &mEventHead)
	{
		total++;
		msg = (MSGEVENT *)list_entry(pos, MSGEVENT, list);
		
		if((*(msg->pEventCount)- msg->update) >= msg->n)
		{
			msg->update = *(msg->pEventCount);
			
			if(msg->step == STEP0)
			{
					msg->step = STEP1;
					msg->n = 2;/*call in 40ms later*/
					msg->trys--;
					send_can_message(msg->canx, msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
				
					/*printf must after send_can_message for fix bug. because cost 6ms to printf*/
					printf("%s: ****trys(%02d) [0x%03x]: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x.\r\n", 
									__func__, msg->trys, msg->id, 
									*(msg->msg),*(msg->msg+1),*(msg->msg+2),*(msg->msg+3),
									*(msg->msg+4),*(msg->msg+5),*(msg->msg+6),*(msg->msg+7));					
					continue;
			}
			else
			{
				if(msg->trys > 0)
				{		
						msg->trys--;
						send_can_message(msg->canx, msg->id, msg->ide, msg->rtr, msg->msg, msg->len);
				}
				else
				{
						list_del(pos);			
						myfree(0, msg);	
				}
			}
		}	
	}

	/*if there haves not Event Message, delete timer.*/	
  if(mEventTimer && total == 0)
	{
		 if(unregister_timer2(mEventTimer) == 0)
		 {
				mEventTimer = NULL;
		 }
	}
}

/*******************************************************************
�������ݵ�ID��ȷ�������Ƿ���Ҫ������
********************************************************************/

static struct msg_periodic * check_periodic_msg(uint32_t id, struct list_head *head)
{
	struct msg_periodic *msg=NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, head) 
	{
		msg = (struct msg_periodic *)list_entry(pos, struct msg_periodic, list);
		
		if(msg->id == id) 
		{
			return msg;
		}
	}		
	
	return NULL;
}

static MSGEVENT * check_event_msg(uint32_t id, struct list_head *head)
{
	MSGEVENT *msg = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, head) 
	{
		msg = (MSGEVENT *)list_entry(pos, MSGEVENT, list);
		
		if(msg->id == id) 
		{
			return msg;
		}
	}		
	
	return NULL;
}

static struct msg_periodic* malloc_periodic_message(int len)
{
	struct msg_periodic* msg = NULL;
	
	msg = (struct msg_periodic*)mymalloc(0, sizeof(struct msg_periodic));

	if(msg == NULL)
	{
		printf("%s:msg_wait mymalloc fail.\r\n", __func__);
	} 
	else 
	{
		msg->msg = (u8*)mymalloc(0, len);
		
		if(msg->msg == NULL) 
		{
			printf("%s:msg_wait->msg mymalloc fail.\r\n", __func__);
			myfree(0, msg);
			msg = NULL;
		} 
	}

	return msg;
}

static MSGEVENT* malloc_event_message(void)
{
	MSGEVENT* msg = NULL;
	
	msg = (MSGEVENT *)mymalloc(0, sizeof(MSGEVENT));

	if(msg == NULL)
	{
		printf("%s: MSGEVENT mymalloc fail.\r\n", __func__);
	}
	return msg;
}

unsigned short calculate_crc(unsigned char *p, unsigned short n)
{
	unsigned short crc = 0;
	unsigned char da;
	
	while(n--)
	{
		da = crc >> 8;
		crc <<= 8;
		crc ^= crctable[da^*p++];
	}
	
	return crc;
}

/********************************************************************
������androidϵͳͨѶ�Ĵ������յ������ݣ�����յ�������������UART_ACK
����յ����������UART_CMD�����ݷֱ𱣴���ack ��cmd �С�
*********************************************************************/

int parse_uart6_fifo(void *fifo, char cmd[], int *cmd_len, char ack[], int *ack_len)
{
	static unsigned short recvCheck;
	static char data_len, isAck;
	static int state = HEAD1, i=0, n=0;	
	
	char data,tmp;
	int ret, tag, t;
	unsigned short calCheck, ackCheckValue;
	
	struct kfifo *mfifo = (struct kfifo *)fifo;
	/*if (fifoClean) state = HEAD1;*/

LOOP:	
	switch(state) 
	{
		case HEAD1:
			i = 0;
			n = 0;
			data_len = 0;
			recvCheck = 0;
			ret = kfifo_get(mfifo, &data, 1);
		
			if(ret == 1 && data == 0xaa) 
			{
				*cmd = data;
				state++;
			} 
			else if(ret == 1) 
			{
				do{		   	
					ret = kfifo_get(mfifo, &data, 1);
			  }while(ret == 1 && data != 0xaa);
				
				if(ret == 0) 
				{
					break;
				} 
				else 
				{
					*cmd = data;
					state++;
				}
			} 
			else if(ret == 0) 
			{
				break;
			} 
			
		case HEAD2:
			ret = kfifo_get(mfifo, &data, 1);
		
			if(ret == 1 && data == 0xbb) 
			{
				*(cmd+state) = data;			
				state++;
			} 
			else if (ret == 0) 
			{
				break;
			} 
			else if (ret == 1) 
			{
				state = HEAD1;
				goto LOOP;
			}
			
		case LEN:
			ret = kfifo_get(mfifo, &data, 1);
		
			if((ret == 1) && (data > 0) && (data < DATA_MAX_LEN)) 
			{
				data_len = data;
				*(cmd+state) = data;			
				state++;	
			} 
			else if (ret == 0)
			{
				break;
			}
			else 
			{
				state = HEAD1;
				goto LOOP;
			}
			
		case CMD:
			ret = kfifo_get(mfifo, &data, 1);
		
			if(ret == 1) 
			{				
				*(cmd+state) = data;
#if ACK2				
				isAck = (data==0x80?1:0);
#else
				isAck = (data>>7) && 0x01;
#endif
				state++;
			} 
			else 
			{
				break;
			}
			
		case DATA:
			tag = 0;
		
			for( ; i<data_len-1; i++) 
			{
				ret = kfifo_get(mfifo, &data, 1);
				
				if(ret == 1)
				{
					*(cmd+state+i) = data;
				} 
				else					
				{
					tag = 1;
					break;
				}
			}
			
			if(tag) 
			{
				break;
			}
			state++;
			
		case CHECK:
			tag = 0;
		
			for(; n<2; n++) 
			{
				ret = kfifo_get(mfifo, &data, 1);
				
				if(ret == 1) 
				{
					if(n == 0) 
					{						
						*(cmd+state+data_len-2) = data;
						recvCheck = data;
					} 
					else if(n == 1) 
					{
						*(cmd+state+data_len-1) = data;
						recvCheck = ((data << 8)& 0xff00) | recvCheck; 
					}
				} 
				else 
				{
					tag = 1;
					break;
				}		
			}
			if(tag) 
			{
				break;
			}
			calCheck = calculate_crc((uint8*)cmd+LEN, data_len+1);		
		
			if(calCheck == recvCheck) 
			{
				if(isAck) 
				{
					*ack_len = state+data_len;
					for(t=0; t<*ack_len; t++)
					{
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
					/*0xaa 0xbb 0x03 0x80 c1 c2 check1 check2 -> ack message*/
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
					for(t=0; t<*cmd_len; t++) 
					{
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
			} 
			else 
			{			
				mfifo->errorCmds++;			
				printf("%s:errCmds=%d, isAck =%d, calCheck = %04x, recvCheck = %04x\r\n", 
				__func__, mfifo->errorCmds, isAck, calCheck, recvCheck);		
				
				for(t=0; t<(state+data_len); t++) 
				{
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
�յ�android�ظ��ķ���������������ɾ����������ж�Ӧ����������ͷ�
�ڴ棬ͬʱ��event_sending�ÿա�
****************************************************************/

void parse_cmd_ack(const char* ack, int ack_len)
{

	if(event_sending != NULL) 
	{
		/*0xaa 0xbb 0x03(LEN) 0x80(CMD) c1 c2 check1 check2 -> ack message*/
		if(ack_len == ACK_LEN) 
		{
			if(*(ack+ACK_D0) != event_sending->data[event_sending->data_len-2]
					|| *(ack+ACK_D1) != event_sending->data[event_sending->data_len-1]) 
			{
				report_car_event(event_sending->state, 
						event_sending->data, event_sending->data_len);	
				/*report again right now!*/
						printf("%s: Get Error Ack From Android!\r\n", __func__);						
				return;
			}
			del_event_from_list(event_sending);
			
			if(event_sending->data != NULL) 
			{
				myfree(0, event_sending->data);
				myfree(0, event_sending);
			}
			event_sending = NULL;
		} 
		else 
		{
			printf("%s->ack_len(%d) != ACK_LEN\r\n", __func__, ack_len);
		}			
	}	
	else 
	{
		printf("%s: event_sending=null\r\n", __func__);
	}
}

/***************************************************************
								���ͷ�������android
****************************************************************/

void send_cmd_ack(const char* ack, int ack_len)
{
	u8 t;
	
	for(t=0;t<ack_len;t++) 
	{
		USART_ClearFlag(USART6,USART_FLAG_TC); 
		/*USART_GetFlagStatus(USART6, USART_FLAG_TC); add for fix bug.*/
		USART_SendData(USART6, ack[t]);
		while(USART_GetFlagStatus(USART6,USART_FLAG_TC)!=SET);
	}			
}

static void update_eventcount_timer(void *argc)
{
		uint32_t *mEventCount = (uint32_t*)argc;
		
		(*mEventCount)++;
		//++mEventCount;
		
		if((*mEventCount%1000) == 0)
			printf("%s: *** mEventCount=%d\r\n", __func__, *mEventCount);
}

static void can_event_message(const char* cmd)
{
	MSGEVENT *msg = NULL;
	u8 res, len, canx;
	uint32_t id=0;
	uint8_t  ide, rtr, trys;
	
	numRecvAndroidCanCmd++;
	
	canx = *(cmd+CAN_CMD) >> 4 & 0x03;  /*bit4-bit5=canx*/
	len = *(cmd+CAN_DATA_LEN)- (CAN_D0-CAN_CMD);	
	id |= ((*(cmd+CAN_ID_0)) << 24); 
	id |= ((*(cmd+CAN_ID_1)) << 16);
	id |= ((*(cmd+CAN_ID_2)) << 8); 
	id |=  (*(cmd+CAN_ID_3));	
	trys = *(cmd+CAN_IDE); 
	ide = 0;
	rtr = 0;
	
	if(sizeof(msg->msg) != len)
		printf("%s: len = %d error.\r\n", __func__, len);
	
	msg = check_event_msg(id, &mEventHead);
	
	if(msg)
	{
		memcpy(msg->msg, cmd+CAN_D0, len);
		msg->trys = EVENT_MSG_TRYS;//4;
	} 
	else
	{
		msg = malloc_event_message();
		
		if(msg)
		{
			msg->canx = canx;
			msg->id = id;
			msg->ide = ide;
			msg->rtr = rtr;
			memcpy(msg->msg, cmd+CAN_D0, sizeof(msg->msg));
			msg->len = len;
			msg->step = STEP0;
			msg->pEventCount = &mEventCount;
			msg->update = mEventCount;
			msg->n = 1; /*call in 1ms~20ms most*/
			msg->trys = EVENT_MSG_TRYS;//4;//trys;
			add_msg_to_list(&(msg->list), &mEventHead);
		}
	}
	
	if(mEventTimer == NULL)
	{
		mEventTimer = register_timer2("event_can", TIMER2PERIOD, update_eventcount_timer, REPEAT, &mEventCount);		
	}			
}

static void can_periodic_message(const char* cmd)
{
	u8 len, canx, n;
	uint32_t id=0;
	uint8_t  ide, rtr, trys;
	struct msg_periodic *msg = NULL;
	
	numRecvAndroidCanCmd++;
	
	canx = *(cmd+CAN_CMD_P) >> 4 & 0x03;  
	/*bit4~bit5   (bit6 not use) (bit7 ����ʹ�ã�)	*/					
	len = *(cmd+CAN_DATA_LEN_P)- (CAN_D0_P-CAN_CMD_P);
	n = *(cmd+CAN_PERIODIC_P);
	id |= ((*(cmd+CAN_ID_0_P)) << 24); 
	id |= ((*(cmd+CAN_ID_1_P)) << 16);
	id |= ((*(cmd+CAN_ID_2_P)) << 8); 
	id |=  (*(cmd+CAN_ID_3_P));		
	trys = *(cmd+CAN_IDE_P); 
	ide = 0;
	rtr = 0;
	
	if(trys <= 0)
		trys = 1;
	
	if(len != CAN_DATA_VALUE_LEN)
	{
		printf("%s: ERROR CAN DATA VALUE LEN! trys=%d\r\n", __func__, trys);
		return;
	}
	else 
	{
		printf("%s: ### trys=%d, id=0x%04x: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x.\r\n", 
							__func__, trys, id, 
							*(cmd+CAN_D0_P),*(cmd+CAN_D1_P),*(cmd+CAN_D2_P),*(cmd+CAN_D3_P),
							*(cmd+CAN_D4_P),*(cmd+CAN_D5_P),*(cmd+CAN_D6_P),*(cmd+CAN_D7_P));
	}		
	msg = check_periodic_msg(id, &periodic_head);

	if(msg != NULL) 
	{
		memcpy(msg->msg, cmd+CAN_D0_P, len);
		msg->try_count = trys;
		msg->send_status = 0;
	} 
	else 
	{
		msg  = malloc_periodic_message(len);
		
		if(msg)
		{
			memcpy(msg->msg, cmd+CAN_D0_P, len);
			msg->try_count = trys;
			msg->len = len; 
			msg->id = id; 
			msg->canx = canx;
			msg->ide = ide; 
			msg->rtr = rtr; 
			msg->periodic = &periodicNum;
			msg->update = periodicNum; 
			msg->n = n;
			msg->send_status = 0;
			
			add_msg_to_list(&(msg->list), &periodic_head);
		}
	}			
}

/*****************************************************
androidϵͳ������ ��ȡRTC��ʱ��
0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02
******************************************************/		

static void on_rtc_event(const char* cmd)
{
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
			timeData[0] = 0x01;
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
		/*printf("%02d-%02d-%02d [WEEK=%d]\r\n", mDate.RTC_Year, mDate.RTC_Month,
							mDate.RTC_Date, mDate.RTC_WeekDay);
			printf("%02d:%02d:%02d [%s]\r\n", mTime.RTC_Hours, mTime.RTC_Minutes,
				mTime.RTC_Seconds, mTime.RTC_H12==RTC_H12_AM?"AM":"PM");*/						
			
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
		
		case 0x03:/*RTC_Set_AlarmA*/
			printf("RTC_Set_AlarmA.\r\n");
			week = cmd[5]; hour = cmd[6]; min = cmd[7]; sec = cmd[8];
			RTC_Set_AlarmA(week, hour, min, sec);
			break;
		
		case 0x04:
			break;
		/*... add if necessary.*/
		default:
			printf("uart6 error command!!!\r\n");
			break;
	}			
}

/*****************************************************
android�ڿ����ɹ��󣬻��·�ָ�AA BB 02 04 01 85 B2
�ڹػ��ɹ��󣬻��·�ָ�AA BB 02 04 00 A4 A2
******************************************************/

static void on_cmd_android_status(const char* cmd)
{
	if(*(cmd+4)) 
	{
		printf("%s: android start success!\r\n", __func__);
		/*AA BB 02 04 01 85 B2*/
		mAndroidRunning = 1;
		/*����֮��logĬ�Ϲرյ��Դ��ڵ����*/
		mDebugUartPrintfEnable = 1;		
		/*0 release soft !!!jzyang*/		
		
		/*Android���к��ϴ���ǰMCU ID ������汾*/
		report_mcu_id_version();
		report_mcu_software_version();
	} 
	else 
	{
		printf("%s: android stop!\r\n", __func__);
		/*�ڹص�ǰ��log���ϴ���ANDROID*/
		report_debug_to_android();
		/*AA BB 02 04 00 A4 A2*/ 
		mAndroidRunning = 0;
		/*�ػ����������ã�android�ѹػ����ж�android��Դ��*/
		power_android(0);
	}			
}

/*****************************************************************
�����ϴ�MCU�������ݽṹ�����ݵ�android��ȥ�����ڸ��ٴ�������״̬��
		�Ӷ�ʵ��mcu��logͨ��android����ת�����ϴ����������Ĺ���
******************************************************************/		

static void on_debug_cmd(const char* cmd)
{
	if(*(cmd+4) == 0x00) 
	{
		/*������Ϣ�ϴ���Android*/
		mDebugUartPrintfEnable = 0;
	}
	else if(*(cmd+4) == 0x01) 
	{
		/*������Ϣ��������Դ���*/
		mDebugUartPrintfEnable = 1;
	}	
	else if(*(cmd+4) == 0x02) 
	{
		/*�ϱ�MCU���е�����״̬*/
		mReportMcuStatusPending = 1;
	}	
}

/***************************************************************
�յ�android�·��������������������н����������������

BYTE:  cmd + PACKET_ITEM ����4λ ��������android�·����������
CMD_CAN_EVENT  			0x01
CMD_CAN_PERIODIC  	0x02
CMD_RTC  						0x03
CMD_ANDROID_STATE   0x04
CMD_UPDATE_BIN      0x05	

1����ӷ���can�ϵĵ��ΰ�
2����ӷ���can�ϵģ������Զ�������
3����Ӳ���RTC���������������ȡrtc������rtc
4�����android�������·�android��ɿ��������ݰ����ػ���
5�����android ����MCU �̼������ݰ�

androidϵͳ�·��������������

0xaa 0xbb len mCmd periodic d0~d3(id) ide rtr data0 data1 .... check01 check02 
	

0xaa 0xbb len mCmd d0~d3(id) ide rtr data0 data1 ... check01 check02
***************************************************************/

//����can���ݷ��͵�����ide��Ϊ���������ݵ��������͸����� 0<n<255
int do_uart_cmd(int result, const char* cmd, int cmd_len)
{
	u8 mCmd;

	mCmd = *(cmd+PACKET_ITEM) & 0x0f;
	
	switch(mCmd)
	{
		case CMD_CAN_EVENT:
			can_event_message(cmd);
			break;
				
		case CMD_CAN_PERIODIC:
			can_periodic_message(cmd);
			break;
		
		case CMD_RTC:
			on_rtc_event(cmd);
			break;
			
		case CMD_ANDROID_STATE:
			on_cmd_android_status(cmd);
			break;
	
		case CMD_UPDATE_BIN:
			return handle_update_bin(cmd, cmd_len);

		case CMD_DEBUG:		
			on_debug_cmd(cmd);
			break;
		
		case CMD_OTHER: 
		break;
	
		default:
			break;
	}

	return NOMAL;
}

void handle_downstream_work(char* cmd, char *ack)
{
	int result = 0, cmd_len, ack_len, ret;
	
	if(kfifo_len(uart6_fifo) > 0) 
	{
		result = parse_uart6_fifo(uart6_fifo, cmd, &cmd_len, ack, &ack_len);
		/*����uart6 fifo������*/
		if(result == UART_CMD) 
		{
			send_cmd_ack(ack, ack_len);
			ret = do_uart_cmd(result, cmd, cmd_len);
			
			if(ret != NOMAL) 
				handle_tmodem_result(ret, ack, ack_len);
		} 
		else if(result == UART_ACK) 
		{
			parse_cmd_ack(ack, ack_len);
		}
	}
	
	/*�������·���can������  periodic head.*/
	//list_periodic_msg();
	list_periodic_message();
	list_event_message();
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

const char mSoftVer[] = {'X', 'i', 'a', 'o', 'P','e', 'n', 'g', ' ', '2', '0', '1', '6', '-', '0', '4', '-', '2', '5'};
	
void report_mcu_software_version(void)
{
	unsigned char cmd[32], t;
	/*����汾*/
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
	/*Ӳ��ID*/
	cmd[0] = 0x01;
	
	memcpy(&cmd[1], &mcuID[0], 4);
	memcpy(&cmd[5], &mcuID[1], 4);
	memcpy(&cmd[9], &mcuID[2], 4);	
	printf("%s: mcu ID [0X%X %8X %8X]\r\n", __func__, *((int*)(&cmd[1])), 
		*((int*)(&cmd[5])), *((int*)(&cmd[9])));
	report_mcu_version_msg(cmd, 13);
}

/*need to invoved in every 100ms.*/
static void update_periodicnum_timer(void *argc)
{
		uint32_t *periodicNum = (uint32_t *) argc;
		
		(*periodicNum)++;
		
		/*if((*periodicNum%100) == 0)
			printf("%s: *** periodicNum=%d\r\n", __func__, *periodicNum);
		*/
}

void uart_command_init(void)
{
		INIT_LIST_HEAD(&mEventHead);
		INIT_LIST_HEAD(&periodic_head);   	
		/*���ڴ��Ҫ�·����¼����¼���uart6���������·���can�ϣ�Ϊ�̶������¼���*/	
	
		/*register_timer2("uart_command0", TIMER2PERIOD, eventcount_update_timer, REPEAT);*/
		register_timer2("periodic_num", TIMER2SECOND/10, update_periodicnum_timer, REPEAT, &periodicNum);
}



