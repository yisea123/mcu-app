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
���ߣ�ƽ��С��֮��
�ҵ��Ա�С�꣺http://shop114407458.taobao.com/
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

extern u8 Way_Angle;                             //��ȡ�Ƕȵ��㷨��1����Ԫ��  2��������  3�������˲� 
extern u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu; //����ң����صı���
extern u8 irq_qian, irq_hou, irq_left, irq_right, irq_sudu;
extern u8 Flag_Stop,Flag_Show;                 //ֹͣ��־λ�� ��ʾ��־λ Ĭ��ֹͣ ��ʾ��
extern int Encoder_Left,Encoder_Right;             //���ұ��������������
extern int Moto1,Moto2;                            //���PWM���� Ӧ��Motor�� ��Moto�¾�	
extern int Temperature;                            //��ʾ�¶�
extern int Voltage;                                //��ص�ѹ������صı���
extern float Angle_Balance,Gyro_Balance,Gyro_Turn; //ƽ����� ƽ�������� ת��������
extern float Show_Data_Mb;                         //ȫ����ʾ������������ʾ��Ҫ�鿴������
extern u32 Distance;                               //���������
extern u8 delay_50,delay_flag,Bi_zhang;          //Ĭ������£����������Ϲ��ܣ������û�����2s���Ͽ��Խ������ģʽ
extern float Acceleration_Z;   
#endif
