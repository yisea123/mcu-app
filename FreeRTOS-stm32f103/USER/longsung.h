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
#include "rfifo.h"
#include "xlist.h"
#include "mqtt_msg.h"

#define NOW_TICK			xTaskGetTickCount()
#define SIZE_ARRAY(a) 		( sizeof( a ) / sizeof( ( a )[0]) )

#define ONE_SECOND			( 1000/portTICK_RATE_MS )
#define MAX_TOKENS			20

/*可使用的JSON长度小于二分之一MAX_LINE_LEN, 
由于4G模块每包最多1500， hex就750个！*/
#define MAX_LINE_LEN		1555//1024 1500

/******************************/
#define ATMQTT					1

/******************************/

#define WAIT					1
#define WAIT_NOT				0

typedef enum  {
	 CHECK_EXIST = 1,
	 CHECK_EXIST_NOT = 0,
	 CHECK_ERROR = -1,
}CheckResult;


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

typedef struct {
	char inited;
	unsigned int pos;
	unsigned char overflow;
	void *p_dev;
	char in[ MAX_LINE_LEN ];
}UartReader;

typedef struct {
	char 	be_used;						/*indicate MqttBuffer if be used*/
	char 	*buff;							/*buffer pointer of MqttBuffer*/
	int 	capacity;						/*capacity of MqttBuffer*/
	char 	num;							/*for store total MqttBuffer count*/
}MqttBuffer;

typedef struct {
	char be_used;							/*for indicate if itself be used, app do not 
												memset AtCommand, or change be_used*/
	unsigned char num;						/*for store total AtCommand count, 
												do not modify this value in app*/											
	char index;								/*for store command type(AT or MQTT type)*/
	int para;								/*para for nomal AT command*/
	unsigned int interval;					/*wait for interval time to finish command*/
	unsigned int interval_a;
	unsigned int interval_b;
	char wait_flag;
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
}AtCommand;

struct device_operations {
	void (* initialise_module )( void *instance );	
	void (* hardware_reset_callback )( void *instance );	
	
	void (* poll_module_peroid )( void *instance );
	void (* check_module_sm )( void *instance ) ;
	void (* check_module_ip )( void *instance ) ;
	void (* module_request_ip )( void *instance ) ;	
	void (* close_module_socket )( void *instance ) ;
	void (* tcp_connect_server )( void *instance ) ;
	void (* push_socket_data )( void *instance, unsigned int tick ) ;
	void (* delete_module_sm )( void *instance, int index ) ;
	void (* read_module_sm )( void *instance, int index ) ;
	void (* send_command_to_module )( void *instance, AtCommand* cmd );
	char* (* at_get_name )( void *instance, int index );
	char* (* make_tcp_packet )( char* buff, unsigned char* data, int len );
	void (* send_tcp_packet )( char* buff, unsigned char* data, int len );
	
	void (* send_push_data_directly )( void *instance );
	void (* close_module_socket_directly )( void *instance, int index ) ;
	int (* is_tcp_connect_server )( void *instance, int index );
};

struct callback_operations {
	void (* tcp_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* connect_err_callback)( void *dev, RemoteTokenizer *tzer );
	void (* connect_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* disconnect_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* signal_strength_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* request_ip_success_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* request_ip_fail_callback)( void *dev, RemoteTokenizer *tzer );

	void (* at_command_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* at_command_success_callback)( void *dev, RemoteTokenizer *tzer );
	void (* at_command_fail_callback)( void *dev, RemoteTokenizer *tzer );

	void (* check_simcard_type_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );

	void (* check_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* read_sm_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* sm_data_callback)( void *dev, RemoteTokenizer *tzer, Token* tok, int index );
	void (* sm_notify_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );
	void (* sm_read_err_callback)( void *dev, RemoteTokenizer *tzer, Token* tok );	
	void (* mqtt_lack_data_callback)( void *mqtt_dev );
};

/*
	对通讯模块进行抽象并实现
	在换其它的模块时，可以在不修改核心
	代码的情况下，只需实现对具体模块
	的操作即可，对模块的具体操作抽象
	成对所有通讯模块操作的最大集合
	目前已实现longsung模块的ComModule的功能集合
*/
typedef struct ComModule {
	void *p_dev;
	void *priv;
	char name[20];
	char is_initialise;
	struct ComModule * next;
	struct device_operations *d_ops;	
	struct callback_operations* c_ops;
	void (* module_reader_parse )( struct ComModule* instance, UartReader *reader );
	void (* module_power_reset )( void *argc );
}ComModule;

struct core_operations {
	CheckResult (* check_at_command_exist )( void  *argc, int index );	
	void (* make_at_command )( void *argc, char index, char wait_falg, unsigned 
				int interval_a, unsigned int interval_b, int para );
	void (* make_mqtt_command )( void *argc, char index, unsigned int interval, int para );
 	void (* atcmd_set_ack )( void *dev, int index );
	void (* set_mqtt_cmd_clean )( void *dev );
	void (* mqtt_set_mesg_ack ) ( void *mqtt_dev, int type, uint16_t msg_id );
	
	void (* system_lock )( void *dev, unsigned int tick);
	void (* system_unlock )( void *dev );	
	void (* system_sleep )( void *dev, unsigned int tick );	
	void (* wake_up_system )( void *dev );	
};

typedef struct {
	int simcard_type;						/*indicate sim care type,  0 is no card*/
	
	char reset_request;						/*request to power reset module*/
	unsigned int reset_times;
	char boot_status;						/*indicate if cmodule boot finish*/
	
	int singal[2];
	
	char tcp_connect_status;				/*indicate tcp status*/
	unsigned int tcp_connect_times;			/*tcp connect service total times*/
	char ip[30];							/*store ip from china mobile*/
	module_ppp_status ppp_status;			/*indicate the net status */

	char socket_close_flag;					/*close all socket flag, if it is 1, close socket*/
	int socket_open[4];						/*store socket number*/
	char socket_num;						/*socket number open in module*/
	
	int sm_index[20];
	char sm_num;
	int sm_index_read;
	int sm_index_delete;
	char sm_read_flag;
	char sm_delete_flag;
	
  	char scsq;								/*signal check AT command send count*/
	char rcsq;								/*receive signal check AT command ack count*/
	
	char mqtt_heartbeat;					/*mqtt heartbeat times ( heartbeat_tick *  hb_timer's peroid ) */
	char period_flag;						/*peroid flag*/
	
	int at_count;							/* AT total count have send*/
	char at_sending[64];					/*store sending at command*/
	
	unsigned int close_tcp_interval;		/*for waitting someting times then close tcp
												1. set by status machine
												2. clean by module instane
											*/										
	unsigned int tick_sum;					/*relate to close_tcp_interval and clean_interval*/
	unsigned int tick_tag;					/*relate to close_tcp_interval and clean_interval*/
	unsigned int malloc_count;
	unsigned int free_count;
	char android_power_status;
	
	struct list_head at_head;				/*all AtCommand sending list*/
	struct list_head mqtt_head;				/*mqtt type AtCommand wait ack list, add to list if need wait ack*/	
	struct list_head at_wait_head;			/*nomal AT type AtCommand wait ack list, add to list if need wait ack*/	

	xSemaphoreHandle system_mutex;				/*for protect list and all thing*/
	unsigned int wu_tick;					/*wake up timer peroid tick*/
	TimerHandle_t wu_timer;					/*timer for wake up pxModuleTask*/	
	TimerHandle_t hb_timer; 				/*heartbeat freeRTOS timer*/
	TaskHandle_t pxModuleTask;				/*pointer module TCB*/

	Ringfifo* uart_fifo;					/*pointer to module uart data fifo*/
	AtCommand* atcmd;						/*AtCommand that be sending*/
	
	AtCommand *p_atcommand;					/*buffer manager for at command*/
	MqttBuffer *p_mqttbuff[3];				/*buffer manager for mqtt buffer*/	
	
	UartReader *reader;						/*UartReader instance*/
	mqtt_dev_status *mqtt_dev;				/*mqtt_dev_status instance*/
	struct core_operations *ops;			/*core operations*/
	ComModule *module;						/*pointer to the instance of ComModule*/
	
	ComModule *module_list;
	xSemaphoreHandle list_mutex;			/*for protect module_list */	

	char debug;								/*for debug, delete when releas codes */
}DevStatus;

extern void HandleModuleTask( void * pvParameters );
extern void test_mqtt_publish( char *data );
extern int register_communication_module( TaskHandle_t task, ComModule * instance );
#endif



