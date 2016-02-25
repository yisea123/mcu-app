#include "longsung.h"

DevStatus dev[1];
static UartReader reader[1];

void print_char(USART_TypeDef* USARTx, char ch)
{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, ch);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
}

void print_line(USART_TypeDef* USARTx, const char* data, int len)
{

	int t;
	/*for fix bug, t must be int type*/
	for(t=0;t<len;t++) 
	{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, data[t]);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
	}		
	
}

void print_line_1(USART_TypeDef* USARTx, const char* data, int len)
{

	int t;
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

void select_sort(int a[], int len)  
{  
    int i,j,x,l;  
    for(i=0;i<len;i++)  
    {  
        x=a[i];  
        l=i;  
        for(j=i;j<len;j++)  
        {  
            if(a[j]<x)  
            {  
                x=a[j];  
                l=j;  
            }  
        }  
        a[l]=a[i];  
        a[i]=x;  
    }  
}  

/*send AT commnad to 4G*/
void send_at_command(const char* ack, int ack_len)
{
	int t;
	for(t=0; t<ack_len; t++) 
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

static int longsung_tokenizer_init( RemoteTokenizer* t, const char* p, const char* end )
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

static Token longsung_tokenizer_get( RemoteTokenizer* t, int index )
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
	/*��ȡIPʧ�ܣ����»�ȡ*/		
}

void on_remote_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	int len;
	
	//printf("%s!\r\n", __func__);
	len = str2int(tok[1].p, tok[1].end);
	printf("[tcp data coming...tzer->count=%d] [data len=%d] data=\r\n", 
			tzer->count, len);
	print_line_1(UART4, tok[2].p, tok[2].end - tok[2].p);	
}

void on_connect_service_success_callback(RemoteTokenizer *tzer, Token* tok)
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

void on_connect_service_fail_callback(RemoteTokenizer *tzer)
{
	//+MIP:ERROR
	//printf("[%s, remote service error.]\r\n", __func__);
}

void on_disconnect_service_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//+MIPCLOSE:2
	int socketId = str2int(tok[0].p+10, tok[0].end);
	dev->socket_open[socketId-1] = -1;
	//printf("[tcp connect close...socketId=%d]\r\n", socketId);
	//printf("%d, %d, %d, %d\r\n", devStatus->socket_open[0], devStatus->socket_open[1]
	//	, devStatus->socket_open[2], devStatus->socket_open[3]);
	//dev->connect_flag = 0;
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

void on_at_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
	memset(dev->at_sending, '\0', sizeof(dev->at_sending));
	if(strlen(tok[0].p) > sizeof(dev->at_sending)) return;
	
	if( tzer->count == 1 ) {
		memcpy(dev->at_sending, tok[0].p, tok[0].end - tok[0].p);
	} else {
	  strcat(dev->at_sending, tok[0].p);
	}
	
	//printf("at_sending=%s, len=%d\r\n", at_sending, strlen(at_sending));
}

void on_at_cmd_success_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) {
		printf("----------------------------------\r\n");
		printf("[result: OK] %s\r\n", dev[0].at_sending);
		
	} else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) {
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		/*�������Ƕ��Ų�ѯ��������*/
		printf("[result: OK] %s\r\n", dev->at_sending);		

		/*ɾ�����ųɹ�֮����ܶ�ȡ��һ�����ţ�������ɹ�
		��������Զ�������ٶ�ȡ����at fail��Ҳ����fix it.*/
		if(dev->sm_num > 0) {
			int i, delflag = 0;
			for(i=0; i< dev->sm_num; i++) {
				if(dev->sm_index[i] == dev->sm_index_delete) {
					delflag = 1;
					break;
				}
			}
			/*�����������ɾ�������кţ��Ž���ɾ����*/
			if(delflag) {
				for(; i<dev->sm_num-1; i++) {
					dev->sm_index[i] = dev->sm_index[i+1];
				}
				dev->sm_index[i] = -1;
				dev->sm_num--;
			}
			
			printf("UPDATE sm_num=%d, sm_index={ ", dev->sm_num);
			for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
				if(dev->sm_index[i] != -1)
					printf("%d, ", dev->sm_index[i]);
			}
			printf("}\r\n");				
		}
		/*ɾ�����ųɹ�����ȡ��һ������*/

		dev->sm_index_delete = -1;
		dev->sm_delete_flag = 1;
		dev->sm_read_flag = 1;		
	}
}

void on_at_cmd_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) {
		dev->socket_close_flag = 1;
		printf("[AT:%s, result: ERROR]\r\n", dev->at_sending);
	} else if(!memcmp(dev->at_sending, "AT+MIPOPEN=1,0,\"", strlen("AT+MIPOPEN=1,0,\""))) {
		//AT+MIPOPEN=1,0,"
		printf("[TCP CONNECT FAIL! AT: %s", dev->at_sending);
	} else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) {
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		printf("[AT:%s, result: ERROR]\r\n", dev->at_sending);
		dev->sm_delete_flag = 1;
		/*����ִ��ɾ�����Ź���*/
		dev->sm_read_flag = 1;
	} else if( !memcmp(dev->at_sending, "AT+CMGR=", strlen("AT+CMGR=")) ) {
		/*��ʧ�ܣ����¶�*/
		dev->sm_read_flag = 1;
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

void on_sm_check_callback(RemoteTokenizer *tzer, Token* tok)
{
	int i;
	
	if(tzer->count == 2) {
		/*+CMGD: (),(0-4)*/
		/*+CMGD: (0),(0-4)*/
		if(!memcmp(tok[0].p, "+CMGD: ()", strlen("+CMGD: ()"))) {
			i = 0;
			dev->sm_num = 0;
		} else {
			i = 1;
			dev->sm_num = 1;
			dev->sm_index[0] = str2int(tok[0].p+8, tok[0].end-1);
		}
		for(; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
			dev->sm_index[i] = -1;
		}
	} else if(tzer->count > 2) {
		/*+CMGD: (0,1,2,4,5,6),(0-4)*/
		dev->sm_num = tzer->count - 1;
		if(dev->sm_num > sizeof(dev->sm_index)/sizeof(dev->sm_index[0])) {
			dev->sm_num = sizeof(dev->sm_index)/sizeof(dev->sm_index[0]);
		}
		
		for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
			if( i < dev->sm_num ) {
				if( i==0 ) 
					dev->sm_index[i] = str2int(tok[i].p+8, tok[i].end);
				else if( i== (dev->sm_num -1) )
					dev->sm_index[i] = str2int(tok[i].p, tok[i].end-1);
				else 
					dev->sm_index[i] = str2int(tok[i].p, tok[i].end);
			} else {
				dev->sm_index[i] = -1;
			}
		}
	}
	
	printf("sm_num=%d, sm_index={ ", dev->sm_num);
	for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
		if(dev->sm_index[i] != -1)
			printf("%d, ", dev->sm_index[i]);
	}
	printf("}\r\n");
}

void on_sm_read_callback(RemoteTokenizer *tzer, Token* tok)
{
	printf("SM read, index=%d\r\n", dev->sm_index_read);
	dev->sm_data_flag = 1;
}

void on_sm_data_callback(RemoteTokenizer *tzer, Token* tok, int index)
{
	printf("SM data, index=%d\r\n", index);
	printf("DATA:%s", tok[0].p);
	/*ֻ�ɶ���һ�У���ˣ��������ݲ�����\n���У�*/
	dev->sm_index_delete = index;
	dev->sm_delete_flag = 1;
	/*��ȡ���������ݣ�ɾ��֮*/
}

void on_sm_notify_callback(RemoteTokenizer *tzer, Token* tok)
{
	/*+CMTI: "SM",1*/
	int i, index, addflag = 1;
	
	index = str2int(tok[1].p, tok[1].end);
	printf("SM notify: index=%d\r\n", index);
	
	if(index != -1) {
		if(dev->sm_num >= sizeof(dev->sm_index)/sizeof(dev->sm_index[0]))
			return;
		
		/*************************************
			for fix bug.
			13-03-40: AT+CMGD=?
			13-03-40: +CMGD: (0,1),(0-4)
			13-03-40: sm_num=2, sm_index={ 0, 1, }
			13-03-40: OK
			13-03-40: 
			13-03-40: +CMTI: "SM",1
			13-03-40: SM notify: index=1
			13-03-40: sm_num=3, sm_index={ 0, 1, 1, }
			13-03-41: AT+MIPCALL?
			13-03-41: +MIPCALL:1,10.124.237.39
			13-03-41: 
			13-03-41: OK		
		*************************************/
		for(i=0; i<dev->sm_num; i++) {
			if(index == dev->sm_index[i]) 
				addflag = 0;
		}
		
		if(addflag) {
			dev->sm_index[dev->sm_num] = index;
			dev->sm_num++;
			select_sort(dev->sm_index, dev->sm_num);
			/*��������*/
		}
		
		printf("sm_num=%d, sm_index={ ", dev->sm_num);
		for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
			if(dev->sm_index[i] != -1)
				printf("%d, ", dev->sm_index[i]);
		}
		printf("}\r\n");		
	}
}

void on_sm_read_err_callback(RemoteTokenizer *tzer, Token* tok)
{
	/*******************************
		�������˴���Ķ���indexʱ����
		AT+CMGR=1

		+CMS ERROR: 321
	********************************/
	printf("SM read index error\r\n");
	dev->sm_read_flag = 1;	
}

static void longsung_reader_parse( UartReader* r )
{
	int i;	
	Token* tok;

	RemoteTokenizer tzer[1];
	
	//printf("r->pos=%d\r\n", r->pos);	
	print_line(UART4, r->in, r->pos);
	/*��ӡ4G�����д�����Ϣ, release �汾ʱȥ����*/
	
	longsung_tokenizer_init(tzer, r->in, r->in+r->pos);
	
	if(tzer->count == 0) return;
	
	tok = (Token*) mymalloc(0, (tzer->count*sizeof(Token)));
	if(tok == NULL) {
		printf("%s: count=%d, mymalloc error!\r\n", __func__, tzer->count);
		return;
	}
	
	for(i=0; i<tzer->count; i++) {
		tok[i] = longsung_tokenizer_get(tzer, i);
	}
	
	if(dev->sm_data_flag) {
		dev->sm_data_flag = 0;
		r->on_sm_data(tzer, tok, dev->sm_index_read);
	}
	
	if(!memcmp(tok[0].p, "AT+", strlen("AT+"))) {
		r->on_at_command(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "OK", strlen("OK"))) {
		r->on_at_success(tzer);
		
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

	} else if(!memcmp(tok[0].p, "+MIPCLOSE:", strlen("+MIPCLOSE:"))&&(tzer->count>=3)) {//4
		r->on_disconnect(tzer, tok);

	} else if(!memcmp(tok[0].p, "+SIMTEST:", strlen("+SIMTEST:"))) {
		r->on_simcard_type(tzer, tok);

	} else if (!memcmp(tok[0].p, "+CMGD: (", strlen("+CMGD: ("))) {
		r->on_sm_check(tzer, tok);

	} else if (!memcmp(tok[0].p, "+CMGR: \"REC READ\"", strlen("+CMGR: \"REC READ\"")) ||
		!memcmp(tok[0].p, "+CMGR: \"REC UNREAD\"", strlen("+CMGR: \"REC UNREAD\""))) {
		r->on_sm_read(tzer, tok);

	} else if (!memcmp(tok[0].p, "+CMTI: \"SM\",", strlen("+CMTI: \"SM\","))) {
		r->on_sm_notify(tzer, tok);

	} else if(!memcmp(tok[0].p, "+CMS ERROR: 321", strlen("+CMS ERROR: 321"))) {
		r->on_sm_read_err(tzer, tok);
		
	}
	
	myfree(0, tok);	
}

static void longsung_reader_addc( UartReader* r, int c )
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
			longsung_reader_parse(r);
			r->pos = 0;
			/*for fix bug. must memset r->in*/
			memset(r->in, '\0', sizeof(r->in));
		}
}

void handle_longsung_uart_msg( void )
{
		int i = 0;
		long long j = 0;
		unsigned char ch;

		for(; i<2; i++) {	
			while(kfifo_len(uart3_fifo) > 0) {
				if(j++%20==0) IWDG_Feed();	
				kfifo_get(uart3_fifo, &ch, 1);
				
				if(!mAndroidPower)
					longsung_reader_addc(reader, ch);
			}	
		}
}

void notify_longsung_period(void)
{	
		dev->period_tick = 1;
}

static void init_longsung_reader(void) 
{
	reader->inited = 1;
	reader->pos = 0;
	reader->overflow = 0;
	
	reader->on_simcard_type = on_simcard_type_callback;	
	reader->on_signal_strength	= on_signal_strength_callback;
	
	reader->on_command = on_remote_command_callback;
	reader->on_connect_fail = on_connect_service_fail_callback;
	reader->on_connect_success = on_connect_service_success_callback;
	reader->on_disconnect = on_disconnect_service_callback;
	
	reader->on_ip_success = on_request_ip_success_callback;
	reader->on_ip_fail = on_request_ip_fail_callback;
	
	reader->on_at_command = on_at_command_callback;
	reader->on_at_success = on_at_cmd_success_callback;
	reader->on_at_fail = on_at_cmd_fail_callback;
	
	reader->on_sm_check = on_sm_check_callback;
	reader->on_sm_read = on_sm_read_callback;
	reader->on_sm_data = on_sm_data_callback;
	reader->on_sm_notify = on_sm_notify_callback;
	reader->on_sm_read_err = on_sm_read_err_callback;
}

static void init_longsung_status(char flag)
{
  int i;
	
	dev->boot_status = 0;
	dev->ppp_status = PPP_DISCONNECT;
	dev->scsq = 0;
	dev->rcsq = 0;
	dev->simcard_type = -1;

	dev->socket_close_flag = 0;
	dev->socket_num = 0;

	dev->heartbeat_tick = 0;
	dev->period_tick = 0;	
	
	dev->sm_num = 0;
	dev->sm_data_flag = 0;
	dev->sm_index_delete = -1;
	
	dev->sm_read_flag = 1;
	dev->sm_delete_flag = 1;
	
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
	
	for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) {
		dev->sm_index[i] = -1;
	}
}

static void do_read_sm(int index)
{
	dev->sm_index_read = index;
	
	switch(index)
	{
		case 0: at("AT+CMGR=0"); break;
		case 1: at("AT+CMGR=1"); break;
		case 2: at("AT+CMGR=2"); break;
		case 3: at("AT+CMGR=3"); break;
		case 4: at("AT+CMGR=4"); break;
		case 5: at("AT+CMGR=5"); break;
		case 6: at("AT+CMGR=6"); break;
		case 7: at("AT+CMGR=7"); break;		
		case 8: at("AT+CMGR=8"); break;
		case 9: at("AT+CMGR=9"); break;
		case 10: at("AT+CMGR=10"); break;
		case 11: at("AT+CMGR=11"); break;
		case 12: at("AT+CMGR=12"); break;
		case 13: at("AT+CMGR=13"); break;
		case 14: at("AT+CMGR=14"); break;
		case 15: at("AT+CMGR=15"); break;
		case 16: at("AT+CMGR=16"); break;
		case 17: at("AT+CMGR=17"); break;		
		case 18: at("AT+CMGR=18"); break;
		case 19: at("AT+CMGR=19"); break;
		default: break;
	}
}

static void do_delete_sm(int index)
{
	switch(index)
	{
		case 0: at("AT+CMGD=0"); break;
		case 1: at("AT+CMGD=1"); break;
		case 2: at("AT+CMGD=2"); break;
		case 3: at("AT+CMGD=3"); break;
		case 4: at("AT+CMGD=4"); break;
		case 5: at("AT+CMGD=5"); break;
		case 6: at("AT+CMGD=6"); break;
		case 7: at("AT+CMGD=7"); break;		
		case 8: at("AT+CMGD=8"); break;
		case 9: at("AT+CMGD=9"); break;
		case 10: at("AT+CMGD=10"); break;
		case 11: at("AT+CMGD=11"); break;
		case 12: at("AT+CMGD=12"); break;
		case 13: at("AT+CMGD=13"); break;
		case 14: at("AT+CMGD=14"); break;
		case 15: at("AT+CMGD=15"); break;
		case 16: at("AT+CMGD=16"); break;
		case 17: at("AT+CMGD=17"); break;		
		case 18: at("AT+CMGD=18"); break;
		case 19: at("AT+CMGD=19"); break;
		default: break;
	}	
}

static void do_close_socket(int index)
{
	switch( index )
	{
		case 1: at("AT+MIPCLOSE=1"); break;
		case 2: at("AT+MIPCLOSE=2"); break;
		case 3: at("AT+MIPCLOSE=3"); break;
		case 4: at("AT+MIPCLOSE=4"); break;
		default: break;
	}	
}

/***************************************************************
�����ݼ��뵽������ȥ
****************************************************************/
static void add_cmd_to_list(AtCommand *cmd)
{
	if(cmd)
		list_add_tail(&cmd->list, &dev->at_head);
}

/***************************************************************
��������ݿ������������ҵ�����������ɾ���������
****************************************************************/
static void del_cmd_from_list(AtCommand *cmd)
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &dev->at_head) 
	{ 
		command = list_entry(pos, AtCommand, list); 
		if(command == cmd && cmd != NULL) 
		{ 
				list_del(pos);
		} 
	} 
}

/***************************************************************
�����������û�����ݣ��򷵻�NULL����������ݾͷ���һ������
****************************************************************/
static AtCommand* get_cmd_from_list()
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &dev->at_head)
	{
		command = (AtCommand *)list_entry(pos, AtCommand, list);
		break;
	}	

	return command;
}

static int check_command_exist(int index)
{
	int ret = 0;
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &dev->at_head)
	{
		command = (AtCommand *)list_entry(pos, AtCommand, list);
		if(index == command->index) {
			ret = 1;
			break;
		}
	}	

	return ret;	
}

static int get_at_command_count(void)
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &dev->at_head)
	{
		count++;	
	}
	
	return count;
}

static void send_command_to_device(AtCommand* cmd)
{
	if(cmd == NULL) return;
	
	//printf("%s, index=%d\r\n", __func__, cmd->index);

	switch(cmd->index)
	{
		case ATCSQ: at("AT+CSQ"); break;
		case ATSIMTEST: at("AT+SIMTEST?"); break;
		case ATCPMS: at("AT+CPMS=\"SM\""); break;
		case ATCMGD_: at("AT+CMGD=?"); break;
		case ATMIPCALL_: at("AT+MIPCALL?"); break;
		case ATMIPCALL0: at("AT+MIPCALL=0"); break;
		case ATMIPPROFILE: at("AT+MIPPROFILE=1,\"3GNET\""); break;
		case ATMIPCALL1: at("AT+MIPCALL=1"); break;
		case ATMIPOPEN: at("AT+MIPOPEN=1,0,\"www.baidu.com\",80,0"); break;
		/*at("AT+MIPOPEN=1,0,\"112.124.102.62\",13334,0");*/
		case ATMIPSEND: at("AT+MIPSEND=1,\"Linux\""); break;
		case ATMIPPUSH: at("AT+MIPPUSH=1"); break;
		case ATCMGF: at("AT+CMGF=1"); break;
		case ATMIPCLOSE: do_close_socket(cmd->para); break;		
		case ATCMGR: do_read_sm(cmd->para); break;
		case ATCMGD: do_delete_sm(cmd->para); break;		
		default: break;
	}
}

static void make_command_to_list(char index, long long interval, int para)
{
	AtCommand *command = NULL;	
	command = (AtCommand *)mymalloc(0, sizeof(AtCommand));
	if(command != NULL) {
		command->index = index;
		command->interval = interval;
		command->para = para;
		add_cmd_to_list(command);
	}	
}

void handle_longsung_setting(void)
{
	int i;
	static char period = 0;
	static AtCommand* cmdSending = NULL;
	
	if( !mAndroidPower ) {
		/*android�رգ�����4Gģ��*/
		if( dev->reset_request ) {	
			init_longsung_status(0);
								
			//reset 4g modules by gpio
			printf("Reset 4G Module.\r\n");
		}
		
		if( dev->period_tick ) {
			dev->heartbeat_tick++;
			/*���ڷ���tick��Ϣ�����ڵ���*/
			period = 1;
			/*Ϊ�˽��ͬ�������⣬����period����*/
			dev->period_tick = 0;
			printf("*********************************\r\n");
			/*��ѯ�ź�ǿ�ȣ�ÿ40�����һ��*/
			make_command_to_list(ATCSQ, ONE_SECOND/5, -1);
			
			/*��ѯSIM���Ƿ����, -1Ϊ��ʼ��״̬��0Ϊ�޿�*/
			if( dev->simcard_type == -1 ) {			
				/*Ϊ�˵�һʱ�������Ϸ�����*/
				make_command_to_list(ATSIMTEST, ONE_SECOND/5, -1);
				make_command_to_list(ATMIPCALL_, ONE_SECOND/5, -1);
				make_command_to_list(ATMIPCALL0, ONE_SECOND/5, -1);	
				make_command_to_list(ATMIPPROFILE, ONE_SECOND/5, -1);	
				make_command_to_list(ATMIPCALL1, ONE_SECOND, -1);
				make_command_to_list(ATMIPOPEN, ONE_SECOND/2, -1);						
				make_command_to_list(ATMIPCALL_, ONE_SECOND/5, -1);		
				dev->heartbeat_tick = 0;				
			}			
		}

		if( dev->boot_status ) {
			/*��ʾ4G���������*/
			if( period ) {
				dev->scsq++;
				if( (dev->scsq)-(dev->rcsq) > 3 ) {
					dev->reset_request = 1;
					dev->socket_close_flag = 1;
					printf("scsq-rcsq=%d! error.\r\n", dev->scsq-dev->rcsq);
				}
				
				/*��SIM�����ȴ��û����룬2�������һ�����4Gģ��*/
				if( dev->simcard_type == 0 && dev->scsq > 4 ) {
					dev->reset_request = 1;
					printf("sim card no exit!\r\n");
				}				

				if( dev->simcard_type > 0 ) {
						/*��ѯSM����δ��index*/
						make_command_to_list(ATCPMS, ONE_SECOND/5, -1);				
						make_command_to_list(ATCMGD_, ONE_SECOND/5, -1);
						/*��ѯSM����δ��index,��ѯ���ж��ţ��ͻ����Ͻ����ģ�����*/
					
						/*��ѯIP*/
						make_command_to_list(ATMIPCALL_, ONE_SECOND/5, -1);							
				}
				
				/*PPP���ӻ�ȡIP*/		
				if( dev->simcard_type > 0 && dev->ppp_status == PPP_DISCONNECT ) {
					dev->ppp_status = PPP_CONNECTING;
					make_command_to_list(ATMIPCALL0, ONE_SECOND/5, -1);	
					make_command_to_list(ATMIPPROFILE, ONE_SECOND/5, -1);	
					make_command_to_list(ATMIPCALL1, ONE_SECOND, -1);								
				}					
			}
			
			/*����Ծ��socket����������һ������*/
			dev->socket_num = 0;
			
			for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) {
				if(dev->socket_open[i] != -1)
					dev->socket_num++;
			}
			
			/*������socket����1���ر�����socket,���½�������*/
			if( (dev->socket_num > 1 || dev->socket_close_flag) ) {
				if(!check_command_exist(ATMIPCLOSE)) { 
					for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) {
						if( dev->socket_open[i] != -1 ) {
							make_command_to_list(ATMIPCLOSE, ONE_SECOND/5, dev->socket_open[i]);											
						}
					}
				}
				if(dev->socket_close_flag == 1) dev->socket_close_flag = 0;
			}
				
			/*������Զ�̷����*/
			if( dev->ppp_status == PPP_CONNECTED && dev->socket_num == 0 ) {
				if( period ) {
					make_command_to_list(ATMIPOPEN, ONE_SECOND/2, -1);							
					dev->heartbeat_tick = 0;
				}
			}
			
			/*����������������*/
			if( dev->socket_num == 1 && dev->ppp_status == PPP_CONNECTED) {
				if( dev->heartbeat_tick >= 50 ) {//3
					dev->heartbeat_tick = 0;
					make_command_to_list(ATMIPSEND, ONE_SECOND/2, -1);	
					make_command_to_list(ATMIPPUSH, ONE_SECOND/5, -1);					
				}
			}

			/*ɾ��sim���еĶ���*/
			if( dev->sm_index_delete != -1 && dev->sm_delete_flag ) {
				make_command_to_list(ATCPMS, ONE_SECOND/5, -1);
				make_command_to_list(ATCMGD, ONE_SECOND/5, dev->sm_index_delete);					
				dev->sm_delete_flag = 0;
			}
			
			/*��ȡsim���еĶ���*/
			if( dev->sm_num > 0 && dev->sm_read_flag ) {
				make_command_to_list(ATCPMS, ONE_SECOND/5, -1);	
				make_command_to_list(ATCMGF, ONE_SECOND/5, -1);
				make_command_to_list(ATCMGR, ONE_SECOND/3, dev->sm_index[0]);					
				dev->sm_read_flag = 0;
			}

		}

		/*��ȡҪ���͵�ATָ�������*/
		if(cmdSending == NULL) {
			cmdSending = get_cmd_from_list();
			if(cmdSending)
				send_command_to_device(cmdSending);
		} else {
			if(cmdSending->interval > 0) {
				cmdSending->interval--;
			} else {
				del_cmd_from_list(cmdSending);
				myfree(0, cmdSending);
				cmdSending = NULL;
			}
		}	
		
		dev->at_count = get_at_command_count();
		period = 0;		
	} else {
		if( !dev->is_inited ) {
			
			for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) {
				if( dev->socket_open[i] != -1 ) {
					switch( dev->socket_open[i] )
					{
						case 1: delay_ms(5); at("AT+MIPCLOSE=1"); delay_ms(10); break;
						case 2: delay_ms(4); at("AT+MIPCLOSE=2"); delay_ms(10); break;
						case 3: delay_ms(5); at("AT+MIPCLOSE=3"); delay_ms(10); break;
						case 4: delay_ms(5); at("AT+MIPCLOSE=4"); delay_ms(10); break;
						default: break;
					}
				}
			}			
			
			init_longsung_status(1);
			
			//reset 4g modules by gpio
			printf("android power on, reset 4G module\r\n");
		}
	}
}

void longsung_init()
{
	init_longsung_reader();
	init_longsung_status(1);
	INIT_LIST_HEAD(&dev->at_head);
}







