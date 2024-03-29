#include "minibalance.h"
#include <string.h>
#include <stdio.h>
#include <stm32f10x.h>
#include "oled.h"
#include "control.h"	
#include "filter.h"	
#include "MPU6050.h"
#include "IOI2C.h"
#include "uart.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "inv_mpu.h"
#include "dmpKey.h"
#include "dmpmap.h"
#include "DataScope_DP.h"
#include "motor.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "encoder.h"
#include "gokit.h"

SYSTIMER *miniTimer = NULL;
	
u8 Way_Angle=2;                             //获取角度的算法，1：四元数  2：卡尔曼  3：互补滤波 
u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu=1; //蓝牙遥控相关的变量
u8 irq_qian, irq_hou, irq_left, irq_right, irq_sudu;
u8 Flag_Stop=0,Flag_Show=0;                 //停止标志位和 显示标志位 默认停止 显示打开
int Encoder_Left,Encoder_Right;             //左右编码器的脉冲计数
unsigned int   Encoder_Left_total=0, Encoder_Right_total = 0;
int Moto1,Moto2;                            //电机PWM变量 应是Motor的 向Moto致敬	
int Temperature;                            //显示温度
int Voltage = 1200;                                //电池电压采样相关的变量
float Angle_Balance,Gyro_Balance,Gyro_Turn; //平衡倾角 平衡陀螺仪 转向陀螺仪
float Show_Data_Mb;                         //全局显示变量，用于显示需要查看的数据
u32 Distance;                               //超声波测距
u8 delay_50,delay_flag,Bi_zhang=0;          //默认情况下，不开启避障功能，长按用户按键2s以上可以进入避障模式
float Acceleration_Z;                       //Z轴加速度计  
float testGyro_Y;

int Balance_Pwm,Velocity_Pwm,Turn_Pwm;
u8 Flag_Target;
float mTurnKp = 20, mTurnKd = 0;
extern int Get_battery_volt(void);
static int remoteCmd = 0;

void minibalance_core_task(void *timer) 
{    
		SYSTIMER* argc = (SYSTIMER*) timer;
		if (argc && argc->isMessage) {
				if (argc->message) {
					switch(argc->message->cmd) 
					{
						case CMD_RUNNING_CONTROL:
							Flag_Stop = !Flag_Stop;
							break;
						
						case CMD_REMOTE_CONTROL:
							handle_remote_control(*(argc->message->data));
							break;
						
						default:
							break;
					}
					release_message(argc->message);
				} else {
					printf("poll_minibalance_core message error!\r\n");
				}
				return;
		}
		
		Flag_Qian = irq_qian;
		Flag_Hou = irq_hou;
		Flag_Right = irq_right;
		Flag_Left = irq_left;

		Encoder_Left = -Read_Encoder(2);                                      //===读取编码器的值，因为两个电机的旋转了180度的，所以对其中一个取反，保证输出极性一致
		Encoder_Right = Read_Encoder(4);

		Encoder_Left_total += Encoder_Left;
		Encoder_Right_total +=  Encoder_Right;
	
		if (Encoder_Left_total > 9000) Encoder_Left_total = 0;
		if (Encoder_Right_total > 9000) Encoder_Right_total = 0;
	
		Get_Angle(Way_Angle);                                    //===更新姿态	
		//Voltage = Get_battery_volt(); 
		Balance_Pwm = balance(Angle_Balance, Gyro_Balance);                   //===平衡PID控制	
		Velocity_Pwm = velocity(Encoder_Left, Encoder_Right);                  //===速度环PID控制	 记住，速度反馈是正反馈，就是小车快的时候要慢下来就需要再跑快一点
		Turn_Pwm = turn(Encoder_Left, Encoder_Right, Gyro_Turn);            //===转向环PID控制     
			
		if (1) {
			 Moto1 = Balance_Pwm + Velocity_Pwm - Turn_Pwm;                            //===计算左轮电机最终PWM
			 Moto2 = Balance_Pwm + Velocity_Pwm + Turn_Pwm;                            //===计算右轮电机最终PWM
		}

		Xianfu_Pwm();          
																								 //===PWM限幅
		if (Pick_Up(Acceleration_Z, Angle_Balance, Encoder_Left, Encoder_Right))//===检查是否小车被那起
				Flag_Stop = 1;	                                                      //===如果被拿起就关闭电机
		if (Put_Down(Angle_Balance, Encoder_Left, Encoder_Right))              //===检查是否小车被放下
				Flag_Stop = 0;	                                                      //===如果被放下就启动电机

		if (Turn_Off(Angle_Balance,Voltage) == 0)                              //===如果不存在异常
				Set_Pwm(Moto1, Moto2); 
}

int myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}

float kp0=175, kd0=0.60;

int balance(float Angle,float Gyro)
{  
		float Bias;
		int balance;
		Bias=Angle-ZHONGZHI;       //===求出平衡的角度中值 和机械相关
		balance=kp0*Bias+Gyro*kd0;   //===计算平衡控制的电机PWM  PD控制   kp是P系数 kd是D系数 
		return balance;
}

int mVelocityAdd = 0;

int velocity(int encoder_left,int encoder_right)
{  
    static float Velocity,Encoder_Least,Encoder,Movement;
	  static float Encoder_Integral,Target_Velocity;
	  //float kp=-75,ki=kp/200;
	/*  {
		float kp=-115,ki=kp/200;
		Target_Velocity=50 + mVelocityAdd;//75; 
	  }
	*/
	  float kp=-125,ki=kp/200;
	  //=============遥控前进后退部分=======================// 
 
		if(/*Bi_zhang==1&&*/Flag_sudu==1)  
				Target_Velocity=50 + mVelocityAdd;//75;                 //如果进入避障模式,自动进入低速模式
		else
				Target_Velocity=80 + mVelocityAdd;//130;                 
	
		if(1==Flag_Qian)    	
		 	Movement=-Target_Velocity/(Flag_sudu);	         //===前进标志位置1 
		else if(1==Flag_Hou)	
			Movement=Target_Velocity/(Flag_sudu);         //===后退标志位置1
	 	else  
			Movement=0;	
	  	
//		if(Bi_zhang==1&&Distance<500&&Flag_Left!=1&&Flag_Right!=1)        //避障标志位置1且非遥控转弯的时候，进入避障模式
//	 	 	Movement=Target_Velocity/Flag_sudu;
   
   //=============速度PI控制器=======================//	
		Encoder_Least =(Encoder_Left+Encoder_Right)-0;                    //===获取最新速度偏差==测量速度（左右编码器之和）-目标速度（此处为零） 
		Encoder *= 0.7;		                                                //===一阶低通滤波器       
		Encoder += Encoder_Least*0.3;	                                    //===一阶低通滤波器    
		Encoder_Integral +=Encoder;                                       //===积分出位移 积分时间：10ms
		Encoder_Integral=Encoder_Integral-Movement;                       //===接收遥控器数据，控制前进后退
		if(Encoder_Integral>15000)  	Encoder_Integral=15000;             //===积分限幅
		if(Encoder_Integral<-15000)	Encoder_Integral=-15000;              //===积分限幅	
		Velocity=Encoder*kp+Encoder_Integral*ki;                          //===速度控制	
		if(Turn_Off(Angle_Balance,Voltage)==1||Flag_Stop==1)   Encoder_Integral=0;      //===电机关闭后清除积分
	  return Velocity;
}

int turn(int encoder_left,int encoder_right,float gyro)//转向控制
{

     static float Turn_Target,Turn,Encoder_temp,Turn_Convert=0.7,Turn_Count;//Kp=/*5*/40,Kd=0;

	   float Turn_Amplitude=50/Flag_sudu;  
	  
	    
	  //=============遥控左右旋转部分=======================//
 
  		if(1==Flag_Left||1==Flag_Right)                      //这一部分主要是根据旋转前的速度调整速度的起始速度，增加小车的适应性
		{
			if(++Turn_Count==1)
			Encoder_temp=myabs(encoder_left+encoder_right);
			Turn_Convert=50/Encoder_temp;
			if(Turn_Convert<0.4)Turn_Convert=0.4;
			if(Turn_Convert>1)Turn_Convert=1;
		}	
	  	else
		{
			Turn_Convert=0.7;
			Turn_Count=0;
			Encoder_temp=0;
		}	
			
		if(1==Flag_Left)	           Turn_Target-=Turn_Convert;        //===接收转向遥控数据
		else if(1==Flag_Right)	     Turn_Target+=Turn_Convert;        //===接收转向遥控数据
		else Turn_Target=0;                                            //===接收转向遥控数据
    	
		if(Turn_Target>Turn_Amplitude)  Turn_Target=Turn_Amplitude;    //===转向速度限幅
	  	if(Turn_Target<-Turn_Amplitude) Turn_Target=-Turn_Amplitude;   //===转向速度限幅
		
		if(Flag_Qian==1||Flag_Hou==1)  mTurnKd=0.6;                         //===接收转向遥控数据直立行走的时候增加陀螺仪就纠正    
		else mTurnKd=0;  
		
		Turn=Turn_Target*mTurnKp-gyro*mTurnKd;                 //===结合Z轴陀螺仪进行PD控制
		
	 	return Turn;
}
 
void Set_Pwm(int moto1,int moto2)
{
		int siqu=400;
		//moto1 = 0;
		//moto2 = 0;
		if(moto1<0)			AIN2=1,			AIN1=0;
		else 	          AIN2=0,			AIN1=1;
		//PWMA=myabs(moto1)+siqu-90+20/*-20-10*/;
		PWMA=myabs(moto1)+siqu-240/*-20-10*/;
		if(moto2<0)	BIN1=1,			BIN2=0;
		else        BIN1=0,			BIN2=1;
		//PWMB=myabs(moto2)+siqu+170+20/*-20-150*/;
		PWMB=myabs(moto2)+siqu-65/*-20-150*/;
		//siqu 的大小可以调节抖动！	
}

 /**************************************************************************
函数功能：限制PWM赋值 
入口参数：无
返回  值：无
**************************************************************************/
void Xianfu_Pwm(void)
{	
	  int Amplitude=6900;    //===PWM满幅是7200 限制在6900
    if(Moto1<-Amplitude) Moto1=-Amplitude;	
		if(Moto1>Amplitude)  Moto1=Amplitude;	
	  if(Moto2<-Amplitude) Moto2=-Amplitude;	
		if(Moto2>Amplitude)  Moto2=Amplitude;		
	
}

extern unsigned long sysTime ;

u8 Turn_Off(float angle, int voltage)
{
		u8 temp;
		if(angle<-45||angle>45 || 1==Flag_Stop || voltage<1130)//电池电压低于11.1V关闭电机
		{	                                                 //===倾角大于40度关闭电机
		temp=1;                                            //===Flag_Stop置1关闭电机
		AIN1=0;                                            
		AIN2=0;
		BIN1=0;
		BIN2=0;
		}
		else
		temp=0;
		return temp;			
}

int Pick_Up(float Acceleration,float Angle,int encoder_left,int encoder_right)
{ 		   
		static u16 flag,count0,count1,count2, tmp = 0;

		if(flag==0)                                                                   //第一步
		{
				if(myabs(encoder_left)+myabs(encoder_right)<30)                         //条件1，小车接近静止
				count0++;
				else 
				count0=0;		
				if(count0>10)				
				flag=1,count0=0; 
		} 
		if(flag==1)                                                                  //进入第二步
		{
				if(++count1>200)       count1=0,flag=0;                                 //超时不再等待2000ms
				if(Acceleration>19000.0 && (Angle>(-20+ZHONGZHI))&&(Angle<(20+ZHONGZHI)))   //条件2，小车是在0度附近被拿起
			{
			printf("%s: Acceleration=%f\r\n", __func__, Acceleration);
				flag=2;
			} 
		} 
		if(flag==2)                                                                  //第三步
		{
			if(++count2>100)       count2=0,flag=0;                                   //超时不再等待1000ms
			if(myabs(encoder_left+encoder_right)>140)                                 //条件3，小车的轮胎因为正反馈达到最大的转速   
			{
				flag=0;                                                                                     
				return 1;                                                               //检测到小车被拿起
			}
		}

		if (++tmp % 600 == 0) {
		tmp = 0;
		//printf("%s: flag=%d, accel=%f, myabs(l,e)=%d, angle=%f!\r\n", __func__, flag, Acceleration, myabs(encoder_left+encoder_right),Angle);	
		}

		return 0;
}

int Put_Down(float Angle,int encoder_left,int encoder_right)
{ 		   
		static u16 flag,count, tmp = 0;	 
	
		if(Flag_Stop==0)                           //防止误检      
			return 0;	
										 
		if(flag==0)                                               
		{
				if(Angle>(-10+ZHONGZHI)&&Angle<(10+ZHONGZHI)&&encoder_left==0&&encoder_right==0)         //条件1，小车是在0度附近的
				flag=1; 
		} 
		if(flag==1)                                               
		{
			if(++count>50)                                          //超时不再等待 500ms
			{
				count=0;flag=0;
			}
			if(encoder_left<-3&&encoder_right<-3&&encoder_left>-30&&encoder_right>-30)                //条件2，小车的轮胎在未上电的时候被人为转动  
			{
				flag=0;
				flag=0;
				return 1;                                             //检测到小车被放下
			}
		}

		if (++tmp % 600 == 0) {
		tmp = 0;
		//	printf("%s: flag=%d,  myabs(l,e)=%d, angle=%f!\r\n", __func__, flag, myabs(encoder_left+encoder_right),Angle);	
		}

		return 0;
}

void Get_Angle(u8 way)
{ 
	  float Accel_Y,Accel_X,Accel_Z,Gyro_Y,Gyro_Z;      
		// Temperature=Read_Temperature(); 	  
	  if (way == 1)                                      //DMP没有涉及到严格的时序问题，在主函数读取
	  {	
			Read_DMP();                      //===读取加速度、角速度、倾角
			Angle_Balance=Pitch;             //===更新平衡倾角
			Gyro_Balance=gyro[1];            //===更新平衡角速度
			Gyro_Turn=gyro[2];               //===更新转向角速度
			Acceleration_Z=accel[2];         //===更新Z轴加速度计		
	  }	else {
			Gyro_Y=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_L);    //读取Y轴陀螺仪
			Gyro_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_L);    //读取Z轴陀螺仪
			Accel_X=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_L); //读取X轴加速度计
			Accel_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L); //读取Z轴加速度计
			if(Gyro_Y>32768)  Gyro_Y-=65536;                       //数据类型转换  也可通过short强制类型转换
			if(Gyro_Z>32768)  Gyro_Z-=65536;                       //数据类型转换
			if(Accel_X>32768) Accel_X-=65536;                      //数据类型转换
			if(Accel_Z>32768) Accel_Z-=65536;                      //数据类型转换
			Gyro_Y = Gyro_Y - (-26);
			Gyro_Z = Gyro_Z - (-5);
			Gyro_Balance=-Gyro_Y;                                  //更新平衡角速度
			
			Accel_Y=atan2(Accel_X,Accel_Z)*180/PI;                 //计算倾角	
			//testGyro_Y = Accel_Y;
			Gyro_Y=Gyro_Y/16.4;                                    //陀螺仪量程转换	
			if(Way_Angle==2)		  	Kalman_Filter(Accel_Y,-Gyro_Y);//卡尔曼滤波	
			else if(Way_Angle==3)   Yijielvbo(Accel_Y,-Gyro_Y);    //互补滤波
			Angle_Balance=angle;                                   //更新平衡倾角
			Gyro_Turn=Gyro_Z;                                      //更新转向角速度
			Acceleration_Z=Accel_Z;                                //===更新Z轴加速度�	
		}
}

void minibalance_motor_init(void)
{
		RCC->APB2ENR|=1<<3;       //PORTB时钟使能   
		GPIOB->CRH&=0X0000FFFF;   //PORTB12 13 14 15推挽输出
		GPIOB->CRH|=0X22220000;   //PORTB12 13 14 15推挽输出
}

void MiniBalance_PWM_Init(u16 arr,u16 psc)
{		 					 
		minibalance_motor_init();  //初始化电机控制所需IO
		RCC->APB2ENR|=1<<11;       //使能TIM1时钟    
		RCC->APB2ENR|=1<<2;        //PORTA时钟使能     
		GPIOA->CRH&=0XFFFF0FF0;    //PORTA8 11复用输出
		GPIOA->CRH|=0X0000B00B;    //PORTA8 11复用输出
		TIM1->ARR=arr;             //设定计数器自动重装值 
		TIM1->PSC=psc;             //预分频器不分频
		TIM1->CCMR2|=6<<12;        //CH4 PWM1模式	
		TIM1->CCMR1|=6<<4;         //CH1 PWM1模式	
		TIM1->CCMR2|=1<<11;        //CH4预装载使能	 
		TIM1->CCMR1|=1<<3;         //CH1预装载使能	  
		TIM1->CCER|=1<<12;         //CH4输出使能	   
		TIM1->CCER|=1<<0;          //CH1输出使能	
		TIM1->BDTR |= 1<<15;       //TIM1必须要这句话才能输出PWM
		TIM1->CR1=0x8000;          //ARPE使能 
		TIM1->CR1|=0x01;          //使能定时器1 			
} 

SYSTIMER *lcdTimer = NULL;
void lcd_display_task(void *timer)
{
		unsigned char index[25];	
		static unsigned int tmp;

		SYSTIMER* argc = (SYSTIMER*) timer;
		if (argc && argc->isMessage) {
				if (argc->message) {
					release_message(argc->message);
				} 
				return;
		}	
		
		Voltage = Get_battery_volt();
		OLED_ShowString(5,20, "              ");
		memset(index, '\0', sizeof(index));
		snprintf((char*)index, sizeof(index), "Time:%05d:%d", tmp/10, tmp%10);
		OLED_ShowString(5,20, index);

		OLED_ShowString(5,0, "               ");
		memset(index, '\0', sizeof(index));
	  snprintf((char*)index, sizeof(index), "%s", Flag_Stop==0?"Running":"Stop!!!");
		OLED_ShowString(5,0, index);

		OLED_ShowString(5,30, "            ");
		memset(index, '\0', sizeof(index));
		//num_B = Read_Encoder(4);
		//snprintf((char*)index, sizeof(index), "kp0:%3.0f", kp0);
		snprintf((char*)index, sizeof(index), "Vol:%4d", Voltage);
		OLED_ShowString(5,30,index);

		//			OLED_ShowString(5,30, "            ");
		//		memset(index, '\0', sizeof(index));
		//		snprintf((char*)index, sizeof(index), "kd0:%1.03f", kd0);				
		//		OLED_ShowString(5,30,index);	   

		OLED_ShowString(5,40, "            ");
		memset(index, '\0', sizeof(index));
		snprintf((char*)index, sizeof(index), "Ang:%3.03f", Angle_Balance);	
		OLED_ShowString(5,40,index);

		OLED_ShowString(5,50, "            ");
		memset(index, '\0', sizeof(index));
		snprintf((char*)index, sizeof(index), "Cmd=%02X", remoteCmd);
		//strcpy(index, "xiaopeng");
		OLED_ShowString(5,50, index);
	
		OLED_Refresh_Gram();	
		tmp++;
}

void handle_remote_control(int uart_receive)
{
		//printf("cmd = %02x\r\n", uart_receive);
		remoteCmd = uart_receive;
	
		if (uart_receive > 10)
		{			
			if(uart_receive==0x5A)	
				irq_qian=0,irq_hou=0,irq_left=0,irq_right=0;
			else if(uart_receive==0x41)	
				irq_qian=1,irq_hou=0,irq_left=0,irq_right=0;
			else if(uart_receive==0x45)	
				irq_qian=0,irq_hou=1,irq_left=0,irq_right=0;
			else if(uart_receive==0x42||uart_receive==0x43||uart_receive==0x44)	
				irq_qian=0,irq_hou=0,irq_left=0,irq_right=1;
			else if(uart_receive==0x46||uart_receive==0x47||uart_receive==0x48)
				irq_qian=0,irq_hou=0,irq_left=1,irq_right=0;

			else if(uart_receive==0x70)
					mTurnKp = 13;	//5
			else if(uart_receive==0x71)
					mTurnKp = 13;
			else if(uart_receive==0x72)
					mTurnKp = 20;
			else if(uart_receive==0x73)
					mTurnKp = 25;
			else if(uart_receive==0x74)
					mTurnKp = 30;
			else if(uart_receive==0x75)
					mTurnKp = 33;
			else if(uart_receive==0x76)
					mTurnKp = 43;
			else if(uart_receive==0x77)
					mTurnKp = 48;

			else if(uart_receive==0x80)
					mVelocityAdd = 0;
			else if(uart_receive==0x81)
					mVelocityAdd = 10;
			else if(uart_receive==0x82)
					mVelocityAdd = 20;
			else if(uart_receive==0x83)
					mVelocityAdd = 30;
			else if(uart_receive==0x84)
					mVelocityAdd = 40;
			else if(uart_receive==0x85)
					mVelocityAdd = 50;
			else if(uart_receive==0x86)
					mVelocityAdd = 60;
			else if(uart_receive==0x87)
					mVelocityAdd = 70;			

			else if(uart_receive==0x90)
					mVelocityAdd = -5;
			else if(uart_receive==0x91)
					mVelocityAdd = -10;
			else if(uart_receive==0x92)
					mVelocityAdd = -15;
			else if(uart_receive==0x93)
					mVelocityAdd = -20;
			else if(uart_receive==0x94)
					mVelocityAdd = -25;

			else 
				irq_qian=0,irq_hou=0,irq_left=0,irq_right=0;
		}		
}


