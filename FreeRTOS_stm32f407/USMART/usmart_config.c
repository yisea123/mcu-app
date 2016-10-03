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
	sizeof(usmart_nametab)/sizeof(struct _m_usmart_nametab),//函数数量
	0,	  	//参数数量
	0,	 	//函数ID
	1,		//参数显示类型,0,10进制;1,16进制
	0,		//参数类型.bitx:,0,数字;1,字符串	    
	0,	  	//每个参数的长度暂存表,需要MAX_PARM个0初始化
	0,		//函数的参数,需要PARM_LEN个0初始化
};   



















