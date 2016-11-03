#ifndef __COMHANDLER_H
#define __COMHANDLER_H	
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"	
#include "can.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TYPE_EVENT		1
#define TYPE_PEROID		2

#define CAN_B			1
#define CAN_E			2

#define SIZE_ARRAY(a) 	(sizeof(a) / sizeof((a)[0]))

typedef struct {
	char 			canx;			/*can Number*/
	int 			id;				/*can message ID*/
	char 			*data;			/*can message DATA*/
	int 			period;			/*period for timer*/
	char    		*pvTimerID;		/*for indentify timer*/
	char    	 	type;			/*TYPE_EVENT or TYPE_PEROID*/
	char 		  	count;			/*for event can message send count*/
	TimerHandle_t 	timer;			/*store the TimerHandle_t struct*/
}CanMsgPriv;

extern int handle_can_command( char *data, int len );

extern void HandleCanCommandTask(void * pvParameters);

extern void Printf_System_Jiffies( void );

extern void stop_timer_work( void );

extern void start_timer_work( void );

extern unsigned char canb_send_lock(unsigned int id, unsigned char* msg, unsigned char len);

extern unsigned char cane_send_lock(unsigned int id, unsigned char* msg, unsigned char len);

#endif



