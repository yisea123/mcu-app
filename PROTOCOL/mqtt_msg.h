/*
 * Copyright (c) 2014, Stephen Robinson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 *  are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <stdint.h>

#ifndef MQTT_MSG_H_
#define MQTT_MSG_H_

#define MQTT_BUFF_DECODE        	0
#define MQTT_BUFF_ENCODE       	 	1

/**********************************
#define MQTT_OUTDATA_CONNECT    	1
#define MQTT_OUTDATA_PUBLISH    	3
#define MQTT_OUTDATA_PUBACK     	4
#define MQTT_OUTDATA_PUBREC     	5
#define MQTT_OUTDATA_PUBREL     	6
#define MQTT_OUTDATA_PUBCOMP    	7
#define MQTT_OUTDATA_SUBSCRIBE  	8
#define MQTT_OUTDATA_SUBACK     	9
#define MQTT_OUTDATA_UNSUBSCRIBE 	10
#define MQTT_OUTDATA_PINGREQ    	12
#define MQTT_OUTDATA_PINGRESP   	13
#define MQTT_OUTDATA_DISCONNECT 	14
#define MQTT_OUTDATA_SUBSCRIBE_TEST  110
**********************************/

enum mqtt_dev_statu
{
	MQTT_DEV_STATUS_NULL				= 0,
	MQTT_DEV_STATUS_CONNECTING	= 1,
  MQTT_DEV_STATUS_CONNECT     = 2,
  MQTT_DEV_STATUS_CONNACK     = 3,
  MQTT_DEV_STATUS_PUBLISH     = 4,
  MQTT_DEV_STATUS_PUBACK      = 5,
  MQTT_DEV_STATUS_PUBREC      = 6,
  MQTT_DEV_STATUS_PUBREL      = 7,
  MQTT_DEV_STATUS_PUBCOMP     = 8,
  MQTT_DEV_STATUS_SUBSCRIBE   = 9,
  MQTT_DEV_STATUS_SUBACK      = 10,
  MQTT_DEV_STATUS_UNSUBSCRIBE = 11,
  MQTT_DEV_STATUS_UNSUBACK    = 12,
  MQTT_DEV_STATUS_PINGREQ     = 13,
  MQTT_DEV_STATUS_PINGRESP    = 14,
  MQTT_DEV_STATUS_DISCONNECT  = 15
};

enum mqtt_message_type
{
  MQTT_MSG_TYPE_CONNECT     = 1,
  MQTT_MSG_TYPE_CONNACK     = 2,
  MQTT_MSG_TYPE_PUBLISH     = 3,
  MQTT_MSG_TYPE_PUBACK      = 4,
  MQTT_MSG_TYPE_PUBREC      = 5,
  MQTT_MSG_TYPE_PUBREL      = 6,
  MQTT_MSG_TYPE_PUBCOMP     = 7,
  MQTT_MSG_TYPE_SUBSCRIBE   = 8,
  MQTT_MSG_TYPE_SUBACK      = 9,
  MQTT_MSG_TYPE_UNSUBSCRIBE = 10,
  MQTT_MSG_TYPE_UNSUBACK    = 11,
  MQTT_MSG_TYPE_PINGREQ     = 12,
  MQTT_MSG_TYPE_PINGRESP    = 13,
  MQTT_MSG_TYPE_DISCONNECT  = 14
};

typedef struct
{
  uint8_t* data;
  uint16_t length;

} mqtt_message_t;

typedef struct
{
  mqtt_message_t message;

  uint16_t message_id;
  uint8_t* buffer;
  uint16_t buffer_length;

} mqtt_connection_t;

typedef struct mqtt_connect_info
{
  const char* client_id;
  const char* username;
  const char* password;
  const char* will_topic;
  const char* will_message;
  int keepalive;
  int will_qos;
  int will_retain;
  int clean_session;

} mqtt_connect_info_t;

typedef struct mqtt_event_data_t
{
  uint8_t type;
  char *topic;
  char *data;

  uint16_t topic_length;
  uint16_t data_length;
  uint16_t data_offset;
} mqtt_event_data_t;

#define MQTT_FLAG_CONNECTED          1
#define MQTT_FLAG_READY              2
#define MQTT_FLAG_EXIT               4

#define MQTT_EVENT_TYPE_NONE                  0
#define MQTT_EVENT_TYPE_CONNECTED             1
#define MQTT_EVENT_TYPE_DISCONNECTED          2
#define MQTT_EVENT_TYPE_SUBSCRIBED            3
#define MQTT_EVENT_TYPE_UNSUBSCRIBED          4
#define MQTT_EVENT_TYPE_PUBLISH               5
#define MQTT_EVENT_TYPE_PUBLISHED             6
#define MQTT_EVENT_TYPE_EXITED                7
#define MQTT_EVENT_TYPE_PUBLISH_CONTINUATION  8

#define OUT_DATA_LEN_MAX        							40

typedef struct mqtt_state_t
{
  uint16_t port;
  int auto_reconnect;
  mqtt_connect_info_t* connect_info;

  uint8_t* in_buffer;
  uint8_t* out_buffer;
  int in_buffer_length;
  int out_buffer_length;
  uint16_t message_length;
  uint16_t message_length_read;
  mqtt_message_t* outbound_message;
  mqtt_connection_t mqtt_connection;
  uint16_t pending_msg_id;
  int pending_msg_type;
	int mqtt_flags;
	unsigned char *outdata[OUT_DATA_LEN_MAX];
	char jsonbuff[750];
} mqtt_state_t;

typedef struct {
	enum mqtt_dev_statu connect_status;
	/*由于4G模块的原因，最多支持1500个字节，也就750个hex!*/
  uint8_t in_buffer[1500];//750 1024
	int in_pos;
	int in_waitting;
  char fixhead;
	char parse_packet_flag;
	/*是否为mqtt协议头标志！*/	
  uint8_t out_buffer[1500];	
	mqtt_state_t mqtt_state[1];
	mqtt_connect_info_t connect_info;
	char subscribe_topic[128];
	uint32_t pub_in_num;
	uint32_t pub_out_num;
	uint32_t reset_count;
	long long recv_bytes;
	char reboot;
	char iot;
}mqtt_dev_status;

static int mqtt_get_type(uint8_t* buffer)   { return (buffer[0] & 0xf0) >> 4; }
static int mqtt_get_dup(uint8_t* buffer)    { return (buffer[0] & 0x08) >> 3; }
static int mqtt_set_dup(uint8_t* buffer)    { return (buffer[0] | 0x08); }
static int mqtt_get_qos(uint8_t* buffer)    { return (buffer[0] & 0x06) >> 1; }
static int mqtt_get_retain(uint8_t* buffer) { return (buffer[0] & 0x01); }

void mqtt_msg_init(mqtt_connection_t* connection, uint8_t* buffer, uint16_t buffer_length);
int mqtt_get_total_length(uint8_t* buffer, uint16_t length);
char* mqtt_get_publish_topic(uint8_t* buffer, uint16_t* length);
char* mqtt_get_publish_data(uint8_t* buffer, uint16_t* length);
uint16_t mqtt_get_id(uint8_t* buffer, uint16_t length);

mqtt_message_t* mqtt_msg_connect(mqtt_connection_t* connection, mqtt_connect_info_t* info);
mqtt_message_t* mqtt_msg_publish(mqtt_connection_t* connection, const char* topic, const char* data, int data_length, int qos, int retain, uint16_t* message_id);
mqtt_message_t* mqtt_msg_puback(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* mqtt_msg_pubrec(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* mqtt_msg_pubrel(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* mqtt_msg_pubcomp(mqtt_connection_t* connection, uint16_t message_id);
mqtt_message_t* mqtt_msg_subscribe(mqtt_connection_t* connection, const char* topic, int qos, uint16_t* message_id);
mqtt_message_t* mqtt_msg_unsubscribe(mqtt_connection_t* connection, const char* topic, uint16_t* message_id);
mqtt_message_t* mqtt_msg_pingreq(mqtt_connection_t* connection);
mqtt_message_t* mqtt_msg_pingresp(mqtt_connection_t* connection);
mqtt_message_t* mqtt_msg_disconnect(mqtt_connection_t* connection);

extern void mqtt_init(mqtt_state_t* state, uint8_t* in_buffer, int 
	in_buffer_length, uint8_t* out_buffer, int out_buffer_length);
#endif

/*
MQTT_MSG_TYPE_SUSCRIBE
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



