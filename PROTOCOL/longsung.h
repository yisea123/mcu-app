#ifndef __LONGSUNG_H
#define __LONGSUNG_H	 

#include "sys.h"
#include "malloc.h"
#include "usart.h"
#include "kfifo.h"
#include "iwdg.h"
#include "ioctr.h"
#include "delay.h"
#include <string.h>

#define MAX_TOKENS		20
#define MAX_LINE_LEN		512

#define PPP_DISCONNECT 		0
#define PPP_CONNECTING 		1
#define PPP_CONNECTED  		2
typedef struct{
	char action;
	
	char *data;
	int len;
}RemoteCommand;

typedef struct {
    const char*  p;
    const char*  end;
} Token;

typedef struct {
	int count;
	Token tokens[MAX_TOKENS];
}RemoteTokenizer;

typedef void (* command_callback)(RemoteTokenizer *tzer, Token* tok);

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

typedef struct {
	char inited;
	
	int pos;
	int overflow;
	
	connect_err_callback on_connect_fail;
	connect_success_callback on_connect_success;
	disconnect_callback on_disconnect;
	command_callback on_command;
	
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
	
	char in[MAX_LINE_LEN];
}RemoteReader;

typedef struct {
	char is_inited;
	int simcard_type;
	
	char reset_request;
	char boot_status;
	
	int singal[2];
	
	char ppp_status;
	char mipopen_status;
	char mippush_status;
	
	char socket_close;
	int socket_open[4];
	char socket_num;
	
	int sm_index[20];
	char sm_num;
	int sm_index_read;
	long sm_read_count;
	int sm_index_delete;
	long sm_delete_count;
	char sm_flag;
	
  char scsq;
	char rcsq;
	
	char ppp_flag;
	char connect_flag;	
	char heartbeat_tick;
	char period_tick;
}DevStatus;

extern void remote_reader_addc( RemoteReader* r, int c);
extern void longsung_init(void);
extern void handle_4g_uart_msg(void);
extern int str2int(const char* p, const char* end);
extern void notify_4g_period(void);
extern void handle_4g_setting(void);

#endif



