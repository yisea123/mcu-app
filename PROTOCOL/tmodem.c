#include "tmodem.h"
#include "kfifo.h"
#include "rtc.h"
#include "stmflash.h"
#include "stm32f4xx_flash.h"
#include "bootloader_update.h"
#include "rsa.h"
#include "md5.h"

char mPACKETSIZE = PACKET_SIZE;

extern char mMcuJumpAppPending;

char mScuRomUpdatePending = 0;

char mBootloaderUpdatePending = 0;

char devNum=MCU_NUM,  session_begin=0, mTickCount=0;

unsigned char md5[32];
unsigned int romSize = 0;
//static unsigned char buf_1024[1024] = { 0 };
static char FileName[FILE_NAME_LENGTH], num_eot = 0, num_ca=0;
static unsigned int packets_received = 0,  continuity=0, tmp0, tmp1;	
static unsigned int targetSector=0;
unsigned int flashdestination = APPLICATION_ADDRESS;

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
unsigned int FLASH_If_Erase_Sector(unsigned int StartSector)
{
  unsigned int UserStartSector = FLASH_Sector_1, i = 0;
	
	FLASH_Unlock();									//解锁 
  FLASH_DataCacheCmd(DISABLE);//FLASH擦除期间,必须禁止数据缓存
	
  /* Get the sector where start the user flash area */
  UserStartSector = GetSector(StartSector);

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
			return 0;
		}
  }
	
  FLASH_DataCacheCmd(ENABLE);
	FLASH_Lock();//上锁 
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
	printf("%s, mTickCount=%d\r\n", __func__, mTickCount);
	tmp0 = 0;
	tmp1 = 0;
	num_ca = 0;
	num_eot = 0;
	session_begin=0;
	continuity = 0;
	packets_received = 0;
	flashdestination = APPLICATION_ADDRESS; 	
}

static void cac_CA_num(char *num_ca)
{
	if(*num_ca == 0) { 
		*num_ca = 1, 
		tmp0 = continuity;
	} 
	if((continuity - tmp0) == 1) {
		if(*num_ca == 1) *num_ca = 2;
		printf("%s: CA CA!\r\n", __func__);
	} else if(session_begin) { 
		*num_ca = 1;
		tmp0 = continuity;
	} else {
		*num_ca = 0;
	}	
}

static int cac_EOT_num(char *num_eot)
{
	if(*num_eot == 0) { 
		*num_eot = 1, 
		tmp1 = continuity;
		return UD3;
	} 
	if((continuity - tmp1) == 1) {
		if(*num_eot == 1) *num_eot = 2;
		packets_received = 0;
		return UD4;						
	} else if(session_begin) { 
		*num_eot = 1;
		tmp1 = continuity;
		return UD3;
	} else {
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

int handle_update_bin(const char* packet_data, int len)
{
	static int mRecvBytes=0;
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
					cac_CA_num(&num_ca);
					if((num_ca) == 2 || (num_ca == 0))
						reset_tmodem_status();//连续接受到两包CA停止更新
					return UDS;
					/* End of transmission */
				case 0:	//EOT包				
					return cac_EOT_num(&num_eot);
							
					/* Normal packet */
				default:				
					if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff)) 
					{
						//Send_Byte(NAK);
						printf("%s: packet_data[PACKET_SEQNO_INDEX](%x)!=(%x)packets_received\r\n", __func__, 
								(packet_data[PACKET_SEQNO_INDEX] & 0xff), (packets_received & 0xff));
						
						if((packets_received & 0xff) - (packet_data[PACKET_SEQNO_INDEX] & 0xff) == 1) {
							return E3;
						} else {
							return E6;//ERR_PACKET_INDEX
						}
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
								for (i = 0, file_ptr = (unsigned char *)(packet_data + PACKET_HEADER); (*file_ptr != 0) && (i < FILE_NAME_LENGTH);) {
									FileName[i++] = *file_ptr++; 
								}
								
								FileName[i++] = '\0';
								printf("%s: FileName=%s\r\n", __func__, FileName);
								for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);) {
									file_size[i++] = *file_ptr++;
								}
								file_size[i++] = '\0';
								Str2Int(file_size, &size);
								size = size/2-LEN_MD5;
								mRecvBytes = 0;
								printf("%s: file size = %d, USE_FLASH_SIZE=%d\r\n",__func__, size, USER_FLASH_SIZE);
								/* Test the size of the image to be sent */
								/* Image size is greater than Flash size */
								if (size > (USER_FLASH_SIZE)) {
									/* End session */
									printf("%s: file size > flash size\r\n", __func__);
									return E4;
								}

								mTickCount = 0;
								romSize = size;
								session_begin = 1;
								packets_received ++;
								
								if(0X1234 != RTC_ReadBackupRegister(RTC_BKP_DR3) && 
									(devNum == MCU_NUM || devNum == SCU_NUM1)) 
								{
									printf("%s: start Erase Sector for download mcu or scu rom!\r\n", __func__);
									FLASH_If_Erase_Sector(flashdestination); 
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
								}
								
								targetSector = GetSector(flashdestination);
								return UD1;
							}     
						
							/* Filename packet is empty, end session */
							else if(session_begin)
							{
								session_done = 1;
								mTickCount = 0;
								break;
							} else {
								return E2;//ERR_TRANSMISS_START
							}
						}
					
						/* Data packet */
						else if(session_begin) {
							if(packets_received==1) printf("%s: flash save addr:%x\r\n", __func__, flashdestination);
							//buf_ptr = (unsigned char*)(packet_data+PACKET_HEADER);
							buf_ptr = decode_packet((unsigned char*)(packet_data+PACKET_HEADER), packet_length);
							packet_length = packet_length/2;
							mRecvBytes+=packet_length;
							if(mRecvBytes > romSize)
								catch_md5_from_packet(buf_ptr, packet_length, mRecvBytes - romSize);
							
							ramsource = (unsigned int)buf_ptr;				
							preFlashdestination = flashdestination;	
							flashdestination = STMFLASH_Write(flashdestination, (unsigned int*) ramsource, packet_length/4);
							if(flashdestination == preFlashdestination + packet_length) {
								packets_received ++;
								if(targetSector != GetSector(flashdestination))
								{
									printf("%s: rom is too big! Erase one more sector!\r\n", __func__);
									targetSector = GetSector(flashdestination);
									FLASH_If_Erase_Sector(flashdestination); 
								}
								mTickCount = 0;
								return UD2;					
							} else {
								/* End session */
								printf("%s: STMFLASH_Write error\r\n", __func__);
								printf("%s: des=0x%x, preDes=0x%x, packet_length=%d\r\n", __func__, flashdestination,
											preFlashdestination, packet_length);
								return E5;			
							}
						} else {
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
		  if(devNum == MCU_NUM || devNum == SCU_NUM1) 
			{
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
					return E7;
				}
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

	if (session_done != 0)
	{
		u32 src;
		
		if((devNum==MCU_NUM) || (devNum==SCU_NUM1)) 
		{
			src = APPLICATION_ADDRESS;
		}
		else
		{
			src = BOOTLOADER_ADDRESS;
		}
		
		printf("%s: Download Finish.\r\n", __func__);
		reset_tmodem_status();
		
		if(check_md5(src, romSize, md5) == 0)
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
		(devNum == MCU_NUM || devNum == SCU_NUM1)) 
	{
		printf("%s: start Erase Sector for download mcu or scu rom!\r\n", __func__);
		FLASH_If_Erase_Sector(APPLICATION_ADDRESS); 
		RTC_WriteBackupRegister(RTC_BKP_DR3, 0x1234);
	}	
}

void handle_tmodem_result(int result, const char* ack, int ack_len)
{
	int rand;
	switch(result)
	{
		case  UD0: 
				 rand = 3;
				 mPACKETSIZE = 40+rand*8;
				 printf("%s: UD0 mPACKETSIZE=%d\r\n",__func__, mPACKETSIZE);

				 if(mPACKETSIZE > 128) mPACKETSIZE=128;
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
							 printf("%s: UD5 devNum=%d\r\n", __func__, devNum);
							 if(MCU_NUM == devNum) {
								 printf("mMcuJumpAppPending=1\r\n");
								 mMcuJumpAppPending=1; 
							 } else if(SCU_NUM1 == devNum) {
								 handle_scu_rom_update();
							 } else if(BOOTLOADER_NUM == devNum) {
								 handle_booloader_rom_update();
							 }
							 break;
		
		case 	E0:  printf("E0\r\n");
		case  E1:  printf("E1\r\n");
		case  E3:  report_tmodem_packet(ACK); printf("E3\r\n"); break;
		case 	E2:  report_tmodem_packet(ERR_TRANSMISS_START); printf("E2\r\n"); break;
		case  E4:  report_tmodem_packet(ERR_SIZE_EXT);printf("E4\r\n"); break;
		case  E5:  report_tmodem_packet(ERR_FLASH_RW); printf("E5\r\n"); check_if_need_to_erase(); break;
		case  E6:  report_tmodem_packet(ERR_PACKET_INDEX); printf("E6\r\n"); break;
		case  E7:  report_tmodem_packet(ERR_UPDATE_REQUEST); printf("E7\r\n"); break;
		
		default:   break;	
	}
}

void handle_scu_rom_update(void)
{
	printf("%s: \r\n", __func__);
	mScuRomUpdatePending = 1;
	mScuRomUpdatePending = 0;
	return;
}

void handle_booloader_rom_update(void)
{
	printf("%s: \r\n", __func__);
	mBootloaderUpdatePending = 1;
	init_bootloaderUpdateState(&mBootloaderUpdatePending, romSize, md5);
}






