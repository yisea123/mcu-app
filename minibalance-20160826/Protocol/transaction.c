#include "transaction.h"

extern uint32_t SystemTimeCount;
TRANSACTION transaction[200];
struct list_head transactionHead;

void add_transaction_to_list(TRANSACTION *tran)
{
	if (tran) {
		list_add_tail(&(tran->list), &transactionHead);
	}
}

void del_transaction_from_list(TRANSACTION *tran)
{
	TRANSACTION *tmp = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &transactionHead) { 
		tmp = list_entry(pos, TRANSACTION, list); 
		
		if(tmp == tran && tran != NULL) 
		{ 
				list_del(pos); 
		} 
	} 
}

TRANSACTION* get_transaction_from_list()
{
	TRANSACTION *tmp = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &transactionHead) {
		tmp = (TRANSACTION *)list_entry(pos, TRANSACTION, list);
		break;
	}	

	return tmp;
}

int get_total_transaction(void)
{
	int count = 0;
	struct list_head *pos, *n;
	
	list_for_each_safe(pos, n, &transactionHead) {
		count++;	
	}
	
	return count;
}

TRANSACTION * check_event(TRANSACTION *tran)
{
	TRANSACTION *tmp = NULL;
	struct list_head *pos = NULL, *n = NULL;
	
	list_for_each_safe(pos, n, &transactionHead) {
		tmp = (TRANSACTION *)list_entry(pos, TRANSACTION, list);
		if(tmp->time == tran->time && tmp->id == tran->id) {
				return tmp;
		}
	}		
	
	return NULL;
}

TRANSACTION* get_free_transaction()
{
		int i;
		
		for (i=0; i<sizeof(transaction)/sizeof(transaction[0]); i++) {
				if (transaction[i].isUse == 0) {
						return &(transaction[i]);
				}
		}
		
		printf("%s: error!!!\r\n", __func__);
		return NULL;
}

void handle_transaction(int moneyType)
{
	TRANSACTION* tran = get_free_transaction();

	if (!tran) return;
	
	memset(tran, '\0', sizeof(TRANSACTION));
	tran->time = (unsigned long)SystemTimeCount;
	tran->id = 12343211;
	switch (moneyType) {
		case 1:
			tran->coin = 1;
			break;
		case 2:
			tran->paper1 = 1;
			break;
		case 3:
			tran->paper5 = 1;
			break;
		case 4:
			tran->paper10 = 1;
			break;
		default:
			break;
	}
	
	if (tran) {
		tran->isUse = 1;
		add_transaction_to_list(tran);
	}
}

void transaction_init(void)
{
		int i;
		
		for (i=0; i<sizeof(transaction)/sizeof(transaction[0]); i++) {
				transaction[i].isUse = 0;
				transaction[i].id = 0;
		}
		INIT_LIST_HEAD(&transactionHead);   
}




