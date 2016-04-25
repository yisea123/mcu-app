#ifndef __BOOTLOADER_UPDATE_H
#define __BOOTLOADER_UPDATE_H	 

#include "list.h"
#include "malloc.h"
#include "usart.h"
#include "kfifo.h"
#include "md5.h"

#define FRAME_SIZE 			(128)

/**********************************************
����һ��ȫ�ֵı������������bootloader�Ľ��ȡ�
***********************************************/
struct bootloaderUpdateState {
		char isInit;				
	  int index;
		int romSize;
		unsigned int des;
		unsigned int src;	
	  unsigned char md5[LEN_MD5];
		char *mBootloaderUpdatePending;
};

/*********************************************************************************************************
ÿ���¼����й̼����ݣ�����ָ��bootloader��д״̬��ָ�룬ͨ�����ָ��Ϳ����ҵ� ����Ҫ���浽flash���ĸ���ַ
**********************************************************************************************************/
struct bootloaderUpdateEvent {
		unsigned char data[130];
		struct bootloaderUpdateState* state;
		struct list_head list;
};

extern void bootloader_init(void);
extern struct list_head bootloader_event_head;
extern struct bootloaderUpdateState bootloaderState;
extern void handle_bootloader_update_work(void);
extern void init_bootloaderUpdateState(char* mBootloaderUpdatePending, unsigned int romSize, const unsigned char* md5);
#endif
