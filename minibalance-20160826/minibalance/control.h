#ifndef __CONTROL_H
#define __CONTROL_H
#include "sys.h"

#define PWMA   TIM1->CCR1  //PA8
#define AIN2   PBout(15)
#define AIN1   PBout(14)
#define BIN1   PBout(13)
#define BIN2   PBout(12)
#define PWMB   TIM1->CCR4  //PA11
void MiniBalance_PWM_Init(u16 arr,u16 psc);
void MiniBalance_Motor_Init(void);


extern void EXTI9_5_Config(void);
extern void MiniBalance_PWM_Init(u16 arr,u16 psc);
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/
#define PI 3.14159265
int TIM1_UP_IRQHandler(void);  
void Get_Angle(u8 way);

extern unsigned long sysTime;
extern u8 Way_Angle;    
extern float Angle_Balance; 
extern int Voltage;
#define PI 3.14159265
#define ZHONGZHI (-2.2)
extern	int Balance_Pwm,Velocity_Pwm,Turn_Pwm;
int EXTI15_10_IRQHandler(void);
int balance(float angle,float gyro);
int velocity(int encoder_left,int encoder_right);
int turn(int encoder_left,int encoder_right,float gyro);
void Set_Pwm(int moto1,int moto2);
void Key_lex(void);
void Xianfu_Pwm(void);
u8 Turn_Off(float angle, int voltage);
void Get_Angle(u8 way);
int myabs(int a);
int Pick_Up(float Acceleration,float Angle,int encoder_left,int encoder_right);
int Put_Down(float Angle,int encoder_left,int encoder_right);
extern unsigned int   Encoder_Left_total, Encoder_Right_total;

extern u8 Way_Angle;                             //获取角度的算法，1：四元数  2：卡尔曼  3：互补滤波 
extern u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu; //蓝牙遥控相关的变量
extern u8 irq_qian, irq_hou, irq_left, irq_right, irq_sudu;
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
