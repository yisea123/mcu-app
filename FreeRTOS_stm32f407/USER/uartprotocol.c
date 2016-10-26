#include <string.h>
#include "uartprotocol.h"
#include "rfifo.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "amrfile.h"
#include "ff.h"
#include "sys.h"
#include "romhandler.h"	
//#include "ymodem.h"

extern TaskHandle_t pxUpStreamTask;

Ringfifo 	   uart6fifo;
Ringfifo 	   upStreamFifo;
WaitAckInfo    waitAckInfo;
xSemaphoreHandle xWaitAckSemaphore = NULL ;
xSemaphoreHandle mUartSendMutex = NULL;

#if( VERSION == 2 )
static const unsigned short crctable[256] =
{
	0x0000 , 0x1021 , 0x2042 , 0x3063 , 0x4084 , 0x50a5 , 0x60c6 , 0x70e7 , 0x8108 , 0x9129 ,
	0xa14a , 0xb16b , 0xc18c , 0xd1ad , 0xe1ce , 0xf1ef , 0x1231 , 0x0210 , 0x3273 , 0x2252 ,
	0x52b5 , 0x4294 , 0x72f7 , 0x62d6 , 0x9339 , 0x8318 , 0xb37b , 0xa35a , 0xd3bd , 0xc39c , 
	0xf3ff , 0xe3de , 0x2462 , 0x3443 , 0x0420 , 0x1401 , 0x64e6 , 0x74c7 , 0x44a4 , 0x5485 , 
	0xa56a , 0xb54b , 0x8528 , 0x9509 , 0xe5ee , 0xf5cf , 0xc5ac , 0xd58d , 0x3653 , 0x2672 , 
	0x1611 , 0x0630 , 0x76d7 , 0x66f6 , 0x5695 , 0x46b4 , 0xb75b , 0xa77a , 0x9719 , 0x8738 , 
	0xf7df , 0xe7fe , 0xd79d , 0xc7bc , 0x48c4 , 0x58e5 , 0x6886 , 0x78a7 , 0x0840 , 0x1861 , 
	0x2802 , 0x3823 , 0xc9cc , 0xd9ed , 0xe98e , 0xf9af , 0x8948 , 0x9969 , 0xa90a , 0xb92b , 
	0x5af5 , 0x4ad4 , 0x7ab7 , 0x6a96 , 0x1a71 , 0x0a50 , 0x3a33 , 0x2a12 , 0xdbfd , 0xcbdc , 
	0xfbbf , 0xeb9e , 0x9b79 , 0x8b58 , 0xbb3b , 0xab1a , 0x6ca6 , 0x7c87 , 0x4ce4 , 0x5cc5 , 
	0x2c22 , 0x3c03 , 0x0c60 , 0x1c41 , 0xedae , 0xfd8f , 0xcdec , 0xddcd , 0xad2a , 0xbd0b , 
	0x8d68 , 0x9d49 , 0x7e97 , 0x6eb6 , 0x5ed5 , 0x4ef4 , 0x3e13 , 0x2e32 , 0x1e51 , 0x0e70 , 
	0xff9f , 0xefbe , 0xdfdd , 0xcffc , 0xbf1b , 0xaf3a , 0x9f59 , 0x8f78 , 0x9188 , 0x81a9 , 
	0xb1ca , 0xa1eb , 0xd10c , 0xc12d , 0xf14e , 0xe16f , 0x1080 , 0x00a1 , 0x30c2 , 0x20e3 , 
	0x5004 , 0x4025 , 0x7046 , 0x6067 , 0x83b9 , 0x9398 , 0xa3fb , 0xb3da , 0xc33d , 0xd31c , 
	0xe37f , 0xf35e , 0x02b1 , 0x1290 , 0x22f3 , 0x32d2 , 0x4235 , 0x5214 , 0x6277 , 0x7256 , 
	0xb5ea , 0xa5cb , 0x95a8 , 0x8589 , 0xf56e , 0xe54f , 0xd52c , 0xc50d , 0x34e2 , 0x24c3 , 
	0x14a0 , 0x0481 , 0x7466 , 0x6447 , 0x5424 , 0x4405 , 0xa7db , 0xb7fa , 0x8799 , 0x97b8 , 
	0xe75f , 0xf77e , 0xc71d , 0xd73c , 0x26d3 , 0x36f2 , 0x0691 , 0x16b0 , 0x6657 , 0x7676 , 
	0x4615 , 0x5634 , 0xd94c , 0xc96d , 0xf90e , 0xe92f , 0x99c8 , 0x89e9 , 0xb98a , 0xa9ab , 
	0x5844 , 0x4865 , 0x7806 , 0x6827 , 0x18c0 , 0x08e1 , 0x3882 , 0x28a3 , 0xcb7d , 0xdb5c , 
	0xeb3f , 0xfb1e , 0x8bf9 , 0x9bd8 , 0xabbb , 0xbb9a , 0x4a75 , 0x5a54 , 0x6a37 , 0x7a16 , 
	0x0af1 , 0x1ad0 , 0x2ab3 , 0x3a92 , 0xfd2e , 0xed0f , 0xdd6c , 0xcd4d , 0xbdaa , 0xad8b , 
	0x9de8 , 0x8dc9 , 0x7c26 , 0x6c07 , 0x5c64 , 0x4c45 , 0x3ca2 , 0x2c83 , 0x1ce0 , 0x0cc1 , 
	0xef1f , 0xff3e , 0xcf5d , 0xdf7c , 0xaf9b , 0xbfba , 0x8fd9 , 0x9ff8 , 0x6e17 , 0x7e36 , 
	0x4e55 , 0x5e74 , 0x2e93 , 0x3eb2 , 0x0ed1 , 0x1ef0  
};
#endif

/*aa bb len(2) msgid(2) type(1) d0~dn = buf, crc(1).     packLen = data len*/
/*aa bb len(1) msgid(2) type(1) d0~dn = buf, crc(2).     packLen = data len*/
CRC_Type CheckSum( unsigned char *buf, int packLen )
{
#if( VERSION == 1 )
    int i;
    unsigned int sum;

    if( buf == NULL || packLen <= 0 ) 
	{
		return 0;
    }
	sum = 0;
    for( i = 0; i < packLen; i++ )
    {
        sum += buf[ i ];
    }

    return ( CRC_Type )(sum % 254);
#else
	unsigned char ch;
	CRC_Type crc = 0;

	while( packLen-- )
	{
		ch = crc >> 8;
		crc <<= 8;
		crc ^= crctable[ ch ^ *buf++ ];
	}

	return crc;
#endif
}

/*
* author: 	yangjianzhou
* function: 	fill ackBuff by using msg_id and crc.
*/
static void fill_ack_buffer( char *ackBuff, MSGID_Type msg_id, CRC_Type crc )
{
	*( ackBuff ) = 0xaa;
	*( ackBuff + 1 ) = 0xbb;
	*( ( LEN_Type* )( ackBuff + 2 ) ) = N_CRC;
	*( ( MSGID_Type* )( ackBuff + 2 + N_LEN ) ) = msg_id;
	*( ( TYPE_Type* )( ackBuff + 2 + N_LEN + N_MSGID) ) = TYPE_ACK;
	*( ( CRC_Type* )( ackBuff + 2 + N_LEN + N_MSGID + N_TYPE ) ) = crc;
	*( ( CRC_Type* )( ackBuff + 2 + N_LEN + N_MSGID + N_TYPE + N_CRC ) ) =
			CheckSum( (unsigned char *) ( ackBuff + 2 ), N_LEN + N_MSGID + N_TYPE + N_CRC );
}

/*
* author: 	yangjianzhou
* function: 	exchange recieve data to ProtocolData struct type.
*/
static void format_protocol_data(  ProtocolData * rData, char *buff, int len )
{
	rData->head =  ( ProtocolHead * ) buff;
	rData->data = &( buff[ N_LEN + N_MSGID + N_TYPE ] );
	rData->crc =  *( ( CRC_Type* ) &buff[ len - N_CRC ] );
}

/*
* author: 	yangjianzhou
* function: 	send_data_lock use uart by take the Mutex.
*/
static void send_data_lock( const char* data, int len )
{
	int i = 0;

	xSemaphoreTake( mUartSendMutex, portMAX_DELAY );
	for( i = 0 ; i < len ; i++ ) 
	{
#if( BOARD_NUM == 3 )	
		USART_ClearFlag( USART6, USART_FLAG_TC ); 
		USART_SendData( USART6, data[i] );
		while(USART_GetFlagStatus( USART6, USART_FLAG_TC ) != SET );
#else
		USART_ClearFlag( USART3, USART_FLAG_TC ); 
		USART_SendData( USART3, data[i] );
		while(USART_GetFlagStatus( USART3, USART_FLAG_TC) != SET );
#endif
	}	
	xSemaphoreGive( mUartSendMutex );
}

static void send_command_ack( const char* data, int len )
{
	//printf("%s:1\r\n", __func__);
	send_data_lock( data, len );	
	//put_buffer_to_stream( data, len )
}
	

/*****************************************
1:	aa bb len(2) msgid(2) type(1) d0~dn crc(1)					
2:	aa bb len(1) msgid(2) type(1) d0~dn crc(2)
*****************************************/

/*
*( CRC_Type * )( buff + N_LEN + N_MSGID + N_TYPE + datalen ) = crc;
for( i = 0; i < ( N_LEN + N_MSGID + N_TYPE + N_CRC + data_len ); i++ ) 
{
	printf("0X%02X ", *( buff + i ) );
}				
printf("\r\n");
*/

static int catch_data_frame( void *fifo, char buff[], int *bufflen )
{
	char 			 data;
	MSGID_Type    	 msgid;
	CRC_Type		 crc;
	CRC_Type   		 calcrc;
	static TYPE_Type type;
	static LEN_Type  datalen;	
	static int		 state = UHEAD1;	
	struct rfifo*    mfifo = ( struct rfifo * ) fifo;

	while( 1 )
	{
		switch( state ) 
		{
			case UHEAD1:			
				if( rfifo_len( mfifo ) < 1 )
				{
					break;
				}			
				rfifo_get( mfifo, &data, 1 );
				if( data == 0xaa ) 
				{
					state++;
				} 
				else
				{
					printf("1*\r\n");
					continue;
				} 
				
			case UHEAD2:
				if( rfifo_len( mfifo ) < 1 )
				{
					break;
				}				
				rfifo_get( mfifo, &data, 1 );
				if( data == 0xbb ) 
				{	
					state++;
				} 
				else 
				{
					state = UHEAD1;
					printf("2*\r\n");
					continue;
				}
				
			case ULEN:
				if( rfifo_len( mfifo ) < N_LEN )
				{
					break;
				}
				rfifo_get( mfifo, &datalen, N_LEN );
				if( datalen >= ( 512 - N_COMMAND_TOTAL ) )
				{
					printf("%s: len go to MAX\r\n", __func__);
					state = UHEAD1;
					printf("3*\r\n");
					continue;
				}
				else
				{
					*( LEN_Type* ) buff = datalen;	
					state++;	
				}
				
			case UMSGID:
				if( rfifo_len( mfifo ) < N_MSGID )
				{
					break;
				}			
				rfifo_get( mfifo, &msgid, N_MSGID );
				*( MSGID_Type* )( buff + N_LEN ) = msgid;
				state++;
				
			case UTYPE:
				if( rfifo_len( mfifo ) < N_TYPE )
				{
					break;
				}			
				rfifo_get( mfifo, &type, N_TYPE );
				if( type != TYPE_ACK && type != TYPE_CMD )
				{
					state = UHEAD1;
					printf("4*\r\n");
					continue;
				}
				else
				{
					state++;
					*( TYPE_Type* )( buff + N_LEN + N_MSGID ) = type;
				}
				
			case UDATA:
				if( rfifo_len( mfifo ) < datalen )
				{
					break;
				}		
				rfifo_get( mfifo,  buff + N_LEN + N_MSGID + N_TYPE, datalen );
				state++;
				
			case UCRC:
				if( rfifo_len( mfifo ) < N_CRC )
				{
					break;
				}		
				state = UHEAD1;
				rfifo_get( mfifo, &crc, N_CRC );
				calcrc = CheckSum( (unsigned char *) buff, N_LEN + N_MSGID + N_TYPE + datalen );
				if( calcrc == crc )
				{
					if( type == TYPE_ACK )
					{
						*bufflen = N_LEN + N_MSGID + N_TYPE + N_CRC;
						return UART_ACK;	
					}
					else if( type == TYPE_CMD )
					{
						*( ( CRC_Type * )( buff + N_LEN + N_MSGID + N_TYPE + datalen ) ) = crc;
						*bufflen = N_LEN + N_MSGID + N_TYPE + N_CRC + datalen;
						return UART_CMD;
					}
					else 
					{
						printf("5*\r\n");
						continue;
					}					
				}
				else 
				{	
					printf("%s: CheckSum Error! type(0x%02x), cal crc(0x%04x),"
						" recv crc(0x%04x)\r\n", __func__, type, calcrc, crc );	
					printf("6*\r\n");

					continue;
				}

			default:
				state = UHEAD1;	
				printf("7*\r\n");
				continue;
		}
		
		break;
	}
	
	return UART_WAIT;
}

/*	
	int i;
	char *p = (char *)rAck;	

	printf("%s:\r\n", __func__);
	for( i = 0; i < N_LEN + N_MSGID + N_TYPE + N_CRC; i++ )
	{
		printf("0X%02X ", *( p + i ) );
	}				
	printf("\r\n");
*/
static void handle_ack_command( ProtocolAck *rAck )
{
	printf("%s \r\n", __func__);
	if( rAck->len == N_CRC && rAck->type == TYPE_ACK )
	{
		if( rAck->msgid == waitAckInfo.msgid && rAck->crc == waitAckInfo.crc 
				&& waitAckInfo.recvRespond == 0 )
		{
			waitAckInfo.recvRespond = 1;
			xSemaphoreGive( xWaitAckSemaphore );
		}
		else
		{
			printf("%s: error ack or no waitting ack!\r\n", __func__);
		}
	}
}

	//printf("%s: crc (0x%04x), and data:\r\n", __func__, rData->crc);
/*	for( i = 0; i < rData->head->len; i++)
	{
		printf("0X%02X ", *( rData->data+ i ) );
	}				
	printf("\r\n"); 
*/
static int handle_remote_command( ProtocolData *rData )
{
	int 	i;
	int 	ret = HANDLE_NULL;
	
	printf("msgid(0x%04x),len(%d)\r\n", rData->head->msgid, rData->head->len);

	switch( rData->head->msgid )
	{
		case MSG_ANDOIRD_CAN:
			printf("%s: MSG_ANDOIRD_CAN.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_SET_RTC:
			printf("%s: MSG_ANDOIRD_SET_RTC.\r\n", __func__);
			break;
		case MSG_ANDOIRD_GET_RTC:
			printf("%s: MSG_ANDOIRD_GET_RTC.\r\n", __func__);
			break;
		case MSG_ANDOIRD_SET_ALRAM:
			printf("%s: MSG_ANDOIRD_SET_ALRAM.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_PWR_REQUEST:
			printf("%s: MSG_ANDOIRD_PWR_REQUEST.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_DEBUG_REQUEST:
			printf("%s: MSG_ANDOIRD_DEBUG_REQUEST.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_CHARGE:
			printf("%s: MSG_ANDOIRD_CHARGE.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_PM_STATUS:
			printf("%s: MSG_ANDOIRD_PM_STATUS.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_AWAKE:
			printf("%s: MSG_ANDOIRD_AWAKE.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_VEHICLE_ID:
			printf("%s: MSG_ANDOIRD_VEHICLE_ID.\r\n", __func__);
			break;	
		case MSG_ANDOIRD_OTA:
			printf("%s: MSG_ANDOIRD_OTA.\r\n", __func__);
			handle_ymodem_command( rData->data, rData->head->len );
			break;	
		case MSG_ANDOIRD_GET_MCU_VER:
			printf("%s: MSG_ANDOIRD_GET_MCU_VER.\r\n", __func__);
			break;					
		default:
			printf("%s: Error msg id.\r\n", __func__);
			break;
	}
	
	return ret;
}

void HandleDownStreamTask( void * pvParameters )
{
	char 	cmdbuff[ 512 ];
	char 	ackbuff[ N_ACK_TOTAL ];
	int 	result = 0;
	int		bufflen;
	ProtocolData rData;
	ProtocolAck  *rAck;
	mUartSendMutex = xSemaphoreCreateMutex();		
	vSetTaskLogLevel(NULL, eLogLevel_0);
	printf("%s: start...\r\n", __func__);
	register_rom_handlers();
	while( 1 )
	{
		/*use ulTaskNotifyTake count function by pass pdFALSE*/
		ulTaskNotifyTake( pdFALSE, portMAX_DELAY );

		while( rfifo_len( &uart6fifo ) > 0 ) 
		{
			result = catch_data_frame( &uart6fifo, cmdbuff, &bufflen );
			if( result == UART_CMD ) 
			{
				format_protocol_data( &rData , cmdbuff, bufflen );
				fill_ack_buffer( ackbuff, rData.head->msgid, rData.crc );
				send_command_ack( ackbuff,  N_ACK_TOTAL );
				handle_remote_command( &rData );
			} 
			else if( result == UART_ACK ) 
			{
				rAck = ( ProtocolAck * ) cmdbuff;
				handle_ack_command( rAck );
			}
			else
			{
				break;
			}
		}
	}
}

int put_buffer_to_stream( char *buff, short len )
{
	static QueueHandle_t  mPutBuffMutex = NULL;
	Ringfifo* streamfifo = &upStreamFifo;

	if( !mPutBuffMutex )
	{
		mPutBuffMutex = xSemaphoreCreateMutex();
	}
	
	/*check upStreamFifo the max size*/
	if( len > streamfifo->size )
	{
		printf("%s: len(%d) error!!\r\n", __func__, len );
		return -1;
	}

	xSemaphoreTake( mPutBuffMutex, portMAX_DELAY );
	while( streamfifo->size - rfifo_len( streamfifo ) < 
		len + sizeof( short ) )
	{
		vTaskDelay( 1 / portTICK_RATE_MS );
	}
	rfifo_put( streamfifo, &len, sizeof( short ) );
	rfifo_put( streamfifo, buff, ( unsigned int )len );
	xSemaphoreGive( mPutBuffMutex );

	/*notify the up stream task*/
	xTaskNotifyGive( pxUpStreamTask );
	
	return 0;
}

void HandleUpstreamTask( void * pvParameters )
{
	short 	  len;
	char 	  pbuff[300];
	Ringfifo* streamfifo = &upStreamFifo;

	rfifo_init( streamfifo );	
	vSetTaskLogLevel( NULL, eLogLevel_3 );	
	vSemaphoreCreateBinary( xWaitAckSemaphore ); 	
	xSemaphoreTake( xWaitAckSemaphore, 0 );

	waitAckInfo.recvRespond = 1;
	waitAckInfo.crc = 255;
	
	vTaskDelay( 10 / portTICK_RATE_MS );
	printf("%s: start...\r\n", __func__);
	
	while( 1 )
	{
		ulTaskNotifyTake( pdFALSE, portMAX_DELAY );

		while( rfifo_len( streamfifo ) > 2 ) 
		{
			rfifo_get( streamfifo, &len, sizeof( short ) );
			while( rfifo_len( streamfifo ) < len )
			{
				vTaskDelay( 1 / portTICK_RATE_MS );
			}
			rfifo_get( streamfifo, pbuff, len );
			waitAckInfo.msgid = *( ( MSGID_Type * ) &pbuff[ 2 + N_LEN ] );
			waitAckInfo.crc   = *( ( CRC_Type* ) &pbuff[ len - N_CRC ] );
			waitAckInfo.recvRespond = 0;
			send_data_lock( pbuff, len );
			
			while( 1 )
			{
				xSemaphoreTake( xWaitAckSemaphore, 5000 / portTICK_RATE_MS );
				if( waitAckInfo.recvRespond == 1 )
				{
					break;
				}
				else
				{
					send_data_lock( pbuff, len );
				}
			}
		}
	}
}





#if 0
char transferAmr = 0;
unsigned int transferSize = 0;
unsigned char transferIndex = 0;
TickType_t sendTickValue = 0;

void transfer_arm( char timeOut )
{
	char *p;//= buff;
	unsigned char md5[16];
	
	if( transferIndex == 0 || transferIndex == 1 )
	{
		if( !timeOut )
			transferIndex++;	
		p[0] = 0xaa;
		p[1] = 0xbb;
		*((short *) &(p[2])) = 22;
		*((short *) &(p[4])) = 0x8001;
		p[6] = TYPE_CMD;
		
		p[7] = transferIndex;
		memcpy( &(p[8]), "test.amr", strlen("test.amr"));
		*((int *) &(p[19])) = 10086;
		caculate_file_md5( "test.amr", md5);		
		memcpy( &(p[23]), md5, 16);
		p[39] = CheckSum( &(p[2]), 36 );
		send_data_lock( p, 40 );
		sendTickValue = xTaskGetTickCount();
		//waitcrc = p[39];
	}
	else
	{
		FIL file;
		unsigned int br;
		
		if( f_open( &file, (const TCHAR*)"0:/amr/test.amr", FA_READ ) == FR_OK )
		{
			if( !timeOut )
			{
				if( transferIndex == 0xff )
				{
					transferIndex = 2;
				}
				else
				{
					transferIndex++;
				}
			}
			f_lseek( &file, transferSize );
			p[0] = 0xaa;
			p[1] = 0xbb;
			*((short *) &(p[2])) = 22;
			*((short *) &(p[4])) = 0x8001;
			p[6] = TYPE_CMD;
			p[7] = transferIndex;
			f_read(&file, &(p[8]), 300, &br);
			if( !timeOut )
				transferSize += br;
			if( br < 300 )
			{
				transferIndex = 0;				
				p[7] = transferIndex;
				transferSize = 0;
			}
			p[7+br+1] = CheckSum( &(p[2]), br + 6);
			send_data_lock( p, br + 9 );
			sendTickValue = xTaskGetTickCount();
			//waitcrc = p[7+br+1];
			f_close( &file );
		}
	}
}

#endif



