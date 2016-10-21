/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2013        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control module to the FatFs module with a defined API.        */
/*-----------------------------------------------------------------------*/
/* FatFs lower layer API */

#include "diskio.h"		
#include "sdio_sdcard.h"
#include "w25qxx.h"
#include "rtc.h"
#include "FreeRTOS.h"
#include "task.h"
#include <stdio.h>
#include <string.h>

#define SD_CARD	 0 
#define EX_FLASH 1
#define DEV_RAM	 2
	  
//前12M字节给fatfs用,12M字节后,用于存放字库,字库占用3.09M.	
//剩余部分,给开发者自己用	 
#if(BOARD_NUM == 1)
static u16 FLASH_SECTOR_COUNT = 2048 * 2;
#elif(BOARD_NUM == 2)
static u16 FLASH_SECTOR_COUNT = 2048 * 2 * 8;
#else
static u16 FLASH_SECTOR_COUNT = 2048 * 2;
#endif
static char xcInit = 0;
static u16	RAM_DISK_SECTOR_COUNT = 60;
static char pcRamdisk[ 10 * 1024 ];

void vPrintSdDisk( unsigned int sector )
{
	char *p;
	int i = 0;
	unsigned char buff[512];
	
	memset( buff, 0, sizeof( buff ) );
	if( 0 == SD_ReadDisk( buff, sector, 1 ) )
	{
		printf("\r\nStart SD Card sector(%d)\r\n", sector );
		printf("	  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n");
		printf("\r\n");
		for( i = 0 ; i < 512/16 ; i++ )
		{
			p = ( char * ) buff + i * 16;
			if( ( i % 2 ) == 0 )
			{
				printf("%02X:	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
					i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
					 p[10], p[11], p[12], p[13], p[14], p[15]);
			}
			else
			{
				printf("	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
					p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
					 p[10], p[11], p[12], p[13], p[14], p[15]);
			}
		}	
	}
	else
	{
		printf("%s: read sd card fail!\r\n", __func__);
	}
}

void vPrintFlashDisk( unsigned int sector )
{
	char *p;
	int i = 0;
	unsigned char buff[512];
	memset( buff, 0, sizeof( buff ) );
	
	printf("\r\nStart Flash sector(%d)\r\n", sector );
	printf("	  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n");
	printf("\r\n");
	W25QXX_Read( buff, sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE );
	
	for( i = 0 ; i < 512/16 ; i++ )
	{
		p = ( char * ) buff + i * 16;
		if( ( i % 2 ) == 0 )
		{
			printf("%02X:	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
				i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
				 p[10], p[11], p[12], p[13], p[14], p[15]);
		}
		else
		{
			printf("	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
				 p[10], p[11], p[12], p[13], p[14], p[15]);
		}
	}	
}

void vPrintRamdDisk( unsigned int sector )
{
	char *p;
	int i = 0;

	if( RAM_DISK_SECTOR_COUNT <= sector )
	{
		printf("%s: sector error!\r\n", __func__);
		return ;
	}
	
	printf("\r\nStart Addr: %p, sector(%d)\r\n", &( pcRamdisk[ sector * 512 ] ), sector );
	printf("	  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f\r\n");
	printf("\r\n");
	for( i = 0 ; i < 512/16 ; i++ )
	{
		p = &( pcRamdisk[ sector * 512 + i * 16 ] );
		if( ( i % 2 ) == 0 )
		{
			printf("%02X:	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
				i, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
				 p[10], p[11], p[12], p[13], p[14], p[15]);
		}
		else
		{
			printf("	  %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\r\n", 
				p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], p[9],
				 p[10], p[11], p[12], p[13], p[14], p[15]);
		}
	}
}

static void xMemncpy ( void* dst, const void* src, unsigned int cnt ) 
{
	char *d = ( char* ) dst;
	const char *s = ( const char* ) src;

	while( cnt-- )
	{
		*d++ = *s++;
	}
}

static void Ramdisk_Read( u8* pBuffer, u32 ReadAddr, u16 NumByteToRead )   
{ 
	if( ( ReadAddr + NumByteToRead ) <= sizeof( pcRamdisk ))
	{
		xMemncpy(pBuffer, pcRamdisk + ReadAddr, NumByteToRead);
	}
	else
	{
		printf("%s: error addr !\r\n", __func__);
	}
}

static void Ramdisk_Write( u8* pBuffer, u32 ReadAddr, u16 NumByteToRead )   
{ 
	if( ( ReadAddr + NumByteToRead ) <= sizeof( pcRamdisk ) )
	{
		xMemncpy( pcRamdisk + ReadAddr, pBuffer, NumByteToRead );
	}
	else
	{
		printf("%s: error addr !\r\n", __func__);
	}
}

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber (0..) */
)
{
	u8 res = 0;	
	
	switch( pdrv )
	{
		case SD_CARD: 
			res=SD_Init(); 
  			break;
			
		case EX_FLASH: 
			W25QXX_Init();
#if(BOARD_NUM == 1)			
			FLASH_SECTOR_COUNT = 2048 * 2;
#elif(BOARD_NUM == 2)
			FLASH_SECTOR_COUNT = 2048 * 2 * 8;
#endif
 			break;
			
		case DEV_RAM:
			if( xcInit == 0 )
			{
				char q[5], *p;
				p = q;
				memcpy(p, "12345", 5);
				printf("1 TEST: %p, %p, %p, %p, %p\r\n", p, p+1, p+2
					, p+3, p+4);
				printf("2 TEST: %p, %p, %p, %p, %p\r\n", p, ++p, ++p
					, ++p, ++p);
				//printf 的运行顺序是从左向右开始运行
				//应该是编译器的问题，在不同芯片上
				//运行的结果不一样, 以后再也不写这样的
				//代码了。
				p = q;
				printf("3 TEST: %p\r\n", p);
				p++;
				printf("3 TEST: %p\r\n", p);
				p++;
				printf("3 TEST: %p\r\n", p);				
				p++;
				printf("3 TEST: %p\r\n", p);				
				p++;
				printf("3 TEST: %p\r\n", p);

				p = q;
				printf("4 TEST: %02x %02x %02x %02x %02x\r\n", p[0], p[1], p[2]
					, p[3], p[4]);		
				printf("5 TEST: %p, %p, %p, %p, %p\r\n", &p[0], &p[1], &p[2]
					, &p[3], &p[4]);					
				xcInit = 1;
				memset( pcRamdisk, 0, sizeof( pcRamdisk ) );
				printf("%s: clean ram disk!!!!\r\n", __func__);
			}
			RAM_DISK_SECTOR_COUNT = sizeof( pcRamdisk ) / RAM_SECTOR_SIZE;
			printf("%s: RAM_DISK_SECTOR_COUNT(%d)\r\n", __func__, RAM_DISK_SECTOR_COUNT);
			break;
			
		default:
			res=1;
			break;
	}		 
	if( res )
	{
		return  STA_NOINIT;
	}
	else 
	{
		return 0;
	}
}  

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber (0..) */
)
{ 
	return 0;
} 

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	u8 res=0; 
	
    if ( !count )
	{
		return RES_PARERR;
	}
	switch( pdrv )
	{
		case SD_CARD:
			res = SD_ReadDisk( buff, sector, count );	 
			while( res )
			{
				SD_Init();
				res = SD_ReadDisk( buff, sector, count );	
			}
			break;
		case EX_FLASH: 
			for( ; count > 0; count-- )
			{
				W25QXX_Read( buff, sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE );
				sector++;
				buff += FLASH_SECTOR_SIZE;
			}
			res = 0;
			break;
		case DEV_RAM:
			for( ; count > 0; count-- )
			{
				Ramdisk_Read( buff, sector * RAM_SECTOR_SIZE, RAM_SECTOR_SIZE );
				sector++;
				buff += RAM_SECTOR_SIZE;
			}
			res = 0;
			break;
			
		default:
			res = 1; 
			break;
	}

    if( res == 0x00 ) 
	{
		return RES_OK;
    }
    else 
	{
		return RES_ERROR;
    }
}

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber (0..) */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	u8 res=0;  
	
    if ( !count )
	{
		return RES_PARERR;	
    }
	switch( pdrv )
	{
		case SD_CARD:
			res = SD_WriteDisk( (u8*) buff, sector, count );
			while( res )
			{
				SD_Init();
				res = SD_WriteDisk( (u8*) buff, sector, count );	
			}
			break;
			
		case EX_FLASH:
			for( ; count>0; count-- )
			{										    
				W25QXX_Write( (u8*) buff, sector * FLASH_SECTOR_SIZE, FLASH_SECTOR_SIZE );
				sector++;
				buff += FLASH_SECTOR_SIZE;
			}
			res=0;
			break;

		case DEV_RAM:
			for( ; count > 0; count-- )
			{
				Ramdisk_Write( (u8*) buff, sector * RAM_SECTOR_SIZE, RAM_SECTOR_SIZE );
				sector++;
				buff += RAM_SECTOR_SIZE;
			}
			res = 0;
			break;	
			
		default:
			res = 1; 
			break;
	}

    if( res == 0x00 )
	{
		return RES_OK;	 
    }
	else 
	{
		return RES_ERROR;	
	}
}
#endif

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res;						  			     
	if( pdrv == SD_CARD ) 
	{
	    switch( cmd )
	    {
		    case CTRL_SYNC:
				res = RES_OK; 
		        break;	
				
		    case GET_SECTOR_SIZE:
				*(DWORD*) buff = 512; 
		        res = RES_OK;
		        break;	 
				
		    case GET_BLOCK_SIZE:
				*(WORD*) buff = SDCardInfo.CardBlockSize;
		        res = RES_OK;
		        break;	 
				
		    case GET_SECTOR_COUNT:
		        *(DWORD*) buff = SDCardInfo.CardCapacity/512;
		        res = RES_OK;
		        break;
				
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}
	else if( pdrv == EX_FLASH )
	{
	    switch( cmd )
	    {
		    case CTRL_SYNC:
				res = RES_OK; 
		        break;	
				
		    case GET_SECTOR_SIZE:
		        *(WORD*) buff = FLASH_SECTOR_SIZE;
		        res = RES_OK;
		        break;	 
				
		    case GET_BLOCK_SIZE:
		        *(WORD*) buff = FLASH_BLOCK_SIZE;
		        res = RES_OK;
		        break;	 
				
		    case GET_SECTOR_COUNT:
		        *(DWORD*) buff = FLASH_SECTOR_COUNT;
		        res = RES_OK;
		        break;
				
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}
	else if( pdrv == DEV_RAM )
	{
	    switch( cmd )
	    {
		    case CTRL_SYNC:
				res = RES_OK; 
		        break;	
				
		    case GET_SECTOR_SIZE:
		        *(WORD*) buff = RAM_SECTOR_SIZE;
		        res = RES_OK;
		        break;	 
				
		    case GET_BLOCK_SIZE:
		        *(WORD*) buff = RAM_BLOCK_SIZE;
		        res = RES_OK;
		        break;	 
				
		    case GET_SECTOR_COUNT:
		        *(DWORD*) buff = RAM_DISK_SECTOR_COUNT;
		        res = RES_OK;
		        break;
				
		    default:
		        res = RES_PARERR;
		        break;
	    }
	}	
	else
	{
		res = RES_ERROR;
	}
    return res;
}
#endif

/**************************************************
	31-25: Year(0-127 org.1980), 
	24-21: Month(1-12), 
	20-16: Day(1-31)                                                                                                                                                                                                                                         
	15-11: Hour(0-23), 
	10-5: Minute(0-59),
	4-0: Second(0-29 *2) 
**************************************************/                                                                                                                                                                                                                                                
DWORD get_fattime ( void )
{	
	unsigned char hour,min,sec,ampm;
	unsigned char year,month,date,week;

	RTC_Get_Time( &hour, &min, &sec, &ampm );
	RTC_Get_Date(&year,&month,&date,&week);
	return ( ( 2000 + year - 1980 ) << 25 ) | 
				(month << 21) | (date << 16) | 
				(hour << 11) | (min << 5) | (sec >> 1);
}

void *ff_memalloc ( UINT size )			
{
	return ( void* ) pvPortMalloc( size );
}

void ff_memfree ( void* mf )		 
{
	( void ) vPortFree( mf );
}

















