#include "adc.h"
#include "delay.h"		
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

QueueHandle_t xAdcQueue =  NULL;

//��ʼ��ADC
//�������ǽ��Թ���ͨ��Ϊ��
//�����¶ȴ�����ͨ��																   
void  Adc_Init(void)
{    
  NVIC_InitTypeDef NVIC_InitStructure ;	
  GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef       ADC_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//ʹ��GPIOAʱ��
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);//ʹ��ADC1ʱ��

  //�ȳ�ʼ��IO��
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//ģ������
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;// ����
  GPIO_Init(GPIOA, &GPIO_InitStructure);//��ʼ��  
 
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);	//ADC1��λ
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);	//��λ����	 
 
  ADC_TempSensorVrefintCmd(ENABLE);//ʹ���ڲ��¶ȴ�����
	
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;//����ģʽ
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMAʧ��
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; //ADCCLK=PCLK2/4=84/4=21Mhz,ADCʱ����ò�Ҫ����36Mhz
  ADC_CommonInit(&ADC_CommonInitStructure);
	
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12λģʽ
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;//��ɨ��ģʽ	
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//��ֹ������⣬ʹ���������
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//�Ҷ���	
  ADC_InitStructure.ADC_NbrOfConversion = 1;//1��ת���ڹ��������� Ҳ����ֻת����������1 
  ADC_Init(ADC1, &ADC_InitStructure);
 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_480Cycles );	//ADC5,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��		
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_480Cycles );	//ADC16,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��		
	ADC_Cmd(ADC1, ENABLE);//����ADת����	 		

  ADC_ITConfig(ADC1, ADC_IT_EOC, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = ADC_IRQn;           //?????ADC_IRQn
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //?????? 1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;    //?????? 2
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //??????
	NVIC_Init(&NVIC_InitStructure);	
	
	//DMA_Config();	
	xAdcQueue = xQueueCreate( 3, sizeof( unsigned short ) );
	if( xAdcQueue )
	{
		printf("init xAdcQueue success...\r\n");
	}
	else
	{
		printf("init xAdcQueue fail...\r\n");
	}
}				  

void ADC_IRQHandler(void)
{
	unsigned short value;
	
	(void) vPortEnterCritical();
	
	 if( ADC_GetITStatus( ADC1, ADC_IT_EOC ) == SET )
	 {
			ADC_ClearITPendingBit( ADC1, ADC_IT_EOC );
		  value = ADC_GetConversionValue( ADC1 );
		  xQueueSendToBackFromISR( xAdcQueue, &value, NULL );
			//printf("Adc transfer finish value=%d!!\r\n", value);
	 }
	 
	(void) vPortExitCritical();		 
}

__IO uint16_t ADCoverVaule;

/*************************************************
Function:    void DMA_Config(void)  
Description: DMA????                  
Input: ?????                      
Output:?                              
Return:?                              
*************************************************/
void DMA_Config(void)
{
  NVIC_InitTypeDef NVIC_InitStructure ;	
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);

	DMA_DeInit(DMA2_Stream0);
	DMA_StructInit( &DMA_InitStructure);
	DMA_InitStructure.DMA_Channel = DMA_Channel_2;           //??Channel_2
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR; //??????????,????
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t)&ADCoverVaule;  //?????????????,??????32?
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;    //??????????????

	DMA_InitStructure.DMA_BufferSize = 1;                      //???????1,???????,???????????????????
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; //?????????????,????????DR?????
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Disable;    //?????????

	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; //???????,??ADC6_DR??????16?,??HalfWord

	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;  //????Byte
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;       //DMA?????Circular,??????
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; //????High

	DMA_Init(DMA2_Stream0, &DMA_InitStructure);
	DMA_Cmd(DMA2_Stream0, ENABLE);      //??DMA2_Stream0??
	/* DMA??? */
	//DMA_ITConfig(DMA2_Stream6, DMA_IT_TC, ENABLE);
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;   //?????DMA2_Stream0_IRQn
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //?????? 1
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;    //?????? 0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;     //??????
	NVIC_Init(&NVIC_InitStructure);	
}

void DMA2_Stream0_IRQHandler(void)  
{   
	(void) vPortEnterCritical();
	
		if( DMA_GetITStatus( DMA2_Stream0, DMA_IT_TCIF0 ) != RESET )
		{		
				DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);
				//DMA_Cmd(DMA2_Stream0, DISABLE);
		}
		//printf("%s: ADCoverVaule=%d\r\n", __func__, ADCoverVaule);
	(void) vPortExitCritical();		
}  

//���ADCֵ
//ch:ͨ��ֵ @ref ADC_channels  0~16��ADC_Channel_0~ADC_Channel_16
//����ֵ:ת�����
u16 Get_Adc(u8 ch)   
{
	static unsigned short value = 0, tmp;
	 //����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );	//ADC1,ADCͨ��,480������,��߲���ʱ�������߾�ȷ��			    
  
	ADC_SoftwareStartConv(ADC1);		//ʹ��ָ����ADC1�����ת����������	
	 
	//while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������
	tmp = value;
	/*sleep for wait ADC finish.*/
	if( xQueueReceive( xAdcQueue, &value, 200 / portTICK_PERIOD_MS /*portMAX_DELAY*/ ) ) 
	{
		return value;
	}
	else 
	{ 
		printf("%s-%d xQueueReceive error!\r\n", __func__,  __LINE__);
		/*return last success value*/
		return tmp;
	}
}
//��ȡͨ��ch��ת��ֵ��ȡtimes��,Ȼ��ƽ�� 
//ch:ͨ�����
//times:��ȡ����
//����ֵ:ͨ��ch��times��ת�����ƽ��ֵ
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);//��ȡͨ��ת��ֵ
		//delay_ms(5);
		vTaskDelay(5 / portTICK_RATE_MS);
	}
	return temp_val/times;
} 

//�õ��¶�ֵ
//����ֵ:�¶�ֵ(������100��,��λ:��.)
short Get_Temprate(void)
{
	u32 adcx;
	short result;
 	double temperate;
	adcx=Get_Adc_Average( ADC_Channel_16, 10 );	//��ȡͨ��16�ڲ��¶ȴ�����ͨ��,10��ȡƽ��
	temperate=(float)adcx*( 3.3/4096 );		//��ѹֵ
	temperate=( temperate-0.76 )/0.0025 + 25; //ת��Ϊ�¶�ֵ 
	result=temperate*= 100;					//����100��.
	return result;
}
	 










