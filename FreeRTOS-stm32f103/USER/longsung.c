#include "longsung.h"

Ringfifo uart2fifo[1];
extern eLogLevel ucOsLogLevel;
extern TaskHandle_t pxModuleTask;

extern void gprs_reset_height( void );
extern void gprs_reset_low( void );
extern void printf_system_jiffies( void );
static int get_at_command_count( struct list_head *head );

/************************************tools codes************************************/

void print_char( USART_TypeDef* USARTx, char ch )
{
	USART_ClearFlag( USARTx, USART_FLAG_TC ); 
	USART_SendData( USARTx, ch );
	while( USART_GetFlagStatus( USARTx, USART_FLAG_TC ) != SET );
}

static void select_sort( int a[], int len )  
{  
    int i,j,x,l;
	
    for( i = 0; i < len; i++ )  
    {  
        x = a[i];  
        l = i;
		
        for( j = i; j < len; j++ )  
        {  
            if( a[j] < x )  
            {  
                x = a[j];  
                l = j;  
            }  
        }  
        a[l] = a[i];  
        a[i] = x;  
    }  
}  

static int is_little_endian()
{
	int wTest = 0x12345678;
	short *pTest = ( short* )&wTest;
	//printf("%s:%04x\r\n", __func__, pTest[0]);
	return !( 0x1234 == pTest[0] );
}

/*网络字节是大端*/
unsigned int htonl( uint32_t hostlong )
{
	int value;
	char *p, *q;
	
	if( is_little_endian() ) 
	{
		p = ( char* )&value;
		q = ( char* )&hostlong;
		*p = *( q + 3 );
		*( p + 1 ) = *( q + 2 );
		*( p + 2 ) = *( q + 1 );
		*( p + 3 ) = *q;
	} 
	else 
	{
		value = hostlong;
	}
	
	//printf("value=%04x, hostlong=%04x\r\n", value, hostlong);
	return value;
}

unsigned int ntohl( uint32_t netlong )
{
	int value;
	char *p, *q;
	
	if( is_little_endian() ) 
	{
		p = ( char* )&value;
		q = ( char* )&netlong;
		*p = *( q + 3 );
		*( p + 1 ) = *( q + 2 );
		*( p + 2 ) = *( q + 1 );
		*( p + 3 ) = *q;
	} 
	else 
	{
		value = netlong;
	}
	
	//printf("value=%04x, netlong=%04x\r\n", value, netlong);
	return value;	
}

/*send AT commnad to 4G*/
static void send_hex_command( const char* ack, int ack_len )
{
	int t;
	
	for( t = 0; t < ack_len; t++ ) 
	{
		USART_ClearFlag( USART2, USART_FLAG_TC ); 
		USART_SendData( USART2, ack[t] );
		while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) != SET );
	}		

}

/*send AT commnad to 4G*/
static void send_at_command( const char* ack, int ack_len )
{
	int t;
	
	for( t = 0; t < ack_len; t++ ) 
	{
		USART_ClearFlag( USART2, USART_FLAG_TC ); 
		USART_SendData( USART2, ack[t] );
		while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) != SET );
	}		
	
	/*add \r for AT command*/
	USART_ClearFlag( USART2, USART_FLAG_TC ); 
	USART_SendData( USART2, 0x0d );	
	while( USART_GetFlagStatus( USART2, USART_FLAG_TC ) != SET );	
}

static void at( char *at )
{
	int len;
	
	len = strlen( at );
	send_at_command( at, len );
}

void *memchr_x( const void *s, int c, int count )
{
	const unsigned char *p = s;

	while( count-- )
	{
		if ( ( unsigned char )c == *p++ )
		{
			return ( void * )( p - 1 );
		}
	}
	
	return NULL;
}

int memcmp_x(const void *cs, const void *ct, int count)
{
	const unsigned char *su1 = cs, *su2 = ct, *end = su1 + count;
	int res = 0;

	while( su1 < end ) 
	{
		res = *su1++ - *su2++;
		if ( res )
			break;
	}
	
	return res;
}

static int str_2_int( const char* p, const char* end )
{
	int   result = 0;
	int   len    = end - p;

	for( ; len > 0; len--, p++ )
	{
		int  c;

		if ( p >= end )
		{
			goto Fail;
		}
		c = *p - '0';
		if ( ( unsigned int ) c >= 10 )
		{
			goto Fail;
		}
			result = result*10 + c;
	}
	
	return  result;

Fail:
	return -1;
}


static uint8_t str_to_hex( char *src )
{
	uint8_t s1,s2;

	s1 = toupper( src[0] ) - 0x30;
	
	if ( s1 > 9 ) 
	{
		s1 -= 7;
	}
	
	s2 = toupper( src[1] ) - 0x30;
	
	if ( s2 > 9 )
	{
		s2 -= 7;
	}
	
	return ( s1*16 + s2 );
}

static int is_ip_valid( RemoteTokenizer *tzer )
{
	int i = 0;
	int ipaddr[4];

	for( i = 0; i< 4; i++)
	{
		ipaddr[i] = str_2_int( tzer->tokens[i].p, tzer->tokens[i].end );
		if( ipaddr[i] == -1 || tzer->tokens[i].p == tzer->tokens[i].end )
		{
			printf("%s: is invalid ip!\r\n", __func__);
			return 0;
		}
	}

    if ( ( ipaddr[0] >= 0 && ipaddr[0] <= 255 ) && ( ipaddr[1] >= 0 && ipaddr[1] <= 255 )
			&&( ipaddr[2] >= 0 && ipaddr[2] <= 255 ) && ( ipaddr[3] >= 0 && ipaddr[3] <= 255 ) )
    {   
		//printf("%s: is valid ip!\r\n", __func__);    
        return 1;       
    }
    else
    {
		printf("%s: is invalid ip!\r\n", __func__);
        return 0;
    }

}

/*
* author:	yangjianzhou
* function: 	module_tokenizer_init,  format data to Token.
*/
static int module_tokenizer_init( RemoteTokenizer* t, char c, const char* p, const char* end )
{
	int count = 0;

	// remove trailing newline
	if ( end > p && end[-1] == '\n' ) 
	{
			end -= 1;
			if ( end > p && end[-1] == '\r' )
				end -= 1;
	}	

	while ( p < end ) 
	{
		const char*  q = p;

		q = memchr(p, c, end-p);
		if ( q == NULL )
			q = end;

		if ( q >= p ) 
		{
			if ( count < MAX_TOKENS ) 
			{
				t->tokens[count].p   = p;
				t->tokens[count].end = q;
				count += 1;
			}
		}
		if ( q < end )
			q += 1;

		p = q;
	}

	t->count = count;

	return count;		
}

/*
* author:	yangjianzhou
* function: 	module_tokenizer_get,  get a Token.
*/
static Token module_tokenizer_get( RemoteTokenizer* t, int index )
{
	Token  tok;
	static const char*  dummy = "";

	if ( index < 0 || index >= t->count ) 
	{
		tok.p = tok.end = dummy;
	} 
	else 
	{
		tok = t->tokens[index];
	}

	return tok;
}

/***************************************instance codes***************************************/

/*longsung communication module, instance ComModule codes*/

static void print_line( USART_TypeDef* USARTx, const char* data, int len )
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
		/*we just print charactor*/
		if( data[t] >= 0x20 || ( data[t] == 0x0d || data[t] == 0x0a ) )
		{
			USART_ClearFlag( USARTx, USART_FLAG_TC ); 
			USART_SendData( USARTx, data[t] );
			while( USART_GetFlagStatus( USARTx, USART_FLAG_TC ) != SET );
		}
	}		
}


/************************************instance codes************************************/

//do not define 1 for AT command, used by MATT

#define A8_CREG_ASK				2
#define A8_CREG_SET				3
#define A8_CGREG_SET			4
#define A8_CSQ					5
#define A8_MIPCALL_SET			6  //PPP connect
#define A8_MIPOPEN				7
#define A8_MIPOPEN_ASK		    8
#define A8_MIPMODE				9
#define A8_MIPSEND				10
#define A8_MIPCLOSE_ASK			11
#define A8_MIPCLOSE_SET			12
#define A8_MIPCALL_CLEAN		13  // PPP disconnect
#define A8_MIPCALL_ASK			14
#define A8_MIPPUSH				15


typedef struct LongSungPriv
{
	char rege_status;
	char cgatt_status;

	char sm_data_flag;	
}LongSungPriv;

/*
* author:	yangjianzhou
* function: 	longsung_power_reset.
* 			
*/
void longsung_power_reset( void *argc )
{
	printf("%s \r\n", __func__);

	gprs_reset_height();
	vTaskDelay( 400 / portTICK_RATE_MS );
    gprs_reset_low();
}

/*
* author:	yangjianzhou
* function: 	longsung_hardware_reset,  power reset callback, when reset the power
* 			we need to clean module data, include private data.
*/
static void longsung_hardware_reset_callback( void *instance )
{
	LongSungPriv *p = ( LongSungPriv *) ( ( ComModule *) instance )->priv;	
	( ( ComModule *) instance )->is_initialise = 0;	

	memset( p, 0, sizeof( LongSungPriv ) );
	
	printf("%s:\r\n", __func__);
}

/*
* author:	yangjianzhou
* function: 	longsung_initialise_module,  init module by AT command.
*/
static void longsung_initialise_module( void *instance )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;
	( ( ComModule *) instance )->is_initialise = 1;

	dev->ops->make_at_command( dev, A8_CREG_SET, WAIT_NOT, ONE_SECOND/10, ONE_SECOND * 5, 0 );
	dev->ops->make_at_command( dev, A8_CGREG_SET, WAIT_NOT, ONE_SECOND/10, 0, 0 );
	dev->ops->make_at_command( dev, A8_CSQ, WAIT_NOT, ONE_SECOND/10, 0, 0 );	
	dev->ops->make_at_command( dev, A8_CREG_ASK, WAIT_NOT, ONE_SECOND/10, 0, 0 );	
	dev->ops->make_at_command( dev, A8_MIPMODE, WAIT_NOT, ONE_SECOND/20, ONE_SECOND * 2, 0 );						
	dev->ops->make_at_command( dev, A8_MIPCALL_SET, WAIT_NOT, ONE_SECOND, ONE_SECOND*6, 0 );
}

/*
* author:	yangjianzhou
* function: 	longsung_poll_signal,  poll signal data.
*/
static void longsung_poll_signal( void *instance )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	dev->ops->make_at_command( dev, A8_CSQ, WAIT_NOT, ONE_SECOND/10, 0, 0 );
}

static void longsung_check_sm( void *instance )
{
	//DevStatus *dev = ( ( ComModule *) instance )->p_dev;
}

static void longsung_check_ip( void *instance )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	/*查询IP*/
	dev->ops->make_at_command( dev, A8_MIPCALL_ASK, WAIT_NOT, ONE_SECOND/20, 0, 0 );
}

static void longsung_close_socket( void *instance )
{
	int i;
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;
	
	/*检查是否已经发过ATMIPCLOSE*/
	if( CHECK_EXIST_NOT == dev->ops->check_at_command_exist( dev, A8_MIPCLOSE_SET ) ) 
	{ 
		for( i = 0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
		{
			if( dev->socket_open[i] != -1 ) 
			{
				dev->ops->make_at_command( dev, A8_MIPCLOSE_SET, WAIT_NOT, ONE_SECOND/5, 0, dev->socket_open[i] );
			}
		}
	}

}

static void longsung_request_ip( void *instance )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	printf("%s:\r\n", __func__);
	dev->ops->make_at_command(dev, A8_MIPCALL_SET, WAIT_NOT, ONE_SECOND/5, 0, 0);	
}

static void longsung_connect_server( void *instance )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	if( CHECK_EXIST_NOT == dev->ops->check_at_command_exist( dev, A8_MIPOPEN ) )
	{
		/*取消上一次要关闭tcp的请求*/
		if( dev->close_tcp_interval ) 
		{
			dev->close_tcp_interval = 0;//动作
		}
		/*发ATMIPOPEN ONE_SECOND/2 后才可发ATMIPHEX， 否则ATMIPHEX失败*/
		if( CHECK_EXIST_NOT == dev->ops->check_at_command_exist( dev, A8_MIPMODE ) )
		{
			dev->ops->make_at_command(dev, A8_MIPMODE, WAIT_NOT, ONE_SECOND/20, ONE_SECOND * 2, 0 );							
		}
		dev->ops->make_at_command(dev, A8_MIPOPEN, WAIT, ONE_SECOND/2, ONE_SECOND * 15, 0 );
		printf("%s: start tcp connect remote server.\r\n", __func__);
	}
}

static void longsung_push_socket_data( void *instance, unsigned int tick )
{
	//DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	//dev->ops->make_at_command( dev, A8_MIPPUSH, WAIT_NOT, tick, 0, 0 );
}

/*
* author:	yangjianzhou
* function: 	list_sm_event,  poll sm event.
*/
static void longsung_delete_sm( void *instance, int index )
{
	//DevStatus *dev = ( ( ComModule *) instance )->p_dev;
}

/*
* author:	yangjianzhou
* function: 	list_sm_event,  poll sm event.
*/
static void longsung_read_sm( void *instance, int index )
{
	//DevStatus *dev = ( ( ComModule *) instance )->p_dev;
}

static void longsung_do_read_sm( DevStatus *dev , int index )
{

}

static void longsung_do_delete_sm( int index )
{

}

static void longsung_do_close_socket( int index )
{
	char cmd[15];
	
	memset(cmd, '\0', sizeof(cmd));
	sprintf(cmd, "AT+MIPCLOSE=%d", index);
	
	at( cmd );
}

static void longsung_do_send_command( void *instance, AtCommand* cmd )
{
	DevStatus *dev = ( ( ComModule *) instance )->p_dev;

	if( !cmd || !dev )
	{
		printf("%s: dev or cmd null pointer error!\r\n", __func__);	
		return;
	}
	dev->at_count++;

	switch( cmd->index )
	{
		case A8_CREG_ASK: 
			at( "AT+CREG?" );			
			break;
/*
AT+CREG?

+CREG: 0,1

OK
*/
		case A8_CREG_SET: 
			at( "AT+CREG=1" ); 
			break;
/*
AT+CGREG=1

OK
*/
		case A8_CSQ: 
			at( "AT+CSQ" ); 
			break;
/*
AT+CSQ

+CSQ: 17,99

OK
*/
		case A8_CGREG_SET: 
			at( "AT+CGREG=1" ); 
			break;
/*
AT+CGREG=1

OK
*/			
		case A8_MIPCALL_SET: 
			at( "AT+MIPCALL=1,\"CMNET\"" ); 
			break;
//41 54 2B 4D 49 50 43 41 4C 4C 3D 31 2C 22 43 4D 4E 45 54 22 0D
/*
AT+MIPCALL=1,"CMNET"

OK

+MIPCALL: 1//出现这个时，才可以进行TCP
*/
		case A8_MIPCALL_CLEAN: 
			at( "AT+MIPCALL=0" ); 
			break;
/*
AT+MIPCALL=0

OK
----------------------------------
AT+MIPCALL?

+MIPCALL: 0,0.0.0.0,0.0.0.0,0.0.0.0

OK
*/
		case A8_MIPCALL_ASK: 
			at( "AT+MIPCALL?" ); 
			break;

		case A8_MIPMODE: 
			at( "AT+MIPMODE=1,0,0" ); // 1:hex 0:ascii, 1:缓存 0:不缓存，
			break;
//41 54 2B 4D 49 50 4D 4F 44 45 3D 31 2C 31 2C 30 0D
			
		case A8_MIPOPEN_ASK: 
			at( "AT+MIPOPEN?" );
			break;//+MIPOPEN:1
			
		case A8_MIPPUSH: 
				//char c = 0X1A;
				//send_at_command( &c, 1 );
			break;
/*
AT+MIPCALL?

+MIPCALL: 1,10.179.225.9,210.21.4.130,221.5.88.88

OK
*/
		case A8_MIPOPEN: 
			if( dev->mqtt_dev->iot ) 
			{
				at( "AT+MIPOPEN=1,\"TCP\",\"198.41.30.241\",1883,0" );
			}
			else 
			{
				at( "AT+MIPOPEN=1,\"TCP\",\"112.124.102.62\",1883,0" );
				//at( "AT+MIPOPEN=1,0,\"198.41.30.241\",1883,0" );
			}
//41 54 2B 4D 49 50 4F 50 45 4E 3D 31 2C 22 54 43 50 22 2C 22 31 31 32 2E 31 32 34 2E 31 30 32 2E 36 32 22 2C 31 38 38 33 2C 30 0D 
			break;
/*
AT+MIPOPEN=1,"TCP","112.124.102.62",1883,0

OK

+MIPOPEN: 1,1

+MIPCLOSE: 1,0

------------------
AT+MIPSEND=1,5
>3132333435[ctrl+z:1A]
+MIPSEND:1,11

18-02-39: AT+MIPSEND=1,"

102300064d5149736470031601cc0009796a69616e7a686f7500036d6375000564656174681A

18-02-39: +MIPSEND:1,1463////37

AT+MIPSEND=1,37
//41 54 2B 4D 49 50 53 45 4E 44 3D 31 2C 33 37 0D 

*/			
		case ATMQTT: 
			dev->mqtt_dev->ops->mqtt_prepare_packet( dev->mqtt_dev, cmd ); 
			break;
			
		default:
			dev->at_count--;
			printf("%s: AT command error!\r\n", __func__);
			break;
	}
}

static char *longsung_at_get_name( void *instance, int index )
{
	instance = instance;
	
	switch( index )
	{
		case A8_CREG_ASK:
			return "ATCSQ";
			
		case A8_CREG_SET:
			return "ATCSQ";
			
		case A8_CGREG_SET:
			return "ATCSQ";	
			
		case A8_CSQ:
			return "ATCSQ";	
			
		case A8_MIPCALL_SET:
			return "ATCSQ";	
			
		case A8_MIPOPEN:
			return "ATCSQ";	
			
		case A8_MIPOPEN_ASK:
			return "ATCSQ";	
			
		case A8_MIPMODE:
			return "ATCSQ";		
			
		case A8_MIPSEND:
			return "ATCSQ";		
			
		case A8_MIPCLOSE_ASK:
			return "ATCSQ";		
			
		case A8_MIPCLOSE_SET:
			return "ATCSQ";		
			
		case A8_MIPCALL_CLEAN:
			return "ATCSQ";			
			
		case A8_MIPCALL_ASK:
			return "ATCSQ";			
			
		case A8_MIPPUSH:
			return "ATCSQ";				
	
		case ATMQTT:
			return "ATMQTT";
			
		default: 
			return "AT UNKNOW";
	}
}

/*
longsung_make_tcp_packet: tcp data len(21)
sending [SUBSCRIBE]! mqtype=8,id=0x0001
AT+MIPSEND=1,"82130001000e2f73797374656d2f73656e736f7202"
+MIPSEND:1,1479

OK
ADD [SUBSCRIBE] to mqtt_head to wait ACK! msgid(0x0001), qos(0)
*/
static char* longsung_make_tcp_packet( char* buff, unsigned char* data, int len )
{
	char *p;	
	int i;

	if( !data || !buff ) 
		return NULL;
	
	//sprintf( buff, "AT+MIPSEND=1,%d", len );
	//p = buff + strlen( "AT+MIPSEND=1,\"" );

	p = buff;
	
	for( i = 0; i < len; i++ ) 
	{
		sprintf( p + 2 * i, "%02x", *( data+i ) );
	}
	//p[ 2 * i ] = '\"';

	printf("%s: tcp data len(%d) buff(%s), strlen(%d)\r\n", __func__,
			len, buff, strlen(buff));
	return buff;
}

static void longsung_send_tcp_packet( char* buff , unsigned char* data, int len )
{
	char ctrz = 0X1A;//ctrl + z
	char cmd[29];

	at( "AT+MIPMODE=1,0,0" );
	vTaskDelay(10);

	memset( cmd, 0, sizeof(cmd));
	sprintf( cmd, "AT+MIPSEND=1,%d", len );//AT+MIPSEND=1,37
	at( cmd );
	vTaskDelay(20);
	send_hex_command( buff, strlen(buff) );
	send_at_command( &ctrz, 1 );
}

static void longsung_send_push_directly( void *instance )
{	
	instance = instance;
	//at( "AT+MIPPUSH=1" );
}

static void longsung_send_close_socket_directly( void *instance, int index )
{
	char cmd[18];
	
	instance = instance;
	memset( cmd, '\0', sizeof( cmd ) );
	sprintf( cmd, "AT+MIPCLOSE=%d", index );
	at( cmd );
}

static int longsung_is_tcp_connect_cmd( void *instance, int index )
{	
	instance = instance;
	
	return index == A8_MIPOPEN ? 1: 0;
}

/* TCP 消息处理函数 */
/*
+MIPRTCP=1,4,20020000
check_packet_from_fixhead: msg_len = nbytes = 4
received (CONNACK)!

+MIPRTCP=1,5,9003000102
check_packet_from_fixhead: msg_len = nbytes = 5
received (SUBACK)! msg_type(9), QOS=0, msg_id=0X01, mesg_len=5, nbytes=5
mqtt_parse_packet: subscribe success.


+MIPRTCP=1,2,D000
check_packet_from_fixhead: msg_len = nbytes = 2
received (PINGRESP)! msg_type(13), QOS=0, msg_id=0X00, mesg_len=2, nbytes=2

+MIPRTCP=1,4,40020003
check_packet_from_fixhead: msg_len = nbytes = 4
received (PUBACK)!
*/
//+MIPDATA: 1,4,20020000

static void on_longsung_tcp_data_callback( void *priv, RemoteTokenizer *tzer, Token* tok )
{
	int i, nbytes;
	static unsigned char tcp_read[750];
	DevStatus *dev = ( DevStatus* ) priv;	
	mqtt_dev_status *mqtt_dev = dev->mqtt_dev;
	
	memset( tcp_read, '\0', sizeof( tcp_read ) );
	nbytes = str_2_int( tok[1].p, tok[1].end );//nbytes <= 750
	
	if( nbytes == -1 )
	{
		printf("%s: nbytes(%d) error!\r\n", __func__, nbytes);
		return;
	}
	if( nbytes > sizeof( tcp_read ) )
	{
		printf("%s: nbytes(%d) error!\r\n", __func__, nbytes);
		return;
	}

	mqtt_dev->recv_bytes += nbytes;

	if( !mqtt_dev->parse_packet_flag )
	{
		printf("%s: drop %d bytes. waitting mqtt reset finish...\r\n", __func__, nbytes);
		return;
	}
	for( i = 0; i < nbytes; i++ )
	{
		tcp_read[i] = str_to_hex( ( char* ) tok[2].p + 2  * i );
	}	
	
	mqtt_dev->ops->mqtt_protocol_parse( mqtt_dev, tcp_read, nbytes );
}

static void on_longsung_connect_service_success_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	//+MIPOPEN: 1,1
	DevStatus *dev = ( DevStatus* ) priv;		
	int i, socketId = str_2_int( tok[0].p + 10, tok[0].end );

	//printf("[%s: socketId = %d]\r\n", __func__, socketId);
	switch( socketId )
	{
		case 1:
		case 2:
		case 3:
		case 4: 
			i = socketId - 1; 
			break;
			
		default: 
			if( socketId == -1 )
			{
				printf("%s: socketId(%d) error!\r\n", __func__, socketId);
			}			
			return;
	}
	
	vTaskDelay(400);
	dev->socket_open[i] = socketId;
	dev->tcp_connect_status = 1;
	printf("%s:TCP connect success! socket id=%d\r\n", __func__, socketId);
	dev->ops->atcmd_set_ack( dev, A8_MIPOPEN );
	dev->mqtt_dev->parse_packet_flag = 1;
	dev->tcp_connect_times++;
}

static void on_longsung_connect_service_fail_callback( void *priv, RemoteTokenizer *tzer)
{
	//+MIP:ERROR
	DevStatus *dev = ( DevStatus* ) priv;	

	printf("[%s, TCP connect error.]\r\n", __func__);
	dev->tcp_connect_status = 0;
	dev->ops->atcmd_set_ack( dev, A8_MIPOPEN);
}

static void on_longsung_disconnect_service_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	//+MIPCLOSE: 1,0
	int socketId = str_2_int( tok[0].p + 11, tok[0].end );	
	DevStatus *dev = ( DevStatus* ) priv;		

	printf("%s: +MIPCLOSE !!!!! success. socket id = %d\r\n", __func__, socketId);
	if( socketId < -1 || socketId > 3)
	{
		printf("%s: socketId(%d) error!\r\n", __func__, socketId);
		return;
	}

	dev->socket_open[ socketId - 1 ] = -1;
	dev->mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
	dev->mqtt_dev->in_pos = 0;
	dev->mqtt_dev->in_waitting = 0;	
	dev->mqtt_dev->fixhead = 1;
	
	dev->tcp_connect_status = 0;
	dev->ops->set_mqtt_cmd_clean( dev );
	//printf("[tcp connect close...socketId=%d]\r\n", socketId);
	//printf("%d, %d, %d, %d\r\n", devStatus->socket_open[0], devStatus->socket_open[1]
	//	, devStatus->socket_open[2], devStatus->socket_open[3]);
	//dev->connect_flag = 0;
}

/*******************************************
+CSQ: 7,99
********************************************/
static void on_longsung_signal_strength_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	int signal[2];
	DevStatus *dev = ( DevStatus* ) priv;	

	dev->boot_status = 1;	
	dev->simcard_type = 3;
	signal[0] = str_2_int( tok[0].p+6, tok[0].end );
	signal[1] = str_2_int( tok[1].p, tok[1].end );
	dev->rcsq++;
	dev->singal[0] = signal[0];
	dev->singal[1] = signal[1];
}

static void on_longsung_at_command_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	
	DevStatus *dev = ( DevStatus* ) priv;	

	memset(dev->at_sending, '\0', sizeof(dev->at_sending));
	if( strlen( tok[0].p ) > sizeof( dev->at_sending ) ) 
	{
	 	memcpy( dev->at_sending, tok[0].p, sizeof( dev->at_sending ) );
	}
	else
	{
		memcpy(dev->at_sending, tok[0].p, strlen(tok[0].p) );
	}
	//printf("at_sending=%s, len=%d\r\n", at_sending, strlen(at_sending));
}

static void on_longsung_at_cmd_success_callback( void *priv, RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	//printf("[at command ok... tzer->count=%d]\r\n", tzer->count);
	DevStatus *dev = ( DevStatus* ) priv;	

	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) 
	{
		//printf("----------------------------------\r\n");
		//printf("[result: OK] %s\r\n", dev[0].at_sending);
		
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
			for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++) 
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
	else if( !memcmp(dev->at_sending, "AT+MIPHEX=1", strlen("AT+MIPHEX=1")))
	{
		printf("%s: success to cmd AT+MIPHEX=1\r\n", __func__);
		dev->ops->atcmd_set_ack( dev, A8_MIPMODE );
	}
	
	memset(dev->at_sending, '\0', sizeof(dev->at_sending));
}

static void on_longsung_at_cmd_fail_callback( void *priv, RemoteTokenizer *tzer)
{
	//printf("%s!\r\n", __func__);
	DevStatus *dev = ( DevStatus* ) priv;	

	if( !memcmp(dev->at_sending, "AT+MIPPUSH=1", strlen("AT+MIPPUSH=1")) ) 
	{
		if( dev->socket_num > 0 )
		{
			dev->socket_close_flag = 1;
		}
		else if( dev->socket_num == 0 && dev->tcp_connect_status )
		{
			dev->tcp_connect_status = 0;
			if( dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT )
			{
				dev->mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
			}
			printf("%s: catch bug#####\r\n", __func__);
		}
		printf("[%s: ERROR AT:%s\r\n", __func__, dev->at_sending);
	} 
	else if(!memcmp(dev->at_sending, "AT+MIPOPEN=1,0,\"", strlen("AT+MIPOPEN=1,0,\""))) 
	{
		//AT+MIPOPEN=1,0,"
		printf("[TCP CONNECT FAIL! AT: %s", dev->at_sending);
		if( dev->socket_num > 0 )
		{
			dev->socket_open[0] = 1;
			dev->socket_open[1] = 2;
			dev->socket_open[2] = 3;
			dev->socket_open[3] = 4;
		}
		dev->tcp_connect_status = 0;
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
	else if( !memcmp(dev->at_sending, "AT+MIPCLOSE=", strlen("AT+MIPCLOSE=")) )
	{
		int socketId = str_2_int( dev->at_sending+12, dev->at_sending+13 );
		
		printf("%s: close socketid[%d] error! AT:%s\r\n", __func__, socketId, dev->at_sending);
		if( socketId < 0 || socketId > 3 )
		{
			printf("%s: socketId(%d) error!\r\n", __func__, socketId);
			return;
		}		
		dev->socket_open[ socketId - 1 ] = -1;  //bug???? jzyang need to check later
	}
	else if( !memcmp(dev->at_sending, "AT+MIPHEX=", strlen("AT+MIPHEX=")))
	{
		printf("%s: fail to cmd AT+MIPHEX\r\n", __func__);
	}
	memset(dev->at_sending, '\0', sizeof(dev->at_sending));	
}

static void on_longsung_simcard_type_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	/*+SIMTEST:3; 3
	AT+SIMTEST?
	+SIMTEST:0
	+SIMTEST:0*/
	DevStatus *dev = ( DevStatus* ) priv;

	dev->simcard_type = str_2_int( tok[0].p+9, tok[0].p+10 );
	dev->boot_status = 1;
	//if( CHECK_EXIST == dev->ops->check_at_command_exist( dev, ATSIMTEST ) )
	//{
	//	dev->ops->atcmd_set_ack( dev, ATSIMTEST );
	//}	
	printf("%s: simcard_type(%d)\r\n", __func__, dev->simcard_type);
}

static void on_longsung_sm_check_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	int i;
	DevStatus *dev = ( DevStatus* ) priv;	

	if( tzer->count == 2 ) 
	{
		/*+CMGD: (),(0-4)*/
		/*+CMGD: (0),(0-4)*/
		if( !memcmp(tok[0].p, "+CMGD: ()", strlen("+CMGD: ()")) ) 
		{
			i = 0;
			dev->sm_num = 0;
		} 
		else 
		{
			i = 1;
			dev->sm_num = 1;
			dev->sm_index[0] = str_2_int( tok[0].p+8, tok[0].end-1 );
		}
		for( ; i < SIZE_ARRAY( dev->sm_index ); i++ ) 
		{
			dev->sm_index[i] = -1;
		}
	} 
	else if( tzer->count > 2 ) 
	{
		/*+CMGD: (0,1,2,4,5,6),(0-4)*/
		dev->sm_num = tzer->count - 1;
		if( dev->sm_num > sizeof(dev->sm_index)/sizeof(dev->sm_index[0]) ) 
		{
			dev->sm_num = sizeof(dev->sm_index)/sizeof(dev->sm_index[0]);
		}
		
		for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++) 
		{
			if( i < dev->sm_num ) 
			{
				if( i==0 ) 
				{
					dev->sm_index[i] = str_2_int( tok[i].p+8, tok[i].end );
				}
				else if( i== (dev->sm_num -1) )
				{
					dev->sm_index[i] = str_2_int( tok[i].p, tok[i].end-1 );
				}
				else 
				{
					dev->sm_index[i] = str_2_int( tok[i].p, tok[i].end );
				}
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
		if( dev->sm_index[i] != -1 )
		{
			printf("%d, ", dev->sm_index[i]);
		}
	}
	printf("}\r\n");
}

static void on_longsung_sm_read_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	DevStatus *dev = ( DevStatus* ) priv;	
	LongSungPriv *p = ( LongSungPriv *) dev->module->priv;

	printf("SM read, index=%d\r\n", dev->sm_index_read);
	p->sm_data_flag = 1;
}

static void on_longsung_sm_data_callback( void *priv, RemoteTokenizer *tzer, Token* tok, int index)
{
	DevStatus *dev = ( DevStatus* ) priv;	

	printf("SM data, index=%d\r\n", index);
	printf("DATA:%s", tok[0].p);
	/*只可读出一行，因此，短信内容不能有\n分行！*/
	dev->sm_index_delete = index;
	dev->sm_delete_flag = 1;
	/*读取到短信内容，删除之*/
}

static void on_longsung_sm_notify_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	/*+CMTI: "SM",1*/
	int i, index, addflag = 1;
	DevStatus *dev = ( DevStatus* ) priv;	
	index = str_2_int( tok[1].p, tok[1].end );
	printf("SM notify: index=%d\r\n", index);
	
	if( index != -1 ) 
	{
		if( dev->sm_num >= sizeof(dev->sm_index)/sizeof(dev->sm_index[0]) )
		{
			return;
		}
		for( i = 0; i<dev->sm_num; i++ ) 
		{
			if( index == dev->sm_index[i] ) 
			{
				addflag = 0;
			}
		}
		
		if( addflag ) 
		{
			dev->sm_index[dev->sm_num] = index;
			dev->sm_num++;
			select_sort( dev->sm_index, dev->sm_num );
			/*重新排序*/
		}
		
		printf("sm_num=%d, sm_index={ ", dev->sm_num);
		for( i = 0; i < SIZE_ARRAY( dev->sm_index ); i++ ) 
		{
			if( dev->sm_index[i] != -1 )
			{
				printf("%d, ", dev->sm_index[i]);
			}
		}
		printf("}\r\n");		
	}
}

/*******************************************
+MIPCALL:1,10.154.27.94
+MIPCALL: 1,10.155.155.75,210.21.4.130,221.5.88.88
********************************************/
static void on_longsung_request_ip_success_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	DevStatus *dev = ( DevStatus* ) priv;

	if( tok[1].end - tok[1].p < sizeof(dev->ip) )
	{
		memset(dev->ip, 0, sizeof(dev->ip));
		memcpy(dev->ip, tok[1].p, tok[1].end - tok[1].p);
		dev->ip[tok[1].end - tok[1].p] = '\0';
	}
	printf("%s: dev->ip(%s)\r\n", __func__, dev->ip);		
	dev->ppp_status = PPP_CONNECTED;
	
	if( CHECK_EXIST == dev->ops->check_at_command_exist( dev, A8_MIPCALL_SET ) )
	{
		dev->ops->atcmd_set_ack( dev, A8_MIPCALL_SET );
	}	
}

static void on_longsung_request_ip_fail_callback( void *priv, RemoteTokenizer *tzer)
{
	DevStatus *dev = ( DevStatus* ) priv;	

	printf("warnning->%s.\r\n", __func__);
	memset(dev->ip, 0, sizeof(dev->ip));
	memcpy(dev->ip, "null", strlen("null"));
	dev->ppp_status = PPP_DISCONNECT;
	/*获取IP失败，重新获取*/		
}

static void on_longsung_sm_read_err_callback( void *priv, RemoteTokenizer *tzer, Token* tok)
{
	/*******************************
		当发送了错误的短信index时发生
		AT+CMGR=1

		+CMS ERROR: 321
	********************************/
	DevStatus *dev = ( DevStatus* ) priv;	
	printf("SM read index error\r\n");
	dev->sm_read_flag = 1;	
}

static void on_lonsung_mqtt_lack_data_callback( void *mqtt_dev )
{
	//do nothing for lonsung module
}

/*
* author:	yangjianzhou
* function: 	module_reader_parse,  parse one line.
*/
static void longsung_reader_parse(  struct ComModule* instance, UartReader* r )
{
	int i;	
	Token tok[MAX_TOKENS];
	RemoteTokenizer tzer[1];
	DevStatus *dev = ( DevStatus * )( instance->p_dev );
	LongSungPriv *p = ( LongSungPriv *) instance->priv;
	
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}
	
	print_line( USART3, r->in, r->pos );
	module_tokenizer_init( tzer, ',', r->in, r->in + r->pos );
	
	if( tzer->count == 0 )
	{
		return;
	}
	if( tzer->count > SIZE_ARRAY( tok ) )
	{
		printf("%s: tzer->count(%d) error!\r\n", __func__, tzer->count);
		return;
	}
	
	for( i = 0; i < tzer->count; i++ ) 
	{
		tok[i] = module_tokenizer_get( tzer, i );
	}


	
	if( tzer->count == 1 && !memcmp( tok[0].p, "+MIPCALL: 1", strlen("+MIPCALL: 1") ) ) 
	{
		//instance->c_ops->request_ip_success_callback( dev, tzer, tok);
		dev->ops->make_at_command( dev, A8_MIPCALL_ASK, WAIT_NOT, 
			ONE_SECOND/10, ONE_SECOND * 2, 0 );	
		
		printf("%s: tcp connect.\r\n", __func__);
		if( dev->tcp_connect_status != 1 && CHECK_EXIST_NOT == 
			dev->ops->check_at_command_exist( dev, A8_MIPOPEN ) )
		{
			dev->ops->make_at_command( dev, A8_MIPOPEN, WAIT, 
				ONE_SECOND/10, ONE_SECOND * 15, 0 );						
		}
	} 
	//+MIPOPEN: 1,1
	//+MIPOPEN: 1,1
	//+MIPCLOSE: 1,0
	else if(!memcmp(tok[0].p, "+MIPOPEN: ", strlen("+MIPOPEN: "))&&(tzer->count>1)) 
	{
		if( str_2_int( tok[1].p, tok[1].end) == 1 ) 
		{
			instance->c_ops->connect_success_callback( dev, tzer, tok);
		}

	} 
	else if(!memcmp(tok[0].p, "+MIPCLOSE:", strlen("+MIPCLOSE:"))&&(tzer->count > 1)) 
	{//4
		instance->c_ops->disconnect_callback( dev, tzer, tok);
	} 	
	else if(!memcmp(tok[0].p, "+CSQ:", strlen("+CSQ:"))&&(tzer->count>1)) 
	{
		instance->c_ops->signal_strength_callback( dev, tzer, tok);
		
	} 
	else if( !memcmp( tok[0].p, "AT+", strlen("AT+") ) ) 
	{
		instance->c_ops->at_command_callback( dev, tzer, tok );
		
	} 
	else if(!memcmp(tok[0].p, "OK", strlen("OK"))) 
	{
		instance->c_ops->at_command_success_callback( dev, tzer);
		
	} 
	//+MIPCALL: 1,10.155.155.75,210.21.4.130,221.5.88.88
	else if(!memcmp(tok[0].p, "+MIPCALL: 1,", strlen("+MIPCALL: 1,"))&&(tzer->count>3)) 
	{
		instance->c_ops->request_ip_success_callback( dev, tzer, tok);
	}
	//+MIPDATA: 1,4,20020000
	else if(!memcmp(tok[0].p, "+MIPDATA:", strlen("+MIPDATA:"))&&(tzer->count>2)) 
	{
		instance->c_ops->tcp_data_callback( dev, tzer, tok );
		
	} 	



	

	return;
	
	if( p->sm_data_flag ) 
	{
		p->sm_data_flag = 0;
		instance->c_ops->sm_data_callback( dev, tzer, tok, dev->sm_index_read );
		return;
	}
	
	if( !memcmp( tok[0].p, "AT+", strlen("AT+") ) ) 
	{
		instance->c_ops->at_command_callback( dev, tzer, tok );
		
	} 
	else if(!memcmp(tok[0].p, "OK", strlen("OK"))) 
	{
		instance->c_ops->at_command_success_callback( dev, tzer);
		
	} 
	else if(!memcmp(tok[0].p, "ERROR", strlen("ERROR"))) 
	{
		instance->c_ops->at_command_fail_callback( dev, tzer);
		
	} 
	else if(!memcmp(tok[0].p, "+MIPRTCP=", strlen("+MIPRTCP="))&&(tzer->count>2)) 
	{
		instance->c_ops->tcp_data_callback( dev, tzer, tok );
		
	} 
	else if(!memcmp(tok[0].p, "+MIPCALL:0", strlen("+MIPCALL:0"))) 
	{
		instance->c_ops->request_ip_fail_callback( dev, tzer);
		
	} 
	else if(!memcmp(tok[0].p, "+MIPCALL:1", strlen("+MIPCALL:1"))&&(tzer->count>1)) 
	{
		instance->c_ops->request_ip_success_callback( dev, tzer, tok);
		
	} 
	else if(!memcmp(tok[0].p, "+CSQ:", strlen("+CSQ:"))&&(tzer->count>1)) 
	{
		instance->c_ops->signal_strength_callback( dev, tzer, tok);
		
	} 
	else if(!memcmp(tok[0].p, "+MIP:ERROR", strlen("+MIP:ERROR"))) 
	{
		instance->c_ops->connect_err_callback( dev, tzer);
		
	} 
	else if(!memcmp(tok[0].p, "+MIPOPEN=", strlen("+MIPOPEN="))&&(tzer->count>1)) 
	{
		if( str_2_int( tok[1].p, tok[1].end) == 1 ) 
		{
			instance->c_ops->connect_success_callback( dev, tzer, tok);
		}

	} 
	else if(!memcmp(tok[0].p, "+MIPCLOSE:", strlen("+MIPCLOSE:"))&&(tzer->count>=3)) 
	{//4
		instance->c_ops->disconnect_callback( dev, tzer, tok);

	} 
	else if(!memcmp(tok[0].p, "+SIMTEST:", strlen("+SIMTEST:"))) 
	{
		instance->c_ops->check_simcard_type_callback( dev, tzer, tok);

	} 
	else if (!memcmp(tok[0].p, "+CMGD: (", strlen("+CMGD: ("))) 
	{
		instance->c_ops->check_sm_callback( dev, tzer, tok);

	} 
	else if (!memcmp(tok[0].p, "+CMGR: \"REC READ\"", strlen("+CMGR: \"REC READ\"")) ||
		!memcmp(tok[0].p, "+CMGR: \"REC UNREAD\"", strlen("+CMGR: \"REC UNREAD\""))) 
	{
		instance->c_ops->read_sm_callback( dev, tzer, tok);

	} 
	else if (!memcmp(tok[0].p, "+CMTI: \"SM\",", strlen("+CMTI: \"SM\","))) 
	{
		instance->c_ops->sm_notify_callback( dev, tzer, tok);

	} 
	else if(!memcmp(tok[0].p, "+CMS ERROR: 321", strlen("+CMS ERROR: 321"))) 
	{
		instance->c_ops->sm_read_err_callback( dev, tzer, tok);
		
	}
}

void load_longsung_instance( TaskHandle_t task )
{
	static ComModule instance[1];

	static struct LongSungPriv longsung_priv;
	
	static struct device_operations ls_dops =
	{
		longsung_initialise_module,
		longsung_hardware_reset_callback,
		longsung_poll_signal,
		longsung_check_sm,
		longsung_check_ip,
		longsung_request_ip,
		longsung_close_socket,
		longsung_connect_server,
		longsung_push_socket_data,
		longsung_delete_sm,
		longsung_read_sm,
		longsung_do_send_command,
		longsung_at_get_name,
		longsung_make_tcp_packet,
		longsung_send_tcp_packet,
		longsung_send_push_directly,
		longsung_send_close_socket_directly,
		longsung_is_tcp_connect_cmd,	
	};
	
	static struct callback_operations ls_cops =
	{
		on_longsung_tcp_data_callback,
		on_longsung_connect_service_fail_callback,
		on_longsung_connect_service_success_callback,
		on_longsung_disconnect_service_callback,
		on_longsung_signal_strength_callback,
		on_longsung_request_ip_success_callback,
		on_longsung_request_ip_fail_callback,
		on_longsung_at_command_callback,
		on_longsung_at_cmd_success_callback,
		on_longsung_at_cmd_fail_callback,
		on_longsung_simcard_type_callback,
		on_longsung_sm_check_callback,
		on_longsung_sm_read_callback,
		on_longsung_sm_data_callback,
		on_longsung_sm_notify_callback,
		on_longsung_sm_read_err_callback,
		on_lonsung_mqtt_lack_data_callback,
	};

	memcpy( instance->name, "longsung", strlen("longsung") );
	instance->next = NULL;
	instance->p_dev = NULL;
	instance->d_ops = &ls_dops;
	instance->c_ops = &ls_cops;
	instance->priv = &longsung_priv;
	instance->is_initialise = 0;	
	instance->module_reader_parse = longsung_reader_parse;
	instance->module_power_reset = longsung_power_reset;

	if( -1 == register_communication_module( task, instance ) )
	{
		printf("%s: fail to register module\r\n", __func__);
	}
}






/***************************************core codes***************************************/

/*
* author:	yangjianzhou
* function: 	do_init_mqtt_buffer, do malloc MQTT total buffer.
*/
static MqttBuffer * do_init_mqtt_buffer( int num, int capacity )
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
		printf("%s: pvPortMalloc MqttBuffer fail\r\n", __func__);
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
			//printf("\r\n");
			mbuffer[i].capacity = capacity;
			mbuffer[i].num = num;
			mbuffer[i].be_used = 0;
		}
	}

	return mbuffer;
}

/*
* author:	yangjianzhou
* function: 	init_mqtt_buffer, malloc MQTT total buffer.
*/
static void init_mqtt_buffer( MqttBuffer **p_mqttbuff, int size )
{
	int i = 0;
	int num = 0;
	int capacity = 0;
	
	for( i = 0; i < size; i++ )
	{
		if( i == 0 )
		{
			num = 16;
			capacity = 55;
		} 
		else if( i == 1 )
		{
			num = 1;
			capacity = 100;
		} 
		else if( i == 2 )
		{
			num = 1;
			capacity = 200;
		}
		
		p_mqttbuff[i] = do_init_mqtt_buffer( num, capacity );
		
		if( !p_mqttbuff[i] )
		{
			printf("%s: pvPortMalloc p_mqttbuff[%d] fail\r\n", __func__, i);
			break;
		}
	}
}

/*
* author:	yangjianzhou
* function: 	alloc_mqtt_buffer, search and return MQTT buffer or NULL.
*/
static char* alloc_mqtt_buffer( int len )
{
	int i = 0;
	int j = 0;
	MqttBuffer* p;

	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return NULL;
	}

	for( i = 0; i < SIZE_ARRAY( dev->p_mqttbuff ); i++ )
	{	
		p = dev->p_mqttbuff[i];
		
		if( p )
		{
			if( len < p[0].capacity )
			{
				for( j = 0; j < p[0].num; j++ )
				{
					if( p[j].be_used == 0 ) 
					{
						memset( p[j].buff, 0, p[j].capacity );
						
						if( dev->debug )
						{
							printf("%s: alloc node ok! len(%d), p_mqttbuff[%d][%d].apacity(%d), buff(%p)\r\n", 
									__func__, len, i, j, p[0].capacity, p[j].buff);
						}
						p[j].be_used = 1;
						dev->malloc_count++;
						return p[j].buff;
					}
				}
			}
		}
	}
	
	printf("%s: alloc node fail! len(%d)\r\n", __func__, len );
	return NULL;
}

/*
* author:	yangjianzhou
* function: 	dealloc_mqtt_buffer, release MQTT buffer.
*/
static void dealloc_mqtt_buffer( char *buff )
{
	int i = 0;
	int j = 0;
	MqttBuffer* p;
	
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return;
	}

	for( i = 0; i < SIZE_ARRAY( dev->p_mqttbuff ); i++ )
	{	
		p = dev->p_mqttbuff[i];

		if( p )
		{
			for( j = 0; j < p[0].num; j++ )
			{
				if( p[j].be_used == 1 &&  p[j].buff == buff ) 
				{
					if( dev->debug )
					{
						printf("%s: dealloc node ok! p_mqttbuff[%d][%d], buff(%p)\r\n", 
								__func__, i, j, p[j].buff);
					}
					p[j].be_used = 0;
					memset( p[j].buff, 0, p[j].capacity );
					dev->free_count++;
					return;
				}
			}
		}
	}

	printf("%s: dealloc node fail!\r\n", __func__ );
}

/*
* author:	yangjianzhou
* function: 	init_command_buffer, malloc AtCommand total buffer.
*/
static void init_command_buffer( AtCommand **p_atcommand, unsigned char num )
{
	int i = 0;

	*p_atcommand = ( AtCommand * ) pvPortMalloc( sizeof( AtCommand ) * num );
	
	if( !( *p_atcommand ) )
	{
		printf("%s: pvPortMalloc AtCommand fail\r\n", __func__);
		return;
	}
	
	for( i = 0; i < num; i++ )
	{
		( *p_atcommand )[ i ].num = num;
		( *p_atcommand )[ i ].be_used = 0;
	}
}

/*
* author:	yangjianzhou
* function: 	alloc_command, search and return AtCommand buffer or NULL.
*/
static AtCommand* alloc_command()
{
	int i = 0;

	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return NULL;
	}

	if( !dev->p_atcommand )
	{
		return NULL;
	}

	for( i = 0; i < dev->p_atcommand[0].num; i++ )
	{
		if( dev->p_atcommand[i].be_used == 0 )
		{
			dev->p_atcommand[i].be_used = 1;
			
			if( dev->debug )
			{
				printf("%s: alloc atcommand[%d] addr(%p) ok.\r\n", __func__, 
						i, &( dev->p_atcommand[i] ));
			}
			dev->malloc_count++;
			return &( dev->p_atcommand[i] );
		}
	}

	printf("%s: alloc_command fail!\r\n", __func__);
	return NULL;
}

/*
* author:	yangjianzhou
* function: 	dealloc_command, release AtCommand buffer.
*/
static void dealloc_command( AtCommand* command )
{
	int i;

	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return;
	}
	if( command )
	{
		for( i = 0; i < command->num; i++ ) 
		{
		 	if( command == &( dev->p_atcommand[i] ) )
		 	{
		 		break;
		 	}
		}
		if( i >= command->num )
		{
			printf("%s: i(%d) error!!!!\r\n", __func__, i);
		}
	}
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
		//printf("%s: dealloc_command ok! (%p), i(%d)\r\n", __func__, command, i);
		dev->free_count++;
	}
	else
	{
		printf("%s: dealloc_command fail! (%p), i(%d), command->be_used(%d)\r\n", 
				__func__, command, i, command->be_used);
	}
}

/*
* author:	yangjianzhou
* function: 	mqtt_get_name.
*/
static char *mqtt_get_name( int type )
{
	switch( type )
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

		default: 
			return "MQTT UNKNOW";
	}
}

static void complete_pending( mqtt_dev_status *mqtt_dev, int event_type, uint16_t mesg_id)
{
	return;
}

/*
* author:	yangjianzhou
* function: 	process_test_mqtt_publish.
*/
static void process_test_mqtt_publish( mqtt_dev_status *mqtt_dev, int qos, char *topic, char *payload )
{
	//mqtt_state_t* state = mqtt_dev->mqtt_state;
	printf("%s: topic=%s, payload=%s, qos=%d\r\n", __func__, topic, payload, qos);
	
	if( qos > 2 || qos < 0 )
	{
		printf("%s: qos error!!!", __func__);
		return;
	}
	mqtt_dev->ops->mqtt_publish( mqtt_dev, topic, payload, qos, 0);		
}

/*
* author:	yangjianzhou
* function: 	get_at_command_count,  get the list member numbers.
*/
static int get_at_command_count( struct list_head *head )
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe( pos, n, head )
	{
		count++;	
	}
	
	return count;
}

/*
* author:	yangjianzhou
* function: 	deliver_publish,  deliver mqtt publish message receive from remote service.
*/
static int deliver_publish( void *argc, uint8_t* message, int length )
{
	char topic[128];
  	mqtt_event_data_t edata;
	mqtt_dev_status *mqtt = ( mqtt_dev_status* ) argc;
	
	memset(topic, '\0', sizeof(topic));
	edata.type = MQTT_EVENT_TYPE_PUBLISH;
	edata.topic_length = length;
	edata.topic = mqtt_get_publish_topic(message, &edata.topic_length);

	edata.data_length = length;
	edata.data = mqtt_get_publish_data(message, &edata.data_length);
	
	if( memcmp( mqtt->subscribe_topic, edata.topic, strlen( mqtt->subscribe_topic)) || 
			edata.topic_length > sizeof(topic) || edata.data_length <= 0 || 
				(edata.data_length + edata.topic_length) > (1500-6)/*fix head+topic.len+msgid*/)
	{	
		printf("%s: DATA ERROR! subscribe.topic=%s, edata.topic=%s, topic.length=%d, payload.length=%d\r\n",
							__func__, mqtt->subscribe_topic, edata.topic, edata.topic_length, edata.data_length);
		return -1;
	}

	memcpy(topic, edata.topic, edata.topic_length);
	topic[edata.topic_length] = '\0';

	{
		eLogLevel level = ucGetTaskLogLevel( pxModuleTask );

		if( level <= ucOsLogLevel )
		{
			printf("topic.length = %d, topic=%s\r\n", edata.topic_length, topic);
			printf("payload.length = %d, payload=%s\r\n", edata.data_length, edata.data);			
		}
		else
		{
			vSetTaskLogLevel( NULL, ucOsLogLevel );
			printf("%s\r\n", edata.data);
			vSetTaskLogLevel( NULL, level );
		}
	}
	
	if( !memcmp(edata.data, "report", strlen("report")))
	{
		char json_buff[450];//big if find
		DevStatus *dev = ( DevStatus *)( mqtt->p_dev );
		
		if( !dev ) 
		{
			return 0;
		}
		sprintf(json_buff, "\r\n	time:[%u(s)]. mqtt_bytes(%u),lostbytes(%d),MQTT:reset_count(%d),\r\n"
				"	in_publish(%d), mq_head(%d),fixhead(%d),in_waitting(%d),in_pos(%d)\r\n"
				"	4G: malloc(%d),free(%d),simcard(%d),reset_times(%d),tcp_connect_times(%d),\r\n"
				"	ppp_status(%d),socket_num(%d),sm_num(%d),scsq(%d),rcsq(%d),at_count(%d),\r\n"
				"	at_head(%d), at_wait_head(%d),mqtt_head(%d),tcp_connect_status(%d)\r\n\r\n", 
				(portTICK_PERIOD_MS * NOW_TICK) / 1000, mqtt->recv_bytes, dev->uart_fifo->lostBytes,
				mqtt->reset_count, mqtt->pub_in_num, get_at_command_count(&dev->mqtt_head), mqtt->fixhead, 
				mqtt->in_waitting, mqtt->in_pos, dev->malloc_count, dev->free_count, dev->simcard_type, 
				dev->reset_times, dev->tcp_connect_times, dev->ppp_status, dev->socket_num, dev->sm_num, 
				dev->scsq, dev->rcsq, dev->at_count, get_at_command_count(&dev->at_head),  
				get_at_command_count(&dev->at_wait_head), get_at_command_count(&dev->mqtt_head), 
				dev->tcp_connect_status);

		process_test_mqtt_publish( mqtt, 1, "/xp/publish", json_buff );
	}
	else if( edata.data_length <= 468)
	{
		process_test_mqtt_publish( mqtt, 0, "/xp/publish", edata.data );
	}
	//APP_4G_OnControlMsg(payload);
	return 1;
}

/* Publish the specified message */
static int mqtt_publish_with_length( mqtt_dev_status *mqtt_dev, const char* topic, const char* data, int data_length, int qos, int retain )
{
	DevStatus *dev = ( DevStatus *)(mqtt_dev->p_dev);
	mqtt_state_t *state = mqtt_dev->mqtt_state;

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return -1;
	}

	state->outbound_message = mqtt_msg_publish( &state->mqtt_connection, 
	                                             topic, data, data_length, 
	                                             qos, retain,
	                                             &state->pending_msg_id );
	state->mqtt_flags &= ~MQTT_FLAG_READY;
	state->pending_msg_type = MQTT_MSG_TYPE_PUBLISH;

	dev->ops->make_mqtt_command( dev, ATMQTT, ( data_length / 300 + 1 ) * ONE_SECOND / 25, MQTT_MSG_TYPE_PUBLISH );
	/*when we need to publish message, need to wak up task.*/
	dev->ops->wake_up_system( dev );
	return 0;
}

/*retain 标志只用于publish 消息，当为1时, 注意data的长度，不超过700*/
static int mqtt_publish( void *argc, const char* topic, char* data, int qos, int retain)
{
	int len = strlen( data );
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;

	return mqtt_publish_with_length( mqtt_dev, topic, data, data != NULL ? len: 0, qos, retain );
}

/*
* author:	yangjianzhou
* function: 	mqtt_subscribe, subscribe a topic for mqtt.
*/
static void mqtt_subscribe( void *argc )
{
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;
	mqtt_state_t *state = mqtt_dev->mqtt_state;
	DevStatus *dev = ( DevStatus *)(mqtt_dev->p_dev);

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}
	state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
	dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/5, MQTT_MSG_TYPE_SUBSCRIBE );
	dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/40 );	
}

/*
* author:	yangjianzhou
* function: 	mqtt_reset_status,  reset mqtt_dev_status if parse error happend.
*/
static void mqtt_reset_status( void *argc )
{	
	mqtt_dev_status *mqtt = ( mqtt_dev_status* ) argc;
	DevStatus *dev = ( DevStatus *)(mqtt->p_dev);

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}
	printf("%s: uart2fifo->lostBytes=%d\r\n", __func__, dev->uart_fifo->lostBytes);

	mqtt->parse_packet_flag = 0;
	/* abandon the recevied buffer */
	
	mqtt->reset_count++;
	mqtt->in_pos = 0;
	mqtt->in_waitting = 0;	
	mqtt->fixhead = 1;		
	
	dev->close_tcp_interval = ONE_SECOND / 2;
	dev->tick_sum = 0;
	dev->tick_tag = NOW_TICK;
	/* close tcp 0.5 second later */
}

/*
* author:	yangjianzhou
* function: 	mqtt_check_packet,  check if it legal packet.
*/
static int mqtt_check_packet( void *argc, int protocol_len )
{
	int ret;
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;	
	mqtt_state_t *state = mqtt_dev->mqtt_state;

	if( state->message_length != protocol_len )
	{
		mqtt_reset_status( mqtt_dev );
		printf("%s: illegal mqtt len. mqtt reset!\r\n", __func__);
		ret = 1;
	}
	else
	{
		ret = 0;
		//printf("%s: ok!\r\n", __func__);
	}

	return ret;
}

/*
* author:	yangjianzhou
* function: 	mqtt_parse_packet,  deal with mqtt message packet.
*/
static void mqtt_parse_packet( void *argc, int nbytes )
{
	unsigned char msg_type;
	unsigned char msg_qos;
	unsigned short msg_id;
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;	
	mqtt_state_t *state = mqtt_dev->mqtt_state;
	DevStatus *dev = ( DevStatus *)( mqtt_dev->p_dev );

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}

	state->in_buffer_length = nbytes;
	state->message_length_read = nbytes;
	state->message_length = mqtt_get_total_length( state->in_buffer, state->message_length_read );

	msg_type = mqtt_get_type( state->in_buffer );
	msg_qos  = mqtt_get_qos( state->in_buffer );
	msg_id	 = mqtt_get_id( state->in_buffer, state->in_buffer_length );
	
	printf("received (%s)! msg_type(%d), QOS=%d, msg_id=0X%02X, mesg_len=%d, nbytes=%d\r\n", 
			mqtt_get_name( ( int )msg_type ), msg_type, msg_qos, msg_id, state->message_length, nbytes );

	switch( msg_type )
	{
		case MQTT_MSG_TYPE_CONNACK:
			/* 4 bytes */		
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
			
			if( state->in_buffer[3] == 0x00 ) 
			{
				mqtt_dev->connect_status = MQTT_DEV_STATUS_CONNECT;
				dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_CONNECT, 0 );
				mqtt_subscribe( mqtt_dev );
			} 
			else 
			{
				mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
			}
			printf("MQTT_MSG_TYPE_CONNACK, ack=%02x, status=%d\r\n", state->in_buffer[ nbytes-1 ],
				 	mqtt_dev->connect_status);
			
			break;
			
		 /*对于SUBACK的msg_id必须比较是否跟发出去的MQTT_MSG_TYPE_SUBSCRIBE msg_id一致！*/
		case MQTT_MSG_TYPE_SUBACK:
			/* 5 bytes */
		  	if( mqtt_check_packet( mqtt_dev, 5 ) ) return;
			
			if( state->pending_msg_type == MQTT_MSG_TYPE_SUBSCRIBE && state->pending_msg_id == msg_id )
				complete_pending( mqtt_dev, MQTT_EVENT_TYPE_SUBSCRIBED, msg_id );
			
			if( state->in_buffer[4] == 0x00 || state->in_buffer[4] == 0x01 ||
					state->in_buffer[4] == 0x02 )
			{
				printf("%s: subscribe success.\r\n", __func__);
				dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_SUBSCRIBE, msg_id );

				//delete when release codes, mqtt_publish for test.
				mqtt_publish( mqtt_dev, "/xp/publish", "MCU be ready to recv command.", 1, 0);
			}

			if( state->in_buffer[4] == 0x80 )
			{
				printf("%s: subscribe fail! state->in_buffer[4]=0x%02x\r\n", __func__, state->in_buffer[4] );
			}
			
			break;
			
		case MQTT_MSG_TYPE_UNSUBACK:
			/* 4 bytes */
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
			
			if( state->pending_msg_type == MQTT_MSG_TYPE_UNSUBSCRIBE && state->pending_msg_id == msg_id )
				complete_pending( mqtt_dev, MQTT_EVENT_TYPE_UNSUBSCRIBED, msg_id );
			
			break;
		
		/*对于收到的PUBLISH消息，不用理会msg_id的值，只需回传给服务器就可以*/
		case MQTT_MSG_TYPE_PUBLISH:
			//msg_type=3, msg_qos=1, msg_id=0x19   订阅消息时如果没有指定qos=0跟qos=1的区别。
			//msg_type=3, msg_qos=0, msg_id=0x00    两个消息在public 都指定了qos=1
			mqtt_dev->pub_in_num++ ;
		
			if( msg_qos == 1 ) 
			{
				state->outbound_message = mqtt_msg_puback( &state->mqtt_connection, msg_id );
				dev->ops->make_mqtt_command(dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBACK );		
			} 
			else if( msg_qos == 2 ) 
			{
				state->outbound_message = mqtt_msg_pubrec( &state->mqtt_connection, msg_id );
				/* recv process: */
				dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREC );
			}
			
			if( -1 == deliver_publish( mqtt_dev, state->in_buffer, state->message_length_read ) )
			{
				printf("%s: deliver_publish error! mqtt reset!\r\n", __func__);
				mqtt_reset_status( mqtt_dev );
				return;
			}
				
			break;
			
		/*当收到MQTT_MSG_TYPE_PUBACK时，msg_id必须与send MQTT_MSG_TYPE_PUBLISH消息
			时的msg_id进行比较！*/	
		case MQTT_MSG_TYPE_PUBACK:
			/* 4 bytes */
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
			
			if( state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id )
				complete_pending( mqtt_dev, MQTT_EVENT_TYPE_PUBLISHED, msg_id );
			
			dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_PUBLISH, msg_id );
			
			break;
			
		case MQTT_MSG_TYPE_PUBREC:
			/*send process: publish received = 4 bytes*/
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
		
			state->outbound_message = mqtt_msg_pubrel( &state->mqtt_connection, msg_id );
		  	dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_PUBLISH, msg_id );
			dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBREL );

			break;
		
		case MQTT_MSG_TYPE_PUBREL:
			/*recv process: publish release = 4 bytes*/
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
		
			state->outbound_message = mqtt_msg_pubcomp( &state->mqtt_connection, msg_id );
		  	dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_PUBREC, msg_id );
			dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PUBCOMP );

			break;
		
		case MQTT_MSG_TYPE_PUBCOMP:
			/*send process: publish completed = 4 bytes*/
		  	if( mqtt_check_packet( mqtt_dev, 4 ) ) return;
		
			if( state->pending_msg_type == MQTT_MSG_TYPE_PUBLISH && state->pending_msg_id == msg_id )
				complete_pending( mqtt_dev, MQTT_EVENT_TYPE_PUBLISHED, msg_id );
			dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_PUBREL, msg_id );
			break;
			
		case MQTT_MSG_TYPE_PINGREQ:
			/*2 bytes*/
		  	if( mqtt_check_packet( mqtt_dev, 2 ) ) return;
		
			dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/45, MQTT_MSG_TYPE_PINGRESP );
			dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/50 );
		
			break;
		
		case MQTT_MSG_TYPE_PINGRESP:
			/* 2 bytes */
		  	if( mqtt_check_packet( mqtt_dev, 2 ) ) return;
		
			dev->ops->mqtt_set_mesg_ack( dev, MQTT_MSG_TYPE_PINGREQ, 0 );
			
			break;
		
		case MQTT_DEV_STATUS_DISCONNECT:
			/* 2 bytes */
		  	if( mqtt_check_packet( mqtt_dev, 2 ) ) return;
		
			mqtt_dev->connect_status = MQTT_DEV_STATUS_DISCONNECT;
		
			break;
			
		default: 
			printf("%s: packet error! mqtt reset!\r\n", __func__);
			mqtt_reset_status( mqtt_dev );
			break;
	}
		
}

/*
* author:	yangjianzhou
* function: 	check_packet_from_fixhead,  deal with mqtt message head.
*/
static void check_packet_from_fixhead( void *argc, unsigned char * read, int nbytes )
{
	int msg_len;
	mqtt_dev_status *mqtt = ( mqtt_dev_status* ) argc;
	DevStatus *dev = ( DevStatus * ) mqtt->p_dev;
	
	msg_len = mqtt_get_total_length( read, nbytes );

	if( !mqtt->parse_packet_flag )
	{
		printf("%s: waitting mqtt tcp connect server!\r\n", __func__);
		return;
	}
	
  	if( msg_len == nbytes )//most 750 = 750 
	{
		printf("%s: msg_len = nbytes = %d\r\n", __func__, nbytes);
		mqtt->in_pos = 0;
		mqtt->in_waitting = 0;		
		memset( mqtt->in_buffer, '\0', sizeof( mqtt->in_buffer ) );
		memcpy( mqtt->in_buffer, read, msg_len );
		mqtt_parse_packet( mqtt, nbytes );
	}
	else if( msg_len < nbytes )//150 < 200
	{
		printf("%s: [%d] msg_len < nbytes [%d]\r\n", __func__, msg_len, nbytes);		
		mqtt->in_waitting = 0;	
		mqtt->in_pos = 0;		
		memset( mqtt->in_buffer, '\0', sizeof( mqtt->in_buffer ) );
		memcpy( mqtt->in_buffer, read, msg_len );
		mqtt_parse_packet( mqtt, msg_len );
		memmove( read, read + msg_len, nbytes - msg_len );
		mqtt->fixhead = 1;		
		check_packet_from_fixhead( mqtt, read, nbytes - msg_len );
	}
	else if( msg_len > nbytes && msg_len <= sizeof( mqtt->in_buffer ) )//msg_len > nbytes  1500 > 1300 > 750
	{
		printf("%s: nbytes(%d) < msg_len(%d) <= sizeof(mqtt_dev->in_buffer)(%d)\r\n", 
				__func__, nbytes, msg_len, sizeof( mqtt->in_buffer ) );			
		mqtt->fixhead = 0;
		memset( mqtt->in_buffer, '\0', sizeof( mqtt->in_buffer ) );
		memcpy( mqtt->in_buffer, read, nbytes );
		mqtt->in_pos = nbytes;
		mqtt->in_waitting = msg_len-nbytes;// 0<in_waitting<=750
		dev->module->c_ops->mqtt_lack_data_callback( mqtt );
	}
	else if(  msg_len > nbytes && msg_len > sizeof( mqtt->in_buffer ) )//1502
	{
		printf("%s: [%d] msg_len > sizeof(dev->mqtt_dev->in_buffer)[%d]\r\n", 
				__func__, msg_len, sizeof( mqtt->in_buffer ));		
		mqtt->fixhead = 0;
		mqtt->in_pos = 0;
		mqtt->in_waitting = -( msg_len - nbytes );//-752 0 > in_waitting
		dev->module->c_ops->mqtt_lack_data_callback( mqtt );		
	}
}

/*
* author:	yangjianzhou
* function: 	mqtt_protocol_parse, parse mqtt packet receive from tcp.
*/
static void mqtt_protocol_parse( void *argc, unsigned char * tcp_read, int nbytes )
{
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;	
	
	if( mqtt_dev->fixhead )
	{
		check_packet_from_fixhead( mqtt_dev, tcp_read, nbytes );
	}
	else 
	{
		if( mqtt_dev->in_waitting > 0 && mqtt_dev->in_waitting <= 750 )
		{
			if( mqtt_dev->in_waitting == nbytes )
			{
				printf("%s: in_waitting == nbytes = %d\r\n", __func__, nbytes);
				memcpy( mqtt_dev->in_buffer + mqtt_dev->in_pos, tcp_read, nbytes );
				mqtt_parse_packet( mqtt_dev, mqtt_dev->in_pos + mqtt_dev->in_waitting );
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;
			}
			else if( mqtt_dev->in_waitting < nbytes )
			{
				printf("%s: [%d] in_waitting < nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes );
				memcpy( mqtt_dev->in_buffer + mqtt_dev->in_pos, tcp_read, mqtt_dev->in_waitting );
				mqtt_parse_packet( mqtt_dev, mqtt_dev->in_pos + mqtt_dev->in_waitting );
				memmove( tcp_read, tcp_read + mqtt_dev->in_waitting, nbytes-mqtt_dev->in_waitting );
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;				
				check_packet_from_fixhead( mqtt_dev, tcp_read, nbytes-mqtt_dev->in_waitting );
			}
			else if( mqtt_dev->in_waitting > nbytes )
			{
				printf("%s: [%d] in_waitting > nbytes [%d]\r\n", __func__, mqtt_dev->in_waitting, nbytes );
				memcpy( mqtt_dev->in_buffer + mqtt_dev->in_pos, tcp_read, nbytes );
				mqtt_dev->in_pos += nbytes;
				mqtt_dev->in_waitting -= nbytes;
			}				
		}
		else if( mqtt_dev->in_waitting < 0 )//-752
		{
			mqtt_dev->in_waitting += nbytes;
			
			if( mqtt_dev->in_waitting < 0 ) 
			{
				printf("%s: Drop packet! in_waitting = %d\r\n", __func__, mqtt_dev->in_waitting);
			}
			else if( mqtt_dev->in_waitting == 0 )
			{
				mqtt_dev->fixhead = 1;
				mqtt_dev->in_pos = 0;
				printf("%s: Drop packet! in_waitting = %d\r\n", __func__, mqtt_dev->in_waitting);
			}
			else if( mqtt_dev->in_waitting > 0 )//in_waitting=-4 + nbytes=20 = in_waitting=16
			{
				int len;
				
				len = mqtt_dev->in_waitting;
				mqtt_dev->in_waitting = 0;
				mqtt_dev->in_pos = 0;
				mqtt_dev->fixhead = 1;
				memmove( tcp_read, tcp_read + ( nbytes - len ), len );
				printf("%s: Drop some char in packet! len = %d, nbytes=%d\r\n", __func__, len, nbytes);
				check_packet_from_fixhead( mqtt_dev, tcp_read, mqtt_dev->in_waitting );
			}
		}
		else if( mqtt_dev->in_waitting > 750 )
		{
			printf("%s: mqtt_dev->in_waitting=%d ERROR!\r\n", __func__, mqtt_dev->in_waitting);
			mqtt_reset_status( mqtt_dev );
		}
	}

}

/*
* author:	yangjianzhou
* function: 	module_reader_addc,  deal with an charactor, then parse  by module instance in line.
*/
static void module_reader_addc(  DevStatus* dev, UartReader* r, int c )
{	
	if ( r->overflow ) 
	{
		r->overflow = ( c != '\n' ? 1 : 0 );
		
		if( !( r->overflow ) )
		{
			memset( r->in, 0, sizeof( r->in ) );
		}
		
		return;
	}

	if ( r->pos >= sizeof( r->in ) ) 
	{
		r->overflow = 1;
		r->pos = 0;
		printf("%s: OVER FLOW!!!!!!!\r\n", __func__);
		return;
	}	

	/*we just need charactor for fix some bug.*/
	if( c < 0x20 &&  c != 0x0d && c != 0x0a  )
	{	
		//pre = c;
		r->overflow = 1;
		r->pos = 0;		
		return;
	}

	/*r->in[] size must more than 1550*/
	r->in[ r->pos ] = ( char ) c;
	r->pos += 1;

	if( c == '\n' ) 
	{
		dev->module->module_reader_parse( dev->module, r );
		r->pos = 0;
		
		memset( r->in, '\0', sizeof( r->in ) );
	}
}

/*
* author:	yangjianzhou
* function: 	handle_module_uart_msg,  handle module message from uart interrupt.
*/
static void handle_module_uart_msg(  DevStatus* dev )
{
	unsigned char ch;
	
	while( rfifo_len( dev->uart_fifo ) > 0 ) 
	{
		rfifo_get( dev->uart_fifo, &ch, 1 );
		module_reader_addc( dev, dev->reader, ch );
	}	
}

/*
* author:	yangjianzhou
* function: 	print_module_struct_info, print the data struct in the system.
*/
void print_module_struct_info( void )
{
	mqtt_dev_status *mqtt;
	ComModule *module;
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return;
	}
	mqtt = dev->mqtt_dev;
	module = dev->module;
	vTaskDelay( 10 );

	
	printf("\r\nmodule:   		name(%s)\r\n", module->name);

	dev->ops->system_lock( dev, portMAX_DELAY );

	printf("dev:   			simcard_type(%d), boot_status(%d), free_count(%d), malloc_count(%d)\r\n"
			"   			ip(%s), ppp_status(%d), scsq(%d), rcsq(%d), at_wait_head(%d)\r\n"
			"   			at_head(%d), mqtt_head(%d), tcp_connect_times(%d), reset_timers(%d)\r\n"
			"   			socket_num(%d), tcp_connect_status(%d)\r\n", 
			dev->simcard_type, dev->boot_status, 
			dev->free_count, dev->malloc_count, dev->ip, dev->ppp_status,
			dev->scsq, dev->rcsq, get_at_command_count(&dev->at_wait_head),
			get_at_command_count(&dev->at_head), get_at_command_count(&dev->mqtt_head),
			dev->tcp_connect_times, dev->reset_times, dev->socket_num,
			dev->tcp_connect_status);

	dev->ops->system_unlock( dev );
	
	printf("mqtt:   		connect_status(%d), pub_in_num(%u), pub_out_num(%u)\r\n"
			"   			fixhead(%d), in_waitting(%d), recv_bytes(%u), reset_count(%u)\r\n",
			mqtt->connect_status, mqtt->pub_in_num, mqtt->pub_out_num, 
			mqtt->fixhead, mqtt->in_waitting, mqtt->recv_bytes, mqtt->reset_count);
	
}

/*
* author:	yangjianzhou
* function: 	test_mqtt_publish,  used by other task to publish mqtt message.
*/
void test_mqtt_publish( char *data )
{	
	char json_buff[38];
	static int qos = 0;
	char *str = ( char* ) data;
	mqtt_dev_status *mqtt;	
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return;
	}

	if( strlen( data ) > sizeof( json_buff ))
	{
		printf("%s: data len(%d) error!\r\n", __func__, strlen( data ));
		return;
	}
	mqtt = dev->mqtt_dev;

	if( strlen( str ) > 1 && mqtt->connect_status == MQTT_DEV_STATUS_CONNECT )
	{
		while( *str == ' ' ) str++;
		
		if( strlen( str ) > 0 )
		{
			memset(json_buff, '\0', sizeof(json_buff));
			memcpy( json_buff, str , strlen( str ) );
			/*start to protect all thing*/
			dev->ops->system_lock( dev, portMAX_DELAY );
			process_test_mqtt_publish( mqtt, qos, "/xp/publish", json_buff );
			dev->ops->system_unlock( dev );
		}
	}
	else
	{
		printf("%s: mqtt is not working!\r\n", __func__);
	}
}

/*
* author:	yangjianzhou
* function: 	mqtt_prepare_packet,  make mqtt buffer and send it.
*/
static void mqtt_prepare_packet( void *argc, void* command )
{
	char buff[ 512 ];
	AtCommand* cmd = command;
	mqtt_dev_status *mqtt_dev = ( mqtt_dev_status* ) argc;
	mqtt_state_t *state = mqtt_dev->mqtt_state;	
	DevStatus *dev = ( DevStatus *)(mqtt_dev->p_dev);
	ComModule *module = dev->module;
	
	memset( buff, '\0', sizeof( buff ) );
	
	switch( cmd->para ) 
	{
		case MQTT_MSG_TYPE_PINGRESP:
			state->outbound_message = mqtt_msg_pingresp( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_PINGRESP;
			if( module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
			}		
		  break;
		
		case MQTT_MSG_TYPE_CONNECT:
			state->outbound_message =  mqtt_msg_connect( &state->mqtt_connection, state->connect_info );
			cmd->mqtype = MQTT_MSG_TYPE_CONNECT;
			cmd->msgid = 0;
			if( module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
			}
			break;
		
		case MQTT_MSG_TYPE_DISCONNECT:
			state->outbound_message =  mqtt_msg_disconnect( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_DISCONNECT;
			if( module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
			}		
			break;
			
		case MQTT_MSG_TYPE_PINGREQ: 
			state->outbound_message = mqtt_msg_pingreq( &state->mqtt_connection );
			cmd->mqtype = MQTT_MSG_TYPE_PINGREQ;
			cmd->msgid = 0;
			if( module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
			}		
			break;
		
		case MQTT_MSG_TYPE_SUBSCRIBE:
		  state->pending_msg_type = MQTT_MSG_TYPE_SUBSCRIBE;
			state->outbound_message = mqtt_msg_subscribe( &state->mqtt_connection, 
                                                   dev->mqtt_dev->subscribe_topic, 2, //此处的QOS关系收到PUBLISH消息的qos
                                                   &state->pending_msg_id );
			cmd->msgid = state->pending_msg_id;		
			cmd->mqtype = MQTT_MSG_TYPE_SUBSCRIBE;

			if( module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
			{
				module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
			}		
			break;
	
		/*需要记录msg_id的消息*/
		case MQTT_MSG_TYPE_PUBLISH:
		case MQTT_MSG_TYPE_PUBCOMP:
		case MQTT_MSG_TYPE_PUBREL:
		case MQTT_MSG_TYPE_PUBREC:
		case MQTT_MSG_TYPE_PUBACK://40head 02len 0017msgid
		
			if( cmd->mqttdata ) 
			{
				memset( dev->mqtt_dev->out_buffer, '\0', sizeof( dev->mqtt_dev->out_buffer ) );
				if( module->d_ops->make_tcp_packet( (char *) dev->mqtt_dev->out_buffer, cmd->mqttdata, cmd->mqttdata_len ) )
				{
					module->d_ops->send_tcp_packet( ( char * )dev->mqtt_dev->out_buffer, cmd->mqttdata, cmd->mqttdata_len );
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

/*
* author:	yangjianzhou
* function: 	reset_module_reader, init UartReader data struct.
*/
static void reset_module_reader( DevStatus *dev, UartReader *reader ) 
{
	reader->inited = 1;
	reader->pos = 0;
	reader->overflow = 0;
	reader->p_dev = dev;
}

/*
* author:	yangjianzhou
* function: 	reset_mqtt_dev, init the mqtt_dev_status data struct .
*/
static void reset_mqtt_dev( DevStatus *dev, mqtt_dev_status* mqtt )
{
	static struct mqtt_dev_operations mqtt_ops =
	{
		mqtt_prepare_packet,
		mqtt_protocol_parse,
		mqtt_subscribe,
		mqtt_publish,
		mqtt_reset_status,
	};

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}
	printf("%s\r\n", __func__);
	memset( mqtt, '\0', sizeof( mqtt_dev_status ) );
	
	mqtt->connect_info.client_id = "yjianzhou";
	mqtt->connect_info.username = NULL;
	mqtt->connect_info.password = NULL;
	mqtt->connect_info.will_topic = "mcu";
	mqtt->connect_info.will_message = "death";
	mqtt->connect_info.keepalive = 460;
	mqtt->connect_info.will_qos = 1;
	mqtt->connect_info.will_retain = 0;
	mqtt->connect_info.clean_session = 1;

	/* Initialise the MQTT client*/
	mqtt_init(mqtt->mqtt_state, mqtt->in_buffer, sizeof(mqtt->in_buffer),
						mqtt->out_buffer, sizeof(mqtt->out_buffer));	
	
	mqtt->mqtt_state->connect_info = &(mqtt->connect_info);
	/* Initialise and send CONNECT message */
	mqtt_msg_init(&(mqtt->mqtt_state->mqtt_connection), mqtt->mqtt_state->out_buffer,
			mqtt->mqtt_state->out_buffer_length);
	
	mqtt->connect_status = MQTT_DEV_STATUS_NULL;
	
	memset(mqtt->subscribe_topic, '\0', sizeof(mqtt->subscribe_topic));
	memcpy(mqtt->subscribe_topic, "/system/sensor", strlen("/system/sensor"));
	mqtt->pub_in_num = 0;
	mqtt->pub_out_num = 0;
	mqtt->in_pos = 0;	
	mqtt->in_waitting = 0;	
	mqtt->fixhead = 1;
	mqtt->parse_packet_flag = 1;
 	mqtt->iot = 0;
	mqtt->recv_bytes = 0;
	mqtt->p_dev = dev;
	mqtt->ops = &mqtt_ops;
}

/*
* author:	yangjianzhou
* function: 	reset_module_status.
*/
static void reset_module_status( DevStatus* dev, char flag )
{
  	int i;

	dev->boot_status = 0;
	dev->ppp_status = PPP_DISCONNECT;
	dev->scsq = 0;
	dev->rcsq = 0;
	dev->simcard_type = -1;

	dev->socket_close_flag = 0;
	dev->socket_num = 0;

	dev->mqtt_heartbeat = 0;
	dev->period_flag = 0;	
	
	dev->sm_num = 0;
	dev->sm_index_delete = -1;
	
	dev->sm_read_flag = 1;
	dev->sm_delete_flag = 1;
	dev->atcmd = NULL;
	dev->close_tcp_interval = 0;
	dev->tcp_connect_status = 0;
	memset(dev->ip, 0, sizeof(dev->ip));	
	memcpy( dev->ip, "null", strlen("null") );
	dev->debug = 0;
	if( flag ) 
	{
		dev->reset_request = 1;
		dev->reset_times = 0;
		dev->tcp_connect_times = 0;
	} 
	else 
	{
		dev->reset_request = 0;
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

/*
* author:	yangjianzhou
* function: 	add_cmd_to_list, add command struct to list head.
*/
static void add_cmd_to_list( AtCommand *cmd, struct list_head *head )
{
	if( cmd )
	{
		list_add_tail(&cmd->list, head);
	}
}

/*
* author:	yangjianzhou
* function: 	del_cmd_from_list, delete command struct from list head.
*/
static void del_cmd_from_list( AtCommand *cmd, struct list_head *head )
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe( pos, n, head ) 
	{ 
		command = list_entry( pos, AtCommand, list ); 
		if( command == cmd && cmd != NULL ) 
		{ 
			list_del(pos);
		} 
	} 
}

/*
* author:	yangjianzhou
* function: 	get_cmd_from_list, get a command struct from list head.
*/
static AtCommand* get_cmd_from_list( struct list_head *head )
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe( pos, n, head )
	{
		command = (AtCommand *)list_entry( pos, AtCommand, list );
		break;
	}	

	return command;
}

/*
* author:	yangjianzhou
* function: 	check_at_command_exist,  checkt if command in the list.
*/
static CheckResult check_at_command_exist( void *argc, int index )
{
	AtCommand *command = NULL;
	struct list_head *pos=NULL, *n=NULL;
	DevStatus *dev = ( DevStatus * ) argc;

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return CHECK_ERROR;
	}
	
	list_for_each_safe( pos, n, &dev->at_head )
	{
		command = ( AtCommand * ) list_entry( pos, AtCommand, list );
		if( index == command->index ) 
		{
			return CHECK_EXIST;
		}
	}
	pos = NULL; n = NULL;
	list_for_each_safe( pos, n, &dev->at_wait_head )
	{
		command = ( AtCommand * ) list_entry( pos, AtCommand, list );
		if( index == command->index ) 
		{
			return CHECK_EXIST;
		}
	}

	return CHECK_EXIST_NOT;	
}

/*
* author:	yangjianzhou
* function: 	make_at_command_to_list,  make a AtCommand and add to at_head list.
*/
static void make_at_command_to_list( void *argc, char index, char wait_falg, 
			unsigned int interval_a, unsigned int interval_b, int para )
{
	AtCommand *cmd;	
	DevStatus *dev = ( DevStatus * ) argc;
	
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}	
	cmd = alloc_command();
	
	if( cmd != NULL ) 
	{
		cmd->mqttdata = NULL;
		cmd->mqttdata_len = 0;
		cmd->mqttack = 0;		
		cmd->index = index;
		cmd->interval = interval_a;
		cmd->interval_a = interval_a;
		cmd->interval_b = interval_b;
		cmd->wait_flag = wait_falg;
		cmd->tick_sum = 0;
		cmd->tick_tag = NOW_TICK;
		cmd->para = para;
		cmd->mqtt_try = 0;
		cmd->mqtt_clean = 0;
		cmd->atack = 0;
		cmd->mqtype = MQTT_MSG_TYPE_NULL;		
		add_cmd_to_list( cmd, &dev->at_head );	
	}
	else
	{
		printf("%s: make node fail!\r\n", __func__);
	}
}

/*
* author:	yangjianzhou
* function: 	fill_cmd_mqtt_part,  fill AtCommand in mqtt part.
*/
static void fill_cmd_mqtt_part( DevStatus *dev, AtCommand *cmd )
{
	cmd->mqttack = 0;
	cmd->mqttdata = NULL;

	switch( cmd->para ) 
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
			cmd->mqtype = ( mqtt_message_type ) cmd->para; 
			break;
			
		default: 
			printf("%s: mqtt para error!\r\n", __func__); 
			break;
	}
	
	/*message that need to store msg_id*/
	if( cmd->para == MQTT_MSG_TYPE_PUBLISH || cmd->para == MQTT_MSG_TYPE_PUBCOMP || 
		cmd->para == MQTT_MSG_TYPE_PUBREL || cmd->para == MQTT_MSG_TYPE_PUBREC || 
		cmd->para == MQTT_MSG_TYPE_PUBACK ) 
	{	
		cmd->mqttdata_len = dev->mqtt_dev->mqtt_state->outbound_message->length;
		cmd->mqttdata = ( unsigned char * ) alloc_mqtt_buffer( cmd->mqttdata_len );
		
		if( !( cmd->mqttdata ) ) 
		{
			printf("ERROR: malloc for MQTT [%s] fail!\r\n", mqtt_get_name(cmd->mqtype));
			cmd->mqttdata = NULL;
			dealloc_command( cmd );
			return;
		} 
		else 
		{
			if( dev->debug )
			{
				printf("%s: alloc mqtt buffer ok, mqttdata_len(%d)\r\n", __func__, cmd->mqttdata_len);
			}
			/*store the outbound_message->data, make cmd->mqttdata point to it. */
			memcpy( cmd->mqttdata, dev->mqtt_dev->mqtt_state->outbound_message->data, cmd->mqttdata_len );
			cmd->msgid = mqtt_get_id( dev->mqtt_dev->mqtt_state->outbound_message->data, cmd->mqttdata_len );
			add_cmd_to_list( cmd, &( dev->at_head ) );
			/*when all alloc, add ATMIPPUSH command*/
			dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/50 );
			if( cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ) 
			{
				dev->mqtt_dev->pub_out_num++;
			}
		}
	} 
	else 
	{
		/*mqtt message that don't need to cached the buffer areadly add ATMIPPUSH command before */
		add_cmd_to_list( cmd, &dev->at_head );
		if( cmd->mqtype == MQTT_MSG_TYPE_CONNECT || cmd->mqtype == MQTT_MSG_TYPE_PINGREQ )
		{
			cmd->msgid = 0;
		}
	}

}

/*
* author:	yangjianzhou
* function: 	make_mqtt_command_to_list,  make a AtCommand and add to at_head list.
*/
static void make_mqtt_command_to_list( void *argc, char index, unsigned int interval,  int para )
{
	AtCommand *cmd;	
	DevStatus *dev = ( DevStatus * ) argc;
	
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}	
	
	cmd = alloc_command();
	
	if( cmd != NULL ) 
	{
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
		fill_cmd_mqtt_part( dev, cmd );
	}
	else
	{
		printf("%s: make node fail!\r\n", __func__);
	}
}

/*
* author:	yangjianzhou
* function: 	atcmd_set_ack,  set ack for nomal AT command.
*/
static void atcmd_set_ack( void *argc, int index )
{
	int del_done = 0;
	AtCommand *cmd = NULL;
	DevStatus *dev = ( DevStatus * ) argc;
	struct list_head *pos, *n;
	
	printf("%s: target %s\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, index ) );
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}	
	if( dev->atcmd != NULL && dev->atcmd->index == index ) 
	{
		del_done = 1;
		dev->atcmd->atack = 1;
		printf("%s: [%s] is ACK, REMOVE it!\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, index ));
		return;
	}
	else
	{
		list_for_each_safe( pos, n, &dev->at_head )
		{
			cmd = list_entry( pos, AtCommand, list );
			
			if( cmd->index == index ) 
			{
				del_done = 1;
				cmd->atack = 1;
				printf("%s:[%s] is ACK, REMOVE it!\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, index ));
				return;
			}
		}
	}
	
	list_for_each_safe( pos, n, &dev->at_wait_head )
	{
		cmd = list_entry( pos, AtCommand, list );
		
		if( cmd->index == index ) 
		{
			del_done = 1;
			cmd->atack = 1;
			printf("%s:[%s] is ACK, REMOVE it!\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, index ));
			return;
		}
	}

	if( del_done == 0 )
	{
		printf("%s: set [%s] fail!\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, index ));
	}		
}

/*
* author:	yangjianzhou
* function: 	mqtt_set_mesg_ack,  set ack for nomal mqtt command, when qos=2, deal more later.
*/
static void mqtt_set_mesg_ack( void *argc, int type, uint16_t msg_id )
{
	int del_done = 0;
	AtCommand *cmd;
	struct list_head *pos, *n;
	DevStatus *dev = ( DevStatus *) argc;

	//printf("%s: dev(%p)\r\n", __func__, dev);

	if( !dev )
	{
		printf("%s: dev null pointer!\r\n", __func__);
		return;
	}
	
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
			}
		}
	}
	
	if( del_done == 0 )
	{
		printf("%s: set [%s] fail! [type=%d, msgid=%d]\r\n", __func__, 
			mqtt_get_name(type), type, msg_id );
	}	
}

/*
* author:	yangjianzhou
* function: 	release_command,  删除链表节点，释放AtCommand.
*/
static void release_command( AtCommand *cmd, struct list_head *head, struct list_head *pos )
{
	//DevStatus *dev = (DevStatus *)( cmd->priv );

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
		dealloc_mqtt_buffer( ( char* ) cmd->mqttdata );
	}
	
	dealloc_command( cmd );
}

/*
* author:	yangjianzhou
* function: 	release_list_command,  release all command is the list.
*/
static void release_list_command( struct list_head *head )
{
	int total = 0;
	AtCommand *cmd = NULL;
	struct list_head *pos, *n;
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	list_for_each_safe( pos, n, head )
	{
		total++;
		cmd = list_entry( pos, AtCommand, list );
		release_command( cmd, NULL, pos );
	}

	if( !dev )
	{
		printf("%s: StoragePointer fail!\r\n", __func__);
		return;
	}
	else
	{
		printf("%s: release %d cmd form %s\r\n", __func__, total, 
			(&(dev->at_head) == head)?"at_head":((&(dev->mqtt_head)
			== head)?"mqtt_head":"at_wait_head"));
	}
}

/*
* author:	yangjianzhou
* function: 	at_wait_list_cmd,  store AT command for send again.
*/
static void at_wait_list_cmd( DevStatus *dev )
{
	unsigned int tick;
	AtCommand *cmd = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe( pos, n, &dev->at_wait_head )
	{
		cmd = list_entry( pos, AtCommand, list );

		tick = NOW_TICK;
		cmd->tick_sum += tick - cmd->tick_tag;
		cmd->tick_tag = tick;
		
		if( cmd->atack == 1 )
		{	
			printf("%s: releas command %s!\r\n", __func__, dev->module->d_ops->at_get_name( dev->module, cmd->index ));
			release_command( cmd, &dev->at_wait_head, NULL );
		}	
		else if( cmd->tick_sum >= cmd->interval )
		{
			list_del( pos );

			if( !dev->module->d_ops->is_tcp_connect_server( dev->module, cmd->index ) )
			{
				cmd->interval = cmd->interval_a;
				add_cmd_to_list( cmd, &dev->at_head );
				dev->ops->wake_up_system( dev );
				cmd->tick_sum = 0;
				printf("%s: add [%s] to AT LIST HEAD again! tick(%u)\r\n", 
					__func__, dev->module->d_ops->at_get_name( dev->module, cmd->index ), NOW_TICK);
			}
			else
			{
				if( dev->socket_num == 0 /*&& dev->ppp_status != PPP_CONNECTED*/ )
				{	
					cmd->interval = cmd->interval_a;
					add_cmd_to_list( cmd, &dev->at_head );
					dev->ops->wake_up_system( dev );
					cmd->tick_sum = 0;
					printf("%s: add [%s] to AT LIST HEAD again! tick(%u)\r\n", __func__,
						dev->module->d_ops->at_get_name( dev->module, cmd->index ), NOW_TICK);					
				}
				else
				{
					printf("%s: warnning->release command whatevet! ip(%s), command(%s)\r\n", 
						__func__, dev->ip, dev->module->d_ops->at_get_name( dev->module, cmd->index ));
					release_command( cmd, NULL, NULL );
				}
			}
		}
		else 
		{
			if( dev->wu_tick > cmd->interval - cmd->tick_sum )
			{
				dev->wu_tick = cmd->interval - cmd->tick_sum;
			}	
		}
	}
	
}

/*
* author:	yangjianzhou
* function: 	mqtt_list_cmd,  store mqtt command for send again.
*/
static void mqtt_list_cmd( DevStatus *dev )
{
	int i = 0;
	unsigned int tick;
	AtCommand *cmd = NULL;
	struct list_head *pos = NULL, *n = NULL;

	list_for_each_safe( pos, n, &dev->mqtt_head )
	{	
		cmd = list_entry( pos, AtCommand, list );

		tick = NOW_TICK;
		cmd->tick_sum += tick - cmd->tick_tag;
		cmd->tick_tag = tick;
	
		if( cmd->mqtt_clean ) 
		{
			printf("%s: clean message [%d].\r\n", __func__, i++);
			release_command( cmd, &dev->mqtt_head, NULL );
		}
		else if( cmd->mqttack == 1 )
		{
			//printf("%s: mqtt mesg is ack.\r\n", __func__);
			release_command( cmd, &dev->mqtt_head, NULL );
		}
		else if( cmd->tick_sum >= cmd->interval )		
		{ 		
			list_del( pos );
			cmd->interval = ONE_SECOND / 20;
			cmd->tick_sum = 0;
			
			/*目前只有MQTT_OUTDATA_PUBLISH才需要检查 mqttdata*/
			if( cmd->mqttdata == NULL && ( cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ) &&
					( cmd->mqtype == dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREL ) &&
					( cmd->mqtype == dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREC ) ) 
			{
				dealloc_command( cmd );
				printf("ERROR: [%s] mqttdata is NULL remote it from mqtt_head.\r\n", mqtt_get_name( cmd->mqtype ));
			}
			else if( ++( cmd->mqtt_try ) > 2 )
			{
				/*超过3次, 断开MQTT的连接*/
				printf("ERROR: [%s mqtt_try > 2 ], type=%d,id=0X%04X,len=%d,p=0x%p\r\n", mqtt_get_name( cmd->mqtype ), 
						cmd->mqtype, cmd->msgid, cmd->mqttdata_len, cmd->mqttdata );
				/*只要发送DISCONNECT包就可以了, 然后再断开TCP连接*/
				dev->ops->make_mqtt_command(dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_DISCONNECT );
				dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/2 );
				dev->close_tcp_interval = 3 * ONE_SECOND;
				dev->tick_sum = 0;
				dev->tick_tag = NOW_TICK;
				release_command( cmd, NULL, NULL );
				if( dev->wu_tick > dev->close_tcp_interval )
				{
					dev->wu_tick = dev->close_tcp_interval;
				}				
			}
			else
			{
				/*目前根据协议只有PUBLISH、SUBSCRIBE用得到，后续添加*/
				if( mqtt_get_qos( cmd->mqttdata ) && ( cmd->mqtype == MQTT_MSG_TYPE_PUBLISH ||
							cmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE ) )
				{
					mqtt_set_dup( cmd->mqttdata );
				}
				add_cmd_to_list( cmd, &dev->at_head );
				dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/40 );
				dev->ops->wake_up_system( dev );
				printf("%s: add [%s] to AT LIST HEAD again! type=%d, msgid=0x%04x!\r\n", 
						__func__, mqtt_get_name( cmd->mqtype ), cmd->mqtype, cmd->msgid);			
			}
		} 	
		else 
		{
			if( dev->wu_tick > cmd->interval - cmd->tick_sum )
			{
				dev->wu_tick = cmd->interval - cmd->tick_sum;
			}	
		}
	}
	
}

/*
* author:	yangjianzhou
* function: 	at_list_cmd,  list all command and send it to module.
*/
static void at_list_cmd( DevStatus *dev )
{
	if( dev->atcmd == NULL ) 
	{
		dev->atcmd = get_cmd_from_list( &dev->at_head );
		
		if( dev->atcmd && dev->atcmd->mqtt_clean ) 
		{
			printf("%s: mqtt_clean#\r\n", __func__);
			release_command( dev->atcmd, &dev->at_head, NULL );
			dev->atcmd = NULL;
			dev->ops->wake_up_system( dev );
		} 
		else if( dev->atcmd ) 
		{
			dev->module->d_ops->send_command_to_module( dev->module, dev->atcmd );
			dev->atcmd->interval = dev->atcmd->interval_a;
			dev->atcmd->tick_tag = NOW_TICK;
			dev->atcmd->tick_sum = 0;
			if( dev->wu_tick > dev->atcmd->interval )
			{
				dev->wu_tick = dev->atcmd->interval;
			}
		}
	} 
	else 
	{
		unsigned int tick = NOW_TICK;
		dev->atcmd->tick_sum += tick - dev->atcmd->tick_tag;
		dev->atcmd->tick_tag = tick;
		
		if( dev->atcmd->mqtt_clean ) 
		{
			printf("%s: mqtt_clean(%d)##\r\n", __func__, dev->atcmd->mqtt_clean);
			release_command( dev->atcmd, &dev->at_head, NULL );
			dev->atcmd = NULL;
			dev->ops->wake_up_system( dev );
		}
		else if( dev->atcmd->tick_sum >= dev->atcmd->interval )
		{
			del_cmd_from_list( dev->atcmd, &dev->at_head );
			
			/*目前只有两个消息需要检查回复, 新加入MQTT_DEV_STATUS_CONNECT*/
			if( ( ( dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBLISH && dev->atcmd->mqttdata != NULL && 
						mqtt_get_qos( dev->atcmd->mqttdata ) ) || dev->atcmd->mqtype == MQTT_MSG_TYPE_SUBSCRIBE ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_CONNECT || dev->atcmd->mqtype == MQTT_MSG_TYPE_PINGREQ ||
						dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREC || dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBREL ) && 
						( dev->atcmd->mqttack == 0 ) ) 
			{
				/*wait for 5 second*/
				if( dev->atcmd->mqtype == MQTT_MSG_TYPE_PINGREQ )
				{
					dev->atcmd->interval = 7 * ONE_SECOND;
				}
				else 
				{
					dev->atcmd->interval = 10 * ONE_SECOND;
				}
				dev->atcmd->tick_sum = 0;
				add_cmd_to_list( dev->atcmd, &dev->mqtt_head );
				printf("ADD [%s] to mqtt_head to wait ACK! msgid(0x%04x), qos(%d)\r\n", 
						mqtt_get_name(dev->atcmd->mqtype), dev->atcmd->msgid, 
						mqtt_get_qos( dev->atcmd->mqttdata ) );
			}
			/* AT command need to be resend*/
			else if( dev->atcmd->index != ATMQTT && dev->atcmd->wait_flag && dev->atcmd->atack == 0 )
			{
				dev->atcmd->interval = dev->atcmd->interval_b;
				dev->atcmd->tick_sum = 0;
				add_cmd_to_list( dev->atcmd, &dev->at_wait_head );
				printf("ADD [%s] to at_wait_head to wait ACK! tick(%u)\r\n", 
					dev->module->d_ops->at_get_name( dev->module, dev->atcmd->index ), NOW_TICK);				
			}
			else 
			{
				if( dev->atcmd->index == ATMQTT )
				{
					printf("%s: release command (%s)\r\n", __func__, 
						dev->module->d_ops->at_get_name( dev->module, dev->atcmd->index ));
					
					if( dev->atcmd->mqtype == MQTT_MSG_TYPE_PUBLISH && dev->atcmd->mqttdata != NULL )
					{
						printf("release [%s]! msgid(0x%04x), qos(%d)\r\n", 
							mqtt_get_name( dev->atcmd->mqtype ), dev->atcmd->msgid, 
							mqtt_get_qos( dev->atcmd->mqttdata ) );
					}
				}
				else
				{
					//just print ATMIPOPEN and ATMIPHEX for debug.
					printf("%s: release(%s).\r\n", __func__, 
							dev->module->d_ops->at_get_name( dev->module, dev->atcmd->index ));
				}
				release_command( dev->atcmd, NULL, NULL );
			}
			
			dev->atcmd = NULL;
			dev->ops->wake_up_system( dev );
		}
		else
		{
			if( dev->wu_tick > dev->atcmd->interval - dev->atcmd->tick_sum )
			{
				dev->wu_tick = dev->atcmd->interval - dev->atcmd->tick_sum;
			}
		}
	}		
}

/*
* author:	yangjianzhou
* function: 	set_mqtt_cmd_clean,  set all mqtt message to clean status.
*/
static void set_mqtt_cmd_clean( void *argc )
{
	int total = 0;
	AtCommand *cmd = NULL;
	DevStatus *dev = ( DevStatus * ) argc;
	struct list_head *pos, *n;

	printf("%s:###\r\n", __func__);
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}	
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

/*
* author:	yangjianzhou
* function: 	check_tcp_close_interval,  for delay set socket_close_flag to 1.
*/
static void check_tcp_close_interval( DevStatus *dev )
{
	if( dev->close_tcp_interval > 0 ) 
	{
		unsigned int tick = NOW_TICK;
		dev->tick_sum += tick - dev->tick_tag;
		dev->tick_tag = tick;
		
		if( dev->wu_tick > dev->close_tcp_interval - dev->tick_sum )
		{
			dev->wu_tick = dev->close_tcp_interval - dev->tick_sum;
		}				
		if( dev->tick_sum >= dev->close_tcp_interval )
		{
			dev->close_tcp_interval = 0;
			dev->socket_close_flag = 1;
			printf("%s: set socket_close_flag to 1\r\n", __func__);
		}
	}
}


/*
* author:	yangjianzhou
* function: 	check_socket_number,  cacluate socket number.
*/
static void check_socket_number( DevStatus *dev )
{
	int i;

	/*检查活跃的socket个数，保持一个连接*/
	dev->socket_num = 0;

	for( i=0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
	{
		if( dev->socket_open[i] != -1 )
		{
			dev->socket_num++;
		}
	}

	/*若发现socket大于1，关闭所有socket,重新建立连接*/
	if( ( dev->socket_num > 1 || dev->socket_close_flag ) ) 
	{
		dev->module->d_ops->close_module_socket( dev->module ); 					
		if( dev->socket_close_flag == 1 )
		{
			dev->socket_close_flag = 0;
		}
	}	
	else if( dev->socket_num == 0 && dev->tcp_connect_status )
	{
		/* for fix a bug 2016/11/11 */
		dev->tcp_connect_status = 0;
		if( dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT )
		{
			dev->mqtt_dev->connect_status = MQTT_DEV_STATUS_NULL;
		}
		printf("%s: catch bug****\r\n", __func__);
	}	
}

/*
* author:	yangjianzhou
* function: 	list_mqtt_event,  add tcp data ( mqtt connect packet or mqtt tick packet ) to module.
*/
static void list_mqtt_event( DevStatus *dev )
{
	if( dev->socket_num == 1 ) 
	{	//触发条件
		if( dev->ppp_status == PPP_CONNECTED && dev->tcp_connect_status ) 
		{	//状态
			/*连接上MQTT*/
			if( dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_NULL ||
				 dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_DISCONNECT ) 
			{	//二级状态
				/*连接前把  mqtt_list at_list 的mqtt消息清空！在+MIPCLOSE:清空*/			
				dev->mqtt_dev->connect_status = MQTT_DEV_STATUS_CONNECTING;
				dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/20, MQTT_MSG_TYPE_CONNECT );
				dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/25 );
			}		
			/*发送心跳包给服务 50s每个tick*/				
			if( dev->mqtt_heartbeat >= 3 ) 
			{	//触发条件
				if(dev->mqtt_dev->connect_status != MQTT_DEV_STATUS_NULL
						&& dev->mqtt_dev->connect_status != MQTT_DEV_STATUS_CONNECTING ) 
				{	//二级状态
					dev->mqtt_heartbeat = 0;
					if( dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT ) 
					{
						//有可能是其它的状态MQTT_DEV_STATUS_CONNACK  MQTT_DEV_STATUS_SUBACK
						dev->ops->make_mqtt_command( dev, ATMQTT, ONE_SECOND/40, MQTT_MSG_TYPE_PINGREQ );
						dev->module->d_ops->push_socket_data( dev->module, ONE_SECOND/50 );
					}
				}									
			} 
		}
	}	
}

/*
* author:	yangjianzhou
* function: 	tcp_connect_server,  make module tcp connect to remote service.
*/
static void tcp_connect_server( DevStatus *dev )
{
	/*连接上远程服务端*/
	if( dev->socket_num == 0 ) 
	{	//触发条件
		if( dev->ppp_status == PPP_CONNECTED && dev->simcard_type > 0 ) 
		{	//状态
			dev->module->d_ops->tcp_connect_server( dev->module );
			dev->mqtt_heartbeat = 6;
		}
	}	
}

/*
* author:	yangjianzhou
* function: 	check_sm_and_ppp,  poll sm status and module's ip.
*/
static void check_sm_and_ppp( DevStatus *dev )
{
	if( dev->simcard_type > 0 ) 
	{
		/*查询SM短信未读index*/
		/*查询SM短信未读index,查询到有短信，就会马上进入读模块代码*/
		dev->module->d_ops->check_module_sm( dev->module );	

		/*查询IP*/
		dev->module->d_ops->check_module_ip( dev->module );	
		
	}

	/*PPP连接获取IP*/		
	if( dev->simcard_type > 0 ) 
	{	//出发条件
		if( dev->ppp_status == PPP_DISCONNECT ) 
		{ 	//状态
			dev->ppp_status = PPP_CONNECTING;//修改状态
			/*ppp for get ip*/
			dev->module->d_ops->module_request_ip( dev->module ); 
		}						
	}	
}

/*
* author:	yangjianzhou
* function: 	check_reset_condition,  check if need to reset module.
*/
static void check_reset_condition( DevStatus *dev )
{
	/*if there have not sim card, we reset communicate module for something 4 peroid*/
	if( dev->simcard_type <= 0 && dev->scsq > 4 ) 
	{
		dev->reset_request = 1;
		dev->reset_times++;
		dev->ops->wake_up_system( dev );
		printf("sim card no exit! reset the communication module!\r\n");
	}	

	if( ( dev->scsq ) - ( dev->rcsq ) > 4 )
	{
		dev->reset_request = 1;
		dev->reset_times++;
		if( dev->socket_num > 0 )
		{
			dev->socket_close_flag = 1;
		}
		dev->ops->wake_up_system( dev );
		printf("scsq-rcsq=%d! reset the communication module!\r\n", 
			dev->scsq - dev->rcsq );
	}
}

/*
* author:	yangjianzhou
* function: 	initialise_module,  init module by AT command.
*/
static void initialise_module( DevStatus *dev )
{
	dev->module->d_ops->initialise_module( dev->module );	
	dev->mqtt_heartbeat = 6;				
}

/*
* author:	yangjianzhou
* function: 	poll_module_peroid,  poll module instance for peroid data.
*/
static void poll_module_peroid( DevStatus *dev )
{
	dev->module->d_ops->poll_module_peroid( dev->module );	
	dev->scsq++;
}

/*
* author:	yangjianzhou
* function: 	list_sm_event,  poll sm event.
*/
static void list_sm_event( DevStatus *dev )
{
	/*删除sim卡中的短信, 短信相关的AT指令时间要长些NE_SECOND/5~ONE_SECOND/2*/
	if( dev->sm_index_delete != -1 && dev->sm_delete_flag ) 
	{
		dev->module->d_ops->delete_module_sm( dev->module, dev->sm_index_delete );	
		dev->sm_delete_flag = 0;
	}

	/*读取sim卡中的短信*/
	if( dev->sm_num > 0 && dev->sm_read_flag ) 
	{
		dev->module->d_ops->read_module_sm( dev->module, dev->sm_index[0] );	
		dev->sm_read_flag = 0;
	}	
}

/*
* author:	yangjianzhou
* function: 	mqtt_disconnect_server,  make and send mqtt disconnect packet.
*/
static void mqtt_disconnect_server( mqtt_dev_status *mqtt_dev )
{
	char buff[128];
	DevStatus *dev = ( DevStatus * ) mqtt_dev->p_dev;
	mqtt_state_t *state = mqtt_dev->mqtt_state;
	
	printf("%s.\r\n", __func__);
	memset( buff, '\0', sizeof( buff ) );
	state->outbound_message = mqtt_msg_disconnect( &state->mqtt_connection );
	
	if( dev->module->d_ops->make_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length ) )
	{
		vTaskDelay( 15 / portTICK_RATE_MS );
		dev->module->d_ops->send_tcp_packet( buff, state->outbound_message->data, state->outbound_message->length );
		vTaskDelay( 15 / portTICK_RATE_MS );
		dev->module->d_ops->send_push_data_directly( dev->module );
		vTaskDelay( 100 / portTICK_RATE_MS );
	}		
}

/*
* author:	yangjianzhou
* function: 	clean_run_status,  clean module running status.
*/
static void clean_run_data( DevStatus *dev )
{
	int i;

	release_list_command( &dev->at_head );
	release_list_command( &dev->mqtt_head );
	release_list_command( &dev->at_wait_head );
	
	if( dev->mqtt_dev->connect_status == MQTT_DEV_STATUS_CONNECT )
	{
		mqtt_disconnect_server( dev->mqtt_dev );
		vTaskDelay( 500 / portTICK_RATE_MS );		
	}
	for( i = 0; i < SIZE_ARRAY( dev->socket_open ); i++ ) 
	{
		if( dev->socket_open[i] != -1 ) 
		{ 
			vTaskDelay( 10 / portTICK_RATE_MS );
			dev->module->d_ops->close_module_socket_directly( dev->module, i );
			vTaskDelay( 300 / portTICK_RATE_MS );				
		}
	}	
}

/*
* author:	yangjianzhou
* function: 	module_reset_work,  reset the status of running.
*/
static void module_reset_work( DevStatus* dev )
{
	/*reset timer when android power down.*/
	if( pdFAIL == xTimerReset( dev->hb_timer, 2 ) )
	{
		printf("%s fail to xTimerReset timer!\r\n", __func__);
	}
	else
	{
		printf("%s hb_timer reset to work.\r\n", __func__);
	}
	
	reset_module_status( dev, 0 );
	reset_module_reader( dev, dev->reader );
	reset_mqtt_dev( dev, dev->mqtt_dev );
	rfifo_clean( dev->uart_fifo );
	/*power reset by gpio control*/
	dev->module->module_power_reset( dev->module );
	release_list_command( &dev->at_head );
	release_list_command( &dev->mqtt_head );
	release_list_command( &dev->at_wait_head );	
}

/*
* author:	yangjianzhou
* function: 	handle_module_setting, handle module running status.
*			状态机模式： 横竖两种写法
*/
static void handle_module_setting(  DevStatus* dev )
{
	char period_flag = 0;
	
	dev->wu_tick = portMAX_DELAY;
	if( xTimerIsTimerActive( dev->wu_timer ) != pdFALSE )
	{
		if( pdFAIL == xTimerStop( dev->wu_timer, 0 ) )
		{
			printf("%s fail to stop wu_timer\r\n", __func__);
		}
	}
	/*android power off, reboot module*/
	if( dev->reset_request ) 
	{	
		module_reset_work( dev );
		dev->module->d_ops->hardware_reset_callback( dev->module );
	}
	
	if( dev->period_flag ) 
	{
		/*check at every hb_timer peroid*/
		dev->period_flag = 0;
		dev->mqtt_heartbeat++;		
		period_flag = 1;	
		
		printf("*********************************\r\n");
		poll_module_peroid( dev );

		if( !dev->module->is_initialise ) 
		{				
			initialise_module( dev );
		}			
	}

	if( dev->boot_status ) 
	{
		if( period_flag ) 
		{
			/*check at every hb_timer peroid*/
			check_reset_condition( dev );		
			check_sm_and_ppp( dev );
		}

		check_tcp_close_interval( dev );
		check_socket_number( dev );
			
		if( !dev->tcp_connect_status ) 
		{ 
			tcp_connect_server( dev );
		}
		list_mqtt_event( dev );
		list_sm_event( dev );
	}

	at_list_cmd( dev );
	mqtt_list_cmd( dev );	
	at_wait_list_cmd( dev );
	
	if( dev->wu_tick != portMAX_DELAY )
	{
		if( pdFAIL == xTimerChangePeriod( dev->wu_timer, dev->wu_tick, 2 ) )
		{
			printf("%s fail to xTimerChangePeriod wu_timer!\r\n", __func__);
		}	
	}
}

/*
* author:	yangjianzhou
* function: 	notify_module_period, need to invoved in every 50s.
*/
static void notify_module_period_timer( TimerHandle_t xTimer )
{	
	eLogLevel level = ucGetTaskLogLevel( pxModuleTask );

	DevStatus * dev = ( DevStatus * ) pvTimerGetTimerID( xTimer );	
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}

	if( level <= ucOsLogLevel )
	{	
		printf_system_jiffies();
	}
	dev->period_flag = 1;
	dev->ops->wake_up_system( dev );
}

/*
* author:	yangjianzhou
* function: 	wake_up_module_timer, wake up HandleModuleTask.
*/
static void wake_up_module_timer( TimerHandle_t xTimer )
{	
	DevStatus * dev = ( DevStatus * ) pvTimerGetTimerID( xTimer );	
	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return;
	}
	dev->ops->wake_up_system( dev );
}

/*
* author:	yangjianzhou
* function: 	stop_os_timer, stop timer cause android start.
*/
static void stop_os_timer( TimerHandle_t timer )
{
	if( !timer )
	{
		return;
	}
	if( xTimerIsTimerActive( timer ) != pdFALSE )
	{
		if( pdFAIL == xTimerStop( timer, 0 ) )
		{
			printf("%s fail to stop os_timer\r\n", __func__);
		}
		else
		{
			printf("%s stop os_timer ok.\r\n", __func__);
		}
	}
}

/*
* author:	yangjianzhou
* function: 	module_clean_work  clean module system.
*/
static void module_clean_work( DevStatus* dev )
{
	stop_os_timer( dev->hb_timer );
	stop_os_timer( dev->wu_timer );
	
	clean_run_data( dev );
	rfifo_clean( dev->uart_fifo );
	reset_module_status( dev, 1 );
	reset_mqtt_dev( dev, dev->mqtt_dev );
	/*gpio reset module power*/
	dev->module->module_power_reset( dev->module ); 	
}

void load_sim900a_instance( TaskHandle_t task );

/*
* author:	yangjianzhou
* function: 	system_lock, lock the system.
*/
void system_lock ( void *dev, unsigned int tick )
{
	
	xSemaphoreTake( ( ( DevStatus * )dev )->system_mutex, tick );
}

/*
* author:	yangjianzhou
* function: 	systme_unlock, unlock system.
*/
void systme_unlock ( void *dev )
{
	xSemaphoreGive( ( ( DevStatus * )dev )->system_mutex );
}

/*
* author:	yangjianzhou
* function: 	system_sleep, system block here.
*/
void system_sleep( void *dev, unsigned int tick )
{
	ulTaskNotifyTake( pdTRUE, tick );
}

/*
* author:	yangjianzhou
* function: 	wake_up_system, wake up the system go to run.
*/
void wake_up_system( void *dev )
{
	xTaskNotifyGive( ( ( DevStatus * )dev )->pxModuleTask );	
}

/*
* author:	yangjianzhou
* function: 	module_system_init  init module system.
*/
static void module_system_init( DevStatus* dev )
{	
	static struct core_operations sys_ops =
	{
		check_at_command_exist,
		make_at_command_to_list,
		make_mqtt_command_to_list,
		atcmd_set_ack,
		set_mqtt_cmd_clean,
		mqtt_set_mesg_ack,
		system_lock,
		systme_unlock,
		system_sleep,
		wake_up_system,
	};
	dev->malloc_count = 0;
	dev->free_count = 0;
	dev->ops = &sys_ops;
	dev->module_list = NULL;
	dev->system_mutex = xSemaphoreCreateMutex();
	dev->list_mutex = xSemaphoreCreateMutex();
	
 	dev->hb_timer = xTimerCreate( "heartbeat", 30000 / portTICK_RATE_MS, 
						pdTRUE, dev, notify_module_period_timer );
	
	dev->wu_timer = xTimerCreate( "wakeup", 100 / portTICK_RATE_MS, 
						pdFALSE, dev, wake_up_module_timer );

	init_command_buffer( &( dev->p_atcommand ), 15 );
	init_mqtt_buffer( dev->p_mqttbuff, 3 );

	reset_module_reader( dev, dev->reader );
	reset_mqtt_dev( dev, dev->mqtt_dev );	
	reset_module_status( dev, 1 );
	
	INIT_LIST_HEAD( &dev->at_head );
	INIT_LIST_HEAD( &dev->mqtt_head );
	INIT_LIST_HEAD( &dev->at_wait_head );	
}

/*
* author:	yangjianzhou
* function: 	notify_android_power_on  can not call in interrupt.
*/
void notify_android_power_on( void )
{
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: pvTaskGetThreadLocalStoragePointer fail!\r\n", __func__);
		return;
	}

	//android_power_on();
	dev->android_power_status = 1;
	dev->ops->wake_up_system( dev );
}

/*
* author:	yangjianzhou
* function: 	notify_android_power_off  can not call in interrupt.
*/
void notify_android_power_off( void )
{
	DevStatus *dev = pvTaskGetThreadLocalStoragePointer( pxModuleTask, 0 );

	if( !dev )
	{
		printf("%s: pvTaskGetThreadLocalStoragePointer fail!\r\n", __func__);
		return;
	}

	//android_power_off();
	dev->android_power_status = 0;
	dev->ops->wake_up_system( dev );
}

/*
* author:	yangjianzhou
* function: 	find_communication_module.
*/
static ComModule **find_communication_module( DevStatus* dev, const char *name, unsigned len )
{
    ComModule **p;
	
    for ( p = &( dev->module_list ); *p; p = &( *p )->next )
    {
        if ( strlen( ( *p )->name ) == len && strncmp( ( *p )->name, name, len ) == 0 )
        {
        	break;
        }
    }
	
	return p;
}

/*
* author:	yangjianzhou
* function: 	register_communication_module, register a ComModule instance to core.
*/
int register_communication_module( TaskHandle_t task, ComModule * instance )
{
    int res = 0;
    ComModule ** p;
	DevStatus* dev = pvTaskGetThreadLocalStoragePointer( task, 0 );

	if( !dev )
	{
		printf("%s: dev null pointer error!\r\n", __func__);
		return -1;
	}
	
    if ( instance->next )
    {
        return -1;
    }
	
	xSemaphoreTake( dev->list_mutex, portMAX_DELAY );
	
    p = find_communication_module( dev, instance->name, strlen( instance->name ) );

	if ( *p )
    {
        res = -1;
    }
    else
    {
    	*p = instance;
    }
	
	xSemaphoreGive( dev->list_mutex );
	
    return res;
}

/*
* author:	yangjianzhou
* function: 	get_communication_module, get a ComModule by name.
*/
static ComModule *get_communication_module( DevStatus* dev, const char *name )
{
    ComModule *instance;
    int len = strlen( name );

	xSemaphoreTake( dev->list_mutex, portMAX_DELAY );
    instance = *( find_communication_module( dev, name, len ) );
	xSemaphoreGive( dev->list_mutex );

    return instance;
}

/*
* author:	yangjianzhou
* function: 	find_module, find a ComModule by name, block until it has registered.
*/
static void find_module( DevStatus* dev , const char * name )
{
	while( !dev->module )
	{
		dev->module = get_communication_module( dev, name );
		
		if( !dev->module )
		{
			printf("%s: find communication module fail!\r\n", __func__);
			vTaskDelay( 1000 );
		}
		else
		{
			dev->module->p_dev = dev;
		}
	}
}

void load_a6_instance( TaskHandle_t task );

/*
* author:	yangjianzhou
* function: 	HandleModuleTask  handle module system.
*/
void HandleModuleTask( void * pvParameters )
{	
	static DevStatus dev[1];
	static UartReader reader[1];
	static mqtt_dev_status mqtt_dev[1];

	dev->uart_fifo = uart2fifo;
	dev->pxModuleTask = pxModuleTask;
	dev->mqtt_dev = mqtt_dev;
	dev->reader = reader;
	dev->android_power_status = 0;
	
	vSetTaskLogLevel( NULL, eLogLevel_3 );	
	vTaskSetThreadLocalStoragePointer( pxModuleTask, 0, dev );
	
	rfifo_init( dev->uart_fifo, 1024 * 1/*2*/ );	
	//uart3_init( 115200 );
	module_system_init( dev );

	/*we can instance communication module now*/
	load_longsung_instance( pxModuleTask );
	
	vTaskDelay( 500 / portTICK_RATE_MS );
	printf("%s: start...\r\n", __func__);

	find_module( dev, "longsung" );
	/*sim900a,  longsung, a6-GPRS, sim800c add later*/
	
	/*notifyAndroidPowerOn();*/
	while( 1 )
	{
		while( dev->android_power_status )
		{
			dev->ops->system_sleep( dev, portMAX_DELAY );
		}
		printf("%s: android power off, go to work!!!\r\n", __func__);

		rfifo_clean( dev->uart_fifo );
		dev->ops->wake_up_system( dev );
		
		while( !dev->android_power_status )
		{	
			dev->ops->system_sleep( dev, portMAX_DELAY );
			
			dev->ops->system_lock( dev, portMAX_DELAY );
			handle_module_uart_msg( dev );
			handle_module_setting( dev );			
			dev->ops->system_unlock( dev );
		}
		
		dev->ops->system_lock( dev, portMAX_DELAY );
		module_clean_work( dev );
		dev->ops->system_unlock( dev );

		printf("%s: android power on, go to sleep!!!\r\n", __func__);		
	}
}

//+STIN:20
