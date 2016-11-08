#include "longsung.h"

#define NOW_TICK		xTaskGetTickCount()
#define SIZE_ARRAY(a) 	(sizeof(a) / sizeof((a)[0]))

Ringfifo 	   	 			uart3fifo[1];
static uint32_t 			uartlost = 0;
static unsigned char 		tcp_read[750];
TimerHandle_t 				xTimer; 
DevStatus 					dev[1];
static UartReader 			reader[1];
mqtt_dev_status 			mqtt_dev[1];
static char 				mAndroidPowerStatus = 0;

/*buffer manager for at command and mqtt buffer*/
AtCommand 					*pAtCommand;
MqttBuffer					*pMqttBuff[3];
/*buffer manager for at command and mqtt buffer*/

extern void cmodule_power_reset( void );
static char* make_mqtt_packet(char* buff, unsigned char* data, int len);
static void make_command_to_list(char index, unsigned int interval, int para);
static void mqtt_set_mesg_ack(int type, uint16_t msg_id);
static void atcmd_set_ack(int index);


MqttBuffer * do_init_mqtt_buffer( int num, int capacity )
{
	int i = 0;
	MqttBuffer *mbuffer;

	if( num == 0 || capacity == 0 )
	{
		printf("%s: num(%d), capacity(%d) error!\r\n", __func__, num, capacity);
		return NULL;
	}
	mbuffer = ( MqttBuffer * ) pvPortMalloc( sizeof( MqttBuffer ) * num );
	if( !mbuffer )
	{
		printf("%s: pvPortMalloc pMqttBuff fail\r\n", __func__);
		return NULL;
	}	
	for( i = 0; i < num; i++ )
	{
		mbuffer[i].buff = (char *) pvPortMalloc( capacity );
		if( !mbuffer[i].buff )
		{
			printf("%s: pvPortMalloc MqttBuffer.buff fail\r\n", __func__);
			mbuffer[i].capacity = 0;
			mbuffer[i].num = num;
			mbuffer[i].be_used = 1;			
			continue;
		}
		else
		{
			mbuffer[i].capacity = capacity;
			mbuffer[i].num = num;
			mbuffer[i].be_used = 0;
		}
	}

	return mbuffer;
}

void init_mqtt_buffer( void )
{
	int i = 0;
	int num = 0;
	int capacity = 0;
	
	for( i = 0; i < SIZE_ARRAY( pMqttBuff ); i++ )
	{
		if( i == 0 )
		{
			num = 40;
			capacity = 50;
		} 
		else if( i == 1 )
		{
			num = 3;
			capacity = 300;
		} 
		else if( i == 2 )
		{
			num = 1;
			capacity = 1024;
		}
		
		pMqttBuff[i] = do_init_mqtt_buffer( num, capacity );
		if( !pMqttBuff[i] )
		{
			printf("%s: pvPortMalloc pMqttBuff[%d] fail\r\n", __func__, i);
			break;
		}
	}
}

char* alloc_mqtt_buffer( int len )
{
	int i = 0;
	int j = 0;
	MqttBuffer* p;
	
	for( i = 0; i < SIZE_ARRAY( pMqttBuff ); i++ )
	{	
		p = pMqttBuff[i];
		if( p )
		{
			if( len < p[0].capacity )
			{
				for( j = 0; j < p[0].num; j++ )
				{
					if( p[j].be_used == 0 ) 
					{
						printf("%s: alloc node ok! len(%d), pMqttBuff[%d][%d].apacity(%d)\r\n", 
							__func__, len, i, j, p[0].capacity);					
						p[j].be_used = 1;
						return p[j].buff;
					}
				}
			}
		}
	}
	
	printf("%s: alloc node fail! len(%d)\r\n", __func__, len );
	return NULL;
}

void dealloc_mqtt_buffer( char *buff )
{
	int i = 0;
	int j = 0;
	MqttBuffer* p;
	
	for( i = 0; i < SIZE_ARRAY( pMqttBuff ); i++ )
	{	
		p = pMqttBuff[i];
		if( p )
		{
			for( j = 0; j < p[0].num; j++ )
			{
				if( p[j].be_used == 1 &&  p[j].buff == buff ) 
				{
					printf("%s: dealloc node ok! pMqttBuff[%d][%d]\r\n", __func__, i, j);					
					p[j].be_used = 0;
					return;
				}
			}
		}
	}

	printf("%s: dealloc node fail!\r\n", __func__ );
}

void init_command_buffer( unsigned char num )
{
	int i = 0;

	pAtCommand = ( AtCommand * )pvPortMalloc( sizeof( AtCommand ) * num );
	if( !pAtCommand )
	{
		printf("%s: pvPortMalloc AtCommand fail\r\n", __func__);
		return;
	}
	
	for( i = 0; i < num; i++ )
	{
		pAtCommand[i].num = num;
		pAtCommand[i].be_used = 0;
	}
}

AtCommand* alloc_command( void )
{
	int i = 0;

	if( !pAtCommand )
	{
		return NULL;
	}

	for( i = 0; i < pAtCommand[0].num; i++ )
	{
		if( pAtCommand[i].be_used == 0 )
		{
			pAtCommand[i].be_used = 1;
			return &( pAtCommand[i] );
		}
	}

	printf("%s: alloc_command fail!\r\n", __func__);
	return NULL;
}

void dealloc_command( AtCommand* command )
{
	if( command && command->be_used )
	{
		command->be_used = 0;
		command->mqtype = MQTT_MSG_TYPE_NULL;
		command->mqttdata = NULL;
		command->mqttdata_len = 0;
		command->tick_sum = 0;
		command->mqtt_clean = 0;
		command->mqtt_try = 0;
		command->atack = 0;
		command->mqttack = 0;
		command->index = 0;
		command->para = 0;
	}
	else
	{
		printf("%s: dealloc_command fail! command->be_used(%d)\r\n", __func__, command->be_used);
	}
}

void print_char(USART_TypeDef* USARTx, char ch)
{
		USART_ClearFlag(USARTx, USART_FLAG_TC); 
		USART_SendData(USARTx, ch);
		while(USART_GetFlagStatus(USARTx,USART_FLAG_TC)!=SET);
}

void print_line(USART_TypeDef* USARTx, const char* data, int len)
{
	int t;

#if( INCLUDE_xTaskLogLevel == 1 )
	if( ucGetTaskLogLevel( NULL ) > 4 )
	{
		return;
	}
#endif

	/*for fix bug, t must be int type*/
	for( t = 0; t < len; t++ ) 
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

static int is_little_endian()
{
	int wTest = 0x12345678;
	short *pTest=(short*)&wTest;
	//printf("%s:%04x\r\n", __func__, pTest[0]);
	return !(0x1234 == pTest[0]);
}

/*�����ֽ��Ǵ��*/
uint32_t htonl(uint32_t hostlong)
{
	int value;
	char *p, *q;
	
	if(is_little_endian()) 
	{
		p = (char*)&value;
		q = (char*)&hostlong;
		*p = *(q+3);
		*(p+1) = *(q+2);
		*(p+2) = *(q+1);
		*(p+3) = *q;
	} 
	else 
	{
		value = hostlong;
	}
	
	//printf("value=%04x, hostlong=%04x\r\n", value, hostlong);
	return value;
}

uint32_t ntohl(uint32_t netlong)
{
	int value;
	char *p, *q;
	
	if(is_little_endian()) 
	{
		p = (char*)&value;
		q = (char*)&netlong;
		*p = *(q+3);
		*(p+1) = *(q+2);
		*(p+2) = *(q+1);
		*(p+3) = *q;
	} 
	else 
	{
		value = netlong;
	}
	
	//printf("value=%04x, netlong=%04x\r\n", value, netlong);
	return value;	
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

	while (su1 < end) 
	{
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

static int cmodule_tokenizer_init( RemoteTokenizer* t, const char* p, const char* end )
{
	int count = 0;

	// remove trailing newline
	if (end > p && end[-1] == '\n') 
	{
			end -= 1;
			if (end > p && end[-1] == '\r')
					end -= 1;
	}	

	while (p < end) 
	{
			const char*  q = p;

			q = memchr(p, ',', end-p);
			if (q == NULL)
					q = end;

			if (q >= p) 
			{
					if (count < MAX_TOKENS) 
					{
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

static Token cmodule_tokenizer_get( RemoteTokenizer* t, int index )
{
	Token  tok;
	static const char*  dummy = "";

	if (index < 0 || index >= t->count) 
	{
			tok.p = tok.end = dummy;
	} 
	else 
	{
			tok = t->tokens[index];
	}

	return tok;
}

/*******************************************
+MIPCALL:1,10.154.27.94		
********************************************/
void on_request_ip_success_callback(RemoteTokenizer *tzer, Token* tok)
{
	if( tok[1].end - tok[1].p < sizeof(dev->ip) )
	{
		memcpy(dev->ip, tok[1].p, tok[1].end - tok[1].p);
		dev->ip[tok[1].end - tok[1].p] = '\0';
	}
	printf("%s: dev->ip(%s)\r\n", __func__, dev->ip);		
	dev->ppp_status = PPP_CONNECTED;
}

void on_request_ip_fail_callback(RemoteTokenizer *tzer)
{
	printf("warnning->%s.\r\n", __func__);
	memcpy(dev->ip, "null", strlen("null"));
	dev->ppp_status = PPP_DISCONNECT;
	/*��ȡIPʧ�ܣ����»�ȡ*/		
}

static void parse_json(char *text)
{
	char /**out,*/ *name; 
	cJSON *json, *namejson=NULL, *formatjson=NULL,
		*typejson=NULL, *widthjson=NULL, *interlace=NULL;

/*
#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6	
*/	
	json=cJSON_Parse(text);
	if (!json) {printf("Error before: [%s]\r\n",cJSON_GetErrorPtr());}
	else
	{
		namejson = cJSON_GetObjectItem(json, "name");
		if(namejson) 
		{
			name = namejson->valuestring;
			printf("namejson: type=%d, name=%s\r\n", namejson->type, name);
		} 
		else 
		{
			printf("name error!\r\n");
			return;
		}
		
		formatjson = cJSON_GetObjectItem(json, "format");
		if(formatjson) 
		{
			widthjson = cJSON_GetObjectItem(formatjson, "width");
			if(widthjson) 
			{
				printf("widthjson: type=%d, name=%d\r\n", widthjson->type, widthjson->valueint);
			}
			
			typejson = cJSON_GetObjectItem(formatjson, "type");
			if(typejson) 
			{
				printf("typejson: type=%d, value=%s\r\n", typejson->type, typejson->valuestring);
			}

			interlace = cJSON_GetObjectItem(formatjson, "interlace");
			if(interlace)
			{
				printf("interlace: type=%d\r\n", interlace->type);
			}
		}
	//"{\"name\": \"Jack (\\\"Bee\\\") Nimble\",\"format\":{\"type\":\"rect\",
	//\"width\":1920,\"height\":1080,\"interlace\":false,\"frame rate\": 24}}";		
		
		//out=cJSON_Print(json);
		cJSON_Delete(json);
		//printf("%s\r\n",out);
		//free(out);
	}
}

static char *mqtt_get_name(int type)
{
	switch(type)
	{
		case MQTT_MSG_TYPE_PUBLISH:
			return "PUBLISH";
		case MQTT_MSG_TYPE_SUBSCRIBE:
			return "SUBSCRIBE";
		case MQTT_MSG_TYPE_PUBCOMP:
			return "PUBCOMP";
		case MQTT_MSG_TYPE_PUBREL:
			return "PUBREL";
		case MQTT_MSG_TYPE_PUBREC:
			return "PUBREC";	
		case MQTT_MSG_TYPE_PUBACK:
			return "PUBACK";	
		case MQTT_MSG_TYPE_PINGREQ:
			return "PINGREQ";
		case MQTT_MSG_TYPE_PINGRESP:
			return "PINGRESP";
		case MQTT_MSG_TYPE_CONNECT:
			return "CONNECT";
		case MQTT_MSG_TYPE_CONNACK:
			return "CONNACK";
		case MQTT_MSG_TYPE_SUBACK:
			return "SUBACK";
		case MQTT_MSG_TYPE_DISCONNECT:
			return "DISCONNECT";
		case MQTT_MSG_TYPE_UNSUBSCRIBE:
			return "UNSUBSCRIBE";
		case MQTT_MSG_TYPE_UNSUBACK:
			return "UNSUBACK";

		default: return "MSG UNKNOW";
	}
}

static char *at_get_name(int index)
{
	switch(index)
	{
		case ATCSQ:
			return "ATCSQ";
		case ATSIMTEST:
			return "ATSIMTEST";
		case ATCPMS:
			return "ATCPMS";
		case ATCMGD_:
			return "ATCMGD_";
		case ATMIPCALL_:
			return "ATMIPCALL_";	
		case ATMIPCALL0:
			return "ATMIPCALL0";	
		case ATMIPPROFILE:
			return "ATMIPPROFILE";
		case ATMIPCALL1:
			return "ATMIPCALL1";
		case ATMIPOPEN:
			return "ATMIPOPEN";
		case ATMIPSEND:
			return "ATMIPSEND";
		case ATMIPPUSH:
			return "ATMIPPUSH";
		case ATCMGF:
			return "ATCMGF";
		case ATMIPCLOSE:
			return "ATMIPCLOSE";
		case ATCMGR:
			return "ATCMGR";
		case ATCMGD:
			return "ATCMGD";
		case ATMIPHEX:
			return "ATMIPHEX";
		case ATRESET:
			return "ATRESET";
		
		default: return "MSG UNKNOW";
	}
}

static void complete_pending(mqtt_state_t* state, int event_type, uint16_t mesg_id)
{

}

static void mqtt_aes_buffer(int encode, char *buf, int buf_len)
{
	int i;
	uint8_t *p, *key = NULL,
  key_value_32[] = {
	0xa0, 0xfd, 0xe5, 0xf9,
	0x39, 0xad, 0x89, 0xc8,
	0x22, 0xf9, 0x8e, 0x70,
	0x74, 0x7d, 0x5f, 0xaf,
	0x6f, 0x35, 0xdc, 0xa7,
	0xfb, 0xb1, 0x54, 0xd7,
	0xaf, 0x34, 0x17, 0xa3,
	0xa9, 0x44, 0xff, 0xef };
	
	if( buf_len%AES_DECODE_LEN != 0) 
	{
		printf("buf_len M AES_DECODE_LEN != 0 ERROR!!!\r\n");
		return;
	}
	
	key = aes_create_key(key_value_32, sizeof(key_value_32)/sizeof(key_value_32[0]));
	if(!key) 
	{	
		printf("aes create key fail!\r\n");	
		return;
	}	
	
	for(i=0; i< buf_len/AES_DECODE_LEN; i++) 
	{
		if(encode)
			p = aes_encode_packet(key, (uint8_t*)(buf + AES_DECODE_LEN*i), AES_DECODE_LEN);
		else
			p = aes_decode_packet(key, (uint8_t*)(buf + AES_DECODE_LEN*i), AES_DECODE_LEN);
		
		if(p)
			memcpy(buf + AES_DECODE_LEN*i, p, AES_DECODE_LEN);
	}

	aes_destory_key(&key);		
}

static int mqtt_check_json_md5(mqtt_state_t* state, char *data, int len, char *md5)
{
	int i, result = 1;
	char calmd5[LEN_MD5];
	
	caculate_buffer_md5((uint8_t *)data, len, calmd5);
	
	/*decode buffer*/
	mqtt_aes_buffer(0, md5, LEN_MD5);
	
  for(i=0; i<LEN_MD5; i++) 
	{
		if(calmd5[i] != md5[i]) result = 0;
	}

	return result;
}

static int mqtt_publish(mqtt_state_t *state, const char* topic, char* data, int qos, int retain);

void mqtt_publish_test( int qos, char *topic, char *payload )
{
	mqtt_state_t* state = mqtt_dev->mqtt_state;
	
	printf("usmart: mqtt publish test. topic=%s, payload=%s, qos=%d\r\n", topic, payload, qos);
	
	if( qos > 2 || qos < 0 )
	{
		printf("%s: qos error!!!", __func__);
		return;
	}
	mqtt_publish(state, topic, state->jsonbuff, qos, 0);		
}

static int get_at_command_count(struct list_head *head);

static int deliver_publish(mqtt_state_t* state, uint8_t* message, int length)
{
	char topic[128];
  	mqtt_event_data_t edata;

	memset(topic, '\0', sizeof(topic));
	edata.type = MQTT_EVENT_TYPE_PUBLISH;
	edata.topic_length = length;
	edata.topic = mqtt_get_publish_topic(message, &edata.topic_length);

	edata.data_length = length;
	edata.data = mqtt_get_publish_data(message, &edata.data_length);
	
	if( memcmp(mqtt_dev->subscribe_topic, edata.topic, strlen(mqtt_dev->subscribe_topic)) || 
			edata.topic_length > sizeof(topic) || edata.data_length <= 0 || 
				(edata.data_length + edata.topic_length) > (1500-6)/*fix head+topic.len+msgid*/)
	{	
		printf("%s: DATA ERROR! subscribe.topic=%s, edata.topic=%s, topic.length=%d, payload.length=%d\r\n",
							__func__, mqtt_dev->subscribe_topic, edata.topic, edata.topic_length, edata.data_length);
		return -1;
	}

	memcpy(topic, edata.topic, edata.topic_length);
	topic[edata.topic_length] = '\0';

	printf("topic.length = %d, topic=%s\r\n", edata.topic_length, topic);
	printf("payload.length = %d, payload=%s\r\n", edata.data_length, edata.data);			

	//APP_4G_OnControlMsg(payload);
	return 1;
}

/* Publish the specified message */
int mqtt_publish_with_length( mqtt_state_t *state, const char* topic, const char* data, int data_length, int qos, int retain )
{
	state->outbound_message = mqtt_msg_publish( &state->mqtt_connection, 
	                                             topic, data, data_length, 
	                                             qos, retain,
	                                             &state->pending_msg_id );
	state->mqtt_flags &= ~MQTT_FLAG_READY;
	state->pending_msg_type = MQTT_MSG_TYPE_PUBLISH;

	make_command_to_list( ATMQTT, ( data_length / 300 + 1 ) * ONE_SECOND / 25, MQTT_MSG_TYPE_PUBLISH );
	return 0;
}

/*retain ��־ֻ����publish ��Ϣ����Ϊ1ʱ, ע��data�ĳ��ȣ�������700*/
static int mqtt_publish(mqtt_state_t *state, const char* topic, char* data, int qos, int retain)
{
	int len = strlen(data);

	return mqtt_publish_with_length( state, topic, data, data != NULL ? len: 0, qos, retain );
}

static void mqtt_subscribe(mqtt_state_t *state)
{
	state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
	make_command_to_list(ATMQTT, ONE_SECOND/5, MQTT_MSG_TYPE_SUBSCRIBE);
	make_command_to_list(ATMIPPUSH, ONE_SECOND/40, MQTT_MSG_TYPE_NULL);	
}

//extern void report_mcu_status(void);

static void mqtt_reset_status(void)
{	
	printf("%s: uartlost=%d, uart3fifo->lostBytes=%d\r\n", 
				__func__, uartlost, uart3fifo->lostBytes);
	//report_mcu_status();
	mqtt_dev->parse_packet_flag = 0;
	/* abandon the recevied buffer */
	
	mqtt_dev->reset_count++;
	mqtt_dev->in_pos = 0;
	mqtt_dev->in_waitting = 0;	
	mqtt_dev->fixhead = 1;		
	
	dev->close_tcp_interval = ONE_SECOND / 2;
	dev->tick_sum = 0;
	dev->tick_tag = NOW_TICK;
	/* close tcp 0.5 second later */
}

static int check_packet_legal(mqtt_state_t *state, int protocol_len)
{
	if(state->message_length != protocol_len)
	{
	  uartlost = uart3fifo->lostBytes - uartlost;		
		printf("%s: illegal mqtt len. mqtt reset!\r\n", __func__);
		mqtt_reset_status();
		return 1;
	}
	else
	{
		return 0;
	}
}

static void parse_mqtt_packet(mqtt_state_t *state, int nbytes)
{
	uint8_t msg_type;
	uint8_t msg_qos;
	uint16_t msg_id;	

	state->in_buffer_length = nbytes;
	state->message_length_read = nbytes;
	state->message_length = mqtt_get_total_length(state->in_buffer, state->message_length_read);

	msg_type = mqtt_get_type(state->in_buffer);
	msg_qos  = mqtt_get_qos(state->in_buffer);
	msg_id	 = mqtt_get_id(state->in_buffer, state->in_buffer_length);
	
	printf("received [%s]! msg_type=%d, QOS=%d, msg_id=0x%02X, mesg_len=%d, nbytes=%d\r\n", 
						mqtt_get_name((int)msg_type), msg_type, 
						msg_qos, msg_id, state->message_length, nbytes);

	switch(msg_type)
	{
		case MQTT_MSG_TYPE_CONNACK:
			/* 4 bytes */		
		  if(check_packet_legal(state, 4)) return;
			
			if(state->in_buffer[3] == 0x00) 
			{
				mqtt_dev->connect_status = MQTT_DEV_STATUS_CONNECT;
				mqtt_set_mesg_ack(MQTT_MSG_TYPE_CONNECT, 0);
				mqtt_subscribe(state);
			} 
			else 
			{
				mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
			}
			printf("MQTT_MSG_TYPE_CONNACK, ack=%02x, status=%d\r\n", state->in_buffer[nbytes-1],
				 mqtt_dev->connect_status);
			
			break;
			
		 /*����SUBACK��msg_id����Ƚ��Ƿ������ȥ��MQTT_MSG_TYPE_SUBSCRIBE msg_idһ�£�*/
		case MQTT_MSG_TYPE_SUBACK:
			/* 5 bytes */
		  if(check_packet_legal(state, 5)) return;
			
			if(state->pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_SUBSCRIBED, msg_id);
			
			if(state->in_buffer[4] == 0x00 || state->in_buffer[4] == 0x01 ||
					state->in_buffer[4] == 0x02)
			{
				printf("%s: subscribe success.\r\n", __func__);
				mqtt_set_mesg_ack(MQTT_MSG_TYPE_SUBSCRIBE, msg_id);
				
				mqtt_publish(state, "/xp/publish", "MCU be ready to recv command.", 1, 0);
			}

			if(state->in_buffer[4] == 0x80)
			{
				printf("%s: subscribe fail! state->in_buffer[4]=0x%02x\r\n", __func__, state->in_buffer[4]);
			}
			
			break;
			
		case MQTT_MSG_TYPE_UNSUBACK:
			/* 4 bytes */
		  if(check_packet_legal(state, 4)) return;
			
			if(state->pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_UNSUBSCRIBED, msg_id);
			
			break;
		
		/*�����յ���PUBLISH��Ϣ���������msg_id��ֵ��ֻ��ش����������Ϳ���*/
		case MQTT_MSG_TYPE_PUBLISH:
			//msg_type=3, msg_qos=1, msg_id=0x19   ������Ϣʱ���û��ָ��qos=0��qos=1������
			//msg_type=3, msg_qos=0, msg_id=0x00    ������Ϣ��public ��ָ����qos=1
			mqtt_dev->pub_in_num++ ;
		
			if(msg_qos == 1) 
			{
				state->outbound_message = mqtt_msg_puback(&state->mqtt_connection, msg_id);
				make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBACK);		
			} 
			else if(msg_qos == 2) 
			{
				state->outbound_message = mqtt_msg_pubrec(&state->mqtt_connection, msg_id);
				/* recv process: */
				make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREC);
			}
			
			if(-1 == deliver_publish(state, state->in_buffer, state->message_length_read))
			{
				printf("%s: deliver_publish error! mqtt reset!\r\n", __func__);
				mqtt_reset_status();
				return;
			}
				
			break;
			
		/*���յ�MQTT_MSG_TYPE_PUBACKʱ��msg_id������send MQTT_MSG_TYPE_PUBLISH��Ϣ
			ʱ��msg_id���бȽϣ�*/	
		case MQTT_MSG_TYPE_PUBACK:
			/* 4 bytes */
		  if(check_packet_legal(state, 4)) return;
			
			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBLISH, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_PUBREC:
			/*send process: publish received = 4 bytes*/
		  if(check_packet_legal(state, 4)) return;
		
			state->outbound_message = mqtt_msg_pubrel(&state->mqtt_connection, msg_id);
		  mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBLISH, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREL);

			break;
		
		case MQTT_MSG_TYPE_PUBREL:
			/*recv process: publish release = 4 bytes*/
		  if(check_packet_legal(state, 4)) return;
		
			state->outbound_message = mqtt_msg_pubcomp(&state->mqtt_connection, msg_id);
		  mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBREC, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBCOMP);

			break;
		
		case MQTT_MSG_TYPE_PUBCOMP:
			/*send process: publish completed = 4 bytes*/
		  if(check_packet_legal(state, 4)) return;
		
			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBREL, msg_id);
			break;
			
		case MQTT_MSG_TYPE_PINGREQ:
			/*2 bytes*/
		  if(check_packet_legal(state, 2)) return;
		
			make_command_to_list(ATMQTT, ONE_SECOND/45, MQTT_MSG_TYPE_PINGRESP);
			make_command_to_list(ATMIPPUSH, ONE_SECOND/50, MQTT_MSG_TYPE_NULL);	
		
			break;
		
		case MQTT_MSG_TYPE_PINGRESP:
			/* 2 bytes */
		  if(check_packet_legal(state, 2)) return;
		
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PINGREQ, 0);
			
			break;
		
		case MQTT_DEV_STATUS_DISCONNECT:
			/* 2 bytes */
		  if(check_packet_legal(state, 2)) return;
		
			mqtt_dev->connect_status = MQTT_DEV_STATUS_DISCONNECT;
		
			break;
			
		default: 
				printf("%s: packet error! mqtt reset!\r\n", __func__);
				mqtt_reset_status();
			break;
	}
		
}

static uint8_t str_to_hex(char *src)
{
	uint8_t s1,s2;

	s1 = toupper(src[0]) - 0x30;
	if (s1 > 9) s1 -= 7;

	s2 = toupper(src[1]) - 0x30;
	if (s2 > 9) s2 -= 7;

	return (s1*16 + s2);
}

/*���Ϊmqtt��Ϣͷ���˺���������*/
void check_packet_from_fixhead(char *fixhead, uint8_t *read, int nbytes)
{
	int msg_len;
	
	msg_len = mqtt_get_total_length(read, nbytes);

	/* �����������������ݹ飬 �����ݹ飡*/
	if(!mqtt_dev->parse_packet_flag)
	{
		printf("%s: waitting mqtt tcp connect server!\r\n", __func__);
		return;
	}
	
  if(msg_len == nbytes)//most 750 = 750 
	{
		printf("%s: msg_len == nbytes = %d\r\n", __func__, nbytes);
		mqtt_dev->in_pos = 0;
		mqtt_dev->in_waitting = 0;		
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
		memcpy(mqtt_dev->in_buffer, read, msg_len);
		parse_mqtt_packet(mqtt_dev->mqtt_state, nbytes);
	}
	else if(msg_len < nbytes)//150 < 200
	{
		printf("%s: [%d] msg_len < nbytes [%d]\r\n", __func__, msg_len, nbytes);		
		mqtt_dev->in_waitting = 0;	
		mqtt_dev->in_pos = 0;		
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
		memcpy(mqtt_dev->in_buffer, read, msg_len);
		parse_mqtt_packet(mqtt_dev->mqtt_state, msg_len);
		memmove(read, read+msg_len, nbytes-msg_len);
		*fixhead = 1;		
		check_packet_from_fixhead(fixhead, read, nbytes-msg_len);
	}
	else if(msg_len <= sizeof(mqtt_dev->in_buffer))//msg_len > nbytes  1500 > 1300 > 750
	{
		printf("%s: [%d]nbytes < [%d] msg_len <= sizeof(mqtt_dev->in_buffer)[%d]\r\n", 
							__func__, nbytes, msg_len, sizeof(mqtt_dev->in_buffer));			
		*fixhead = 0;
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
		memcpy(mqtt_dev->in_buffer, read, nbytes);
		mqtt_dev->in_pos = nbytes;
		mqtt_dev->in_waitting = msg_len-nbytes;// 0<in_waitting<=750
	}
	else if(msg_len > sizeof(mqtt_dev->in_buffer))//1502
	{
		printf("%s: [%d] msg_len > sizeof(mqtt_dev->in_buffer)[%d]\r\n", 
							__func__, msg_len, sizeof(mqtt_dev->in_buffer));		
		*fixhead = 0;
		mqtt_dev->in_pos = 0;
		mqtt_dev->in_waitting = -(msg_len-nbytes);//-752 0 > in_waitting
	}
}

/* TCP ��Ϣ������ */
void on_tcp_data_callback(RemoteTokenizer *tzer, Token* tok)
{
	int i, nbytes;

	memset(tcp_read, '\0', sizeof(tcp_read));
	nbytes = str2int(tok[1].p, tok[1].end);//nbytes <= 750

	mqtt_dev->recv_bytes += nbytes;
		/* ������������� ������������ */
	if(!mqtt_dev->parse_packet_flag)
	{
		printf("%s: drop %d bytes. waitting mqtt reset finish...\r\n", __func__, nbytes);
		return;
	}
	
	for(i=0; i<nbytes; i++)
	{
		tcp_read[i] = str_to_hex((char*)tok[2].p+2*i);
	}	
	
	if(mqtt_dev->fixhead)
	{
		check_packet_from_fixhead(&(mqtt_dev->fixhead), tcp_read, nbytes);
	}
	else 
	{
		if(mqtt_dev->in_waitting > 0 && mqtt_dev->in_waitting <= 750)
		{
			if(mqtt_dev->in_waitting == nbytes)
			{
				printf("%s: in_waitting == nbytes = %d\r\n", __func__, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, tcp_read, nbytes);
				parse_mqtt_packet(mqtt_dev->mqtt_state, mqtt_dev->in_pos+mqtt_dev->in_waitting);
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;
			}
			else if(mqtt_dev->in_waitting < nbytes)
			{
				printf("%s: [%d] in_waitting < nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, tcp_read, mqtt_dev->in_waitting);
				parse_mqtt_packet(mqtt_dev->mqtt_state, mqtt_dev->in_pos+mqtt_dev->in_waitting);
				memmove(tcp_read, tcp_read+mqtt_dev->in_waitting, nbytes-mqtt_dev->in_waitting);
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;				
				check_packet_from_fixhead(&(mqtt_dev->fixhead), tcp_read, nbytes-mqtt_dev->in_waitting);
			}
			else if(mqtt_dev->in_waitting > nbytes)
			{
				printf("%s: [%d] in_waitting > nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, tcp_read, nbytes);
				mqtt_dev->in_pos += nbytes;
				mqtt_dev->in_waitting -= nbytes;
			}				
		}
		else if(mqtt_dev->in_waitting < 0)//-752
		{
			mqtt_dev->in_waitting += nbytes;
			
			if(mqtt_dev->in_waitting < 0) 
			{
				printf("%s: Drop packet! in_waitting = %d\r\n", __func__, mqtt_dev->in_waitting);
			}
			else if(mqtt_dev->in_waitting == 0)
			{
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_pos = 0;
				printf("%s: Drop packet! in_waitting = %d\r\n", __func__, mqtt_dev->in_waitting);
			}
			else if(mqtt_dev->in_waitting > 0)//in_waitting=-4 + nbytes=20 = in_waitting=16
			{
				int len;
				
				len = mqtt_dev->in_waitting;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;
				mqtt_dev->fixhead = 1;
				memmove(tcp_read, tcp_read+(nbytes-len), len);
				printf("%s: Drop some char in packet! len = %d, nbytes=%d\r\n", __func__, len, nbytes);
				check_packet_from_fixhead(&(mqtt_dev->fixhead), tcp_read, mqtt_dev->in_waitting);
			}
		}
		else if(mqtt_dev->in_waitting > 750)
		{
			printf("%s: mqtt_dev->in_waitting=%d ERROR!\r\n", __func__, mqtt_dev->in_waitting);
			mqtt_reset_status();
		}
	}
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
	dev->tcp_connect = 1;
	printf("%s:TCP connect success! socket id=%d\r\n", __func__, socketId);
	atcmd_set_ack(ATMIPOPEN);
	mqtt_dev->parse_packet_flag = 1;
}

void on_connect_service_fail_callback(RemoteTokenizer *tzer)
{
	//+MIP:ERROR
	printf("[%s, TCP connect error.]\r\n", __func__);
	dev->tcp_connect = 0;
	atcmd_set_ack(ATMIPOPEN);
}

static void mqtt_msg_set_clean(void);

void on_disconnect_service_callback(RemoteTokenizer *tzer, Token* tok)
{
	//+MIPCLOSE:2
	int socketId = str2int(tok[0].p+10, tok[0].end);
	
	printf("%s: +MIPCLOSE !!!!! success. socket id = %d\r\n", __func__, socketId);
	dev->socket_open[socketId-1] = -1;
	mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
	mqtt_dev->in_pos = 0;
	mqtt_dev->in_waitting = 0;	
	mqtt_dev->fixhead = 1;
	
	dev->tcp_connect = 0;
	mqtt_msg_set_clean();
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
	
	if( tzer->count == 1 ) 
	{
		memcpy(dev->at_sending, tok[0].p, tok[0].end - tok[0].p);
	} 
	else 
	{
	  strcat(dev->at_sending, tok[0].p);
	}
	
	//printf("at_sending=%s, len=%d\r\n", at_sending, strlen(at_sending));
}

void on_at_cmd_success_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) 
	{
		//printf("----------------------------------\r\n");
		//printf("[result: OK] %s\r\n", dev[0].at_sending);
		
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) 
	{
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		/*�������Ƕ��Ų�ѯ��������*/
		printf("[result: OK] %s\r\n", dev->at_sending);		

		/*ɾ�����ųɹ�֮����ܶ�ȡ��һ�����ţ�������ɹ�
		��������Զ�������ٶ�ȡ����at fail��Ҳ����fix it.*/
		if(dev->sm_num > 0) 
		{
			int i, delflag = 0;
			for(i=0; i< dev->sm_num; i++) 
			{
				if(dev->sm_index[i] == dev->sm_index_delete) 
				{
					delflag = 1;
					break;
				}
			}
			/*�����������ɾ�������кţ��Ž���ɾ����*/
			if(delflag) 
			{
				for(; i<dev->sm_num-1; i++) 
				{
					dev->sm_index[i] = dev->sm_index[i+1];
				}
				dev->sm_index[i] = -1;
				dev->sm_num--;
			}
			
			printf("UPDATE sm_num=%d, sm_index={ ", dev->sm_num);
			for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++) 
			{
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
	else if( !memcmp(dev->at_sending, "AT+MIPHEX=1", strlen("AT+MIPHEX=1")))
	{
		printf("%s: success to cmd AT+MIPHEX=1\r\n", __func__);
		atcmd_set_ack( ATMIPHEX );
	}
}

void on_at_cmd_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) 
	{
		dev->socket_close_flag = 1;
		printf("[%s: result: ERROR AT:%s\r\n", __func__, dev->at_sending);
	} 
	else if(!memcmp(dev->at_sending, "AT+MIPOPEN=1,0,\"", strlen("AT+MIPOPEN=1,0,\""))) 
	{
		//AT+MIPOPEN=1,0,"
		printf("[TCP CONNECT FAIL! AT: %s", dev->at_sending);
		dev->socket_open[0] = 1;
		dev->socket_open[1] = 2;
		dev->socket_open[2] = 3;
		dev->socket_open[3] = 4;
		dev->tcp_connect = 0;
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) 
	{
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		printf("[AT:%s, result: ERROR]\r\n", dev->at_sending);
		dev->sm_delete_flag = 1;
		/*����ִ��ɾ�����Ź���*/
		dev->sm_read_flag = 1;
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGR=", strlen("AT+CMGR=")) ) 
	{
		/*��ʧ�ܣ����¶�*/
		dev->sm_read_flag = 1;
	}
	else if( !memcmp(dev->at_sending, "AT+MIPCLOSE=", strlen("AT+MIPCLOSE=")) )
	{
		int socketId = str2int(dev->at_sending+12, dev->at_sending+13);
		
		printf("%s: close socketid[%d] error! AT:%s\r\n", __func__, socketId, dev->at_sending);
		
		dev->socket_open[socketId-1] = -1;
	}
	else if( !memcmp(dev->at_sending, "AT+MIPHEX=", strlen("AT+MIPHEX=")))
	{
		printf("%s: fail to cmd AT+MIPHEX\r\n", __func__);
	}
}

void on_simcard_type_callback(RemoteTokenizer *tzer, Token* tok)
{
	/*+SIMTEST:3; 3
	AT+SIMTEST?
	+SIMTEST:0
	+SIMTEST:0*/
	
	dev->simcard_type = str2int(tok[0].p+9, tok[0].p+10);
	dev->boot_status = 1;
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	//print_line_1(UART4, tok[0].p, tok[0].end - tok[0].p);
	printf("%s: simcard_type(%d)\r\n", __func__, dev->simcard_type);
}

void on_sm_check_callback(RemoteTokenizer *tzer, Token* tok)
{
	int i;
	
	if(tzer->count == 2) 
	{
		/*+CMGD: (),(0-4)*/
		/*+CMGD: (0),(0-4)*/
		if(!memcmp(tok[0].p, "+CMGD: ()", strlen("+CMGD: ()"))) 
		{
			i = 0;
			dev->sm_num = 0;
		} 
		else 
		{
			i = 1;
			dev->sm_num = 1;
			dev->sm_index[0] = str2int(tok[0].p+8, tok[0].end-1);
		}
		for(; i < SIZE_ARRAY( dev->sm_index ); i++) 
		{
			dev->sm_index[i] = -1;
		}
	} 
	else if(tzer->count > 2) 
	{
		/*+CMGD: (0,1,2,4,5,6),(0-4)*/
		dev->sm_num = tzer->count - 1;
		if(dev->sm_num > sizeof(dev->sm_index)/sizeof(dev->sm_index[0])) 
		{
			dev->sm_num = sizeof(dev->sm_index)/sizeof(dev->sm_index[0]);
		}
		
		for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++) 
		{
			if( i < dev->sm_num ) 
			{
				if( i==0 ) 
					dev->sm_index[i] = str2int(tok[i].p+8, tok[i].end);
				else if( i== (dev->sm_num -1) )
					dev->sm_index[i] = str2int(tok[i].p, tok[i].end-1);
				else 
					dev->sm_index[i] = str2int(tok[i].p, tok[i].end);
			} 
			else 
			{
				dev->sm_index[i] = -1;
			}
		}
	}
	
	printf("sm_num=%d, sm_index={ ", dev->sm_num);
	for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++ ) 
	{
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
	
	if(index != -1) 
	{
		if(dev->sm_num >= sizeof(dev->sm_index)/sizeof(dev->sm_index[0]))
			return;

		for(i=0; i<dev->sm_num; i++) 
		{
			if(index == dev->sm_index[i]) 
				addflag = 0;
		}
		
		if(addflag) 
		{
			dev->sm_index[dev->sm_num] = index;
			dev->sm_num++;
			select_sort(dev->sm_index, dev->sm_num);
			/*��������*/
		}
		
		printf("sm_num=%d, sm_index={ ", dev->sm_num);
		for(i=0; i < SIZE_ARRAY( dev->sm_index ); i++) 
		{
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
	Token tok[35];
	RemoteTokenizer tzer[1];
	
	//printf("r->pos=%d\r\n", r->pos);	
	print_line( UART4, r->in, r->pos );//jzyang
	/*��ӡ4G�����д�����Ϣ, release �汾ʱȥ����*/
	
	cmodule_tokenizer_init( tzer, r->in, r->in + r->pos );
	
	if( tzer->count == 0 ) 
		return;
	if( tzer->count > SIZE_ARRAY( tok ) )
	{
		printf("%s: tzer->count(%d) error!\r\n", __func__, tzer->count);
		return;
	}
	
	for( i = 0; i < tzer->count; i++ ) 
	{
		tok[i] = cmodule_tokenizer_get(tzer, i);
	}
	
	if( dev->sm_data_flag ) 
	{
		dev->sm_data_flag = 0;
		r->on_sm_data(tzer, tok, dev->sm_index_read);
	}
	
	if( !memcmp( tok[0].p, "AT+", strlen("AT+") ) ) {
		r->on_at_command( tzer, tok );
		
	} else if(!memcmp(tok[0].p, "OK", strlen("OK"))) {
		r->on_at_success(tzer);
		
	} else if(!memcmp(tok[0].p, "ERROR", strlen("ERROR"))) {
		r->on_at_fail(tzer);
		
	} else if(!memcmp(tok[0].p, "+MIPRTCP=", strlen("+MIPRTCP="))&&(tzer->count>2)) {
		r->on_tcp_data(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "+MIPCALL:0", strlen("+MIPCALL:0"))) {
		r->on_ip_fail(tzer);
		
	} else if(!memcmp(tok[0].p, "+MIPCALL:1", strlen("+MIPCALL:1"))&&(tzer->count>1)) {
		r->on_ip_success(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "+CSQ:", strlen("+CSQ:"))&&(tzer->count>1)) {
		r->on_signal_strength(tzer, tok);
		
	} else if(!memcmp(tok[0].p, "+MIP:ERROR", strlen("+MIP:ERROR"))) {
		r->on_connect_fail(tzer);
		
	} else if(!memcmp(tok[0].p, "+MIPOPEN=", strlen("+MIPOPEN="))&&(tzer->count>1)) {
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
}

static void cmodule_reader_addc( UartReader* r, int c )
{
	if ( r->overflow ) 
	{
		r->overflow = ( c != '\n' );
		return;
	}

	if ( r->pos >= (int) sizeof(r->in) ) 
	{
		r->overflow = 1;
		r->pos = 0;
		printf("%s: OVER FLOW!!!!!!!\r\n", __func__);
		return;
	}	

	/*����Ҫ����1550����*/
	r->in[r->pos] = (char)c;
	r->pos += 1;

	if( c == '\n' /*|| c == '\0'*/ ) 
	{
		longsung_reader_parse( r );
		r->pos = 0;
		/*for fix bug. must memset r->in*/
		memset( r->in, '\0', sizeof(r->in) );
	}
}

void handle_cmodule_uart_msg( void )
{
	unsigned char ch;

	while( rfifo_len( uart3fifo ) > 0 ) 
	{
		rfifo_get( uart3fifo, &ch, 1 );
		cmodule_reader_addc( reader, ch );
	}	
}

/*need to invoved in every 50s.*/
extern void Printf_System_Jiffies( void );

void notify_longsung_period( TimerHandle_t xTimer )
{	
	DevStatus * dev = ( DevStatus * ) pvTimerGetTimerID( xTimer );	
	Printf_System_Jiffies();
	dev->period_tick = 1;
}

void test_mqtt_publish( void *argc )
{	
	static int i = 0;
	char *str = ( char* ) argc;

	mqtt_dev_status *mqtt = dev->mqtt_dev;
	
	if( strlen( str ) == 0 && mqtt->connect_status == MQTT_DEV_STATUS_CONNECT )
	{
		memset(mqtt->mqtt_state->jsonbuff, '\0', sizeof(mqtt->mqtt_state->jsonbuff));
		sprintf(mqtt->mqtt_state->jsonbuff, "IN360s: time:[%llds]. mqtt_bytes=%lld, lostbytes=%d, MQTT:reset_count=%d, in_publish=%d, mq_head=%d,fixhead=%d,in_waitting=%d,in_pos=%d\r\n\
		4G: malloc=%d, free=%d, simcard=%d,reset=%d,ppp_status=%d,socket_num=%d,sm_num=%d,scsq=%d,rcsq=%d,at_count=%d, at_head=%d, atcmd_head=%d", 
					dev->sys_time, mqtt->recv_bytes, uart3fifo->lostBytes, mqtt->reset_count, mqtt->pub_in_num, get_at_command_count(&dev->mqtt_head), mqtt->fixhead, 
					mqtt->in_waitting, mqtt->in_pos, dev->malloc_count, dev->free_count, dev->simcard_type, 
					dev->reset_request, dev->ppp_status, dev->socket_num, dev->sm_num, dev->scsq, dev->rcsq, dev->at_count, get_at_command_count(&dev->at_head),  
					get_at_command_count(&dev->atcmd_head));		
		
		mqtt_publish_test( i, "/xp/publish", mqtt->mqtt_state->jsonbuff );
		if( ++i > 2 ) i = 0;
	}
	else if( strlen( str ) > 1 && mqtt->connect_status == MQTT_DEV_STATUS_CONNECT )
	{
		while( *str == ' ' ) str++;
		
		if( strlen( str ) > 0 )
		{
			memset(mqtt->mqtt_state->jsonbuff, '\0', sizeof(mqtt->mqtt_state->jsonbuff));
			memcpy( mqtt->mqtt_state->jsonbuff, str , strlen( str ) );		
			mqtt_publish_test( i, "/xp/publish", mqtt->mqtt_state->jsonbuff );	
		}
	}

}

static void init_cmodule_reader(void) 
{
	reader->inited = 1;
	reader->pos = 0;
	reader->overflow = 0;
	
	reader->on_simcard_type = on_simcard_type_callback;	
	reader->on_signal_strength	= on_signal_strength_callback;
	
	reader->on_tcp_data = on_tcp_data_callback;
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

static void init_mqtt_dev(mqtt_dev_status* dev)
{
	int i;

	printf("%s\r\n", __func__);
	memset(dev, '\0', sizeof(mqtt_dev_status));
	
	dev->connect_info.client_id = "jzyang";
	dev->connect_info.username = NULL;//"yang";//NULL 
	dev->connect_info.password = NULL;//"zhou";// NULL
	dev->connect_info.will_topic = "mcu";
	dev->connect_info.will_message = "death";
	dev->connect_info.keepalive = 450;
	dev->connect_info.will_qos = 1;
	dev->connect_info.will_retain = 0;
	dev->connect_info.clean_session = 1;

	/* Initialise the MQTT client*/
	mqtt_init(dev->mqtt_state, mqtt_dev->in_buffer, sizeof(mqtt_dev->in_buffer),
						mqtt_dev->out_buffer, sizeof(mqtt_dev->out_buffer));	
	
	dev->mqtt_state->connect_info = &(dev->connect_info);
	/* Initialise and send CONNECT message */
	mqtt_msg_init(&(dev->mqtt_state->mqtt_connection), dev->mqtt_state->out_buffer,
			dev->mqtt_state->out_buffer_length);
	
	dev->connect_status = MQTT_DEV_STATUS_NULL;
	
	memset(mqtt_dev->subscribe_topic, '\0', sizeof(mqtt_dev->subscribe_topic));
	memcpy(mqtt_dev->subscribe_topic, "/system/sensor", strlen("/system/sensor"));
	mqtt_dev->pub_in_num = 0;
	mqtt_dev->pub_out_num = 0;
	mqtt_dev->in_pos = 0;	
	mqtt_dev->in_waitting = 0;	
	mqtt_dev->fixhead = 1;
	mqtt_dev->parse_packet_flag = 1;
 	mqtt_dev->iot = 0;
	for(i=0; i<OUT_DATA_LEN_MAX; i++) 
	{
		dev->mqtt_state->outdata[i] = NULL;
	}
}

static void init_cmodule_status(char flag)
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
	dev->atcmd = NULL;
	dev->close_tcp_interval = 0;
	dev->clean_interval = 3 * ONE_SECOND / 2;
	memcpy( dev->ip, "null", strlen("null") );
	
	if( flag ) 
	{
		dev->reset_request = 1;
		dev->is_inited = 1;
	} 
	else 
	{
		dev->reset_request = 0;
		dev->is_inited = 0;
	}
	
	for( i = 0; i < SIZE_ARRAY(dev->socket_open); i++ ) 
	{
		dev->socket_open[i] = -1;
	}
	
	for( i = 0; i < SIZE_ARRAY(dev->sm_index); i++ ) 
	{
		dev->sm_index[i] = -1;
	}
}

static void do_read_sm(int index)
{
	char cmd[15];

	dev->sm_index_read = index;
	
	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "AT+CMGR=%d", index);
	
	at(cmd);
}

static void do_delete_sm(int index)
{
	char cmd[15];
	
	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "AT+CMGD=%d", index);
	
	at(cmd);
}

static void do_close_socket(int index)
{
	char cmd[15];
	
	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "AT+MIPCLOSE=%d", index);
	
	at( cmd );
}

/***************************************************************
�����ݼ��뵽������ȥ
****************************************************************/
static void add_cmd_to_list(AtCommand *cmd, struct list_head *head)
{
	if(cmd)
		list_add_tail(&cmd->list, head);
}

/***************************************************************
��������ݿ������������ҵ�����������ɾ���������
****************************************************************/
static void del_cmd_from_list( AtCommand *cmd, struct list_head *head )
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, head) 
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
static AtCommand* get_cmd_from_list(struct list_head *head)
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, head)
	{
		command = (AtCommand *)list_entry(pos, AtCommand, list);
		break;
	}	

	return command;
}

static int check_command_exist(int index, struct list_head *head)
{
	int ret = 0;
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, head)
	{
		command = (AtCommand *)list_entry(pos, AtCommand, list);
		if(index == command->index) 
		{
			ret = 1;
			break;
		}
	}	

	return ret;	
}


static int get_at_command_count(struct list_head *head)
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, head)
	{
		count++;	
	}
	
	return count;
}

static char* make_mqtt_packet( char* buff, unsigned char* data, int len )
{
	char *p;	
	int i;

	if( !data || !buff ) 
		return NULL;
	
	strcpy(buff, "AT+MIPSEND=1,\"");
	p = buff+strlen("AT+MIPSEND=1,\"");
	
	for( i = 0; i < len; i++ ) 
	{
		sprintf( p + 2 * i, "%02x", *( data+i ) );
	}
	p[ 2 * i ] = '\"';

	//printf("strlen(buff)=%d\r\n", strlen(buff));
	//printf("----------------------------------------------\r\n");
	return buff;
}

static void mqtt_disconnect_server(mqtt_state_t *state)
{
	char buff[128];

	printf("%s.\r\n", __func__);
	memset(buff, '\0', sizeof(buff));
	state->outbound_message = mqtt_msg_disconnect(&state->mqtt_connection);
	
	if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
	{
		vTaskDelay( 15 / portTICK_RATE_MS );
		at(buff);
		vTaskDelay( 15 / portTICK_RATE_MS );
		at("AT+MIPPUSH=1");
		vTaskDelay( 100 / portTICK_RATE_MS );
	}		
}

static void prepare_mqtt_packet( mqtt_state_t *state, AtCommand* cmd )
{
	char buff[512];
	
	memset( buff, '\0', sizeof( buff ) );
	
	switch( cmd->para ) 
	{
		case MQTT_MSG_TYPE_PINGRESP:
			state->outbound_message = mqtt_msg_pingresp( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_PINGRESP;
			if( make_mqtt_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				at( buff );
			}		
		  break;
		
		case MQTT_MSG_TYPE_CONNECT:
			state->outbound_message =  mqtt_msg_connect( &state->mqtt_connection, state->connect_info );
			cmd->mqtype = MQTT_MSG_TYPE_CONNECT;
			cmd->msgid = 0;
			if( make_mqtt_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				at( buff );
			}
			break;
		
		case MQTT_MSG_TYPE_DISCONNECT:
			state->outbound_message =  mqtt_msg_disconnect( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_DISCONNECT;
			if( make_mqtt_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				at( buff );
			}		
			break;
			
		case MQTT_MSG_TYPE_PINGREQ: 
			state->outbound_message = mqtt_msg_pingreq( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_PINGREQ;
			cmd->msgid = 0;
			if( make_mqtt_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				at( buff );
			}		
			break;
		
		case MQTT_MSG_TYPE_SUBSCRIBE:
		  state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
			state->outbound_message = mqtt_msg_subscribe( &state->mqtt_connection, 
                                                   mqtt_dev->subscribe_topic, 2, //�˴���QOS��ϵ�յ�PUBLISH��Ϣ��qos
                                                   &state->pending_msg_id );
			cmd->msgid = state->pending_msg_id;		
			cmd->mqtype = MQTT_MSG_TYPE_SUBSCRIBE;

			if( make_mqtt_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				at( buff );
			}		
			break;
	
		/*��Ҫ��¼msg_id����Ϣ*/
		case MQTT_MSG_TYPE_PUBLISH:
		case MQTT_MSG_TYPE_PUBCOMP:
		case MQTT_MSG_TYPE_PUBREL:
		case MQTT_MSG_TYPE_PUBREC:
		case MQTT_MSG_TYPE_PUBACK://40head 02len 0017msgid			
			if( cmd->mqttdata ) 
			{
				memset( mqtt_dev->out_buffer, '\0', sizeof( mqtt_dev->out_buffer ) );
				if( make_mqtt_packet( (char *) mqtt_dev->out_buffer, cmd->mqttdata, cmd->mqttdata_len ) )
				{
					at( ( char * )mqtt_dev->out_buffer );
				}
			} 
			else 
			{
				printf("ERROR: %s mqttdata=NULL!\r\n", __func__);
			}				
			break;
			
		default: 
			break;
	}
	
	printf("sending [%s]! mqtype=%d,id=0x%04x\r\n", mqtt_get_name( cmd->mqtype ), cmd->mqtype, cmd->msgid );	
}

static void send_command_to_device( AtCommand* cmd )
{
	if( cmd == NULL ) 
		return;
	
	//printf("%s: index=%d\r\n", __func__, cmd->index);
	/*
	case ATMIPOPEN: at("AT+MIPOPEN=1,0,\"198.41.30.241\",1883,0"); break;
	case ATMIPOPEN: at("AT+MIPOPEN=1,0,\"iot.eclipse.org\",1883,0"); break;
	at("AT+MIPOPEN=1,0,\"112.124.102.62\",13334,0");
	iot.eclipse.org   198.41.30.241
	*/

	switch( cmd->index )
	{
		case ATCSQ: 
			at( "AT+CSQ" );			
			break;
		case ATSIMTEST: 
			at( "AT+SIMTEST?" ); 
			break;
		case ATCPMS: 
			at( "AT+CPMS=\"SM\"" ); 
			break;
		case ATCMGD_: 
			at( "AT+CMGD=?" ); 
			break;
		case ATMIPCALL_: 
			at( "AT+MIPCALL?" ); 
			break;
		case ATMIPCALL0: 
			at( "AT+MIPCALL=0" ); 
			break;
		case ATMIPPROFILE: 
			at( "AT+MIPPROFILE=1,\"3GNET\"" ); 
			break;//3GNET
		case ATMIPCALL1: 
			at( "AT+MIPCALL=1" );
			break;
		case ATMIPOPEN: 
			if( mqtt_dev->iot ) 
			{
				at( "AT+MIPOPEN=1,0,\"198.41.30.241\",1883,0" );
			}
			else 
			{
				at( "AT+MIPOPEN=1,0,\"112.124.102.62\",1883,0" );
			}
		break;
		case ATMIPSEND: 
			//prepare_tick_packet();  delete
			break;
		case ATMIPPUSH: 
			at( "AT+MIPPUSH=1" ); 
			break;
		case ATCMGF: 
			at( "AT+CMGF=1" ); 
			break;
		case ATMIPCLOSE: 
			do_close_socket( cmd->para ); 
			break;		
		case ATCMGR: 
			do_read_sm( cmd->para ); 
			break;
		case ATCMGD: 
			do_delete_sm( cmd->para ); 
			break;	
		case ATMIPHEX: 
			at( "AT+MIPHEX=1" ); 
			break;
		case ATRESET: 
			at( "AT^RESET" ); 
			break;
		case ATMQTT: 
			prepare_mqtt_packet( mqtt_dev->mqtt_state, cmd ); 
			break;
			
		default: 
			break;
	}
}

static void make_command_to_list( char index, unsigned int interval, int para )
{
  	int i;	
	AtCommand *cmd = NULL;	
	
	//cmd = ( AtCommand * ) xmalloc( 0, sizeof( AtCommand ) );
	cmd = alloc_command();
	
	if( cmd != NULL ) 
	{
		dev->malloc_count++;
		cmd->mqttdata = NULL;
		cmd->mqttdata_len = 0;
		cmd->mqttack = 0;		
		cmd->index = index;
		cmd->interval = interval;
		cmd->tick_sum = 0;
		cmd->tick_tag = NOW_TICK;
		cmd->para = para;
		cmd->mqtt_try = 0;
		cmd->mqtt_clean = 0;
		cmd->atack = 0;
		
		if( index != ATMQTT )
		{
			cmd->mqtype = MQTT_MSG_TYPE_NULL;		
			add_cmd_to_list( cmd, &dev->at_head );	
		}
		else
		{
			cmd->mqttack = 0;
			cmd->mqttdata = NULL;
		
			switch( para ) 
			{
				case MQTT_MSG_TYPE_PUBLISH:
				case MQTT_MSG_TYPE_SUBSCRIBE:
				case MQTT_MSG_TYPE_PUBCOMP:
				case MQTT_MSG_TYPE_PUBREL:
				case MQTT_MSG_TYPE_PUBREC:
				case MQTT_MSG_TYPE_PUBACK:
				case MQTT_MSG_TYPE_PINGREQ:
				case MQTT_MSG_TYPE_DISCONNECT:
				case MQTT_MSG_TYPE_CONNECT: 
					cmd->mqtype = ( mqtt_message_type ) para; 
					break;
				default: 
					printf("%s: mqtt para error!\r\n", __func__); 
					break;
			}
			
			/*��Ҫmalloc�������Ҫ����msg_id����Ϣ*/
			if(	para == MQTT_MSG_TYPE_PUBLISH || para == MQTT_MSG_TYPE_PUBCOMP || 
				para == MQTT_MSG_TYPE_PUBREL || para == MQTT_MSG_TYPE_PUBREC || 
				para == MQTT_MSG_TYPE_PUBACK ) 
			{	
				for( i = 0; i < OUT_DATA_LEN_MAX; i++ )
				{
					if( mqtt_dev->mqtt_state->outdata[i] == NULL )
					{
						break;
					}
				}	
				
				if( i >= OUT_DATA_LEN_MAX ) 
				{
					cmd->mqttdata = NULL;
					//xfree( 0, cmd );
					dealloc_command( cmd );
					dev->free_count++;
					printf("\r\n");
					printf("ERROR: i=%d +++++++++++++OUT_DATA_LEN_MAX +++++++++++\r\n", i);
					printf("\r\n");
					return;
				} 
				else 
				{
					cmd->mqttdata_len = mqtt_dev->mqtt_state->outbound_message->length;
					//mqtt_dev->mqtt_state->outdata[i] = xmalloc( 0, cmd->mqttdata_len );
					mqtt_dev->mqtt_state->outdata[i] = ( unsigned char * ) alloc_mqtt_buffer( cmd->mqttdata_len );
//					if( lentag < cmd->mqttdata_len )
//						lentag = cmd->mqttdata_len;
					
					if( !mqtt_dev->mqtt_state->outdata[i] ) 
					{
						printf("\r\n");
						printf("ERROR: malloc for MQTT [%s] fail!\r\n", mqtt_get_name(cmd->mqtype));
						printf("\r\n");
						cmd->mqttdata = NULL;
						//xfree(0, cmd);
						dealloc_command( cmd );
						dev->free_count++;
						return;
					} 
					else 
					{
						/*store the outbound_message->data, make cmd->mqttdata point to it. */
						dev->malloc_count++;
						memcpy( mqtt_dev->mqtt_state->outdata[i], mqtt_dev->mqtt_state->outbound_message->data, cmd->mqttdata_len );
						cmd->mqttdata = mqtt_dev->mqtt_state->outdata[i];
						cmd->msgid = mqtt_get_id( mqtt_dev->mqtt_state->outbound_message->data, cmd->mqttdata_len );
						add_cmd_to_list( cmd, &dev->at_head );					
						/*when all alloc, add ATMIPPUSH command*/
						make_command_to_list( ATMIPPUSH, ONE_SECOND/50, MQTT_MSG_TYPE_NULL );
						if( cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ) 
						{
							mqtt_dev->pub_out_num++;
						}
					}
				}
			} 
			else 
			{
				/*mqtt message that don't need to cached the buffer*/
				add_cmd_to_list( cmd, &dev->at_head );	
				
				if( cmd->mqtype == MQTT_MSG_TYPE_CONNECT || cmd->mqtype == MQTT_MSG_TYPE_PINGREQ )
				{
					cmd->msgid = 0;
				}
			}
		} 
	}
}

/*set ack for nomal AT command*/
static void atcmd_set_ack(int index)
{
	int del_done = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	printf("%s: target %s\r\n", __func__, at_get_name(index));
	
	if(dev->atcmd != NULL && dev->atcmd->index == index) 
	{
		del_done = 1;
		dev->atcmd->atack = 1;
		printf("%s: [%s] is ACK, REMOVE it!\r\n", __func__, at_get_name(index));
	}
	else
	{		
		list_for_each_safe(pos, n, &dev->atcmd_head)
		{
			cmd = list_entry(pos, AtCommand, list);
			
			if(cmd->index == index) 
			{
				del_done = 1;
				cmd->atack = 1;
				printf("%s:[%s] is ACK, REMOVE it!\r\n", __func__,at_get_name(index));
				break;
			}
		}
	}
	
	if( del_done == 0 )
	{
		printf("%s: set [%s] fail!\r\n", __func__, at_get_name(index));
	}		
}

/*set ack for nomal mqtt command*/
static void mqtt_set_mesg_ack(int type, uint16_t msg_id)
{
	int del_done = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	if( dev->atcmd != NULL && dev->atcmd->mqtype == type &&
			dev->atcmd->msgid == msg_id && dev->atcmd->mqttack == 0 ) 
	{
		del_done = 1;
		dev->atcmd->mqttack = 1;
		printf("%s: type=%d, msgid=0x%04x, [%s] is ACK, REMOVE it!\r\n", __func__, 
							dev->atcmd->mqtype, dev->atcmd->msgid, mqtt_get_name(cmd->mqtype));
	}
	else
	{		
		list_for_each_safe(pos, n, &dev->mqtt_head)
		{
			cmd = list_entry(pos, AtCommand, list);
			
			if(cmd->mqtype == type && cmd->msgid == msg_id) 
			{
				del_done = 1;
				cmd->mqttack = 1;
				printf("%s: msgid=0x%04x, [%s] is ACK, REMOVE it!\r\n", __func__, 
						msg_id , mqtt_get_name( type ) );
				//break; delete for add MQTT_MSG_TYPE_PINGREQ in mqtt_head!
			}
		}
	}
	
	if( del_done == 0 )
	{
		printf("%s: set [%s] fail! [type=%d, msgid=%d]\r\n", __func__, 
			mqtt_get_name(type), type, msg_id );
	}	
}

/*ɾ������ڵ㣬�ͷ�AtCommand*/
static void longsung_release_command( AtCommand *cmd, struct list_head *head, struct list_head *pos )
{
	int i;
	
	if( head )
	{
		del_cmd_from_list( cmd, head );
	}
	else if( pos )
	{
		list_del( pos );
	}

	/*release mqtt type message data: mqttdata pointer!*/
	if( cmd->mqtype != MQTT_MSG_TYPE_NULL && cmd->mqttdata != NULL ) 
	{	
		for( i = 0; i < OUT_DATA_LEN_MAX; i++ )
		{
			if( cmd->mqttdata != NULL && mqtt_dev->mqtt_state->outdata[i] == cmd->mqttdata ) 
			{
				dealloc_mqtt_buffer( ( char* ) cmd->mqttdata );
				dev->free_count++;
				mqtt_dev->mqtt_state->outdata[i] = NULL;						
				break;
			}
		}
	}
	
	dealloc_command( cmd );
	dev->free_count++;	
}

static void release_command_from_list( struct list_head *head )
{
	int total = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	printf("%s start.\r\n", __func__);
	
	list_for_each_safe( pos, n, head )
	{
		total++;
		cmd = list_entry( pos, AtCommand, list );
		longsung_release_command( cmd, NULL, pos );
	}
	
	printf("%s: release %d cmd form %s\r\n", __func__, total, (&(dev->at_head)==head)?"at_head":"mqtt_head");
}

/*store AT command for send again.*/
static void atcmd_list_cmd( void )
{
	unsigned int tick;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	list_for_each_safe( pos, n, &dev->atcmd_head )
	{
		cmd = list_entry( pos, AtCommand, list );

		tick = NOW_TICK;
		cmd->tick_sum += tick - cmd->tick_tag;
		cmd->tick_tag = tick;
		
		if( cmd->atack == 1 )
		{	
			printf("%s: releas command %s!\r\n", __func__, at_get_name( cmd->index ));
			longsung_release_command( cmd, &dev->atcmd_head, NULL );
		}	
		else if( cmd->tick_sum >= cmd->interval )
		{
			list_del( pos );
			
			if( cmd->index == ATMIPOPEN && dev->socket_num == 0 && dev->ppp_status == PPP_CONNECTED )
			{
				cmd->interval = ONE_SECOND / 3;
				add_cmd_to_list( cmd, &dev->at_head );				
			}
			else if( cmd->index == ATMIPHEX )
			{
				cmd->interval = ONE_SECOND / 20;
				add_cmd_to_list( cmd, &dev->at_head );						
			}
			else
			{
				longsung_release_command( cmd, NULL, NULL );
				printf("%s: warnning->release command whatevet! ip(%s)\r\n", __func__, dev->ip);
				return;
			}
			cmd->tick_sum = 0;
			printf("%s: add [%s] to AT LIST HEAD again! tick(%u)\r\n", __func__, at_get_name( cmd->index ),
				NOW_TICK);			
		}
	}
	
}

/*store mqtt command for send again*/
static void mqtt_list_cmd( void )
{
	int i = 0;
	unsigned int tick;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	list_for_each_safe( pos, n, &dev->mqtt_head )
	{	
		cmd = list_entry( pos, AtCommand, list );

		tick = NOW_TICK;
		cmd->tick_sum += tick - cmd->tick_tag;
		cmd->tick_tag = tick;
		
		if( cmd->mqtt_clean ) 
		{
			printf("%s: clean message [%d].\r\n", __func__, i++);
			longsung_release_command( cmd, &dev->mqtt_head, NULL );
			continue;
		}
		else if( cmd->mqttack == 1 )
		{
			longsung_release_command( cmd, &dev->mqtt_head, NULL );
		}
		else if( cmd->tick_sum >= cmd->interval )		
		{ 
			list_del( pos );
			cmd->interval = ONE_SECOND / 20;
			cmd->tick_sum = 0;
			
			/*Ŀǰֻ��MQTT_OUTDATA_PUBLISH����Ҫ��� mqttdata*/
			if( cmd->mqttdata == NULL && ( cmd->mqtype==MQTT_MSG_TYPE_PUBLISH ) &&
					( cmd->mqtype==dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREL ) &&
					( cmd->mqtype==dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREC ) ) 
			{
				dealloc_command( cmd );
				dev->free_count++;
				printf("ERROR: [%s] mqttdata is NULL remote it from mqtt_head.\r\n", mqtt_get_name( cmd->mqtype ));
				continue;
			}
			
			/*����3��, �Ͽ�MQTT������*/
			if( ++( cmd->mqtt_try ) > 2 )
			{
				printf("ERROR: [%s mqtt_try > 2 ], type=%d,id=0X%04X,len=%d,p=0x%p\r\n", mqtt_get_name( cmd->mqtype ), 
									cmd->mqtype, cmd->msgid, cmd->mqttdata_len, cmd->mqttdata );
				/*ֻҪ����DISCONNECT���Ϳ�����*/
				/*Ȼ���ٶϿ�TCP����*/
				make_command_to_list( ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_DISCONNECT );
				make_command_to_list( ATMIPPUSH, ONE_SECOND/2, MQTT_MSG_TYPE_NULL );
				dev->close_tcp_interval = 3 * ONE_SECOND;
				dev->tick_sum = 0;
				dev->tick_tag = NOW_TICK;				
				/*ֻ�ͷ�CMD*/
				longsung_release_command( cmd, NULL, NULL );
				continue;
			}
			
			/*Ŀǰ����Э��ֻ��PUBLISH��SUBSCRIBE�õõ����������*/
			if( mqtt_get_qos( cmd->mqttdata ) && ( cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ||
						cmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE ) )
			{
				mqtt_set_dup( cmd->mqttdata );
			}
			add_cmd_to_list( cmd, &dev->at_head );
			make_command_to_list( ATMIPPUSH, ONE_SECOND/40, MQTT_MSG_TYPE_NULL );
			printf("%s: add [%s] to AT LIST HEAD again! type=%d, msgid=0x%04x!\r\n", 
							__func__, mqtt_get_name( cmd->mqtype ), cmd->mqtype, cmd->msgid);			
		} 	
	}
	
}

/*get a command to send
root@2:/ #mqttp hi tom      
str( hi tom) strlen(str)=7
str(hi tom)
usmart: mqtt publish test. topic=/xp/publish, payload=hi tom, qos=2
alloc_mqtt_buffer: alloc node ok! len(23), pMqttBuff[0][0].apacity(50)

root@2:/ #sending [PUBLISH]! mqtype=3,id=0x0035
AT+MIPSEND=1,"3415000b2f78702f7075626c6973680035686920746f6d"
+MIPSEND:1,1477

OK
sending [PUBLISH]! mqtype=3,id=0x0035
AT+MIPSEND=1,"3415000b2f78702f7075626c6973680035686920746f6d"
+MIPSEND:1,1454

OK
sending [PUBLISH]! mqtype=3,id=0x0035
AT+MIPSEND=1,"3415000b2f78702f7075626c6973680035686920746f6d"
+MIPSEND:1,1431

OK
sending [PUBLISH]! mqtype=3,id=0x0035
AT+MIPSEND=1,"3415000b2f78702f7075626c6973680035686920746f6d"
+MIPSEND:1,1408

OK
sending [PUBLISH]! mqtype=3,id=0x0035
AT+MIPSEND=1,"3415000b2f78702f7075626c6973680035686920746f6d"
+MIPSEND:1,1385

OK
sending [PUBLISH]! mqtype=3,id=0x0035

4G: malloc=404, free=9388,



sending [PUBLISH]! mqtype=3,id=0x0004
AT+MIPSEND=1,"3215000b2f78702f7075626c6973680004686920746f6d"
ERROR
Sytem times from boot is 1450 (s), system tick = 1450036 (ms).
sending [PUBLISH]! mqtype=3,id=0x0004
********************************* lentag(274)
scsq-rcsq=4! error.
AT+MIPSEND=1,"3215000b2f78702f7075626c6973680004686920746f6d"
ERROR
init_mqtt_dev
cmodule_power_reset 
sending [PUBLISH]! mqtype=3,id=0x0004
sending [PINGREQ]! mqtype=12,id=0x0000
sending [PINGREQ]! mqtype=12,id=0x0000
mqtt_list_cmd: add [PINGREQ] to AT LIST HEAD again! type=12, msgid=0x0000!
sending [PINGREQ]! mqtype=12,id=0x0000
[0] welcome to lk

[10] platform_init()
[10] target_init()
[10] smem ptable found: ver: 4 len: 19
[10] Loading boot image (3684352): start
mqtt_list_cmd: add [PINGREQ] to AT LIST HEAD again! type=12, msgid=0x0000!

*/
static void at_list_cmd( void )
{
	/*��ȡҪ���͵�ATָ�������*/
	if( dev->atcmd == NULL ) 
	{
		dev->atcmd = get_cmd_from_list( &dev->at_head );
		
		if( dev->atcmd && dev->atcmd->mqtt_clean ) 
		{
			printf("%s: clean\r\n", __func__);
			longsung_release_command( dev->atcmd, &dev->at_head, NULL );
			dev->atcmd = NULL;		
		} 
		else if( dev->atcmd ) 
		{
			send_command_to_device( dev->atcmd );
			dev->atcmd->tick_tag = NOW_TICK;
			dev->atcmd->tick_sum = 0;
		}
	} 
	else 
	{
		unsigned int tick = NOW_TICK;
		dev->atcmd->tick_sum += tick - dev->atcmd->tick_tag;
		dev->atcmd->tick_tag = tick;
		
		if( dev->atcmd->mqtt_clean ) 
		{
			printf("%s: clean\r\n", __func__);
			longsung_release_command( dev->atcmd, &dev->at_head, NULL );
			dev->atcmd = NULL;
			return;
		}
		
		if( dev->atcmd->tick_sum >= dev->atcmd->interval )
		{
			del_cmd_from_list( dev->atcmd, &dev->at_head );
			
			/*Ŀǰֻ��������Ϣ��Ҫ���ظ�, �¼���MQTT_DEV_STATUS_CONNECT*/
			if( ( ( dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBLISH && dev->atcmd->mqttdata!= NULL && 
						mqtt_get_qos( dev->atcmd->mqttdata ) ) || dev->atcmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_CONNECT || dev->atcmd->mqtype == MQTT_MSG_TYPE_PINGREQ ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREC || dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREL ) && 
						( dev->atcmd->mqttack == 0 ) ) 
			{
				/*wait for 5 second*/
				if( dev->atcmd->mqtype == MQTT_MSG_TYPE_PINGREQ )
				{
					dev->atcmd->interval = 3 * ONE_SECOND;
				}
				else 
				{
					dev->atcmd->interval = 6 * ONE_SECOND;
				}
				dev->atcmd->tick_sum = 0;
				add_cmd_to_list( dev->atcmd, &dev->mqtt_head );
				printf("ADD [%s] to mqtt_head to wait ACK! msgid(0x%04x), qos(%d)\r\n", 
						mqtt_get_name(dev->atcmd->mqtype), dev->atcmd->msgid, 
						mqtt_get_qos( dev->atcmd->mqttdata ) );
			}
			/* AT command need to be resend*/
			else if( ( dev->atcmd->index == ATMIPOPEN || dev->atcmd->index == ATMIPHEX ) && 
						dev->atcmd->atack == 0 )
			{
				if( dev->atcmd->index == ATMIPOPEN )
				{
					dev->atcmd->interval = 8 * ONE_SECOND;
				}
				else if( dev->atcmd->index == ATMIPHEX )
				{
					dev->atcmd->interval = 2 * ONE_SECOND;
				}
				dev->atcmd->tick_sum = 0;
				add_cmd_to_list( dev->atcmd, &dev->atcmd_head );
				printf("ADD [%s] to atcmd_head to wait ACK! tick(%u)\r\n", at_get_name( dev->atcmd->index ), NOW_TICK);				
			}
			else 
			{
				/*just print ATMIPOPEN and ATMIPHEX for debug.*/
				if( ( dev->atcmd->index == ATMIPOPEN || dev->atcmd->index == ATMIPHEX) )
				{
					printf("%s: release command (%s)\r\n", __func__, at_get_name( dev->atcmd->index ));
				}
				longsung_release_command( dev->atcmd, NULL, NULL );
			}
			
			dev->atcmd = NULL;
		}
	}		
}

static void mqtt_msg_set_clean(void)
{
	int total = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &dev->mqtt_head)
	{
		cmd = list_entry(pos, AtCommand, list);
		if(cmd->mqtype != MQTT_MSG_TYPE_NULL && cmd->mqtt_clean == 0) 
		{
			total++;
      cmd->mqtt_clean = 1;
		}
	}
	
	list_for_each_safe(pos, n, &dev->at_head)
	{
		cmd = list_entry(pos, AtCommand, list);
		if(cmd->mqtype != MQTT_MSG_TYPE_NULL && cmd->mqtt_clean == 0) 
		{
			total++;
      cmd->mqtt_clean = 1; 
		}
	}		
	
	printf("%s: clean [%d] mqtt message!\r\n", __func__, total);	
}

static void check_socket_number( void )
{
	int i;
	
	/*����Ծ��socket����������һ������*/
	dev->socket_num = 0;

	for( i=0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
	{
		if( dev->socket_open[i] != -1 )
			dev->socket_num++;
	}

	/*������socket����1���ر�����socket,���½�������*/
	if( ( dev->socket_num > 1 || dev->socket_close_flag ) ) 
	{
		/*����Ƿ��Ѿ�����ATMIPCLOSE*/
		if(!check_command_exist( ATMIPCLOSE, &dev->at_head )) 
		{ 
			//for( i=0; i < sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) 
			for( i = 0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
			{
				if( dev->socket_open[i] != -1 ) 
				{
					make_command_to_list( ATMIPCLOSE, ONE_SECOND/5, dev->socket_open[i] );
				}
			}
		}
		
		if( dev->socket_close_flag == 1 ) 
			dev->socket_close_flag = 0;
	}	
}

static void mqtt_send_connect_or_tick(void)
{
	if( dev->socket_num == 1) 
	{//��������
		if( dev->ppp_status == PPP_CONNECTED && dev->tcp_connect ) 
		{//״̬
			/*������MQTT*/
			if( mqtt_dev->connect_status == MQTT_DEV_STATUS_NULL ||
				 mqtt_dev->connect_status == MQTT_DEV_STATUS_DISCONNECT ) 
			{//����״̬
				/*����ǰ��  mqtt_list at_list ��mqtt��Ϣ��գ���+MIPCLOSE:���*/			
				mqtt_dev->connect_status = MQTT_DEV_STATUS_CONNECTING;
				make_command_to_list(ATMQTT, ONE_SECOND/20, MQTT_MSG_TYPE_CONNECT);
				make_command_to_list(ATMIPPUSH, ONE_SECOND/25, MQTT_MSG_TYPE_NULL);	
			}		
			/*���������������� 50sÿ��tick*/				
			if( dev->heartbeat_tick >= 3 ) 
			{//��������
				if(mqtt_dev->connect_status != MQTT_DEV_STATUS_NULL
						&& mqtt_dev->connect_status != MQTT_DEV_STATUS_CONNECTING) 
				{//����״̬
					dev->heartbeat_tick = 0;
					if(mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT) 
					{
						//�п�����������״̬MQTT_DEV_STATUS_CONNACK  MQTT_DEV_STATUS_SUBACK
						make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PINGREQ);
						make_command_to_list(ATMIPPUSH, ONE_SECOND/50, MQTT_MSG_TYPE_NULL);			
					}
				}									
			} 
		}
	}	
}

static void tcp_connect_server(void)
{
	/*������Զ�̷����*/
	if( dev->socket_num == 0 ) 
	{//��������
		if( dev->ppp_status == PPP_CONNECTED && dev->simcard_type > 0 ) 
		{//״̬
			if( !check_command_exist( ATMIPOPEN, &dev->at_head ) &&
					!check_command_exist( ATMIPOPEN, &dev->atcmd_head ) )
			{
				/*ȡ����һ��Ҫ�ر�tcp������*/
				if( dev->close_tcp_interval ) 
				{
					dev->close_tcp_interval = 0;//����
				}
				/*��ATMIPOPEN ONE_SECOND/2 ��ſɷ�ATMIPHEX�� ����ATMIPHEXʧ��*/
				make_command_to_list( ATMIPHEX, ONE_SECOND/30, MQTT_MSG_TYPE_NULL );							
				make_command_to_list( ATMIPOPEN, ONE_SECOND/2, MQTT_MSG_TYPE_NULL );			
				//make_command_to_list(ATMIPHEX, ONE_SECOND/2, MQTT_MSG_TYPE_NULL);					
				dev->heartbeat_tick = 6;
			}
		}
	}	
}

static void check_sm_and_ppp(void)
{
	if( dev->simcard_type > 0 ) 
	{
		/*��ѯSM����δ��index*/
		make_command_to_list(ATCPMS, ONE_SECOND/20, MQTT_MSG_TYPE_NULL);				
		make_command_to_list(ATCMGD_, ONE_SECOND/20, MQTT_MSG_TYPE_NULL);
		/*��ѯSM����δ��index,��ѯ���ж��ţ��ͻ����Ͻ����ģ�����*/

		/*��ѯIP*/
		make_command_to_list(ATMIPCALL_, ONE_SECOND/20, MQTT_MSG_TYPE_NULL);							
	}

	/*PPP���ӻ�ȡIP*/		
	if( dev->simcard_type > 0 ) 
	{//��������
		if( dev->ppp_status == PPP_DISCONNECT ) 
		{ //״̬
			dev->ppp_status = PPP_CONNECTING;//�޸�״̬
			/*ppp for get ip*/
			make_command_to_list(ATMIPCALL0, ONE_SECOND/5, MQTT_MSG_TYPE_NULL);//����	
			make_command_to_list(ATMIPPROFILE, ONE_SECOND/5, MQTT_MSG_TYPE_NULL);	
			make_command_to_list(ATMIPCALL1, ONE_SECOND, MQTT_MSG_TYPE_NULL);
		}						
	}	
}

static void check_reset_condition(void)
{
	/*if there have not sim card, we reset communicate module for something 4 peroid*/
	if( dev->simcard_type == 0 && dev->scsq > 3 ) 
	{
		dev->reset_request = 1;
		printf("sim card no exit! reset the communication module!\r\n");
	}	

	if( ( dev->scsq ) - ( dev->rcsq ) > 3 ) 
	{
		dev->reset_request = 1;
		dev->socket_close_flag = 1;
		printf("scsq-rcsq=%d! reset the communication module!\r\n", 
			dev->scsq - dev->rcsq );
	}
}

static void send_init_at_command(void)
{
	/*��ѯSIM���Ƿ����, -1Ϊ��ʼ��״̬��0Ϊ�޿�*/
		
	/*Ϊ�˵�һʱ�������Ϸ�����*/
	make_command_to_list(ATSIMTEST, ONE_SECOND/10, MQTT_MSG_TYPE_NULL);
	make_command_to_list(ATMIPCALL_, ONE_SECOND/10, MQTT_MSG_TYPE_NULL);
	make_command_to_list(ATMIPCALL0, ONE_SECOND/10, MQTT_MSG_TYPE_NULL);	
	make_command_to_list(ATMIPPROFILE, ONE_SECOND/10, MQTT_MSG_TYPE_NULL);	
	make_command_to_list(ATMIPCALL1, ONE_SECOND, -1);
	/*ATMIPHEX��ʱû�ɹ���ע��ԭ��*/
	make_command_to_list(ATMIPHEX, ONE_SECOND/20, MQTT_MSG_TYPE_NULL);						
	make_command_to_list(ATMIPOPEN, ONE_SECOND/2, MQTT_MSG_TYPE_NULL);

	/*****************************************************************************
	ATMIPHEX���ɹ������·���1438 ������1469�� 1500Ϊbuffer���ȡ�
	AT+MIPSEND=1,"101d00064d5149736470030400780003594a5a00036d637500056465617468"
	+MIPSEND:1,1438
	*****************************************************************************/				

	make_command_to_list(ATMIPCALL_, ONE_SECOND/5, MQTT_MSG_TYPE_NULL);		
	dev->heartbeat_tick = 6;				
}

static void list_sm_event(void)
{
	/*ɾ��sim���еĶ���, ������ص�ATָ��ʱ��Ҫ��Щ�ONE_SECOND/5~ONE_SECOND/2*/
	if( dev->sm_index_delete != -1 && dev->sm_delete_flag ) 
	{
		make_command_to_list(ATCPMS, ONE_SECOND/5, MQTT_MSG_TYPE_NULL );
		make_command_to_list(ATCMGD, ONE_SECOND/2, dev->sm_index_delete);					
		dev->sm_delete_flag = 0;
	}

	/*��ȡsim���еĶ���*/
	if( dev->sm_num > 0 && dev->sm_read_flag ) 
	{
		make_command_to_list(ATCPMS, ONE_SECOND/5, MQTT_MSG_TYPE_NULL );	
		make_command_to_list(ATCMGF, ONE_SECOND/5, MQTT_MSG_TYPE_NULL );
		make_command_to_list(ATCMGR, ONE_SECOND/5, dev->sm_index[0]);					
		dev->sm_read_flag = 0;
	}	
}

static void clean_run_status()
{
	int i;
	char cmd[15];

	if( dev->tick_sum == 0 )
	{	
		dev->tick_sum++;
		release_command_from_list( &dev->at_head );
		release_command_from_list( &dev->mqtt_head );
		release_command_from_list( &dev->atcmd_head );
		
		if( mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT )
		{
			mqtt_disconnect_server( mqtt_dev->mqtt_state );
		}
		vTaskDelay( 500 / portTICK_RATE_MS );
		for( i = 0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
		{
			if( dev->socket_open[i] != -1 ) 
			{ 
				memset(cmd, '\0', sizeof(cmd));
				sprintf(cmd, "AT+MIPCLOSE=%d", i);
				vTaskDelay( 10 / portTICK_RATE_MS );
				at( cmd );
				vTaskDelay( 50 / portTICK_RATE_MS );				
			}
		}				
	}
}

/*״̬��ģʽ�� ��������д����*/
void handle_cmodule_setting(void)
{
	static char period = 0;
	
	/*android�رգ�����4Gģ��*/
	if( dev->reset_request ) 
	{	
		/*register timer when android power down.*/
		if( pdFAIL == xTimerReset( xTimer, 2 ) )
		{
			printf("%s fail to xTimerReset timer\r\n", __func__);
		}
		else
		{
			printf("%s xTimer working.\r\n", __func__);
		}

		init_cmodule_status( 0 );
		init_mqtt_dev( mqtt_dev );					
		/*reset 4g modules by gpio*/
		cmodule_power_reset();
		release_command_from_list( &dev->at_head );
		release_command_from_list( &dev->mqtt_head );
		release_command_from_list( &dev->atcmd_head );		
	}
	
	if( dev->period_tick ) 
	{
		/*check at every 50s*/
		dev->period_tick = 0;
		dev->heartbeat_tick++;		
		period = 1;	
		
		printf("*********************************\r\n");
		make_command_to_list( ATCSQ, ONE_SECOND/10, MQTT_MSG_TYPE_NULL );
		dev->scsq++;

		if( dev->simcard_type == -1 ) 
		{				
			send_init_at_command();
		}			
	}

	if( dev->boot_status ) 
	{
		/*check at every moment*/
		if( period ) 
		{
			/*check at every 40s*/
			check_reset_condition();		
			check_sm_and_ppp();
		}

		/*for delay set socket_close_flag to 1*/
		if( dev->close_tcp_interval > 0 ) 
		{
			unsigned int tick = NOW_TICK;
			dev->tick_sum += tick - dev->tick_tag;
			dev->tick_tag = tick;				
			if( dev->tick_sum >= dev->close_tcp_interval )
			{
				dev->close_tcp_interval = 0;
				dev->socket_close_flag = 1;
				printf("%s: set socket_close_flag to 1\r\n", __func__);
			}
		}
		check_socket_number();
			
		if( !dev->tcp_connect ) 
		{ 
			tcp_connect_server();
		}
		mqtt_send_connect_or_tick();
		list_sm_event();
	}

	at_list_cmd();
	mqtt_list_cmd();	
	atcmd_list_cmd(); 
	period = 0;	
}

void cmodule_clean_work( void )
{
	unsigned int tick;

	/*init tick_sum and tick_tag*/
	dev->tick_sum = 0;
	dev->tick_tag = NOW_TICK;
	
	while( 1 ) 
	{
		if( !dev->is_inited )
		{
			if( xTimerIsTimerActive( xTimer ) != pdFALSE )
			{
				if( pdFAIL == xTimerStop( xTimer, 0 ) )
				{
					printf("%s fail to stop xTimer\r\n", __func__);
				}
				else
				{
					printf("%s stop xTimer ok.\r\n", __func__);
				}
			}
			clean_run_status();
			tick = NOW_TICK;
			dev->tick_sum += tick - dev->tick_tag;
			dev->tick_tag = tick;					
			if( dev->tick_sum < dev->clean_interval )
			{
				continue;
			}
			else 
			{
				printf("timeout! clean_interval = 0, enter ANDROID.\r\n");
			}
			rfifo_clean( uart3fifo );
			/*set mqtt_alive to 0*/
			init_cmodule_status( 1 );
			init_mqtt_dev( mqtt_dev );
			/*reset 4g modules by gpio*/
			cmodule_power_reset(); 	
		}
		
		break;
	}		

}

void cmodule_init()
{	
	init_cmodule_reader();
	init_cmodule_status( 1 );
	
	INIT_LIST_HEAD( &dev->at_head );
	INIT_LIST_HEAD( &dev->mqtt_head );
	INIT_LIST_HEAD( &dev->atcmd_head );	
	
	init_mqtt_dev( mqtt_dev );
	
	/*init one time!*/
	dev->sys_time = 0;
	uartlost = uart3fifo->lostBytes;
	mqtt_dev->recv_bytes = 0;
	dev->mqtt_dev = mqtt_dev;
	mqtt_dev->pDev = dev;	
}

extern void android_power_on( void );
extern void android_power_off( void );
extern TaskHandle_t pxLongSungTask;
/*
* author:	yangjianzhou
* function: 	notifyAndroidPowerOn  can not call in interrupt.
*/
void notifyAndroidPowerOn( void )
{
	android_power_on();
	mAndroidPowerStatus = 1;
}

/*
* author:	yangjianzhou
* function: 	notifyAndroidPowerOff  can not call in interrupt.
*/
void notifyAndroidPowerOff( void )
{
	android_power_off();
	mAndroidPowerStatus = 0;
	xTaskNotifyGive( pxLongSungTask );
}

/*
* author:	yangjianzhou
* function: 	HandleCanTask  handle can interrupt data, push to sender task stream.
*/
void HandleLongSungTask( void * pvParameters )
{
	init_command_buffer( 60 );
	init_mqtt_buffer();
	rfifo_init( uart3fifo, 1024 * 2 );	
	(void) uart3_init( 115200 );
	cmodule_init();
 	xTimer = xTimerCreate( "longsung", 50000 / portTICK_RATE_MS, 
							pdTRUE, dev, notify_longsung_period );
	vSetTaskLogLevel( NULL, eLogLevel_3 );
	printf("%s: start...\r\n", __func__);
	//notifyAndroidPowerOn();
	while( 1 )
	{
		if( mAndroidPowerStatus )
		{
			printf("%s: android power on, go to sleep..\r\n", __func__);
			ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		}
		printf("%s: android power off, go to work..\r\n", __func__);
		while( !mAndroidPowerStatus )
		{
			handle_cmodule_uart_msg();
			handle_cmodule_setting();
		}
		cmodule_clean_work();
	}
}








