#include "stmflash.h"
#include "delay.h"
#include "usart.h" 


 
 
//��ȡָ����ַ�İ���(16λ����) 
//faddr:����ַ 
//����ֵ:��Ӧ����.
u32 STMFLASH_ReadWord(u32 faddr)
{
	return *(vu32*)faddr; 
}  

//��ȡĳ����ַ���ڵ�flash����
//addr:flash��ַ
//����ֵ:0~11,��addr���ڵ�����
uint16_t STMFLASH_GetFlashSector(u32 Address)
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
	/*STM32F407VET6 512K flash, so FLASH_Sector_1 ~ FLASH_Sector_7*/	
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

//��ָ����ַ��ʼд��ָ�����ȵ�����
//�ر�ע��:��ΪSTM32F4������ʵ��̫��,û�취���ر�����������,���Ա�����
//         д��ַ�����0XFF,��ô���Ȳ������������Ҳ�������������.����
//         д��0XFF�ĵ�ַ,�����������������ݶ�ʧ.����д֮ǰȷ��������
//         û����Ҫ����,��������������Ȳ�����,Ȼ����������д. 
//�ú�����OTP����Ҳ��Ч!��������дOTP��!
//OTP�����ַ��Χ:0X1FFF7800~0X1FFF7A0F
//WriteAddr:��ʼ��ַ(�˵�ַ����Ϊ4�ı���!!)
//pBuffer:����ָ��
//NumToWrite:��(32λ)��(����Ҫд���32λ���ݵĸ���.) 
//���ȫ��д�ɹ������ص�ַΪ(WriteAddr+NumToWrite*4), ����дʧ��
u32 STMFLASH_Write(u32 WriteAddr,u32 *pBuffer,u32 NumToWrite)	
{ 
  FLASH_Status status = FLASH_COMPLETE;
//	u32 addrx=0;
	u32 endaddr=0;	
	u32 readValue = 0;
	
  if(WriteAddr<STM32_FLASH_BASE||WriteAddr%4)return WriteAddr;	//�Ƿ���ַ
	FLASH_Unlock();									//���� 
  FLASH_DataCacheCmd(DISABLE);//FLASH�����ڼ�,�����ֹ���ݻ���
 		
//	addrx=WriteAddr;				//д�����ʼ��ַ
	endaddr=WriteAddr+NumToWrite*4;	//д��Ľ�����ַ
					 //0x08008000
/**
	if(addrx<0X1FFF0000)			//ֻ�����洢��,����Ҫִ�в�������!!
	{
		while(addrx<endaddr)		//ɨ��һ���ϰ�.(�Է�FFFFFFFF�ĵط�,�Ȳ���)
		{
			if(STMFLASH_ReadWord(addrx)!=0XFFFFFFFF)//�з�0XFFFFFFFF�ĵط�,Ҫ�����������
			{   
				status=FLASH_EraseSector(STMFLASH_GetFlashSector(addrx),VoltageRange_3);//VCC=2.7~3.6V֮��!!
				printf("%s:EraseSector ->sector=%d\r\n", __func__, STMFLASH_GetFlashSector(addrx));
				if(status!=FLASH_COMPLETE)break;	//����������
			}else addrx+=4;
		} 
	}
*/	
	if(status==FLASH_COMPLETE)
	{
		while(WriteAddr<endaddr)//д����
		{
			if(FLASH_ProgramWord(WriteAddr,*pBuffer)!=FLASH_COMPLETE)//д������
			{ 
				break;	//д���쳣
			}
			STMFLASH_Read(WriteAddr, &readValue, 1);
			if(readValue != *pBuffer) break;
			
			WriteAddr+=4;
			pBuffer++;
		} 
	}
  FLASH_DataCacheCmd(ENABLE);	//FLASH��������,�������ݻ���
	FLASH_Lock();//����
	return WriteAddr;
} 

//��ָ����ַ��ʼ����ָ�����ȵ�����
//ReadAddr:��ʼ��ַ
//pBuffer:����ָ��
//NumToRead:��(4λ)��
void STMFLASH_Read(u32 ReadAddr,u32 *pBuffer,u32 NumToRead)   	
{
	u32 i;
	for(i=0;i<NumToRead;i++)
	{
		pBuffer[i]=STMFLASH_ReadWord(ReadAddr);//��ȡ4���ֽ�.
		ReadAddr+=4;//ƫ��4���ֽ�.	
	}
}

/**
  * @brief  This function does an erase of all user flash area
  * @param  StartSector: start of user flash area
  * @retval 0: user flash area successfully erased
  *         1: error occurred
  */
unsigned int FLASH_If_Erase_Sector( unsigned int StartSector )
{
  unsigned int UserStartSector = FLASH_Sector_1, i = 0;
	
	//printf("%s\r\n", __func__);
	FLASH_Unlock();									//���� 
  FLASH_DataCacheCmd( DISABLE );//FLASH�����ڼ�,�����ֹ���ݻ���
	
  /* Get the sector where start the user flash area */
  UserStartSector = STMFLASH_GetFlashSector( StartSector );
  for( i = 0; i <= 10; i++ )
  {
    /* Device voltage range supposed to be [2.7V to 3.6V], the operation will
       be done by word */ 
    if( FLASH_EraseSector( UserStartSector, VoltageRange_3 ) != FLASH_COMPLETE )
    {
      /* Error occurred while page erase */
			printf("%s: addr=0X%X, sector=%d, error\r\n", __func__, StartSector, UserStartSector);
    }
		else 
		{
			//printf("%s: addr=0X%X,sector=%d, success\r\n", __func__, StartSector, UserStartSector);
			FLASH_DataCacheCmd( ENABLE );	//FLASH��������,�������ݻ���
			FLASH_Lock();//����
			//printf("%s out.\r\n", __func__);
			printf("%s: addr=0X%X success!\r\n", __func__, StartSector);
			return 0;
		}
  }
	
  FLASH_DataCacheCmd(ENABLE);	//FLASH��������,�������ݻ���
	FLASH_Lock();//���� 
  return ( 1 );
}



/************************************************************************************
��һ����¼������¼bootlader�� sector0, ��¼rom������6

#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//����0��ʼ��ַ, 16 Kbytes    
------->���bootloader�������������rom����־��
���true���������9 copy������6��ɾ�����±�־��Ȼ����������6���У�����ֱ�Ӵ�����6���С�

#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//����1��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//����2��ʼ��ַ, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//����3��ʼ��ַ, 16 Kbytes    
------->����Ƿ������rom���ı�־�� �Լ�rom����С��

#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//����4��ʼ��ַ, 64 Kbytes    
------->BOOTLOADER_ADDRESS ���bootloader��bin�̼�

#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//����5��ʼ��ַ, 128 Kbytes  







ֻҪ���벻����256KB,Ҳ���Ǳ�����bin�ļ�������256KB, Ӧ�þͲ����д sector6 and sector7,
��Ϊ��sector6 sector7 �����洢cc936.c���������ݡ�

#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//����6��ʼ��ַ, 128 Kbytes   
------->���mcu��rom���� android�и���rom����ʱ����
rom copy�� sector9, ������ɺ��޸�rom���±�־�� Ȼ�������reset->SCU_RESET_Force();

#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//����7��ʼ��ַ, 128 Kbytes  
									STM32F407VET6 END ADDR_FLASH_SECTOR_7!!!!






#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//����8��ʼ��ַ, 128 Kbytes  
#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//����9��ʼ��ַ, 128 Kbytes   
------->��sector9-sector11����Ŵ�android���µ�rom��

#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//����10��ʼ��ַ,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//����11��ʼ��ַ,128 Kbytes  

************************************************************************************/









