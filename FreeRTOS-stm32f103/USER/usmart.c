#include "usmart.h"
#include "rfifo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"
#include "uart.h"
#include "list.h"
#include "string.h"

extern void gprs_vcc_off( void );
extern void gprs_vcc_on( void );

extern void gprs_reset_height( void );
extern void gprs_reset_low( void );
extern void gprs_reset( void );
extern void test_mqtt_publish( char *data );
extern unsigned char xmem_used_pecent( unsigned char memx );
extern void print_module_struct_info( void );

typedef enum
{
	ROOT = 0,
	NOBOOT
} vUserType;

typedef enum
{
	DELETETASK = 1,
	SUSPENDTASK,
	RESUMETASK,
	ABORTTASK
} vActionType;

typedef struct commandRecord
{
	char *command;
	int len;
	ListItem_t			xStateListItem;
} cmdRecord;

static List_t xCommandRecordList;
vUserType xUserID = ROOT;
static char xUserName[ 36 ] = { 'r', 'o', 'o', 't', '\0' };
static char pBuf[ 512 ];
Ringfifo uart1fifo;

#if( INCLUDE_xTaskLogLevel == 1 )
/*0~8*/
extern eLogLevel ucOsLogLevel;

void set_os_log_level( char *param )
{
	int i = 0;
	char logLevel = 0;
	
	while( *param == ' ')
		param++;

	while( *param != '\0' )
	{
		if( i == 1 )
		{
			printf("Error Os log level len!\r\n");
			return;
		}
		logLevel = *param;
		param++;
	}

	if( ( logLevel - '0' ) >= 0 && ( logLevel - '0' ) <= 8 )
	{
		ucOsLogLevel = ( eLogLevel ) ( logLevel - '0' );
		printf("OS log level set ok! ucOsLogLevel = %d\r\n", ucOsLogLevel);
	}
	else
	{
		if( logLevel == 0)
		{
			printf("ucOsLogLevel = %d\r\n", ucOsLogLevel);
		}
		else
		{
			printf("oslevel Error! 0~8 is right value\r\n");

		}
	}
}

void set_task_log_level( char *param )
{
	TaskHandle_t target;
		int i = 0, level;
		char taskName[configMAX_TASK_NAME_LEN];
		char logLevel[2];
	
		printf("%s: command = %s\r\n", __func__, param);
		memset(taskName, '\0', sizeof(taskName));
		while( *param == ' ')
			param++;

		while( *param != ' ' )
		{
			if( i == ( configMAX_TASK_NAME_LEN - 1 ) )
			{
				printf("Error task name len!\r\n");
				return;
			}
			taskName[i++] = *param;
			param++;
		}
		taskName[i] = '\0';

		while( *param == ' ')
			param++;

		i = 0;
		while( *param != '\0' )
		{
			if( i == 1 )
			{
				printf("Error log level len!\r\n");
				return;
			}
			logLevel[i++] = *param;
			param++;
		}
		logLevel[i] = '\0';
		level = logLevel[0] - '0';
		
		printf("taskName = %s, log level = %d\r\n", taskName, level);
		target = xTaskGetHandle( taskName );

		if( target )
		{
			if( level >= 0 && level <= 8 )
			{
				vSetTaskLogLevel( target, ( eLogLevel ) level );
			}
			else
			{
				printf("setlevel Error! 0~8 is right value\r\n");
			}
		}
		else
		{
			printf("target = %p error! please input right task name.\r\n", target);
		}
}

#endif

void change_user_name( char *userName )
{
	while( *userName == ' ')
		userName++;

	if( strlen(userName) > 35 || strlen(userName) == 0)
	{
		printf("Current user name is %s\r\n", xUserName);
		printf("Enter su + userName, userNamer must length 1~35 !\r\n");
	}
	else
	{
		memset( xUserName, '\0', sizeof(xUserName));
		strcpy( xUserName, userName );
	}
	
	if( strcmp(xUserName, "root") == 0)
	{
		xUserID = ROOT;
	}
	else
	{
		xUserID = NOBOOT;
	}
}

void printf_system_jiffies( void )
{
	TickType_t tick = xTaskGetTickCount();
	printf("Sytem times from boot is %u (s), system tick = %u (ms).\r\n", 
			portTICK_PERIOD_MS * tick / 1000, portTICK_PERIOD_MS * tick);	
}

__asm void Stm32f103_Force_Reboot( void )
{
/*
  MOV R0, #1           //; 
  MSR FAULTMASK, R0    //; Clear FAULTMASK flag to disable all interrupts
  LDR R0, =0xE000ED0C  //; address of Application Interrupt and Reset Control Register
  LDR R1, =0x05FA0004  //; value of Application Interrupt and Reset Control Register
  STR R1, [R0]         //; system reset by soft ware   
 
deadloop
	B deadloop        //; deadloop
*/
}

void reboot_system( void )
{
	printf("reboot system ...\r\n");
	vTaskDelay( 30 / portTICK_RATE_MS);
	Stm32f103_Force_Reboot();
}

void list_all_task_information( void )
{
		vTaskList( pBuf );
		#if( INCLUDE_xTaskLogLevel == 1 )	
		printf("Name	   Status  Priority  WaterMark	Num  Loglevel\r\n\r\n");
		#else		
		printf("Name       Status  Priority  WaterMark  Num\r\n\r\n");
		#endif
		printf("%s\r\n", pBuf);
}

extern Ringfifo uart2fifo[1];
void list_struct_information( void )
{
		printf("*********struct********\r\n");
		printf("uart1fifo: 		lostBytes(%u), in(%u), out(%u), size(%u)\r\n", 
						uart1fifo.lostBytes, uart1fifo.in, uart1fifo.out, uart1fifo.size);

		printf("uart2fifo: 		lostBytes(%u), in(%u), out(%u), size(%u)\r\n", 
						uart2fifo->lostBytes, uart2fifo->in, uart2fifo->out, uart2fifo->size);

		printf("*********struct********\r\n");	
}

void printf_memory_information( void )
{
		size_t xFreeSize = xPortGetFreeHeapSize();
		float  fUsePercent = 100 - 100 * ( xFreeSize*1.0 / ( configTOTAL_HEAP_SIZE ) );
	
		printf("Memory1 used %f%%, Free Size = %d, Total = %d.\r\n", 
				fUsePercent, xFreeSize, configTOTAL_HEAP_SIZE);
		printf("Memory1 used (%d).\r\n",xmem_used_pecent(0));		
}

void deal_command_for_task( unsigned char *command , vActionType action)
{
	TaskHandle_t target;
	UBaseType_t pre;
	
		char taskName[configMAX_TASK_NAME_LEN];
		pre = uxTaskPriorityGet( NULL );
		printf("%s: command = %s\r\n", __func__, command);
		memset(taskName, '\0', sizeof(taskName));
		while( *command == ' ')
			command++;
		
		strcpy(taskName, (char *)command);
		printf("taskName = %s\r\n", taskName);
		target = xTaskGetHandle( taskName );
		
		if( target )
		{
			switch( action )
			{
				case DELETETASK:
								vTaskDelete( target );
								printf("delete task %s success!\r\n", taskName);
								break;
				case SUSPENDTASK:
								vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );						
								vTaskSuspend(target);
								vTaskPrioritySet( NULL, pre );	
								printf("suspend task %s success!\r\n", taskName);
								break;
				case RESUMETASK:
								vTaskResume(target);
								printf("resume task %s success!\r\n", taskName);
								break;
				case ABORTTASK:
								if( pdTRUE == xTaskAbortDelay(target) )
								{
									printf("wake up task %s from block success!\r\n", taskName);
								}
								else
								{
									printf("wake up task %s from block fail!\r\n", taskName);
								}
								break;
				//case 5:
				//				
				//				break;			
				default:
								break;
			}
		}
		else
		{
			printf("target = %p error! please input right task name.\r\n", target);
		}
}

/*
*  system command list.	
*/
u8 *sys_cmd_tab[]=
{
	"?",
	"help",
	"list",
	"id",
	"hex",
	"dec",
	"runtime",
	/*FreeRTOS*/
	"ps",
	"kill",
	"suspend",
	"resume",
	"abort",
	"exit",
	"struct",
	"memory",
	"reboot",
	"jiffies",
	"su",
	"setlevel",
	"oslevel",
	"gprson",
	"gprsoff",
	"hreset",
	"lreset",
	"reset",
	"mqttp",
};	    

/*
*	author: unknow
* function: usmart_sys_cmd_exe. 
		   run system command, return 0:scuess , othe: fail!
**/
u8 usmart_sys_cmd_exe( u8 *str )
{
UBaseType_t pre;	
	u8 i;
	u8 sfname[MAX_FNAME_LEN];//存放本地函数名
	u8 pnum;
	u8 rval;
	u32 res;  
	
	res = usmart_get_cmdname( str, sfname, &i, MAX_FNAME_LEN );//得到指令及指令长度
	if( res )
	{
		return USMART_FUNCERR;//错误的指令 
	}
	str += i;	 	 			    
	for( i=0; i < sizeof(sys_cmd_tab)/sizeof(sys_cmd_tab[0]); i++ )//支持的系统指令
	{
		if( usmart_strcmp( sfname, sys_cmd_tab[i] )==0 )
			break;
	}
	switch( i )
	{					   
		case 0:
		case 1://帮助指令
#if USMART_USE_HELP
			pre = uxTaskPriorityGet( NULL );
			vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
			printf("          Debug System Command:     \r\n");
			printf("\r\n");			
			printf("?:    print help information.\r\n");
			printf("help: print help information.\r\n");
			printf("md5:	calculate file md5, use md5 + filename.\r\n");			
			printf("list: print available Function name list.\r\n");
			printf("id:   print available Function ID list.\r\n");
			printf("hex:  binary exchange, etc hex + 0x10.\r\n");
			printf("dec:  binary exchange, etc dec + 10.\r\n");
			printf("runtime: 1,open calculate func cost times; 0,colse calculate;\r\n");			
			printf("please input the Function and params just like you coding.\r\n"); 
		
			printf("\r\n----------------FreeRTOS---------------\r\n");
			printf("ps:	 list FreeRTOS all Task information.\r\n");		
			printf("kill:	 kill the task in task name.\r\n");		
			printf("suspend: suspend task by task name.\r\n");	
			printf("resume: resume task from suspend status to ready status.\r\n");		
			printf("abort:  abort task from delay status to ready status.\r\n");	
			printf("struct: print application data sturct information.\r\n");	
			printf("memory: print system memory information.\r\n");	
			printf("baud:   set Baud for Debug Uart.\r\n");
			printf("reboot: reboot FreeRTOS system.\r\n");
			printf("jiffies: print system jiffies.\r\n");
			printf("time: print Date/time from rtc.\r\n");
			printf("su:   change user name.\r\n");
			printf("version: print application version.\r\n");		
			printf("setlevel: setlevel + task name + level num( 0 ~ 8 ).\r\n");	
			printf("oslevel:  oslevel + level num ( 0 ~ 8 ).\r\n");
			printf("testflash: test flash write and read.\r\n");
			printf("----------------FreeRTOS---------------\r\n");		

			printf("\r\n----------------Fatfs---------------\r\n");
			printf("pwd: 	display current directory.\r\n");
			printf("ls: 	display current directory's file and dir.\r\n");
			printf("df: 	display the size of volume be used and free size.\r\n");
			printf("ll: 	display more information about current directory's file and dir.\r\n");			
			printf("cd: 	chang directory, use as: cd + dir_path.\r\n");
			printf("mkdir:  create an directory, use as: mkdir + dir_path.\r\n");
			printf("rm: 	delete an directory, use as: rm + dir_path/file_path.\r\n");
			printf("cat: 	display file's content, use as: cat + file_path.\r\n");
			printf("echo: 	append contents to file, use as: echo xxx > file_path.\r\n");
			printf("touch: 	create a zero size file, use as: touch + file_path.\r\n");
			printf("mount: 	chang driver for fatfs current dir, use as: mount + sd/flash/ram.\r\n");
			printf("dump: 	dump sector data for sd/flash/ram disk, use as: dump + sector number.\r\n");
			printf("fatfs: 	dump all system Fatfs struct information.\r\n");
			printf("cp: 	copy file to dir, use cp file_path + dir_path.\r\n");

			printf("----------------Fatfs---------------\r\n");		
			vTaskDelay( 15 / portTICK_RATE_MS );
 			printf("\r\n----------------Player---------------\r\n");	
			printf("add:	add sound for headphone.\r\n");
			printf("del:	del sound for headphone.\r\n");
			printf("next:	play next music.\r\n");
			printf("pre:	play previous music.\r\n");
			printf("stop:	stop to play current music.\r\n");
			printf("continue:	continue to play the sotp music.\r\n");
			printf("circle:	circle to play current music.\r\n");
			printf("amrrate: set sample rate for AMR music.\r\n");
			printf("adds:	add sound for speaker.\r\n");
			printf("dels:	del sound for speaker..\r\n");
			printf("----------------Player---------------\r\n");				
			//printf("--------------------------------------------- \r\n");
			vTaskPrioritySet( NULL, pre );				
#else
			printf("not support command!\r\n");
#endif
			break;
		case 2:
			printf("---------------Function List--------------- \r\n");
			for( i=0; i < usmart_dev.fnum; i++ )
			{
				printf("%s\r\n", usmart_dev.funs[i].name);
			}
			printf("\r\n");
			break;	 
		case 3:
			printf("\r\n");
			printf("---------------Function ID --------------- \r\n");
			for( i=0; i < usmart_dev.fnum; i++ )
			{
				usmart_get_fname((u8*)usmart_dev.funs[i].name,sfname, &pnum, &rval );//得到本地函数名 
				printf("%s id is:\r\n0X%08X\r\n", sfname, usmart_dev.funs[i].func ); //显示ID
			}
			printf("\r\n");
			break;
		case 4://hex指令
			printf("\r\n");
			usmart_get_aparm( str, sfname, &i );
			if( i == 0 )//参数正常
			{
				i = usmart_str2num( sfname, &res );	   	//记录该参数	
				if( i == 0 )						  	//进制转换功能
				{
					printf("HEX:0X%X\r\n",res);	   	//转为16进制
				}
				else if( i != 4 )
				{
					return USMART_PARMERR;//参数错误.
				}
				else 				   				//参数显示设定功能
				{
					printf("16进制参数显示!\r\n");
					usmart_dev.sptype = SP_TYPE_HEX;  
				}

			}
			else
			{
				return USMART_PARMERR;			//参数错误.
			}
			printf("\r\n"); 
			break;
		case 5://dec指令
			printf("\r\n");
			usmart_get_aparm( str, sfname, &i );
			if( i == 0 )//参数正常
			{
				i = usmart_str2num( sfname, &res );	   	//记录该参数	
				if( i == 0 )						   	//进制转换功能
				{
					printf("DEC:%lu\r\n",res);	   	//转为10进制
				}
				else if( i != 4 )
				{
					return USMART_PARMERR;//参数错误.
				}
				else 				   				//参数显示设定功能
				{
					printf("10进制参数显示!\r\n");
					usmart_dev.sptype = SP_TYPE_DEC;  
				}

			}
			else
			{
				return USMART_PARMERR;			//参数错误. 
			}
			printf("\r\n"); 
			break;	 
		case 6://runtime指令,设置是否显示函数执行时间
			printf("\r\n");
			usmart_get_aparm( str, sfname, &i );
			if( i == 0 )//参数正常
			{
				i = usmart_str2num( sfname, &res );	   		//记录该参数	
				if( i == 0 )						   		//读取指定地址数据功能
				{
					if( USMART_ENTIMX_SCAN == 0 )
					{
						printf("\r\nError! \r\nTo EN RunTime function,Please set USMART_ENTIMX_SCAN = 1 first!\r\n");//报错
					}
					else
					{
						usmart_dev.runtimeflag = res;
						if( usmart_dev.runtimeflag )
						{
							printf("Run Time Calculation ON\r\n");
						}
						else
						{
							printf("Run Time Calculation OFF\r\n"); 
						}
					}
				}
				else
				{
					return USMART_PARMERR;   			//未带参数,或者参数错误	 
				}
			}
			else 
			{
				return USMART_PARMERR;				//参数错误. 
			}
			printf("\r\n"); 
			break;	 

		/*Add For FreeRTOS*/
		case 7:
			( void ) list_all_task_information();
			break;
		case 8:
			( void ) deal_command_for_task(str, DELETETASK);
			break;
		case 9:
			( void ) deal_command_for_task(str, SUSPENDTASK);
			break;
		case 10:
			( void ) deal_command_for_task(str, RESUMETASK);
			break;		
		case 11:
			( void ) deal_command_for_task(str, ABORTTASK);
			break;			
		case 12://exit
			//( void ) deal_command_for_task(str, 5);
			break;			
		case 13:
			( void ) list_struct_information();
			break;		
		case 14:
			( void ) printf_memory_information();
			break;		
		case 15:
			( void ) reboot_system();
			break;		
		case 16:
			( void ) printf_system_jiffies();
			break;
			
		case 17:
			( void ) change_user_name((char *)str);
			break;			

		case 18:
			( void ) set_task_log_level((char *)str);
			break;	
			
		case 19:
			( void ) set_os_log_level((char *)str);
			break;		

		case 20:

			gprs_vcc_on();
			break;
			
		case 21:
			gprs_vcc_off();
			break;
			
		case 22:
			gprs_reset_height();
			break;
			
		case 23:
			gprs_reset_low();
			break;

		case 24:
			gprs_reset();
			break;
			
		case 25:
			test_mqtt_publish( (char*)str );
			break;		
		/*Add For FreeRTOS*/
		default://非法指令
			return USMART_FUNCERR;
	}
	return 0;
}

////////////////////////////////////////////////////////////////////////////////////////
//移植注意:本例是以stm32为例,如果要移植到其他mcu,请做相应修改.
//usmart_reset_runtime,清除函数运行时间,连同定时器的计数寄存器以及标志位一起清零.并设置重装载值为最大,以最大限度的延长计时时间.
//usmart_get_runtime,获取函数运行时间,通过读取CNT值获取,由于usmart是通过中断调用的函数,所以定时器中断不再有效,此时最大限度
//只能统计2次CNT的值,也就是清零后+溢出一次,当溢出超过2次,没法处理,所以最大延时,控制在:2*计数器CNT*0.1ms.对STM32来说,是:13.1s左右
//其他的:TIM4_IRQHandler和Timer2_Init,需要根据MCU特点自行修改.确保计数器计数频率为:10Khz即可.另外,定时器不要开启自动重装载功能!!

#if 1
//复位runtime
//需要根据所移植到的MCU的定时器参数进行修改
void usmart_reset_runtime(void)
{
	/*
	TIM_ClearFlag(TIM4,TIM_FLAG_Update);//清除中断标志位 
	TIM_SetAutoreload(TIM4,0XFFFF);//将重装载值设置到最大
	TIM_SetCounter(TIM4,0);		//清空定时器的CNT
	usmart_dev.runtime=0;
		*/
}
//获得runtime时间
//返回值:执行时间,单位:0.1ms,最大延时时间为定时器CNT值的2倍*0.1ms
//需要根据所移植到的MCU的定时器参数进行修改
u32 usmart_get_runtime(void)
{
	/*
	if(TIM_GetFlagStatus(TIM4,TIM_FLAG_Update)==SET)//在运行期间,产生了定时器溢出
	{
		usmart_dev.runtime+=0XFFFF;
	}
	usmart_dev.runtime+=TIM_GetCounter(TIM4);
	return usmart_dev.runtime;		//返回计数值
	*/
	return 0;
}
//下面这两个函数,非USMART函数,放到这里,仅仅方便移植. 
//定时器4中断服务程序	 

void usamrt_debug_task( void *argc ) 
{
	/*usamrt has the heighest priroty.*/
	vSetTaskLogLevel(NULL, eLogLevel_0);
	vListInitialise( &xCommandRecordList );
	printf("%s start...\r\n", __func__);
	usmart_dev.sptype = 0;	//0,10进制;1,16进制;
	rfifo_init( &uart1fifo, 128 );	
	
	/*Run in lowest priroty, set system log level*/
	ucOsLogLevel = eLogLevel_4;
	
	while (1) 
	{
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		usmart_dev.scan( 0 );
	}
}

#endif

void usmart_init(void)
{
	;
}

/*
*	author: unknow
* function: usmart_cmd_rec. 
		   search function accodring char *str, return 0: scuess , other: fail!
**/
u8 usmart_cmd_rec( u8* str ) 
{
	u8 sta, i, rval;//状态	 
	u8 rpnum, spnum;
	u8 rfname[MAX_FNAME_LEN];//暂存空间,用于存放接收到的函数名  
	u8 sfname[MAX_FNAME_LEN];//存放本地函数名

	sta = usmart_get_fname( str, rfname, &rpnum, &rval );//得到接收到的数据的函数名及参数个数	  
	if( sta )
	{
		return sta;//错误
	}
	for( i = 0; i < usmart_dev.fnum; i++ )
	{
		sta = usmart_get_fname( (u8*)usmart_dev.funs[i].name, sfname, &spnum, &rval );//得到本地函数名及参数个数
		if( sta )
		{
			return sta;//本地解析有误	  
		}
		if( usmart_strcmp( sfname, rfname ) == 0 )//相等
		{
			if( spnum > rpnum )
			{
				return USMART_PARMERR;//参数错误(输入参数比源函数参数少)
			}
			usmart_dev.id = i;
			break;
		}	
	}
	if( i == usmart_dev.fnum )
	{
		return USMART_NOFUNCFIND;	//未找到匹配的函数
 	}
	sta = usmart_get_fparam( str, &i );					//得到函数参数个数	
	if( sta )
	{
		return sta;								//返回错误
	}
	usmart_dev.pnum = i;								//参数个数记录

    return USMART_OK;
}

//usamrt执行函数
//该函数用于最终执行从串口收到的有效函数.
//最多支持10个参数的函数,更多的参数支持也很容易实现.不过用的很少.一般5个左右的参数的函数已经很少见了.
//该函数会在串口打印执行情况.以:"函数名(参数1，参数2...参数N)=返回值".的形式打印.
//当所执行的函数没有返回值的时候,所打印的返回值是一个无意义的数据.
void usmart_exe(void)
{
	u8 id, i;
	u32 res;		   
	u32 temp[MAX_PARM];//参数转换,使之支持了字符串 
	u8 sfname[MAX_FNAME_LEN];//存放本地函数名
	u8 pnum,rval;
	
	id = usmart_dev.id;
	if( id >= usmart_dev.fnum )
	{
		return;//不执行.
	}
	usmart_get_fname( (u8*)usmart_dev.funs[id].name, 
		sfname, &pnum, &rval );//得到本地函数名,及参数个数 

	for( i=0; i < pnum; i++ )//输出参数
	{
		if( usmart_dev.parmtype & ( 1 << i ) )//参数是字符串
		{	//is string
			temp[i] = (u32)&( usmart_dev.parm[usmart_get_parmpos(i)] );
		}
		else
		{	//is number
			temp[i] = *(u32*)( usmart_dev.parm+usmart_get_parmpos(i) );
			if( usmart_dev.sptype == SP_TYPE_DEC )
				;//printf("%lu",temp[i]);//10进制参数显示
			else
				;   
		}
	}
	usmart_reset_runtime();	//计时器清零,开始计时
	switch( usmart_dev.pnum )
	{
		case 0://无参数(void类型)											  
			res = (*(u32(*)())usmart_dev.funs[id].func)();
			break;
	    case 1://有1个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0]);
			break;
	    case 2://有2个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1]);
			break;
	    case 3://有3个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2]);
			break;
	    case 4://有4个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3]);
			break;
	    case 5://有5个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4]);
			break;
	    case 6://有6个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5]);
			break;
	    case 7://有7个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6]);
			break;
	    case 8://有8个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7]);
			break;
	    case 9://有9个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8]);
			break;
	    case 10://有10个参数
			res = (*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8],temp[9]);
			break;
	}
	usmart_get_runtime();
	if( rval == 1 )
	{
		if( usmart_dev.sptype == SP_TYPE_DEC )
		{
			printf("=%lu;\r\n",res); 
		}
		else 
		{
			printf("=0X%X;\r\n",res);    
		}
	}

	if( usmart_dev.runtimeflag )
	{ 
		printf("Function Run Time:%d.%1dms\r\n",usmart_dev.runtime/10,usmart_dev.runtime%10);
	}	
}

static unsigned char USART_RX_BUF[ 100 ];

void usmart_scan( int uart1DataLen )
{
	static int index = 0;
	static unsigned int num = 0xffffffff;
	cmdRecord *cmdItem;
	unsigned char status, len, add = 0;
	
	while( rfifo_len( &uart1fifo ) > 0 ) 
	{
		rfifo_get( &uart1fifo, USART_RX_BUF + index, 1 );

		//printf("%02x ", USART_RX_BUF[index]);

		if( index >= 2 && 
			( USART_RX_BUF[index] == 0x44 ||
			USART_RX_BUF[index] == 0x43 ||
			USART_RX_BUF[index] == 0x41 ||
			USART_RX_BUF[index] == 0x42 ) && 
			USART_RX_BUF[index-1] == 0x5b &&
			USART_RX_BUF[index-2] == 0x1b )
		{
			if( USART_RX_BUF[index] == 0x44 )
			{
				/*avoid right and left*/
				printf("%c", 0x44);
				printf("%c", 0x1b);
				printf("%c", 0x5b);
				printf("%c", 0x43);
				index -= 2;
				return;
			}
			if( USART_RX_BUF[index] == 0x43 )
			{
				printf("%c", 0x43);
				printf("%c", 0x1b);
				printf("%c", 0x5b);
				printf("%c", 0x44);
				index -= 2;
				return;
			}			
			if( listLIST_IS_EMPTY( &xCommandRecordList ) != pdFALSE )
			{
				printf(" ");				
			}
			else
			{
				cmdItem = NULL;
				/*UP*/
				if( USART_RX_BUF[index] == 0x41 )
				{
					listGET_OWNER_OF_NEXT_ENTRY( cmdItem, &xCommandRecordList );
				}
				/*DOWN*/
				else if( USART_RX_BUF[index] == 0x42 )
				{
					listGET_OWNER_OF_PRE_ENTRY( cmdItem, &xCommandRecordList );
				}
				if( cmdItem )
				{
					/*delete before command.*/
					if( index > 3 )
					{
						int i;						
						printf("%c", 0x20);
						for( i = 0; i < ( index - 2 ); i++ )
						{
							printf("%c", 0x08);
							/*clear an char*/
							printf("%c", 0x20);
							printf("%c", 0x08);							
						}						
						printf("%s ", cmdItem->command);
					}
					else
					{
						printf(" %s ", cmdItem->command);
					}
					memset( USART_RX_BUF, '\0', sizeof( USART_RX_BUF ) );
					memcpy( USART_RX_BUF, cmdItem->command, cmdItem->len );
					index = cmdItem->len;
				}
			}			
			continue;
		}
		
		/*check if is backspace key*/
		if( USART_RX_BUF[index] == 0x08 )
		{
				if( index > 0)
				{
					index--;
					printf("%c", 0x08);
					/*clear an char*/
					printf("%c", 0x20);
					printf("%c", 0x08);					
				}
				continue;				
		}
		
		index++;
		/*if character is enter, just print it.*/
		if( USART_RX_BUF[index - 1] != 0x0d )
		{
			printf("%c", USART_RX_BUF[index - 1]);
		}		
		else
		{		
			if( index == 1 || index > 69 )
			{
				if( index > 69 )
				{
					printf("Error command!\r\n");
				}
				//root@octopus-t8v10:/system # 				
				printf("\r\n%s #", xUserName);
				index = 0;
				continue;
			}
			else
			{
				USART_RX_BUF[index - 1] = '\0';
			}
						
			printf("\r\n");
			/*parse buffer to check it if match functions in defined list*/
			status = usmart_dev.cmd_rec( USART_RX_BUF );	
			if( status == USMART_OK ) 
			{		
				/*match function*/
				add = 1;
				usmart_dev.exe();
			}
			else 
			{  
				/*parse buffer to check it if match commands in defined list*/
				len = usmart_sys_cmd_exe( USART_RX_BUF );

				if( len == USMART_OK)
				{
					/*match command*/
					add = 1;
				}
				if( len != USMART_FUNCERR ) 
				{
					status = len;
				}
				if( status )
				{
					switch( status )
					{
						case USMART_FUNCERR:
							printf("Error command!\r\n");
							break;	
						case USMART_PARMERR:
							printf("Error paramer!\r\n");
							break;				
						case USMART_PARMOVER:
							printf("Error paramer too much!\r\n");						
							break;		
						case USMART_NOFUNCFIND:
							printf("Error function!\r\n");	
							break;		
					}
				}
			}

			if( add )
			{
				cmdRecord *pxFirstCmd, *pxNextCmd;
			
				cmdItem = NULL;
				
				if( listCURRENT_LIST_LENGTH( &xCommandRecordList ) > ( UBaseType_t ) 0 )
				{
					listGET_OWNER_OF_NEXT_ENTRY( pxFirstCmd, &xCommandRecordList );
				
					do
					{
						listGET_OWNER_OF_NEXT_ENTRY( pxNextCmd, &xCommandRecordList );
						if( memcmp(pxNextCmd->command, USART_RX_BUF, strlen( ( char* )USART_RX_BUF ) ) == 0 )
						{
							uxListRemove( &( pxNextCmd->xStateListItem ) );
							cmdItem = pxNextCmd;
							break;
						}

					} while( pxNextCmd != pxFirstCmd );
				}
				if( cmdItem == NULL )
				{
					cmdItem =  pvPortMalloc( sizeof( cmdRecord ) );
					if( cmdItem )
					{
						cmdItem->command = pvPortMalloc( index );
						if( cmdItem->command )
						{				
							vListInitialiseItem( &( cmdItem->xStateListItem ) );
							listSET_LIST_ITEM_OWNER( &( cmdItem->xStateListItem ), cmdItem );
							listSET_LIST_ITEM_VALUE( &( cmdItem->xStateListItem ), num );
							num--;
							memset( cmdItem->command, '\0', index);
							memcpy( cmdItem->command, USART_RX_BUF, index);
							cmdItem->len = index;
							
							vListInsert( &xCommandRecordList, &( cmdItem->xStateListItem ) );
							(&xCommandRecordList)->pxIndex = ( &( cmdItem->xStateListItem ) )->pxPrevious;
						}
						else
						{
							vPortFree( cmdItem );
						}
					}
				}
				else
				{				
					listSET_LIST_ITEM_VALUE( &( cmdItem->xStateListItem ), num );
					num--;
					vListInsert( &xCommandRecordList, &( cmdItem->xStateListItem ) );
					(&xCommandRecordList)->pxIndex = ( &( cmdItem->xStateListItem ) )->pxPrevious;
				}
			}
			
			index = 0;
			printf("%s #", xUserName);			
		}
	}
}

#if USMART_USE_WRFUNS==1 

u32 read_addr( u32 addr )
{
	return *( u32* ) addr;
}

void write_addr( u32 addr,u32 val )
{
	*( u32* ) addr = val; 	
}

#endif













