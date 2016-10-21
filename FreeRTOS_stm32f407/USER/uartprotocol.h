#ifndef __UARTPROTOCOL_H
#define __UARTPROTOCOL_H	

#include <stdlib.h>
#include <string.h>

#define	VERSION			1
// 	1:	aa bb len(2) msgid(2) type(1) d0~dn crc(1)					
//   2:	aa bb len(1) msgid(2) type(1) d0~dn crc(2)

#define MSGID_VOICE			0X0001

#define UHEAD1				1
#define UHEAD2				2
#define ULEN				3
#define UMSGID				4
#define UTYPE				5
#define UDATA				6
#define UCRC				7

#define TYPE_ACK			0X00
#define TYPE_CMD			0X01

#define UART_ACK			1
#define UART_CMD			2
#define UART_WAIT		    3

#define MSG_ANDOIRD_CAN					0x0001
#define MSG_ANDOIRD_SET_RTC				0x0002
#define MSG_ANDOIRD_GET_RTC				0x0003
#define MSG_ANDOIRD_SET_ALRAM			0x0004
#define MSG_ANDOIRD_PWR_REQUEST			0x0005
#define MSG_ANDOIRD_DEBUG_REQUEST		0x0006
#define MSG_ANDOIRD_CHARGE				0x0007
#define MSG_ANDOIRD_PM_STATUS			0x0008
#define MSG_ANDOIRD_AWAKE				0x0009
#define MSG_ANDOIRD_VEHICLE_ID			0x000A
#define MSG_ANDOIRD_OTA					0x000B
#define MSG_ANDOIRD_GET_MCU_VER			0x000C



#define MSG_FILE			0x8001
#define MSG_YMODEM			0x8002

#define HANDLE_NULL			0
#define HANDLE_YMODEM		1

#if( VERSION == 1 )

typedef  unsigned short LEN_Type;
typedef  unsigned short MSGID_Type;
typedef  unsigned char  TYPE_Type;
typedef  unsigned char  CRC_Type;

#elif( VERSION == 2)

typedef  unsigned char  LEN_Type;
typedef  unsigned short MSGID_Type;
typedef  unsigned char  TYPE_Type;
typedef  unsigned short CRC_Type;

#else
		#error VERSION must be define  to 1 or 2.
#endif

#define N_LEN		( sizeof( LEN_Type ) )
#define N_MSGID		( sizeof( MSGID_Type ) )
#define N_TYPE		( sizeof( TYPE_Type ) )
#define N_CRC		( sizeof( CRC_Type ) )

#define N_PROTOCOL_TOTAL		( 2 + N_LEN + N_MSGID + N_TYPE + N_CRC )

typedef __packed struct 
{
	LEN_Type 		len;
	MSGID_Type 		msgid;
} ProtocolHead;

typedef __packed struct 
{
	LEN_Type 		len;
	MSGID_Type 		msgid;
	TYPE_Type  		type;
	CRC_Type  		crc;
} ProtocolAck;

typedef struct 
{
	ProtocolHead*   	head;
	char *		   		data;
	CRC_Type  		    crc;
} ProtocolData;

typedef struct 
{
	MSGID_Type 		msgid;
	CRC_Type		crc;
	char 			recvRespond;
} WaitAckInfo;

extern void HandleDownStreamTask( void * pvParameters );
extern void HandleUpstreamTask( void * pvParameters );
extern CRC_Type CheckSum( unsigned char *buf, int packLen );
extern int put_buffer_to_stream( char *buff, short len );
#endif



