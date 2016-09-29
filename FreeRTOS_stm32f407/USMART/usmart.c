#include "usmart.h"
#include "rfifo.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "stdio.h"

//#include "usart.h"

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

static char pBuf[512];

void getAllTask( void )
{
		vTaskList( pBuf );
		printf("Name   \t Status\t Priority   WaterMark   Num\r\n");
		printf("%s", pBuf);
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
			printf("toKill = %p Error!\r\n", target);
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
	/*For FreeRTOS*/
	"ps",
	"kill",
	"suspend",
	"resume",
	"abort",
	"exit",
};	    
//����ϵͳָ��
//0,�ɹ�����;����,�������;
u8 usmart_sys_cmd_exe( u8 *str )
{
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
			printf("\r\n");
#if USMART_USE_HELP
			printf("Debugϵͳ��������:\r\n");
			printf("?:    ��ȡ������Ϣ\r\n");
			printf("help: ��ȡ������Ϣ\r\n");
			printf("list: ���õĺ����б�\r\n");
			printf("id:   ���ú�����ID�б�\r\n");
			vTaskDelay(5 / portTICK_RATE_MS);
			printf("hex:  ����16������ʾ,����ո�+���ּ�ִ�н���ת��\r\n");
			printf("dec:  ����10������ʾ,����ո�+���ּ�ִ�н���ת��\r\n");
			printf("runtime:1, �����������м�ʱ;0,�رպ������м�ʱ;\r\n");
			vTaskDelay(5 / portTICK_RATE_MS);
			printf("----------------FreeRTOS---------------\r\n");
			printf("ps:	    �г�FreeRTOS����������Ϣ\r\n");		
			printf("kill:	  ����ո�+�������֣�ɱ��ָ�����ֵ�����\r\n");		
			printf("suspend:����ո�+�������֣�����ָ�����ֵ�����\r\n");	
			vTaskDelay(5 / portTICK_RATE_MS);
			printf("resume: ����ո�+�������֣�������ָ���ready״̬\r\n");		
			printf("abort:  ����ո�+�������֣�������ĳ�ready״̬\r\n");	
			printf("----------------FreeRTOS---------------\r\n");		
			//printf("�밴�ճ����д��ʽ���뺯�������������Իس�������.\r\n");    
			//printf("--------------------------------------------- \r\n");
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
			getAllTask();
			break;
		case 8:
			Deal_Command_For_Task(str, 1);
			break;
		case 9:
			Deal_Command_For_Task(str, 2);
			break;
		case 10:
			Deal_Command_For_Task(str, 3);
			break;		
		case 11:
			Deal_Command_For_Task(str, 4);
			break;			
		case 12:
			Deal_Command_For_Task(str, 5);
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
Ringfifo uart1fifo;

void usamrt_debug_task(void *argc) 
{
	int len;
	
	printf("%s start...\r\n", __func__);
	usmart_dev.sptype = 0;	//0,10����;1,16����;
	rfifo_init(&uart1fifo);	
	xDebugQueue = xQueueCreate( 20, sizeof(int));

	while (1) {
		if( xQueueReceive( xDebugQueue, &len, portMAX_DELAY ) ) {
				usmart_dev.scan(len);	//ִ��usmartɨ��
		} else { 
			printf("%s-%d xQueueReceive error!\r\n", __func__,  __LINE__);
		}
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

void usmart_scan(int uart1DataLen)
{
	u8 sta,len;
	if (rfifo_len(&uart1fifo) > 0 && uart1DataLen > 0) {
		len = rfifo_get(&uart1fifo, USART_RX_BUF, uart1DataLen);
		if (USART_RX_BUF[len-1] == 0x0a && 
				USART_RX_BUF[len-2] == 0x0d)
		{		
			len -= 2;
			USART_RX_BUF[len]='\0'; 
			sta=usmart_dev.cmd_rec(USART_RX_BUF);
			if(sta==0)usmart_dev.exe();
			else 
			{  
				len=usmart_sys_cmd_exe(USART_RX_BUF);
				if(len!=USMART_FUNCERR)sta=len;
				if(sta)
				{
					switch(sta)
					{
						case USMART_FUNCERR:
							printf("��������!\r\n");   			
							break;	
						case USMART_PARMERR:
							printf("��������!\r\n");   			
							break;				
						case USMART_PARMOVER:
							printf("����̫��!\r\n");   			
							break;		
						case USMART_NOFUNCFIND:
							printf("δ�ҵ�ƥ��ĺ���!\r\n");   			
							break;		
					}
				}
			}
			USART_RX_STA=0;//״̬�Ĵ������	    
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













