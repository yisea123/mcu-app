#ifndef __MCU_EVENT_H
#define __MCU_EVENT_H	 

#include "list.h"
#include "malloc.h"
#include "usart.h"
#include "kfifo.h"

#define CAN_NULL   0
#define CAN_EVENT  1

struct car_event
{
	char *data;
	char data_len;
	char tim_count;
	uint32_t ID;
	char state;
	struct list_head list;
};

extern struct list_head event_head;
extern void list_event_to_android(void);
extern void add_event_to_list(struct car_event *event);
extern void del_event_from_list(struct car_event *event);
//extern struct car_event* check_event(struct car_event *event);
extern struct car_event* get_event_from_list(void);
extern int list_event(void);
//extern int parse_can1_fifo(void *fifo, char cmd[], int *cmd_len);
//extern int parse_can2_fifo(void *fifo, char cmd[], int *cmd_len);
extern void make_event_to_list(const char *cmd, int cmd_len, int result, char hasId);
extern void report_car_event(int result, const char* cmd, int cmd_len);
extern void handle_upstream_work(char *cmd1, char *cmd2);
extern struct car_event *event_sending;
extern long cmd_sended, mMcuReportRepeatNum, numMcuSendedCmdToAndroid, numMcuReportToAndroid;
#endif
