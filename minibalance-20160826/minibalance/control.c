#include "control.h"	
#include "filter.h"	
#include "MPU6050.h"
#include "IOI2C.h"
#include "usart.h"
#include "inv_mpu_dmp_motion_driver.h"
#include "inv_mpu.h"
#include "dmpKey.h"
#include "dmpmap.h"
#include "DataScope_DP.h"
//#include "usart3.h"
#include "motor.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_i2c.h"
#include "stm32f10x_dma.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "encoder.h"
//u8 Way_Angle=3;    
//float Angle_Balance; 

int Balance_Pwm,Velocity_Pwm,Turn_Pwm;
u8 Flag_Target;
float mTurnKp = 20, mTurnKd = 0;
extern int Get_battery_volt(void);
static void Key_Scan(void);

int EXTI9_5_IRQHandler(void) 
{ 
	 if(EXTI_GetITStatus(EXTI_Line7) != RESET)		
	{     
 		//Get_Angle(Way_Angle);
	     //EXTI_ClearITPendingBit(EXTI_Line7);
		   Flag_Target=!Flag_Target;
  		 if(delay_flag==1)
			 {
				 if(((++delay_50)%40 == 0))	{	//200 = 1s
				  delay_50=0,delay_flag=0;                     //给主函数提供精准延时
					 if (((++delay_50)%400 == 0)) {
						 LED = !LED;
					 }
				 }
			 }
	 		if(0 && Flag_Target==1)                                                  //5ms读取一次陀螺仪和加速度计的值，更高的采样频率可以改善卡尔曼滤波和互补滤波的效果
			{
				Get_Angle(Way_Angle);                                               //===更新姿态	
				return 0;	                                               
			}       
			Flag_Qian = irq_qian;
			Flag_Hou = irq_hou;
			Flag_Right = irq_right;
			Flag_Left = irq_left;
			
			Encoder_Left=-Read_Encoder(2);                                      //===读取编码器的值，因为两个电机的旋转了180度的，所以对其中一个取反，保证输出极性一致
			Encoder_Right=Read_Encoder(4);
	
			Encoder_Left_total+= Encoder_Left;
			Encoder_Right_total+=  Encoder_Right;
			if (Encoder_Left_total > 9000) Encoder_Left_total = 0;
			if (Encoder_Right_total > 9000) Encoder_Right_total = 0;
			Get_Angle(Way_Angle);                                    //===更新姿态	
  		Voltage=Get_battery_volt(); 

 			Balance_Pwm =balance(Angle_Balance,Gyro_Balance);                   //===平衡PID控制	
		 	Velocity_Pwm=velocity(Encoder_Left,Encoder_Right);                  //===速度环PID控制	 记住，速度反馈是正反馈，就是小车快的时候要慢下来就需要再跑快一点
  	  		Turn_Pwm    = turn(Encoder_Left,Encoder_Right,Gyro_Turn);            //===转向环PID控制     
 		  	
			if (1) {
				Moto1=Balance_Pwm+Velocity_Pwm-Turn_Pwm;                            //===计算左轮电机最终PWM
 	  			Moto2=Balance_Pwm+Velocity_Pwm+Turn_Pwm;                            //===计算右轮电机最终PWM
			}
			
   		Xianfu_Pwm();          
			                                             //===PWM限幅
//			if(Pick_Up(Acceleration_Z, Angle_Balance, Encoder_Left, Encoder_Right))//===检查是否小车被那起
//				Flag_Stop=1;	                                                      //===如果被拿起就关闭电机
//			if(Put_Down(Angle_Balance, Encoder_Left, Encoder_Right))              //===检查是否小车被放下
//				Flag_Stop=0;	                                                      //===如果被放下就启动电机

      if(Turn_Off(Angle_Balance,Voltage)==0)                              //===如果不存在异常
 				Set_Pwm(Moto1,Moto2); 
			Key_Scan();    
			EXTI_ClearITPendingBit(EXTI_Line7);		
	}
	return 0;
}
  /**************************************************************************
作者：平衡小车之家
我的淘宝小店：http://shop114407458.taobao.com/
**************************************************************************/
int TIM1_UP_IRQHandler(void)  
{    
	if(TIM1->SR&0X0001)//5ms定时中断
	{   
		  TIM1->SR&=~(1<<0);                                       //===清除定时器1中断标志位		 		
	}       	
	 return 0;	  
} 


/**************************************************************************
函数功能：绝对值函数
入口参数：int
返回  值：unsigned int
**************************************************************************/
int myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}

/**************************************************************************
函数功能：直立PD控制
入口参数：角度、角速度
返回  值：直立控制PWM
作    者：平衡小车之家
**************************************************************************/

float kp0=175, kd0=0.60;

int balance(float Angle,float Gyro)
{  
   float Bias;
	 int balance;
	 Bias=Angle-ZHONGZHI;       //===求出平衡的角度中值 和机械相关
	 balance=kp0*Bias+Gyro*kd0;   //===计算平衡控制的电机PWM  PD控制   kp是P系数 kd是D系数 
	 return balance;
}

/**************************************************************************
函数功能：速度PI控制 修改前进后退遥控速度，请修Target_Velocity，比如，改成60就比较慢了
入口参数：左轮编码器、右轮编码器
返回  值：速度控制PWM
作    者：平衡小车之家
**************************************************************************/

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
			Target_Velocity=60 + mVelocityAdd;//75;                 //如果进入避障模式,自动进入低速模式
   		else
	        Target_Velocity=90 + mVelocityAdd;//130;                 
	
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

/**************************************************************************
函数功能：转向控制  修改转向速度，请修改Turn_Amplitude即可
入口参数：左轮编码器、右轮编码器、Z轴陀螺仪
返回  值：转向控制PWM
作    者：平衡小车之家
**************************************************************************/


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
		                                 
  	//=============转向PD控制器=======================//
		Turn=Turn_Target*mTurnKp-gyro*mTurnKd;                 //===结合Z轴陀螺仪进行PD控制
	 	return Turn;

  /*
 	static float Encoder_temp, Turn, Kp = 30, Kd, Bias, Turn_Target, Turn_Count, Turn_Convert=0.7;

	float Turn_Amplitude=50/Flag_sudu;

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
			Turn_Convert=0.4;
			Turn_Count=0;
			Encoder_temp=0;
	}
			
	if(1==Flag_Left)	           
		Turn_Target-=Turn_Convert;        //===接收转向遥控数据
	else if(1==Flag_Right)	     
		Turn_Target+=Turn_Convert;        //===接收转向遥控数据
	else 
		Turn_Target=0;                                            //===接收转向遥控数据

	if(Turn_Target>Turn_Amplitude)  
		Turn_Target=Turn_Amplitude;    //===转向速度限幅
  	if(Turn_Target<-Turn_Amplitude) 
		Turn_Target=-Turn_Amplitude;   //===转向速度限幅

 	if(Flag_Qian==1||Flag_Hou==1)  
		Kd=0.6;                         //===接收转向遥控数据直立行走的时候增加陀螺仪就纠正    
	else 
		Kd=0; 

 	Bias = gyro - 0; 
 	//Turn = -Bias*Kp;
	Turn_Target=0;
	Turn=Turn_Target*Kp - Bias*Kd;                 //===结合Z轴陀螺仪进行PD控制

 	return Turn;
  */
}
/**************************************************************************
函数功能：赋值给PWM寄存器
入口参数：左轮PWM、右轮PWM
返回  值：无
**************************************************************************/
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

/**************************************************************************
函数功能：异常关闭电机
入口参数：倾角和电压
返回  值：1：异常  0：正常
**************************************************************************/

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

/**************************************************************************
函数功能：检测小车是否被拿起
入口参数：int
返回  值：unsigned int
**************************************************************************/
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
		printf("%s: flag=%d, accel=%f, myabs(l,e)=%d, angle=%f!\r\n", __func__, flag, Acceleration, myabs(encoder_left+encoder_right),Angle);	
	 }

	return 0;
}

/**************************************************************************
函数功能：检测小车是否被放下
入口参数：int
返回  值：unsigned int
**************************************************************************/
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





/**************************************************************************
函数功能：获取角度
入口参数：获取角度的算法 1：无  2：卡尔曼 3：互补滤波
返回  值：无
**************************************************************************/
extern float testGyro_Y;
void Get_Angle(u8 way)
{ 
      //float Accel_Y,Accel_X,Accel_Z,Gyro_Y;
	  float Accel_Y,Accel_X,Accel_Z,Gyro_Y,Gyro_Z;      
	  Temperature=Read_Temperature(); 	  
	  if(way==1)                                      //DMP没有涉及到严格的时序问题，在主函数读取
	  {	
			Read_DMP();                      //===读取加速度、角速度、倾角
			Angle_Balance=Pitch;             //===更新平衡倾角
			Gyro_Balance=gyro[1];            //===更新平衡角速度
			Gyro_Turn=gyro[2];               //===更新转向角速度
			Acceleration_Z=accel[2];         //===更新Z轴加速度计		
	  }			
      else
      {

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

  /*
			Gyro_Y=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_L);    //读取Y轴陀螺仪
			Accel_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L);    //读取Z轴陀螺仪
		  	Accel_X=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_L); //读取X轴加速度记
	 		
			if(Gyro_Y>32768)  Gyro_Y-=65536;     //数据类型转换  也可通过short强制类型转换
			if(Accel_Z>32768)  Accel_Z-=65536;     //数据类型转换
	  		if(Accel_X>32768) Accel_X-=65536;    //数据类型转换
	   		
			Accel_Y=atan2(Accel_X,Accel_Z)*180/PI;                 //计算与地面的夹角	
		  	Gyro_Y=Gyro_Y/16.4; 
			                                   //陀螺仪量程转换	
      		if(Way_Angle==2)		  
				Kalman_Filter(Accel_Y,-Gyro_Y);//卡尔曼滤波	
			else if(Way_Angle==3)   
				Yijielvbo(Accel_Y,-Gyro_Y);    //互补滤波
	    	
			Angle_Balance=angle;                                   //更新平衡倾角
  */
	  	}
}


 void EXTI9_5_Config(void)
{
	EXTI_InitTypeDef   EXTI_InitStructure;
	GPIO_InitTypeDef   GPIO_InitStructure;
	NVIC_InitTypeDef   NVIC_InitStructure;
#if defined (STM32F10X_HD_VL) || defined (STM32F10X_HD) || defined (STM32F10X_XL)
  /* Enable GPIOG clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);

  /* Configure PG.08 pin as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOG, &GPIO_InitStructure);

  /* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  /* Connect EXTI8 Line to PG.08 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOG, GPIO_PinSource8);

  /* Configure EXTI8 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line8;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI9_5 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);
#else
  /* Enable GPIOB clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

  /* Configure PB.09 pin as input floating */
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
  GPIO_Init(GPIOA, &GPIO_InitStructure);

  /* Enable AFIO clock */
  RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);
  /* Connect EXTI9 Line to PB.09 pin */
  GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource7);

  /* Configure EXTI9 line */
  EXTI_InitStructure.EXTI_Line = EXTI_Line7;
  EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
  EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
  EXTI_InitStructure.EXTI_LineCmd = ENABLE;
  EXTI_Init(&EXTI_InitStructure);

  /* Enable and set EXTI9_5 Interrupt to the lowest priority */
  NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

  NVIC_Init(&NVIC_InitStructure);
#endif
}

void MiniBalance_Motor_Init(void)
{
	RCC->APB2ENR|=1<<3;       //PORTB时钟使能   
	GPIOB->CRH&=0X0000FFFF;   //PORTB12 13 14 15推挽输出
	GPIOB->CRH|=0X22220000;   //PORTB12 13 14 15推挽输出
}
void MiniBalance_PWM_Init(u16 arr,u16 psc)
{		 					 
	MiniBalance_Motor_Init();  //初始化电机控制所需IO
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

/**************************************************************************
函数功能：按键修改小车运行状态 
入口参数：无
返回  值：无
**************************************************************************/
u8 isKp0 = 1;

static void Key_Scan(void)
{	


	u8 tmp,tmp2;
	tmp=click_N_Double(50); 
	if(tmp==1) {
		Flag_Stop=!Flag_Stop; 
		LED=~LED; 
		printf("%s: single click\r\n", __func__); 
		if (isKp0) {
			kp0 += 10; 
		} else { 
			kd0 += 0.1; 
		}
	}//单击控制小车的启停
	if(tmp==2) {
		Flag_Show=!Flag_Show;
		LED=~LED;
		printf("%s: double click\r\n", __func__);//双击控制小车的显示状态
		if (isKp0) {
			kp0 -= 10; 
		} else { 
			kd0 -= 0.1; 
		}
	}
	tmp2=Long_Press();                   
  	if(tmp2==1) /*Bi_zhang=!Bi_zhang,*/ isKp0 = !isKp0, printf("%s: long click\r\n", __func__), LED=~LED;		//长按控制小车是否进入超声波避障模式 
}






