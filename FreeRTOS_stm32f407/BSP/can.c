#include "can.h" 
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "rfifo.h"
#include <stdio.h>

extern Ringfifo 					canfifo;
extern TaskHandle_t 			pxCanTask;
////////////////////////////////////////////////////////////////////////////////// 	 

uint16_t Can1IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};

uint16_t Can2IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};



Filter_config Fitler_Config={
         
  	(sizeof(Can1IdFiter)/sizeof(uint16_t)),//1y??can1��?id??��y��?��??��2?2��1y4*14=56
	 Can1IdFiter,//1y??��?ido?
	(sizeof(Can2IdFiter)/sizeof(uint16_t)),//1y??can2��?id??��y��?��??��2?2��1y4*14=56
	 Can2IdFiter,
	 TurnOn//??2���䨰?a
};

//����1228????2��?�¡�����?0-13��?????fifi0 ��?14-27��?????fifi1
//��������CAN2��???2������??��??��?YCAN1���䨦��??��??����?3?��??����?����o��??2��??D������??��?��?
//??2��?y��������??����??0��?����??����㨹����??1��?
uint8_t CAN_FilterSet(Filter_config *fitler_cfg)
{
	uint8_t i,j=0;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	if(fitler_cfg->filter_switch==TurnOn)//??2��?�¡䨰?a
	{
			CAN1->FMR |= 1;//1y???�¡���1�����?��3?��??��?�꨺?    
			CAN1->FMR &= 0xffffc0ff;//can2��?1y???�¡䨮14?a��? 
			CAN1->FMR |= (14<<8); 
			CAN1->FFA1R = 0x0fffc000;//0--13o?1y??����1?��a��?fifi0��?14--271?��a��?fifi1
			if(fitler_cfg->sum_can1_id<=56&&fitler_cfg->sum_can1_id<=56)
			{	
					for(i=0,j=0;i<fitler_cfg->sum_can1_id;i++)
					{
							CAN1->FM1R |= (1<<i);//1y???�¡���i��???��??�¡���1�����?����D����?�꨺?,��???1y??��������??????������?????2��id  
																	 //???��?a16??��?2??32??��??a????16????��??�¡�?1y??                                                
							CAN1->FA1R &= ~(1<<i);//??��?1y??����i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;   
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)
							 {
									CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									CAN1->FA1R |= (1<<i);
									break;
							 }//��1?��1y???�¡���i			
							fitler_cfg->can1_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//��1?��1y???�¡���i	
							fitler_cfg->can1_id++;  
							CAN1->sFilterRegister[i].FR2 &= 0x00000000; 
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//��1?��1y???�¡���i	
							fitler_cfg->can1_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//��1?��1y???�¡���i						
							fitler_cfg->can1_id++; 
							
							CAN1->FA1R |= (1<<i);//��1?��1y???�¡���i   
					}
					
					
					for(i=14,j=14;i<fitler_cfg->sum_can2_id+14;i++)
					{
							CAN1->FM1R |= (1<<i);//1y???�¡���i��???��??�¡���1�����?����D����?�꨺?  
																	 //???��?a16??��?2??32??��??a????16????��??�¡�?1y??                                                
							CAN1->FA1R &= ~(1<<i);//??��?1y??����i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;  
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
								 CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
								 CAN1->FA1R |= (1<<i);
								 break;
							 }//��1?��1y???�¡���i			
							fitler_cfg->can2_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//��1?��1y???�¡���i	
							fitler_cfg->can2_id++; 
							CAN1->sFilterRegister[i].FR2 &= 0x00000000;  					
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//��1?��1y???�¡���i	
							fitler_cfg->can2_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//��1?��1y???�¡���i						
							fitler_cfg->can2_id++; 
							
							CAN1->FA1R |= (1<<i);//��1?��1y???�¡���i   
					}

				}
				else return 1;
				
				CAN1->FMR &= ~1; //1y???��?y3��1�����
	}	
	else//?����??����Did
	{	
		    /* ����??CAN ��????��0 */
				CAN_FilterInitStructure.CAN_FilterNumber = 0;		/*  ��????��D��o?��?0-13��?1214????2��?�� */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* ��????��?�꨺?��?����??ID?��???�꨺? */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32??��????�� */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* ?��??o��ID��???16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* ?��??o��ID��?�̨�16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID?��???��??16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID?��???�̨̦�16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;		/* ��????�¡㨮?��FIFO 0 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* ��1?����????�� */
				CAN_FilterInit(&CAN_FilterInitStructure);
					
				/* ����??CAN��????��D��o?14 ��?CAN2��????��D��o? 14--27��???CAN1��?��?0--13*/
				CAN_FilterInitStructure.CAN_FilterNumber = 16;						/* ��????��D��o?��?14-27��?1214????2��?�� */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* ��????��?�꨺?��?����??ID?��???�꨺? */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32??��????�� */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* ?��??o��ID��???16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* ?��??o��ID��?�̨�16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID?��???��??16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID?��???�̨̦�16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO1;		/*  ��????�¡㨮?��FIFO 1 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* ��1?����????�� */
				CAN_FilterInit(&CAN_FilterInitStructure);	
	  }
    return 0;

}


u16 CAN1_RX_STA=0;

u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
//  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //��1?��?��1?����?��
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//��1?��PORTA����?��	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//��1?��CAN1����?��	
	
    //3?��??��GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9| GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?�䨮?1|?��
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//��?������?3?
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//��?��-
    GPIO_Init(GPIOB, &GPIO_InitStructure);//3?��??��PA11,PA12
	
	  //��y???�䨮?��3��?????
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_CAN1); //GPIOA11?�䨮??aCAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_CAN1); //GPIOA12?�䨮??aCAN1
	  
  	//CAN�̣�?a����??
		CAN_DeInit(CAN1); //lex
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//��?����??�䣤���騪��D??�꨺?   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//����?t��??����???1������	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//?��???�꨺?����1y����?t??D?(??3yCAN->MCR��?SLEEP??)
  	CAN_InitStructure.CAN_NART=DISABLE;	//???1����??��??����??�� 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//����??2????��,D?��??2???����?  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//��??��??��������??������?��????�� 
  	CAN_InitStructure.CAN_Mode= mode;	 //?�꨺?����?? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??D?��?2?��????��?��(Tsjw)?atsjw+1??����??�̣�?? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1��??��CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2��??��CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //��??��?�̨�y(Fdiv)?abrp+1	
  	CAN_Init(CAN1, &CAN_InitStructure);   // 3?��??��CAN1 
    
		//????1y???��
/*		
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //1y???��0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32?? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32??ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32??MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//1y???��01?��a��?FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?��??1y???��0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??2��?��3?��??��
*/
		CAN_FilterSet(&Fitler_Config);
		
#if CAN1_RX0_INT_ENABLE
	
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0???��1��o??D???��D��.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     // ?�¨�??��???a1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // ��?��??��???a0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN1_RX0_INT_ENABLE	//��1?��RX0?D??
//?D??��t??o����y		
long num_can1_IRQ = 0;

void CAN1_RX0_IRQHandler(void)
{
	CanRxMsg RxMessage;
	struct rfifo *fifo = &canfifo;
	num_can1_IRQ++;

	(void) vPortEnterCritical();
	
	CAN_Receive(CAN1, 0, &RxMessage);
	while( fifo->size - rfifo_len( fifo ) < sizeof(unsigned int) + sizeof(unsigned char)*8 )
	{

		printf("can fifo error!\r\n");
		(void) vPortExitCritical();				
		return;
	}

	if (RxMessage.IDE == CAN_Id_Standard)
	{
		rfifo_put(fifo, &RxMessage.StdId, sizeof(unsigned int));
		rfifo_put(fifo, RxMessage.Data, sizeof(unsigned char)*8);
		if( pxCanTask )
		{
			vTaskNotifyGiveFromISR( pxCanTask, NULL );		
			
		}			
		
		//xSemaphoreGiveFromISR(xDownStreamSemaphore, NULL);
	}	
	
	(void) vPortExitCritical();				

}
#endif


u8 CAN1_Send_Msg(u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=0;		  // ��1��?��??1������?��?
  TxMessage.RTR=0;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=0;		  // ��1��?��??1������?��?
  TxMessage.RTR=0;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=id;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=ide;		  // ��1��?��??1������?��?
  TxMessage.RTR=rtr;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}
//can?��?����?��y?Y2��?��
//buf:��y?Y?o��???;	 
//����???��:0,?T��y?Y��?��?��?;
//		 ????,?����?��?��y?Y3��?��;
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//??��D?����?��?��y?Y,?��?����?3? 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//?����?��y?Y	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}

/************************************CAN2****************************************************/
/*************************************CAN2***************************************************/
/*************************************CAN2***************************************************/
/************************************CAN2****************************************************/
/***********************************CAN2*CAN2************************************************/
/************************************CAN2****************************************************/
/************************************CAN2****************************************************/
/************************************CAN2****************************************************/
/*************************************CAN2***************************************************/

u16 CAN2_RX_STA=0;

u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
//  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN2_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //��1?��?��1?����?��
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  //��1?��PORTB����?��	 PB5=CAN2_RX    PB6=CAN2_TX                  											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);//��1?��CAN2����?��	
	
    //3?��??��GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6; 
	  //GPIO_Pin_5 GPIO_Pin_6| GPIO_Pin_12 GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?�䨮?1|?��
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//��?������?3?
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//��?��-
    GPIO_Init(GPIOB, &GPIO_InitStructure);//3?��??��PA11,PA12
	
	  //��y???�䨮?��3��?????
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); //GPIOA5?�䨮??aCAN2
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2); //GPIOA12?�䨮??aCAN2
	  
  	//CAN�̣�?a����??
		CAN_DeInit(CAN2);
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//��?����??�䣤���騪��D??�꨺?   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//����?t��??����???1������	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//?��???�꨺?����1y����?t??D?(??3yCAN->MCR��?SLEEP??)
  	CAN_InitStructure.CAN_NART=DISABLE;	//???1����??��??����??�� ///////LEXLEXLEX
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//����??2????��,D?��??2???����?  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//��??��??��������??������?��????�� 
  	CAN_InitStructure.CAN_Mode= mode;	 //?�꨺?����?? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??D?��?2?��????��?��(Tsjw)?atsjw+1??����??�̣�?? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1��??��CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2��??��CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //��??��?�̨�y(Fdiv)?abrp+1	
  	if(CAN_InitStatus_Success == CAN_Init(CAN2, &CAN_InitStructure))
			;//printf("init can2: CAN_Init success.\r\n");   // 3?��??��CAN1 
		else 
			printf("error -> init can2: CAN_Init fail.\r\n");   // 3?��??��CAN1 
    
		//????1y???�� ��?����1ycan1���䨦��??��?��?????1�����?�꨺?��?2����??���̨�?����??�¡�?����??
		//��D��???can?????�¡�?can1?�¡�? can2�䨮��??????????�¨�D��y??����?������??��?��???fifo
		//????fifo��D��y???����?����???��
/* 	  CAN_FilterInitStructure.CAN_FilterNumber=16;	  //1y???��0   ��??����a�䨮����14
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32?? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32??ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32??MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//1y???��01?��a��?FIFO0  //CAN_Filter_FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?��??1y???��0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??2��?��3?��??��
*/		
#if CAN2_RX1_INT_ENABLE
	
	  CAN_ITConfig(CAN2, CAN_IT_FMP1/*|CAN_IT_FF1|CAN_IT_FOV1*/, ENABLE);//FIFO1???��1��o??D???��D��.		    //CAN_IT_FMP0 
    //CAN_IT_FMP1 FIFO 1 message pending Interrupt
		
		//NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);//add by lex
		//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;  //CAN2_RX0_IRQn ??��|FIFO1
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     // ?�¨�??��???a1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;            // ��?��??��???a0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN2_RX1_INT_ENABLE	//��1?��RX0?D??
//?D??��t??o����y			  
/*	
	printf("\r\n");
	printf("%s->RxMessage.StdId=0x%03x, RxMessage.IDE=0x%x, RxMessage.RTR=0x%x\r\n", 
		__func__, RxMessage.StdId, RxMessage.IDE, RxMessage.RTR);
	//RTR = 1 ������???3��??
	//IDE = 1 ������?��??1??
	//StdId   ID 11bit or 29bit
	printf("data len=%d\r\n", RxMessage.DLC);//data len
	
	for(i=0;i<8;i++)
		printf("%d ", RxMessage.Data[i]);
	printf("\r\n");
*/

long num_can2_IRQ = 0;

void CAN2_RX1_IRQHandler(void) //CAN2_RX0_IRQHandler
{
	CanRxMsg RxMessage;

	struct rfifo *fifo = &canfifo;
	num_can2_IRQ++;

	(void) vPortEnterCritical();
	
	CAN_Receive(CAN2, CAN_FIFO1, &RxMessage);
	
	while( fifo->size - rfifo_len( fifo ) < sizeof(unsigned int) + sizeof(unsigned char)*8 )
	{
		printf("can fifo error!\r\n");
		(void) vPortExitCritical();				
		return;
	}

	if (RxMessage.IDE == CAN_Id_Standard)
	{
		rfifo_put(fifo, &RxMessage.StdId, sizeof(unsigned int));
		rfifo_put(fifo, RxMessage.Data, sizeof(unsigned char)*8);
		if( pxCanTask )
		{
			vTaskNotifyGiveFromISR( pxCanTask, NULL );	
		}		
		
		//xSemaphoreGiveFromISR(xDownStreamSemaphore, NULL);
	}	

	(void) vPortExitCritical();				
		
}
#endif

//can����?����?������y?Y(1��?��??��?:ID?a0X12,������???,��y?Y??)	
//len:��y?Y3��?��(��?�䨮?a8)				     
//msg:��y?Y????,��?�䨮?a8??��??��.
//����???��:0,3��1|;
//		 ????,����㨹;
u8 CAN2_Send_Msg(u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=0;		  // ��1��?��??1������?��?
  TxMessage.RTR=0;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=0;		  // ��1��?��??1������?��?
  TxMessage.RTR=0;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:������?ID(11??)/��??1ID(11??+18??)	    
//ide:0,������???;1,��??1??
//rtr:0,��y?Y??;1,??3��??
//len:��y?Y3��?��
//msg:��y?Y?o��???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ������?������?��??a0 			11bit   0x712
  TxMessage.ExtId=id;	 // ����??��??1������?��?�ꡧ29??��?
  TxMessage.IDE=ide;		  // ��1��?��??1������?��?
  TxMessage.RTR=rtr;		  // ???�騤��D��?a��y?Y??��?��???8??
  TxMessage.DLC=len;							 // ����?����???D??��
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // �̨���???D??��          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�̨���y����?��?����?
  if(i>=0XFFF)return 1;
  return 0;		

}
//can?��?����?��y?Y2��?��
//buf:��y?Y?o��???;	 
//����???��:0,?T��y?Y��?��?��?;
//		 ????,?����?��?��y?Y3��?��;
u8 CAN2_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN2,CAN_FIFO0)==0)return 0;		//??��D?����?��?��y?Y,?��?����?3? 
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);//?����?��y?Y	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}






