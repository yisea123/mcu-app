#include "longsung.h"

DevStatus devStatus[1];

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

void service_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	int len;
	
	//printf("%s!\r\n", __func__);
	len = str2int(tok[1].p, tok[1].end);
	printf("[tcp data coming...tzer->count=%d] [data len=%d] data=\r\n", 
			tzer->count, len);
	print_line_1(UART4, tok[2].p, tok[2].end - tok[2].p);	
}

void service_connect_callback(RemoteTokenizer *tzer)
{
	printf("%s!\r\n", __func__);
}

void service_connect_fail_callback(RemoteTokenizer *tzer)
{
	printf("%s!\r\n", __func__);
}

void service_disconnect_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	printf("[tcp connect close...tzer->count=%d]]\r\n", tzer->count);		
}

/*******************************************
+CSQ: 7,99
********************************************/
void on_signal_strength_callback(RemoteTokenizer *tzer, Token* tok)
{
	int signal[2];
	
	signal[0] = str2int(tok[0].p+6, tok[0].end);
	signal[1] = str2int(tok[1].p, tok[1].end);

	devStatus->boot_status = 1;
	devStatus->rcsq++;
	devStatus->singal[0] = signal[0];
	devStatus->singal[1] = signal[1];
	//printf("[(%d)(%d), tzer->count=%d]\r\n", signal[0], signal[1], tzer->count);	
}

/*******************************************
+MIPCALL:1,10.154.27.94		
********************************************/
void on_request_ip_success_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[get ip success...tzer->count=%d]], ip = \r\n", tzer->count);	
	//print_line_1(UART4, tok[1].p, tok[1].end - tok[1].p);
	devStatus->ppp_status = PPP_CONNECTED;
}

void on_request_ip_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[get ip fail...tzer->count=%d]]\r\n", tzer->count);
	devStatus->ppp_status = PPP_DISCONNECT;
	/*获取IP失败，重新获取*/		
}

void on_at_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
}

void on_at_cmd_success_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);	
}

void on_at_cmd_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	printf("[at command error...tzer->count=%d]\r\n", tzer->count);	
}

void on_simcard_type_callback(RemoteTokenizer *tzer, Token* tok)
{
	//+SIMTEST:3; 3
	devStatus->simcard_type = str2int(tok[0].p+9, tok[0].p+10);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
	printf("simcard_type=%d\r\n", devStatus->simcard_type);
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
			r->on_connect_success(tzer);
		}
		//+MIPOPEN=1,1
	} else if(!memcmp(tok[0].p, "+MIPCLOSE:", strlen("+MIPCLOSE:"))) {
		r->on_disconnect(tzer);
		
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
		}
}

static RemoteReader reader[1];

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

char check_period = 0, ppp_count = 0;

void notify_4g_period(void)
{
		check_period = 1;
		ppp_count = 1;
}

void handle_4g_setting(void)
{
	if(!mAndroidPower) 
	{
		static long long mdelay = 0;
		
		if(devStatus->reset_request) {
			devStatus->reset_request = 0;
			devStatus->is_inited = 0;
			
			devStatus->boot_status = 0;
			devStatus->ppp_status = PPP_DISCONNECT;
			devStatus->scsq = 0;
			devStatus->rcsq = 0;
			devStatus->simcard_type = -1;	
			
			//reset 4g modules by gpio
			printf("Reset 4G Module.\r\n");
		}
		
		if(check_period) {
			check_period = 0;
			mdelay = 0;
			
			/*查询信号强度*/
			at("AT+CSQ");
			
			if(devStatus->boot_status) {
				devStatus->scsq++;
				if((devStatus->scsq)-(devStatus->rcsq) > 3) {
					devStatus->reset_request = 1;
					printf("!!!!!scsq-rcsq=%d\r\n", devStatus->scsq-devStatus->rcsq);
				}
				
				/*无SIM卡，等待用户插入，2分钟左右会重启4G模块*/
				if(devStatus->simcard_type == 0 && devStatus->scsq > 4) {
					devStatus->reset_request = 1;
					printf("sim card no exit!\r\n");
				}
			}
		}

		if(mdelay <= 1000000) mdelay++;
		
		/*查询SIM卡是否插入*/
		if(devStatus->boot_status && mdelay == 200000 && 
				devStatus->simcard_type == -1) {
			at("AT+SIMTEST?");
		}
		
		/*查询IP*/
		if(devStatus->boot_status && mdelay == 1000000) {
			at("AT+MIPCALL?");
		}
		
		/*PPP连接获取IP*/		
		if(devStatus->boot_status && devStatus->simcard_type != -1 &&
				devStatus->simcard_type != 0) {
			if(devStatus->ppp_status == PPP_DISCONNECT && ppp_count == 1) {
				ppp_count = 0;
				devStatus->ppp_status = PPP_CONNECTING;
				delay_ms(5);
				at("AT+MIPCALL=0");
				delay_ms(10);
				at("AT+MIPPROFILE=1,\"3GNET\"");
				delay_ms(10);
				at("AT+MIPCALL=1");
			}
		}
		
	}
	
	else 
	{
		if(!devStatus->is_inited) {
			devStatus->reset_request = 1;
			devStatus->boot_status = 0;
			devStatus->ppp_status = PPP_DISCONNECT;
			devStatus->scsq = 0;
			devStatus->rcsq = 0;
			devStatus->simcard_type = -1;
			devStatus->is_inited = 1;
			
			//reset 4g modules by gpio
			printf("android power on, reset 4G module\r\n");
		}
	}
}

void remote_reader_init()
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
	
	devStatus->reset_request = 1;
	devStatus->boot_status = 0;
	devStatus->ppp_status = PPP_DISCONNECT;
	devStatus->scsq = 0;
	devStatus->rcsq = 0;
	devStatus->simcard_type = -1;
}





