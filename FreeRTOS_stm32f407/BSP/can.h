#ifndef __CAN_H
#define __CAN_H
#include "sys.h"

#if( BOARD_NUM == 3)	

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct {
    uint8_t sum_can1_id;//can1米?1y??id℅邦那y,℅??角2?2迄1y4*14=56
	  uint16_t *can1_id;//can1??足?米?1y??id
    //???a㊣那℅???
	  uint16_t sum_can2_id;//can2米?1y??id℅邦那y,℅??角2?2迄1y4*14=56
	  uint16_t *can2_id;
	  uint8_t  filter_switch;//??2“?a1?
}Filter_config;

#define TurnOn   0  //??2“?¯∩辰?a
#define TurnOff  1  //??2“?¯1?㊣?


//CAN1?車那?RX0?D??那1?邦
#define CAN1_RX0_INT_ENABLE	1		//0,2?那1?邦;1,那1?邦.								    

extern u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN3?那??‘
 
extern u8 CAN1_Send_Msg(u8* msg,u8 len);						//﹞⊿?赤那y?Y

extern u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN1_Receive_Msg(u8 *buf);							//?車那?那y?Y

extern u16 CAN1_RX_STA; 

//CAN2?車那?RX0?D??那1?邦
#define CAN2_RX0_INT_ENABLE	1		//0,2?那1?邦;1,那1?邦.								    
//CAN2?車那?RX0?D??那1?邦
#define CAN2_RX1_INT_ENABLE	1		//0,2?那1?邦;1,那1?邦.	

extern u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN3?那??‘
 
extern u8 CAN2_Send_Msg(u8* msg,u8 len);						//﹞⊿?赤那y?Y

extern u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN2_Receive_Msg(u8 *buf);							//?車那?那y?Y

extern u16 CAN2_RX_STA; 

extern long num_can1_IRQ, num_can2_IRQ;	 				    
#endif
#endif
