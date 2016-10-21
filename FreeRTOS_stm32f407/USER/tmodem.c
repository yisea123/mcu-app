#include "tmodem.h"
#include "kfifo.h"
#include "rtc.h"
#include "stmflash.h"
#include "stm32f4xx_flash.h"
#include "bootloader_update.h"
#include "rsa.h"
#include "md5.h"

/*****************************************************
		0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02
																									ACK     
																									C  
		AA BB 131  0x05 00 FF foo.c 102400 CRC1 CRC2
																									ACK
																									C
		AA BB 131  0x05 01 FE Data[128] CRC1 CRC2
																									ACK
		AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		
																									ACK
		.....																					
																									ACK 
		.....
																									ACK 		
		.....
																									ACK 		
		.....
																									ACK 		
		AA BB 2  0x05 EOT CRC1 CRC2
																									ACK
																									NAK
		AA BB 2  0x05 EOT CRC1 CRC2
																									ACK
																									C
		AA BB 131  cmd 00 FF NULL[128] CRC1 CRC2
																									ACK
******************************************************/


/**********************************************************************
		0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02
本协议由Ymodem修改而来，适配我们android与MCU通讯时的协议，
所以命名为Tmodem, 可先了解Ymodem再看此协议。

经过测试，此Tmodem协议传输文件的稳定性很高，目前还没有发现哪种情况会导致                 
文件丢失而误以为传输成功，要么传输成功，要么传输失败。                                   

PS: mPACKETSIZE
PR: packets_received & 0xff

1、	android 下发更新rom请求： 更新由android系统触发                                      
	  AA BB 3  0x05 UPDATE  devNum CRC1 CRC2													              ack                                    
																									UPDATE PS
																									ACK PR  packets_received=0 packetNum = 1 
																									2、 mcu上报C命令                       
																									C                   
3、 anroid 下发rom的文件名，以及文件大小                              
		AA BB 131  0x05 00 FF foo.c 102400 CRC1 CRC2                                         
																									ack                 
																									ACK PR  packets_received=1  packetNum = 2
																									4、 mcu上报C命令                       
																									C                   
5、 android 连续下发rom文件数据，最后一包不足128bytes时填充0          
		AA BB 131  0x05 01 FE Data[128] CRC1 CRC2                             	             
																									ack
																									ACK PR   packets_received=2  packetNum = 3
		AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		  							                         
																									ack
																									ACK PR   packets_received=3  packetNum = 4
		.....                                                                                
																									ack
	                                                ACK PR   packets_received=4  packetNum = 5
		.....                                                                                
																									ack
																									ACK PR   packets_received=5  packetNum = 6
																									                                       
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

static TIMER2* timer = NULL;
static uint8_t *key = NULL;
extern char mMcuJumpAppPending;
static unsigned char aes_key[32];
unsigned char tmodem_md5[LEN_MD5];
unsigned int romSize = 0, flashdestination = APPLICATION_ADDRESS;
char devNum=MCU_NUM, session_begin=0, mTmodemTickCount=0, 
	mScuRomUpdatePending=0, mBootloaderUpdatePending=0, mPACKETSIZE = PACKET_SIZE;

static char FileName[FILE_NAME_LENGTH], num_eot = 0, num_ca=0;
static unsigned int packets_received = 0,  continuity=0, tmp0, tmp1, targetSector=0;	

static void tmodem_timer(void* argc);

unsigned int Str2Int(unsigned char *inputstr, unsigned int *intnum)
{
  unsigned int i = 0, res = 0;
  unsigned int val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X')) 
	{
    if (inputstr[2] == '\0') 
		{
      return 0;
    }
		
    for (i = 2; i < 11; i++) 
		{
      if (inputstr[i] == '\0') 
			{
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
			
      if (ISVALIDHEX(inputstr[i])) 
			{
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      } 
			else 
			{
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 8 digit hex --invalid */
    if (i >= 11) 
		{
      res = 0;
    }
  } 
	else 
	{
    for (i = 0;i < 11;i++) 
		{
      if (inputstr[i] == '\0') 
			{
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      } 
			else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0)) 
			{
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      } 
			else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0)) 
			{
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      } 
			else if (ISVALIDDEC(inputstr[i])) 
			{
        val = val * 10 + CONVERTDEC(inputstr[i]);
      } 
			else 
			{
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11) 
		{
      res = 0;
    }
  }

  return res;
}

static unsigned int GetSector(unsigned int Address)
{
  unsigned int sector = 0;
  
  if((Address < ADDR_FLASH_SECTOR_1) && (Address >= ADDR_FLASH_SECTOR_0))
  {
    sector = FLASH_Sector_0;  
  }
  else if((Address < ADDR_FLASH_SECTOR_2) && (Address >= ADDR_FLASH_SECTOR_1))
  {
    sector = FLASH_Sector_1;  
  }
  else if((Address < ADDR_FLASH_SECTOR_3) && (Address >= ADDR_FLASH_SECTOR_2))
  {
    sector = FLASH_Sector_2;  
  }
  else if((Address < ADDR_FLASH_SECTOR_4) && (Address >= ADDR_FLASH_SECTOR_3))
  {
    sector = FLASH_Sector_3;  
  }
  else if((Address < ADDR_FLASH_SECTOR_5) && (Address >= ADDR_FLASH_SECTOR_4))
  {
    sector = FLASH_Sector_4;  
  }
  else if((Address < ADDR_FLASH_SECTOR_6) && (Address >= ADDR_FLASH_SECTOR_5))
  {
    sector = FLASH_Sector_5;  
  }
  else if((Address < ADDR_FLASH_SECTOR_7) && (Address >= ADDR_FLASH_SECTOR_6))
  {
    sector = FLASH_Sector_6;  
  }
  else if((Address < ADDR_FLASH_SECTOR_8) && (Address >= ADDR_FLASH_SECTOR_7))
  {
    sector = FLASH_Sector_7;  
  }
  else if((Address < ADDR_FLASH_SECTOR_9) && (Address >= ADDR_FLASH_SECTOR_8))
  {
    sector = FLASH_Sector_8;  
  }
  else if((Address < ADDR_FLASH_SECTOR_10) && (Address >= ADDR_FLASH_SECTOR_9))
  {
    sector = FLASH_Sector_9;  
  }
  else if((Address < ADDR_FLASH_SECTOR_11) && (Address >= ADDR_FLASH_SECTOR_10))
  {
    sector = FLASH_Sector_10;  
  }
  else/*(Address < FLASH_END_ADDR) && (Address >= ADDR_FLASH_SECTOR_11))*/
  {
    sector = FLASH_Sector_11;  
  }
    return sector;
}

void FLASH_If_Init(void)
{ 
  FLASH_Unlock(); 

  /* Clear pending flags (if any) */  
  FLASH_ClearFlag(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | 
                  FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR|FLASH_FLAG_PGSERR);
}

unsigned int FLASH_If_DisableWriteProtection(void)
{
  __IO unsigned int UserStartSector = FLASH_Sector_1, UserWrpSectors = OB_WRP_Sector_1;

  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(APPLICATION_ADDRESS);

  /* Mark all sectors inside the user flash area as non protected */  
  UserWrpSectors = 0xFFF-((1 << (UserStartSector/8))-1);
   
  /* Unlock the Option Bytes */
  FLASH_OB_Unlock();

  /* Disable the write protection for all sectors inside the user flash area */
  FLASH_OB_WRPConfig(UserWrpSectors, DISABLE);

  /* Start the Option Bytes programming process. */  
  if (FLASH_OB_Launch() != FLASH_COMPLETE)
  {
    /* Error: Flash write unprotection failed */
    return (2);
  }

  /* Write Protection successfully disabled */
  return (1);
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  StartSector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  */
unsigned int FLASH_If_Erase_Sector(unsigned int StartSector)
{
  unsigned int UserStartSector = FLASH_Sector_1, i = 0;
	
	FLASH_Unlock();									//解锁 
  FLASH_DataCacheCmd(DISABLE);//FLASH擦除期间,必须禁止数据缓存
	
  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(StartSector);
  printf("%s\r\n", __func__);
	
  for(i = 0; i <= 10; i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (FLASH_EraseSector(UserStartSector, VoltageRange_3) != FLASH_COMPLETE)
    {
      /* Error occurred while page erase */
			printf("%s: addr=0X%X, sector=%d, error\r\n", __func__, StartSector, UserStartSector);
    }
		else 
		{
			//printf("%s: addr=0X%X,sector=%d, success\r\n", __func__, StartSector, UserStartSector);
			FLASH_DataCacheCmd(ENABLE);	//FLASH擦除结束,开启数据缓存
			FLASH_Lock();//上锁
			printf("%s out.\r\n", __func__);
			printf("%s: addr=0X%X, sector=%d, success!\r\n", __func__, StartSector, UserStartSector);
			return 0;
		}
  }
	
  FLASH_DataCacheCmd(ENABLE);	//FLASH擦除结束,开启数据缓存
	FLASH_Lock();//上锁 
	printf("%s out.\r\n", __func__);
  return (0);
}

unsigned int FLASH_If_Write(__IO unsigned int* FlashAddress, unsigned int* Data ,unsigned int DataLength)
{
  unsigned int i = 0;

  for (i = 0; (i < DataLength) && (*FlashAddress <= (USER_FLASH_END_ADDRESS-4)); i++)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (FLASH_ProgramWord(*FlashAddress, *(unsigned int*)(Data+i)) == FLASH_COMPLETE)
    {
     /* Check the written value */
      if (*(uint32_t*)*FlashAddress != *(unsigned int*)(Data+i))
      {
        /* Flash content doesn't match SRAM content */
        return(2);
      }
      /* Increment FLASH destination address */
      *FlashAddress += 4;
    }
    else
    {
      /* Error occurred while writing data in Flash memory */
      return (1);
    }
  }

  return (0);
}

static unsigned int continuity_add(char session_begin)
{
	if(session_begin) continuity++;
	return continuity;
}

void reset_tmodem_status(void)
{
	printf("%s, mTmodemTickCount=%d\r\n", __func__, mTmodemTickCount);
	tmp0 = 0;
	tmp1 = 0;
	num_ca = 0;
	num_eot = 0;
	session_begin=0;
	continuity = 0;
	packets_received = 0;
	flashdestination = APPLICATION_ADDRESS; 	
	//在此处删除 session_begin=1时添加的定时器，同时用一个变量去标示此定时器
	//是否设置，如果设置了，在此处删除此定时器。
}

static void cac_CA_num(char *num_ca)
{
	if(*num_ca == 0) 
	{ 
		*num_ca = 1, 
		tmp0 = continuity;
		//printf("%s: CA!\r\n", __func__);
	} 
	
	if((continuity - tmp0) == 1) 
	{
		if(*num_ca == 1) 
			*num_ca = 2;
		printf("%s: CA CA!\r\n", __func__);
	} 
	else if(session_begin) 
	{ 
		*num_ca = 1;
		tmp0 = continuity;
		//printf("%s: CA X X X CA !\r\n", __func__);
	} 
	else 
	{
		*num_ca = 0;
	}	
}

static int cac_EOT_num(char *num_eot)
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
		packets_received = 0;
		return UD4;						
	} 
	else if(session_begin) 
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

static int phase_packet (const char *data, int len, int *length)
{
  unsigned char c;	
  unsigned short packet_size;

  *length = 0;

  if(data[2] == mPACKETSIZE+3) c = SOH;
  else if((data[2] == CMD_PACKET_LEN) && (data[4] == EOT)) c = EOT;
  else if((data[2] == CMD_UPDATE_PACKET_LEN) && (data[4] == UPDATE)) c = UPDATE;	
  else if((data[2] == CMD_PACKET_LEN) && (data[4] == CA)) c = CA;	
  else if((data[2] == DATA_PACKET_LEN1)) c = STX; 
	
  switch (c)
  {
    case SOH:
      packet_size = mPACKETSIZE;
      break;
    case STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:
			//printf("%s: EOT\r\n", __func__);
			continuity_add(session_begin);
      return 0;

		case CA:
			//printf("%s: CA\r\n", __func__);
			*length = -1;
			continuity_add(session_begin);
			return 0;
		
		case UPDATE:
			printf("%s: UPDATE\r\n", __func__);
			continuity_add(session_begin);
			return 2;
		
    default:
			printf("%s:error cmd, data[2]=%d\r\n", __func__, data[2]);
      return -1;
  }
  
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  
  *length = packet_size;
  continuity_add(session_begin);
	
  return 0;
}

/************************************************************************************************
static void catch_md5_from_packet(const unsigned char *buf_ptr, const int ps, const int k)
{
	static unsigned char *ptr;
	
	if(k < LEN_MD5) 
	{
		ptr = md5;
		memcpy(ptr, buf_ptr+(ps-k), k);
		ptr += k;
	}
	else if((k >= LEN_MD5)&&(k < ps)) 
	{
		memcpy(md5, buf_ptr+(ps-k), LEN_MD5);
		md5[LEN_MD5] = '\0';
		printf("%s:00--[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]\r\n", __func__,
				md5[0], md5[1],md5[2], md5[3],md5[4], md5[5],md5[6], md5[7],md5[8], md5[9],
				md5[10], md5[11],md5[12], md5[13],md5[14], md5[15]); 
	}
	else if(k == ps) 
	{
		memcpy(md5, buf_ptr, LEN_MD5);
		md5[LEN_MD5] = '\0';
		printf("%s:01--[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]\r\n", __func__,
				md5[0], md5[1],md5[2], md5[3],md5[4], md5[5],md5[6], md5[7],md5[8], md5[9],
				md5[10], md5[11],md5[12], md5[13],md5[14], md5[15]); 							
	}
	else if(k > ps) 
	{
		memcpy(ptr, buf_ptr, LEN_MD5-(k-ps));
		ptr = md5;
		md5[LEN_MD5] = '\0';
		printf("%s:02--[%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X]\r\n", __func__,
				md5[0], md5[1],md5[2], md5[3],md5[4], md5[5],md5[6], md5[7],md5[8], md5[9],
				md5[10], md5[11],md5[12], md5[13],md5[14], md5[15]); 
	}
	else
		printf("%s:error!\r\n", __func__);
}
************************************************************************************************/

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

/***********************************************************
0xaa 0xbb len 0x05 d0 d1 d2 d3... check01 check02		
固件更新状态：在更新固件过程中返回TMODEM协议所要求的反馈			
************************************************************/		

int handle_update_bin(const char* packet_data, int len)
{
	static unsigned int mRecvBytes=0;
	int i, session_done = 0, packet_length;
	unsigned int size = 0, ramsource = 0, preFlashdestination;
	unsigned char *buf_ptr, *file_ptr, file_size[FILE_SIZE_LENGTH];	
	
	switch (phase_packet(packet_data, len, &packet_length))	
	{
	 case 0:
      switch (packet_length)
      {
				/* Abort by sender */
				case -1: //CA包
					/* 用于计算有连续的两个CA命令从android下发过来*/
					cac_CA_num(&num_ca);
					if((num_ca) == 2 || (num_ca == 0))
						reset_tmodem_status();//连续接受到两包CA停止更新
					return UDS;
					/* End of transmission */
				case 0:	//EOT包				
					/* 用于计算有连续的两个EOT命令从android下发过来*/
					return cac_EOT_num(&num_eot);
							
					/* Normal packet */
				default:				
					
					if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff)) 
					{
						//Send_Byte(NAK);
						printf("%s: packet_data[PACKET_SEQNO_INDEX](%x)!=(%x)packets_received\r\n", __func__, 
								(packet_data[PACKET_SEQNO_INDEX] & 0xff), (packets_received & 0xff));
						
						if((packets_received & 0xff) - (packet_data[PACKET_SEQNO_INDEX] & 0xff) == 1) 
						{
							return E3;
						} 
						else 
						{
							return E6;//ERR_PACKET_INDEX
						}
						//返回某个指令，让android停止更新。
					} 
					else 
					{	 	
						if (packets_received == 0)
						{
							/* Filename packet */
							if (packet_data[PACKET_HEADER] != 0) 
							{
								if((packet_data[PACKET_SEQNO_INDEX] & 0xff) != 0) return E6;//erro file info! packet num error
								else ;//printf("%s: file info right.\r\n", __func__);
								/* Filename packet has valid data */
								for (i = 0, file_ptr = (unsigned char *)(packet_data + PACKET_HEADER); (*file_ptr != 0) && (i < FILE_NAME_LENGTH);) 
								{
									FileName[i++] = *file_ptr++; 
								}
								
								FileName[i++] = '\0';
								printf("%s: FileName=%s\r\n", __func__, FileName);
								
								for (i = 0, file_ptr ++; (*file_ptr != '\0') && (i < FILE_SIZE_LENGTH);) 
								{
									file_size[i++] = *file_ptr++;
								}
								file_size[i++] = '\0';
								Str2Int(file_size, &size);
								/*RSA 脱密后， 真实的bin文件， size 只有一半, 并且最后16bytes为MD5值*/
								//size = size/2-LEN_MD5;
								size = size-LEN_MD5;
								mRecvBytes = 0;
								printf("%s: file size = %d, USE_FLASH_SIZE=%d\r\n",__func__, size, USER_FLASH_SIZE);
								/* Test the size of the image to be sent */
								/* Image size is greater than Flash size */
								if (size > (USER_FLASH_SIZE)) 
								{
									/* End session */
									printf("%s: file size > flash size\r\n", __func__);
									return E4; //rom过大，falsh不够保存 ERR_SIZE_EXT
								}
								
								printf("aes_key:\r\n[");
								
								for (i = 0, file_ptr ++; i < sizeof(aes_key)/sizeof(aes_key[0]); i++) 
								{
									aes_key[i] = *file_ptr++;
									printf("%02x,", aes_key[i]);
								}								
								printf("]\r\n");
								
								if(key) { aes_destory_key(key); key = NULL;}
								key = aes_create_key(aes_key, sizeof(aes_key)/sizeof(aes_key[0]));
								
								if(!key) 
									return E2;
								
								/*在此处添加一个定时器，用于计算android 在4秒钟之内是否 还在继续发送rom的packet包，如果没有，调用reset_tmodem_status（）
								同时用一个变量去标示此定时器, 与uart_command.c中的代码，共用定时器2   TIM2_IRQHandler*/
								mTmodemTickCount = 0;
								/**计数清0，在session_begin = 1的情况下，长时间不清0会导致 reset_tmodem_status被调用*/
								romSize = size;
								session_begin = 1;
								packets_received ++;
								
								if(0X1234 != RTC_ReadBackupRegister(RTC_BKP_DR3) && 
										(devNum == MCU_NUM || devNum == SCU_NUM)) 
								{
									/*如果bootloader已擦除， RTC_BKP_DR3为0X1234*/
									printf("%s: start Erase Sector for download mcu or scu rom!\r\n", __func__);
									FLASH_If_Erase_Sector(flashdestination);
									//FLASH_If_Erase_Sector(APPLICATION_ADDRESS_1);
									//FLASH_If_Erase_Sector(APPLICATION_ADDRESS_2);									
								}
								
								if(devNum == BOOTLOADER_NUM)
								{
									printf("%s: start Erase Sector for download bootloader rom!\r\n", __func__);
									FLASH_If_Erase_Sector(flashdestination); 
								}
								else 
								{
									printf("%s: reset RTC_BKP_DR3 = 0!\r\n", __func__);
									RTC_WriteBackupRegister(RTC_BKP_DR3,0x0000);
									/*如果下载失败，再次进来时，就会进入擦除扇区的分支！*/
								}
								
								targetSector = GetSector(flashdestination);
								
								if(timer == NULL)
									timer = register_timer2("tmodem_timer", TIMER2SECOND, tmodem_timer, REPEAT, &session_begin);
								/*必须擦写扇区为0XFFFFFFFF*/
								return UD1;
							}     
						
							/* Filename packet is empty, end session */
							else if(session_begin)
							{
								session_done = 1;
								mTmodemTickCount = 0;
								break;
							} 
							else 
							{
								return E2;//ERR_TRANSMISS_START
							}
						}
					
						/* Data packet */
						else if(session_begin) 
						{
							if(packets_received==1) 
								printf("%s: flash save addr:%x\r\n", __func__, flashdestination);
							//buf_ptr = (unsigned char*)(packet_data+PACKET_HEADER);
							for(i=0; i< packet_length/AES_DECODE_LEN; i++) 
							{
								buf_ptr = aes_decode_packet(key, (uint8_t*)(packet_data+PACKET_HEADER+AES_DECODE_LEN*i), AES_DECODE_LEN);
								
								/*RSA 脱密后，  有效数据只有packet_length的一半*/
								/*
								packet_length = packet_length/2;
								mRecvBytes+=packet_length;
								if(mRecvBytes > romSize)
								catch_md5_from_packet(buf_ptr, packet_length, mRecvBytes - romSize);
								*/
								
								ramsource = (unsigned int)buf_ptr;				
								preFlashdestination = flashdestination;	
								flashdestination = STMFLASH_Write(flashdestination, (uint32_t *) ramsource, AES_DECODE_LEN/4);
								
								if(flashdestination == preFlashdestination + AES_DECODE_LEN) 
								{
									//packets_received ++;
									if(targetSector != GetSector(flashdestination))
									{
										printf("%s: rom is too big! Erase one more sector!\r\n", __func__);
										targetSector = GetSector(flashdestination);
										FLASH_If_Erase_Sector(flashdestination); 
									}
									mTmodemTickCount = 0;
									/**计数清0，在session_begin = 1的情况下，长时间不清0会导致 reset_tmodem_status被调用*/
									//return UD2;					
								} 
								else 
								{
									/* End session */
									printf("%s: STMFLASH_Write error\r\n", __func__);
									printf("%s: des=0x%x, preDes=0x%x, packet_length=%d\r\n", __func__, flashdestination,
												preFlashdestination, packet_length);
									return E5; //MCU读写flash 出错，停止更新 ERR_FLASH_RW				
								}
							} 
							mRecvBytes+=packet_length;
							/*
							if(mRecvBytes > romSize)
								catch_md5_from_packet(buf_ptr, packet_length, mRecvBytes - romSize);
							*/
							//if(mRecvBytes >= romSize+LEN_MD5) read_md5_from_flash(devNum, romSize, md5);
							packets_received ++;
							return UD2;
						}
						else 
						{
							printf("session_begin=0, but android send rom data...\r\n");
							return E2; //ERR_TRANSMISS_START
						}
				 }

				//break;
			}
		break;

		case 2:
			//update mcu rom request
			devNum = packet_data[5];
			printf("%s: want to update dev num = %d\r\n", __func__, devNum);
		
		  if(devNum == MCU_NUM || devNum == SCU_NUM) 
			{
				/*把需要更新的rom数据保存到不同的位置，根据不同的设备rom!*/
				if(mScuRomUpdatePending == 0 && mMcuJumpAppPending == 0) 
				{
					reset_tmodem_status();
					flashdestination = APPLICATION_ADDRESS;
					return UD0;
				}
				else 
				{
					return E7;
				}
			} 
			else if(devNum == BOOTLOADER_NUM) 
			{
				if(mScuRomUpdatePending == 0)
				{ 
					reset_tmodem_status();
					flashdestination = BOOTLOADER_ADDRESS;
					return UD0;
				} 
				else
				{
					/*上一次bootloader更新数据还没有处理完*/
					return E7;
				}
			}
			else 
			{
				/*APPLICATION_ADDRESS 数据还没被处理，不能更新，请等待。。。*/
				return E7; 
				/*后续添加android对E7的处理。 */
			}
		case -1:
			/*传输非协议包 do nothing */
			printf("%s: ERROR E0\r\n", __func__);
			return E0;
		
    default:

			break;
		
	}

	if (session_done != 0)
	{
		u32 src;
		
		if(timer)
		{
				if(unregister_timer2(timer) == 0)
				{
					timer = NULL;
				}
				else
				{
					printf("%s: fail to delete timer\r\n", __func__);
				}
		}
		
		read_md5_from_flash(devNum, romSize, tmodem_md5);
		
		if((devNum==MCU_NUM) || (devNum==SCU_NUM)) 
		{
			src = APPLICATION_ADDRESS;
		}
		else
		{
			src = BOOTLOADER_ADDRESS;
		}
		
		printf("%s: Download Finish.\r\n", __func__);
		reset_tmodem_status();
		
		if(compare_flashdata_md5(src, romSize, tmodem_md5) == 0)
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

void report_tmodem_packet(char C)
{
	char cmd[7];
	unsigned short checkValue;	
	//printf("%s, c=%x\r\n", __func__, C);
	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	cmd[2] = 2;
	cmd[3] = 0x0e;
	cmd[4] = C;
	checkValue = calculate_crc((uint8*)cmd+LEN, 3);
	cmd[5] = (unsigned char)(checkValue & 0xff);
	cmd[6] = (unsigned char)((checkValue & 0xff00) >> 8);	
	
	make_event_to_list(cmd, 7, CAN_EVENT, 0);			
}

void report_tmodem_packet0(char C, char C0)
{
	char cmd[8];
	unsigned short checkValue;	
	//printf("%s, c=%x\r\n", __func__, C);
	cmd[0] = 0xaa;
	cmd[1] = 0xbb;
	cmd[2] = 3;
	cmd[3] = 0x0e;
	cmd[4] = C;
	cmd[5] = C0;
	checkValue = calculate_crc((uint8*)cmd+LEN, 4);
	cmd[6] = (unsigned char)(checkValue & 0xff);
	cmd[7] = (unsigned char)((checkValue & 0xff00) >> 8);	
	
	make_event_to_list(cmd, 8, CAN_EVENT, 0);			
}

void check_if_need_to_erase()
{
	if(0X1234 != RTC_ReadBackupRegister(RTC_BKP_DR3) && 
		(devNum == MCU_NUM || devNum == SCU_NUM)) 
	{
		/*如果bootloader已擦除， RTC_BKP_DR3为0X1234*/
		printf("%s: start Erase Sector for download mcu or scu rom!\r\n", __func__);
		FLASH_If_Erase_Sector(APPLICATION_ADDRESS);
		FLASH_If_Erase_Sector(APPLICATION_ADDRESS_1);
		FLASH_If_Erase_Sector(APPLICATION_ADDRESS_2);		
		RTC_WriteBackupRegister(RTC_BKP_DR3, 0x1234);
	}	
}

void erase_flash(void)
{
	printf("erase flash test!\r\n");
	FLASH_If_Erase_Sector(APPLICATION_ADDRESS);
	FLASH_If_Erase_Sector(APPLICATION_ADDRESS_1);
	FLASH_If_Erase_Sector(APPLICATION_ADDRESS_2);	
}

extern uint32_t rand_count;
extern long numMcuReportToAndroid;
extern char mAndroidShutDownPending;
/**
处理更新MCU rom的命令包后，所返回的结果，根据Tmodem协议严格回答android， 否则将会导致更新出问题!
**/
void handle_tmodem_result(int result, const char* ack, int ack_len)
{
	int rand;
	
	switch(result)
	{
		//处理UPDATE 状态
		case  UD0: 
							 rand = rand_count%5;
							 mPACKETSIZE = 64+rand*16;	
							 printf("%s: UD0 mPACKETSIZE=%d\r\n",__func__, mPACKETSIZE);
							 if(mPACKETSIZE>128 || mPACKETSIZE<64) mPACKETSIZE=128;
							 report_tmodem_packet0(UPDATE, mPACKETSIZE);     
							 report_tmodem_packet0(ACK, (packets_received & 0xff)); 
							 report_tmodem_packet(CRC16);break;
		
		case  UD1: report_tmodem_packet0(ACK, (packets_received & 0xff)); 
							 report_tmodem_packet(CRC16); break;
		case  UD4: report_tmodem_packet(EOT_ACK); 
		           report_tmodem_packet(CRC16); break;
		case  UDS: break;//CA CA
		case  UD2: report_tmodem_packet0(ACK, (packets_received & 0xff)); break;
		case  UD3: report_tmodem_packet(EOT_ACK); report_tmodem_packet(NAK); break;
		case  UD5: report_tmodem_packet(LAST_ACK); report_tmodem_packet(UPDATE_DONE); 
		/*可直接返回update done， 如果还没完成对flash数据的使用，android再次请求update时， 返回E7错误！*/
							 printf("%s: UD5 devNum=%d\r\n", __func__, devNum);
		
							 if(MCU_NUM == devNum) 
							 {
								 printf("mMcuJumpAppPending=1\r\n");
								 mAndroidShutDownPending = 1;
								 mMcuJumpAppPending=1; 
							 } 
							 else if(SCU_NUM == devNum) 
							 {
								 handle_scu_rom_update();
							 } 
							 else if(BOOTLOADER_NUM == devNum) 
							 {
								 handle_booloader_rom_update();
							 }
							 break;//更新完成，请求重启！
		
		//处理错误信息
		case 	E0:  printf("E0\r\n");
		case  E1:  printf("E1\r\n");
		case  E3:  report_tmodem_packet(ACK); printf("E3\r\n");
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		case 	E2:  report_tmodem_packet(ERR_TRANSMISS_START); printf("E2\r\n"); 
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		case  E4:  report_tmodem_packet(ERR_SIZE_EXT);printf("E4\r\n");
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		case  E5:  report_tmodem_packet(ERR_FLASH_RW); printf("E5\r\n"); 
							 check_if_need_to_erase(); 
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		case  E6:  report_tmodem_packet(ERR_PACKET_INDEX); printf("E6\r\n"); 
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		case  E7:  report_tmodem_packet(ERR_UPDATE_REQUEST); printf("E7\r\n"); 
							 
							 if(key) {aes_destory_key(key); key=NULL;} break;
		
		default:   if(key) {aes_destory_key(key); key=NULL;} break;	
	}
}

void handle_scu_rom_update(void)
{
	printf("%s: \r\n", __func__);
	mScuRomUpdatePending = 1;
	/*copy APPLICATION_ADDRESS to SCU flash! 处理完之后*/
	mScuRomUpdatePending = 0;
	return;
}

/*bootloader 本应该在这里直接拷贝到0X08000000，为给更新scu做示范，使用一帧一帧更新的处理方式*/
void handle_booloader_rom_update(void)
{
	printf("%s: \r\n", __func__);
	mBootloaderUpdatePending = 1;
	init_bootloaderUpdateState(&mBootloaderUpdatePending, romSize, tmodem_md5);
}

/*need to invoved in every 1s.*/
void tmodem_timer(void* argc)
{
		char *begin = (char *)argc;
		
		printf("%s\r\n", __func__);
		
		if(*begin)
		{
				if(++mTmodemTickCount > 6)
				{
						reset_tmodem_status();
				}
		}	
}

void tmodem_init(void)
{
		;//register_timer2("tmodem_timer", TIMER2SECOND, tmodem_timer, REPEAT, &session_begin);		
}





