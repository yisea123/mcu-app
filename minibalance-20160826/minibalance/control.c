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
				  delay_50=0,delay_flag=0;                     //¸øÖ÷º¯ÊýÌá¹©¾«×¼ÑÓÊ±
					 if (((++delay_50)%400 == 0)) {
						 LED = !LED;
					 }
				 }
			 }
	 		if(0 && Flag_Target==1)                                                  //5ms¶ÁÈ¡Ò»´ÎÍÓÂÝÒÇºÍ¼ÓËÙ¶È¼ÆµÄÖµ£¬¸ü¸ßµÄ²ÉÑùÆµÂÊ¿ÉÒÔ¸ÄÉÆ¿¨¶ûÂüÂË²¨ºÍ»¥²¹ÂË²¨µÄÐ§¹û
			{
				Get_Angle(Way_Angle);                                               //===¸üÐÂ×ËÌ¬	
				return 0;	                                               
			}       
			Flag_Qian = irq_qian;
			Flag_Hou = irq_hou;
			Flag_Right = irq_right;
			Flag_Left = irq_left;
			
			Encoder_Left=-Read_Encoder(2);                                      //===¶ÁÈ¡±àÂëÆ÷µÄÖµ£¬ÒòÎªÁ½¸öµç»úµÄÐý×ªÁË180¶ÈµÄ£¬ËùÒÔ¶ÔÆäÖÐÒ»¸öÈ¡·´£¬±£Ö¤Êä³ö¼«ÐÔÒ»ÖÂ
			Encoder_Right=Read_Encoder(4);
	
			Encoder_Left_total+= Encoder_Left;
			Encoder_Right_total+=  Encoder_Right;
			if (Encoder_Left_total > 9000) Encoder_Left_total = 0;
			if (Encoder_Right_total > 9000) Encoder_Right_total = 0;
			Get_Angle(Way_Angle);                                    //===¸üÐÂ×ËÌ¬	
  		Voltage=Get_battery_volt(); 

 			Balance_Pwm =balance(Angle_Balance,Gyro_Balance);                   //===Æ½ºâPID¿ØÖÆ	
		 	Velocity_Pwm=velocity(Encoder_Left,Encoder_Right);                  //===ËÙ¶È»·PID¿ØÖÆ	 ¼Ç×¡£¬ËÙ¶È·´À¡ÊÇÕý·´À¡£¬¾ÍÊÇÐ¡³µ¿ìµÄÊ±ºòÒªÂýÏÂÀ´¾ÍÐèÒªÔÙÅÜ¿ìÒ»µã
  	  		Turn_Pwm    = turn(Encoder_Left,Encoder_Right,Gyro_Turn);            //===×ªÏò»·PID¿ØÖÆ     
 		  	
			if (1) {
				Moto1=Balance_Pwm+Velocity_Pwm-Turn_Pwm;                            //===¼ÆËã×óÂÖµç»ú×îÖÕPWM
 	  			Moto2=Balance_Pwm+Velocity_Pwm+Turn_Pwm;                            //===¼ÆËãÓÒÂÖµç»ú×îÖÕPWM
			}
			
   		Xianfu_Pwm();          
			                                             //===PWMÏÞ·ù
//			if(Pick_Up(Acceleration_Z, Angle_Balance, Encoder_Left, Encoder_Right))//===¼ì²éÊÇ·ñÐ¡³µ±»ÄÇÆð
//				Flag_Stop=1;	                                                      //===Èç¹û±»ÄÃÆð¾Í¹Ø±Õµç»ú
//			if(Put_Down(Angle_Balance, Encoder_Left, Encoder_Right))              //===¼ì²éÊÇ·ñÐ¡³µ±»·ÅÏÂ
//				Flag_Stop=0;	                                                      //===Èç¹û±»·ÅÏÂ¾ÍÆô¶¯µç»ú

      if(Turn_Off(Angle_Balance,Voltage)==0)                              //===Èç¹û²»´æÔÚÒì³£
 				Set_Pwm(Moto1,Moto2); 
			Key_Scan();    
			EXTI_ClearITPendingBit(EXTI_Line7);		
	}
	return 0;
}
  /**************************************************************************
×÷Õß£ºÆ½ºâÐ¡³µÖ®¼Ò
ÎÒµÄÌÔ±¦Ð¡µê£ºhttp://shop114407458.taobao.com/
**************************************************************************/
int TIM1_UP_IRQHandler(void)  
{    
	if(TIM1->SR&0X0001)//5ms¶¨Ê±ÖÐ¶Ï
	{   
		  TIM1->SR&=~(1<<0);                                       //===Çå³ý¶¨Ê±Æ÷1ÖÐ¶Ï±êÖ¾Î»		 		
	}       	
	 return 0;	  
} 


/**************************************************************************
º¯Êý¹¦ÄÜ£º¾ø¶ÔÖµº¯Êý
Èë¿Ú²ÎÊý£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
int myabs(int a)
{ 		   
	  int temp;
		if(a<0)  temp=-a;  
	  else temp=a;
	  return temp;
}

/**************************************************************************
º¯Êý¹¦ÄÜ£ºÖ±Á¢PD¿ØÖÆ
Èë¿Ú²ÎÊý£º½Ç¶È¡¢½ÇËÙ¶È
·µ»Ø  Öµ£ºÖ±Á¢¿ØÖÆPWM
×÷    Õß£ºÆ½ºâÐ¡³µÖ®¼Ò
**************************************************************************/

float kp0=175, kd0=0.60;

int balance(float Angle,float Gyro)
{  
   float Bias;
	 int balance;
	 Bias=Angle-ZHONGZHI;       //===Çó³öÆ½ºâµÄ½Ç¶ÈÖÐÖµ ºÍ»úÐµÏà¹Ø
	 balance=kp0*Bias+Gyro*kd0;   //===¼ÆËãÆ½ºâ¿ØÖÆµÄµç»úPWM  PD¿ØÖÆ   kpÊÇPÏµÊý kdÊÇDÏµÊý 
	 return balance;
}

/**************************************************************************
º¯Êý¹¦ÄÜ£ºËÙ¶ÈPI¿ØÖÆ ÐÞ¸ÄÇ°½øºóÍËÒ£¿ØËÙ¶È£¬ÇëÐÞTarget_Velocity£¬±ÈÈç£¬¸Ä³É60¾Í±È½ÏÂýÁË
Èë¿Ú²ÎÊý£º×óÂÖ±àÂëÆ÷¡¢ÓÒÂÖ±àÂëÆ÷
·µ»Ø  Öµ£ºËÙ¶È¿ØÖÆPWM
×÷    Õß£ºÆ½ºâÐ¡³µÖ®¼Ò
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

/**************************************************************************
º¯Êý¹¦ÄÜ£º×ªÏò¿ØÖÆ  ÐÞ¸Ä×ªÏòËÙ¶È£¬ÇëÐÞ¸ÄTurn_Amplitude¼´¿É
Èë¿Ú²ÎÊý£º×óÂÖ±àÂëÆ÷¡¢ÓÒÂÖ±àÂëÆ÷¡¢ZÖáÍÓÂÝÒÇ
·µ»Ø  Öµ£º×ªÏò¿ØÖÆPWM
×÷    Õß£ºÆ½ºâÐ¡³µÖ®¼Ò
**************************************************************************/


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
		                                 
  	//=============×ªÏòPD¿ØÖÆÆ÷=======================//
		Turn=Turn_Target*mTurnKp-gyro*mTurnKd;                 //===½áºÏZÖáÍÓÂÝÒÇ½øÐÐPD¿ØÖÆ
	 	return Turn;

  /*
 	static float Encoder_temp, Turn, Kp = 30, Kd, Bias, Turn_Target, Turn_Count, Turn_Convert=0.7;

	float Turn_Amplitude=50/Flag_sudu;

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
			Turn_Convert=0.4;
			Turn_Count=0;
			Encoder_temp=0;
	}
			
	if(1==Flag_Left)	           
		Turn_Target-=Turn_Convert;        //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý
	else if(1==Flag_Right)	     
		Turn_Target+=Turn_Convert;        //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý
	else 
		Turn_Target=0;                                            //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾Ý

	if(Turn_Target>Turn_Amplitude)  
		Turn_Target=Turn_Amplitude;    //===×ªÏòËÙ¶ÈÏÞ·ù
  	if(Turn_Target<-Turn_Amplitude) 
		Turn_Target=-Turn_Amplitude;   //===×ªÏòËÙ¶ÈÏÞ·ù

 	if(Flag_Qian==1||Flag_Hou==1)  
		Kd=0.6;                         //===½ÓÊÕ×ªÏòÒ£¿ØÊý¾ÝÖ±Á¢ÐÐ×ßµÄÊ±ºòÔö¼ÓÍÓÂÝÒÇ¾Í¾ÀÕý    
	else 
		Kd=0; 

 	Bias = gyro - 0; 
 	//Turn = -Bias*Kp;
	Turn_Target=0;
	Turn=Turn_Target*Kp - Bias*Kd;                 //===½áºÏZÖáÍÓÂÝÒÇ½øÐÐPD¿ØÖÆ

 	return Turn;
  */
}
/**************************************************************************
º¯Êý¹¦ÄÜ£º¸³Öµ¸øPWM¼Ä´æÆ÷
Èë¿Ú²ÎÊý£º×óÂÖPWM¡¢ÓÒÂÖPWM
·µ»Ø  Öµ£ºÎÞ
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

/**************************************************************************
º¯Êý¹¦ÄÜ£ºÒì³£¹Ø±Õµç»ú
Èë¿Ú²ÎÊý£ºÇã½ÇºÍµçÑ¹
·µ»Ø  Öµ£º1£ºÒì³£  0£ºÕý³£
**************************************************************************/

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

/**************************************************************************
º¯Êý¹¦ÄÜ£º¼ì²âÐ¡³µÊÇ·ñ±»ÄÃÆð
Èë¿Ú²ÎÊý£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
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
		printf("%s: flag=%d, accel=%f, myabs(l,e)=%d, angle=%f!\r\n", __func__, flag, Acceleration, myabs(encoder_left+encoder_right),Angle);	
	 }

	return 0;
}

/**************************************************************************
º¯Êý¹¦ÄÜ£º¼ì²âÐ¡³µÊÇ·ñ±»·ÅÏÂ
Èë¿Ú²ÎÊý£ºint
·µ»Ø  Öµ£ºunsigned int
**************************************************************************/
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





/**************************************************************************
º¯Êý¹¦ÄÜ£º»ñÈ¡½Ç¶È
Èë¿Ú²ÎÊý£º»ñÈ¡½Ç¶ÈµÄËã·¨ 1£ºÎÞ  2£º¿¨¶ûÂü 3£º»¥²¹ÂË²¨
·µ»Ø  Öµ£ºÎÞ
**************************************************************************/
extern float testGyro_Y;
void Get_Angle(u8 way)
{ 
      //float Accel_Y,Accel_X,Accel_Z,Gyro_Y;
	  float Accel_Y,Accel_X,Accel_Z,Gyro_Y,Gyro_Z;      
	  Temperature=Read_Temperature(); 	  
	  if(way==1)                                      //DMPÃ»ÓÐÉæ¼°µ½ÑÏ¸ñµÄÊ±ÐòÎÊÌâ£¬ÔÚÖ÷º¯Êý¶ÁÈ¡
	  {	
			Read_DMP();                      //===¶ÁÈ¡¼ÓËÙ¶È¡¢½ÇËÙ¶È¡¢Çã½Ç
			Angle_Balance=Pitch;             //===¸üÐÂÆ½ºâÇã½Ç
			Gyro_Balance=gyro[1];            //===¸üÐÂÆ½ºâ½ÇËÙ¶È
			Gyro_Turn=gyro[2];               //===¸üÐÂ×ªÏò½ÇËÙ¶È
			Acceleration_Z=accel[2];         //===¸üÐÂZÖá¼ÓËÙ¶È¼Æ		
	  }			
      else
      {

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

  /*
			Gyro_Y=(I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_GYRO_YOUT_L);    //¶ÁÈ¡YÖáÍÓÂÝÒÇ
			Accel_Z=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_ZOUT_L);    //¶ÁÈ¡ZÖáÍÓÂÝÒÇ
		  	Accel_X=(I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_H)<<8)+I2C_ReadOneByte(devAddr,MPU6050_RA_ACCEL_XOUT_L); //¶ÁÈ¡XÖá¼ÓËÙ¶È¼Ç
	 		
			if(Gyro_Y>32768)  Gyro_Y-=65536;     //Êý¾ÝÀàÐÍ×ª»»  Ò²¿ÉÍ¨¹ýshortÇ¿ÖÆÀàÐÍ×ª»»
			if(Accel_Z>32768)  Accel_Z-=65536;     //Êý¾ÝÀàÐÍ×ª»»
	  		if(Accel_X>32768) Accel_X-=65536;    //Êý¾ÝÀàÐÍ×ª»»
	   		
			Accel_Y=atan2(Accel_X,Accel_Z)*180/PI;                 //¼ÆËãÓëµØÃæµÄ¼Ð½Ç	
		  	Gyro_Y=Gyro_Y/16.4; 
			                                   //ÍÓÂÝÒÇÁ¿³Ì×ª»»	
      		if(Way_Angle==2)		  
				Kalman_Filter(Accel_Y,-Gyro_Y);//¿¨¶ûÂüÂË²¨	
			else if(Way_Angle==3)   
				Yijielvbo(Accel_Y,-Gyro_Y);    //»¥²¹ÂË²¨
	    	
			Angle_Balance=angle;                                   //¸üÐÂÆ½ºâÇã½Ç
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
	RCC->APB2ENR|=1<<3;       //PORTBÊ±ÖÓÊ¹ÄÜ   
	GPIOB->CRH&=0X0000FFFF;   //PORTB12 13 14 15ÍÆÍìÊä³ö
	GPIOB->CRH|=0X22220000;   //PORTB12 13 14 15ÍÆÍìÊä³ö
}
void MiniBalance_PWM_Init(u16 arr,u16 psc)
{		 					 
	MiniBalance_Motor_Init();  //³õÊ¼»¯µç»ú¿ØÖÆËùÐèIO
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

/**************************************************************************
º¯Êý¹¦ÄÜ£º°´¼üÐÞ¸ÄÐ¡³µÔËÐÐ×´Ì¬ 
Èë¿Ú²ÎÊý£ºÎÞ
·µ»Ø  Öµ£ºÎÞ
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
	}//µ¥»÷¿ØÖÆÐ¡³µµÄÆôÍ£
	if(tmp==2) {
		Flag_Show=!Flag_Show;
		LED=~LED;
		printf("%s: double click\r\n", __func__);//Ë«»÷¿ØÖÆÐ¡³µµÄÏÔÊ¾×´Ì¬
		if (isKp0) {
			kp0 -= 10; 
		} else { 
			kd0 -= 0.1; 
		}
	}
	tmp2=Long_Press();                   
  	if(tmp2==1) /*Bi_zhang=!Bi_zhang,*/ isKp0 = !isKp0, printf("%s: long click\r\n", __func__), LED=~LED;		//³¤°´¿ØÖÆÐ¡³µÊÇ·ñ½øÈë³¬Éù²¨±ÜÕÏÄ£Ê½ 
}






