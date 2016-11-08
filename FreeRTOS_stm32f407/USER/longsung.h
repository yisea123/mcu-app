#ifndef __LONGSUNG_H
#define __LONGSUNG_H	 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"	
#include "aes.h"
#include "md5.h"
#include "sys.h"
#include "xmalloc.h"
#include "usart.h"
#include "rfifo.h"
#include "xlist.h"
#include "cjson.h"
#include "mqtt_msg.h"
#include <string.h>
#include <stdio.h>
#include "mqtt_msg.h"

//#define ONE_SECOND		300300
#define ONE_SECOND		( 1000/portTICK_RATE_MS )
#define MAX_TOKENS		20

/*可使用的JSON长度小于二分之一MAX_LINE_LEN, 
由于4G模块每包最多1500， hex就750个！*/
#define MAX_LINE_LEN		1555//1024 1500

/******************************/
#define PPP_DISCONNECT 		0
#define PPP_CONNECTING 		1
#define PPP_CONNECTED  		2
/******************************/

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

typedef struct {
    const char*  p;
    const char*  end;
} Token;

typedef struct {
	int count;
	Token tokens[MAX_TOKENS];
}RemoteTokenizer;

typedef void (* tcp_data_callback)(RemoteTokenizer *tzer, Token* tok);

typedef void (* connect_err_callback)(RemoteTokenizer *tzer);
typedef void (* connect_success_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* disconnect_callback)(RemoteTokenizer *tzer, Token* tok);

typedef void (* signal_strength_callback)(RemoteTokenizer *tzer, Token* tok);

typedef void (* get_ip_success_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* get_ip_fail_callback)(RemoteTokenizer *tzer);

typedef void (* at_command_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* at_command_success_callback)(RemoteTokenizer *tzer);
typedef void (* at_command_fail_callback)(RemoteTokenizer *tzer);

typedef void (* check_simcard_type_callback)(RemoteTokenizer *tzer, Token* tok);

typedef void (* check_sm_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* read_sm_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* sm_data_callback)(RemoteTokenizer *tzer, Token* tok, int index);
typedef void (* sm_notify_callback)(RemoteTokenizer *tzer, Token* tok);
typedef void (* sm_read_err_callback)(RemoteTokenizer *tzer, Token* tok);

typedef struct {
	char inited;
	int pos;
	int overflow;
	
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
	uint16_t msgid;							/*message id for mqtt message*/
	char mqttack;							/*for check mqtt ack*/
	unsigned char *mqttdata;				/*for pointer malloc mqtt data*/
	int mqttdata_len;						/*for store mqtt data length*/
	char mqtt_try;							/*for store mqtt message send times*/		
	char mqtt_clean;							/*clean for mqtt command*/
	struct list_head list;					/*node for list*/
}AtCommand;

struct device_operations {
	void (* init_module )( void *dev );	
	void (* poll_module_signal )( void *dev );
	void (* check_module_sm )( void *dev ) ;
	void (* check_module_ip )( void *dev ) ;
	void (* module_request_ip )( void *dev ) ;	
	void (* close_module_socket )( void *dev ) ;
	void (* tcp_connect_server )( void *dev ) ;
	void (* push_socket_data )( void *dev ) ;
	void (* delete_module_sm )( void *dev ) ;
	void (* read_module_sm )( void *dev ) ;
};

struct module_callback_operations {
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

typedef struct {
	char name[20];
	struct device_operations *d_ops;	
	struct module_callback_operations* c_ops;
	void (* module_reader_parse )( UartReader *reader );	
	void *dev;
}ComModule;

typedef struct {
	char is_inited;
	int simcard_type;						/*indicate sim care type,  0 is no card*/
	
	char reset_request;
	char boot_status;						/*indicate if cmodule boot finish*/
	
	int singal[2];
	
	char tcp_connect;
	char ip[30];							/*store ip from china mobile*/
	char ppp_status;						/*indicate the net status*/
	char socket_close_flag;
	int socket_open[4];
	char socket_num;
	
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
	char at_sending[64];
	AtCommand* atcmd;						/*AtCommand that be sending*/
	struct list_head at_head;				/*all AtCommand sending list*/
	struct list_head mqtt_head;				/*mqtt type AtCommand wait ack list, add to list if need wait ack*/	
	struct list_head atcmd_head;			/*nomal AT type AtCommand wait ack list, add to list if need wait ack*/
	unsigned int close_tcp_interval;		/*for waitting someting times then close tcp*/
	unsigned int clean_interval;			/*for clean waitting someting times*/
	unsigned int tick_sum;					/*relate to close_tcp_interval and clean_interval*/
	unsigned int tick_tag;					/*relate to close_tcp_interval and clean_interval*/
	uint32_t malloc_count;
	uint32_t free_count;
	long long sys_time;
	mqtt_dev_status *mqtt_dev;				/*pointer to mqtt_dev_status*/
	ComModule *module;
}DevStatus;

extern DevStatus dev[1];
extern void HandleLongSungTask( void * pvParameters );
#endif



