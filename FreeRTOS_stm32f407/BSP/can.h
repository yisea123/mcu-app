#ifndef __CAN_H
#define __CAN_H
#include "sys.h"

#if( BOARD_NUM == 3)	

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;

typedef struct {
    uint8_t sum_can1_id;//can1��?1y??id������y,��??��2?2��1y4*14=56
	  uint16_t *can1_id;//can1??��?��?1y??id
    //???a������???
	  uint16_t sum_can2_id;//can2��?1y??id������y,��??��2?2��1y4*14=56
	  uint16_t *can2_id;
	  uint8_t  filter_switch;//??2��?a1?
}Filter_config;

#define TurnOn   0  //??2��?�¡䨰?a
#define TurnOff  1  //??2��?��1?��?


//CAN1?����?RX0?D??��1?��
#define CAN1_RX0_INT_ENABLE	1		//0,2?��1?��;1,��1?��.								    

extern u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN3?��??��
 
extern u8 CAN1_Send_Msg(u8* msg,u8 len);						//����?����y?Y

extern u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN1_Receive_Msg(u8 *buf);							//?����?��y?Y

extern u16 CAN1_RX_STA; 

//CAN2?����?RX0?D??��1?��
#define CAN2_RX0_INT_ENABLE	1		//0,2?��1?��;1,��1?��.								    
//CAN2?����?RX0?D??��1?��
#define CAN2_RX1_INT_ENABLE	1		//0,2?��1?��;1,��1?��.	

extern u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN3?��??��
 
extern u8 CAN2_Send_Msg(u8* msg,u8 len);						//����?����y?Y

extern u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN2_Receive_Msg(u8 *buf);							//?����?��y?Y

extern u16 CAN2_RX_STA; 

extern long num_can1_IRQ, num_can2_IRQ;	 				    
#endif
#endif
