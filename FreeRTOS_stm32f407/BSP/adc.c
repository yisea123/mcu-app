#include "adc.h"
#include "delay.h"		
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include <stdio.h>

QueueHandle_t xAdcQueue =  NULL;

//初始化ADC
//这里我们仅以规则通道为例
//开启温度传感器通道																   
void  Adc_Init(void)
{    
  NVIC_InitTypeDef NVIC_InitStructure ;	
  GPIO_InitTypeDef  GPIO_InitStructure;
	ADC_CommonInitTypeDef ADC_CommonInitStructure;
	ADC_InitTypeDef       ADC_InitStructure;
	
  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);//使能GPIOA时钟
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);//使能ADC1时钟

  //先初始化IO口
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;//模拟输入
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;// 上拉
  GPIO_Init(GPIOA, &GPIO_InitStructure);//初始化  
 
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,ENABLE);	//ADC1复位
	RCC_APB2PeriphResetCmd(RCC_APB2Periph_ADC1,DISABLE);	//复位结束	 
 
  ADC_TempSensorVrefintCmd(ENABLE);//使能内部温度传感器
	
  ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;//独立模式
  ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
  ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled; //DMA失能
  ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div4; //ADCCLK=PCLK2/4=84/4=21Mhz,ADC时钟最好不要超过36Mhz
  ADC_CommonInit(&ADC_CommonInitStructure);
	
  ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;//12位模式
  ADC_InitStructure.ADC_ScanConvMode = DISABLE;//非扫描模式	
  ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
  ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;//禁止触发检测，使用软件触发
  ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//右对齐	
  ADC_InitStructure.ADC_NbrOfConversion = 1;//1个转换在规则序列中 也就是只转换规则序列1 
  ADC_Init(ADC1, &ADC_InitStructure);
 
	ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_480Cycles );	//ADC5,ADC通道,480个周期,提高采样时间可以提高精确度		
	ADC_RegularChannelConfig(ADC1, ADC_Channel_16, 1, ADC_SampleTime_480Cycles );	//ADC16,ADC通道,480个周期,提高采样时间可以提高精确度		
	ADC_Cmd(ADC1, ENABLE);//开启AD转换器	 		

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

//获得ADC值
//ch:通道值 @ref ADC_channels  0~16：ADC_Channel_0~ADC_Channel_16
//返回值:转换结果
u16 Get_Adc(u8 ch)   
{
	static unsigned short value = 0, tmp;
	 //设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_480Cycles );	//ADC1,ADC通道,480个周期,提高采样时间可以提高精确度			    
  
	ADC_SoftwareStartConv(ADC1);		//使能指定的ADC1的软件转换启动功能	
	 
	//while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//等待转换结束
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
//获取通道ch的转换值，取times次,然后平均 
//ch:通道编号
//times:获取次数
//返回值:通道ch的times次转换结果平均值
u16 Get_Adc_Average(u8 ch,u8 times)
{
	u32 temp_val=0;
	u8 t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);//获取通道转换值
		//delay_ms(5);
		vTaskDelay(5 / portTICK_RATE_MS);
	}
	return temp_val/times;
} 

//得到温度值
//返回值:温度值(扩大了100倍,单位:℃.)
short Get_Temprate(void)
{
	u32 adcx;
	short result;
 	double temperate;
	adcx=Get_Adc_Average( ADC_Channel_16, 10 );	//读取通道16内部温度传感器通道,10次取平均
	temperate=(float)adcx*( 3.3/4096 );		//电压值
	temperate=( temperate-0.76 )/0.0025 + 25; //转换为温度值 
	result=temperate*= 100;					//扩大100倍.
	return result;
}
	 










