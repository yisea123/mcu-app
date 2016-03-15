#ifndef __LONGSUNG_H
#define __LONGSUNG_H	 

#include "sys.h"
#include "malloc.h"
#include "usart.h"
#include "kfifo.h"
#include "iwdg.h"
#include "ioctr.h"
#include "delay.h"
#include "list.h"
#include "cjson.h"
#include <string.h>

#define ONE_SECOND		110000
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
#define ATCSQ						1
#define ATSIMTEST				2
#define ATCPMS					3
#define ATCMGD_					4
#define ATMIPCALL_			5
#define ATMIPCALL0			6
#define ATMIPPROFILE		7
#define ATMIPCALL1			8
#define ATMIPOPEN				9
#define ATMIPSEND				10
#define ATMIPPUSH				11
#define ATCMGF					12
#define ATMIPCLOSE			13
#define ATCMGR					14
#define ATCMGD					15
#define ATMIPHEX				16
#define ATRESET         17
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
	char index;
	int para;
	long long interval;
	
	char atack;
	
	int mqtype;
	uint16_t msgid;
	char mqttack;
	unsigned char *mqttdata;
	int mqttdatalen;
	char try;
	
	char clean;
	struct list_head list;	
}AtCommand;

typedef struct __attribute((__packed__)) {
	uint8_t ver;
	uint8_t compress_encrypt;
	uint8_t frametype;
	uint8_t servicetype;
	uint8_t frameinfo;
	uint8_t sessionid;
	uint8_t frameid;
	uint8_t tmp;
	uint32_t datasize;
	uint8_t revert[4];
}FrameHead;

typedef struct __attribute((__packed__)) {
  uint8_t rpc;
	uint8_t rpcmethrod[3];
	uint32_t rpcid;
	uint32_t jsonlen;
}DataHead;

typedef struct {
	char is_inited;
	int simcard_type;
	
	char reset_request;
	char boot_status;
	
	int singal[2];
	
	char tcp_connect;
	char ppp_status;
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
	
  char scsq;
	char rcsq;
	
	char heartbeat_tick;
	char period_tick;
	
	int at_count;
	char at_sending[64];
	FrameHead fh;
	DataHead dh;
	AtCommand* atcmd;
	struct list_head at_head;
	struct list_head mqtt_head;
	struct list_head atcmd_head;
	long long close_tcp_interval;
	char mqtt_alive;
	long long clean_interval;
	uint32_t malloc_count;
	uint32_t free_count;
	long long sys_time;
}DevStatus;

extern void longsung_init(void);
extern void handle_longsung_uart_msg(void);
extern int str2int(const char* p, const char* end);
extern void notify_longsung_period(void);
extern void handle_longsung_setting(void);
extern void notify_longsung_second(void);
extern DevStatus dev[1];
#endif



