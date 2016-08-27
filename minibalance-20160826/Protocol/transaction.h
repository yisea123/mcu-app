#ifndef __TRANSACTION_H
#define __TRANSACTION_H	 

#include "list.h"
#include "rfifo.h"


typedef struct transaction
{
	char isUse;
	unsigned long id;
	unsigned long time;
	char coin;
	char paper1;
	char paper5;
	char paper10;
	char hasSave;
	struct list_head list;
} TRANSACTION;

#endif
