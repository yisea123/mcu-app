#include "usmart.h"
#include "usmart_str.h"
////////////////////////////�û�������///////////////////////////////////////////////
//������Ҫ�������õ��ĺ�����������ͷ�ļ�(�û��Լ����) 
#include "delay.h"		
#include "sys.h"
#include "rtc.h"

//extern void led_set(u8 sta);
//extern void test_fun(void(*ledset)(u8),u8 sta);
extern int send_can_mesg(void);
extern int send_can1_mesg1(uint32_t id);
extern int send_can2_mesg1(uint32_t id);
//extern void report_mesg_to_t8(void);
//extern void add_event(int id);
//extern void del_event(int id);

extern void power_android(uint32_t on);

extern void set_can_num_event(uint32_t num);
extern void android_down(void);

extern void update(void);
extern void updateb(void);

extern void report_mcu_status(void);
extern void enable_debug_tx(uint32_t num);
extern void at(char *at);
extern void mqtt_publish_test(int qos, char *topic, char *payload);

//�������б��ʼ��(�û��Լ����)
//�û�ֱ������������Ҫִ�еĺ�����������Ҵ�
struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS==1 	//���ʹ���˶�д����
	(void*)read_addr,"u32 read_addr(u32 addr)",
	(void*)write_addr,"void write_addr(u32 addr,u32 val)",	 
#endif		   
	(void*)delay_ms,"void delay_ms(u16 nms)",
 	(void*)delay_us,"void delay_us(u32 nus)",	 
//	(void*)report_mesg_to_t8,"void report_mesg_to_t8(void)",
		
  (void*)send_can_mesg,"int send_can_mesg(void)",		
  (void*)send_can1_mesg1,"int send_can1_mesg1(u32 id) id < 2047",	
  (void*)send_can2_mesg1,"int send_can2_mesg1(u32 id) id < 2047",	
//	(void*)add_event,"void add_event(int id);",		
//  (void*)del_event,"void del_event(int id)",		

	(void*)power_android,"void power_android(uint32_t on)",

	(void*)set_can_num_event,"void set_can_num_event(num), set num to start send data to can1.",
	(void*)enable_debug_tx,"void enable_debug_tx(num), make uart printf enable or disable.",
		
	(void*)android_down,"void android_down(void), set mAndroidShutDownPending = 1",
		
	(void*)update,"void update(void), request update mcu rom",	
	(void*)updateb,"void updateb(void), request update bootloader rom",	
	(void*)report_mcu_status,"void report_mcu_status(void), report_mcu_status to android",	 
		
 	(void*)RTC_Set_Time,"u8 RTC_Set_Time(u8 hour,u8 min,u8 sec,u8 ampm)",		   			  	    
 	(void*)RTC_Set_Date,"u8 RTC_Set_Date(u8 year,u8 month,u8 date,u8 week)",		   			  	    
 	(void*)RTC_Set_AlarmA,"void RTC_Set_AlarmA(u8 week,u8 hour,u8 min,u8 sec)",		   			  	    
 	(void*)RTC_Set_WakeUp,"void RTC_Set_WakeUp(u8 wksel,u16 cnt)",		
	(void*)at,"void at(char *at)",
	(void*)mqtt_publish_test,"void mqtt_publish_test(int qos, char* topic, char*payload)", 
};						  
///////////////////////////////////END///////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
//�������ƹ�������ʼ��
//�õ������ܿغ���������
//�õ�����������
struct _m_usmart_dev usmart_dev=
{
	usmart_nametab,
	usmart_init,
	usmart_cmd_rec,
	usmart_exe,
	usmart_scan,
	sizeof(usmart_nametab)/sizeof(struct _m_usmart_nametab),//��������
	0,	  	//��������
	0,	 	//����ID
	1,		//������ʾ����,0,10����;1,16����
	0,		//��������.bitx:,0,����;1,�ַ���	    
	0,	  	//ÿ�������ĳ����ݴ��,��ҪMAX_PARM��0��ʼ��
	0,		//�����Ĳ���,��ҪPARM_LEN��0��ʼ��
};   



















