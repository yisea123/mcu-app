#include "bootloader_update.h"
#include "tmodem.h"
#include "stmflash.h"
#include "delay.h"

extern char mRequestSoftRestPending;
struct list_head bootloader_event_head;
struct bootloaderUpdateState bootloaderState;

void add_b_event_to_list(struct bootloaderUpdateEvent *event)
{
	if(event)
		list_add_tail(&event->list, &bootloader_event_head); //add to tail
}

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

void make_b_event_to_list(struct bootloaderUpdateState *bootloaderState)
{
	struct bootloaderUpdateEvent *event=NULL;
	
	if(bootloaderState->isInit) 
	{
		event = (struct bootloaderUpdateEvent*)mymalloc(0, sizeof(struct bootloaderUpdateEvent));
		if(event == NULL) {
			printf("%s: mymalloc struct bootloaderUpdateEvent fail!\r\n", __func__);
			return;
		}
		
		event->state = bootloaderState;
		STMFLASH_Read((event->state->src), (u32*)event->data, FRAME_SIZE/4);		
		event->state->src+=FRAME_SIZE;
		event->state->index++;
		
		add_b_event_to_list(event);
	}
}

void deal_bootloader_event(struct bootloaderUpdateState *bootloaderState)
{
	unsigned char *buf_ptr;
	unsigned int ramsource = 0, start;
	
	struct bootloaderUpdateEvent *event=NULL;
	event = get_b_event_from_list();
	if(event) 
	{
		buf_ptr = &(event->data[0]);
		ramsource = (unsigned int)buf_ptr;
		start = event->state->des;
		event->state->des = STMFLASH_Write(event->state->des, (unsigned int*) ramsource, FRAME_SIZE/4);	
		
		if((event->state->des - start) != FRAME_SIZE) {
			printf("%s: burn bootloader error\r\n", __func__);
			event->state->des = start;
			return;
		} else {
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
				reset_bootloaderUpdateState(event->state);
				mRequestSoftRestPending = 1;
			}
			else
			{
				printf("%s:burn booloader fail!, try again!\r\n", __func__);
				init_bootloaderUpdateState(event->state->mBootloaderUpdatePending,
					event->state->romSize, event->state->md5);			
			}
		}
		myfree(0, event);
		event = NULL;
	}
}

void handle_bootloader_update_work(void)
{
	if(*(bootloaderState.mBootloaderUpdatePending) == 1) 
	{
		if(list_b_event() == 0)  
		{
			make_b_event_to_list(&bootloaderState);
		}
		if(list_b_event() > 0) 
		{
			deal_bootloader_event(&bootloaderState);
		}
	}
}




