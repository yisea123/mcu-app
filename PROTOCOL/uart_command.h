#ifndef __UART_COMMAND_H
#define __UART_COMMAND_H	 

#define CMD_ADD_END 0 // 1

#define CHAR_END 0xFF //can't equal 0x00

#define CMD_UART_LEN 138 

#define DATA_MAX_LEN CMD_UART_LEN

#define ACK2 1

#define PACKET_ITEM 3

/**********************************/
//event type
//0xaa 0xbb len mCmd d0~d3(id) ide rtr data0 data1 ... check01 check02
#define CAN_HEAD_0 0
#define CAN_HEAD_1 1

#define CAN_DATA_LEN 2
#define CAN_CMD 3

#define CAN_ID_0 4
#define CAN_ID_1 5
#define CAN_ID_2 6
#define CAN_ID_3 7

#define CAN_IDE 8
#define CAN_RTR 9

#define CAN_D0 10
#define CAN_D1 11
#define CAN_D2 12
#define CAN_D3 13
#define CAN_D4 14
#define CAN_D5 15
#define CAN_D6 16
#define CAN_D7 17

#define CAN_CHECK_0 18
#define CAN_CHECK_1 19
#define CAN_MSG_LEN 20
/**********************************/

/**********************************/
//periodic type
//0xaa 0xbb len mCmd periodic d0~d3(id) ide rtr data0 data1 .... check01 check02 
#define CAN_HEAD_0_P 0
#define CAN_HEAD_1_P  1

#define CAN_DATA_LEN_P  2
#define CAN_CMD_P  3

#define CAN_PERIODIC_P  4

#define CAN_ID_0_P  5
#define CAN_ID_1_P  6
#define CAN_ID_2_P  7
#define CAN_ID_3_P  8

#define CAN_IDE_P  9
#define CAN_RTR_P  10

#define CAN_D0_P  11
#define CAN_D1_P  12
#define CAN_D2_P  13
#define CAN_D3_P  14
#define CAN_D4_P  15
#define CAN_D5_P  16
#define CAN_D6_P  17
#define CAN_D7_P  18

#define CAN_CHECK_0_P  19
#define CAN_CHECK_1_P  20
#define CAN_MSG_LEN_P  21
/**********************************/

/**********************************/
//ack type
//0xaa 0xbb 0x03 0x80 c1 c2 check1 check2 -> ack message
#define ACK_HEAD1 0
#define ACK_HEAD2 1
#define ACK_DATA_LEN 2
#define ACK_CMD 3
#define ACK_D0 4
#define ACK_D1 5
#define ACK_CHECK_0 6
#define ACK_CHECK_1 7
#define ACK_LEN 8
/**********************************/

/***********************************/
#define UART_NULL		  0
#define UART_ACK			1
#define UART_CMD			2

#define CMD_CAN_EVENT  				(0x13)
#define CMD_CAN_PERIODIC  		(0x24)
#define CMD_RTC  							(0x25)
#define CMD_ANDROID_STATE   	(0x26)
#define CMD_UPDATE_BIN      	(0x27)
#define CMD_DEBUG     				(0x28)
#define CMD_OTHER							(0x2f)
/***********************************/

#include "kfifo.h"
#include "usart.h"
#include "kfifo.h"
#include "can.h"
#include "car_event.h"
#include "rtc.h"
#include "list.h"

//uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len
struct msg_periodic {
	uint32_t id;
	uint8_t ide;
	uint8_t rtr;
	u8 *msg;
	u8 len;
	u8 canx;
	
	uint32_t update;
	uint32_t *periodic; // ָ��һ��ȫ�ֵı�������ʱ����ʱ ��ÿ100ms���޸Ĵ˱�����ֵ��
	u8 n;  //n*100ms send msg to can. 
	struct list_head list;
};


extern uint32_t periodicNum;
extern struct list_head periodic_head;
extern int parse_uart6_fifo(void *fifo, char cmd[], int *cmd_len, char ack[], int *ack_len);
extern int do_uart_cmd(int result, const char* cmd, int cmd_len);
extern void send_cmd_ack(const char* ack, int ack_len);
extern void parse_cmd_ack(const char* ack, int ack_len);
extern unsigned short calculate_crc(unsigned char *p, unsigned short n);
extern void Timer2_Init(u16 arr,u16 psc);
extern void list_periodic_msg(void);
extern void handle_downstream_work(char* cmd, char *ack);
extern int make_event_to_list0(const char *cmd, int cmd_len, int result, char hasId);
#endif
