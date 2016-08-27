#include "can.h"
#include "led.h"
#include "delay.h"
#include "usart.h"
#include "uart_command.h"

struct kfifo* can1_fifo;
struct kfifo* can2_fifo;

////////////////////////////////////////////////////////////////////////////////// 	 

//CAN��ʼ��
//tsjw:����ͬ����Ծʱ�䵥Ԫ. @ref CAN_synchronisation_jump_width   ��Χ: ; CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:ʱ���2��ʱ�䵥Ԫ.   @ref CAN_time_quantum_in_bit_segment_2 ��Χ:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:ʱ���1��ʱ�䵥Ԫ.   @refCAN_time_quantum_in_bit_segment_1  ��Χ: ;	  CAN_BS1_1tq ~CAN_BS1_16tq
//brp :�����ʷ�Ƶ��.��Χ:1~1024;(ʵ��Ҫ��1,Ҳ����1~1024) tq=(brp)*tpclk1
//������=Fpclk1/((tsjw+tbs1+tbs2+3)*brp);
//mode: @ref CAN_operating_mode ��Χ��CAN_Mode_Normal,��ͨģʽ;CAN_Mode_LoopBack,�ػ�ģʽ;
//Fpclk1��ʱ���ڳ�ʼ����ʱ������Ϊ36M,�������CAN_Normal_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//������Ϊ:42M/((1+6+7)*6)=500Kbps
//����ֵ:0,��ʼ��OK;
//    ����,��ʼ��ʧ��;

//ʾ���������ӣ�
//�����˲���id
uint16_t Can1IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};

uint16_t Can2IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};



Filter_config Fitler_Config={
         
  (sizeof(Can1IdFiter)/sizeof(uint16_t)),//����can1��id��������಻�ٹ�4*14=56
	 Can1IdFiter,//���˵�id��
	(sizeof(Can2IdFiter)/sizeof(uint16_t)),//����can2��id��������಻�ٹ�4*14=56
	 Can2IdFiter,
	 TurnOn//�˲���
};

//�ܹ�28���˲����飬0-13�����fifi0 ��14-27�����fifi1
//����CAN2���˲������Ǹ���CAN1�����ã����Գ�ʼ����ʱ���˲�ֻ������һ��
//�˲���ȷ���÷���0������ʧ�ܷ���1��
uint8_t CAN_FilterSet(Filter_config *fitler_cfg)
{
	uint8_t i,j=0;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	if(fitler_cfg->filter_switch==TurnOn)//�˲�����
	{
			CAN1->FMR |= 1;//�������鹤���ڳ�ʼ��ģʽ    
			CAN1->FMR &= 0xffffc0ff;//can2�Ĺ�������14��ʼ 
			CAN1->FMR |= (14<<8); 
			CAN1->FFA1R = 0x0fffc000;//0--13�Ź����������fifi0��14--27������fifi1
			if(fitler_cfg->sum_can1_id<=56&&fitler_cfg->sum_can1_id<=56)
			{	
					for(i=0,j=0;i<fitler_cfg->sum_can1_id;i++)
					{
							CAN1->FM1R |= (1<<i);//��������i�ļĴ����鹤�����б�ģʽ,һ�������������ĸ���׼���˲�id  
																	 //λ��Ϊ16λ��2��32λ��Ϊ�ĸ�16λ�Ĵ���������                                                
							CAN1->FA1R &= ~(1<<i);//���ù�����i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;   
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)
							 {
									CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									CAN1->FA1R |= (1<<i);
									break;
							 }//ʹ�ܹ�������i			
							fitler_cfg->can1_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//ʹ�ܹ�������i	
							fitler_cfg->can1_id++;  
							CAN1->sFilterRegister[i].FR2 &= 0x00000000; 
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//ʹ�ܹ�������i	
							fitler_cfg->can1_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//ʹ�ܹ�������i						
							fitler_cfg->can1_id++; 
							
							CAN1->FA1R |= (1<<i);//ʹ�ܹ�������i   
					}
					
					
					for(i=14,j=14;i<fitler_cfg->sum_can2_id+14;i++)
					{
							CAN1->FM1R |= (1<<i);//��������i�ļĴ����鹤�����б�ģʽ  
																	 //λ��Ϊ16λ��2��32λ��Ϊ�ĸ�16λ�Ĵ���������                                                
							CAN1->FA1R &= ~(1<<i);//���ù�����i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;  
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
								 CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
								 CAN1->FA1R |= (1<<i);
								 break;
							 }//ʹ�ܹ�������i			
							fitler_cfg->can2_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//ʹ�ܹ�������i	
							fitler_cfg->can2_id++; 
							CAN1->sFilterRegister[i].FR2 &= 0x00000000;  					
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//ʹ�ܹ�������i	
							fitler_cfg->can2_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//ʹ�ܹ�������i						
							fitler_cfg->can2_id++; 
							
							CAN1->FA1R |= (1<<i);//ʹ�ܹ�������i   
					}

				}
				else return 1;
				
				CAN1->FMR &= ~1; //��������������
	}	
	else//��������id
	{	
		    /* ����CAN ɸѡ��0 */
				CAN_FilterInitStructure.CAN_FilterNumber = 0;		/*  ɸѡ����ţ�0-13����14���˲��� */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* ɸѡ��ģʽ������ID����ģʽ */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32λɸѡ�� */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* �����ID�ĸ�16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* �����ID�ĵ�16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID����ֵ��16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID����ֵ��16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;		/* ɸѡ����FIFO 0 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* ʹ��ɸѡ�� */
				CAN_FilterInit(&CAN_FilterInitStructure);
					
				/* ����CANɸѡ�����14 ��CAN2ɸѡ����� 14--27����CAN1����0--13*/
				CAN_FilterInitStructure.CAN_FilterNumber = 16;						/* ɸѡ����ţ�14-27����14���˲��� */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* ɸѡ��ģʽ������ID����ģʽ */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32λɸѡ�� */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* �����ID�ĸ�16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* �����ID�ĵ�16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID����ֵ��16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID����ֵ��16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO1;		/*  ɸѡ����FIFO 1 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* ʹ��ɸѡ�� */
				CAN_FilterInit(&CAN_FilterInitStructure);	
	  }
    return 0;

}




//CAN��ʼ��
//tsjw:����ͬ����Ծʱ�䵥Ԫ. @ref CAN_synchronisation_jump_width   ��Χ: ; CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:ʱ���2��ʱ�䵥Ԫ.   @ref CAN_time_quantum_in_bit_segment_2 ��Χ:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:ʱ���1��ʱ�䵥Ԫ.   @refCAN_time_quantum_in_bit_segment_1  ��Χ: ;	  CAN_BS1_1tq ~CAN_BS1_16tq
//brp :�����ʷ�Ƶ��.��Χ:1~1024;(ʵ��Ҫ��1,Ҳ����1~1024) tq=(brp)*tpclk1
//������=Fpclk1/((tsjw+tbs1+tbs2+3)*brp);
//mode: @ref CAN_operating_mode ��Χ��CAN_Mode_Normal,��ͨģʽ;CAN_Mode_LoopBack,�ػ�ģʽ;
//Fpclk1��ʱ���ڳ�ʼ����ʱ������Ϊ36M,�������CAN_Normal_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//������Ϊ:42M/((1+6+7)*6)=500Kbps
//����ֵ:0,��ʼ��OK;
//    ����,��ʼ��ʧ��;

u16 CAN1_RX_STA=0;

u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
//  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //ʹ�����ʱ��
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//ʹ��PORTAʱ��	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//ʹ��CAN1ʱ��	
	
    //��ʼ��GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9| GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
    GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PA11,PA12
	
	  //���Ÿ���ӳ������
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_CAN1); //GPIOA11����ΪCAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_CAN1); //GPIOA12����ΪCAN1
	  
  	//CAN��Ԫ����
		CAN_DeInit(CAN1); //lex
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//��ʱ�䴥��ͨ��ģʽ   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//����Զ����߹���	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//˯��ģʽͨ���������(���CAN->MCR��SLEEPλ)
  	CAN_InitStructure.CAN_NART=DISABLE;	//��ֹ�����Զ����� 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//���Ĳ�����,�µĸ��Ǿɵ�  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//���ȼ��ɱ��ı�ʶ������ 
  	CAN_InitStructure.CAN_Mode= mode;	 //ģʽ���� 
  	CAN_InitStructure.CAN_SJW=tsjw;	//����ͬ����Ծ���(Tsjw)Ϊtsjw+1��ʱ�䵥λ CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1��ΧCAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2��ΧCAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //��Ƶϵ��(Fdiv)Ϊbrp+1	
  	CAN_Init(CAN1, &CAN_InitStructure);   // ��ʼ��CAN1 
    
		//���ù�����
/*		
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //������0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32λ 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32λID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32λMASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//������0������FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //���������0
  	CAN_FilterInit(&CAN_FilterInitStructure);//�˲�����ʼ��
*/
		CAN_FilterSet(&Fitler_Config);
		
#if CAN1_RX0_INT_ENABLE
	
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0��Ϣ�Һ��ж�����.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // �����ȼ�Ϊ1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;            // �����ȼ�Ϊ0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN1_RX0_INT_ENABLE	//ʹ��RX0�ж�
//�жϷ�����		
long num_can1_IRQ = 0;

void CAN1_RX0_IRQHandler(void)
{
  CanRxMsg RxMessage;
	char data[CAN_MSG_LEN+1];
	int i=0;
//	u8 len;
	uint32_t ret=0;
	//reset_fifo(can1_fifo);
	num_can1_IRQ++;
  CAN_Receive(CAN1, 0, &RxMessage);

//0xaa 0xbb len mCmd d0~d3 ide rtr data0... check01 check02 cmdend
// 0     1   2   3    4-7   8   9   10-17    18     19     20
	data[CAN_HEAD_0] = 0xAA;
	data[CAN_HEAD_1] = 0xBB;	
	data[CAN_DATA_LEN] = 7 + RxMessage.DLC;
	data[CAN_CMD] = 0x01;
  if (RxMessage.IDE == CAN_Id_Standard){
		data[CAN_ID_0] = 0x00;
		data[CAN_ID_1] = 0x00;
		data[CAN_ID_2] = RxMessage.StdId >> 8 & 0x07;
		data[CAN_ID_3] = RxMessage.StdId & 0xff;
  } else {
		data[CAN_ID_0] = RxMessage.ExtId >> 24 & 0x1f;
		data[CAN_ID_1] = RxMessage.ExtId >> 16 & 0xff;
		data[CAN_ID_2] = RxMessage.ExtId >> 8 & 0xff;
		data[CAN_ID_3] = RxMessage.ExtId & 0xff;
  }		
	
	data[CAN_IDE] = RxMessage.IDE;
	data[CAN_RTR] = RxMessage.RTR;
	for(i=0;i<RxMessage.DLC;i++) {
		data[CAN_D0+i] = RxMessage.Data[i];
	}

//	if(kk%100==0 || kfifo_len(can1_fifo) > (can1_fifo->size-30) ) printf("%ld->len=%d\r\n", kk, kfifo_len(can1_fifo));
	if(can1_fifo->size - kfifo_len(can1_fifo) >= 10+RxMessage.DLC) 
	{
		ret = kfifo_put(can1_fifo, data, 10+RxMessage.DLC);
	}

	if(ret != 10+RxMessage.DLC) 
	{
		can1_fifo->lostBytes+= 10+RxMessage.DLC;
	//	printf("CAN1_RX0_IRQHandler cause can1_fifo full.\r\n lostBytes=%d\r\n", 
	//		can1_fifo->lostBytes);
	}		 	 	
}
#endif

//can����һ������(�̶���ʽ:IDΪ0X12,��׼֡,����֡)	
//len:���ݳ���(���Ϊ8)				     
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;
//		 ����,ʧ��;
u8 CAN1_Send_Msg(u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=0;		  // ʹ����չ��ʶ��
  TxMessage.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=0;		  // ʹ����չ��ʶ��
  TxMessage.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=id;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=ide;		  // ʹ����չ��ʶ��
  TxMessage.RTR=rtr;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}
//can�ڽ������ݲ�ѯ
//buf:���ݻ�����;	 
//����ֵ:0,�����ݱ��յ�;
//		 ����,���յ����ݳ���;
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//û�н��յ�����,ֱ���˳� 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//��ȡ����	
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
    //ʹ�����ʱ��
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  //ʹ��PORTBʱ��	 PB5=CAN2_RX    PB6=CAN2_TX                  											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);//ʹ��CAN2ʱ��	
	
    //��ʼ��GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6; 
	  //GPIO_Pin_5 GPIO_Pin_6| GPIO_Pin_12 GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//���ù���
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//�������
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//����
    GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��PA11,PA12
	
	  //���Ÿ���ӳ������
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); //GPIOA5����ΪCAN2
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2); //GPIOA12����ΪCAN2
	  
  	//CAN��Ԫ����
		CAN_DeInit(CAN2);
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//��ʱ�䴥��ͨ��ģʽ   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//����Զ����߹���	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//˯��ģʽͨ���������(���CAN->MCR��SLEEPλ)
  	CAN_InitStructure.CAN_NART=DISABLE;	//��ֹ�����Զ����� ///////LEXLEXLEX
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//���Ĳ�����,�µĸ��Ǿɵ�  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//���ȼ��ɱ��ı�ʶ������ 
  	CAN_InitStructure.CAN_Mode= mode;	 //ģʽ���� 
  	CAN_InitStructure.CAN_SJW=tsjw;	//����ͬ����Ծ���(Tsjw)Ϊtsjw+1��ʱ�䵥λ CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1��ΧCAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2��ΧCAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //��Ƶϵ��(Fdiv)Ϊbrp+1	
  	if(CAN_InitStatus_Success == CAN_Init(CAN2, &CAN_InitStructure))
			;//printf("init can2: CAN_Init success.\r\n");   // ��ʼ��CAN1 
		else 
			printf("error -> init can2: CAN_Init fail.\r\n");   // ��ʼ��CAN1 
    
		//���ù����� ��ͨ��can1�����õģ���������ģʽ�������ʵȿ��Ը�������
		//������can��������can1���� can2�ӣ�ÿ���������������������䣬����fifo
		//ÿ��fifo�������������䡣
/* 	  CAN_FilterInitStructure.CAN_FilterNumber=16;	  //������0   һ��Ҫ����14
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32λ 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32λID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32λMASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//������0������FIFO0  //CAN_Filter_FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //���������0
  	CAN_FilterInit(&CAN_FilterInitStructure);//�˲�����ʼ��
*/		
#if CAN2_RX1_INT_ENABLE
	
	  CAN_ITConfig(CAN2, CAN_IT_FMP1/*|CAN_IT_FF1|CAN_IT_FOV1*/, ENABLE);//FIFO1��Ϣ�Һ��ж�����.		    //CAN_IT_FMP0 
    //CAN_IT_FMP1 FIFO 1 message pending Interrupt
		
		//NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);//add by lex
		//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;  //CAN2_RX0_IRQn ��ӦFIFO1
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // �����ȼ�Ϊ1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // �����ȼ�Ϊ0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN2_RX1_INT_ENABLE	//ʹ��RX0�ж�
//�жϷ�����			  
/*	
	printf("\r\n");
	printf("%s->RxMessage.StdId=0x%03x, RxMessage.IDE=0x%x, RxMessage.RTR=0x%x\r\n", 
		__func__, RxMessage.StdId, RxMessage.IDE, RxMessage.RTR);
	//RTR = 1 ��ʾԶ��֡
	//IDE = 1 ��ʾ��չ֡
	//StdId   ID 11bit or 29bit
	printf("data len=%d\r\n", RxMessage.DLC);//data len
	
	for(i=0;i<8;i++)
		printf("%d ", RxMessage.Data[i]);
	printf("\r\n");
*/

long num_can2_IRQ = 0;

void CAN2_RX1_IRQHandler(void) //CAN2_RX0_IRQHandler
{
	int i=0;
	uint32_t ret=0;
  CanRxMsg RxMessage;
	char data[CAN_MSG_LEN+1];
	
	num_can2_IRQ++;
  CAN_Receive(CAN2, CAN_FIFO1, &RxMessage);

//0xaa 0xbb len mCmd d0~d3 ide rtr data0... check01 check02 cmdend
// 0     1   2   3    4-7   8   9   10-17    18     19     20
	data[CAN_HEAD_0] = 0xAA;
	data[CAN_HEAD_1] = 0xBB;	
	data[CAN_DATA_LEN] = 7 + RxMessage.DLC;
	data[CAN_CMD] = 0x01;
  if (RxMessage.IDE == CAN_Id_Standard){
		data[CAN_ID_0] = 0x00;
		data[CAN_ID_1] = 0x00;
		data[CAN_ID_2] = RxMessage.StdId >> 8 & 0x07;
		data[CAN_ID_3] = RxMessage.StdId & 0xff;
  } else {
		data[CAN_ID_0] = RxMessage.ExtId >> 24 & 0x1f;
		data[CAN_ID_1] = RxMessage.ExtId >> 16 & 0xff;
		data[CAN_ID_2] = RxMessage.ExtId >> 8 & 0xff;
		data[CAN_ID_3] = RxMessage.ExtId & 0xff;
  }		
	
	data[CAN_IDE] = RxMessage.IDE;
	data[CAN_RTR] = RxMessage.RTR;
	for(i=0;i<RxMessage.DLC;i++) {
		data[CAN_D0+i] = RxMessage.Data[i];
	}

	if(can2_fifo->size - kfifo_len(can2_fifo) >= 10+RxMessage.DLC) 
	{
		ret = kfifo_put(can2_fifo, data, 10+RxMessage.DLC);
	}

	if(ret != 10+RxMessage.DLC) 
	{
		can2_fifo->lostBytes+= 10+RxMessage.DLC;
	}		 	
}
#endif

//can����һ������(�̶���ʽ:IDΪ0X12,��׼֡,����֡)	
//len:���ݳ���(���Ϊ8)				     
//msg:����ָ��,���Ϊ8���ֽ�.
//����ֵ:0,�ɹ�;
//		 ����,ʧ��;
u8 CAN2_Send_Msg(u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=0;		  // ʹ����չ��ʶ��
  TxMessage.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=0;		  // ʹ����չ��ʶ��
  TxMessage.RTR=0;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:��׼ID(11λ)/��չID(11λ+18λ)	    
//ide:0,��׼֡;1,��չ֡
//rtr:0,����֡;1,Զ��֡
//len:���ݳ���
//msg:���ݻ�����	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // ��׼��ʶ��Ϊ0 			11bit   0x712
  TxMessage.ExtId=id;	 // ������չ��ʾ����29λ��
  TxMessage.IDE=ide;		  // ʹ����չ��ʶ��
  TxMessage.RTR=rtr;		  // ��Ϣ����Ϊ����֡��һ֡8λ
  TxMessage.DLC=len;							 // ������֡��Ϣ
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // ��һ֡��Ϣ          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//�ȴ����ͽ���
  if(i>=0XFFF)return 1;
  return 0;		

}
//can�ڽ������ݲ�ѯ
//buf:���ݻ�����;	 
//����ֵ:0,�����ݱ��յ�;
//		 ����,���յ����ݳ���;
u8 CAN2_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN2,CAN_FIFO0)==0)return 0;		//û�н��յ�����,ֱ���˳� 
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);//��ȡ����	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}










