#include "longsung.h"
#include "mqtt_msg.h"
#include "md5.h"

static int waitting_len = 0;
static int in_buffer_pos = 0;

DevStatus dev[1];
static UartReader reader[1];
mqtt_dev_status mqtt_dev[1];
	
static char* make_mqtt_packet(char* buff, unsigned char* data, int len);
static void make_command_to_list(char index, long long interval, int para);
static void mqtt_set_mesg_ack(int type, uint16_t msg_id);

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

static int is_little_endian()
{
	int wTest = 0x12345678;
	short *pTest=(short*)&wTest;
	//printf("%s:%04x\r\n", __func__, pTest[0]);
	return !(0x1234 == pTest[0]);
}

/*网络字节是大端*/
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

static int longsung_tokenizer_init( RemoteTokenizer* t, const char* p, const char* end )
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

static Token longsung_tokenizer_get( RemoteTokenizer* t, int index )
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
/*
static void parse_frame_head(char *buf, int len)
{
	FrameHead *fhead;
	
	fhead = (FrameHead *)buf;
	memcpy(&(dev->fh), fhead, sizeof(FrameHead));

	printf("fhead->ver=%02x\r\n", fhead->ver);
	
	if((fhead->compress_encrypt >> 4) == 0x01) {
		printf("FrameHead->compress = 1\r\n");//compress type 1
	}
	if((fhead->compress_encrypt & 0x0f) == 0x01) {
		printf("FrameHead->encrypt=0x01\r\n");//encrypt
	}

	fhead->datasize = ntohl(fhead->datasize);
	printf("FrameHead->datasize = %04x\r\n", fhead->datasize);	
}

static void parse_data_head(char *buf, int len)
{
	DataHead *dhead;

	dhead = (DataHead*)buf;
	memcpy(&(dev->dh), dhead, sizeof(DataHead));
	
	printf("dhead->rpc=%02x\r\n", dhead->rpc);
	printf("dhead->rpcmethrod=%02x, %02x, %02x\r\n", 
		dhead->rpcmethrod[0],dhead->rpcmethrod[1],dhead->rpcmethrod[2] );
	dhead->rpcid = ntohl(dhead->rpcid);
	printf("DataHead->rpcid=%04x\r\n", dhead->rpcid);
	dhead->jsonlen = ntohl(dhead->jsonlen);
	printf("DataHead->jsonlen=%04x\r\n", dhead->jsonlen);	
}


static void parse_packet(char *buf, int len)
{
	parse_frame_head(buf, len);
	if(dev->fh.datasize > 0) {
		parse_data_head(buf+sizeof(FrameHead), len);
		if(dev->dh.jsonlen > 0) {
			parse_json(buf+sizeof(FrameHead)+sizeof(DataHead));
		}
		if(dev->fh.datasize-sizeof(DataHead)-dev->dh.jsonlen) {
			;//处理二进制包
		}
	}
}
*/

static char *mqtt_get_name(int type)
{
	switch(type)
	{
		case MQTT_MSG_TYPE_PUBLISH:
			return "MQTT_MSG_TYPE_PUBLISH";
		case MQTT_MSG_TYPE_SUBSCRIBE:
			return "MQTT_MSG_TYPE_SUBSCRIBE";
		case MQTT_MSG_TYPE_PUBCOMP:
			return "MQTT_MSG_TYPE_PUBCOMP";
		case MQTT_MSG_TYPE_PUBREL:
			return "MQTT_MSG_TYPE_PUBREL";
		case MQTT_MSG_TYPE_PUBREC:
			return "MQTT_MSG_TYPE_PUBREC";	
		case MQTT_MSG_TYPE_PUBACK:
			return "MQTT_MSG_TYPE_PUBACK";	
		case MQTT_MSG_TYPE_PINGREQ:
			return "MQTT_MSG_TYPE_PINGREQ";
		case MQTT_MSG_TYPE_PINGRESP:
			return "MQTT_MSG_TYPE_PINGRESP";
		case MQTT_MSG_TYPE_CONNECT:
			return "MQTT_MSG_TYPE_CONNECT";
		case MQTT_MSG_TYPE_CONNACK:
			return "MQTT_MSG_TYPE_CONNACK";
		case MQTT_MSG_TYPE_SUBACK:
			return "MQTT_MSG_TYPE_SUBACK";
		case MQTT_MSG_TYPE_DISCONNECT:
			return "MQTT_MSG_TYPE_DISCONNECT";
		case MQTT_MSG_TYPE_UNSUBSCRIBE:
			return "MQTT_MSG_TYPE_UNSUBSCRIBE";
		case MQTT_MSG_TYPE_UNSUBACK:
			return "MQTT_MSG_TYPE_UNSUBACK";

		default: return "MSG UNKNOW";
	}
}

static void complete_pending(mqtt_state_t* state, int event_type, uint16_t mesg_id)
{
/*	
  mqtt_event_data_t event_data;

  state->pending_msg_type = 0;
  state->mqtt_flags |= MQTT_FLAG_READY;
  event_data.type = event_type;	
*/	
	printf("%s: type=%d, msg_id=%d\r\n", __func__, event_type, mesg_id);
  //process_post_synch(state->calling_process, mqtt_event, &event_data);
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

	aes_destory_key(key);		
}

static int mqtt_check_json_md5(mqtt_state_t* state, char *data, int len, char *md5)
{
	int i, result = 1;
	char calmd5[LEN_MD5];
	
	cal_md5((uint8_t *)data, len, calmd5);
	
	/*decode buffer*/
	mqtt_aes_buffer(0, md5, LEN_MD5);
	
  for(i=0; i<LEN_MD5; i++) 
	{
		if(calmd5[i] != md5[i]) result = 0;
	}

	//printf("%s: result=%d\r\n", __func__, result);
	return result;
}

static int mqtt_publish(mqtt_state_t *state, const char* topic, char* data, int qos, int retain);
	
static void deliver_publish(mqtt_state_t* state, uint8_t* message, int length)
{
	char topic[128], *payload = NULL;
  mqtt_event_data_t event_data;

  event_data.type = MQTT_EVENT_TYPE_PUBLISH;
  event_data.topic_length = length;
  event_data.topic = mqtt_get_publish_topic(message, &event_data.topic_length);

  event_data.data_length = length;
  event_data.data = mqtt_get_publish_data(message, &event_data.data_length);
	
	if(event_data.topic_length > sizeof(topic) || event_data.data_length <= 0
		 || (event_data.data_length + event_data.topic_length) > 1480)
	{
		waitting_len = 0;
		
		printf("%s: DATA information ERROR! topic.length = %d,payload.length = %d\r\n"
							,__func__, event_data.topic_length, event_data.data_length);
		return;
	}
	/*不管是对的消息还是错的消息，都应该把waitting_len置0*/
	waitting_len = 0;
	
	payload = mymalloc(0, event_data.data_length+1);
	if(!payload) 
	{
		printf("%s: malloc payload fail! len=%d\r\n", __func__, event_data.data_length+1);
		return ;
	}
	//printf("event_data.data_length <= LEN_MD5, recv data len error !\r\n");
	memcpy(topic, event_data.topic, event_data.topic_length);
	memcpy(payload, event_data.data, event_data.data_length);
	topic[event_data.topic_length] = '\0';
	payload[event_data.data_length] = '\0';	

	printf("topic.length = %d, topic=%s\r\n", event_data.topic_length, topic);
	printf("payload.length = %d, payload=%s\r\n", event_data.data_length, payload);		
	
	if(!memcmp(payload, "8222280", strlen("8222280")))
	{
		printf("MCU SEND publish message.....\r\n");
		memset(state->jsonbuff, '\0', sizeof(state->jsonbuff));
		//memcpy(state->jsonbuff, "ABCD", strlen("ABCD"));
		sprintf(state->jsonbuff, "{\"addr\": \"/xp/publish\",\"format\":{\"type\":\"rect\",\"publish_num\":%d}}", mqtt_dev->pub_in_num);
		
		mqtt_publish(state, "/xp/publish", state->jsonbuff, 1, 0);		
	}

	if(!memcmp(payload, "123ddd0000", strlen("123ddd")))
	{
		//printf("%s: 123ddd0000\r\n", __func__);
		printf("MCU SEND publish message.....\r\n");
		memset(state->jsonbuff, '\0', sizeof(state->jsonbuff));
		memcpy(state->jsonbuff, "MCU mesg!", strlen("MCU mesg!"));
		//memcpy(state->jsonbuff, "{\"name\": \"jzyang\",\"format\":{\"type\":\"rect\",\"width\":1080,\"interlace\":false}}t", 
		//	strlen("{\"name\": \"jzyang\",\"format\":{\"type\":\"rect\",\"width\":1080,\"interlace\":false}}t"));

		mqtt_publish(state, "sensor", state->jsonbuff, 1, 0);					
	}		
	
	myfree(0, payload);
	
	return ;

/*	
	event_data.data_length = event_data.data_length - LEN_MD5;
  memmove((char*)event_data.data + 1, (char*)event_data.data, event_data.data_length);
  event_data.data += 1;
  ((char*)event_data.topic)[event_data.topic_length] = '\0';
  ((char*)event_data.data)[event_data.data_length] = '\0';

	printf("length = %d, topic=%s\r\n", event_data.topic_length, event_data.topic);
	printf("length = %d, data=%s\r\n", event_data.data_length,  event_data.data);
	
	if(!mqtt_check_json_md5(state, event_data.data, event_data.data_length, md5))
		printf("json information error!!!!!\r\n");
	else 
		printf("json message check sun OK!\r\n");
	
	mqtt_aes_buffer(0, event_data.data, event_data.data_length);
	printf("length = %d, data=%s\r\n", event_data.data_length,  event_data.data);
	
	parse_json(event_data.data);
*/	
}

/* Publish the specified message */
int mqtt_publish_with_length(mqtt_state_t *state, const char* topic, const char* data, int data_length, int qos, int retain)
{
	//char md5[LEN_MD5];
	
	//state->pending_msg_id = 0x0a;//jzyang 没有作用的。
  state->outbound_message = mqtt_msg_publish(&state->mqtt_connection, 
                                                 topic, data, data_length, 
                                                 qos, retain,
                                                 &state->pending_msg_id);
	
	/* md5 所填充的JSON信息 
	cal_md5(state->outbound_message->data + state->outbound_message->length - data_length, 
		data_length - LEN_MD5, md5);
	

	mqtt_aes_buffer(1, md5, LEN_MD5);
	
	memcpy(state->outbound_message->data + state->outbound_message->length - LEN_MD5, md5, LEN_MD5);
	*/
  state->mqtt_flags &= ~MQTT_FLAG_READY;
  state->pending_msg_type = MQTT_MSG_TYPE_PUBLISH;
	//printf("mqtt: sending publish...data len=%d\r\n", state->outbound_message->length);
	make_command_to_list(ATMQTT, ONE_SECOND/20, MQTT_MSG_TYPE_PUBLISH);
	//make_command_to_list(ATMIPPUSH, ONE_SECOND/40, -1);
  return 0;
}

/*retain 标志只用于publish 消息，当为1时*/
static int mqtt_publish(mqtt_state_t *state, const char* topic, char* data, int qos, int retain)
{
	//char buf[512];	
	int len = strlen(data);
/*
	if( len%AES_DECODE_LEN != 0) {//16 0r 17
		len += 16 - (strlen(data)%AES_DECODE_LEN);
		//memset(buf + strlen(data), 0, len);
	}
	printf("len = %d\r\n",len);//96
	
	//memset(buf, '\0', sizeof(buf));
	//memcpy(buf, data, strlen(data));	
	printf("strlen(data) = %d\r\n",strlen(data));//81			

	//mqtt_aes_buffer(1, data, len);
*/
	//printf(" strlen(data) = %d, len = %d\r\n", strlen(data), len);
  return mqtt_publish_with_length(state, topic, data, data != NULL ? len/* + LEN_MD5*/: 0, qos, retain);
}

static void mqtt_subscribe(mqtt_state_t *state)
{
	state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
	make_command_to_list(ATMQTT, ONE_SECOND/5, MQTT_MSG_TYPE_SUBSCRIBE);
	make_command_to_list(ATMIPPUSH, ONE_SECOND/40, -1);	
}

static void parse_mqtt_packet(mqtt_state_t *state, int nbytes)
{
	uint8_t msg_type;
	uint8_t msg_qos;
	uint16_t msg_id;	
	int over_len = 0;

	state->in_buffer_length = nbytes;
	state->message_length_read = nbytes;
	state->message_length = mqtt_get_total_length(state->in_buffer, state->message_length_read);

	msg_type = mqtt_get_type(state->in_buffer);
	msg_qos  = mqtt_get_qos(state->in_buffer);
	msg_id	 = mqtt_get_id(state->in_buffer, state->in_buffer_length);
	
	printf("[%s] come! msg_type=%d,QOS=%d,msg_id=0x%02X, mesg_len=%d,nbytes=%d\r\n", 
						mqtt_get_name((int)msg_type), msg_type, 
						msg_qos, msg_id, state->message_length, nbytes);
	
	over_len = nbytes - state->message_length;
						
	/*检验包的完整性*/
	if(over_len <0) 
	{
		in_buffer_pos = nbytes;
		waitting_len = state->message_length - nbytes;
		printf("%s: pack ERROR! waitting_num = %d, in_buffer_pos=%d\r\n",
							__func__, waitting_len, in_buffer_pos);
		
		if(nbytes < 750)
		{
			in_buffer_pos = 0;
			waitting_len = 0;
		}
		if(waitting_len > 750)
		{
			waitting_len = -waitting_len;
			in_buffer_pos = 0;
			printf("%s: OUT memory! waitting_num = %d, in_buffer_pos=%d\r\n",
							__func__, waitting_len, in_buffer_pos);
		}
		return;
	} 
	else if((over_len > 0)) 
	{
		/*有可能包含2个或者更多的包*/
		if(mqtt_get_total_length(state->in_buffer+state->message_length, over_len) <= over_len) 
		{
			printf("Attention new packet appending!\r\n");
		} 
		else 
		{
			printf("ERROR: Attention check new packet appending fail! usart error!\r\n");
			over_len = 0;
		}
	}
	else if(over_len == 0)
	{
		waitting_len = 0;
		in_buffer_pos = 0;
	}

	switch(msg_type)
	{
		case MQTT_MSG_TYPE_CONNACK:
			
			if(state->in_buffer[nbytes-1] == 0x00) 
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
			
		 /*对于SUBACK的msg_id必须比较是否跟发出去的MQTT_MSG_TYPE_SUBSCRIBE msg_id一致！*/
		case MQTT_MSG_TYPE_SUBACK:

			if(state->pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_SUBSCRIBED, msg_id);
			
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_SUBSCRIBE, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_UNSUBACK:

			if(state->pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_UNSUBSCRIBED, msg_id);
			
			break;
		
		/*对于收到的PUBLISH消息，不用理会msg_id的值，只需回传给服务器就可以*/
		case MQTT_MSG_TYPE_PUBLISH:
			//msg_type=3, msg_qos=1, msg_id=0x19   订阅消息时如果没有指定qos=0跟qos=1的区别。
			//msg_type=3, msg_qos=0, msg_id=0x00    两个消息在public 都指定了qos=1
			mqtt_dev->pub_in_num++ ;
		
			if(msg_qos == 1) 
			{
				state->outbound_message = mqtt_msg_puback(&state->mqtt_connection, msg_id);
				make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBACK);		
			} 
			else if(msg_qos == 2) 
			{
				state->outbound_message = mqtt_msg_pubrec(&state->mqtt_connection, msg_id);
				make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREC);
			}
			
			deliver_publish(state, state->in_buffer, state->message_length_read);
			
			if(over_len) 
			{
				memmove(mqtt_dev->in_buffer, mqtt_dev->in_buffer+state->message_length, over_len);
				printf("ENTER Parse_mqtt_packet start...\r\n");
				parse_mqtt_packet(mqtt_dev->mqtt_state, over_len);
				printf("ENTER Parse_mqtt_packet end...\r\n");
			}
			
			break;
			
		/*当收到MQTT_MSG_TYPE_PUBACK时，msg_id必须与send MQTT_MSG_TYPE_PUBLISH消息
			时的msg_id进行比较！*/	
		case MQTT_MSG_TYPE_PUBACK:
			
			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBLISH, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_PUBREC:
			
			state->outbound_message = mqtt_msg_pubrel(&state->mqtt_connection, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBREL);

			break;
		
		case MQTT_MSG_TYPE_PUBREL:

			state->outbound_message = mqtt_msg_pubcomp(&state->mqtt_connection, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBCOMP);
		
			break;
		
		case MQTT_MSG_TYPE_PUBCOMP:

			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_PINGREQ:

			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PINGRESP);
			make_command_to_list(ATMIPPUSH, ONE_SECOND/30, -1);	
		
			break;
		
		case MQTT_MSG_TYPE_PINGRESP:

			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PINGREQ, 0);
		
			break;
		
		case MQTT_DEV_STATUS_DISCONNECT:

			mqtt_dev->connect_status = MQTT_DEV_STATUS_DISCONNECT;
		
			break;
	}
		
}

static void parse_mqtt_packet1(mqtt_state_t *state, int nbytes)
{
	uint8_t msg_type;
	uint8_t msg_qos;
	uint16_t msg_id;	
	int over_len = 0;

	//state->in_buffer_length = nbytes;
	state->message_length_read = nbytes;
	state->message_length = mqtt_get_total_length(state->in_buffer, state->message_length_read);

	msg_type = mqtt_get_type(state->in_buffer);
	msg_qos  = mqtt_get_qos(state->in_buffer);
	msg_id	 = mqtt_get_id(state->in_buffer, state->in_buffer_length);
	
	printf("[%s] come! msg_type=%d, QOS=%d, msg_id=0x%02X, mesg_len=%d, nbytes=%d\r\n", 
						mqtt_get_name((int)msg_type), msg_type, 
						msg_qos, msg_id, state->message_length, nbytes);
	
	over_len = nbytes - state->message_length;
						
	/*检验包的完整性*/
	if(over_len <0) 
	{
		printf("%s: pack ERROR!\r\n", __func__);
		return;
	} 
	else if((over_len > 0)) 
	{
		/*有可能包含2个或者更多的包*/
		if(mqtt_get_total_length(state->in_buffer+state->message_length, over_len) <= over_len) 
		{
			printf("Attention new packet appending!\r\n");
		} 
		else 
		{
			printf("ERROR: Attention check new packet appending fail! usart error!\r\n");
			over_len = 0;
		}
	}

	switch(msg_type)
	{
		case MQTT_MSG_TYPE_CONNACK:
			
			if(state->in_buffer[nbytes-1] == 0x00) 
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
			
		 /*对于SUBACK的msg_id必须比较是否跟发出去的MQTT_MSG_TYPE_SUBSCRIBE msg_id一致！*/
		case MQTT_MSG_TYPE_SUBACK:

			if(state->pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_SUBSCRIBED, msg_id);
			
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_SUBSCRIBE, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_UNSUBACK:

			if(state->pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_UNSUBSCRIBED, msg_id);
			
			break;
		
		/*对于收到的PUBLISH消息，不用理会msg_id的值，只需回传给服务器就可以*/
		case MQTT_MSG_TYPE_PUBLISH:
			//msg_type=3, msg_qos=1, msg_id=0x19   订阅消息时如果没有指定qos=0跟qos=1的区别。
			//msg_type=3, msg_qos=0, msg_id=0x00    两个消息在public 都指定了qos=1
			mqtt_dev->pub_in_num++ ;
		
			if(msg_qos == 1) 
			{
				state->outbound_message = mqtt_msg_puback(&state->mqtt_connection, msg_id);
				make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBACK);		
			} 
			else if(msg_qos == 2) 
			{
				state->outbound_message = mqtt_msg_pubrec(&state->mqtt_connection, msg_id);
				make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREC);
			}
			
			deliver_publish(state, state->in_buffer, state->message_length_read);
			
			if(over_len) 
			{
				memmove(mqtt_dev->in_buffer, mqtt_dev->in_buffer+state->message_length, over_len);
				printf("ENTER Parse_mqtt_packet start...\r\n");
				parse_mqtt_packet(mqtt_dev->mqtt_state, over_len);
				printf("ENTER Parse_mqtt_packet end...\r\n");
			}
			
			break;
			
		/*当收到MQTT_MSG_TYPE_PUBACK时，msg_id必须与send MQTT_MSG_TYPE_PUBLISH消息
			时的msg_id进行比较！*/	
		case MQTT_MSG_TYPE_PUBACK:
			
			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			
			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PUBLISH, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_PUBREC:
			
			state->outbound_message = mqtt_msg_pubrel(&state->mqtt_connection, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBREL);

			break;
		
		case MQTT_MSG_TYPE_PUBREL:

			state->outbound_message = mqtt_msg_pubcomp(&state->mqtt_connection, msg_id);
			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PUBCOMP);
		
			break;
		
		case MQTT_MSG_TYPE_PUBCOMP:

			if(state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id)
				complete_pending(state, MQTT_EVENT_TYPE_PUBLISHED, msg_id);
			
			break;
			
		case MQTT_MSG_TYPE_PINGREQ:

			make_command_to_list(ATMQTT, ONE_SECOND/30, MQTT_MSG_TYPE_PINGRESP);
			make_command_to_list(ATMIPPUSH, ONE_SECOND/30, -1);	
		
			break;
		
		case MQTT_MSG_TYPE_PINGRESP:

			mqtt_set_mesg_ack(MQTT_MSG_TYPE_PINGREQ, 0);
			
			break;
		
		case MQTT_DEV_STATUS_DISCONNECT:

			mqtt_dev->connect_status = MQTT_DEV_STATUS_DISCONNECT;
		
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

/*如果为mqtt消息头，此函数解析！*/
void check_packet_by_fixhead(char *fixhead, uint8_t *read, int nbytes)
{
	int msg_len;
	
	msg_len = mqtt_get_total_length(read, nbytes);
	printf("%s: nbytes = %d, msg_len=%d\r\n", __func__, nbytes, msg_len);
	
  if(msg_len == nbytes)//most 750 = 750 
	{
		printf("%s: msg_len == nbytes = %d\r\n", __func__, nbytes);
		mqtt_dev->in_pos = 0;
		mqtt_dev->in_waitting = 0;		
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
		memcpy(mqtt_dev->in_buffer, read, msg_len);
		parse_mqtt_packet1(mqtt_dev->mqtt_state, nbytes);
	}
	else if(msg_len < nbytes)//150 < 200
	{
		printf("%s: [%d] msg_len < nbytes [%d]\r\n", __func__, msg_len, nbytes);		
		mqtt_dev->in_waitting = 0;	
		mqtt_dev->in_pos = 0;		
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
		memcpy(mqtt_dev->in_buffer, read, msg_len);
		parse_mqtt_packet1(mqtt_dev->mqtt_state, msg_len);
		memmove(read, read+msg_len, nbytes-msg_len);
		*fixhead = 1;		
		check_packet_by_fixhead(fixhead, read, nbytes-msg_len);
	}
	else if(msg_len <= sizeof(mqtt_dev->in_buffer))//msg_len > nbytes  1500 > 1300 > 750
	{
		printf("%s: [%d] msg_len <= sizeof(mqtt_dev->in_buffer)[%d]\r\n", 
							__func__, msg_len, sizeof(mqtt_dev->in_buffer));			
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

static unsigned char read[750];

void on_remote_command_callback1(RemoteTokenizer *tzer, Token* tok)
{
	int i, nbytes;

	memset(read, '\0', sizeof(read));
	nbytes = str2int(tok[1].p, tok[1].end);//nbytes <= 750
	
	for(i=0; i<nbytes; i++)
	{
		read[i] = str_to_hex((char*)tok[2].p+2*i);
	}	
	
	if(mqtt_dev->fixhead)
	{
		check_packet_by_fixhead(&(mqtt_dev->fixhead), read, nbytes);
	}
	else 
	{
		if(mqtt_dev->in_waitting > 0 && mqtt_dev->in_waitting <= 750)
		{
			if(mqtt_dev->in_waitting == nbytes)
			{
				printf("%s: in_waitting == nbytes = %d\r\n", __func__, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, read, nbytes);
				parse_mqtt_packet1(mqtt_dev->mqtt_state, mqtt_dev->in_pos+mqtt_dev->in_waitting);
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;
			}
			else if(mqtt_dev->in_waitting < nbytes)
			{
				printf("%s: [%d] in_waitting < nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, read, mqtt_dev->in_waitting);
				parse_mqtt_packet1(mqtt_dev->mqtt_state, mqtt_dev->in_pos+mqtt_dev->in_waitting);
				memmove(read, read+mqtt_dev->in_waitting, nbytes-mqtt_dev->in_waitting);
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;				
				check_packet_by_fixhead(&(mqtt_dev->fixhead), read, nbytes-mqtt_dev->in_waitting);
			}
			else if(mqtt_dev->in_waitting > nbytes)
			{
				printf("%s: [%d] in_waitting > nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes);
				memcpy(mqtt_dev->in_buffer+mqtt_dev->in_pos, read, nbytes);
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
				memmove(read, read+(nbytes-len), len);
				printf("%s: Drop some char in packet! len = %d, nbytes=%d\r\n", __func__, len, nbytes);
				check_packet_by_fixhead(&(mqtt_dev->fixhead), read, mqtt_dev->in_waitting);
			}
		}
		else if(mqtt_dev->in_waitting > 750)
		{
			printf("%s: mqtt_dev->in_waitting=%d ERROR!\r\n", __func__, mqtt_dev->in_waitting);
		}
	}
}

void on_remote_command_callback(RemoteTokenizer *tzer, Token* tok)
{
	int i, nbytes;

	nbytes = str2int(tok[1].p, tok[1].end);
	
	if(waitting_len == 0)
	{
		in_buffer_pos = 0;
		memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
	}

	if(waitting_len < 0)
	{
		in_buffer_pos = 0;
		waitting_len += nbytes;
		printf("%s: waitting_len = %d.\r\n", __func__, waitting_len);
		
		if(waitting_len <= 0) 
		{
			printf("%s: drop packet!\r\n", __func__);
			return;
		}
		else
		{
			memset(mqtt_dev->in_buffer, '\0', sizeof(mqtt_dev->in_buffer));
			printf("%s: contain usefull packet. len=%d\r\n", __func__, waitting_len);
			for(i=0; i<waitting_len; i++)
			{
				mqtt_dev->in_buffer[i] = str_to_hex((char*)(tok[2].p+2*(nbytes-waitting_len))+2*i);
				parse_mqtt_packet(mqtt_dev->mqtt_state, waitting_len);
				return;
			}			
		}
	}

	
	for(i=0; i<nbytes; i++)
	{
		if(i+in_buffer_pos >= sizeof(mqtt_dev->in_buffer))
		{
			printf("%s: packet next! nbytes=%d, waitting_len=%d, in_buffer_pos=%d\r\n", 
								__func__, nbytes, waitting_len, in_buffer_pos);
			nbytes -= waitting_len;
			waitting_len = 0;
			in_buffer_pos = 0;
			printf("%s: packet next! nbytes = %d\r\n", __func__, nbytes);
		}
		if(waitting_len)
			mqtt_dev->in_buffer[i+in_buffer_pos] = str_to_hex((char*)tok[2].p+2*i);
		else 
			mqtt_dev->in_buffer[i] = str_to_hex((char*)tok[2].p+2*i);
	}

	if(waitting_len) 
	{
		waitting_len = 0;
		nbytes += in_buffer_pos;
	}
	
	parse_mqtt_packet(mqtt_dev->mqtt_state, nbytes);
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

static void mqtt_msg_set_clean(void);

void on_disconnect_service_callback(RemoteTokenizer *tzer, Token* tok)
{
	//+MIPCLOSE:2
	int socketId = str2int(tok[0].p+10, tok[0].end);
	
	printf("%s: +MIPCLOSE !!!!!\r\n", __func__);
	dev->socket_open[socketId-1] = -1;
	mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
	waitting_len = 0;
	mqtt_dev->in_pos = 0;
	mqtt_dev->in_waitting = 0;	
	mqtt_dev->fixhead = 1;	
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
		printf("----------------------------------\r\n");
		printf("[result: OK] %s\r\n", dev[0].at_sending);
		
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) 
	{
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		/*此命令是短信查询，不处理*/
		printf("[result: OK] %s\r\n", dev->at_sending);		

		/*删除短信成功之后才能读取下一条短信，如果不成功
		，可能永远都不会再读取，在at fail中也处理fix it.*/
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
			/*如果数组中有删除的序列号，才进行删除。*/
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
			for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
			{
				if(dev->sm_index[i] != -1)
					printf("%d, ", dev->sm_index[i]);
			}
			printf("}\r\n");				
		}
		/*删除短信成功，读取下一条短信*/

		dev->sm_index_delete = -1;
		dev->sm_delete_flag = 1;
		dev->sm_read_flag = 1;		
	}
}

void on_at_cmd_fail_callback(RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) 
	{
		dev->socket_close_flag = 1;
		printf("[AT:%s, result: ERROR]\r\n", dev->at_sending);
	} 
	else if(!memcmp(dev->at_sending, "AT+MIPOPEN=1,0,\"", strlen("AT+MIPOPEN=1,0,\""))) 
	{
		//AT+MIPOPEN=1,0,"
		printf("[TCP CONNECT FAIL! AT: %s", dev->at_sending);
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGD=", strlen("AT+CMGD=")) ) 
	{
		if( !memcmp(dev->at_sending, "AT+CMGD=?", strlen("AT+CMGD=?")) ) return;
		printf("[AT:%s, result: ERROR]\r\n", dev->at_sending);
		dev->sm_delete_flag = 1;
		/*重新执行删除短信工作*/
		dev->sm_read_flag = 1;
	} 
	else if( !memcmp(dev->at_sending, "AT+CMGR=", strlen("AT+CMGR=")) ) 
	{
		/*读失败，重新读*/
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
		for(; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
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
		
		for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
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
	for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
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
	/*只可读出一行，因此，短信内容不能有\n分行！*/
	dev->sm_index_delete = index;
	dev->sm_delete_flag = 1;
	/*读取到短信内容，删除之*/
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
			/*重新排序*/
		}
		
		printf("sm_num=%d, sm_index={ ", dev->sm_num);
		for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
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
		当发送了错误的短信index时发生
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
	print_line(UART4, r->in, r->pos);//jzyang
	/*打印4G的所有串口消息, release 版本时去掉。*/
	
	longsung_tokenizer_init(tzer, r->in, r->in+r->pos);
	
	if(tzer->count == 0) return;
	
	tok = (Token*) mymalloc(0, (tzer->count*sizeof(Token)));
	if(tok == NULL) 
	{
		printf("%s: count=%d, mymalloc error!\r\n", __func__, tzer->count);
		return;
	}
	
	for(i=0; i<tzer->count; i++) 
	{
		tok[i] = longsung_tokenizer_get(tzer, i);
	}
	
	if(dev->sm_data_flag) 
	{
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
	if (r->overflow) 
	{
		r->overflow = (c != '\n');
		return;
	}

	if (r->pos >= (int) sizeof(r->in)) 
	{
		r->overflow = 1;
		r->pos = 0;
		printf("%s: over flow!\r\n", __func__);
		return;
	}	

	/*必须要大于1550左右*/
	r->in[r->pos] = (char)c;
	r->pos += 1;

	if(c == '\n' /*|| c == '\0'*/) 
	{
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

	if(!mAndroidPower) dev->mqtt_alive = 1;
	
	for(; i<2; i++) 
	{	
		while(kfifo_len(uart3_fifo) > 0) 
		{
			if(j++%20==0) IWDG_Feed();	
			kfifo_get(uart3_fifo, &ch, 1);
			
			if(dev->mqtt_alive)
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
	
	reader->on_command = on_remote_command_callback1;
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
	
	memset(dev, '\0', sizeof(mqtt_dev_status));
	
	dev->connect_info.client_id = "jzyang";
	dev->connect_info.username = NULL;//"yang";//NULL 
	dev->connect_info.password = NULL;//"zhou";// NULL
	dev->connect_info.will_topic = "mcu";
	dev->connect_info.will_message = "death";
	dev->connect_info.keepalive = 400;
	dev->connect_info.will_qos = 1;
	dev->connect_info.will_retain = 0;
	dev->connect_info.clean_session = 1;

	/* The list of topics that we want to subscribe to
	static const char* topics[] =
	{
		"sensor", "qing", "xiaopeng", "car", "zhang", "linux", NULL
	};	
	*/
	printf("%s\r\n", __func__);

	/* Initialise the MQTT client*/
	mqtt_init(dev->mqtt_state, mqtt_dev->in_buffer, sizeof(mqtt_dev->in_buffer),
						mqtt_dev->out_buffer, sizeof(mqtt_dev->out_buffer));	
	
	dev->mqtt_state->connect_info = &(dev->connect_info);

	/* Initialise and send CONNECT message */
	mqtt_msg_init(&(dev->mqtt_state->mqtt_connection), dev->mqtt_state->out_buffer,
			dev->mqtt_state->out_buffer_length);

	dev->connect_status = MQTT_DEV_STATUS_NULL;
	
	mqtt_dev->pub_in_num = 0;
	
	mqtt_dev->pub_out_num = 0;

	mqtt_dev->in_pos = 0;
	
	mqtt_dev->in_waitting = 0;	

	mqtt_dev->fixhead = 1;
	
	for(i=0; i<OUT_DATA_LEN_MAX; i++) 
	{
		dev->mqtt_state->outdata[i] = NULL;
	}
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
	dev->atcmd = NULL;
	dev->close_tcp_interval = 0;
	dev->clean_interval = 3*ONE_SECOND/2;

	if(flag) 
	{
		dev->reset_request = 1;
		dev->is_inited = 1;
		dev->mqtt_alive = 0;		
	} 
	else 
	{
		dev->reset_request = 0;
		dev->is_inited = 0;
		dev->mqtt_alive = 1;
	}
	
	for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) 
	{
		dev->socket_open[i] = -1;
	}
	
	for(i=0; i<sizeof(dev->sm_index)/sizeof(dev->sm_index[0]); i++) 
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
	
	at(cmd);
}

/***************************************************************
把数据加入到链表中去
****************************************************************/
static void add_cmd_to_list(AtCommand *cmd, struct list_head *head)
{
	if(cmd)
		list_add_tail(&cmd->list, head);
}

/***************************************************************
如果此数据可以在链表中找到，从链表中删除这个数据
****************************************************************/
static void del_cmd_from_list(AtCommand *cmd, struct list_head *head)
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
如果链表里面没有数据，则返回NULL，如果有数据就返回一个数据
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

static int check_command_exist(int index)
{
	int ret = 0;
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &dev->at_head)
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

/*
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
*/
static char * make_packet(char* buff, FrameHead* fh, DataHead* dh, char json[])
{
	char *p;	
	int i, len = 0;
	char data[256];
	
	memset(data, '\0', sizeof(data));
	
	if(!fh) return NULL;

	if(dh) {
		dh->rpcid = htonl(dh->rpcid);
		dh->jsonlen = htonl(dh->jsonlen);
		memcpy(data+sizeof(FrameHead), dh, sizeof(DataHead));
		len += sizeof(DataHead);
		fh->datasize = sizeof(DataHead);
	}	

	if(json) {
		memcpy(data+sizeof(FrameHead)+sizeof(DataHead), json, strlen(json));
		len += strlen(json);
		fh->datasize += strlen(json);
	}

	len += sizeof(FrameHead);
	fh->datasize = htonl(fh->datasize);
	memcpy(data, fh, sizeof(FrameHead));
	
	strcpy(buff, "AT+MIPSEND=1,\"");
	p = buff+strlen("AT+MIPSEND=1,\"");

	if(json)
		printf("len=%d, strlen(json)=%d, DATA:[%s]\r\n", len, strlen(json), json);
	printf("----------------------------------------------\r\n");
	
	for(i=0; i<len; i++) {
		sprintf(p+2*i, "%02x", *(data+i));
	}
	p[2*i] = '\"';

	printf("strlen(buff)=%d, DATA:[%s]\r\n", strlen(buff), buff);
	printf("----------------------------------------------\r\n");
	return buff;
}

static char* make_mqtt_packet(char* buff, unsigned char* data, int len)
{
	char *p;	
	int i;

	if(!data || !buff) return NULL;
	
	strcpy(buff, "AT+MIPSEND=1,\"");
	p = buff+strlen("AT+MIPSEND=1,\"");
	
	for(i=0; i<len; i++) {
		sprintf(p+2*i, "%02x", *(data+i));
	}
	p[2*i] = '\"';

	//printf("strlen(buff)=%d\r\n", strlen(buff));
	//printf("----------------------------------------------\r\n");
	return buff;
}

static void prepare_tick_packet(void)
{
	char buff[300];
	FrameHead fh[1];
	DataHead dh[1];
	char json[] = "{\"name\": \"yangjianzhou\",\"format\":{\"type\":\"rect\",\"width\":19200,\"interlace\":true}}";
	
	memset(fh, '\0', sizeof(FrameHead));
	memset(dh, '\0', sizeof(DataHead));
	memset(buff, '\0', sizeof(buff));
	
	fh->ver = 0x01;
	fh->compress_encrypt = 0x11;
	fh->frametype = 0x03;
	fh->servicetype = 0x01;
	fh->frameinfo = 0x00;
	fh->sessionid = 0x00;
	fh->frameid = 0x00;
	fh->tmp = 0xff;
	fh->datasize = 0x00;
	fh->revert[0] = 0xff;
	fh->revert[1] = 0xff;
	fh->revert[2] = 0xff;
	fh->revert[3] = 0xff;
	
	dh->rpc = 0x01;
	dh->rpcmethrod[0] = 0x01;
	dh->rpcmethrod[1] = 0x02;
	dh->rpcmethrod[2] = 0x03;
	dh->rpcid = 0x11223344;
	dh->jsonlen = strlen(json);
	
	make_packet(buff, fh, dh, json);
	//make_packet(buff, fh, NULL, NULL);
	at(buff);
}

static void mqtt_disconnect_server(mqtt_state_t *state)
{
	char buff[128];

	printf("%s.\r\n", __func__);
	memset(buff, '\0', sizeof(buff));
	state->outbound_message = mqtt_msg_disconnect(&state->mqtt_connection);
	
	if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
	{
		delay_ms(15);
		at(buff);
		delay_ms(15);
		at("AT+MIPPUSH=1");
		delay_ms(100);
	}		
}

static void prepare_mqtt_packet(mqtt_state_t *state, AtCommand* cmd)
{
	char buff[512];
	
	memset(buff, '\0', sizeof(buff));
	
	switch(cmd->para) 
	{
		case MQTT_MSG_TYPE_PINGRESP:
			state->outbound_message = mqtt_msg_pingresp(&state->mqtt_connection);
			cmd->mqtype = MQTT_MSG_TYPE_PINGRESP;
		
			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}		
		  break;
		
		case MQTT_MSG_TYPE_CONNECT:
			state->outbound_message =  mqtt_msg_connect(&state->mqtt_connection, state->connect_info);
			cmd->mqtype = MQTT_MSG_TYPE_CONNECT;
			cmd->msgid = 0;
			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}
			break;
		
		case MQTT_MSG_TYPE_DISCONNECT:
			state->outbound_message =  mqtt_msg_disconnect(&state->mqtt_connection);
			cmd->mqtype = MQTT_MSG_TYPE_DISCONNECT;
		
			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}		
			break;
			
		case MQTT_MSG_TYPE_PINGREQ: 
			state->outbound_message = mqtt_msg_pingreq(&state->mqtt_connection);
			cmd->mqtype = MQTT_MSG_TYPE_PINGREQ;
			cmd->msgid = 0;
		
			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}		
			break;

/*			
		case MQTT_OUTDATA_SUBSCRIBE_TEST:
		  state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
			state->outbound_message = mqtt_msg_subscribe(&state->mqtt_connection, 
                                                   "sensor", 1, 
                                                   &state->pending_msg_id);
			cmd->msgid = state->pending_msg_id;	
		  cmd->mqtype = MQTT_MSG_TYPE_SUBSCRIBE;
		
			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}					
			break;
*/			
		case MQTT_MSG_TYPE_SUBSCRIBE:
		  state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
			state->outbound_message = mqtt_msg_subscribe(&state->mqtt_connection, 
                                                   "/system/sensor", 1, //此处的QOS关系收到PUBLISH消息的qos
                                                   &state->pending_msg_id);
			cmd->msgid = state->pending_msg_id;		
			cmd->mqtype = MQTT_MSG_TYPE_SUBSCRIBE;

			if(make_mqtt_packet(buff, state->outbound_message->data, state->outbound_message->length))
			{
				at(buff);
			}		
			break;
	
		case MQTT_MSG_TYPE_PUBLISH:
		case MQTT_MSG_TYPE_PUBCOMP:
		case MQTT_MSG_TYPE_PUBREL:
		case MQTT_MSG_TYPE_PUBREC:
		case MQTT_MSG_TYPE_PUBACK://40head 02len 0017msgid			

			if(cmd->mqttdata) 
			{
				if(make_mqtt_packet(buff, cmd->mqttdata, cmd->mqttdatalen))
				{
					at(buff);
				}
			} 
			else 
			{
				printf("ERROR: %s mqttdata=NULL!\r\n", __func__);
			}				
			break;
			
		default: break;
	}
	
	printf("SEND [%s] mqtype=%d,id=0x%04x\r\n", mqtt_get_name(cmd->mqtype), cmd->mqtype, cmd->msgid);	
}

static void send_command_to_device(AtCommand* cmd)
{
	if(cmd == NULL) return;
	
	//printf("%s: index=%d\r\n", __func__, cmd->index);

	switch(cmd->index)
	{
		case ATCSQ: at("AT+CSQ"); break;
		case ATSIMTEST: at("AT+SIMTEST?"); break;
		case ATCPMS: at("AT+CPMS=\"SM\""); break;
		case ATCMGD_: at("AT+CMGD=?"); break;
		case ATMIPCALL_: at("AT+MIPCALL?"); break;
		case ATMIPCALL0: at("AT+MIPCALL=0"); break;
		case ATMIPPROFILE: at("AT+MIPPROFILE=1,\"3GNET\""); break;//3GNET
		case ATMIPCALL1: at("AT+MIPCALL=1"); break;
		case ATMIPOPEN: at("AT+MIPOPEN=1,0,\"iot.eclipse.org\",1883,0"); break;
		/*at("AT+MIPOPEN=1,0,\"112.124.102.62\",13334,0");iot.eclipse.org*/
		case ATMIPSEND: prepare_tick_packet(); break;
		case ATMIPPUSH: at("AT+MIPPUSH=1"); break;
		case ATCMGF: at("AT+CMGF=1"); break;
		case ATMIPCLOSE: do_close_socket(cmd->para); break;		
		case ATCMGR: do_read_sm(cmd->para); break;
		case ATCMGD: do_delete_sm(cmd->para); break;	
		case ATMIPHEX: at("AT+MIPHEX=1"); break;
		case ATMQTT: prepare_mqtt_packet(mqtt_dev->mqtt_state, cmd); break;
		default: break;
	}
}

static void make_command_to_list(char index, long long interval, int para)
{
  int i, total = 0;	
	AtCommand *cmd = NULL;	
	
	cmd = (AtCommand *)mymalloc(0, sizeof(AtCommand));
	
	if(cmd != NULL) 
	{
		cmd->mqtype = -1;
		cmd->index = index;
		cmd->interval = interval;
		cmd->para = para;
		cmd->try = 0;
		cmd->clean = 0;
		
		if(index == ATMQTT) 
		{
			cmd->mqttack = 0;
			cmd->mqttdata = NULL;
		
			switch(para) 
			{
				case MQTT_MSG_TYPE_PUBLISH:
				case MQTT_MSG_TYPE_SUBSCRIBE:
				case MQTT_MSG_TYPE_PUBCOMP:
				case MQTT_MSG_TYPE_PUBREL:
				case MQTT_MSG_TYPE_PUBREC:
				case MQTT_MSG_TYPE_PUBACK:
				case MQTT_MSG_TYPE_PINGREQ:
				case MQTT_MSG_TYPE_DISCONNECT:
				case MQTT_MSG_TYPE_CONNECT: cmd->mqtype = para; break;
				default: printf("%s: mqtt para error!\r\n", __func__); break;
			}
			
			/*需要malloc的命令！*/
			if(para == MQTT_MSG_TYPE_PUBLISH || para == MQTT_MSG_TYPE_PUBCOMP || 
					para == MQTT_MSG_TYPE_PUBREL || para == MQTT_MSG_TYPE_PUBREC || 
					para == MQTT_MSG_TYPE_PUBACK) 
			{	
				for(i=0; i<OUT_DATA_LEN_MAX; i++)
				{
					if(mqtt_dev->mqtt_state->outdata[i] == NULL)
						break;
				}	
				
				if(i >= OUT_DATA_LEN_MAX) 
				{
					cmd->mqttdata = NULL;
					myfree(0, cmd);
					printf("\r\n");
					printf("ERROR: i=%d +++++++++++++OUT_DATA_LEN_MAX +++++++++++\r\n", i);
					printf("\r\n");
					return;
				} 
				else 
				{
					cmd->mqttdatalen = mqtt_dev->mqtt_state->outbound_message->length;
					mqtt_dev->mqtt_state->outdata[i] = mymalloc(0, mqtt_dev->mqtt_state->outbound_message->length);
					
					if(!mqtt_dev->mqtt_state->outdata[i]) 
					{
						printf("\r\n");
						printf("ERROR: malloc for MQTT [%s] fail!\r\n", mqtt_get_name(cmd->mqtype));
						printf("\r\n");
						cmd->mqttdata = NULL;
						myfree(0, cmd);
						return;
					} 
					else 
					{
						memcpy(mqtt_dev->mqtt_state->outdata[i], mqtt_dev->mqtt_state->outbound_message->data, cmd->mqttdatalen);
						cmd->mqttdata = mqtt_dev->mqtt_state->outdata[i];
						cmd->msgid = mqtt_get_id(mqtt_dev->mqtt_state->outbound_message->data, 
																				mqtt_dev->mqtt_state->outbound_message->length);
						
						for(i=0; i<OUT_DATA_LEN_MAX; i++)
						{		
							/*为了统计malloc的数据个数*/
							if(mqtt_dev->mqtt_state->outdata[i] != NULL) total++;
						}
						
						printf("MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM [total = %d]\r\n", total);						
						printf("MALLOC mqtt[%s] ok.type=%d,id=0X%04X,len=%d,p=%p\r\n", mqtt_get_name(cmd->mqtype), 
											cmd->mqtype, cmd->msgid, cmd->mqttdatalen, cmd->mqttdata);	
						
						add_cmd_to_list(cmd, &dev->at_head);							
						/*申请成功了才加入ATMIPPUSH*/
						make_command_to_list(ATMIPPUSH, ONE_SECOND/40, -1);
						
						if(cmd->mqtype == MQTT_MSG_TYPE_PUBLISH) mqtt_dev->pub_out_num++;
					}
				}
			} 
			else 
			{
				/*没有malloc的走这个通道*/
				add_cmd_to_list(cmd, &dev->at_head);	
				
				if(cmd->mqtype == MQTT_MSG_TYPE_CONNECT || cmd->mqtype == MQTT_MSG_TYPE_PINGREQ) 
					cmd->msgid = 0;
				printf("%s: ADD [%s] to list success. mqtype=%d,msg_id=0X%04X\r\n", __func__,  
									mqtt_get_name(cmd->mqtype),cmd->mqtype, cmd->msgid);
			}
		} 
		else 
		{
			add_cmd_to_list(cmd, &dev->at_head);	
		}
		
	}
}

static void mqtt_set_mesg_ack(int type, uint16_t msg_id)
{
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	if(dev->atcmd != NULL && dev->atcmd->mqtype == type &&
			dev->atcmd->msgid == msg_id && dev->atcmd->mqttack == 0) 
	{
		dev->atcmd->mqttack = 1;
		printf("%s: type=%d, msgid=0x%04x, [%s] is ACK, REMOVE it!\r\n", __func__, 
							dev->atcmd->mqtype, dev->atcmd->msgid, mqtt_get_name(cmd->mqtype));
		return;
	}
			
	list_for_each_safe(pos, n, &dev->mqtt_head)
	{
		cmd = list_entry(pos, AtCommand, list);
		
		if(cmd != NULL && cmd->mqtype == type && cmd->msgid == msg_id) 
		{
			cmd->mqttack = 1;
			printf("%s:type=%d,msgid=0x%04x, [%s] is ACK, REMOVE it!\r\n", __func__, 
								cmd->mqtype, cmd->msgid , mqtt_get_name(cmd->mqtype));
			//break; delete for add MQTT_MSG_TYPE_PINGREQ in mqtt_head!
		}
		else 
		{
			printf("%s: set [%s] fail! [type=%d, msgid=%d]\r\n", __func__, 
								mqtt_get_name(cmd->mqtype), type, msg_id);
		}
	}		
}

/*删除链表节点，释放AtCommand*/
static void longsung_release_command(AtCommand *cmd, struct list_head *head, struct list_head *pos)
{
	int i, total = 0;
	
	if(head)
		del_cmd_from_list(cmd, head);
	else if(pos)
		list_del(pos);
		  
	if(cmd->mqtype != -1 && cmd->mqttdata != NULL) 
	{	
		for(i=0; i<OUT_DATA_LEN_MAX; i++)
		{
			if(cmd->mqttdata != NULL && mqtt_dev->mqtt_state->outdata[i] == cmd->mqttdata) 
			{
				printf("FREE mqtt[%s] buff ok.type=%d,id=0X%04X,len=%d,p=0x%p\r\n", mqtt_get_name(cmd->mqtype), 
									cmd->mqtype, cmd->msgid, cmd->mqttdatalen, cmd->mqttdata);	
				myfree(0, cmd->mqttdata);
				mqtt_dev->mqtt_state->outdata[i] = NULL;						
				break; //当去掉统计时，要加上break;
			}
		}
		
		for(i=0; i<OUT_DATA_LEN_MAX; i++)
		{		
			/*为了统计malloc的数据个数*/
			if(mqtt_dev->mqtt_state->outdata[i] != NULL) total++;
		}
		
		//printf("FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF[n = %d]\r\n", n);
		printf("YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY [total = %d]\r\n", total);		
	}
	
	if(cmd->mqtype != -1)
			printf("%s: REMOVE [%s] type=%d, msgid=0x%04x!\r\n", __func__, 
								mqtt_get_name(cmd->mqtype), cmd->mqtype, cmd->msgid);			
	
	myfree(0, cmd);			
}

static void release_command_from_list(struct list_head *head)
{
	int total = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	printf("%s start.\r\n", __func__);
	
	list_for_each_safe(pos, n, head)
	{
		total++;
		cmd = list_entry(pos, AtCommand, list);
		longsung_release_command(cmd, NULL, pos);
	}
	
	printf("%s: release %d cmd form %s\r\n", __func__, total, (&(dev->at_head)==head)?"at_head":"mqtt_head");
}

static void mqtt_list_cmd(void)
{
	int i = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &dev->mqtt_head)
	{
		cmd = list_entry(pos, AtCommand, list);
		if(cmd->clean) 
		{
			printf("%s: clean message [%d].\r\n", __func__, i);
			longsung_release_command(cmd, &dev->mqtt_head, NULL);
			continue;
		}
		
		else if(cmd->mqttack == 1) 
			longsung_release_command(cmd, &dev->mqtt_head, NULL);
 
		else if((--cmd->interval < 0)) 
		{ 
			printf("%s: add [%s] to AT LIST HEAD again! type=%d, msgid=0x%04x!\r\n", 
							__func__, mqtt_get_name(cmd->mqtype), cmd->mqtype, cmd->msgid);
			/*从mqtt_head中删除*/
			list_del(pos);
			cmd->interval = ONE_SECOND/20;
	
			/*目前只有MQTT_OUTDATA_PUBLISH才需要检查 mqttdata*/
			if(cmd->mqttdata == NULL && (cmd->mqtype==MQTT_MSG_TYPE_PUBLISH)) 
			{
				myfree(0, cmd);
				printf("ERROR: [%s] mqttdata is NULL remote it from mqtt_head.\r\n", mqtt_get_name(cmd->mqtype));
				continue;
			}
			
			/*超过3次, 断开MQTT的连接*/
			if(cmd->try++ > 3)
			{
				printf("ERROR: [%s try>=3], type=%d,id=0X%04X,len=%d,p=0x%p\r\n", mqtt_get_name(cmd->mqtype), 
									cmd->mqtype, cmd->msgid, cmd->mqttdatalen, cmd->mqttdata);
				/*只要发送DISCONNECT包就可以了*/
				/*然后再断开TCP连接*/
				make_command_to_list(ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_DISCONNECT);
				make_command_to_list(ATMIPPUSH, ONE_SECOND/2, -1);
				dev->close_tcp_interval = 3*ONE_SECOND;
				/*只释放CMD*/
				longsung_release_command(cmd, NULL, NULL);
				continue;
			}
			
			/*目前根据协议只有PUBLISH、SUBSCRIBE用得到，后续吸添加*/
			if(mqtt_get_qos(cmd->mqttdata) && (cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ||
						cmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE))
					mqtt_set_dup(cmd->mqttdata);
			
			add_cmd_to_list(cmd, &dev->at_head);
			make_command_to_list(ATMIPPUSH, ONE_SECOND/20, -1);
		} 	
	}
	
}

static void at_list_cmd(void)
{
	/*获取要发送的AT指令，并发送*/
	if(dev->atcmd == NULL) 
	{
		dev->atcmd = get_cmd_from_list(&dev->at_head);
		
		if(dev->atcmd && dev->atcmd->clean) 
		{
			printf("%s: clean\r\n", __func__);
			longsung_release_command(dev->atcmd, &dev->at_head, NULL);
			dev->atcmd = NULL;		
		} 
		else if(dev->atcmd) 
		{
			send_command_to_device(dev->atcmd);
		}
	} 
	else 
  {
		if(dev->atcmd->clean) 
		{
			printf("%s: clean\r\n", __func__);
			longsung_release_command(dev->atcmd, &dev->at_head, NULL);
			dev->atcmd = NULL;
			return;
		}
		
		if(dev->atcmd->interval > 0) 
		{
			dev->atcmd->interval--;
		} 
		else 
		{
			del_cmd_from_list(dev->atcmd, &dev->at_head);
			
			/*目前只有两个消息需要检查回复, 新加入MQTT_DEV_STATUS_CONNECT*/
			if((dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBLISH ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_CONNECT ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_PINGREQ) && 
						(dev->atcmd->mqttack == 0)) 
			{
				/*wait for 5 second*/
				dev->atcmd->interval = 5*ONE_SECOND;
				add_cmd_to_list(dev->atcmd, &dev->mqtt_head);
				printf("ADD [%s] to mqtt_head to wait ACK!, type=%d, msgid=0x%04x\r\n", 
								mqtt_get_name(dev->atcmd->mqtype), dev->atcmd->mqtype, dev->atcmd->msgid);
			} 
			else 
			{
				longsung_release_command(dev->atcmd, NULL, NULL);
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
		if(cmd->mqtype != -1 && cmd->clean == 0) 
		{
			total++;
      cmd->clean = 1;
		}
	}
	
	list_for_each_safe(pos, n, &dev->at_head)
	{
		cmd = list_entry(pos, AtCommand, list);
		if(cmd->mqtype != -1 && cmd->clean == 0) 
		{
			total++;
      cmd->clean = 1; 
		}
	}		
	
	printf("%s: clean [%d] mqtt message!\r\n", __func__, total);	
}

static void check_socket_number(void)
{
	int i;
	
	/*检查活跃的socket个数，保持一个连接*/
	dev->socket_num = 0;

	for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) 
	{
		if(dev->socket_open[i] != -1)
			dev->socket_num++;
	}

	/*若发现socket大于1，关闭所有socket,重新建立连接*/
	if( (dev->socket_num > 1 || dev->socket_close_flag) ) 
	{
		/*检查是否已经发过ATMIPCLOSE*/
		if(!check_command_exist(ATMIPCLOSE)) 
		{ 
			for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) 
			{
				if( dev->socket_open[i] != -1 ) 
				{
					make_command_to_list(ATMIPCLOSE, ONE_SECOND/5, dev->socket_open[i]);
				}
			}
		}
		
		if(dev->socket_close_flag == 1) dev->socket_close_flag = 0;
	}	
}

static void mqtt_send_connect_or_tick(void)
{
	if( dev->socket_num == 1) 
	{//触发条件
		if(dev->ppp_status == PPP_CONNECTED) 
		{//状态
			/*连接上MQTT*/
			if(mqtt_dev->connect_status == MQTT_DEV_STATUS_NULL ||
				 mqtt_dev->connect_status == MQTT_DEV_STATUS_DISCONNECT) 
			{//二级状态
				/*连接前把  mqtt_list at_list 的mqtt消息清空！在+MIPCLOSE:清空*/			
				mqtt_dev->connect_status = MQTT_DEV_STATUS_CONNECTING;
				make_command_to_list(ATMQTT, ONE_SECOND/20, MQTT_MSG_TYPE_CONNECT);
				make_command_to_list(ATMIPPUSH, ONE_SECOND/20, -1);	
			}		
			/*发送心跳包给服务 40s每个tick*/				
			if( dev->heartbeat_tick >= 7) 
			{//触发条件
				if(mqtt_dev->connect_status != MQTT_DEV_STATUS_NULL
						&& mqtt_dev->connect_status != MQTT_DEV_STATUS_CONNECTING) 
				{//二级状态
					dev->heartbeat_tick = 0;
					if(mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT) 
					{
						//有可能是其它的状态MQTT_DEV_STATUS_CONNACK  MQTT_DEV_STATUS_SUBACK
						make_command_to_list(ATMQTT, ONE_SECOND/20, MQTT_MSG_TYPE_PINGREQ);
						make_command_to_list(ATMIPPUSH, ONE_SECOND/40, -1);			
					}
				}									
			} 
		}
	}	
}

static void tcp_connect_server(void)
{
	/*连接上远程服务端*/
	if(dev->socket_num == 0 ) 
	{//触发条件
		if( dev->ppp_status == PPP_CONNECTED) 
		{//状态
			/*取消上一次要关闭tcp的请求*/
			if(dev->close_tcp_interval) dev->close_tcp_interval = 0;//动作
			/*发ATMIPOPEN ONE_SECOND/2 后才可发ATMIPHEX， 否则ATMIPHEX失败*/
			make_command_to_list(ATMIPHEX, ONE_SECOND/5, -1);							
			make_command_to_list(ATMIPOPEN, ONE_SECOND/2, -1);			
			//make_command_to_list(ATMIPHEX, ONE_SECOND/2, -1);					
			dev->heartbeat_tick = 6;
		}
	}	
}

static void check_sm_and_ppp(void)
{
	if( dev->simcard_type > 0 ) 
	{
		/*查询SM短信未读index*/
		make_command_to_list(ATCPMS, ONE_SECOND/20, -1);				
		make_command_to_list(ATCMGD_, ONE_SECOND/20, -1);
		/*查询SM短信未读index,查询到有短信，就会马上进入读模块代码*/

		/*查询IP*/
		make_command_to_list(ATMIPCALL_, ONE_SECOND/20, -1);							
	}

	/*PPP连接获取IP*/		
	if( dev->simcard_type > 0 ) 
	{//出发条件
		if( dev->ppp_status == PPP_DISCONNECT ) 
		{ //状态
			dev->ppp_status = PPP_CONNECTING;//修改状态
			make_command_to_list(ATMIPCALL0, ONE_SECOND/5, -1);//动作	
			make_command_to_list(ATMIPPROFILE, ONE_SECOND/5, -1);	
			make_command_to_list(ATMIPCALL1, ONE_SECOND, -1);
		}						
	}	
}

static void check_longsung_status(void)
{
	dev->scsq++;

	if( (dev->scsq)-(dev->rcsq) > 3 ) 
	{
		dev->reset_request = 1;
		dev->socket_close_flag = 1;
		printf("scsq-rcsq=%d! error.\r\n", dev->scsq-dev->rcsq);
	}

	/*无SIM卡，等待用户插入，2分钟左右会重启4G模块*/
	if( dev->simcard_type == 0 && dev->scsq > 4 ) 
	{
		dev->reset_request = 1;
		printf("sim card no exit!\r\n");
	}		
}

static void send_init_at_command(void)
{
	/*查询SIM卡是否插入, -1为初始化状态，0为无卡*/
		
	/*为了第一时间连接上服务器*/
	make_command_to_list(ATSIMTEST, ONE_SECOND/5, -1);
	make_command_to_list(ATMIPCALL_, ONE_SECOND/5, -1);
	make_command_to_list(ATMIPCALL0, ONE_SECOND/5, -1);	
	make_command_to_list(ATMIPPROFILE, ONE_SECOND/5, -1);	
	make_command_to_list(ATMIPCALL1, ONE_SECOND, -1);
	/*ATMIPHEX有时没成功，注意原因*/
	make_command_to_list(ATMIPHEX, ONE_SECOND/5, -1);						
	make_command_to_list(ATMIPOPEN, ONE_SECOND/2, -1);

	/*****************************************************************************
	ATMIPHEX不成功，导致返回1438 而不是1469， 1500为buffer长度。
	AT+MIPSEND=1,"101d00064d5149736470030400780003594a5a00036d637500056465617468"
	+MIPSEND:1,1438
	*****************************************************************************/				

	make_command_to_list(ATMIPCALL_, ONE_SECOND/5, -1);		
	dev->heartbeat_tick = 6;				
}

static void list_sm_event(void)
{
	/*删除sim卡中的短信, 短信相关的AT指令时间要长些NE_SECOND/5~ONE_SECOND/2*/
	if( dev->sm_index_delete != -1 && dev->sm_delete_flag ) 
	{
		make_command_to_list(ATCPMS, ONE_SECOND/5, -1);
		make_command_to_list(ATCMGD, ONE_SECOND/2, dev->sm_index_delete);					
		dev->sm_delete_flag = 0;
	}

	/*读取sim卡中的短信*/
	if( dev->sm_num > 0 && dev->sm_read_flag ) 
	{
		make_command_to_list(ATCPMS, ONE_SECOND/5, -1);	
		make_command_to_list(ATCMGF, ONE_SECOND/5, -1);
		make_command_to_list(ATCMGR, ONE_SECOND/5, dev->sm_index[0]);					
		dev->sm_read_flag = 0;
	}	
}

static void clean_mqtt_connect()
{
	int i;
	char cmd[15];
	
	if(dev->clean_interval == 3*ONE_SECOND/2)	
	{				
		release_command_from_list(&dev->at_head);
		release_command_from_list(&dev->mqtt_head);
		
		if(mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT)
			mqtt_disconnect_server(mqtt_dev->mqtt_state);
					
	}
	else if(dev->clean_interval == ONE_SECOND)
	{
		for( i=0; i<sizeof(dev->socket_open)/sizeof(dev->socket_open[0]); i++ ) 
		{
			if( dev->socket_open[i] != -1 ) 
			{ 
				memset(cmd, '\0', sizeof(cmd));
				sprintf(cmd, "AT+MIPCLOSE=%d", i);
				delay_ms(10);
				at(cmd);
				delay_ms(50);				
			}
		}						
	}	
}

/*状态机模式： 横竖两种写法。*/
void handle_longsung_setting(void)
{
	static char period = 0;
	
	if(!mAndroidPower) 
	{
		/*android关闭，重启4G模块*/
		if(dev->reset_request) 
		{	
			init_longsung_status(0);
			init_mqtt_dev(mqtt_dev);					
			//reset 4g modules by gpio
			printf("Reset 4G Module.\r\n");
		}
		
		if(dev->period_tick) 
		{
			dev->period_tick = 0;
			period = 1;	
			dev->heartbeat_tick++;
			
			printf("*********************************\r\n");
			make_command_to_list(ATCSQ, ONE_SECOND/10, -1);

			if(dev->simcard_type == -1) 
			{				
				send_init_at_command();
			}			
			printf("MQTT pub_in=%d, pub_out=%d\r\n", mqtt_dev->pub_in_num, mqtt_dev->pub_out_num);
		}

		if(dev->boot_status) 
		{
			if( period ) 
			{
				check_longsung_status();		
				check_sm_and_ppp();
			}
			
			if(dev->close_tcp_interval > 0) 
			{
				if(--dev->close_tcp_interval==0) 
				{
					dev->socket_close_flag = 1;
					printf("%s: set socket_close_flag to 1\r\n", __func__);
				}
			}
			check_socket_number();
				
			if(period) 
			{
				tcp_connect_server();
			}
			mqtt_send_connect_or_tick();
			list_sm_event();
		}

		at_list_cmd();
		mqtt_list_cmd();	
		
		period = 0;	
	} 
	else 
	{
		if(!dev->is_inited)
		{
			clean_mqtt_connect();
					
			if((--dev->clean_interval) > 0) 
				return;
			else 
				printf("\r\nTimeout! clean_interval = 0, enter ANDROID.\r\n");
			
			/*set mqtt_alive to 0*/
			init_longsung_status(1);
			init_mqtt_dev(mqtt_dev);
			//reset 4g modules by gpio
			printf("android power on, reset 4G module\r\n");
			//erase_flash(); add for test bug.
		}
	}
}

void longsung_init()
{	
	init_longsung_reader();
	init_longsung_status(1);
	
	INIT_LIST_HEAD(&dev->at_head);
	INIT_LIST_HEAD(&dev->mqtt_head);	

	init_mqtt_dev(mqtt_dev);
}


/*
MQTT_MSG_TYPE_SUBACK
AT+MIPSEND=1,"821a000100152f73797374656d2f6c69622f68772f73656e736f7201"

fixed header:
82 1a

variable header:
0001 //msg_id  在SUBACK中需要比较

payload:
0015 // "/system/lib/hw/sensor".len
2f73797374656d2f6c69622f68772f73656e736f72 //"/system/lib/hw/sensor"
01 //qos

... 可重复多个topic的信息
...
...

***********************************************************************
MQTT_MSG_TYPE_SUBACK
+MIPRTCP=1,5,9003000101
--mqtt packet:
90,03,00,01,01,
message_length=5
msg_type=9, msg_qos=0, msg_id=0x01
------->MQTT_MSG_TYPE_SUBACK state->pending_msg_id=0X0001, msg_id=0X0001
complete_pending: type=3

fixed header:
90,03,
variable header:
00,01, //msg_id
01, //qos
***********************************************************************
CONNECT
	dev->connect_info.client_id = "yangjianzhou";
	dev->connect_info.username = NULL;//"yang";//NULL 
	dev->connect_info.password = NULL;//"zhou";// NULL
	dev->connect_info.will_topic = "mcu";
	dev->connect_info.will_message = "death";
	dev->connect_info.keepalive = 300;
	dev->connect_info.will_qos = 1;
	dev->connect_info.will_retain = 0;
	dev->connect_info.clean_session = 1;
	
MQTT_MSG_TYPE_CONNECT
fixed header:
10 26 
variable header:
00 06 
4d5149736470 //MQIsdp
03 //version
16 //0001,0110
012c //300
000c //yangjianzhou.len = 12
79616e676a69616e7a686f75 //"yangjianzhou"
0003 "mcu".len
6d6375 "mcu"
0005 "death".len
6465617468" "death"

***********************************************************************
"321c
0006
73656e736f72
0002
4d43552046524f4d205849414f2050454e47"

MQTT_MSG_TYPE_PUBLISH
fixed header:
32,39,
variable header:
00,15,  // "/system/lib/hw/sensor".len=21
2F,73,79,73,74,65,6D,2F,6C,69,62,2F,68,77,2F,73,65,6E,73,6F,72, /system/lib/hw/sensor
00,17, //MSG_ID 不关注此ID的值，把它返回给server
payload:
31,32,33,34,35,61,62,63,64,65,00,00,00,00,00,00, //"12345abcef******################"
8D,C5,46,5E,6F,81,9F,03,25,59,8C,BE,1C,7B,45,8F,

***********************************************************************
MQTT_MSG_TYPE_PUBACK
fixed header:
40,02,
variable header:
00,04, //msg_id  需比较此ID，是否为发出去的消息的ID一致

***********************************************************************
MQTT_MSG_TYPE_CONNACK
fixed header:
2002
variable header:
0000 要比较第四个字节的值，从而得到连接结果。
***********************************************************************
*/	



/**
AT+CPMS="SM"
+CPMS: 0,50,0,90,0,50

OK
AT+CMGD=?
+CMGD: (),(0-4)
sm_num=0, sm_index={ }

OK
AT+MIPCALL?
+MIPCALL:1,10.232.231.76

OK
AT+MIPOPEN=1,0,"112.124.102.62",1883,0
+MIPOPEN=1,1
+MIP:OK
OK
make_command_to_list: ADD [MQTT_MSG_TYPE_CONNECT] to list success. mqtype=1,msg_id=0XB3BB
SEND [MQTT_MSG_TYPE_CONNECT] mqtype=1,id=0xb3bb
AT+MIPSEND=1,"101d00064d5149736470030400780003594a5a00036d637500056465617468"
+MIPSEND:1,1469 1500


+MIPSEND:1,1438//31
OK
longsung_release_command: REMOVE [MQTT_MSG_TYPE_CONNECT] type=1, msgid=0xb3bb!
AT+MIPPUSH=1
OK
----------------------------------
[result: OK] AT+MIPPUSH=1

+MIPRTCP=1,4,20020000
[MQTT_MSG_TYPE_CONNACK] come! msg_type=2,QOS=0,msg_id=0x00, mesg_len=4,nbytes=4
make_command_to_list: ADD [MQTT_MSG_TYPE_SUBSCRIBE] to list success. mqtype=8,msg_id=0X0002
MQTT_MSG_TYPE_CONNACK, ack=00, status=2
make_command_to_list: ADD [MQTT_MSG_TYPE_PINGREQ] to list success. mqtype=12,msg_id=0XA197
SEND [MQTT_MSG_TYPE_SUBSCRIBE] mqtype=8,id=0x0003
AT+MIPSEND=1,"821a000300152f73797374656d2f6c69622f68772f73656e736f7201"
+MIPSEND:1,1472



AT+CMGD=?
+CMGD: (),(0-4)
sm_num=0, sm_index={ }

OK
AT+MIPCALL?
+MIPCALL:1,10.216.135.188

OK
AT+MIPOPEN=1,0,"112.124.102.62",1883,0
+MIPOPEN=1,1
+MIP:OK
OK
make_command_to_list: ADD [MQTT_MSG_TYPE_CONNECT] to list success. mqtype=1,msg_id=0XB3BB
SEND [MQTT_MSG_TYPE_CONNECT] mqtype=1,id=0xb3bb
AT+MIPSEND=1,"101d00064d5149736470030400780003594a5a00036d637500056465617468"
+MIPSEND:1,1438

OK
longsung_release_command: REMOVE [MQTT_MSG_TYPE_CONNECT] type=1, msgid=0xb3bb!
AT+MIPPUSH=1
OK
----------------------------------
[result: OK] AT+MIPPUSH=1

+MIPCLOSE:1,62,0,0
on_disconnect_service_callback: +MIPCLOSE !!!!!
mqtt_msg_set_clean: clean [0] mqtt message!
*********************************


T+MIPCALL=0
OK
AT+MIPPROFILE=1,"3GNET"
OK
AT+MIPCALL=1
+MIPCALL:1,10.153.106.80
+MIP:OK
OK
AT+MIPOPEN=1,0,"112.124.102.62",1883,0
+MIPOPEN=1,1
+MIP:OK
OK
make_command_to_list: ADD [MQTT_MSG_TYPE_CONNECT] to list success. mqtype=1,msg_id=0XB3BB
AT+MIPCALL?
+MIPCALL:1,10.153.106.80

OK
SEND [MQTT_MSG_TYPE_CONNECT] mqtype=1,id=0xb3bb
AT+MIPSEND=1,"101d00064d5149736470030400780003594a5a00036d637500056465617468"
+MIPSEND:1,1438

OK
longsung_release_command: REMOVE [MQTT_MSG_TYPE_CONNECT] type=1, msgid=0xb3bb!
AT+MIPPUSH=1
OK
----------------------------------
[result: OK] AT+MIPPUSH=1

+MIPCLOSE:1,62,0,0
on_disconnect_service_callback: +MIPCLOSE !!!!!
mqtt_msg_set_clean: clean [0] mqtt message!

**/


