#ifndef __CAN_H
#define __CAN_H	 
#include "sys.h"
	    
#define TurnOn   0  //滤波器打开
#define TurnOff  1  //滤波器关闭

typedef struct {
    uint8_t sum_can1_id;//can1的过滤id总数,最多不操过4*14=56
	  uint16_t *can1_id;//can1具体的过滤id
    //都为标准幀
	  uint16_t sum_can2_id;//can2的过滤id总数,最多不操过4*14=56
	  uint16_t *can2_id;
	  uint8_t  filter_switch;//滤波开关
}Filter_config;


//CAN1接收RX0中断使能
#define CAN1_RX0_INT_ENABLE	1		//0,不使能;1,使能.								    

extern u8 CAN1_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN初始化
 
extern u8 CAN1_Send_Msg(u8* msg,u8 len);						//发送数据

extern u8 CAN1_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN1_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN1_Receive_Msg(u8 *buf);							//接收数据

extern u16 CAN1_RX_STA; 

//CAN2接收RX0中断使能
#define CAN2_RX0_INT_ENABLE	1		//0,不使能;1,使能.								    
//CAN2接收RX0中断使能
#define CAN2_RX1_INT_ENABLE	1		//0,不使能;1,使能.	

extern u8 CAN2_Mode_Init(u8 tsjw,u8 tbs2,u8 tbs1,u16 brp,u8 mode);//CAN初始化
 
extern u8 CAN2_Send_Msg(u8* msg,u8 len);						//发送数据

extern u8 CAN2_Send_Msg1(uint32_t id, u8* msg,u8 len);

extern u8 CAN2_Send_Msg2(uint32_t id, uint8_t ide, uint8_t rtr, u8* msg,u8 len);

extern u8 CAN2_Receive_Msg(u8 *buf);							//接收数据

extern u16 CAN2_RX_STA; 

extern struct kfifo* can1_fifo;

extern struct kfifo* can2_fifo;

extern long num_can1_IRQ, num_can2_IRQ;

#endif

















