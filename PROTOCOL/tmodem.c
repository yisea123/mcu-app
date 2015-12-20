#include "tmodem.h"
#include "kfifo.h"
#include "rtc.h"
#include "stmflash.h"
#include "stm32f4xx_flash.h"
//0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02

//AA BB 131  0x05 00 FF foo.c 102400 CRC1 CRC2
//								ACK
//								C
//AA BB 131  0x05 01 FE Data[128] CRC1 CRC2
//								ACK
//AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		
//								ACK
//.....
//AA BB 2  0x05 EOT CRC1 CRC2
//								NAK
//AA BB 2  0x05 EOT CRC1 CRC2
//								ACK
//								C
//AA BB 131  cmd 00 FF NULL[128] CRC1 CRC2
//								ACKS

unsigned int packets_received = 0;
int session_begin=0;
unsigned int flashdestination = APPLICATION_ADDRESS;	
char FileName[FILE_NAME_LENGTH];
pFunction Jump_To_Application;
unsigned int AppAddress;
unsigned char buf_1024[1024] = { 0 };

 unsigned int Str2Int(unsigned char *inputstr, unsigned int *intnum)
{
  unsigned int i = 0, res = 0;
  unsigned int val = 0;

  if (inputstr[0] == '0' && (inputstr[1] == 'x' || inputstr[1] == 'X')) {
    if (inputstr[2] == '\0') {
      return 0;
    }
    for (i = 2; i < 11; i++) {
      if (inputstr[i] == '\0') {
        *intnum = val;
        /* return 1; */
        res = 1;
        break;
      }
      if (ISVALIDHEX(inputstr[i])) {
        val = (val << 4) + CONVERTHEX(inputstr[i]);
      } else {
        /* Return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 8 digit hex --invalid */
    if (i >= 11) {
      res = 0;
    }
  } else {
    for (i = 0;i < 11;i++) {
      if (inputstr[i] == '\0') {
        *intnum = val;
        /* return 1 */
        res = 1;
        break;
      } else if ((inputstr[i] == 'k' || inputstr[i] == 'K') && (i > 0)) {
        val = val << 10;
        *intnum = val;
        res = 1;
        break;
      } else if ((inputstr[i] == 'm' || inputstr[i] == 'M') && (i > 0)) {
        val = val << 20;
        *intnum = val;
        res = 1;
        break;
      } else if (ISVALIDDEC(inputstr[i])) {
        val = val * 10 + CONVERTDEC(inputstr[i]);
      } else {
        /* return 0, Invalid input */
        res = 0;
        break;
      }
    }
    /* Over 10 digit decimal --invalid */
    if (i >= 11) {
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
unsigned int FLASH_If_Erase(unsigned int StartSector)
{
  unsigned int UserStartSector = FLASH_Sector_1, i = 0;

  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(APPLICATION_ADDRESS);

  for(i = UserStartSector; i <= FLASH_Sector_11; i += 8)
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if (FLASH_EraseSector(i, VoltageRange_3) != FLASH_COMPLETE)
    {
      /* Error occurred while page erase */
      return (1);
    }
  }
  
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

//AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		
//								ACK
//.....
//AA BB 2  0x05 EOT CRC1 CRC2

static int phase_packet (const char *data, int len, int *length)
{
  unsigned short packet_size;
  unsigned char c;
  *length = 0;

  if(data[2] == 131) c = SOH;
  else if(data[2] == 1027) c = STX;
  else if(data[2] == 2 && data[4] == EOT) c = EOT;
  else if(data[2] == 2 && data[4] == ABORT1) c = ABORT1;
  else if(data[2] == 2 && data[4] == ABORT2) c = ABORT2;
  else if(data[2] == 2 && data[4] == CA) c = CA;
  
  switch (c)
  {
    case SOH:
      packet_size = PACKET_SIZE;
      break;
    case STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:
      return 0;

	case CA:
	  *length = -1;
	  return 0;

	//AA BB 2  0x05 ABORT1 CRC1 CRC2

    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }
  
  *length = packet_size;
  
  return 0;
}

int handle_update_bin(const char* packet_data, int len)
{
	int i, session_done = 0, packet_length;
	unsigned int size = 0, ramsource = 0, preFlashdestination;
	unsigned char *buf_ptr, *file_ptr, file_size[FILE_SIZE_LENGTH];	
	
	switch (phase_packet(packet_data, len, &packet_length))	
	{
		 case 0:
				switch (packet_length)
				{
					/* Abort by sender */
					case -1:				
						//终止包
						//Send_Byte(ACK);
						session_begin=0;
						packets_received = 0;
						flashdestination = APPLICATION_ADDRESS;          
						return 1;
						/* End of transmission */
					case 0:					
						//正常结束包
						//Send_Byte(ACK);
						packets_received = 0;
						return 2;
						//break;
						/* Normal packet */
					default:				
							//正常数据包
							if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff)) 
							{
								//Send_Byte(NAK);
								return -2;
							} 
							else 
							{	 	
								if (packets_received == 0)
								{
									/* Filename packet */
									if (packet_data[PACKET_HEADER] != 0) 
									{
										/* Filename packet has valid data */
										for (i = 0, file_ptr = (unsigned char *)(packet_data + PACKET_HEADER); (*file_ptr != 0) && (i < FILE_NAME_LENGTH);) 
										{
											FileName[i++] = *file_ptr++; 
										}
										FileName[i++] = '\0';
										printf("%s: FileName=%s\r\n", __func__, FileName);
										for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);) 
										{
											file_size[i++] = *file_ptr++;
										}
										file_size[i++] = '\0';
										Str2Int(file_size, &size);
										printf("%s: file size = %d\r\n", __func__, size);
										/* Test the size of the image to be sent */
										/* Image size is greater than Flash size */
										if (size > (/*USER_FLASH_SIZE*/0x080FFFFF-0x08080000 + 1)) 
										{
											/* End session */
											// Send_Byte(CA);
											// Send_Byte(CA);	
											printf("%s: file size > flash size\r\n", __func__);
											return -1;
										}
										FLASH_If_Init();	  //解锁FLASH
										/* erase user application area */
										FLASH_If_Erase(APPLICATION_ADDRESS);
										//Send_Byte(ACK);
										//Send_Byte(CRC16);		
										session_begin = 1;
										packets_received ++;
										//FLASH_If_DisableWriteProtection();
										//使用原子的代码，这个调用可不调用
										return 0;
									}     
									
									/* Filename packet is empty, end session */
									else 
									{
										//Send_Byte(ACK);
										session_done = 1;
										break;
									}
								}
								
								/* Data packet */
								else
								{
									buf_ptr = &buf_1024[0];
													memcpy(buf_ptr, packet_data+PACKET_HEADER, packet_length);
													ramsource = (unsigned int)buf_ptr;				

									preFlashdestination = flashdestination;	
									flashdestination = STMFLASH_Write(flashdestination, (unsigned int*) ramsource, packet_length/4);
									if(flashdestination == preFlashdestination+packet_length)
									{
										//Send_Byte(ACK);
										packets_received ++;
										return 0;					
									}
									else
									{
										/* End session */
										//Send_Byte(CA);
										//Send_Byte(CA);
										return -2;					
									}
									/* Write received data in Flash */
									if (FLASH_If_Write(&flashdestination, (unsigned int*) ramsource, 
										(unsigned short) packet_length/4)  == 0)
									{
										//Send_Byte(ACK);
										packets_received ++;
										return 0;
									}
									else /* An error occurred while writing to Flash memory */
									{
										/* End session */
										//Send_Byte(CA);
										//Send_Byte(CA);
										return -2;
									}
							 }
					}

				//break;
			}
			break;

    case 1: //人为中断发送bin
      //Send_Byte(CA);
      //Send_Byte(CA);
      return -3;
	  
    default:// -1 数据发生错误
		
			break;
		
	}

	if (session_done != 0)
	{
		FLASH_Lock();
	  session_begin=0;
	  packets_received = 0;
	  flashdestination = APPLICATION_ADDRESS;
	  //finish get xxx.bin
		
    if (((*(__IO unsigned int*)APPLICATION_ADDRESS) & 0x2FFE0000 ) == 0x20000000)  
		//测试代码是否从APPLICATION_ADDRESS开始
    { 
			RTC_WriteBackupRegister(RTC_BKP_DR19,0x5a5a);	//标记已经初始化过了
			printf("%s: RTC_BKP_DR19=%x\r\n", __func__, RTC_ReadBackupRegister(RTC_BKP_DR19));
			printf("%s: start jump to new addr %x\r\n", __func__, APPLICATION_ADDRESS);
      AppAddress = *(__IO unsigned int*) (APPLICATION_ADDRESS + 4);	
      Jump_To_Application = (pFunction) AppAddress;	
      __set_MSP(*(__IO unsigned int*) APPLICATION_ADDRESS);
      Jump_To_Application();
    }		
	}
	
	return 0;
}

