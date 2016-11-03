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
         
  	(sizeof(Can1IdFiter)/sizeof(uint16_t)),//1y??can1米?id??那y㏒?℅??角2?2迄1y4*14=56
	 Can1IdFiter,//1y??米?ido?
	(sizeof(Can2IdFiter)/sizeof(uint16_t)),//1y??can2米?id??那y㏒?℅??角2?2迄1y4*14=56
	 Can2IdFiter,
	 TurnOn//??2“∩辰?a
};

//℅邦1228????2“?¯℅谷㏒?0-13﹞?????fifi0 ㏒?14-27﹞?????fifi1
//車谷車迆CAN2米???2“谷豕??那??迄?YCAN1角∩谷豕??㏒??迄辰?3?那??‘米?那㊣o辰??2“??D豕谷豕??辰?∩?
//??2“?y豕﹞谷豕??﹞米??0㏒?谷豕??那∫～邦﹞米??1㏒?
uint8_t CAN_FilterSet(Filter_config *fitler_cfg)
{
	uint8_t i,j=0;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	if(fitler_cfg->filter_switch==TurnOn)//??2“?¯∩辰?a
	{
			CAN1->FMR |= 1;//1y???¯℅谷1∟℅¯?迆3?那??‘?㏒那?    
			CAN1->FMR &= 0xffffc0ff;//can2米?1y???¯∩車14?a那? 
			CAN1->FMR |= (14<<8); 
			CAN1->FFA1R = 0x0fffc000;//0--13o?1y??℅谷1?芍a米?fifi0㏒?14--271?芍a米?fifi1
			if(fitler_cfg->sum_can1_id<=56&&fitler_cfg->sum_can1_id<=56)
			{	
					for(i=0,j=0;i<fitler_cfg->sum_can1_id;i++)
					{
							CAN1->FM1R |= (1<<i);//1y???¯℅谷i米???∩??¯℅谷1∟℅¯?迆芍D㊣赤?㏒那?,辰???1y??℅谷谷豕??????㊣那℅?????2“id  
																	 //???赤?a16??㏒?2??32??﹞??a????16????∩??¯㏒?1y??                                                
							CAN1->FA1R &= ~(1<<i);//??車?1y??℅谷i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;   
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)
							 {
									CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									CAN1->FA1R |= (1<<i);
									break;
							 }//那1?邦1y???¯℅谷i			
							fitler_cfg->can1_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//那1?邦1y???¯℅谷i	
							fitler_cfg->can1_id++;  
							CAN1->sFilterRegister[i].FR2 &= 0x00000000; 
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//那1?邦1y???¯℅谷i	
							fitler_cfg->can1_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//那1?邦1y???¯℅谷i						
							fitler_cfg->can1_id++; 
							
							CAN1->FA1R |= (1<<i);//那1?邦1y???¯℅谷i   
					}
					
					
					for(i=14,j=14;i<fitler_cfg->sum_can2_id+14;i++)
					{
							CAN1->FM1R |= (1<<i);//1y???¯℅谷i米???∩??¯℅谷1∟℅¯?迆芍D㊣赤?㏒那?  
																	 //???赤?a16??㏒?2??32??﹞??a????16????∩??¯㏒?1y??                                                
							CAN1->FA1R &= ~(1<<i);//??車?1y??℅谷i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;  
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
								 CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
								 CAN1->FA1R |= (1<<i);
								 break;
							 }//那1?邦1y???¯℅谷i			
							fitler_cfg->can2_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//那1?邦1y???¯℅谷i	
							fitler_cfg->can2_id++; 
							CAN1->sFilterRegister[i].FR2 &= 0x00000000;  					
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//那1?邦1y???¯℅谷i	
							fitler_cfg->can2_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//那1?邦1y???¯℅谷i						
							fitler_cfg->can2_id++; 
							
							CAN1->FA1R |= (1<<i);//那1?邦1y???¯℅谷i   
					}

				}
				else return 1;
				
				CAN1->FMR &= ~1; //1y???¯?y3㏒1∟℅¯
	}	
	else//?車那??迄車Did
	{	
		    /* 谷豕??CAN 谷????¯0 */
				CAN_FilterInitStructure.CAN_FilterNumber = 0;		/*  谷????¯D辰o?㏒?0-13㏒?1214????2“?¯ */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* 谷????¯?㏒那?㏒?谷豕??ID?迆???㏒那? */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32??谷????¯ */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* ?迆??o車ID米???16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* ?迆??o車ID米?米赤16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID?迆???米??16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID?迆???米米赤16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;		/* 谷????¯～車?“FIFO 0 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* 那1?邦谷????¯ */
				CAN_FilterInit(&CAN_FilterInitStructure);
					
				/* 谷豕??CAN谷????¯D辰o?14 ㏒?CAN2谷????¯D辰o? 14--27㏒???CAN1米?那?0--13*/
				CAN_FilterInitStructure.CAN_FilterNumber = 16;						/* 谷????¯D辰o?㏒?14-27㏒?1214????2“?¯ */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* 谷????¯?㏒那?㏒?谷豕??ID?迆???㏒那? */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32??谷????¯ */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* ?迆??o車ID米???16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* ?迆??o車ID米?米赤16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID?迆???米??16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID?迆???米米赤16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO1;		/*  谷????¯～車?“FIFO 1 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* 那1?邦谷????¯ */
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
    //那1?邦?角1?那㊣?車
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//那1?邦PORTA那㊣?車	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//那1?邦CAN1那㊣?車	
	
    //3?那??‘GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9| GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?∩車?1|?邦
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//赤?赤足那?3?
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//谷?角-
    GPIO_Init(GPIOB, &GPIO_InitStructure);//3?那??‘PA11,PA12
	
	  //辰y???∩車?車3谷?????
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_CAN1); //GPIOA11?∩車??aCAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_CAN1); //GPIOA12?∩車??aCAN1
	  
  	//CAN米ㄓ?a谷豕??
		CAN_DeInit(CAN1); //lex
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//﹞?那㊣??∩ㄓ﹞⊿赤“D??㏒那?   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//豕赤?t℅??‘角???1邦角赤	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//?‘???㏒那?赤“1y豕赤?t??D?(??3yCAN->MCR米?SLEEP??)
  	CAN_InitStructure.CAN_NART=DISABLE;	//???1㊣“??℅??‘∩??赤 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//㊣“??2????“,D?米??2???谷米?  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//車??豕??車谷㊣“??㊣那那?﹞????“ 
  	CAN_InitStructure.CAN_Mode= mode;	 //?㏒那?谷豕?? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??D?赤?2?足????赤?豕(Tsjw)?atsjw+1??那㊣??米ㄓ?? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1﹞??∫CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2﹞??∫CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //﹞??米?米那y(Fdiv)?abrp+1	
  	CAN_Init(CAN1, &CAN_InitStructure);   // 3?那??‘CAN1 
    
		//????1y???¯
/*		
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //1y???¯0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32?? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32??ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32??MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//1y???¯01?芍a米?FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?∟??1y???¯0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??2“?¯3?那??‘
*/
		CAN_FilterSet(&Fitler_Config);
		
#if CAN1_RX0_INT_ENABLE
	
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0???⊿1辰o??D???那D赤.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     // ?¯車??豕???a1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // ∩?車??豕???a0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN1_RX0_INT_ENABLE	//那1?邦RX0?D??
//?D??﹞t??o‘那y		
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
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=0;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=0;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=0;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=0;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=id;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=ide;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=rtr;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}
//can?迆?車那?那y?Y2谷?‘
//buf:那y?Y?o∩???;	 
//﹞米???米:0,?T那y?Y㊣?那?米?;
//		 ????,?車那?米?那y?Y3∟?豕;
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//??車D?車那?米?那y?Y,?㊣?車赤?3? 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//?芍豕?那y?Y	
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
    //那1?邦?角1?那㊣?車
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  //那1?邦PORTB那㊣?車	 PB5=CAN2_RX    PB6=CAN2_TX                  											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);//那1?邦CAN2那㊣?車	
	
    //3?那??‘GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6; 
	  //GPIO_Pin_5 GPIO_Pin_6| GPIO_Pin_12 GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//?∩車?1|?邦
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//赤?赤足那?3?
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//谷?角-
    GPIO_Init(GPIOB, &GPIO_InitStructure);//3?那??‘PA11,PA12
	
	  //辰y???∩車?車3谷?????
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); //GPIOA5?∩車??aCAN2
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2); //GPIOA12?∩車??aCAN2
	  
  	//CAN米ㄓ?a谷豕??
		CAN_DeInit(CAN2);
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//﹞?那㊣??∩ㄓ﹞⊿赤“D??㏒那?   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//豕赤?t℅??‘角???1邦角赤	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//?‘???㏒那?赤“1y豕赤?t??D?(??3yCAN->MCR米?SLEEP??)
  	CAN_InitStructure.CAN_NART=DISABLE;	//???1㊣“??℅??‘∩??赤 ///////LEXLEXLEX
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//㊣“??2????“,D?米??2???谷米?  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//車??豕??車谷㊣“??㊣那那?﹞????“ 
  	CAN_InitStructure.CAN_Mode= mode;	 //?㏒那?谷豕?? 
  	CAN_InitStructure.CAN_SJW=tsjw;	//??D?赤?2?足????赤?豕(Tsjw)?atsjw+1??那㊣??米ㄓ?? CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1﹞??∫CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2﹞??∫CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //﹞??米?米那y(Fdiv)?abrp+1	
  	if(CAN_InitStatus_Success == CAN_Init(CAN2, &CAN_InitStructure))
			;//printf("init can2: CAN_Init success.\r\n");   // 3?那??‘CAN1 
		else 
			printf("error -> init can2: CAN_Init fail.\r\n");   // 3?那??‘CAN1 
    
		//????1y???¯ 那?赤“1ycan1角∩谷豕??米?㏒?????1∟℅¯?㏒那?㏒?2“足??那米豕?谷辰??¯℅?谷豕??
		//車D芍???can?????¯㏒?can1?¯㏒? can2∩車㏒??????????¯車D豕y??﹞⊿?赤車那??㏒?芍???fifo
		//????fifo車D豕y???車那?車那???㏒
/* 	  CAN_FilterInitStructure.CAN_FilterNumber=16;	  //1y???¯0   辰??“辰a∩車車迆14
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32?? 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32??ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32??MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//1y???¯01?芍a米?FIFO0  //CAN_Filter_FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //?∟??1y???¯0
  	CAN_FilterInit(&CAN_FilterInitStructure);//??2“?¯3?那??‘
*/		
#if CAN2_RX1_INT_ENABLE
	
	  CAN_ITConfig(CAN2, CAN_IT_FMP1/*|CAN_IT_FF1|CAN_IT_FOV1*/, ENABLE);//FIFO1???⊿1辰o??D???那D赤.		    //CAN_IT_FMP0 
    //CAN_IT_FMP1 FIFO 1 message pending Interrupt
		
		//NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);//add by lex
		//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;  //CAN2_RX0_IRQn ??車|FIFO1
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;     // ?¯車??豕???a1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;            // ∩?車??豕???a0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN2_RX1_INT_ENABLE	//那1?邦RX0?D??
//?D??﹞t??o‘那y			  
/*	
	printf("\r\n");
	printf("%s->RxMessage.StdId=0x%03x, RxMessage.IDE=0x%x, RxMessage.RTR=0x%x\r\n", 
		__func__, RxMessage.StdId, RxMessage.IDE, RxMessage.RTR);
	//RTR = 1 ㊣赤那???3足??
	//IDE = 1 ㊣赤那?角??1??
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

//can﹞⊿?赤辰?℅谷那y?Y(1足?“??那?:ID?a0X12,㊣那℅???,那y?Y??)	
//len:那y?Y3∟?豕(℅?∩車?a8)				     
//msg:那y?Y????,℅?∩車?a8??℅??迆.
//﹞米???米:0,3谷1|;
//		 ????,那∫～邦;
u8 CAN2_Send_Msg(u8* msg,u8 len)
{	
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=0;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=0;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=0;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=0;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:㊣那℅?ID(11??)/角??1ID(11??+18??)	    
//ide:0,㊣那℅???;1,角??1??
//rtr:0,那y?Y??;1,??3足??
//len:那y?Y3∟?豕
//msg:那y?Y?o∩???	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ㊣那℅?㊣那那?﹞??a0 			11bit   0x712
  TxMessage.ExtId=id;	 // 谷豕??角??1㊣那那?﹞?㏒“29??㏒?
  TxMessage.IDE=ide;		  // 那1車?角??1㊣那那?﹞?
  TxMessage.RTR=rtr;		  // ???⊿角角D赤?a那y?Y??㏒?辰???8??
  TxMessage.DLC=len;							 // ﹞⊿?赤芍???D??⊿
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 米迆辰???D??⊿          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//米豕∩y﹞⊿?赤?芍那?
  if(i>=0XFFF)return 1;
  return 0;		

}
//can?迆?車那?那y?Y2谷?‘
//buf:那y?Y?o∩???;	 
//﹞米???米:0,?T那y?Y㊣?那?米?;
//		 ????,?車那?米?那y?Y3∟?豕;
u8 CAN2_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN2,CAN_FIFO0)==0)return 0;		//??車D?車那?米?那y?Y,?㊣?車赤?3? 
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);//?芍豕?那y?Y	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}






