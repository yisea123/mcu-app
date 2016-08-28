#ifndef __MESSAGE_H
#define __MESSAGE_H
#include <string.h> 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "list.h"
#include "malloc.h"

typedef enum
{
    TYPE_EMERGENT			= 1,
    TYPE_NOMAL				= 2,
	
} HANDLETYPE;

typedef enum
{
    TYPE_UNREAD			= 1,
    TYPE_READ				= 2,
	
} HANDLESTATUS;

typedef struct message {
		int size;
		char *data;
		char cmd;
		HANDLETYPE handleType;
		HANDLESTATUS handleStatus;
		void *timer;
		struct list_head list;
} MESSAGE;

extern MESSAGE* get_message(void *timer);
extern MESSAGE* make_message(char cmd, char *data, int size);
extern void deliver_message(MESSAGE *msg, void *timer);
extern void release_message(MESSAGE *msg);
extern void clean_message(void *timer);
#endif
