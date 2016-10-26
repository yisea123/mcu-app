#include "handlers.h"
	

/*add handlers in handlers[]*/
static YmodemHandler handlers[6];

static void init_ymodem_handlers( void )
{
	int i;

	for( i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++ )
	{
		handlers[i].devNum = -1;
		handlers[i].packet_handler = NULL;
		handlers[i].error_handler = NULL;
		handlers[i].finish_handler = NULL;
	}
}

/*
*
*return 0 is scueess,  return -1 is fail to register.
*/
int register_ymodem_handlers( int devNum, PacketHandler handler1, 
	ErrorHandler handler2, FinishHandler handler3 )
{	
	int i;
	static char init = 0;

	if( !init )
	{
		init = 1;
		init_ymodem_handlers();
	}

	for( i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++ )
	{
		if( handlers[i].devNum == devNum )
		{
			printf("%s: error devNum!\r\n", __func__);
			return -1;
		}
	}	

	for( i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++ )
	{
		if( handlers[i].devNum == -1 )
		{
			handlers[i].devNum = devNum;
			handlers[i].packet_handler = handler1;
			handlers[i].error_handler = handler2;
			handlers[i].finish_handler = handler3;
			printf("%s: scuess!!!\r\n", __func__);
			return 0;
		}
	}		

	printf("%s: error in max YmodemHandler!\r\n", __func__);
	return -1;
}


/*according the devNum, we get the Ymodem handler.*/
YmodemHandler* get_ymodem_handlers( int devNum )
{
	int i;
	
	for( i = 0; i < sizeof(handlers)/sizeof(handlers[0]); i++)
	{
		if( devNum == handlers[i].devNum )
		{
			return &handlers[i];
		}
	}
	return NULL;
}


