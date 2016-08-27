#ifndef __USRAT3_H
#define __USRAT3_H 
#include "sys.h"	  	

extern u8 Usart3_Receive;
void uart3_init(u32 pclk2,u32 bound);
void USART3_IRQHandler(void);

extern u8 Way_Angle;                             //获取角度的算法，1：四元数  2：卡尔曼  3：互补滤波 
extern u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu; //蓝牙遥控相关的变量
extern u8 Flag_Stop,Flag_Show;                 //停止标志位和 显示标志位 默认停止 显示打开
extern int Encoder_Left,Encoder_Right;             //左右编码器的脉冲计数
extern int Moto1,Moto2;                            //电机PWM变量 应是Motor的 向Moto致敬	
extern int Temperature;                            //显示温度
extern int Voltage;                                //电池电压采样相关的变量
extern float Angle_Balance,Gyro_Balance,Gyro_Turn; //平衡倾角 平衡陀螺仪 转向陀螺仪
extern float Show_Data_Mb;                         //全局显示变量，用于显示需要查看的数据
extern u32 Distance;                               //超声波测距
extern u8 delay_50,delay_flag,Bi_zhang;          //默认情况下，不开启避障功能，长按用户按键2s以上可以进入避障模式
extern float Acceleration_Z;        
#endif

