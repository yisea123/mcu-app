#ifndef __UART_COMMAND_H
#define __UART_COMMAND_H	 

#define EVENT_MSG_TRYS		4

#define CMD_ADD_END 0 // 1

#define CHAR_END 0xFF //can't equal 0x00

#define CMD_UART_LEN 138 

#define DATA_MAX_LEN CMD_UART_LEN

#define ACK2 1

#define PACKET_ITEM 3

#define CAN_DATA_VALUE_LEN		8
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

#define CMD_CAN_EVENT  				(0x01)
#define CMD_CAN_PERIODIC  		(0x02)
#define CMD_RTC  							(0x03)
#define CMD_ANDROID_STATE   	(0x04)
#define CMD_UPDATE_BIN      	(0x05)
#define CMD_DEBUG     				(0x06)
#define CMD_OTHER							(0x0f)
/***********************************/

#include "kfifo.h"
#include "usart.h"
#include "kfifo.h"
#include "can.h"
#include "car_event.h"
#include "rtc.h"
#include "list.h"
#include "pending.h"

//uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len
typedef struct msg_periodic {
	uint32_t id;
	uint8_t ide;
	uint8_t rtr;
	u8 *msg;
	u8 len;
	u8 canx;
	
	uint32_t update;
	uint32_t *periodic; // 指向一个全局的变量，定时器定时 （每100ms）修改此变量的值。
	u8 n;  //n*100ms send msg to can.
	u8 is_idle;
	u8 send_status;
	int try_count;
	struct list_head list;
} MSGPERIODIC;

#define	STEP0			1
#define	STEP1			2

typedef struct msg_event {
	u8				step;
	uint32_t 	id;
	uint8_t 	ide;
	uint8_t 	rtr;	
	u8 				msg[8];
	u8				len;
	u8				canx;
	
	uint32_t 	update;
	uint32_t	*pEventCount;
	u8				n;
	
	int			trys;
	struct list_head list;
} MSGEVENT;

//return 0: do nothing

//return 1: 接收到上报的命令所返回的确认信号,把发送成功的命令包从链表中去除
//其中ack就是所上报发送的命令，ack_len就是命令的长度。

//return 2:cmd接收到大屏发送过来的命令,cmd_len命令长度
//ack为组织好的确认包，ack_len为确认包长度
//发确认包ACK给大屏，并处理CMD

extern long numRecvAndroidCanCmd;
extern char mReportMcuStatusPending;
//extern uint32_t periodicNum;
extern struct list_head periodic_head;
extern struct list_head periodic_wait_head;
extern int parse_uart6_fifo(void *fifo, char cmd[], int *cmd_len, char ack[], int *ack_len);
extern int do_uart_cmd(int result, const char* cmd, int cmd_len);
extern void send_cmd_ack(const char* ack, int ack_len);
extern void parse_cmd_ack(const char* ack, int ack_len);
extern unsigned short calculate_crc(unsigned char *p, unsigned short n);
//extern void list_periodic_msg(void);
extern void handle_downstream_work(char* cmd, char *ack);
extern int make_event_to_list0(const char *cmd, int cmd_len, int result, char hasId);
extern void uart_command_init(void);
#endif
