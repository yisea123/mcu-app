#include <stdio.h>
#include <string.h>
#include "md5.h"
#include "ff.h"
#include "uartprotocol.h"
#include "ymodem.h"
#include "handlers.h"

//#include "rtc.h"
//#include "stmflash.h"
//#include "stm32f4xx_flash.h"

/**********************************************************************

#define SOH                     (0x01) 
#define STX                     (0x02)
#define EOT                     (0x04)  
#define ACK                     (0x06) 
#define UPDATE                  (0x08) 
#define UPDATE_DONE             (0x09) 
#define NAK                     (0x15)  
#define CA                      (0x18)  

#define LAST_ACK				(0x16)
#define EOT_ACK					(0x17)

#define ERR_PACKET_INDEX        (0x20)
#define ERR_SIZE_EXT            (0x21)
#define ERR_FLASH_RW            (0x22)
#define ERR_TRANSMISS_START     (0x23)
#define ERR_UPDATE_REQUEST      (0x24)
#define CRC16                   (0x43) 


串口协议:   aa bb len(1) msgid(2) type(1) d0~dn crc(2)

本协议由Ymodem修改而来，适配我们android与MCU通讯时的协议，                     
PS: mPacketSize
PR: pReceived & 0xff
ack: 为串口通讯协议层的ack
UPDATE,PS:表示mcu发送的数据有两个AA BB 0X02 07 80 01 UPDATE PS CRC1 CRC2
C :表示mcu发送的数据有一个AA BB 0X01 07 80 01 C CRC1 CRC2,   C就是CRC16(0X43)


1、	android 下发更新rom请求： 更新由android系统触发                                      
AA BB 2  0x0B 0x00 0x01 UPDATE  devNum CRC1 CRC2	
	  
	  				ack                                    
					UPDATE,PS
					ACK,PR  							pReceived=0 packetNum = 1 
					2、 mcu上报C命令                       
					C     
					
3、 anroid 下发rom的文件名，以及文件大小                              
AA BB 130  0x0B 0x00 0x01 00 FF [foo.c/size/aeskey....] = 128 CRC1 CRC2
		
					ack                 
					ACK,PR  							pReceived=1  packetNum = 2
					4、 mcu上报C命令                       
					C             
					
5、 android 连续下发rom文件数据，最后一包不足128bytes时填充0          
AA BB 130  0x0B 0x00 0x01 01 FE Data[128] CRC1 CRC2     
		
					ack
					ACK,PR   						pReceived=2  packetNum = 3
					
AA BB 130  0x0B 0x00 0x01 02 FD Data[128] CRC1 CRC2		
		
					ack
					ACK,PR   						pReceived=3  packetNum = 4
					
		.....                                                                                
					ack
                                  ACK,PR  							 pReceived=4  packetNum = 5
		.....                                                                                
					ack
					ACK,PR   						pReceived=5  packetNum = 6
																									                                       
6、 android 连续 下发两条end of transmission（EOT）包									                   
AA BB 1  0x0B 0x00 0x01 EOT CRC1 CRC2     
		
					ack                                    
					EOT_ACK    						packetNum = 7               
					7、 MCU收到第一条EOT包回复NAK命令包		 
					NAK                         				 packetTotal = 4           
					
8、 android下发第二条EOT包																						                   
AA BB 1  0x0B 0x00 0x01  EOT CRC1 CRC2    
		
					ack                                    
					EOT_ACK    						packetNum = 8               
					9、 MCU回复C命令包										 
					C                                      
					
10、android下发rom更新结束包，MCU收到 后更新成功！										                   
AA BB 130  0x0B 0x00 0x01 00 FF NULL[128] CRC1 CRC2            
		
					ack		
					LAST_ACK    						packetNum = 9              
					UPDATE_DONE                            

***********************************************************************/

static YmodemHandler *handler = NULL;
char devNum, sessionBegin = 0, mTmodemTickCount=0,
	mPacketSize = PACKET_SIZE;
static char num_eot = 0, num_ca=0;
static unsigned int pReceived = 0, continuity = 0, tmp0, tmp1;	

void reset_tmodem_status(void)
{
	tmp0 = 0;
	tmp1 = 0;
	num_ca = 0;
	num_eot = 0;
	sessionBegin =0;
	continuity = 0;
	pReceived = 0;
	//flashdestination = APPLICATION_ADDRESS;	
	printf("%s, mTmodemTickCount=%d\r\n", __func__, mTmodemTickCount);
}

static unsigned int continuity_add(char sessionBegin)
{
	if( sessionBegin ) continuity++;
	return continuity;
}

static void cac_ca_num(char *num_ca)
{
	if( *num_ca == 0 ) 
	{ 
		*num_ca = 1, 
		tmp0 = continuity;
		printf("%s: CA!\r\n", __func__);
	} 
	
	if((continuity - tmp0) == 1) 
	{
		if(*num_ca == 1) 
			*num_ca = 2;
		printf("%s: CA CA!\r\n", __func__);
	} 
	else if(sessionBegin) 
	{ 
		*num_ca = 1;
		tmp0 = continuity;
		printf("%s: CA X X X CA !\r\n", __func__);
	} 
	else 
	{
		*num_ca = 0;
	}	
}

static int cac_eot_num(char *num_eot)
{
	if(*num_eot == 0) 
	{ 
		*num_eot = 1, 
		tmp1 = continuity;
		//printf("EOT!\r\n");
		return UD3;
	} 
	
	if((continuity - tmp1) == 1) 
	{
		if(*num_eot == 1) 
			*num_eot = 2;
		//printf("EOT EOT!\r\n");
		pReceived = 0;
		return UD4;						
	} 
	else if( sessionBegin ) 
	{ 
		*num_eot = 1;
		tmp1 = continuity;
		//printf("EOT X X X EOT!\r\n");
		return UD3;
	} 
	else 
	{
		*num_eot = 0;
		return E3;
	}	
}

//AA BB 2	   msgid(0x8007)  type EOT EOT CRC1 CRC2   
void report_ymodem_packet_two( char C, char C0 )
{
	int len;
	char data[2];
	char command[ 2 + N_LEN + N_MSGID + N_TYPE + 2 + N_CRC ];
	
	data[0] = C;
	data[1] = C0;	
	len = compose_protocol_command( 2, 0x8007, data, command );
	add_command_to_stream( command, len, WAIT_BLOCK );
}

//AA BB 1	   msgid(0x8007)  type EOT  CRC1 CRC2   
void report_ymodem_packet_one( char C )
{
	int  len;
	char data[1];	
	char command[ 2 + N_LEN + N_MSGID + N_TYPE + 1 + N_CRC ];	
	
	data[0] = C;
	len = compose_protocol_command( 1, 0x8007, data, command );
	add_command_to_stream( command, len, WAIT_BLOCK );
}

//AA BB 131    0x05 01 FE Data[128] CRC1 CRC2 
//AA BB 2	   0x05 EOT CRC1 CRC2  
//AA BB 3	   0x05 UPDATE	  devNum CRC1 CRC2	

//AA BB 130    msgid(0x8007)  type D0~Dn(01 FE Data[128])	  length=sizeof( D0 ~ Dn )
//AA BB 2	   msgid(0x8007)  type UPDATE  devNum CRC1 CRC2	
//AA BB 1	   msgid(0x8007)  type EOT  CRC1 CRC2   

static int parse_packet ( const char *rpacket, int len, int *plen )
{
  unsigned char c;	
  unsigned short psize;

  *plen = 0;
  if( len == mPacketSize + 2 ) 
  {
  	c = SOH;
  }
  else if( ( len == CMD_PACKET_LEN ) && ( rpacket[0] == EOT ) )
  {
  	c = EOT;
  }
  else if( ( len == CMD_UPDATE_PACKET_LEN ) && ( rpacket[0] == UPDATE ) ) 
  {
  	c = UPDATE;	
  }
  else if( ( len == CMD_PACKET_LEN ) && ( rpacket[0] == CA ) ) 
  {
  	c = CA;	
  }
  else if( ( len == DATA_PACKET_LEN1 ) ) 
  {
  	c = STX; 
  }
  
  switch ( c )
  {
    case SOH:
      	psize = mPacketSize;
     	break;
    case STX:
      	psize = PACKET_1K_SIZE;
      	break;
    case EOT:
		continuity_add( sessionBegin );
      	return 0;
	case CA:
		*plen = -1;
		continuity_add( sessionBegin );
		return 0;
	case UPDATE:
		printf("%s: UPDATE\r\n", __func__);
		continuity_add( sessionBegin );
		return 2;
		
    default:
		printf("%s:error cmd, data[2]=%d\r\n", __func__, rpacket[0]);
      	return -1;
  }
  
  if ( rpacket[ SEQNO_INDEX ] != 
  	( ( rpacket[ SEQNO_COMP_INDEX ] ^ 0xff ) & 0xff ) )
  {
  	return -1;
  }
  
  *plen = psize;
  continuity_add( sessionBegin );
	
  return 0;
}

int parse_ymodem_command(const char* rpacket, int len)
{
	static unsigned int mRecvBytes=0;
	int sessionDone = 0, plen, ret;

	switch ( parse_packet( rpacket, len, &plen ) )	
	{
		case 0:
		 	
		  switch ( plen )
		  {
			case -1:
				cac_ca_num( &num_ca );
				if(( num_ca ) == 2 || ( num_ca == 0 ))
					reset_tmodem_status(); 
				return UDS;
				
			case 0:	
				return cac_eot_num( &num_eot );
				
			default:				
				if ( ( rpacket[ SEQNO_INDEX ] & 0xff ) != ( pReceived & 0xff ) ) 
				{
					printf("%s: (%02x)!=(%02x) error!\r\n", __func__, 
							( rpacket[ SEQNO_INDEX ] & 0xff ), ( pReceived & 0xff ) );
					
					if( ( pReceived & 0xff ) - 
							( rpacket[ SEQNO_INDEX ] & 0xff ) == 1 ) 
					{
						return E3;
					} 
					else 
					{
						return E6;
					}
				} 
				else 
				{	 	
					if ( pReceived == 0 )
					{
						if ( rpacket[ PACKET_HEADER ] != 0 ) 
						{
							if( ( rpacket[ SEQNO_INDEX ] & 0xff ) != 0 ) 
							{
								return E6;
							}

							ret = handler->packet_handler( devNum, pReceived, rpacket, plen );
							if( ret < 0)
							{
								return ret;
							}
							mTmodemTickCount = 0;
							sessionBegin = 1;
							pReceived ++;
							return UD1;
						}     
					
						else if( sessionBegin )
						{
							sessionDone = 1;
							mTmodemTickCount = 0;
							break;
						} 
						else 
						{
							return E2;
						}
					}
					else if( sessionBegin ) 
					{
						ret = handler->packet_handler( devNum, pReceived, rpacket, plen);
						if( ret < 0)
						{
							return ret;
						}						
						mTmodemTickCount = 0;
						mRecvBytes += plen;
						pReceived++;
						return UD2;
					}
					else 
					{
						printf("sessionBegin = 0, but android send rom data...\r\n");
						return E2;
					}
			 }
			}
			break;

		case 2:
				devNum = rpacket[ 1 ];
				printf("%s: want to update dev num = %d\r\n", __func__, devNum );
				
				handler = get_ymodem_handlers(devNum);
				if( handler ) 
				{
					return UD0;
				} 
				else 
				{
					return E7; 
				}
		case -1:
				printf("%s: ERROR E0\r\n", __func__);
				return E0;
				
		default:
			break;
	}

	if ( sessionDone != 0 )
	{		
		if( 0/*timer*/ )
		{
				if( 1/*unregister_timer2( timer ) == 0*/ )
				{
					//timer = NULL;
				}
				else
				{
					printf("%s: fail to delete timer\r\n", __func__);
				}
		}
		reset_tmodem_status();
		return handler->finish_handler( devNum );
	}

	return E3;
}

static void process_ymodem_result( int result )
{
	//int rand;
	
	switch(result)
	{
		case  UD0: 
			//rand = rand_count % 5;
			//mPacketSize = 64 + rand * 16;	
			printf("%s: UD0 mPacketSize=%d\r\n",__func__, mPacketSize);
			if( mPacketSize > 128 || mPacketSize < 64 )
			{
				mPacketSize = 128;
			}
			report_ymodem_packet_two( UPDATE, mPacketSize );     
			report_ymodem_packet_two( ACK, ( pReceived & 0xff ) ); 
			report_ymodem_packet_one( CRC16 ); 
			break;
		
		case  UD1: 
			report_ymodem_packet_two( ACK, ( pReceived & 0xff ) ); 
			report_ymodem_packet_one( CRC16 ); 
			break;
		case  UD4: 
			report_ymodem_packet_one( EOT_ACK ); 
		    report_ymodem_packet_one( CRC16 ); 
			break;
		case  UDS: break;//CA CA
		case  UD2: 
			report_ymodem_packet_two( ACK, ( pReceived & 0xff ) ); 
			break;
		case  UD3: 
			report_ymodem_packet_one( EOT_ACK ); 
			report_ymodem_packet_one( NAK ); 
			break;
		case  UD5: 
			report_ymodem_packet_one( LAST_ACK ); 
			report_ymodem_packet_one( UPDATE_DONE ); 
			printf("%s: UD5 devNum=%d\r\n", __func__, devNum);
			printf("Ymodem done!!!!!!\r\n");
		break;
		
		case  E0:  
			printf("E0\r\n");
		case  E1:  
			printf("E1\r\n");
		case  E3:  
			report_ymodem_packet_one(ACK); printf("E3\r\n");					 
			if( handler )
			{
				handler->error_handler( devNum, E3 );
			}
			break;
		case  E2:  
			report_ymodem_packet_one( ERR_TRANSMISS_START ); printf("E2\r\n"); 
			if( handler )
			{
				handler->error_handler( devNum, E2 );
			}
			break;
			
		case  E4:  
			report_ymodem_packet_one(ERR_SIZE_EXT); printf("E4\r\n");
			if( handler )
			{
				handler->error_handler( devNum, E4 );
			}
			break;
			
		case  E5:  
			report_ymodem_packet_one(ERR_FLASH_RW); printf("E5\r\n"); 
			if( handler )
			{
				handler->error_handler( devNum, E5 );
			}
			break;
			
		case  E6:  
			report_ymodem_packet_one(ERR_PACKET_INDEX); printf("E6\r\n"); 
			if( handler )
			{
				handler->error_handler( devNum, E6 );
			}
			break;
			
		case  E7:  
			report_ymodem_packet_one(ERR_UPDATE_REQUEST); printf("E7\r\n"); 
			if( handler )
			{
				handler->error_handler( devNum, E7 );
			}
			break;
		
		default: 
			if( handler )
			{
				handler->error_handler( devNum, 0 );
			}
			break;	
	}
}

int handle_ymodem_command( char* rpacket, int len )
{
	int result;
	
	result = parse_ymodem_command( rpacket, len );
	process_ymodem_result( result );
	return 0;
}



