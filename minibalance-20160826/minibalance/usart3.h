#ifndef __USRAT3_H
#define __USRAT3_H 
#include "sys.h"	  	

extern u8 Usart3_Receive;
void uart3_init(u32 pclk2,u32 bound);
void USART3_IRQHandler(void);

extern u8 Way_Angle;                             //��ȡ�Ƕȵ��㷨��1����Ԫ��  2��������  3�������˲� 
extern u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu; //����ң����صı���
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

