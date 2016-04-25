#include "bootloader_update.h"
#include "tmodem.h"
#include "stmflash.h"
#include "delay.h"

extern char mRequestSoftRestPending;

struct list_head bootloader_event_head;

struct bootloaderUpdateState bootloaderState;

/***************************************************************
把数据加入到链表中去
****************************************************************/
void add_b_event_to_list(struct bootloaderUpdateEvent *event)
{
	if(event)
		list_add_tail(&event->list, &bootloader_event_head); //add to tail
}

/***************************************************************
如果此数据可以在链表中找到，从链表中删除这个数据
****************************************************************/
void del_b_event_from_list(struct bootloaderUpdateEvent *event)
{
	struct bootloaderUpdateEvent *mevent = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &bootloader_event_head) 
	{ 
		mevent = list_entry(pos, struct bootloaderUpdateEvent, list); 
		
		if(mevent == event && event != NULL) 
		{ 
				list_del(pos); 
		} 
	} 
}

/***************************************************************
如果链表里面没有数据，则返回NULL，如果有数据就返回一个数据
****************************************************************/
struct bootloaderUpdateEvent* get_b_event_from_list()
{
	struct bootloaderUpdateEvent *event = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &bootloader_event_head)
	{
		event = (struct bootloaderUpdateEvent *)list_entry(pos, struct bootloaderUpdateEvent, list);
		break;
	}	

	return event;
}

/*************************************
遍历链表中的数据，返回链表中数据的个数
**************************************/
int list_b_event(void)
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &bootloader_event_head)
	{
		count++;	
	}
	
	return count;
}

extern unsigned int FLASH_If_Erase_Sector(unsigned int StartSector);

void init_bootloaderUpdateState(char* mBootloaderUpdatePending, unsigned int romSize, const unsigned char* md5)
{
	printf("%s:romSize=%d , mBootloaderUpdatePending=%d\r\n", __func__, romSize, *mBootloaderUpdatePending);
	bootloaderState.mBootloaderUpdatePending = mBootloaderUpdatePending;
	bootloaderState.isInit = 1;
	bootloaderState.index = 0;
	bootloaderState.romSize = romSize;
	bootloaderState.src = BOOTLOADER_ADDRESS;
	bootloaderState.des = BOOTLOADER;	
	memcpy(bootloaderState.md5, md5, LEN_MD5);
	FLASH_If_Erase_Sector(bootloaderState.des);
}

void reset_bootloaderUpdateState(struct bootloaderUpdateState *state)
{
	printf("%s:\r\n", __func__);
	*(state->mBootloaderUpdatePending) = 0;
	bootloaderState.isInit = 0;
	state->index = 0;
	state->romSize = 0;
	state->src = BOOTLOADER_ADDRESS;
	state->des = BOOTLOADER;		
}

/***********************************************************************
生成一个bootloader烧录事件，从flash中读取固件数据保存到事件结构休的data域
中，然后更新bootloaderState进度。
***********************************************************************/
void make_b_event_to_list(struct bootloaderUpdateState *bootloaderState)
{
	struct bootloaderUpdateEvent *event=NULL;
	
	//printf("%s\r\n", __func__);
	if(bootloaderState->isInit) 
	{
		event = (struct bootloaderUpdateEvent*)mymalloc(0, sizeof(struct bootloaderUpdateEvent));
		
		if(event == NULL) 
		{
			printf("%s: mymalloc struct bootloaderUpdateEvent fail!\r\n", __func__);
			return;
		}
		
		event->state = bootloaderState;
		/*开始烧录后，第一帧从BOOTLOADER_ADDRESS读取数据128bytes, 然后把src指到BOOTLOADER_ADDRESS+128，
		下一次就从BOOTLOADER_ADDRESS+128开始读取*/
		STMFLASH_Read((event->state->src), (u32*)event->data, FRAME_SIZE/4);
		
		/****************************
						更新烧录状态
		*****************************/
		event->state->src+=FRAME_SIZE;
		event->state->index++;
		/*第一帧index=1 第二帧index=2...*/
		
		add_b_event_to_list(event);
	}
}

/***********************************************************************
从链表中获取要操作的事件，事件中有固件的数据包，还有要写的flash目标地址
如果是bootloader烧录事件， 写完就可以把事件删除，如果是scu烧录事件，写完
可能还要等待scu回复后才可以删除数据，需要根据协议来。
***********************************************************************/
void deal_bootloader_event(struct bootloaderUpdateState *bootloaderState)
{
	unsigned char *buf_ptr;
	unsigned int ramsource = 0, start;
	
	struct bootloaderUpdateEvent *event=NULL;
	event = get_b_event_from_list();
	//printf("%s: event=%p\r\n", __func__, event);
	if(event) 
	{
		buf_ptr = &(event->data[0]);
		ramsource = (unsigned int)buf_ptr;
		start = event->state->des;
		event->state->des = STMFLASH_Write(event->state->des, (unsigned int*) ramsource, FRAME_SIZE/4);	
		
		if((event->state->des - start) != FRAME_SIZE) 
		{
			/*如果写不成功，还原des的指向，直接返回，等待下一次进入这个函数*/
			printf("%s: burn bootloader error\r\n", __func__);
			event->state->des = start;
			return;
		}
		else
		{
			/*如果写成功，从链表中去除这个节点，这样，就可再次进入make_b_event_to_list*/
			printf("%s: write bootloader success.\r\n", __func__);
			del_b_event_from_list(event);
		}
		
		if((event->state->des - BOOTLOADER) > event->state->romSize+16)
		{
			printf("event->state->des - BOOTLOADER =%d, bootloader size=%d, bootloader burn finish!\r\n", 
				(event->state->des - BOOTLOADER), event->state->romSize);
			
			if(check_md5(BOOTLOADER, event->state->romSize, event->state->md5) == 0) 
			{
				printf("%s:burn booloader 100%% ok!\r\n", __func__);
				/*romSize的大小都写入到0x08000000开始的位置，代表烧录完成！*/
				reset_bootloaderUpdateState(event->state);
				//delay_ms(20);
				mRequestSoftRestPending = 1;
			}
			else
			{
				printf("%s:burn booloader fail!, try again!\r\n", __func__);
				init_bootloaderUpdateState(event->state->mBootloaderUpdatePending,
					event->state->romSize, event->state->md5);			
			}
		}
		/*释放内存空间*/
		myfree(0, event);
		event = NULL;
	}
}

void handle_bootloader_update_work(void)
{
	/*开始了烧录bootloader请求*/
	if(*(bootloaderState.mBootloaderUpdatePending) == 1) 
	{
		/*如果没有烧录的数据，读取flash数据，保存到链表节点的数据域中*/
		if(list_b_event() == 0)  
		{
			make_b_event_to_list(&bootloaderState);
		}
		
		/*链表中有要烧录的数据，把数据写到bootloader的启动扇区*/
		if(list_b_event() > 0) 
		{
			deal_bootloader_event(&bootloaderState);
		}
	}
}

void bootloader_init(void)
{
		INIT_LIST_HEAD(&bootloader_event_head);   	
}


