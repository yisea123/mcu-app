#include "cmdhandler.h"

static char cStart = 1;
static xSemaphoreHandle mCanSendMutex = NULL;

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

static CanMsgPriv pcMsg[] = 
{
	CAN_B, 0x358, data358, 100, "358", TYPE_PEROID, 0XFF, NULL,
	CAN_B, 0x34a, data34a, 100, "34a", TYPE_PEROID, 0XFF, NULL,
	CAN_B, 0x203, data203, 30, 	"203", TYPE_EVENT, 	0x00, NULL,		
	CAN_B, 0x205, data205, 30,	"205", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x205, data335, 100,	"335", TYPE_PEROID,	0x00, NULL, 	
	CAN_B, 0x3d,  data03d, 30,	"03d", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x351, data351, 100,	"351", TYPE_PEROID, 0x00, NULL, 	
	CAN_B, 0x3d,  data03e, 30,	"03e", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x3d,  data03f, 30,	"03f", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x339, data339, 100, "339", TYPE_PEROID, 0XFF, NULL,
	CAN_B, 0x33e, data33e, 100, "33e", TYPE_PEROID, 0XFF, NULL,	
	CAN_B, 0x40,  data040, 30,	"040", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x4f,  data04f, 30,	"04f", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x47,  data047, 30,	"047", TYPE_EVENT,	0x00, NULL, 	
	CAN_B, 0x35a, data35a, 100, "35a", TYPE_PEROID, 0XFF, NULL,
	CAN_B, 0x15e, data15e, 100, "15e", TYPE_PEROID, 0XFF, NULL,	
};

static void obtain_can_mutex( void )
{
	xSemaphoreTake( mCanSendMutex, portMAX_DELAY );	
}

static void release_can_mutex( void )
{
	xSemaphoreGive( mCanSendMutex );	
}

unsigned char canb_send_lock(unsigned int id, unsigned char* msg, unsigned char len)
{
	unsigned char ret;
	
	obtain_can_mutex();
	ret = CAN1_Send_Msg1( id, msg, len );
	release_can_mutex();

	return ret;
}

unsigned char cane_send_lock(unsigned int id, unsigned char* msg, unsigned char len)
{
	unsigned char ret;

	obtain_can_mutex();
	ret = CAN2_Send_Msg1( id, msg, len );
	release_can_mutex();

	return ret;	
}

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

int handle_can_command( char *data, int len)
{
	unsigned int id, canx, i;
	char msg[8], tmp[60];
	
	id = *((unsigned int *)data);
	canx = data[4] & 0x01;
	memcpy(msg, &data[5], 8);

	sprintf(tmp, "id(0x%x), can(%c), [%02x %02x %02x %02x %02x %02x %02x %02x]",
		id, canx == 0 ? 'B' : 'E', msg[0], msg[1], msg[2], msg[3], msg[4], 
		msg[5], msg[6], msg[7]);
	
	printf("%s\r\n", tmp);

	for ( i = 0; i< SIZE_ARRAY(pcMsg); i++ ) 
	{
		if( pcMsg[i].id == id )
		{
			if( pcMsg[i].type == TYPE_PEROID )
			{
				memcpy( pcMsg[i].data, msg, 8 );
			}
			else if( pcMsg[i].type == TYPE_EVENT )
			{
				pcMsg[i].count = 3;
				memcpy( pcMsg[i].data, msg, 8 );
				if( cStart && pdFAIL == xTimerReset( pcMsg[i].timer, 1 ))
				{
					printf("%s fail to xTimerReset.\r\n", __func__);;
				}
			}
			return 0;
		}
	}

	return 0;
}

void do_send_can_msg( TimerHandle_t xTimer )
{
	CanMsgPriv *pPriv;
	
	pPriv = pvTimerGetPrivate( xTimer );	
	if( pPriv == NULL )
	{
		printf("%s pvTimerGetPrivate return Null.\r\n", __func__);
		return;
	}
	
	if( pPriv->canx == CAN_B ) 
	{
		canb_send_lock( pPriv->id, ( unsigned char* )( pPriv->data ), 8 );
	}
	else if( pPriv->canx == CAN_E )
	{
		cane_send_lock( pPriv->id, ( unsigned char* )( pPriv->data ), 8 );
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

void HandleCanCommandTask(void * pvParameters)
{
	int i;
	int count = 1;
	static TimerHandle_t *mTimerHandler;

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
		mTimerHandler[i] = xTimerCreate( pcMsg[i].pvTimerID, pcMsg[i].period / portTICK_RATE_MS, 
			pdTRUE, pcMsg[i].pvTimerID, do_send_can_msg );
		
		if( mTimerHandler[i] )
		{	
			pcMsg[i].timer = mTimerHandler[i];
			vTimerSetPrivate( mTimerHandler[i], &( pcMsg[i] ) );
			
			if( TYPE_PEROID == pcMsg[i].type )
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








//vTaskDelay( 8000 / portTICK_RATE_MS );




