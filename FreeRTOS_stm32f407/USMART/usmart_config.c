#include "usmart.h"
#include "usmart_str.h" 

#include "delay.h"						
 
//extern void usmart_test(void);
//extern void test_put_msg(char cmd, char *text);
extern void vSetPrintfTime(int time);

struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS
	(void*)read_addr, "u32 read_addr(u32 addr)",
	(void*)write_addr, "void write_addr(u32 addr,u32 val)",	 
#endif
	(void*)delay_ms, "void delay_ms(u16 nms)",
	(void*)delay_us, "void delay_us(u32 nus)",
	(void*)vSetPrintfTime, "void vSetPrintfTime(int time)",
//	(void*)usmart_test,"void usmart_test(void)",
//	(void*)test_put_msg,"void test_put_msg(char cmd, char *text)",		
};						  

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



















