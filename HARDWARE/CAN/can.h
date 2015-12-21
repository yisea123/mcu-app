#ifndef __CAN_H
#define __CAN_H	 
#include "sys.h"	    


//CAN1����RX0�ж�ʹ��
#define CAN1_RX0_INT_ENABLE	1		//0,��ʹ��;1,ʹ��.								    

extern u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN��ʼ��
 
extern u8 CAN1_Send_Msg(u8* msg,u8 len);						//��������

extern u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN1_Receive_Msg(u8 *buf);							//��������

extern u16 CAN1_RX_STA; 

//CAN2����RX0�ж�ʹ��
#define CAN2_RX0_INT_ENABLE	1		//0,��ʹ��;1,ʹ��.								    
//CAN2����RX0�ж�ʹ��
#define CAN2_RX1_INT_ENABLE	1		//0,��ʹ��;1,ʹ��.	

extern u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN��ʼ��
 
extern u8 CAN2_Send_Msg(u8* msg,u8 len);						//��������

extern u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN2_Receive_Msg(u8 *buf);							//��������

extern u16 CAN2_RX_STA; 
#endif

















