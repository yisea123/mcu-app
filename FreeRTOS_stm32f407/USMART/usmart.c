#include "usmart.h"
#include "rfifo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"
#include "usart.h"
#include "list.h"

//#include "usart.h"
extern Ringfifo mLogFifo;
extern void Printf_Application_Version( void );
Ringfifo uart1fifo;

u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	 

//////////////////////////////////////////////////////////////////////////////////	 
//升级说明
//V1.4
//增加了对参数为string类型的函数的支持.适用范围大大提高.
//优化了内存占用,静态内存占用为79个字节@10个参数.动态适应数字及字符串长度
//V2.0 
//1,修改了list指令,打印函数的完整表达式.
//2,增加了id指令,打印每个函数的入口地址.
//3,修改了参数匹配,支持函数参数的调用(输入入口地址).
//4,增加了函数名长度宏定义.	
//V2.1 20110707		 
//1,增加dec,hex两个指令,用于设置参数显示进制,及执行进制转换.
//注:当dec,hex不带参数的时候,即设定显示参数进制.当后跟参数的时候,即执行进制转换.
//如:"dec 0XFF" 则会将0XFF转为255,由串口返回.
//如:"hex 100" 	则会将100转为0X64,由串口返回
//2,新增usmart_get_cmdname函数,用于获取指令名字.
//V2.2 20110726	
//1,修正了void类型参数的参数统计错误.
//2,修改数据显示格式默认为16进制.
//V2.3 20110815
//1,去掉了函数名后必须跟"("的限制.
//2,修正了字符串参数中不能有"("的bug.
//3,修改了函数默认显示参数格式的修改方式. 
//V2.4 20110905
//1,修改了usmart_get_cmdname函数,增加最大参数长度限制.避免了输入错误参数时的死机现象.
//2,增加USMART_ENTIM4_SCAN宏定义,用于配置是否使用TIM2定时执行scan函数.
//V2.5 20110930
//1,修改usmart_init函数为void usmart_init(u8 sysclk),可以根据系统频率自动设定扫描时间.(固定100ms)
//2,去掉了usmart_init函数中的uart_init函数,串口初始化必须在外部初始化,方便用户自行管理.
//V2.6 20111009
//1,增加了read_addr和write_addr两个函数.可以利用这两个函数读写内部任意地址(必须是有效地址).更加方便调试.
//2,read_addr和write_addr两个函数可以通过设置USMART_USE_WRFUNS为来使能和关闭.
//3,修改了usmart_strcmp,使其规范化.			  
//V2.7 20111024
//1,修正了返回值16进制显示时不换行的bug.
//2,增加了函数是否有返回值的判断,如果没有返回值,则不会显示.有返回值时才显示其返回值.
//V2.8 20111116
//1,修正了list等不带参数的指令发送后可能导致死机的bug.
//V2.9 20120917
//1,修改了形如：void*xxx(void)类型函数不能识别的bug。
//V3.0 20130425
//1,新增了字符串参数对转义符的支持。
//V3.1 20131120
//1,增加runtime系统指令,可以用于统计函数执行时间.
//用法:
//发送:runtime 1 ,则开启函数执行时间统计功能
//发送:runtime 0 ,则关闭函数执行时间统计功能
///runtime统计功能,必须设置:USMART_ENTIMX_SCAN 为1,才可以使用!!
/////////////////////////////////////////////////////////////////////////////////////
//系统命令

typedef enum
{
	ROOT = 0,
	NOBOOT
} vUserType;

typedef struct commandRecord
{
	char *command;
	int len;
	ListItem_t			xStateListItem;
} cmdRecord;

static List_t xCommandRecordList;

vUserType xUserID = ROOT;
static char xUserName[36] = { 'r', 'o', 'o', 't', '\0' };

static char pBuf[512];
extern TaskHandle_t pxTimeTask;
extern TaskHandle_t pxTempretureTask;

void Change_User_Name( char *userName )
{
	while( *userName == ' ')
		userName++;

	//printf("%s: userName = %s\r\n", __func__, userName);
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

void Printf_System_Tempreture( void )
{
	if( pdTRUE == xTaskAbortDelay(pxTempretureTask) )
	{
		//printf("wake up task %s from block success!\r\n", taskName);
	}
	else
	{
		printf("wake up task pxTempretureTask from block fail!\r\n");
	}	
}

void Printf_Time_From_Rtc( void )
{
	if( pdTRUE == xTaskAbortDelay(pxTimeTask) )
	{
		//printf("wake up task %s from block success!\r\n", taskName);
	}
	else
	{
		printf("wake up task rtc from block fail!\r\n");
	}	
}

void Printf_System_Jiffies( void )
{
	TickType_t tick = xTaskGetTickCount();
	printf("Sytem times from boot is %u (s), system tick = %u (ms).\r\n", 
			tick/1000, tick);	
}

__asm void Stm32f407_Force_Reboot( void )
{
  MOV R0, #1           //; 
  MSR FAULTMASK, R0    //; Clear FAULTMASK flag to disable all interrupts
  LDR R0, =0xE000ED0C  //; address of Application Interrupt and Reset Control Register
  LDR R1, =0x05FA0004  //; value of Application Interrupt and Reset Control Register
  STR R1, [R0]         //; system reset by soft ware   
 
deadloop
	B deadloop        //; deadloop
}

void Reboot_System( void )
{
	printf("reboot system ...\r\n");
	vTaskDelay( 30 / portTICK_RATE_MS);
	Stm32f407_Force_Reboot();
}

void Change_Uart_Baud( unsigned char *command )
{
	static int xCurrentBaud = 115200;
	int bound, i = 0;
	char tmp[16];

	//printf("%s: command = %s\r\n", __func__, command);
	memset(tmp, '\0', sizeof(tmp));
	while( *command == ' ')
		command++;
	
	strcpy(tmp, (char *)command);
	printf("Set baud to %s!\r\n", tmp);
	while( tmp[i] >= '0' && tmp[i] <= '9' )
		i++;
	
	if( tmp[i] != '\0')
	{
		printf("Error command! use baud + number\r\n");
		return;
	}
	bound = atoi(tmp);
		
	/*Wait for 15ms to print the Log*/
	vTaskDelay( 15 / portTICK_RATE_MS);
	switch( bound)
	{			
		case 2400:
			uart_deinit();
			xCurrentBaud = bound;
			vTaskDelay( 40 / portTICK_RATE_MS);			
			uart_init( 2400 );				
			break;			
		case 4800:
			uart_deinit();			
			xCurrentBaud = bound;
			vTaskDelay( 20 / portTICK_RATE_MS);
			uart_init( 4800 );				
			break;			
		case 9600:
			uart_deinit();			
			xCurrentBaud = bound;
			uart_init( 9600 );				
			break;	
		case 19200:
			uart_deinit();			
			xCurrentBaud = bound;
			uart_init( 19200 );				
			break;		
		case 115200:
			uart_deinit();			
			xCurrentBaud = bound;
			uart_init( 115200 );
			break;
		case 230400:
			uart_deinit();			
			xCurrentBaud = bound;
			uart_init( 230400 );
			break;
		case 460800:
			uart_deinit();			
			xCurrentBaud = bound;
			uart_init( 460800 );				
			break;
		
		default:
			printf("xCurrentBaud = %d\r\n", xCurrentBaud);
			printf("please input 9600/19200/115200/230400/460800 !\r\n");
			break;
	}
	
}

void List_All_Task_Information( void )
{
		vTaskList( pBuf );
		printf("Name       Status  Priority  WaterMark  Num\r\n");
		printf("%s", pBuf);
}

void List_Struct_Information( void )
{
		printf("*********struct********\r\n");
		printf("uart1fifo: lostBytes=%u, in=%u, out=%u, size=%u\r\n", 
						uart1fifo.lostBytes, uart1fifo.in, uart1fifo.out, uart1fifo.size);								
		printf("mLogFifo:  lostBytes=%u, in=%u, out=%u, size=%u\r\n", 
						mLogFifo.lostBytes, mLogFifo.in, mLogFifo.out, mLogFifo.size);	
		printf("*********struct********\r\n");	
}

void Printf_Memory_Information( void )
{
		size_t xFreeSize = xPortGetFreeHeapSize();
		float  fUsePercent = 100 - 100 * ( xFreeSize*1.0 / ( configTOTAL_HEAP_SIZE ) );
	
		printf("Memory used %f%%, Free Size = %d, Total = %d\r\n", 
				fUsePercent, xFreeSize, configTOTAL_HEAP_SIZE);
}

void Deal_Command_For_Task( unsigned char *command , char action)
{
	TaskHandle_t target;
		char taskName[configMAX_TASK_NAME_LEN];
	
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
				case 1:
								vTaskDelete( target );
								printf("delete task %s success!\r\n", taskName);
								break;
				case 2:
								vTaskSuspend(target);
								printf("suspend task %s success!\r\n", taskName);
								break;
				case 3:
								vTaskResume(target);
								printf("resume task %s success!\r\n", taskName);
								break;
				case 4:
								if( pdTRUE == xTaskAbortDelay(target) )
								{
									printf("wake up task %s from block success!\r\n", taskName);
								}
								else
								{
									printf("wake up task %s from block fail!\r\n", taskName);
								}
								break;
				case 5:
								
								break;			
				default:
								break;
			}
		}
		else
		{
			printf("target = %p error! please input right task name.\r\n", target);
		}
}

u8 *sys_cmd_tab[]=
{
	"?",
	"help",
	"list",
	"id",
	"hex",
	"dec",
	"runtime",
	
	"ps",
	"kill",
	"suspend",
	"resume",
	"abort",
	"exit",
	"struct",
	"memory",
	"baud",
	"reboot",
	"jiffies",
	"time",	
	"tempreture",
	"su",
	"version",
};	    
//处理系统指令
//0,成功处理;其他,错误代码;
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
		return USMART_FUNCERR;//错误的指令 
	
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
			printf("          Debug系统调试命令:     \r\n");
			printf("\r\n");			
			printf("?:    获取帮助信息\r\n");
			printf("help: 获取帮助信息\r\n");
			printf("list: 可用的函数列表\r\n");
			printf("id:   可用函数的ID列表\r\n");
			printf("hex:  参数16进制显示,后跟空格+数字即执行进制转换\r\n");
			printf("dec:  参数10进制显示,后跟空格+数字即执行进制转换\r\n");
			printf("runtime:1, 开启函数运行计时;0,关闭函数运行计时;\r\n");
			printf("请按照程序编写格式输入函数名及参数并以回车键结束.\r\n"); 
		
			printf("----------------FreeRTOS---------------\r\n");
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
			printf("----------------FreeRTOS---------------\r\n");		
   
			//printf("--------------------------------------------- \r\n");
			vTaskPrioritySet( NULL, pre );				
#else
			printf("指令失效\r\n");
#endif
			break;
		case 2://查询指令
			printf("\r\n");
			printf("-------------------------函数清单--------------------------- \r\n");
			for(i=0;i<usmart_dev.fnum;i++)printf("%s\r\n",usmart_dev.funs[i].name);
			printf("\r\n");
			break;	 
		case 3://查询ID
			printf("\r\n");
			printf("-------------------------函数 ID --------------------------- \r\n");
			for(i=0;i<usmart_dev.fnum;i++)
			{
				usmart_get_fname((u8*)usmart_dev.funs[i].name,sfname,&pnum,&rval);//得到本地函数名 
				printf("%s id is:\r\n0X%08X\r\n",sfname,usmart_dev.funs[i].func); //显示ID
			}
			printf("\r\n");
			break;
		case 4://hex指令
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//参数正常
			{
				i=usmart_str2num(sfname,&res);	   	//记录该参数	
				if(i==0)						  	//进制转换功能
				{
					printf("HEX:0X%X\r\n",res);	   	//转为16进制
				}else if(i!=4)return USMART_PARMERR;//参数错误.
				else 				   				//参数显示设定功能
				{
					printf("16进制参数显示!\r\n");
					usmart_dev.sptype=SP_TYPE_HEX;  
				}

			}else return USMART_PARMERR;			//参数错误.
			printf("\r\n"); 
			break;
		case 5://dec指令
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//参数正常
			{
				i=usmart_str2num(sfname,&res);	   	//记录该参数	
				if(i==0)						   	//进制转换功能
				{
					printf("DEC:%lu\r\n",res);	   	//转为10进制
				}else if(i!=4)return USMART_PARMERR;//参数错误.
				else 				   				//参数显示设定功能
				{
					printf("10进制参数显示!\r\n");
					usmart_dev.sptype=SP_TYPE_DEC;  
				}

			}else return USMART_PARMERR;			//参数错误. 
			printf("\r\n"); 
			break;	 
		case 6://runtime指令,设置是否显示函数执行时间
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//参数正常
			{
				i=usmart_str2num(sfname,&res);	   		//记录该参数	
				if(i==0)						   		//读取指定地址数据功能
				{
					if(USMART_ENTIMX_SCAN==0)printf("\r\nError! \r\nTo EN RunTime function,Please set USMART_ENTIMX_SCAN = 1 first!\r\n");//报错
					else
					{
						usmart_dev.runtimeflag=res;
						if(usmart_dev.runtimeflag)printf("Run Time Calculation ON\r\n");
						else printf("Run Time Calculation OFF\r\n"); 
					}
				}else return USMART_PARMERR;   			//未带参数,或者参数错误	 
 			}else return USMART_PARMERR;				//参数错误. 
			printf("\r\n"); 
			break;	 

		/*Add For FreeRTOS*/
		case 7:
			( void ) List_All_Task_Information();
			break;
		case 8:
			( void ) Deal_Command_For_Task(str, 1);
			break;
		case 9:
			( void ) Deal_Command_For_Task(str, 2);
			break;
		case 10:
			( void ) Deal_Command_For_Task(str, 3);
			break;		
		case 11:
			( void ) Deal_Command_For_Task(str, 4);
			break;			
		case 12:
			( void ) Deal_Command_For_Task(str, 5);
			break;			
		case 13:
			( void ) List_Struct_Information();
			break;		
		case 14:
			( void ) Printf_Memory_Information();
			break;		
		case 15:
			( void ) Change_Uart_Baud(str);
			break;
		case 16:
			( void ) Reboot_System();
			break;		
		case 17:
			( void ) Printf_System_Jiffies();
			break;
		case 18:
			( void ) Printf_Time_From_Rtc();
			break;		
		case 19:
			( void ) Printf_System_Tempreture();
			break;		
		case 20:
			( void ) Change_User_Name((char *)str);
			break;			
		case 21:
			( void ) Printf_Application_Version();
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

#if USMART_ENTIMX_SCAN==1
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


//OS_EVENT *mUart1Seam = NULL;
//OS_EVENT *mMsgQUart = NULL;
QueueHandle_t xDebugQueue =  NULL;

void usamrt_debug_task(void *argc) 
{
	//int len;
	vListInitialise( &xCommandRecordList );
	//vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
	printf("%s start...\r\n", __func__);
	usmart_dev.sptype = 0;	//0,10进制;1,16进制;
	rfifo_init(&uart1fifo);	
	xDebugQueue = xQueueCreate( 20, sizeof(int));

	while (1) {
		ulTaskNotifyTake( pdTRUE, portMAX_DELAY );
		usmart_dev.scan(0);
		/*
		if( xQueueReceive( xDebugQueue, &len, portMAX_DELAY ) ) {
				usmart_dev.scan(len);
		} else { 
			if ( ( len = rfifo_len( &uart1fifo ) ) > 0 ) {
				rfifo_get(&uart1fifo, USART_RX_BUF, len);		
				printf("%s-%d xQueueReceive error!\r\n", __func__,  __LINE__);				
			}
		}
		*/
	}
}

#endif
////////////////////////////////////////////////////////////////////////////////////////
//初始化串口控制器
//sysclk:系统时钟（Mhz）

void usmart_init(void)
{
	;
}		
//从str中获取函数名,id,及参数信息
//*str:字符串指针.
//返回值:0,识别成功;其他,错误代码.
u8 usmart_cmd_rec(u8*str) 
{
	u8 sta,i,rval;//状态	 
	u8 rpnum,spnum;
	u8 rfname[MAX_FNAME_LEN];//暂存空间,用于存放接收到的函数名  
	u8 sfname[MAX_FNAME_LEN];//存放本地函数名
	sta=usmart_get_fname(str,rfname,&rpnum,&rval);//得到接收到的数据的函数名及参数个数	  
	if(sta)return sta;//错误
	for(i=0;i<usmart_dev.fnum;i++)
	{
		sta=usmart_get_fname((u8*)usmart_dev.funs[i].name,sfname,&spnum,&rval);//得到本地函数名及参数个数
		if(sta)return sta;//本地解析有误	  
		if(usmart_strcmp(sfname,rfname)==0)//相等
		{
			if(spnum>rpnum)return USMART_PARMERR;//参数错误(输入参数比源函数参数少)
			usmart_dev.id=i;//记录函数ID.
			break;//跳出.
		}	
	}
	if(i==usmart_dev.fnum)return USMART_NOFUNCFIND;	//未找到匹配的函数
 	sta=usmart_get_fparam(str,&i);					//得到函数参数个数	
	if(sta)return sta;								//返回错误
	usmart_dev.pnum=i;								//参数个数记录
    return USMART_OK;
}
//usamrt执行函数
//该函数用于最终执行从串口收到的有效函数.
//最多支持10个参数的函数,更多的参数支持也很容易实现.不过用的很少.一般5个左右的参数的函数已经很少见了.
//该函数会在串口打印执行情况.以:"函数名(参数1，参数2...参数N)=返回值".的形式打印.
//当所执行的函数没有返回值的时候,所打印的返回值是一个无意义的数据.
void usmart_exe(void)
{
	u8 id,i;
	u32 res;		   
	u32 temp[MAX_PARM];//参数转换,使之支持了字符串 
	u8 sfname[MAX_FNAME_LEN];//存放本地函数名
	u8 pnum,rval;
	id=usmart_dev.id;
	if(id>=usmart_dev.fnum)return;//不执行.
	usmart_get_fname((u8*)usmart_dev.funs[id].name,sfname,&pnum,&rval);//得到本地函数名,及参数个数 
	//printf("\r\n%s(",sfname);//输出正要执行的函数名
	for(i=0;i<pnum;i++)//输出参数
	{
		if(usmart_dev.parmtype&(1<<i))//参数是字符串
		{
			//printf("%c",'"');			 
			//printf("%s",usmart_dev.parm+usmart_get_parmpos(i));
			//printf("%c",'"');
			temp[i]=(u32)&(usmart_dev.parm[usmart_get_parmpos(i)]);
		}else						  //参数是数字
		{
			temp[i]=*(u32*)(usmart_dev.parm+usmart_get_parmpos(i));
			if(usmart_dev.sptype==SP_TYPE_DEC);//printf("%lu",temp[i]);//10进制参数显示
			else ;//printf("0X%X",temp[i]);//16进制参数显示 	   
		}
		//if(i!=pnum-1)printf(",");
	}
	//printf(")");
	usmart_reset_runtime();	//计时器清零,开始计时
	switch(usmart_dev.pnum)
	{
		case 0://无参数(void类型)											  
			res=(*(u32(*)())usmart_dev.funs[id].func)();
			break;
	    case 1://有1个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0]);
			break;
	    case 2://有2个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1]);
			break;
	    case 3://有3个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2]);
			break;
	    case 4://有4个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3]);
			break;
	    case 5://有5个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4]);
			break;
	    case 6://有6个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5]);
			break;
	    case 7://有7个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6]);
			break;
	    case 8://有8个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7]);
			break;
	    case 9://有9个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8]);
			break;
	    case 10://有10个参数
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8],temp[9]);
			break;
	}
	usmart_get_runtime();//获取函数执行时间
	if(rval==1)//需要返回值.
	{
		if(usmart_dev.sptype==SP_TYPE_DEC)printf("=%lu;\r\n",res);//输出执行结果(10进制参数显示)
		else printf("=0X%X;\r\n",res);//输出执行结果(16进制参数显示)	   
	}else ;//printf(";\r\n");		//不需要返回值,直接输出结束
	if(usmart_dev.runtimeflag)	//需要显示函数执行时间
	{ 
		printf("Function Run Time:%d.%1dms\r\n",usmart_dev.runtime/10,usmart_dev.runtime%10);//打印函数执行时间 
	}	
}
//usmart扫描函数
//通过调用该函数,实现usmart的各个控制.该函数需要每隔一定时间被调用一次
//以及时执行从串口发过来的各个函数.
//本函数可以在中断里面调用,从而实现自动管理.
//如果非ALIENTEK用户,则USART_RX_STA和USART_RX_BUF[]需要用户自己实现
extern Ringfifo uart1fifo;

void usmart_scan( int uart1DataLen )
{
	static unsigned int num = 0xffffffff;
	cmdRecord *cmdItem;
	static int index = 0;
	unsigned char sta, len;
	
	while( rfifo_len( &uart1fifo ) > 0 ) 
	{
		rfifo_get( &uart1fifo, USART_RX_BUF+index, 1 );
		
		if( index >= 2 && USART_RX_BUF[index] == 0x41 && 
					USART_RX_BUF[index-1] == 0x5b &&
					USART_RX_BUF[index-2] == 0x1b)
		{
			//printf(" up ");
			if( listLIST_IS_EMPTY( &xCommandRecordList ) != pdFALSE )
			{
				printf(" ");				
			}
			else
			{
				cmdItem = NULL;
				listGET_OWNER_OF_NEXT_ENTRY( cmdItem, &xCommandRecordList );
				if( cmdItem )
				{
					printf(" %s ", cmdItem->command);
					memset( USART_RX_BUF, '\0', sizeof(USART_RX_BUF) );
					memcpy( USART_RX_BUF, cmdItem->command, cmdItem->len);
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
		if( USART_RX_BUF[index - 1] == 0x0d )
		{		
			if( index == 1 || index > 69 )
			{
				if( index > 69 )
					printf("Error command!\r\n");
				printf("\r\n%s#", xUserName);
				index = 0;
				continue;
			}
			else
			{
				USART_RX_BUF[index - 1] = '\0';
			}
			
			{
			cmdRecord *pxFirstCmd, *pxNextCmd;
				
				if( listCURRENT_LIST_LENGTH( &xCommandRecordList ) > ( UBaseType_t ) 0 )
				{
					listGET_OWNER_OF_NEXT_ENTRY( pxFirstCmd, &xCommandRecordList );
				
					do
					{
						listGET_OWNER_OF_NEXT_ENTRY( pxNextCmd, &xCommandRecordList );
						if( memcmp(pxNextCmd->command, USART_RX_BUF, strlen((char*)USART_RX_BUF)) == 0)
						{
							//printf("\r\n *****remove command %s *****\r\n", USART_RX_BUF);
							uxListRemove( &( pxNextCmd->xStateListItem ) );
							vPortFree(pxNextCmd->command);
							vPortFree(pxNextCmd);
							pxNextCmd = NULL;
							break;
						}

					} while( pxNextCmd != pxFirstCmd );
					//printf("***out\r\n");
				}
			}

			cmdItem =  pvPortMalloc( sizeof(cmdRecord) );
			if( cmdItem)
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
					(&xCommandRecordList)->pxIndex = (&( cmdItem->xStateListItem ))->pxPrevious;
				}
				else
				{
					vPortFree(cmdItem);
				}
			}
			
			printf("\r\n");
			sta = usmart_dev.cmd_rec( USART_RX_BUF );	
			if( sta == 0 ) 
			{				
				usmart_dev.exe();
			}
			else 
			{  
				len = usmart_sys_cmd_exe( USART_RX_BUF );
				if( len != USMART_FUNCERR ) 
				{
					sta = len;
				}
				if( sta )
				{
					switch( sta )
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
			index = 0;
			printf("%s#", xUserName);
			USART_RX_STA = 0; 		
		}
		else
		{
			printf("%c", USART_RX_BUF[index - 1]);
		}
	}
}

#if USMART_USE_WRFUNS==1 	//如果使能了读写操作
//读取指定地址的值		 
u32 read_addr(u32 addr)
{
	return *(u32*)addr;//	
}
//在指定地址写入指定的值		 
void write_addr(u32 addr,u32 val)
{
	*(u32*)addr=val; 	
}
#endif













