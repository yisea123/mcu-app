#ifndef __TEST_H
#define __TEST_H	 

#include "can.h"
#include "usart.h"

extern int send_can_mesg(void);
extern int send_can_mesg1(uint32_t id);
extern void report_mesg_to_t8(void);

#endif

