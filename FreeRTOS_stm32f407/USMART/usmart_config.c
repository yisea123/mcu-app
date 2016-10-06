#include "usmart.h"
#include "usmart_str.h" 
#include "fattester.h"
#include "delay.h"						
 
//extern void usmart_test(void);
//extern void test_put_msg(char cmd, char *text);
extern void vSetPrintfTime(int time);
extern void Save_String_To_Flash( unsigned int addr, const char *data );
extern void Read_String_From_Flash( unsigned int addr, int size );

struct _m_usmart_nametab usmart_nametab[]=
{
#if USMART_USE_WRFUNS
	(void*)read_addr, "u32 read_addr(u32 addr)",
	(void*)write_addr, "void write_addr(u32 addr,u32 val)",	 
#endif
	(void*)delay_ms, "void delay_ms(u16 nms)",
	(void*)delay_us, "void delay_us(u32 nus)",
	(void*)vSetPrintfTime, "void vSetPrintfTime(int time)",
	(void*)Save_String_To_Flash, "void Save_String_To_Flash( unsigned int addr, const char *data )",
	(void*)Read_String_From_Flash, "void Read_String_From_Flash( unsigned int addr, int size )",
	(void*)mf_mount,"u8 mf_mount(u8* path,u8 mt)", 
	(void*)mf_open,"u8 mf_open(u8*path,u8 mode)", 
	(void*)mf_close,"u8 mf_close(void)", 
	(void*)mf_read,"u8 mf_read(u16 len)", 
	(void*)mf_write,"u8 mf_write(u8*dat,u16 len)", 
	(void*)mf_opendir,"u8 mf_opendir(u8* path)", 
	(void*)mf_closedir,"u8 mf_closedir(void)", 
	(void*)mf_readdir,"u8 mf_readdir(void)", 
	(void*)mf_scan_files,"u8 mf_scan_files(u8 * path)", 
	(void*)mf_showfree,"u32 mf_showfree(u8 *drv)", 
	(void*)mf_lseek,"u8 mf_lseek(u32 offset)", 
	(void*)mf_tell,"u32 mf_tell(void)", 
	(void*)mf_size,"u32 mf_size(void)", 
	(void*)mf_mkdir,"u8 mf_mkdir(u8*pname)", 
	(void*)mf_fmkfs,"u8 mf_fmkfs(u8* path,u8 mode,u16 au)", 
	(void*)mf_unlink,"u8 mf_unlink(u8 *pname)", 
	(void*)mf_rename,"u8 mf_rename(u8 *oldname,u8* newname)", 
	(void*)mf_getlabel,"void mf_getlabel(u8 *path)", 
	(void*)mf_setlabel,"void mf_setlabel(u8 *path)", 
	(void*)mf_gets,"void mf_gets(u16 size)", 
	(void*)mf_putc,"u8 mf_putc(u8 c)", 
	(void*)mf_puts,"u8 mf_puts(u8*c)", 		
	(void*)mf_chdir,"u8 mf_chdir(u8* path)", 
	(void*)mf_chdrive,"u8 mf_chdrive(u8* path)", 
	(void*)mf_getcwd,"u8 mf_getcwd( void )", 
	(void*)mf_stat,"u8 mf_stat( u8* path )",		
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



















