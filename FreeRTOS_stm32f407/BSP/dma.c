#include "dma.h"																	   	  
#include "delay.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"


//DMAx�ĸ�ͨ������
//����Ĵ�����ʽ�ǹ̶���,���Ҫ���ݲ�ͬ��������޸�
//�Ӵ洢��->����ģʽ/8λ���ݿ���/�洢������ģʽ
//DMA_Streamx:DMA������,DMA1_Stream0~7/DMA2_Stream0~7
//chx:DMAͨ��ѡ��,@ref DMA_channel DMA_Channel_0~DMA_Channel_7
//par:�����ַ
//mar:�洢����ַ
//ndtr:���ݴ�����  
void MYDMA_Config(DMA_Stream_TypeDef *DMA_Streamx,u32 chx,u32 par,u32 mar,u16 ndtr)
{ 
  NVIC_InitTypeDef NVIC_InitStructure ;  
	DMA_InitTypeDef  DMA_InitStructure;
	
	if((u32)DMA_Streamx>(u32)DMA2)//�õ���ǰstream������DMA2����DMA1
	{
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2,ENABLE);//DMA2ʱ��ʹ�� 
		
	}
	else 
	{
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA1,ENABLE);//DMA1ʱ��ʹ�� 
	}
		
  DMA_DeInit(DMA_Streamx);
	
	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}//�ȴ�DMA������ 
	
  /* ���� DMA Stream */
  DMA_InitStructure.DMA_Channel = chx;  //ͨ��ѡ��
  DMA_InitStructure.DMA_PeripheralBaseAddr = par;//DMA�����ַ
  DMA_InitStructure.DMA_Memory0BaseAddr = mar;//DMA �洢��0��ַ
  DMA_InitStructure.DMA_DIR = DMA_DIR_MemoryToPeripheral;//�洢��������ģʽ
  DMA_InitStructure.DMA_BufferSize = ndtr;//���ݴ����� 
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;//���������ģʽ
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;//�洢������ģʽ
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;//�������ݳ���:8λ
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;//�洢�����ݳ���:8λ
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;// ʹ����ͨģʽ 
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;//�е����ȼ�
  DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Disable;         
  DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
  DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;//�洢��ͻ�����δ���
  DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;//����ͻ�����δ���
  DMA_Init(DMA_Streamx, &DMA_InitStructure);//��ʼ��DMA Stream
	
	DMA_ITConfig(DMA_Streamx, DMA_IT_TC, ENABLE);
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream6_IRQn;//DMA2_Stream7_IRQn;  DMA1_Stream3_IRQn
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;  
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;  
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;  
	NVIC_Init(&NVIC_InitStructure);  
} 
//����һ��DMA����
//DMA_Streamx:DMA������,DMA1_Stream0~7/DMA2_Stream0~7 
//ndtr:���ݴ�����  
void MYDMA_Enable(DMA_Stream_TypeDef *DMA_Streamx,u16 ndtr)
{
 
	DMA_Cmd(DMA_Streamx, DISABLE);                      //�ر�DMA���� 
	
	while (DMA_GetCmdStatus(DMA_Streamx) != DISABLE){}	//ȷ��DMA���Ա�����  
		
	DMA_SetCurrDataCounter(DMA_Streamx,ndtr);          //���ݴ�����  
 
	DMA_Cmd(DMA_Streamx, ENABLE);                      //����DMA���� 
}	  

//extern QueueHandle_t 			mDmaSemaphore; 
extern TaskHandle_t 			pxLogTask;
extern xSemaphoreHandle 		xSenderSemaphore;
void DMA2_Stream7_IRQHandler(void)  
{   
	(void) vPortEnterCritical();
	
		if( DMA_GetITStatus( DMA2_Stream7, DMA_IT_TCIF7 ) != RESET )
		{		
				DMA_ClearITPendingBit(DMA2_Stream7, DMA_IT_TCIF7);
				DMA_Cmd(DMA2_Stream7, DISABLE);
				/*
			  if( mDmaSemaphore )
				{
					xSemaphoreGiveFromISR( mDmaSemaphore, NULL );
				}
				*/
				//vTaskNotifyGiveFromISR( pxLogTask, NULL);
				xSemaphoreGiveFromISR(xSenderSemaphore, NULL);
		}

	(void) vPortExitCritical();		
}  


void DMA2_Stream6_IRQHandler(void)  
{   
	(void) vPortEnterCritical();
	
		if( DMA_GetITStatus( DMA2_Stream6, DMA_IT_TCIF6 ) != RESET )
		{		
				DMA_ClearITPendingBit(DMA2_Stream6, DMA_IT_TCIF6);
				DMA_Cmd(DMA2_Stream6, DISABLE);
				xSemaphoreGiveFromISR(xSenderSemaphore, NULL);
		}

	(void) vPortExitCritical();		
}  

void DMA1_Stream3_IRQHandler(void)  
{   
	(void) vPortEnterCritical();
	
		if( DMA_GetITStatus( DMA1_Stream3, DMA_IT_TCIF3 ) != RESET )
		{		
				DMA_ClearITPendingBit(DMA1_Stream3, DMA_IT_TCIF3);
				DMA_Cmd(DMA1_Stream3, DISABLE);
				xSemaphoreGiveFromISR(xSenderSemaphore, NULL);
		}

	(void) vPortExitCritical();		
}  




















