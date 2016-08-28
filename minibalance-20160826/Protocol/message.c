#include "message.h"
#include "timer.h"

MESSAGE* make_message(char cmd, char *data, int size)
{
		MESSAGE *msg = NULL;
		msg = (MESSAGE*) mymalloc(0, sizeof(MESSAGE));
		if (msg) {
				msg->data = (char *)mymalloc(0, size);
				if (msg->data == NULL) {
						myfree(0, msg);
						msg = NULL;
						printf("%s: malloc message->data fail\r\n", __func__);
				} else {
						memcpy(msg->data, data, size);
						msg->size = size;
						msg->cmd = cmd;
						msg->handleType = TYPE_EMERGENT;
						//printf("msg=%p, data=%02x\r\n", msg, *(msg->data));
				}
		} else {
				printf("%s: malloc message fail\r\n", __func__);
		}
		
		return msg;
}

void deliver_message(MESSAGE *msg, void *timer)
{
	if (timer && msg) {
		msg->handleStatus = TYPE_UNREAD;
		msg->timer = timer;
		list_add_tail(&(msg->list), &(((SYSTIMER*)timer)->msgHead));
	}
}

void release_message(MESSAGE *msg)
{
	MESSAGE *tmp = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	if (!msg) return;
	
	list_for_each_safe(pos, n, &(((SYSTIMER*)msg->timer)->msgHead)) { 
		tmp = list_entry(pos, MESSAGE, list); 
		if (tmp == msg) { 
				list_del(pos); 
				if (msg->data && msg->size > 0) {
						myfree(0, msg->data);
						myfree(0, msg);
				}
		} 
	} 
}

void clean_message(void *timer)
{
	MESSAGE *tmp = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &(((SYSTIMER*)timer)->msgHead)) { 
		tmp = list_entry(pos, MESSAGE, list); 
		if (tmp->data && tmp->size > 0) {
				myfree(0, tmp->data);
		}		
		list_del(pos); 
	} 
}

MESSAGE* get_message(void *timer)
{
	MESSAGE *tmp = NULL;
	struct list_head *pos=NULL, *n=NULL;
	
	list_for_each_safe(pos, n, &(((SYSTIMER*)timer)->msgHead)) {
		tmp = (MESSAGE *)list_entry(pos, MESSAGE, list);
		tmp->handleStatus = TYPE_READ;
		//printf("get_message msg=%p\r\n", tmp);		
		break;
	}	

	return tmp;
}