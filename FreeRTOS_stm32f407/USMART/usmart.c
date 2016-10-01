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

u8 USART_RX_BUF[USART_REC_LEN];     //���ջ���,���USART_REC_LEN���ֽ�.
//����״̬
//bit15��	������ɱ�־
//bit14��	���յ�0x0d
//bit13~0��	���յ�����Ч�ֽ���Ŀ
u16 USART_RX_STA=0;       //����״̬���	 

//////////////////////////////////////////////////////////////////////////////////	 
//����˵��
//V1.4
//�����˶Բ���Ϊstring���͵ĺ�����֧��.���÷�Χ������.
//�Ż����ڴ�ռ��,��̬�ڴ�ռ��Ϊ79���ֽ�@10������.��̬��Ӧ���ּ��ַ�������
//V2.0 
//1,�޸���listָ��,��ӡ�������������ʽ.
//2,������idָ��,��ӡÿ����������ڵ�ַ.
//3,�޸��˲���ƥ��,֧�ֺ��������ĵ���(������ڵ�ַ).
//4,�����˺��������Ⱥ궨��.	
//V2.1 20110707		 
//1,����dec,hex����ָ��,�������ò�����ʾ����,��ִ�н���ת��.
//ע:��dec,hex����������ʱ��,���趨��ʾ��������.�����������ʱ��,��ִ�н���ת��.
//��:"dec 0XFF" ��Ὣ0XFFתΪ255,�ɴ��ڷ���.
//��:"hex 100" 	��Ὣ100תΪ0X64,�ɴ��ڷ���
//2,����usmart_get_cmdname����,���ڻ�ȡָ������.
//V2.2 20110726	
//1,������void���Ͳ����Ĳ���ͳ�ƴ���.
//2,�޸�������ʾ��ʽĬ��Ϊ16����.
//V2.3 20110815
//1,ȥ���˺�����������"("������.
//2,�������ַ��������в�����"("��bug.
//3,�޸��˺���Ĭ����ʾ������ʽ���޸ķ�ʽ. 
//V2.4 20110905
//1,�޸���usmart_get_cmdname����,������������������.����������������ʱ����������.
//2,����USMART_ENTIM4_SCAN�궨��,���������Ƿ�ʹ��TIM2��ʱִ��scan����.
//V2.5 20110930
//1,�޸�usmart_init����Ϊvoid usmart_init(u8 sysclk),���Ը���ϵͳƵ���Զ��趨ɨ��ʱ��.(�̶�100ms)
//2,ȥ����usmart_init�����е�uart_init����,���ڳ�ʼ���������ⲿ��ʼ��,�����û����й���.
//V2.6 20111009
//1,������read_addr��write_addr��������.��������������������д�ڲ������ַ(��������Ч��ַ).���ӷ������.
//2,read_addr��write_addr������������ͨ������USMART_USE_WRFUNSΪ��ʹ�ܺ͹ر�.
//3,�޸���usmart_strcmp,ʹ��淶��.			  
//V2.7 20111024
//1,�����˷���ֵ16������ʾʱ�����е�bug.
//2,�����˺����Ƿ��з���ֵ���ж�,���û�з���ֵ,�򲻻���ʾ.�з���ֵʱ����ʾ�䷵��ֵ.
//V2.8 20111116
//1,������list�Ȳ���������ָ��ͺ���ܵ���������bug.
//V2.9 20120917
//1,�޸������磺void*xxx(void)���ͺ�������ʶ���bug��
//V3.0 20130425
//1,�������ַ���������ת�����֧�֡�
//V3.1 20131120
//1,����runtimeϵͳָ��,��������ͳ�ƺ���ִ��ʱ��.
//�÷�:
//����:runtime 1 ,��������ִ��ʱ��ͳ�ƹ���
//����:runtime 0 ,��رպ���ִ��ʱ��ͳ�ƹ���
///runtimeͳ�ƹ���,��������:USMART_ENTIMX_SCAN Ϊ1,�ſ���ʹ��!!
/////////////////////////////////////////////////////////////////////////////////////
//ϵͳ����

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
//����ϵͳָ��
//0,�ɹ�����;����,�������;
u8 usmart_sys_cmd_exe( u8 *str )
{
UBaseType_t pre;	
	u8 i;
	u8 sfname[MAX_FNAME_LEN];//��ű��غ�����
	u8 pnum;
	u8 rval;
	u32 res;  
	res = usmart_get_cmdname( str, sfname, &i, MAX_FNAME_LEN );//�õ�ָ�ָ���
	if( res )
		return USMART_FUNCERR;//�����ָ�� 
	
	str += i;	 	 			    
	for( i=0; i < sizeof(sys_cmd_tab)/sizeof(sys_cmd_tab[0]); i++ )//֧�ֵ�ϵͳָ��
	{
		if( usmart_strcmp( sfname, sys_cmd_tab[i] )==0 )
			break;
	}
	switch( i )
	{					   
		case 0:
		case 1://����ָ��
#if USMART_USE_HELP
			pre = uxTaskPriorityGet( NULL );
			vTaskPrioritySet( NULL, configMAX_PRIORITIES - 2 );	
			printf("          Debugϵͳ��������:     \r\n");
			printf("\r\n");			
			printf("?:    ��ȡ������Ϣ\r\n");
			printf("help: ��ȡ������Ϣ\r\n");
			printf("list: ���õĺ����б�\r\n");
			printf("id:   ���ú�����ID�б�\r\n");
			printf("hex:  ����16������ʾ,����ո�+���ּ�ִ�н���ת��\r\n");
			printf("dec:  ����10������ʾ,����ո�+���ּ�ִ�н���ת��\r\n");
			printf("runtime:1, �����������м�ʱ;0,�رպ������м�ʱ;\r\n");
			printf("�밴�ճ����д��ʽ���뺯�������������Իس�������.\r\n"); 
		
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
			printf("ָ��ʧЧ\r\n");
#endif
			break;
		case 2://��ѯָ��
			printf("\r\n");
			printf("-------------------------�����嵥--------------------------- \r\n");
			for(i=0;i<usmart_dev.fnum;i++)printf("%s\r\n",usmart_dev.funs[i].name);
			printf("\r\n");
			break;	 
		case 3://��ѯID
			printf("\r\n");
			printf("-------------------------���� ID --------------------------- \r\n");
			for(i=0;i<usmart_dev.fnum;i++)
			{
				usmart_get_fname((u8*)usmart_dev.funs[i].name,sfname,&pnum,&rval);//�õ����غ����� 
				printf("%s id is:\r\n0X%08X\r\n",sfname,usmart_dev.funs[i].func); //��ʾID
			}
			printf("\r\n");
			break;
		case 4://hexָ��
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//��������
			{
				i=usmart_str2num(sfname,&res);	   	//��¼�ò���	
				if(i==0)						  	//����ת������
				{
					printf("HEX:0X%X\r\n",res);	   	//תΪ16����
				}else if(i!=4)return USMART_PARMERR;//��������.
				else 				   				//������ʾ�趨����
				{
					printf("16���Ʋ�����ʾ!\r\n");
					usmart_dev.sptype=SP_TYPE_HEX;  
				}

			}else return USMART_PARMERR;			//��������.
			printf("\r\n"); 
			break;
		case 5://decָ��
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//��������
			{
				i=usmart_str2num(sfname,&res);	   	//��¼�ò���	
				if(i==0)						   	//����ת������
				{
					printf("DEC:%lu\r\n",res);	   	//תΪ10����
				}else if(i!=4)return USMART_PARMERR;//��������.
				else 				   				//������ʾ�趨����
				{
					printf("10���Ʋ�����ʾ!\r\n");
					usmart_dev.sptype=SP_TYPE_DEC;  
				}

			}else return USMART_PARMERR;			//��������. 
			printf("\r\n"); 
			break;	 
		case 6://runtimeָ��,�����Ƿ���ʾ����ִ��ʱ��
			printf("\r\n");
			usmart_get_aparm(str,sfname,&i);
			if(i==0)//��������
			{
				i=usmart_str2num(sfname,&res);	   		//��¼�ò���	
				if(i==0)						   		//��ȡָ����ַ���ݹ���
				{
					if(USMART_ENTIMX_SCAN==0)printf("\r\nError! \r\nTo EN RunTime function,Please set USMART_ENTIMX_SCAN = 1 first!\r\n");//����
					else
					{
						usmart_dev.runtimeflag=res;
						if(usmart_dev.runtimeflag)printf("Run Time Calculation ON\r\n");
						else printf("Run Time Calculation OFF\r\n"); 
					}
				}else return USMART_PARMERR;   			//δ������,���߲�������	 
 			}else return USMART_PARMERR;				//��������. 
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
		default://�Ƿ�ָ��
			return USMART_FUNCERR;
	}
	return 0;
}
////////////////////////////////////////////////////////////////////////////////////////
//��ֲע��:��������stm32Ϊ��,���Ҫ��ֲ������mcu,������Ӧ�޸�.
//usmart_reset_runtime,�����������ʱ��,��ͬ��ʱ���ļ����Ĵ����Լ���־λһ������.��������װ��ֵΪ���,������޶ȵ��ӳ���ʱʱ��.
//usmart_get_runtime,��ȡ��������ʱ��,ͨ����ȡCNTֵ��ȡ,����usmart��ͨ���жϵ��õĺ���,���Զ�ʱ���жϲ�����Ч,��ʱ����޶�
//ֻ��ͳ��2��CNT��ֵ,Ҳ���������+���һ��,���������2��,û������,���������ʱ,������:2*������CNT*0.1ms.��STM32��˵,��:13.1s����
//������:TIM4_IRQHandler��Timer2_Init,��Ҫ����MCU�ص������޸�.ȷ������������Ƶ��Ϊ:10Khz����.����,��ʱ����Ҫ�����Զ���װ�ع���!!

#if USMART_ENTIMX_SCAN==1
//��λruntime
//��Ҫ��������ֲ����MCU�Ķ�ʱ�����������޸�
void usmart_reset_runtime(void)
{
	/*
	TIM_ClearFlag(TIM4,TIM_FLAG_Update);//����жϱ�־λ 
	TIM_SetAutoreload(TIM4,0XFFFF);//����װ��ֵ���õ����
	TIM_SetCounter(TIM4,0);		//��ն�ʱ����CNT
	usmart_dev.runtime=0;
		*/
}
//���runtimeʱ��
//����ֵ:ִ��ʱ��,��λ:0.1ms,�����ʱʱ��Ϊ��ʱ��CNTֵ��2��*0.1ms
//��Ҫ��������ֲ����MCU�Ķ�ʱ�����������޸�
u32 usmart_get_runtime(void)
{
	/*
	if(TIM_GetFlagStatus(TIM4,TIM_FLAG_Update)==SET)//�������ڼ�,�����˶�ʱ�����
	{
		usmart_dev.runtime+=0XFFFF;
	}
	usmart_dev.runtime+=TIM_GetCounter(TIM4);
	return usmart_dev.runtime;		//���ؼ���ֵ
	*/
	return 0;
}
//��������������,��USMART����,�ŵ�����,����������ֲ. 
//��ʱ��4�жϷ������	 


//OS_EVENT *mUart1Seam = NULL;
//OS_EVENT *mMsgQUart = NULL;
QueueHandle_t xDebugQueue =  NULL;

void usamrt_debug_task(void *argc) 
{
	//int len;
	vListInitialise( &xCommandRecordList );
	//vListInitialiseItem( &( pxNewTCB->xStateListItem ) );
	printf("%s start...\r\n", __func__);
	usmart_dev.sptype = 0;	//0,10����;1,16����;
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
//��ʼ�����ڿ�����
//sysclk:ϵͳʱ�ӣ�Mhz��

void usmart_init(void)
{
	;
}		
//��str�л�ȡ������,id,��������Ϣ
//*str:�ַ���ָ��.
//����ֵ:0,ʶ��ɹ�;����,�������.
u8 usmart_cmd_rec(u8*str) 
{
	u8 sta,i,rval;//״̬	 
	u8 rpnum,spnum;
	u8 rfname[MAX_FNAME_LEN];//�ݴ�ռ�,���ڴ�Ž��յ��ĺ�����  
	u8 sfname[MAX_FNAME_LEN];//��ű��غ�����
	sta=usmart_get_fname(str,rfname,&rpnum,&rval);//�õ����յ������ݵĺ���������������	  
	if(sta)return sta;//����
	for(i=0;i<usmart_dev.fnum;i++)
	{
		sta=usmart_get_fname((u8*)usmart_dev.funs[i].name,sfname,&spnum,&rval);//�õ����غ���������������
		if(sta)return sta;//���ؽ�������	  
		if(usmart_strcmp(sfname,rfname)==0)//���
		{
			if(spnum>rpnum)return USMART_PARMERR;//��������(���������Դ����������)
			usmart_dev.id=i;//��¼����ID.
			break;//����.
		}	
	}
	if(i==usmart_dev.fnum)return USMART_NOFUNCFIND;	//δ�ҵ�ƥ��ĺ���
 	sta=usmart_get_fparam(str,&i);					//�õ�������������	
	if(sta)return sta;								//���ش���
	usmart_dev.pnum=i;								//����������¼
    return USMART_OK;
}
//usamrtִ�к���
//�ú�����������ִ�дӴ����յ�����Ч����.
//���֧��10�������ĺ���,����Ĳ���֧��Ҳ������ʵ��.�����õĺ���.һ��5�����ҵĲ����ĺ����Ѿ����ټ���.
//�ú������ڴ��ڴ�ӡִ�����.��:"������(����1������2...����N)=����ֵ".����ʽ��ӡ.
//����ִ�еĺ���û�з���ֵ��ʱ��,����ӡ�ķ���ֵ��һ�������������.
void usmart_exe(void)
{
	u8 id,i;
	u32 res;		   
	u32 temp[MAX_PARM];//����ת��,ʹ֧֮�����ַ��� 
	u8 sfname[MAX_FNAME_LEN];//��ű��غ�����
	u8 pnum,rval;
	id=usmart_dev.id;
	if(id>=usmart_dev.fnum)return;//��ִ��.
	usmart_get_fname((u8*)usmart_dev.funs[id].name,sfname,&pnum,&rval);//�õ����غ�����,���������� 
	//printf("\r\n%s(",sfname);//�����Ҫִ�еĺ�����
	for(i=0;i<pnum;i++)//�������
	{
		if(usmart_dev.parmtype&(1<<i))//�������ַ���
		{
			//printf("%c",'"');			 
			//printf("%s",usmart_dev.parm+usmart_get_parmpos(i));
			//printf("%c",'"');
			temp[i]=(u32)&(usmart_dev.parm[usmart_get_parmpos(i)]);
		}else						  //����������
		{
			temp[i]=*(u32*)(usmart_dev.parm+usmart_get_parmpos(i));
			if(usmart_dev.sptype==SP_TYPE_DEC);//printf("%lu",temp[i]);//10���Ʋ�����ʾ
			else ;//printf("0X%X",temp[i]);//16���Ʋ�����ʾ 	   
		}
		//if(i!=pnum-1)printf(",");
	}
	//printf(")");
	usmart_reset_runtime();	//��ʱ������,��ʼ��ʱ
	switch(usmart_dev.pnum)
	{
		case 0://�޲���(void����)											  
			res=(*(u32(*)())usmart_dev.funs[id].func)();
			break;
	    case 1://��1������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0]);
			break;
	    case 2://��2������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1]);
			break;
	    case 3://��3������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2]);
			break;
	    case 4://��4������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3]);
			break;
	    case 5://��5������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4]);
			break;
	    case 6://��6������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5]);
			break;
	    case 7://��7������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6]);
			break;
	    case 8://��8������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7]);
			break;
	    case 9://��9������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8]);
			break;
	    case 10://��10������
			res=(*(u32(*)())usmart_dev.funs[id].func)(temp[0],temp[1],temp[2],temp[3],temp[4],\
			temp[5],temp[6],temp[7],temp[8],temp[9]);
			break;
	}
	usmart_get_runtime();//��ȡ����ִ��ʱ��
	if(rval==1)//��Ҫ����ֵ.
	{
		if(usmart_dev.sptype==SP_TYPE_DEC)printf("=%lu;\r\n",res);//���ִ�н��(10���Ʋ�����ʾ)
		else printf("=0X%X;\r\n",res);//���ִ�н��(16���Ʋ�����ʾ)	   
	}else ;//printf(";\r\n");		//����Ҫ����ֵ,ֱ���������
	if(usmart_dev.runtimeflag)	//��Ҫ��ʾ����ִ��ʱ��
	{ 
		printf("Function Run Time:%d.%1dms\r\n",usmart_dev.runtime/10,usmart_dev.runtime%10);//��ӡ����ִ��ʱ�� 
	}	
}
//usmartɨ�躯��
//ͨ�����øú���,ʵ��usmart�ĸ�������.�ú�����Ҫÿ��һ��ʱ�䱻����һ��
//�Լ�ʱִ�дӴ��ڷ������ĸ�������.
//�������������ж��������,�Ӷ�ʵ���Զ�����.
//�����ALIENTEK�û�,��USART_RX_STA��USART_RX_BUF[]��Ҫ�û��Լ�ʵ��
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

#if USMART_USE_WRFUNS==1 	//���ʹ���˶�д����
//��ȡָ����ַ��ֵ		 
u32 read_addr(u32 addr)
{
	return *(u32*)addr;//	
}
//��ָ����ַд��ָ����ֵ		 
void write_addr(u32 addr,u32 val)
{
	*(u32*)addr=val; 	
}
#endif













