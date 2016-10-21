#include <stdio.h>
#include <string.h>
#include "md5.h"
#include "ff.h"
#include "uartprotocol.h"
#include "ymodem.h"
//#include "rtc.h"
//#include "stmflash.h"
//#include "stm32f4xx_flash.h"

/**********************************************************************
		0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02
本协议由Ymodem修改而来，适配我们android与MCU通讯时的协议，
所以命名为Tmodem, 可先了解Ymodem再看此协议。

经过测试，此Tmodem协议传输文件的稳定性很高，目前还没有发现哪种情况会导致                 
文件丢失而误以为传输成功，要么传输成功，要么传输失败。                                   

PS: mPacketSize
PR: pReceived & 0xff

1、	android 下发更新rom请求： 更新由android系统触发                                      
	  AA BB 3  0x05 UPDATE  devNum CRC1 CRC2	
	  
	  				ack                                    
					UPDATE PS
					ACK PR  pReceived=0 packetNum = 1 
					2、 mcu上报C命令                       
					C     
					
3、 anroid 下发rom的文件名，以及文件大小                              
		AA BB 131  0x05 00 FF foo.c 102400 CRC1 CRC2
		
					ack                 
					ACK PR  pReceived=1  packetNum = 2
					4、 mcu上报C命令                       
					C             
					
5、 android 连续下发rom文件数据，最后一包不足128bytes时填充0          
		AA BB 131  0x05 01 FE Data[128] CRC1 CRC2     
		
					ack
					ACK PR   pReceived=2  packetNum = 3
					
		AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		
		
					ack
					ACK PR   pReceived=3  packetNum = 4
					
		.....                                                                                
					ack
                                  ACK PR   pReceived=4  packetNum = 5
		.....                                                                                
					ack
					ACK PR   pReceived=5  packetNum = 6
																									                                       
6、 android 连续 下发两条end of transmission（EOT）包									                   
		AA BB 2  0x05 EOT CRC1 CRC2     
		
					ack                                    
					EOT_ACK    packetNum = 7               
					7、 MCU收到第一条EOT包回复NAK命令包		 
					NAK                          packetTotal = 4           
					
8、 android下发第二条EOT包																						                   
		AA BB 2  0x05 EOT CRC1 CRC2    
		
					ack                                    
					EOT_ACK    packetNum = 8               
					9、 MCU回复C命令包										 
					C                                      
					
10、android下发rom更新结束包，MCU收到 后更新成功！										                   
		AA BB 131  cmd 00 FF NULL[128] CRC1 CRC2            
		
					ack		
					LAST_ACK    packetNum = 9              
					UPDATE_DONE                            
					
***********************************************************************/

static unsigned char *key = NULL;
extern char mMcuJumpAppPending;
static unsigned char aes_key[ 32 ];
unsigned char tmodem_md5[ LEN_MD5 ];
unsigned int romSize = 0;
char devNum, sessionBegin = 0, mTmodemTickCount=0,
	mPacketSize = PACKET_SIZE;
static char FileName[FILE_NAME_LENGTH], num_eot = 0, num_ca=0;
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
	char cmd[ 2 + N_LEN + N_MSGID + N_TYPE + 2 + N_CRC ];
	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	
	*( LEN_Type* )  &cmd[ 2 ] = 2;
	*( MSGID_Type* )&cmd[ 2 + N_LEN ] = 0x8007;
	*( TYPE_Type* ) &cmd[ 2 + N_LEN + N_MSGID ] = TYPE_CMD;

	cmd[ 2 + N_LEN + N_MSGID + N_TYPE ] = C;
	cmd[ 2 + N_LEN + N_MSGID + N_TYPE + 1 ] = C0;

	*( CRC_Type* )   &cmd[ 2 + N_LEN + N_MSGID + N_TYPE + 2 ] = 
		CheckSum( &cmd[2], N_LEN + N_MSGID + N_TYPE + 2 );

	put_buffer_to_stream( cmd,  2 + N_LEN + N_MSGID + N_TYPE + 2 + N_CRC );
}

//AA BB 1	   msgid(0x8007)  type EOT  CRC1 CRC2   
void report_ymodem_packet_one( char C )
{
	char cmd[ 2 + N_LEN + N_MSGID + N_TYPE + 1 + N_CRC ];
	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	
	*( LEN_Type* )&cmd[ 2 ] = 2;
	*( MSGID_Type* )&cmd[ 2 + N_LEN ] = 0x8007;
	*( TYPE_Type* )&cmd[ 2 + N_LEN + N_MSGID ] = TYPE_CMD;

	cmd[ 2 + N_LEN + N_MSGID + N_TYPE ] = C;

	*( CRC_Type* )&cmd[ 2 + N_LEN + N_MSGID + N_TYPE + 1 ] = 
		CheckSum( &cmd[2], N_LEN + N_MSGID + N_TYPE + 1 );
	
	put_buffer_to_stream( cmd,  2 + N_LEN + N_MSGID + N_TYPE + 1 + N_CRC );
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
	int i, sessionDone = 0, plen;
	unsigned int size = 0, ramsource = 0, preFlashdestination;
	unsigned char *pbuf, *pfile, pfsize[ FILE_SIZE_LENGTH ];	

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

						/* Filename packet has valid data */
						pfile = ( unsigned char * )( rpacket + PACKET_HEADER );
						for ( i = 0; ( *pfile != 0 ) && ( i < FILE_NAME_LENGTH ); ) 
						{
							FileName[ i++ ] = *pfile++; 
						}
						
						FileName[i++] = '\0';
						
						for ( i = 0, pfile++; ( *pfile != '\0' ) && ( i < FILE_SIZE_LENGTH ); ) 
						{
							pfsize[ i++ ] = *pfile++;
						}
						pfsize[ i++ ] = '\0';
						//Str2Int( pfsize, &size );
						size = atoi( pfsize );
						//size = size - LEN_MD5;
						mRecvBytes = 0;
						printf("%s: FileName(%s) size(%d)\r\n", __func__, FileName, size );

						/*
						printf("%s: file size = %d, USE_FLASH_SIZE=%d\r\n",
									__func__, size, USER_FLASH_SIZE);
						if ( size > ( USER_FLASH_SIZE ) ) 
						{
							printf("%s: file size > flash size\r\n", __func__);
							return E4;
						}
						
						printf("aes_key:\r\n[");
						
						for (i = 0, pfile++; i < sizeof( aes_key )/sizeof( aes_key[ 0 ] ); i++) 
						{
							aes_key[ i ] = *pfile++;
							printf("%02x,", aes_key[ i ] );
						}								
						printf("]\r\n");
						
						if( key ) 
						{ 
							aes_destory_key( key ); 
							key = NULL;
						}
						key = aes_create_key( aes_key, sizeof( aes_key )/sizeof( aes_key[0] ));
						
						if( !key ) 
							return E2;
						*/
						mTmodemTickCount = 0;
						romSize = size;
						sessionBegin = 1;
						pReceived ++;

						/*
						if( 0X1234 != RTC_ReadBackupRegister( RTC_BKP_DR3 ) && 
								( devNum == MCU_NUM || devNum == SCU_NUM ) ) 
						{
							printf("%s: start Erase Sector!\r\n", __func__);
							FLASH_If_Erase_Sector( flashdestination );							
						}
						
						if( devNum == BOOTLOADER_NUM )
						{
							printf("%s: start Erase Sector\r\n", __func__);
							FLASH_If_Erase_Sector( flashdestination ); 
						}
						else 
						{
							printf("%s: reset RTC_BKP_DR3 = 0!\r\n", __func__);
							RTC_WriteBackupRegister( RTC_BKP_DR3, 0x0000 );
						}
						
						targetSector = GetSector( flashdestination );
						
						if( timer == NULL )
						{
							//timer = register_timer2("tmodem_timer", TIMER2SECOND, tmodem_timer, REPEAT, &sessionBegin);
						}
						*/
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
			
				/* Data packet */
				else if( sessionBegin ) 
				{
					if( pReceived == 1 ) 
					{
						//printf("%s: flash save addr:%x\r\n", __func__, flashdestination);
					}
					/*
					for( i=0; i< plen/ AES_DECODE_LEN; i++ ) 
					{
						pbuf = aes_decode_packet( key, 
							( unsigned char* )( rpacket + PACKET_HEADER + AES_DECODE_LEN * i ), 
							AES_DECODE_LEN );
						
						ramsource = ( unsigned int ) pbuf;				
						preFlashdestination = flashdestination;	
						flashdestination = STMFLASH_Write( flashdestination, 
							(unsigned int *) ramsource, AES_DECODE_LEN / 4 );
						 
						if( flashdestination == preFlashdestination + AES_DECODE_LEN ) 
						{
							if( targetSector != GetSector( flashdestination ) )
							{
								printf("%s: rom is too big! Erase one more sector!\r\n", __func__);
								targetSector = GetSector( flashdestination );
								FLASH_If_Erase_Sector( flashdestination ); 
							}
							mTmodemTickCount = 0;				
						} 
						else 
						{
							printf("%s: STMFLASH_Write error\r\n", __func__);
							printf("%s: des=0x%x, preDes=0x%x, packet_length=%d\r\n", 
								__func__, flashdestination,
										preFlashdestination, plen);
							return E5;
						}
					} 
					*/
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
		
			if( 1/*devNum == MCU_NUM || devNum == SCU_NUM*/ ) 
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
		unsigned int src;
		
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
		
		//read_md5_from_flash( devNum, romSize, tmodem_md5 );
		
		if( 1/*( devNum==MCU_NUM ) || ( devNum==SCU_NUM )*/) 
		{
			//src = APPLICATION_ADDRESS;
		}
		else
		{
			//src = BOOTLOADER_ADDRESS;
		}
		
		printf("%s: Download Finish.\r\n", __func__);
		reset_tmodem_status();
		
		if( 1/*compare_flashdata_md5( src, romSize, tmodem_md5 ) == 0*/ )
		{
			printf("%s: Check MD5 Ok!\r\n", __func__);
			return UD5;
		}
		else
		{
			printf("%s: Check MD5 Fail!\r\n", __func__);
			return E5;
		}
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
/*		
			if( MCU_NUM == devNum ) 
			{
				//mAndroidShutDownPending = 1;
				//mMcuJumpAppPending = 1; 
				printf("jump mcu update..\r\n");
			} 
			else if( SCU_NUM == devNum ) 
			{
			 	//handle_scu_rom_update();
			 	printf("jump scu update..\r\n");
			} 
			else if( BOOTLOADER_NUM == devNum ) 
			{
			 	//handle_booloader_rom_update();
			 	printf("jump booloader update..\r\n");
			}
*/
		break;
		
		case  E0:  
			printf("E0\r\n");
		case  E1:  
			printf("E1\r\n");
		case  E3:  
			report_ymodem_packet_one(ACK); printf("E3\r\n");					 
			//if( key ) { aes_destory_key( key ); key = NULL; }
			break;
		case  E2:  
			report_ymodem_packet_one( ERR_TRANSMISS_START ); printf("E2\r\n"); 
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;
			
		case  E4:  
			report_ymodem_packet_one(ERR_SIZE_EXT); printf("E4\r\n");
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;
			
		case  E5:  
			report_ymodem_packet_one(ERR_FLASH_RW); printf("E5\r\n"); 
			//check_if_need_to_erase(); 
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;
			
		case  E6:  
			report_ymodem_packet_one(ERR_PACKET_INDEX); printf("E6\r\n"); 
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;
			
		case  E7:  
			report_ymodem_packet_one(ERR_UPDATE_REQUEST); printf("E7\r\n"); 
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;
		
		default:   
			//if( key ) {aes_destory_key( key ); key = NULL;} 
			break;	
	}
}

int handle_ymodem_command( char* rpacket, int len )
{
	int result;
	
	result = parse_ymodem_command( rpacket, len );
	process_ymodem_result( result );
}


