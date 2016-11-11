#ifndef __LONGSUNG_H
#define __LONGSUNG_H
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"	
#include "sys.h"
#include "usart.h"
#include "rfifo.h"
#include "xlist.h"
#include "mqtt_msg.h"

#define NOW_TICK		xTaskGetTickCount()
#define SIZE_ARRAY(a) 	(sizeof(a) / sizeof((a)[0]))

#define ONE_SECOND		( 1000/portTICK_RATE_MS )
#define MAX_TOKENS		20

/*可使用的JSON长度小于二分之一MAX_LINE_LEN, 
由于4G模块每包最多1500， hex就750个！*/
#define MAX_LINE_LEN		1555//1024 1500

/******************************/
#define ATCSQ					1
#define ATSIMTEST				2
#define ATCPMS					3
#define ATCMGD_					4
#define ATMIPCALL_				5
#define ATMIPCALL0				6
#define ATMIPPROFILE			7
#define ATMIPCALL1				8
#define ATMIPOPEN				9
#define ATMIPSEND				10
#define ATMIPPUSH				11
#define ATCMGF					12
#define ATMIPCLOSE				13
#define ATCMGR					14
#define ATCMGD					15
#define ATMIPHEX				16
#define ATRESET         		17
#define ATMQTT					18
/******************************/

typedef enum {
	 PPP_DISCONNECT = 0,
	 PPP_CONNECTING = 1,
	 PPP_CONNECTED  = 2
}module_ppp_status;

typedef struct {
    const char*  p;
    const char*  end;
} Token;

typedef struct {
	int count;
	Token tokens[MAX_TOKENS];
}RemoteTokenizer;

typedef void (* tcp_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);

typedef void (* connect_err_callback)( void *dev, RemoteTokenizer *tzer);
typedef void (* connect_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* disconnect_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);

typedef void (* signal_strength_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);

typedef void (* get_ip_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* get_ip_fail_callback)( void *dev, RemoteTokenizer *tzer);

typedef void (* at_command_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* at_command_success_callback)( void *dev, RemoteTokenizer *tzer);
typedef void (* at_command_fail_callback)( void *dev, RemoteTokenizer *tzer);

typedef void (* check_simcard_type_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);

typedef void (* check_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* read_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* sm_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok, int index);
typedef void (* sm_notify_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);
typedef void (* sm_read_err_callback)( void *dev, RemoteTokenizer *tzer, Token* tok);

typedef struct {
	char inited;
	int pos;
	int overflow;
	void *p_dev;
	
	connect_err_callback on_connect_fail;
	connect_success_callback on_connect_success;
	disconnect_callback on_disconnect;
	tcp_data_callback on_tcp_data;
	
	signal_strength_callback on_signal_strength;
	
	get_ip_success_callback on_ip_success;
	get_ip_fail_callback on_ip_fail;
	
	at_command_callback on_at_command;
	at_command_success_callback on_at_success;
	at_command_fail_callback	on_at_fail;
	check_simcard_type_callback on_simcard_type;
	
	check_sm_callback on_sm_check;
	read_sm_callback on_sm_read;
	sm_data_callback on_sm_data;
	sm_notify_callback on_sm_notify;
	sm_read_err_callback on_sm_read_err;
	
	char in[MAX_LINE_LEN];
}UartReader;

typedef struct {
	char 	be_used;						/*indicate MqttBuffer if be used*/
	char 	*buff;							/*buffer pointer of MqttBuffer*/
	int 	capacity;						/*capacity of MqttBuffer*/
	char 	num;							/*for store total MqttBuffer count*/
}MqttBuffer;

typedef struct {
	char be_used;							/*for indicate if itself be used*/
	unsigned char num;						/*for store total AtCommand count, do not modify this value*/
	char index;								/*for store command type(AT or MQTT type)*/
	int para;								/*para for nomal AT command*/
	unsigned int interval;					/*wait for interval time to finish command*/
	unsigned int tick_sum;					/*for caculate the interval*/
	unsigned int tick_tag;					/*for caculate the interval*/
	char atack;								/*for check nomal AT command(that need to 
											check ack) ack*/
	mqtt_message_type mqtype;				/*MQTT_MSG_TYPE_CONNECT ... 
											type for mqtt message,  MQTT_MSG_TYPE_NULL
											for AT type*/
	unsigned short msgid;					/*message id for mqtt message*/
	char mqttack;							/*for check mqtt ack*/
	unsigned char *mqttdata;				/*for pointer malloc mqtt data*/
	int mqttdata_len;						/*for store mqtt data length*/
	char mqtt_try;							/*for store mqtt message send times*/		
	char mqtt_clean;						/*clean for mqtt command*/
	struct list_head list;					/*node for list*/
	//void *priv;								/*private data pointer*/
}AtCommand;

struct device_operations {
	void (* initialise_module )( void *dev );	
	void (* poll_module_signal )( void *dev );
	void (* check_module_sm )( void *dev ) ;
	void (* check_module_ip )( void *dev ) ;
	void (* module_request_ip )( void *dev ) ;	
	void (* close_module_socket )( void *dev ) ;
	void (* tcp_connect_server )( void *dev ) ;
	void (* push_socket_data )( void *dev ) ;
	void (* delete_module_sm )( void *dev ) ;
	void (* read_module_sm )( void *dev ) ;
	void (* send_command_to_module )( AtCommand* cmd );
};

struct callback_operations {
	void (* tcp_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* connect_err_callback)( void *dev, RemoteTokenizer *tzer );
	void (* connect_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* disconnect_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* signal_strength_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* get_ip_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* get_ip_fail_callback)( void *dev, RemoteTokenizer *tzer );

	void (* at_command_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* at_command_success_callback)( void *dev, RemoteTokenizer *tzer );
	void (* at_command_fail_callback)( void *dev, RemoteTokenizer *tzer );

	void (* check_simcard_type_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* check_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* read_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* sm_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok, int index );
	void (* sm_notify_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* sm_read_err_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );	
};

/*
后续会对通讯模块进行抽象并实现
在换其它的模块时，可以在不修改核心
代码的情况下，只需实现对具体模块
的操作即可，对模块的具体操作抽象
成对所有通讯模块操作的最大集合
*/
typedef struct ComModule{
	void *p_dev;	
	char name[20];
	struct ComModule * next;
	struct device_operations *d_ops;	
	struct callback_operations* c_ops;
	void (* module_reader_parse )( UartReader *reader );
}ComModule;

typedef struct {
	int simcard_type;						/*indicate sim care type,  0 is no card*/
	
	char reset_request;						/*request to power reset module*/
	char boot_status;						/*indicate if cmodule boot finish*/
	
	int singal[2];
	
	char tcp_connect_status;				/*indicate tcp status*/
	char ip[30];							/*store ip from china mobile*/
	module_ppp_status ppp_status;			/*indicate the net status*/
	char socket_close_flag;					/*close all socket flag, if it is 1, close socket*/
	int socket_open[4];						/*store socket number*/
	char socket_num;						/*socket number open in module*/
	
	int sm_index[20];
	char sm_num;
	int sm_index_read;
	int sm_index_delete;
	char sm_data_flag;
	char sm_read_flag;
	char sm_delete_flag;
	
  	char scsq;								/*signal check AT command send count*/
	char rcsq;								/*receive signal check AT command ack count*/
	
	char heartbeat_tick;					/*mqtt tick times (heartbeat_tick*peroid) */
	char period_tick;						/*peroid flag*/
	
	int at_count;
	char at_sending[64];					/*store sending at command*/
	unsigned int close_tcp_interval;		/*for waitting someting times then close tcp*/
	unsigned int tick_sum;					/*relate to close_tcp_interval and clean_interval*/
	unsigned int tick_tag;					/*relate to close_tcp_interval and clean_interval*/
	unsigned int malloc_count;
	unsigned int free_count;
	char android_power_status;
	
	struct list_head at_head;				/*all AtCommand sending list*/
	struct list_head mqtt_head;				/*mqtt type AtCommand wait ack list, add to list if need wait ack*/	
	struct list_head atcmd_head;			/*nomal AT type AtCommand wait ack list, add to list if need wait ack*/	

	xSemaphoreHandle os_mutex;				/*for protect list and all thing*/
	unsigned int timer_tick;				/*wake_timer's peroid tick*/
	TimerHandle_t wake_timer;				/*timer for wake up pxModuleTask*/	
	TimerHandle_t os_timer; 				/*freeRTOS timer*/
	TaskHandle_t pxModuleTask;				/*pointer module TCB*/
	Ringfifo* uart_fifo;					/*pointer to module uart data fifo*/

	AtCommand* atcmd;						/*AtCommand that be sending*/
	AtCommand *p_atcommand;					/*buffer manager for at command*/
	MqttBuffer *p_mqttbuff[3];				/*buffer manager for mqtt buffer*/		
	UartReader *reader;						/*UartReader instance*/
	mqtt_dev_status *mqtt_dev;				/*mqtt_dev_status instance*/
	ComModule *module;	
}DevStatus;

extern void HandleModuleTask( void * pvParameters );
extern void test_mqtt_publish( void *data );
#endif



