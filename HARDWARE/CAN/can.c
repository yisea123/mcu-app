#include "can.h"
#include "led.h"
#include "delay.h"
#include "usart.h"
#include "uart_command.h"

struct kfifo* can1_fifo;
struct kfifo* can2_fifo;

////////////////////////////////////////////////////////////////////////////////// 	 

//CAN初始化
//tsjw:重新同步跳跃时间单元. @ref CAN_synchronisation_jump_width   范围: ; CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   @ref CAN_time_quantum_in_bit_segment_2 范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   @refCAN_time_quantum_in_bit_segment_1  范围: ;	  CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024;(实际要加1,也就是1~1024) tq=(brp)*tpclk1
//波特率=Fpclk1/((tsjw+tbs1+tbs2+3)*brp);
//mode: @ref CAN_operating_mode 范围：CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为36M,如果设置CAN_Normal_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//则波特率为:42M/((1+6+7)*6)=500Kbps
//返回值:0,初始化OK;
//    其他,初始化失败;

//示范配置例子，
//设置滤波的id
uint16_t Can1IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};

uint16_t Can2IdFiter[]={0x0320,0x0321,0x031D,0x042B,0x0420,0x0318,0x031C,0x03C0,0x0330,
0x0345, 0x0338, 0x0348, 0x0307, 0x0325, 0x033A, 0x0336, 0x0393, 0x0340, 0x034B, 0x0353,
0x0352, 0x031E, 0x0428, 0x0265, 0x004D, 0x031F, 0x0436};



Filter_config Fitler_Config={
         
  (sizeof(Can1IdFiter)/sizeof(uint16_t)),//过滤can1的id个数，最多不操过4*14=56
	 Can1IdFiter,//过滤的id号
	(sizeof(Can2IdFiter)/sizeof(uint16_t)),//过滤can2的id个数，最多不操过4*14=56
	 Can2IdFiter,
	 TurnOn//滤波打开
};

//总共28个滤波器组，0-13分配给fifi0 ，14-27分配给fifi1
//由于CAN2的滤波设置是根据CAN1来设置，所以初始化的时候滤波只需设置一次
//滤波正确设置返回0；设置失败返回1；
uint8_t CAN_FilterSet(Filter_config *fitler_cfg)
{
	uint8_t i,j=0;
	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
	if(fitler_cfg->filter_switch==TurnOn)//滤波器打开
	{
			CAN1->FMR |= 1;//过滤器组工作在初始化模式    
			CAN1->FMR &= 0xffffc0ff;//can2的过滤器从14开始 
			CAN1->FMR |= (14<<8); 
			CAN1->FFA1R = 0x0fffc000;//0--13号过滤组关联到fifi0，14--27关联到fifi1
			if(fitler_cfg->sum_can1_id<=56&&fitler_cfg->sum_can1_id<=56)
			{	
					for(i=0,j=0;i<fitler_cfg->sum_can1_id;i++)
					{
							CAN1->FM1R |= (1<<i);//过滤器组i的寄存器组工作在列表模式,一个过滤组设置四个标准幀滤波id  
																	 //位宽为16位，2个32位分为四个16位寄存器，过滤                                                
							CAN1->FA1R &= ~(1<<i);//禁用过滤组i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;   
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)
							 {
									CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									CAN1->FA1R |= (1<<i);
									break;
							 }//使能过滤器组i			
							fitler_cfg->can1_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//使能过滤器组i	
							fitler_cfg->can1_id++;  
							CAN1->sFilterRegister[i].FR2 &= 0x00000000; 
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000; 
							if(++j==fitler_cfg->sum_can1_id)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//使能过滤器组i	
							fitler_cfg->can1_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can1_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can1_id)	{CAN1->FA1R |= (1<<i);break;}//使能过滤器组i						
							fitler_cfg->can1_id++; 
							
							CAN1->FA1R |= (1<<i);//使能过滤器组i   
					}
					
					
					for(i=14,j=14;i<fitler_cfg->sum_can2_id+14;i++)
					{
							CAN1->FM1R |= (1<<i);//过滤器组i的寄存器组工作在列表模式  
																	 //位宽为16位，2个32位分为四个16位寄存器，过滤                                                
							CAN1->FA1R &= ~(1<<i);//禁用过滤组i
								
							CAN1->sFilterRegister[i].FR1 &= 0x00000000;  
							CAN1->sFilterRegister[i].FR1 =(((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
								 CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
								 CAN1->FA1R |= (1<<i);
								 break;
							 }//使能过滤器组i			
							fitler_cfg->can2_id++;
							CAN1->sFilterRegister[i].FR1 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//使能过滤器组i	
							fitler_cfg->can2_id++; 
							CAN1->sFilterRegister[i].FR2 &= 0x00000000;  					
							CAN1->sFilterRegister[i].FR2 = (((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<21)&0xffff0000;
							if(++j==fitler_cfg->sum_can2_id+14)	
							 {
									 CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
									 CAN1->FA1R |= (1<<i);
									 break;
							 }//使能过滤器组i	
							fitler_cfg->can2_id++;    
							CAN1->sFilterRegister[i].FR2 |= ((((uint32_t)(*fitler_cfg->can2_id)|CAN_ID_STD|CAN_RTR_DATA)<<5)&0x0000ffff);
							if(++j==fitler_cfg->sum_can2_id+14)	{CAN1->FA1R |= (1<<i);break;}//使能过滤器组i						
							fitler_cfg->can2_id++; 
							
							CAN1->FA1R |= (1<<i);//使能过滤器组i   
					}

				}
				else return 1;
				
				CAN1->FMR &= ~1; //过滤器正常工作
	}	
	else//接收所有id
	{	
		    /* 设置CAN 筛选器0 */
				CAN_FilterInitStructure.CAN_FilterNumber = 0;		/*  筛选器序号，0-13，共14个滤波器 */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* 筛选器模式，设置ID掩码模式 */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32位筛选器 */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* 掩码后ID的高16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* 掩码后ID的低16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID掩码值高16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID掩码值低16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO0;		/* 筛选器绑定FIFO 0 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* 使能筛选器 */
				CAN_FilterInit(&CAN_FilterInitStructure);
					
				/* 设置CAN筛选器序号14 ，CAN2筛选器序号 14--27，而CAN1的是0--13*/
				CAN_FilterInitStructure.CAN_FilterNumber = 16;						/* 筛选器序号，14-27，共14个滤波器 */
				CAN_FilterInitStructure.CAN_FilterMode = CAN_FilterMode_IdMask;		/* 筛选器模式，设置ID掩码模式 */
				CAN_FilterInitStructure.CAN_FilterScale = CAN_FilterScale_32bit;	/* 32位筛选器 */
				CAN_FilterInitStructure.CAN_FilterIdHigh = 0x0000;					/* 掩码后ID的高16bit */
				CAN_FilterInitStructure.CAN_FilterIdLow = 0x0000;					/* 掩码后ID的低16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdHigh = 0x0000;				/* ID掩码值高16bit */
				CAN_FilterInitStructure.CAN_FilterMaskIdLow = 0x0000;				/* ID掩码值低16bit */
				CAN_FilterInitStructure.CAN_FilterFIFOAssignment = CAN_FIFO1;		/*  筛选器绑定FIFO 1 */
				CAN_FilterInitStructure.CAN_FilterActivation = ENABLE;				/* 使能筛选器 */
				CAN_FilterInit(&CAN_FilterInitStructure);	
	  }
    return 0;

}




//CAN初始化
//tsjw:重新同步跳跃时间单元. @ref CAN_synchronisation_jump_width   范围: ; CAN_SJW_1tq~ CAN_SJW_4tq
//tbs2:时间段2的时间单元.   @ref CAN_time_quantum_in_bit_segment_2 范围:CAN_BS2_1tq~CAN_BS2_8tq;
//tbs1:时间段1的时间单元.   @refCAN_time_quantum_in_bit_segment_1  范围: ;	  CAN_BS1_1tq ~CAN_BS1_16tq
//brp :波特率分频器.范围:1~1024;(实际要加1,也就是1~1024) tq=(brp)*tpclk1
//波特率=Fpclk1/((tsjw+tbs1+tbs2+3)*brp);
//mode: @ref CAN_operating_mode 范围：CAN_Mode_Normal,普通模式;CAN_Mode_LoopBack,回环模式;
//Fpclk1的时钟在初始化的时候设置为36M,如果设置CAN_Normal_Init(CAN_SJW_1tq,CAN_BS2_6tq,CAN_BS1_7tq,6,CAN_Mode_LoopBack);
//则波特率为:42M/((1+6+7)*6)=500Kbps
//返回值:0,初始化OK;
//    其他,初始化失败;

u16 CAN1_RX_STA=0;

u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode)
{

  	GPIO_InitTypeDef GPIO_InitStructure; 
	  CAN_InitTypeDef        CAN_InitStructure;
//  	CAN_FilterInitTypeDef  CAN_FilterInitStructure;
#if CAN1_RX0_INT_ENABLE 
   	NVIC_InitTypeDef  NVIC_InitStructure;
#endif
    //使能相关时钟
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);//使能PORTA时钟	                   											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1, ENABLE);//使能CAN1时钟	
	
    //初始化GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9| GPIO_Pin_8;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PA11,PA12
	
	  //引脚复用映射配置
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource9,GPIO_AF_CAN1); //GPIOA11复用为CAN1
	  GPIO_PinAFConfig(GPIOB,GPIO_PinSource8,GPIO_AF_CAN1); //GPIOA12复用为CAN1
	  
  	//CAN单元设置
		CAN_DeInit(CAN1); //lex
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//非时间触发通信模式   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//软件自动离线管理	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//睡眠模式通过软件唤醒(清除CAN->MCR的SLEEP位)
  	CAN_InitStructure.CAN_NART=DISABLE;	//禁止报文自动传送 
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//报文不锁定,新的覆盖旧的  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//优先级由报文标识符决定 
  	CAN_InitStructure.CAN_Mode= mode;	 //模式设置 
  	CAN_InitStructure.CAN_SJW=tsjw;	//重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位 CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1范围CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2范围CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //分频系数(Fdiv)为brp+1	
  	CAN_Init(CAN1, &CAN_InitStructure);   // 初始化CAN1 
    
		//配置过滤器
/*		
 	  CAN_FilterInitStructure.CAN_FilterNumber=0;	  //过滤器0
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32位 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32位ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32位MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO0;//过滤器0关联到FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //激活过滤器0
  	CAN_FilterInit(&CAN_FilterInitStructure);//滤波器初始化
*/
		CAN_FilterSet(&Fitler_Config);
		
#if CAN1_RX0_INT_ENABLE
	
	  CAN_ITConfig(CAN1,CAN_IT_FMP0,ENABLE);//FIFO0消息挂号中断允许.		    
  
  	NVIC_InitStructure.NVIC_IRQChannel = CAN1_RX0_IRQn;
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // 主优先级为1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;            // 次优先级为0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN1_RX0_INT_ENABLE	//使能RX0中断
//中断服务函数		
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

//can发送一组数据(固定格式:ID为0X12,标准帧,数据帧)	
//len:数据长度(最大为8)				     
//msg:数据指针,最大为8个字节.
//返回值:0,成功;
//		 其他,失败;
u8 CAN1_Send_Msg(u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 设置扩展标示符（29位）
  TxMessage.IDE=0;		  // 使用扩展标识符
  TxMessage.RTR=0;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 设置扩展标示符（29位）
  TxMessage.IDE=0;		  // 使用扩展标识符
  TxMessage.RTR=0;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=id;	 // 设置扩展标示符（29位）
  TxMessage.IDE=ide;		  // 使用扩展标识符
  TxMessage.RTR=rtr;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN1, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN1, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}
//can口接收数据查询
//buf:数据缓存区;	 
//返回值:0,无数据被收到;
//		 其他,接收的数据长度;
u8 CAN1_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN1,CAN_FIFO0)==0)return 0;		//没有接收到数据,直接退出 
    CAN_Receive(CAN1, CAN_FIFO0, &RxMessage);//读取数据	
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
    //使能相关时钟
	  RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	  //使能PORTB时钟	 PB5=CAN2_RX    PB6=CAN2_TX                  											 

  	RCC_APB1PeriphClockCmd(RCC_APB1Periph_CAN1 | RCC_APB1Periph_CAN2, ENABLE);//使能CAN2时钟	
	
    //初始化GPIO
	  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5| GPIO_Pin_6; 
	  //GPIO_Pin_5 GPIO_Pin_6| GPIO_Pin_12 GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;//复用功能
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;//推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;//100MHz
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;//上拉
    GPIO_Init(GPIOB, &GPIO_InitStructure);//初始化PA11,PA12
	
	  //引脚复用映射配置
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource5, GPIO_AF_CAN2); //GPIOA5复用为CAN2
	  GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_CAN2); //GPIOA12复用为CAN2
	  
  	//CAN单元设置
		CAN_DeInit(CAN2);
   	CAN_InitStructure.CAN_TTCM=DISABLE;	//非时间触发通信模式   
  	CAN_InitStructure.CAN_ABOM=DISABLE;	//软件自动离线管理	  
  	CAN_InitStructure.CAN_AWUM=DISABLE;//睡眠模式通过软件唤醒(清除CAN->MCR的SLEEP位)
  	CAN_InitStructure.CAN_NART=DISABLE;	//禁止报文自动传送 ///////LEXLEXLEX
  	CAN_InitStructure.CAN_RFLM=DISABLE;	//报文不锁定,新的覆盖旧的  
  	CAN_InitStructure.CAN_TXFP=DISABLE;	//优先级由报文标识符决定 
  	CAN_InitStructure.CAN_Mode= mode;	 //模式设置 
  	CAN_InitStructure.CAN_SJW=tsjw;	//重新同步跳跃宽度(Tsjw)为tsjw+1个时间单位 CAN_SJW_1tq~CAN_SJW_4tq
  	CAN_InitStructure.CAN_BS1=tbs1; //Tbs1范围CAN_BS1_1tq ~CAN_BS1_16tq
  	CAN_InitStructure.CAN_BS2=tbs2;//Tbs2范围CAN_BS2_1tq ~	CAN_BS2_8tq
  	CAN_InitStructure.CAN_Prescaler=brp;  //分频系数(Fdiv)为brp+1	
  	if(CAN_InitStatus_Success == CAN_Init(CAN2, &CAN_InitStructure))
			;//printf("init can2: CAN_Init success.\r\n");   // 初始化CAN1 
		else 
			printf("error -> init can2: CAN_Init fail.\r\n");   // 初始化CAN1 
    
		//配置过滤器 是通过can1来设置的，其他工作模式，波特率等可以各自设置
		//有两个can控制器，can1主， can2从，每个控制器有三个发送邮箱，两个fifo
		//每个fifo有三个接收邮箱。
/* 	  CAN_FilterInitStructure.CAN_FilterNumber=16;	  //过滤器0   一定要大于14
  	CAN_FilterInitStructure.CAN_FilterMode=CAN_FilterMode_IdMask; 
  	CAN_FilterInitStructure.CAN_FilterScale=CAN_FilterScale_32bit; //32位 
  	CAN_FilterInitStructure.CAN_FilterIdHigh=0x0000;////32位ID  0x0000 lex change
  	CAN_FilterInitStructure.CAN_FilterIdLow=0x0000;
  	CAN_FilterInitStructure.CAN_FilterMaskIdHigh=0x0000;//32位MASK
  	CAN_FilterInitStructure.CAN_FilterMaskIdLow=0x0000;
   	CAN_FilterInitStructure.CAN_FilterFIFOAssignment=CAN_Filter_FIFO1;//过滤器0关联到FIFO0  //CAN_Filter_FIFO0
  	CAN_FilterInitStructure.CAN_FilterActivation=ENABLE; //激活过滤器0
  	CAN_FilterInit(&CAN_FilterInitStructure);//滤波器初始化
*/		
#if CAN2_RX1_INT_ENABLE
	
	  CAN_ITConfig(CAN2, CAN_IT_FMP1/*|CAN_IT_FF1|CAN_IT_FOV1*/, ENABLE);//FIFO1消息挂号中断允许.		    //CAN_IT_FMP0 
    //CAN_IT_FMP1 FIFO 1 message pending Interrupt
		
		//NVIC_SetVectorTable(NVIC_VectTab_RAM, 0x0);//add by lex
		//NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x0);
  	NVIC_InitStructure.NVIC_IRQChannel = CAN2_RX1_IRQn;  //CAN2_RX0_IRQn 对应FIFO1
  	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;     // 主优先级为1
  	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;            // 次优先级为0
  	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  	NVIC_Init(&NVIC_InitStructure);
#endif
	return 0;
}   
 
#if CAN2_RX1_INT_ENABLE	//使能RX0中断
//中断服务函数			  
/*	
	printf("\r\n");
	printf("%s->RxMessage.StdId=0x%03x, RxMessage.IDE=0x%x, RxMessage.RTR=0x%x\r\n", 
		__func__, RxMessage.StdId, RxMessage.IDE, RxMessage.RTR);
	//RTR = 1 表示远程帧
	//IDE = 1 表示扩展帧
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

//can发送一组数据(固定格式:ID为0X12,标准帧,数据帧)	
//len:数据长度(最大为8)				     
//msg:数据指针,最大为8个字节.
//返回值:0,成功;
//		 其他,失败;
u8 CAN2_Send_Msg(u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=0x12;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 设置扩展标示符（29位）
  TxMessage.IDE=0;		  // 使用扩展标识符
  TxMessage.RTR=0;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=0x12;	 // 设置扩展标示符（29位）
  TxMessage.IDE=0;		  // 使用扩展标识符
  TxMessage.RTR=0;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}

u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len)
{	
//id:标准ID(11位)/扩展ID(11位+18位)	    
//ide:0,标准帧;1,扩展帧
//rtr:0,数据帧;1,远程帧
//len:数据长度
//msg:数据缓存区	
  u8 mbox;
  u16 i=0;
  CanTxMsg TxMessage;
  TxMessage.StdId=id;	 // 标准标识符为0 			11bit   0x712
  TxMessage.ExtId=id;	 // 设置扩展标示符（29位）
  TxMessage.IDE=ide;		  // 使用扩展标识符
  TxMessage.RTR=rtr;		  // 消息类型为数据帧，一帧8位
  TxMessage.DLC=len;							 // 发送两帧信息
  for(i=0;i<len;i++)
  TxMessage.Data[i]=msg[i];				 // 第一帧信息          
  mbox= CAN_Transmit(CAN2, &TxMessage);   
  i=0;
  while((CAN_TransmitStatus(CAN2, mbox)==CAN_TxStatus_Failed)&&(i<0XFFF))i++;	//等待发送结束
  if(i>=0XFFF)return 1;
  return 0;		

}
//can口接收数据查询
//buf:数据缓存区;	 
//返回值:0,无数据被收到;
//		 其他,接收的数据长度;
u8 CAN2_Receive_Msg(u8 *buf)
{		   		   
 	u32 i;
	CanRxMsg RxMessage;
    if( CAN_MessagePending(CAN2,CAN_FIFO0)==0)return 0;		//没有接收到数据,直接退出 
    CAN_Receive(CAN2, CAN_FIFO0, &RxMessage);//读取数据	
    for(i=0;i<RxMessage.DLC;i++)
    buf[i]=RxMessage.Data[i];  
	return RxMessage.DLC;	
}










