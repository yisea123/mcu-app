#include "handlers.h"
#include "romhandler.h"	
#include "aes.h"
#include "ymodem.h"
#include "stmflash.h"
#include "md5.h"

static void read_md5_from_flash(char devNum, unsigned int romSize, unsigned char md5[])
{
	u32 src;
	int mRead = 0;	
	unsigned char data[4], *p;
	
	p = md5;
	
	if((devNum==MCU_NUM) || (devNum==SCU_NUM)) 
	{
		src = APPLICATION_ADDRESS + romSize;
	} 
	else 
	{
		src = BOOTLOADER_ADDRESS + romSize;
	}	

	printf("%s: src=%x, binSize=%d\r\n", __func__, src, romSize);
	
	while(mRead < LEN_MD5) 
	{
		STMFLASH_Read(src, (u32 *)data, 1);
		src += 4;
		mRead += 4;
		memcpy(p, data, 4);
		p += 4;
	}

	printf("%s:[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]\r\n", __func__,
			md5[0], md5[1],md5[2], md5[3],md5[4], md5[5],md5[6], md5[7],md5[8], md5[9],
			md5[10], md5[11],md5[12], md5[13],md5[14], md5[15]); 	
}

static char FileName[FILE_NAME_LENGTH];
static unsigned char aes_key[ 32 ];
static unsigned char *key = NULL;
static unsigned int romSize = 0, flashdestination = APPLICATION_ADDRESS;
static unsigned int targetSector = 0;	
static unsigned char tmodem_md5[ LEN_MD5 ];

int rom_packet_handler(  int devNum, unsigned int index, const char *rpacket, int plen )
{
	static unsigned int mRecvBytes=0;
	int i, sessionDone = 0;
	unsigned int size = 0, ramsource = 0, preFlashdestination;
	unsigned char *pbuf, *pfile, pfsize[ FILE_SIZE_LENGTH ];	

	if( devNum != 5)
		return -1;

	if( index == 0 )
	{
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
		size = size - LEN_MD5;
		mRecvBytes = 0;
		printf("%s: FileName(%s) size(%d)\r\n", __func__, FileName, size );

		printf("%s: file size = %d, USE_FLASH_SIZE=%d\r\n", __func__, size, /*USER_FLASH_SIZE*/512*1024);
		if ( size > ( /*USER_FLASH_SIZE*/512*1024 ) ) 
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
			aes_destory_key( &key ); 
			key = NULL;
		}
		key = aes_create_key( aes_key, sizeof( aes_key )/sizeof( aes_key[0] ));
		
		if( !key ) 
			return E2;
		
		romSize = size;

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
		
		targetSector = STMFLASH_GetFlashSector( flashdestination );		

	}
	else
	{
		if( index==1 )
		{
			printf("%s: flash save addr:%x\r\n", __func__, flashdestination);
		}
		for( i = 0; i< plen/AES_DECODE_LEN; i++ ) 
		{
			pbuf = aes_decode_packet(key, (unsigned char*)(rpacket+PACKET_HEADER+AES_DECODE_LEN*i), AES_DECODE_LEN);
			ramsource = ( unsigned int ) pbuf;				
			preFlashdestination = flashdestination; 
			flashdestination = STMFLASH_Write(flashdestination, (unsigned int *) ramsource, AES_DECODE_LEN/4);
			
			if(flashdestination == preFlashdestination + AES_DECODE_LEN) 
			{
				if(targetSector != STMFLASH_GetFlashSector(flashdestination))
				{
					printf("%s: rom is too big! Erase one more sector!\r\n", __func__);
					targetSector = STMFLASH_GetFlashSector(flashdestination);
					FLASH_If_Erase_Sector(flashdestination); 
				}
			} 
			else 
			{
				printf("%s: STMFLASH_Write error\r\n", __func__);
				printf("%s: des=0x%x, preDes=0x%x, packet_length=%d\r\n", __func__, 
					flashdestination, preFlashdestination, plen);
				return E5;			
			}
		} 
		mRecvBytes += plen;

	}
	
	return 0;
}

int rom_error_handler( int devNum, int error )
{
	if( error == E5 )
	{
		//check_if_need_to_erase(); 
	}
	if( key ) 
	{ 
		aes_destory_key( &key );
	}
	printf("%s\r\n", __func__);
	return 0;
}

int rom_finish_handler( int devNum )
{
	unsigned int src;

	read_md5_from_flash( devNum, romSize, tmodem_md5 );
	
	if( ( devNum==MCU_NUM ) || ( devNum==SCU_NUM )) 
	{
		src = APPLICATION_ADDRESS;
	}
	else
	{
		src = BOOTLOADER_ADDRESS;
	}
	
	printf("%s: Download Finish.\r\n", __func__);
	if( compare_flashdata_md5( src, romSize, tmodem_md5 ) == 0 )
	{
		printf("%s: Check MD5 Ok! start reboot...\r\n", __func__);
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
		return UD5;
	}
	else
	{
		printf("%s: Check MD5 Fail!\r\n", __func__);
		return E5;
	}
}

void register_rom_handlers( void )
{
	if( register_ymodem_handlers( 5, rom_packet_handler, rom_error_handler,
					rom_finish_handler	) == 0 )
	{
		printf("%s: init ok!\r\n", __func__);
	}
	else
	{
		printf("%s: init fail!\r\n", __func__);		
	}
}

int amr_packet_handler(  int devNum, unsigned int index, const char *packet, int plen  )
{
	return 0;
}

int amr_error_handler( int devNum, int error )
{
	return 0;
}

int amr_finish_handler( int devNum )
{
	return 0;
}



