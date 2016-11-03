#include "cmdhandler.h"

static char cStart = 1;
static xSemaphoreHandle mCanSendMutex = NULL;

/*
* add can message here
*/
static char data358[8] = { 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static char data34a[8] = { 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static char data203[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data205[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data335[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data03d[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data351[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data03e[8] = { 0xaa, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static char data03f[8] = { 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static char data339[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data33e[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data040[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data04f[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data047[8] = { 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11 };
static char data35a[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
static char data15e[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

/*
* add can message here
*/
static CanMsgPriv pcMsg[] = 
{
	CAN_B, 0x358, data358, 100, "358", TYPE_PEROID, 0XFF, NULL, NULL,
	CAN_B, 0x34a, data34a, 100, "34a", TYPE_PEROID, 0XFF, NULL, NULL,
	CAN_B, 0x203, data203, 30, 	"203", TYPE_EVENT, 	0x00, NULL, NULL,		
	CAN_B, 0x205, data205, 30,	"205", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x205, data335, 100,	"335", TYPE_PEROID,	0x00, NULL, NULL, 	
	CAN_B, 0x3d,  data03d, 30,	"03d", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x351, data351, 100,	"351", TYPE_PEROID, 0x00, NULL, NULL, 	
	CAN_B, 0x3d,  data03e, 30,	"03e", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x3d,  data03f, 30,	"03f", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x339, data339, 100, "339", TYPE_PEROID, 0XFF, NULL, NULL,
	CAN_B, 0x33e, data33e, 100, "33e", TYPE_PEROID, 0XFF, NULL, NULL,	
	CAN_B, 0x40,  data040, 30,	"040", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x4f,  data04f, 30,	"04f", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x47,  data047, 30,	"047", TYPE_EVENT,	0x00, NULL, NULL, 	
	CAN_B, 0x35a, data35a, 100, "35a", TYPE_PEROID, 0XFF, NULL, NULL,
	CAN_B, 0x15e, data15e, 100, "15e", TYPE_PEROID, 0XFF, NULL, NULL,	
};

/*
* author: 	yangjianzhou
* function: 	obtain_can_mutex : get mCanSendMutex mutex.
*/
static void obtain_can_mutex( void )
{
	xSemaphoreTake( mCanSendMutex, portMAX_DELAY );	
}

/*
* author: 	yangjianzhou
* function: 	release_can_mutex : release mCanSendMutex mutex.
*/
static void release_can_mutex( void )
{
	xSemaphoreGive( mCanSendMutex );	
}

/*
* author: 	yangjianzhou
* function: 	canb_send_lock : send can message lock.
*/
unsigned char canb_send_lock(unsigned int id, unsigned char* msg, unsigned char len)
{
	unsigned char ret;
	
	obtain_can_mutex();
	ret = CAN1_Send_Msg1( id, msg, len );
	release_can_mutex();

	return ret;
}

/*
* author: 	yangjianzhou
* function: 	cane_send_lock : send can message lock.
*/
unsigned char cane_send_lock(unsigned int id, unsigned char* msg, unsigned char len)
{
	unsigned char ret;

	obtain_can_mutex();
	ret = CAN2_Send_Msg1( id, msg, len );
	release_can_mutex();

	return ret;	
}

/*
* author: 	yangjianzhou
* function: 	start_timer_work : stop can message send timer.
*/
void stop_timer_work( void )
{
	int i;
	
	cStart = 0;
	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		if( pcMsg[i].timer )
		{
			if( pdFAIL == xTimerStop( pcMsg[i].timer, 0 ) )
			{
				printf("%s fail to stop timer.\r\n", __func__);
			}
		}
	}
}

/*
* author: 	yangjianzhou
* function: 	start_timer_work : start can message send timer.
*/
void start_timer_work( void )
{
	int i;

	cStart = 1;
	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		if( pcMsg[i].timer && pcMsg[i].type == TYPE_PEROID )
		{
			if( pdFAIL == xTimerStart( pcMsg[i].timer, 0 ) )
			{
				printf("%s fail to start timer.\r\n", __func__);
			}			
		}
	}
}

/*
* author: 	yangjianzhou
* function: 	handle_can_command : handle android can command.
*/
int handle_can_command( char *data, int len)
{
	unsigned int id, canx, i;
	char msg[CAN_DATA_LEN], tmp[60];
	CanMsgPriv *p = NULL;
	
	id = *( ( unsigned int * ) data );
	canx = data[4] & 0x01;
	memcpy( msg, &data[5], 8 );

	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		p = &( pcMsg[i] );
		
		if( p->id == id )
		{
			if( p->type == TYPE_PEROID )
			{
				xSemaphoreTake( p->mutex, portMAX_DELAY );
				memcpy( p->data, msg, CAN_DATA_LEN );
				xSemaphoreGive( p->mutex );
			}
			else if( p->type == TYPE_EVENT )
			{
				p->count = EVENT_COUNT;							
				xSemaphoreTake( p->mutex, portMAX_DELAY );			
				memcpy( p->data, msg, CAN_DATA_LEN );
				xSemaphoreGive( p->mutex );

				if( cStart && pdFAIL == xTimerReset( p->timer, 1 ))
				{
					printf("%s fail to xTimerReset.\r\n", __func__);;
				}
			}
			break;
		}
	}

	sprintf(tmp, "id(0x%x), can(%c), [%02x %02x %02x %02x %02x %02x %02x %02x]",
		id, canx == 0 ? 'B' : 'E', msg[0], msg[1], msg[2], msg[3], msg[4], 
		msg[5], msg[6], msg[7]);
	
	printf("%s\r\n", tmp);
	
	return 0;
}

/*
* author: 	yangjianzhou
* function: 	do_send_can_msg timer handle fucntion, which do send can message.
*/
void do_send_can_msg( TimerHandle_t xTimer )
{
	CanMsgPriv *pPriv;
	unsigned char message[CAN_DATA_LEN];
	
	pPriv = pvTimerGetPrivate( xTimer );	
	if( pPriv == NULL )
	{
		printf("%s pvTimerGetPrivate return Null.\r\n", __func__);
		return;
	}
	
	xSemaphoreTake( pPriv->mutex, portMAX_DELAY );
	memcpy( message, pPriv->data, CAN_DATA_LEN );	
	xSemaphoreGive( pPriv->mutex );
	
	if( pPriv->canx == CAN_B ) 
	{
		canb_send_lock( pPriv->id, message, CAN_DATA_LEN );
	}
	else if( pPriv->canx == CAN_E )
	{
		cane_send_lock( pPriv->id, message, CAN_DATA_LEN );
	}	
	
	if( pPriv->type == TYPE_EVENT )
	{
		if( --( pPriv->count ) <= 0 )
		{
			if( pdFAIL == xTimerStop( pPriv->timer, 1 ) )
			{
				printf("%s fail to stop timer.\r\n", __func__);
			}					
		}
	}
}

/*
* author: 	yangjianzhou
* function: 	HandleCanCommandTask init timer.
*/
void HandleCanCommandTask(void * pvParameters)
{
	int i;
	int count = 1;
	static TimerHandle_t *mTimerHandler;
	CanMsgPriv *p = NULL;

	mCanSendMutex = xSemaphoreCreateMutex();
	vSetTaskLogLevel( NULL, eLogLevel_3 );	
	printf("%s ...\r\n", __func__);

	mTimerHandler = ( TimerHandle_t * ) 
		pvPortMalloc( SIZE_ARRAY( pcMsg ) * sizeof( TimerHandle_t ) );
	
	if( !mTimerHandler ) {
		printf("%s fail to pvPortMalloc mTimerHandler!\r\n", __func__);
	}

	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		if( TYPE_PEROID == pcMsg[i].type )
		{
			count++;
		}
	}

	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		p = &( pcMsg[i] );	
		p->mutex = xSemaphoreCreateMutex();
	    if( p->mutex == NULL )
	    {
			printf("%s fail to create mutex.\r\n", __func__);
	    }		
		mTimerHandler[i] = xTimerCreate( p->pvTimerID, p->period / portTICK_RATE_MS, 
			pdTRUE, p->pvTimerID, do_send_can_msg );
		
		if( mTimerHandler[i] )
		{	
			p->timer = mTimerHandler[i];
			vTimerSetPrivate( mTimerHandler[i], p );
			
			if( TYPE_PEROID == p->type )
			{
				if( pdFAIL == xTimerStart( mTimerHandler[i], 0 ) )
				{
					printf("%s fail to start timer(mTimerHandler[%d])\r\n", __func__, i);
				}
				vTaskDelay( ( 100 / count ) / portTICK_RATE_MS );				
			}
		} 
		else
		{
			printf("%s fail to pvPortMalloc mTimerHandler[i]!\r\n", __func__);
		}
	}	
	
	while (1)
	{
		ulTaskNotifyTake( pdFALSE, portMAX_DELAY );		
	}
	
}





