#include "longsung.h"

static DevStatus dev[1];
static RemoteReader reader[1];

void print_char(USART_TypeDef* USARTx, char ch)
{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, ch);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
}

void print_line(USART_TypeDef* USARTx, const char* data, int len)
{

	u8 t;
	for(t=0;t<len;t++) 
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, data[t]);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
	}		
	
}

void print_line_1(USART_TypeDef* USARTx, const char* data, int len)
{

	u8 t;
	for(t=0;t<len;t++) 
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, data[t]);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
	}		
	
	//add \r\n for new line
	USART_ClearFlag(USARTx, USART_FLAG_TC); 
	USART_SendData(USARTx, 0x0d);	
	while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);	

	USART_ClearFlag(USARTx, USART_FLAG_TC); 
	USART_SendData(USARTx, 0x0a);	
	while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);		
	
}

/*send AT commnad to 4G*/
void send_at_command(const char* ack, int ack_len)
{
	u8 t;
	for(t=0;t<ack_len;t++) 
	{
		USART_ClearFlag(USART3, USART_FLAG_TC); 
		USART_SendData(USART3, ack[t]);
		while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET);
	}		
	
	/*add \r for AT command*/
	USART_ClearFlag(USART3, USART_FLAG_TC); 
	USART_SendData(USART3, 0x0d);	
	while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET);	
}

void at(char *at)
{
	int len;
	
	len = strlen(at);
	send_at_command(at, len);
}

void *memchr_x(const void *s, int c, int count)
{
	const unsigned char *p = s;

	while (count--)
		if ((unsigned char)c == *p++)
			return (void *)(p - 1);
	return NULL;
}

int memcmp_x(const void *cs, const void *ct, int count)
{
	const unsigned char *su1 = cs, *su2 = ct, *end = su1 + count;
	int res = 0;

	while (su1 < end) {
		res = *su1++ - *su2++;
		if (res)
			break;
	}
	return res;
}

int str2int(const char* p, const char* end)
{
    int   result = 0;
    int   len    = end - p;

    for ( ; len > 0; len--, p++ )
    {
        int  c;

        if (p >= end)
            goto Fail;

        c = *p - '0';
        if ((unsigned)c >= 10)
            goto Fail;

        result = result*10 + c;
    }
    return  result;

Fail:
    return -1;
}

int remote_tokenizer_init( RemoteTokenizer* t, const char* p, const char* end )
{
    int count = 0;
		
    // remove trailing newline
    if (end > p && end[-1] == '\n') {
        end -= 1;
        if (end > p && end[-1] == '\r')
            end -= 1;
    }	
		
    while (p < end) {
        const char*  q = p;

        q = memchr(p, ',', end-p);
        if (q == NULL)
            q = end;

        if (q >= p) {
            if (count < MAX_TOKENS) {
                t->tokens[count].p   = p;
                t->tokens[count].end = q;
                count += 1;
            }
        }
        if (q < end)
            q += 1;

        p = q;
    }

    t->count = count;
		
    return count;		
}

Token remote_tokenizer_get( RemoteTokenizer* t, int index )
{
    Token  tok;
    static const char*  dummy = "";

    if (index < 0 || index >= t->count) {
        tok.p = tok.end = dummy;
    } else {
        tok = t->tokens[index];
		}
		
    return tok;
}

/*******************************************
+MIPCALL:1,10.154.27.94		
********************************************/
void on_request_ip_success_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[get ip success...tzer->count=%d]], ip = \r\n", tzer->count);	
	//print_line_1(UART4, tok[1].p, tok[1].end - tok[1].p);
	dev->ppp_status = PPP_CONNECTED;
}

void on_request_ip_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[get ip fail...tzer->count=%d]]\r\n", tzer->count);
	dev->ppp_status = PPP_DISCONNECT;
	/*获取IP失败，重新获取*/		
}

void service_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	int len;
	
	//printf("%s!\r\n", __func__);
	len = str2int(tok[1].p, tok[1].end);
	printf("[tcp data coming...tzer->count=%d] [data len=%d] data=\r\n", 
			tzer->count, len);
	print_line_1(UART4, tok[2].p, tok[2].end - tok[2].p);	
}

void service_connect_callback(RemoteTokenizer *tzer, Token* tok)
{
	//+MIPOPEN=1,1
	int i, socketId = str2int(tok[0].p+9, tok[0].end);
	//printf("[%s: socketId = %d]\r\n", __func__, socketId);
	switch(socketId)
	{
		case 1:
		case 2:
		case 3:
		case 4: i = socketId - 1; break;
		default: return;
	}
	dev->socket_open[i] = socketId;
}

void service_connect_fail_callback(RemoteTokenizer *tzer)
{
	//+MIP:ERROR
	//printf("[%s, remote service error.]\r\n", __func__);
}

void service_disconnect_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//+MIPCLOSE:2
	int socketId = str2int(tok[0].p+10, tok[0].end);
	dev->socket_open[socketId-1] = -1;
	//printf("[tcp connect close...socketId=%d]\r\n", socketId);
	//printf("%d, %d, %d, %d\r\n", devStatus->socket_open[0], devStatus->socket_open[1]
	//	, devStatus->socket_open[2], devStatus->socket_open[3]);
	dev->connect_flag = 0;
}

/*******************************************
+CSQ: 7,99
********************************************/
void on_signal_strength_callback(RemoteTokenizer *tzer, Token* tok)
{
	int signal[2];
	
	signal[0] = str2int(tok[0].p+6, tok[0].end);
	signal[1] = str2int(tok[1].p, tok[1].end);

	dev->boot_status = 1;
	dev->rcsq++;
	dev->singal[0] = signal[0];
	dev->singal[1] = signal[1];
	//printf("[(%d)(%d), tzer->count=%d]\r\n", signal[0], signal[1], tzer->count);	
}

char at_sending[64];

void on_at_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
	memset(at_sending, '\0', sizeof(at_sending));
	if(strlen(tok[0].p) > sizeof(at_sending)) return;
	
	if( tzer->count == 1 ) {
		memcpy(at_sending, tok[0].p, tok[0].end - tok[0].p);
	} else {
	  strcat(at_sending, tok[0].p);
	}
	
	//printf("at_sending=%s, len=%d\r\n", at_sending, strlen(at_sending));
}

void on_at_cmd_success_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	if( !memcmp(at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) {
		printf("[AT:%s, result: OK]\r\n", at_sending);
	}
}

void on_at_cmd_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	if( !memcmp(at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) {
		dev->socket_close = 1;
		printf("[AT:%s, result: ERROR]\r\n", at_sending);
	} else if(!memcmp(at_sending, "AT+MIPOPEN=1,0,\"", strlen("AT+MIPOPEN=1,0,\""))) {
		//AT+MIPOPEN=1,0,"
		printf("[TCP CONNECT FAIL! AT: %s", at_sending);
	}
}

void on_simcard_type_callback(RemoteTokenizer *tzer, Token* tok)
{
	//+SIMTEST:3; 3
	dev->simcard_type = str2int(tok[0].p+9, tok[0].p+10);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
	//printf("simcard_type=%d\r\n", devStatus->simcard_type);
}

void remote_reader_parse( RemoteReader* r )
{
	int i;	
	Token* tok;

	RemoteTokenizer tzer[1];
	
	print_line(UART4, r->in, r->pos);
	/*打印4G的所有串口消息*/
	
	remote_tokenizer_init(tzer, r->in, r->in+r->pos);
	
	if(tzer->count == 0) return;
	
	tok = (Token*) mymalloc(0, (tzer->count*sizeof(Token)));
	if(tok == NULL) {
		printf("%s: count=%d, mymalloc error!\r\n", __func__, tzer->count);
		return;
	}
	
	for(i=0; i<tzer->count; i++) {
		tok[i] = remote_tokenizer_get(tzer, i);
	}
	
	if(!memcmp(tok[0].p, "OK", strlen("OK"))) {
		r->on_at_success(tzer);
		
	} else if(!memcmp(tok[0].p, "AT+", strlen("AT+"))) {
		r->on_at_command(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "ERROR", strlen("ERROR"))) {
		r->on_at_fail(tzer);
		
	} else if(!memcmp(tok[0].p, "+MIPRTCP=", strlen("+MIPRTCP="))&&(tzer->count>2)) {
		r->on_command(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "+MIPCALL:0", strlen("+MIPCALL:0"))) {
		r->on_ip_fail(tzer);
		
	} else if(!memcmp(tok[0].p, "+MIPCALL:1", strlen("+MIPCALL:1"))&&(tzer->count>1)) {
		r->on_ip_success(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "+CSQ:", strlen("+CSQ:"))&&(tzer->count>1)) {
		r->on_signal_strength(tzer, tok);
		
	}else if(!memcmp(tok[0].p, "+MIP:ERROR", strlen("+MIP:ERROR"))) {
		r->on_connect_fail(tzer);
		
	}else if(!memcmp(tok[0].p, "+MIPOPEN=", strlen("+MIPOPEN="))&&(tzer->count>1)) {
		if(str2int(tok[1].p, tok[1].end)==1) {
			r->on_connect_success(tzer, tok);
		}
		//+MIPOPEN=1,1
	} else if(!memcmp(tok[0].p, "+MIPCLOSE:", strlen("+MIPCLOSE:"))&&(tzer->count>=3)) {//4
		r->on_disconnect(tzer, tok);
		//+MIPCLOSE:1,12,0,0
	} else if(!memcmp(tok[0].p, "+SIMTEST:", strlen("+SIMTEST:"))) {
		r->on_simcard_type(tzer, tok);
		//+SIMTEST:3; 3
		//+SIMTEST:0;
	}
	
	myfree(0, tok);	
}

void remote_reader_addc( RemoteReader* r, int c )
{
    if (r->overflow) {
        r->overflow = (c != '\n');
        return;
    }

    if (r->pos >= (int) sizeof(r->in)) {
        r->overflow = 1;
        r->pos = 0;
        return;
    }	

    r->in[r->pos] = (char)c;
    r->pos += 1;

		if(c == '\n' /*|| c == '\0'*/) {
			remote_reader_parse(r);
			r->pos = 0;
			/*for fix bug. must memset r->in*/
			memset(r->in, '\0', sizeof(r->in));
		}
}

void handle_4g_uart_msg( void )
{
		int i = 0;
		long long j = 0;
		unsigned char ch;

		for(; i<2; i++) {	
			while(kfifo_len(uart3_fifo) > 0) {
				if(j++%20==0) IWDG_Feed();	
				kfifo_get(uart3_fifo, &ch, 1);
				
				remote_reader_addc(reader, ch);
			}	
		}
}

void notify_4g_period(void)
{
		dev->heartbeat_tick++;	
		dev->period_tick = 1;
		dev->connect_flag = 1;
		dev->ppp_flag = 1;
}

void init_longsung_status(char flag)
{
  int i;
	
	dev->boot_status = 0;
	dev->ppp_status = PPP_DISCONNECT;
	dev->scsq = 0;
	dev->rcsq = 0;
	dev->simcard_type = -1;

	dev->socket_close = 0;
	dev->socket_num = 0;

	dev->ppp_flag = 0;
	dev->connect_flag = 0;
	dev->heartbeat_tick = 0;
	dev->period_tick = 0;	
	
	if(flag) {
		dev->reset_request = 1;
		dev->is_inited = 1;		
	} else {
		dev->reset_request = 0;
		dev->is_inited = 0;		
	}
	
	for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) {
		dev->socket_open[i] = -1;
	}		
}

void handle_4g_setting(void)
{
	if( !mAndroidPower ) {
		int i;
		static long long mdelay[3] = { 0, 0 ,0 };
		
		/*android关闭，重启4G模块*/
		if( dev->reset_request ) {	
			init_longsung_status(0);
								
			//reset 4g modules by gpio
			printf("Reset 4G Module.\r\n");
		}
		
		if( dev->period_tick ) {
			dev->period_tick = 0;
			mdelay[0] = 0;
			
			/*查询信号强度*/
			at("AT+CSQ");
			
			if( dev->boot_status ) {
				dev->scsq++;
				if( (dev->scsq)-(dev->rcsq) > 3 ) {
					dev->reset_request = 1;
					dev->socket_close = 1;
					printf("scsq-rcsq=%d! error.\r\n", dev->scsq-dev->rcsq);
				}
				
				/*无SIM卡，等待用户插入，2分钟左右会重启4G模块*/
				if( dev->simcard_type == 0 && dev->scsq > 4 ) {
					dev->reset_request = 1;
					printf("sim card no exit!\r\n");
				}
			}
		}

		if( mdelay[0] <= 2000000 ) mdelay[0]++;
		
		/*查询SIM卡是否插入*/
		if( dev->simcard_type == -1 && 
				dev->boot_status && mdelay[0] == 500000 ) {
			at("AT+SIMTEST?");
		}
		
		/*查询IP*/
		if( mdelay[0] == 2000000 && dev->boot_status ) {
			at("AT+MIPCALL?");
		}
		
		/*PPP连接获取IP*/		
		if( dev->simcard_type != 0 && 
				dev->simcard_type != -1 && dev->boot_status ) {
			if( dev->ppp_flag == 1 && dev->ppp_status == PPP_DISCONNECT ) {
				dev->ppp_flag = 0;
				dev->ppp_status = PPP_CONNECTING;
				delay_ms(10);
				at("AT+MIPCALL=0");
				delay_ms(10);
				at("AT+MIPPROFILE=1,\"3GNET\"");
				delay_ms(10);
				at("AT+MIPCALL=1");
			}
		}
		
		/*检查活跃的socket个数，保持一个连接*/
		dev->socket_num = 0;
		
		for( i=0; i<4; i++ ) {
			if(dev->socket_open[i] != -1)
				dev->socket_num++;
		}
		
		/*若发现socket大于1，关闭所有socket,重新建立连接*/
		if( dev->socket_num > 1 || dev->socket_close == 1) {
			for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) {
				if( dev->socket_open[i] != -1 ) {
					switch( dev->socket_open[i] )
					{
						case 1: delay_ms(4); at("AT+MIPCLOSE=1"); delay_ms(6); break;
						case 2: delay_ms(4); at("AT+MIPCLOSE=2"); delay_ms(6); break;
						case 3: delay_ms(4); at("AT+MIPCLOSE=3"); delay_ms(6); break;
						case 4: delay_ms(4); at("AT+MIPCLOSE=4"); delay_ms(6); break;
						default: break;
					}
				}
			}
					
		  dev->socket_close = 0;
		}
			
		/*连接上远程服务端*/
		if( dev->ppp_status == PPP_CONNECTED && 
				dev->socket_num == 0 && dev->connect_flag == 1 ) {
					
			dev->connect_flag = 0;
			mdelay[1] = 0;
		}
		
		if( mdelay[1] <= 1000000 ) mdelay[1]++;
		
		if( dev->ppp_status == PPP_CONNECTED && 
					dev->socket_num == 0 && mdelay[1] == 900000 ) {
						
			at("AT+MIPOPEN=1,0,\"www.baidu.com\",80,0");
			//at("AT+MIPOPEN=1,0,\"112.124.102.62\",13334,0");			
			dev->heartbeat_tick = 0;
		}
		/*连接上远程服务端*/
		
		/*发送心跳包给服务*/
		if( dev->socket_num == 1 && dev->heartbeat_tick >= 3 ) {
			dev->heartbeat_tick = 0;
			mdelay[2] = 0;
		}
		
		if( mdelay[2] <= 1500000 ) mdelay[2]++;
		
		if( dev->socket_num == 1 && mdelay[2] == 1200000 ) {
			at("AT+MIPSEND=1,\"hello world.\"");
		}	
		
		if( dev->socket_num == 1 && mdelay[2] == 1400000 ) {
			at("AT+MIPPUSH=1");
		}
		/*发送心跳包给服务*/
		
		
	} else {
		if( !dev->is_inited ) {	
			init_longsung_status(1);
			
			//reset 4g modules by gpio
			printf("android power on, reset 4G module\r\n");
		}
	}
}

void init_remote_reader(void) 
{
	reader->inited = 1;
	reader->pos = 0;
	reader->overflow = 0;
	reader->on_command = service_command_callback;
	
	reader->on_connect_fail = service_connect_fail_callback;
	reader->on_connect_success = service_connect_callback;
	reader->on_disconnect = service_disconnect_callback;
	
	reader->on_signal_strength	= on_signal_strength_callback;
	reader->on_ip_success = on_request_ip_success_callback;
	reader->on_ip_fail = on_request_ip_fail_callback;
	
	reader->on_at_command = on_at_command_callback;
	reader->on_at_success = on_at_cmd_success_callback;
	reader->on_at_fail = on_at_cmd_fail_callback;
	
	reader->on_simcard_type = on_simcard_type_callback;	
}

void longsung_init()
{
	init_remote_reader();
	init_longsung_status(1);
}




