#include "car_event.h"
#include "uart_command.h"
#include "ioctr.h"

struct list_head event_head;
struct car_event* event_sending = NULL;
long count_cmd = 0;
long numMcuSendedCmdToAndroid = 0;
long numMcuReportToAndroid = 0;
extern long num_can1_IRQ, can1_report_bytes;


/***************************************************************
把数据加入到链表中去
****************************************************************/
void add_event_to_list(struct car_event *event)
{
	numMcuReportToAndroid++;

	if(event)
		list_add_tail(&event->list, &event_head); //add to tail
}

/***************************************************************
如果此数据可以在链表中找到，从链表中删除这个数据
****************************************************************/
void del_event_from_list(struct car_event *event)
{
	struct car_event *mevent = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &event_head) 
	{ 
		mevent = list_entry(pos, struct car_event, list); 
		if(mevent == event && event != NULL) 
		{ 
				list_del(pos); 
				//myfree(0, event); 
		} 
	 } 
}

/***************************************************************
如果链表里面没有数据，则返回NULL，如果有数据就返回一个数据
****************************************************************/
struct car_event* get_event_from_list()
{
	struct car_event *event = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &event_head)
	{
		event = (struct car_event *)list_entry(pos, struct car_event, list);
		break;
	}	

	return event;
}

int list_event(void)
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &event_head)
	{
		count++;	
	}
	return count;
}

struct car_event * check_event(struct car_event *event)
{
	struct car_event *event0 = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
//	printf("{\r\n");
	list_for_each_safe(pos, n, &event_head) {
		event0 = (struct car_event *)list_entry(pos, struct car_event, list);
		if(event0->ID == event->ID && event0->data_len == event->data_len ) {
			//printf("ID= %X\r\n", event0->ID);
			return event0;
		}
	}		
	return NULL;
}

/***************************************************************
解析can1队列中的数据，如果正确解析完整的一帧，返回CAN_EVENT
并且cmd中保存了一帧的数据， cmd_len保存这帧数据的长度
****************************************************************/
int parse_can1_fifo(void *fifo, char cmd[], int *cmd_len/*, int fifoClean*/)
{
	static char data_len;
	static int state = HEAD1, i=0;	
	
	char data, mData[30];
	unsigned short checkValue;
	int ret; 
	struct kfifo *mfifo = (struct kfifo *)fifo;
	
//	if (fifoClean) state = HEAD1;

LOOP:	
	switch(state) {
		
		case HEAD1:
			i = 0;
			data_len = 0;
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xaa) {
				*cmd = data;			
				state++;
			} else if(ret == 1) {
				printf("%s->error1!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			} else if(ret == 0) {
				break;
			} 
			
		case HEAD2:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xbb){
				*(cmd+state) = data;
				state++;
			} else if (ret == 0) {
				break;
			} else if (ret == 1) {
				printf("%s->error2!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			}
			
		case LEN:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data != 0) {
				data_len = data;
				*(cmd+state) = data;
				//state++;	
			} else if (ret == 0) {
				printf("%s->error3-1!\r\n", __func__);
				break;
			} else {
				printf("%s->error3-2!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			}
			
		  if(data_len > sizeof(mData)) {	
				printf("data_len > %d error!\r\n", sizeof(mData));
			}
			
		  ret = kfifo_get(mfifo, &mData, data_len);
			if(ret != data_len) {
				printf("%s-> read error 4! ret=%d, data_len=%d\r\n", 
				__func__, ret, data_len);
						
				state = HEAD1;
				goto LOOP;
			}
		
			for(i=0; i< data_len; i++) {
				*(cmd+LEN+1+i) = mData[i];
			}
//0xaa 0xbb len mCmd d0~d3 ide rtr data0... check01 check02			
			checkValue = calculate_crc((uint8*)cmd+LEN, data_len+1);
			*(cmd+LEN+data_len+1) = (unsigned char)(checkValue & 0xff);
			*(cmd+LEN+data_len+2) = (unsigned char)((checkValue & 0xff00) >> 8);
			mfifo->acceptCmds++;
			*cmd_len = 5+data_len;
			state = HEAD1;
			return CAN_EVENT;
			
		default:
			state = HEAD1;			
			goto LOOP;
		}

		return CAN_NULL;
}

/***************************************************************
解析can2队列中的数据，如果正确解析完整的一帧，返回CAN_EVENT
并且cmd中保存了一帧的数据， cmd_len保存这帧数据的长度
****************************************************************/
int parse_can2_fifo(void *fifo, char cmd[], int *cmd_len/*, int fifoClean*/)
{
	static char data_len;
	static int state = HEAD1, i=0;	
	
	char data, mData[30];
	unsigned short checkValue;
	int ret; 
	struct kfifo *mfifo = (struct kfifo *)fifo;
	
//	if (fifoClean) state = HEAD1;

LOOP:	
	switch(state) {
		
		case HEAD1:
			i = 0;
			data_len = 0;
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xaa) {
				*cmd = data;			
				state++;
			} else if(ret == 1) {
				printf("%s->error1!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			} else if(ret == 0) {
				break;
			} 
			
		case HEAD2:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data == 0xbb){
				*(cmd+state) = data;
				state++;
			} else if (ret == 0) {
				break;
			} else if (ret == 1) {
				printf("%s->error2!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			}
			
		case LEN:
			ret = kfifo_get(mfifo, &data, 1);
			if(ret == 1 && data != 0) {
				data_len = data;
				*(cmd+state) = data;
				//state++;	
			} else if (ret == 0) {
				printf("%s->error3-1!\r\n", __func__);
				break;
			} else {
				printf("%s->error3-2!\r\n", __func__);
				state = HEAD1;
				goto LOOP;
			}
			
		  if(data_len > sizeof(mData)) {	
				printf("data_len > %d error!\r\n", sizeof(mData));
			}
			
		  ret = kfifo_get(mfifo, &mData, data_len);
			if(ret != data_len) {
				printf("%s-> read error 4! ret=%d, data_len=%d\r\n", 
				__func__, ret, data_len);
						
				state = HEAD1;
				goto LOOP;
			}
		
			for(i=0; i< data_len; i++) {
				*(cmd+LEN+1+i) = mData[i];
			}
//0xaa 0xbb len mCmd d0~d3 ide rtr data0... check01 check02			
			checkValue = calculate_crc((uint8*)cmd+LEN, data_len+1);
			*(cmd+LEN+data_len+1) = (unsigned char)(checkValue & 0xff);
			*(cmd+LEN+data_len+2) = (unsigned char)((checkValue & 0xff00) >> 8);
			mfifo->acceptCmds++;
			*cmd_len = 5+data_len;
			state = HEAD1;
			return CAN_EVENT;
			
		default:
			state = HEAD1;			
			goto LOOP;
		}

		return CAN_NULL;
}

/***************************************************************
根据一帧的数据，生成struct car_event数据，保存到链表中，此链表由
链表头struct list_head event_head 标定。
****************************************************************/
void make_event_to_list(const char *cmd, int cmd_len, int result, char hasId)
{
	struct car_event* event = (struct car_event*)mymalloc(0, sizeof(struct car_event));
	if(event == NULL) {
		//if(kfifo_len(debug_fifo) < DEBUG_KFIFO_LEN/2)
		printf("%s: mymalloc car_event fail!\r\n", __func__);
		return;
	}
	//event->id = (unsigned long)event;
	event->data = (char *)mymalloc(0, cmd_len);
	if(event->data == NULL) {
		//if(kfifo_len(debug_fifo) < DEBUG_KFIFO_LEN/2)
		printf("%s: CAN1 mymalloc event->data fail!\r\n", __func__);
		myfree(0, event);
		return;
	}				
	memcpy(event->data, cmd, cmd_len);				
	event->data_len = cmd_len;
	event->tim_count = 1;
	event->state = result;
	
	if(hasId) {
		event->ID = 0;
		event->ID |= ((*(cmd+CAN_ID_0)) << 24);  
		//必须根据协议修改
		event->ID |= ((*(cmd+CAN_ID_1)) << 16);
		event->ID |= ((*(cmd+CAN_ID_2)) << 8);
		event->ID |=  (*(cmd+CAN_ID_3));	
	} else {
		event->ID = 0xff00 + *(cmd+CAN_CMD);
	}
	add_event_to_list(event);	
}

/*此函数只给handle_debug_msg_work调用，请勿随便调用！*/
int make_event_to_list0(const char *cmd, int cmd_len, int result, char hasId)
{
	struct car_event* event = (struct car_event*)mymalloc(0, sizeof(struct car_event));
	if(event == NULL) {
		return -1;
	}
	//event->id = (unsigned long)event;
	event->data = (char *)mymalloc(0, cmd_len);
	if(event->data == NULL) {
		myfree(0, event);
		return -1;
	}				
	memcpy(event->data, cmd, cmd_len);				
	event->data_len = cmd_len;
	event->tim_count = 1;
	event->state = result;
	
	if(hasId) {
		event->ID = 0;
		event->ID |= ((*(cmd+CAN_ID_0)) << 24);  
		//必须根据协议修改
		event->ID |= ((*(cmd+CAN_ID_1)) << 16);
		event->ID |= ((*(cmd+CAN_ID_2)) << 8);
		event->ID |=  (*(cmd+CAN_ID_3));	
	} else {
		event->ID = 0xff00 + *(cmd+CAN_CMD);
	}
	add_event_to_list(event);	
	return 0;
}

/***************************************************************
								往android系统上报一帧的数据                     
****************************************************************/
void report_car_event(int result, const char* cmd, int cmd_len)
{
	u8 t;
//	u8 canbuf[8];
	
	switch(result)
	{
		case CAN_NULL:
			//printf("%s: case 0 error!\r\n", __func__);
			break;
		
		case CAN_EVENT:
			
			//上报can上的信息到大屏上
			for(t=0; t<cmd_len; t++)
			{
				//printf("cmd[%d]=%x ", t, cmd[t]);
				USART_ClearFlag(USART6,USART_FLAG_TC); 
				//fix bug lost first byte.
				//USART_GetFlagStatus(USART6, USART_FLAG_TC);
				USART_SendData(USART6, cmd[t]);         //向串口6发送数据
				while(USART_GetFlagStatus(USART6,USART_FLAG_TC)!=SET);//等待发送结束
			}		
			
			numMcuSendedCmdToAndroid++;
			break;
		default:
			break;
	}	
}

void decode_can_fifo(struct kfifo *mfifo, char *cmd)
{
	int result = 0, len = 0;
	
	if(kfifo_len(mfifo) > 0)
	{	
		if(mfifo == can1_fifo) {
			result = parse_can1_fifo(mfifo, cmd, &len);
		} else if(mfifo == can2_fifo) {
			result = parse_can2_fifo(mfifo, cmd, &len);
		}
		//处理can总线上获得的数据，解析成功后加入链表头
		if(result == CAN_EVENT) {
			make_event_to_list(cmd, len, result, 1);
		} else {
			printf("%s->r\r\n", __func__);
		}
	}		
}

long mMcuReportRepeatNum=0;

void list_event_to_android(void)
{
	if(event_sending == NULL) 
	{
		event_sending = get_event_from_list();	
		if(event_sending != NULL && mAndroidRunning == 1) { //只有android在运行的时候才往uart上报数据。
			report_car_event(event_sending->state, event_sending->data, event_sending->data_len);		
		}
	} 
	else 
	{
		if(event_sending->tim_count%(6) == 0 && mAndroidRunning == 1) {
			mMcuReportRepeatNum++;
			event_sending->tim_count++;
			report_car_event(event_sending->state, event_sending->data, event_sending->data_len);	
			printf("%s: id=%04x, tim_count=%d\r\n", __func__, event_sending->ID, event_sending->tim_count);	
		}			
	}		
}

void handle_upstream_work(char *cmd1, char *cmd2)
{
	decode_can_fifo(can1_fifo, cmd1);
	decode_can_fifo(can2_fifo, cmd2);
	list_event_to_android();
}


