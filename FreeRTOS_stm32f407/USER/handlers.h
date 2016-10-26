#ifndef __HANDLERS_H
#define __HANDLERS_H	

#include <stdlib.h>
#include <string.h>

/*the interface layer between ymodem layter and packets handler layter */
typedef int ( *PacketHandler )( int devNum, unsigned int index, const char *packet, int plen );
typedef int ( *ErrorHandler )( int devNum, int error );
typedef int ( *FinishHandler )( int devNum );

/*
ymodem handler
packet buffer handler, called it when recevie an buffer
error handler, called it when error happended in transport
finish handler,  called it when finish transport
*/
typedef struct ymodemHandler
{
	int devNum;
	PacketHandler packet_handler;
	ErrorHandler error_handler;
	FinishHandler finish_handler;
}YmodemHandler;


extern int register_ymodem_handlers( int devNum, PacketHandler handler1, 
	ErrorHandler handler2, FinishHandler handler3 );

extern YmodemHandler* get_ymodem_handlers( int devNum );

#endif



