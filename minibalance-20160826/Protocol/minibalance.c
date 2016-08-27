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

u8 Way_Angle=2;                             //»ñÈ¡½Ç¶ÈµÄËã·¨£¬1£ºËÄÔªÊý  2£º¿¨¶ûÂü  3£º»¥²¹ÂË²¨ 
u8 Flag_Qian,Flag_Hou,Flag_Left,Flag_Right,Flag_sudu=1; //À¶ÑÀÒ£¿ØÏà¹ØµÄ±äÁ¿
u8 irq_qian, irq_hou, irq_left, irq_right, irq_sudu;
u8 Flag_Stop=0,Flag_Show=0;                 //Í£Ö¹±êÖ¾Î»ºÍ ÏÔÊ¾±êÖ¾Î» Ä¬ÈÏÍ£Ö¹ ÏÔÊ¾´ò¿ª
int Encoder_Left,Encoder_Right;             //×óÓÒ±àÂëÆ÷µÄÂö³å¼ÆÊý
unsigned int   Encoder_Left_total=0, Encoder_Right_total = 0;
int Moto1,Moto2;                            //µç»úPWM±äÁ¿ Ó¦ÊÇMotorµÄ ÏòMotoÖÂ¾´	
int Temperature;                            //ÏÔÊ¾ÎÂ¶È
int Voltage = 1200;                                //µç³ØµçÑ¹²ÉÑùÏà¹ØµÄ±äÁ¿
float Angle_Balance,Gyro_Balance,Gyro_Turn; //Æ½ºâÇã½Ç Æ½ºâÍÓÂÝÒÇ ×ªÏòÍÓÂÝÒÇ
float Show_Data_Mb;                         //È«¾ÖÏÔÊ¾±äÁ¿£¬ÓÃÓÚÏÔÊ¾ÐèÒª²é¿´µÄÊý¾Ý
u32 Distance;                               //³¬Éù²¨²â¾à
u8 delay_50,delay_flag,Bi_zhang=0;          //Ä¬ÈÏÇé¿öÏÂ£¬²»¿ªÆô±ÜÕÏ¹¦ÄÜ£¬³¤°´ÓÃ»§°´¼ü2sÒÔÉÏ¿ÉÒÔ½øÈë±ÜÕÏÄ£Ê½
float Acceleration_Z;                       //ZÖá¼ÓËÙ¶È¼Æ  
float testGyro_Y;

int Balance_Pwm,Velocity_Pwm,Turn_Pwm;
u8 Flag_Target;
float mTurnKp = 20, mTurnKd = 0;
extern int Get_battery_volt(void);

void poll_minibalance_core(void *timer) 
{    
		Flag_Qian = irq_qian;
		Flag_Hou = irq_hou;
		Flag_Right = irq_right;
		Flag_Left = irq_left;

		Encoder_Left = -Read_Encoder(2);                                      //===¶ÁÈ¡±àÂëÆ÷µÄÖµ£¬ÒòÎªÁ½¸öµç»úµÄÐý×ªÁË180¶ÈµÄ£¬ËùÒÔ¶ÔÆäÖÐÒ»¸öÈ¡·´£¬±£Ö¤Êä³ö¼«ÐÔÒ»ÖÂ
		Encoder_Right = Read_Encoder(4);

		Encoder_Left_total += Encoder_Left;
		Encoder_Right_total +=  Encoder_Right;
	
		if (Encoder_Left_total > 9000) Encoder_Left_total = 0;
		if (Encoder_Right_total > 9000) Encoder_Right_total = 0;
	
		Get_Angle(Way_Angle);                                    //===¸üÐÂ×ËÌ¬	
		//Voltage = Get_battery_volt(); 
		Balance_Pwm = balance(Angle_Balance, Gyro_Balance);                   //===Æ½ºâPID¿ØÖÆ	
		Velocity_Pwm = velocity(Encoder_Left, Encoder_Right);                  //===ËÙ¶È»·PID¿ØÖÆ	 ¼Ç×¡£¬ËÙ¶È·´À¡ÊÇÕý·´À¡£¬¾ÍÊÇÐ¡³µ¿ìµÄÊ±ºòÒªÂýÏÂÀ´¾ÍÐèÒªÔÙÅÜ¿ìÒ»µã
		Turn_Pwm = turn(Encoder_Left, Encoder_Right, Gyro_Turn);            //===×ªÏò»·PID¿ØÖÆ     
			
		if (1) {
			 Moto1 = Balance_Pwm + Velocity_Pwm - Turn_Pwm;                            //===¼ÆËã×óÂÖµç»ú×îÖÕPWM
			 Moto2 = Balance_Pwm + Velocity_Pwm + Turn_Pwm;                            //===¼ÆËãÓÒÂÖµç»ú×îÖÕPWM
		}

		Xianfu_Pwm();          
																								 //===PWMÏÞ·ù
		if (Pick_Up(Acceleration_Z, Angle_Balance, Encoder_Left, Encoder_Right))//===¼ì²éÊÇ·ñÐ¡³µ±»ÄÇÆð
				Flag_Stop = 1;	                                                      //===Èç¹û±»ÄÃÆð¾Í¹Ø±Õµç»ú
		if (Put_Down(Angle_Balance, Encoder_Left, Encoder_Right))              //===¼ì²éÊÇ·ñÐ¡³µ±»·ÅÏÂ
				Flag_Stop = 0;	                                                      //===Èç¹û±»·ÅÏÂ¾ÍÆô¶¯µç»ú

		if (Turn_Off(Angle_Balance,Voltage) == 0)                              //===Èç¹û²»´æÔÚÒì³£
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
		Bias=Angle-ZHONGZHI;       //===Çó³öÆ½ºâµÄ½Ç¶ÈÖÐÖµ ºÍ»úÐµÏà¹Ø
		balance=kp0*Bias+Gyro*kd0;   //===¼ÆËãÆ½ºâ¿ØÖÆµÄµç»úPWM  PD¿ØÖÆ   kpÊÇPÏµÊý kdÊÇDÏµÊý 
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
	  //=============Ò£¿ØÇ°½øºóÍË²¿·Ö=======================// 
 
		if(/*Bi_zhang==1&&*/Flag_sudu==1)  
				Target_Velocity=60 + mVelocityAdd;//75;                 //Èç¹û½øÈë±ÜÕÏÄ£Ê½,×Ô¶¯½øÈëµÍËÙÄ£Ê½
		else
				Target_Velocity=90 + mVelocityAdd;//130;                 
	
		if(1==Flag_Qian)    	
		 	Movement=-Target_Velocity/(Flag_sudu);	         //===Ç°½ø±êÖ¾Î»ÖÃ1 
		else if(1==Flag_Hou)	
			Movement=Target_Velocity/(Flag_sudu);         //===ºóÍË±êÖ¾Î»ÖÃ1
	 	else  
			Movement=0;	
	  	
//		if(Bi_zhang==1&&Distance<500&&Flag_Left!=1&&Flag_Right!=1)        //±ÜÕÏ±êÖ¾Î»ÖÃ1ÇÒ·ÇÒ£¿Ø×ªÍäµÄÊ±ºò£¬½øÈë±ÜÕÏÄ£Ê½
//	 	 	Movement=Target_Velocity/Flag_sudu;
   
   //=============ËÙ¶ÈPI¿ØÖÆÆ÷=======================//	
		Encoder_Least =(Encoder_Left+Encoder_Right)-0;                    //===»ñÈ¡×îÐÂËÙ¶ÈÆ«²î==²âÁ¿ËÙ¶È£¨×óÓÒ±àÂëÆ÷Ö®ºÍ£©-Ä¿±êËÙ¶È£¨´Ë´¦ÎªÁã£© 
		Encoder *= 0.7;		                                                //===Ò»½×µÍÍ¨ÂË²¨Æ÷       
		Encoder += Encoder_Least*0.3;	                                    //===Ò»½×µÍÍ¨ÂË²¨Æ÷    
		Encoder_Integral +=Encoder;                                       //===»ý·Ö³öÎ»ÒÆ »ý·ÖÊ±¼ä£º10ms
		Encoder_Integral=Encoder_Integral-Movement;                       //===½ÓÊÕÒ£¿ØÆ÷Êý¾Ý£¬¿ØÖÆÇ°½øºóÍË
		if(Encoder_Integral>15000)  	Encoder_Integral=15000;             //===»ý·ÖÏÞ·ù
		if(Encoder_Integral<-15000)	Encoder_Integral=-15000;              //===»ý·ÖÏÞ·ù	
		Velocity=Encoder*kp+Encoder_Integral*ki;                          //===ËÙ¶È¿ØÖÆ	
		if(Turn_Off(Angle_Balance,Voltage)==1||Flag_Stop==1)   Encoder_Integral=0;      //===µç»ú¹Ø±ÕºóÇå³ý»ý·Ö
	  return Velocity;
}

int turn(int encoder_left,int encoder_right,float gyro)//×ªÏò¿ØÖÆ
{

     static float Turn_Target,Turn,Encoder_temp,Turn_Convert=0.7,Turn_Count;//Kp=/*5*/40,Kd=0;

	   float Turn_Amplitude=50/Flag_sudu;  
	  
	    
	  //=============Ò£¿Ø×óÓÒÐý×ª²¿·Ö=======================//
 
  		if(1==Flag_Left||1==Flag_Right)                      //ÕâÒ»²¿·ÖÖ÷ÒªÊÇ¸ù¾ÝÐý×ªÇ°µÄËÙ¶Èµ÷ÕûËÙ¶ÈµÄÆðÊ¼ËÙ¶È£¬Ôö¼ÓÐ¡³µµÄÊÊÓ¦ÐÔ
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
			
		if(1==Flag_Left)	           Turn_Target-=Turn_Convert;        //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý
		else if(1==Flag_Right)	     Turn_Target+=Turn_Convert;        //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý
		else Turn_Target=0;                                            //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý
    	
		if(Turn_Target>Turn_Amplitude)  Turn_Target=Turn_Amplitude;    //===×ªÏòËÙ¶ÈÏÞ·ù
	  	if(Turn_Target<-Turn_Amplitude) Turn_Target=-Turn_Amplitude;   //===×ªÏòËÙ¶ÈÏÞ·ù
		
		if(Flag_Qian==1||Flag_Hou==1)  mTurnKd=0.6;                         //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾ÝÖ±Á¢ÐÐ×ßµÄÊ±ºòÔö¼ÓÍÓÂÝÒÇ¾Í¾ÀÕý    
		else mTurnKd=0;  
		
		Turn=Turn_Target*mTurnKp-gyro*mTurnKd;                 //===½áºÏZÖáÍÓÂÝÒÇ½øÐÐPD¿ØÖÆ
		
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
		//siqu µÄ´óÐ¡¿ÉÒÔµ÷½Ú¶¶¶¯£¡	
}

 /**************************************************************************
º¯Êý¹¦ÄÜ£ºÏÞÖÆPWM¸³Öµ 
Èë¿Ú²ÎÊý£ºÎÞ
·µ»Ø  Öµ£ºÎÞ
**************************************************************************/
void Xianfu_Pwm(void)
{	
	  int Amplitude=6900;    //===PWMÂú·ùÊÇ7200 ÏÞÖÆÔÚ6900
    if(Moto1<-Amplitude) Moto1=-Amplitude;	
		if(Moto1>Amplitude)  Moto1=Amplitude;	
	  if(Moto2<-Amplitude) Moto2=-Amplitude;	
		if(Moto2>Amplitude)  Moto2=Amplitude;		
	
}

extern unsigned long sysTime ;

u8 Turn_Off(float angle, int voltage)
{
		u8 temp;
		if(angle<-45||angle>45 || 1==Flag_Stop || voltage<1130)//µç³ØµçÑ¹µÍÓÚ11.1V¹Ø±Õµç»ú
		{	                                                 //===Çã½Ç´óÓÚ40¶È¹Ø±Õµç»ú
		temp=1;                                            //===Flag_StopÖÃ1¹Ø±Õµç»ú
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

		if(flag==0)                                                                   //µÚÒ»²½
		{
				if(myabs(encoder_left)+myabs(encoder_right)<30)                         //Ìõ¼þ1£¬Ð¡³µ½Ó½ü¾²Ö¹
				count0++;
				else 
				count0=0;		
				if(count0>10)				
				flag=1,count0=0; 
		} 
		if(flag==1)                                                                  //½øÈëµÚ¶þ²½
		{
				if(++count1>200)       count1=0,flag=0;                                 //³¬Ê±²»ÔÙµÈ´ý2000ms
				if(Acceleration>19000.0 && (Angle>(-20+ZHONGZHI))&&(Angle<(20+ZHONGZHI)))   //Ìõ¼þ2£¬Ð¡³µÊÇÔÚ0¶È¸½½ü±»ÄÃÆð
			{
			printf("%s: Acceleration=%f\r\n", __func__, Acceleration);
				flag=2;
			} 
		} 
		if(flag==2)                                                                  //µÚÈý²½
		{
			if(++count2>100)       count2=0,flag=0;                                   //³¬Ê±²»ÔÙµÈ´ý1000ms
			if(myabs(encoder_left+encoder_right)>140)                                 //Ìõ¼þ3£¬Ð¡³µµÄÂÖÌ¥ÒòÎªÕý·´À¡´ïµ½×î´óµÄ×ªËÙ   
			{
				flag=0;                                                                                     
				return 1;                                                               //¼ì²âµ½Ð¡³µ±»ÄÃÆð
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
	
		if(Flag_Stop==0)                           //·ÀÖ¹Îó¼ì      
			return 0;	
										 
		if(flag==0)                                               
		{
				if(Angle>(-10+ZHONGZHI)&&Angle<(10+ZHONGZHI)&&encoder_left==0&&encoder_right==0)         //Ìõ¼þ1£¬Ð¡³µÊÇÔÚ0¶È¸½½üµÄ
				flag=1; 
		} 
		if(flag==1)                                               
		{
			if(++count>50)                                          //³¬Ê±²»ÔÙµÈ´ý 500ms
			{
				count=0;flag=0;
			}
			if(encoder_left<-3&&encoder_right<-3&&encoder_left>-30&&encoder_right>-30)                //Ìõ¼þ2£¬Ð¡³µµÄÂÖÌ¥ÔÚÎ´ÉÏµçµÄÊ±ºò±»ÈËÎª×ª¶¯  
			{
				flag=0;
				flag=0;
				return 1;                                             //¼ì²âµ½Ð¡³µ±»·ÅÏÂ
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
	  if (way == 1)                                      //DMPÃ»ÓÐÉæ¼°µ½ÑÏ¸ñµÄÊ±ÐòÎÊÌâ£¬ÔÚÖ÷º¯Êý¶ÁÈ¡
	  {	
			Read_DMP();                      //===¶ÁÈ¡¼ÓËÙ¶È¡¢½ÇËÙ¶È¡¢Çã½Ç
			Angle_Balance=Pitch;             //===¸üÐÂÆ½ºâÇã½Ç
			Gyro_Balance=gyro[1];            //===¸üÐÂÆ½ºâ½ÇËÙ¶È
			Gyro_Turn=gyro[2];               //===¸üÐÂ×ªÏò½ÇËÙ¶È
			Acceleration_Z=accel[2];         //===¸üÐÂZÖá¼ÓËÙ¶È¼Æ		
	  }	else {
			Gyro_Y=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_L);    //¶ÁÈ¡YÖáÍÓÂÝÒÇ
			Gyro_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_ZOUT_L);    //¶ÁÈ¡ZÖáÍÓÂÝÒÇ
			Accel_X=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_L); //¶ÁÈ¡XÖá¼ÓËÙ¶È¼Æ
			Accel_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L); //¶ÁÈ¡ZÖá¼ÓËÙ¶È¼Æ
			if(Gyro_Y>32768)  Gyro_Y-=65536;                       //Êý¾ÝÀàÐÍ×ª»»  Ò²¿ÉÍ¨¹ýshortÇ¿ÖÆÀàÐÍ×ª»»
			if(Gyro_Z>32768)  Gyro_Z-=65536;                       //Êý¾ÝÀàÐÍ×ª»»
			if(Accel_X>32768) Accel_X-=65536;                      //Êý¾ÝÀàÐÍ×ª»»
			if(Accel_Z>32768) Accel_Z-=65536;                      //Êý¾ÝÀàÐÍ×ª»»
			Gyro_Y = Gyro_Y - (-26);
			Gyro_Z = Gyro_Z - (-5);
			Gyro_Balance=-Gyro_Y;                                  //¸üÐÂÆ½ºâ½ÇËÙ¶È
			
			Accel_Y=atan2(Accel_X,Accel_Z)*180/PI;                 //¼ÆËãÇã½Ç	
			//testGyro_Y = Accel_Y;
			Gyro_Y=Gyro_Y/16.4;                                    //ÍÓÂÝÒÇÁ¿³Ì×ª»»	
			if(Way_Angle==2)		  	Kalman_Filter(Accel_Y,-Gyro_Y);//¿¨¶ûÂüÂË²¨	
			else if(Way_Angle==3)   Yijielvbo(Accel_Y,-Gyro_Y);    //»¥²¹ÂË²¨
			Angle_Balance=angle;                                   //¸üÐÂÆ½ºâÇã½Ç
			Gyro_Turn=Gyro_Z;                                      //¸üÐÂ×ªÏò½ÇËÙ¶È
			Acceleration_Z=Accel_Z;                                //===¸üÐÂZÖá¼ÓËÙ¶ÈÆ	
		}
}

void minibalance_motor_init(void)
{
		RCC->APB2ENR|=1<<3;       //PORTBÊ±ÖÓÊ¹ÄÜ   
		GPIOB->CRH&=0X0000FFFF;   //PORTB12 13 14 15ÍÆÍìÊä³ö
		GPIOB->CRH|=0X22220000;   //PORTB12 13 14 15ÍÆÍìÊä³ö
}

void MiniBalance_PWM_Init(u16 arr,u16 psc)
{		 					 
		minibalance_motor_init();  //³õÊ¼»¯µç»ú¿ØÖÆËùÐèIO
		RCC->APB2ENR|=1<<11;       //Ê¹ÄÜTIM1Ê±ÖÓ    
		RCC->APB2ENR|=1<<2;        //PORTAÊ±ÖÓÊ¹ÄÜ     
		GPIOA->CRH&=0XFFFF0FF0;    //PORTA8 11¸´ÓÃÊä³ö
		GPIOA->CRH|=0X0000B00B;    //PORTA8 11¸´ÓÃÊä³ö
		TIM1->ARR=arr;             //Éè¶¨¼ÆÊýÆ÷×Ô¶¯ÖØ×°Öµ 
		TIM1->PSC=psc;             //Ô¤·ÖÆµÆ÷²»·ÖÆµ
		TIM1->CCMR2|=6<<12;        //CH4 PWM1Ä£Ê½	
		TIM1->CCMR1|=6<<4;         //CH1 PWM1Ä£Ê½	
		TIM1->CCMR2|=1<<11;        //CH4Ô¤×°ÔØÊ¹ÄÜ	 
		TIM1->CCMR1|=1<<3;         //CH1Ô¤×°ÔØÊ¹ÄÜ	  
		TIM1->CCER|=1<<12;         //CH4Êä³öÊ¹ÄÜ	   
		TIM1->CCER|=1<<0;          //CH1Êä³öÊ¹ÄÜ	
		TIM1->BDTR |= 1<<15;       //TIM1±ØÐëÒªÕâ¾ä»°²ÅÄÜÊä³öPWM
		TIM1->CR1=0x8000;          //ARPEÊ¹ÄÜ 
		TIM1->CR1|=0x01;          //Ê¹ÄÜ¶¨Ê±Æ÷1 			
} 

void poll_led_display(void *timer)
{
		unsigned char index[25];	
		static unsigned int tmp;	
	
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

		//	OLED_ShowString(5,50, "            ");
		//	memset(index, '\0', sizeof(index));
		//snprintf((char*)index, sizeof(index), "%4d | %4d", Encoder_Left_total, Encoder_Right_total);
		//	strcpy(index, "xiaopeng");
		//	OLED_ShowString(5,50, index);
		OLED_Refresh_Gram();	
		tmp++;
}

void handle_remote_control(int uart_receive)
{
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
					mTurnKp = 28;
			else if(uart_receive==0x74)
					mTurnKp = 33;
			else if(uart_receive==0x75)
					mTurnKp = 38;
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


