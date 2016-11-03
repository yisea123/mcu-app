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
#include "cmdhandler.h"
#include "ymodem.h"
#include "transport.h"
#include "dma.h"
#include "can.h"

#define 	MSG_STATUS_WAITING				0
#define 	MSG_STATUS_ACK					1

extern TaskHandle_t 	pxCanTask;
extern TaskHandle_t     pxUpStreamTask;
Ringfifo 	   	 		uart6fifo;
Ringfifo 	    		upStreamFifo;
Ringfifo 					canfifo;
static WaitAckInfo      waitAckInfo;
static xSemaphoreHandle xWaitAckSemaphore = NULL ;
static xSemaphoreHandle mUartSendMutex = NULL;
xSemaphoreHandle 		xDownStreamSemaphore = NULL ;
static QueueHandle_t  	mStreamMutex = NULL;
xSemaphoreHandle 		xSenderSemaphore = NULL ;
static char 			mSendBuffer[300];

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

static void block_wait_ack( void )
{
	xSemaphoreTake( xWaitAckSemaphore, 300 / portTICK_RATE_MS );
}

static void notify_stream_task( void )
{
	xSemaphoreGive( xWaitAckSemaphore );	
}

static void obtain_uart_mutex( void )
{
	xSemaphoreTake( mUartSendMutex, portMAX_DELAY );	
}

static void release_uart_mutex( void )
{
	xSemaphoreGive( mUartSendMutex );	
}

static void obtain_stream_mutex( void )
{
	xSemaphoreTake( mStreamMutex, portMAX_DELAY );	
}

static void release_stream_mutex( void )
{
	xSemaphoreGive( mStreamMutex );	
}

/*
* author: 	yangjianzhou
* function: 	fill ackBuff by using msg_id and crc.
*/

/*aa bb len(2) msgid(2) type(1) d0~dn = buf, crc(1).     packLen = data len*/
/*aa bb len(1) msgid(2) type(1) d0~dn = buf, crc(2).     packLen = data len*/
CRC_Type calculate_crc( unsigned char *buf, int packLen )
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
* function: 	compose_protocol_command len: data of len, msgid in protocol,
			data: d0~dn,  command is the result of compose.
*/
int compose_protocol_command( LEN_Type len , MSGID_Type msgid,
					char data[], char command[] )
{
	command[0] = 0xAA;
	command[1] = 0xBB;
	*( LEN_Type* )  &command[ 2 ] = len;
	*( MSGID_Type* )&command[ 2 + N_LEN ] = msgid;
	*( TYPE_Type* ) &command[ 2 + N_LEN + N_MSGID ] = TYPE_CMD;
	memcpy( &command[ 2 + N_LEN + N_MSGID + N_TYPE ], data, len );
	*( CRC_Type* )  &command[ 2 + N_LEN + N_MSGID + N_TYPE + *( LEN_Type* )&command[ 2 ] ] = 
		calculate_crc( (unsigned char *)&command[2], N_LEN + N_MSGID + N_TYPE + *( LEN_Type* )&command[ 2 ] );
	return 2 + N_LEN + N_MSGID + N_TYPE + len + N_CRC;
}

/*
* author:	yangjianzhou
* function: 	add_command_to_stream  report command use this function, will send again if no ack.
* 			flag(1) add command until stream has memory to store it.
*			flag(0) add command if stream has memory, else return right now.
*/
int add_command_to_stream( char *buff, short len , char flag )
{
	Ringfifo* streamfifo = &upStreamFifo;

	if( len > streamfifo->size )
	{
		printf("%s: len(%d) error!!\r\n", __func__, len );
		return -1;
	}
	obtain_stream_mutex();
	while( streamfifo->size - rfifo_len( streamfifo ) < len + sizeof( short ))
	{
		if( flag == WAIT_BLOCK )
		{
			vTaskDelay( 1 / portTICK_RATE_MS );
		}
		else
		{
			streamfifo->lostBytes += len;
			release_stream_mutex();
			return -1;
		}
	}
	rfifo_put( streamfifo, &len, sizeof( short ) );
	rfifo_put( streamfifo, buff, ( unsigned int )len );
	release_stream_mutex();

	xTaskNotifyGive( pxUpStreamTask );
	return 0;
}

/*
* author: 	yangjianzhou
* function: 	fill ackBuff by using msg_id and crc.
*/
static void fill_ack_buffer( char *ackbuff, MSGID_Type msg_id, CRC_Type crc )
{
	*( ackbuff ) = 0xaa;
	*( ackbuff + 1 ) = 0xbb;
	*( ( LEN_Type* )( ackbuff + 2 ) ) = N_CRC;
	*( ( MSGID_Type* )( ackbuff + 2 + N_LEN ) ) = msg_id;
	*( ( TYPE_Type* )( ackbuff + 2 + N_LEN + N_MSGID) ) = TYPE_ACK;
	*( ( CRC_Type* )( ackbuff + 2 + N_LEN + N_MSGID + N_TYPE ) ) = crc;
	*( ( CRC_Type* )( ackbuff + 2 + N_LEN + N_MSGID + N_TYPE + N_CRC ) ) =
			calculate_crc( (unsigned char *) ( ackbuff + 2 ), N_LEN + N_MSGID + N_TYPE + N_CRC );
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
static void uart_send_lock( const char* data, int len )
{
	if( len > sizeof( mSendBuffer ) )
	{
		printf("%s: len(%d) error!\r\n", __func__, len);
		return;
	}
	
	obtain_uart_mutex();	
	memcpy( mSendBuffer, data, len );
	
#if( BOARD_NUM == 3 )		
	USART_DMACmd( USART6, USART_DMAReq_Tx, ENABLE );  
	MYDMA_Enable( DMA2_Stream6, len );
#else
	USART_DMACmd( USART3, USART_DMAReq_Tx, ENABLE );  
	MYDMA_Enable( DMA1_Stream3, len );
#endif
	xSemaphoreTake( xSenderSemaphore, 300 / portTICK_PERIOD_MS );

	release_uart_mutex();
}

/*
* author:	yangjianzhou
* function: 	handle_ack_command handle an ack message, then notify the sender task.
*/
static void handle_ack_command( ProtocolAck *rAck )
{
	//printf("%s \r\n", __func__);
	if( rAck->len == N_CRC && rAck->type == TYPE_ACK )
	{
		WaitAckInfo *wait = &waitAckInfo;
		
		if( rAck->msgid == wait->msgid && rAck->crc == wait->crc )
		{
			wait->status = MSG_STATUS_ACK;
			notify_stream_task();
		}
		else
		{
			printf("%s: error ack!\r\n", __func__);
			printf("rAck->msgid(0x%x) rAck->crc(0x%x)!\r\n", rAck->msgid, rAck->crc);
			printf("wait->msgid(0x%x) wait->crc(0x%x)\r\n", wait->msgid, wait->crc );
		}
	}
}

/*
* author: 	yangjianzhou
* function: 	process_ack_command send directly in recevie task or use add_ack_to_stream to 
*			put the ack data in other task.
*/
static void process_ack_command( char* data, int len )
{
	uart_send_lock( data, len );
}

/*
* author:	yangjianzhou
* function: 	handle_remote_command we get an command from android.
*/
static int handle_remote_command( ProtocolData *rData )
{
	int ret = HANDLE_NULL;
	//printf("msgid(0x%04x),len(%d)\r\n", rData->head->msgid, rData->head->len);

	switch( rData->head->msgid )
	{
		case MSG_ANDOIRD_CAN:
			printf("%s: MSG_ANDOIRD_CAN.\r\n", __func__);
			handle_can_command( rData->data, rData->head->len );
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
		case MSG_FILE_OTA:
			handler_file_message( rData->data );
			break;
		default:
			printf("%s: Error msg id.\r\n", __func__);
			break;
	}
	
	return ret;
}	

/*
* author:	yangjianzhou
* function: 	catch_data_frame get a frame data.
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

	//1:	aa bb len(2) msgid(2) type(1) d0~dn crc(1)					
	//2:	aa bb len(1) msgid(2) type(1) d0~dn crc(2)

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
				calcrc = calculate_crc( (unsigned char *) buff, N_LEN + N_MSGID + N_TYPE + datalen );
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
				}
				else 
				{	
					printf("%s: CheckSum Error! type(0x%02x), cal crc(0x%04x),"
						" recv crc(0x%04x)\r\n", __func__, type, calcrc, crc );	
					continue;
				}

			default:
				state = UHEAD1;	
				continue;
		}
		
		break;
	}
	
	return UART_WAIT;
}

/*
* author:	yangjianzhou
* function: 	HandleDownStreamTask  parse uart data form android.
*/
void HandleDownStreamTask( void * pvParameters )
{
	char 	cmdbuff[ 512 ];
	char 	ackbuff[ N_ACK_TOTAL ];
	int 	result = 0;
	int		bufflen;
	ProtocolData rData;
	ProtocolAck  *rAck;
	
	register_rom_handlers();	
	xDownStreamSemaphore 	= xSemaphoreCreateCounting( 0xffffffff, 0 );
	mUartSendMutex 		 	= xSemaphoreCreateMutex();		
	waitAckInfo.status 		= MSG_STATUS_ACK;
	waitAckInfo.time 		= 0;
	vSetTaskLogLevel( NULL, eLogLevel_3 );
	printf("%s: start...\r\n", __func__);
	
	while( 1 )
	{
		xSemaphoreTake( xDownStreamSemaphore, portMAX_DELAY );

		while( rfifo_len( &uart6fifo ) > 0 ) 
		{
			result = catch_data_frame( &uart6fifo, cmdbuff, &bufflen );
			if( result == UART_CMD ) 
			{
				format_protocol_data( &rData , cmdbuff, bufflen );
				fill_ack_buffer( ackbuff, rData.head->msgid, rData.crc );
				process_ack_command( ackbuff,  N_ACK_TOTAL );
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

/*
* author:	yangjianzhou
* function: 	HandleUpstreamTask  handle all command or ack which need to be sended to android.
*/
void HandleUpstreamTask( void * pvParameters )
{
	short 	  len;
	char 	  buffer[300];
	Ringfifo* streamfifo = &upStreamFifo;
	WaitAckInfo *wait 	 = &waitAckInfo;
	wait->crc 			 = 0xFF;
	mStreamMutex  		 = xSemaphoreCreateMutex();
	vSetTaskLogLevel( NULL, eLogLevel_3 );	
	rfifo_init( streamfifo, 1024 * 8 );	
	vSemaphoreCreateBinary( xWaitAckSemaphore ); 	
	xSemaphoreTake( xWaitAckSemaphore, 0 );
	vSemaphoreCreateBinary( xSenderSemaphore ); 	
	xSemaphoreTake( xSenderSemaphore, 0 );

#if( BOARD_NUM == 3 )	
	MYDMA_Config( DMA2_Stream6, DMA_Channel_5, ( unsigned int )&USART6->DR,
			( unsigned int )mSendBuffer, sizeof( mSendBuffer ) );
#else
	MYDMA_Config( DMA1_Stream3, DMA_Channel_4, ( unsigned int )&USART3->DR,
			( unsigned int )mSendBuffer, sizeof( mSendBuffer ) );
#endif	

	vTaskDelay( 40 / portTICK_RATE_MS );
	printf("%s: start...\r\n", __func__);
	
	while( 1 )
	{
		wait->status = MSG_STATUS_ACK;	
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );

		while( rfifo_len( streamfifo ) > 2 ) 
		{
			rfifo_get( streamfifo, &len, sizeof( short ) );
			while( rfifo_len( streamfifo ) < len )
			{
				vTaskDelay( 1 / portTICK_RATE_MS );
			}
			rfifo_get( streamfifo, buffer, len );
			wait->msgid = *( ( MSGID_Type * ) &buffer[ 2 + N_LEN ] );
			wait->crc   = *( ( CRC_Type* ) &buffer[ len - N_CRC ] );			
			wait->status = MSG_STATUS_WAITING;			
			do {
				uart_send_lock( buffer, len );
				block_wait_ack();
				if( wait->status != MSG_STATUS_ACK ) {
					printf("%s: send again\r\n", __func__);
				}
			} while( wait->status != MSG_STATUS_ACK );
		}
	}
}

/*
* author:	yangjianzhou
* function: 	process_can_data  put can data to stream.
*			aa bb len msgid type id(4) canx(1) d0~d8(8) crc1 crc2
*/
void process_can_message(unsigned int id, unsigned char buffer[])
{	
	int 	len;
	char 	data[13];
	char 	command[ 2 + N_LEN + N_MSGID + N_TYPE + 13 + N_CRC ];

	memcpy( data, &id, sizeof( unsigned int ) );
	*( data + sizeof( unsigned int ) ) = 0;
	memcpy( data + sizeof( unsigned int ) + 1, buffer, 8 );
	
	len = compose_protocol_command( 13, 0x8001, data, command );
	add_command_to_stream( command,  len, WAIT_NOT );	
}

/*
* author:	yangjianzhou
* function: 	HandleCanTask  handle can interrupt data, push to sender task stream.
*/
void HandleCanTask( void * pvParameters )
{
	unsigned int id;
	unsigned char message[8];
	
	rfifo_init( &canfifo, 1024 * 4 );	
	CAN1_Mode_Init( CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 6, CAN_Mode_Normal );	
	CAN2_Mode_Init( CAN_SJW_1tq, CAN_BS2_6tq, CAN_BS1_7tq, 6, CAN_Mode_Normal );	
	vSetTaskLogLevel( NULL, eLogLevel_3 );
	printf("%s: start...\r\n", __func__);
 
	while( 1 )
	{
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		while( rfifo_len( &canfifo ) >= sizeof(unsigned int) + sizeof(unsigned char) * 8 )
		{
			rfifo_get( &canfifo, &id, sizeof(unsigned int) );
			rfifo_get( &canfifo, message, sizeof(unsigned char) * 8 );
			process_can_message( id, message );
		}
	}

}




/*
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
*/

