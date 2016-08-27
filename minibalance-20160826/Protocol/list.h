#ifndef __LIST_H
#define __LIST_H	 

struct list_head{
	struct list_head *next, *prev;
};

#define offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
//#define container_of(ptr, type, member) ({			\
//	const typeof(((type *)0)->member) * __mptr = (ptr);	\
//	(type *)((char *)__mptr - offsetof(type, member)); })

#define container_of(ptr, type, member)  (type *)( (char *)ptr - offsetof(type,member) )
	
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)
		
#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)
	
#define list_for_each_prev(pos, head) \
	for (pos = (head)->prev; pos != (head); pos = pos->prev)

#define list_for_each_safe(pos, n, head) \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)
		
#define list_for_each_prev_safe(pos, n, head) \
	for (pos = (head)->prev, n = pos->prev; \
	     pos != (head); \
	     pos = n, n = pos->prev)		

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

extern void INIT_LIST_HEAD(struct list_head *list);
extern void list_add(struct list_head *new, struct list_head *head);
extern void list_add_tail(struct list_head *new, struct list_head *head);
extern void list_del(struct list_head *entry);
extern void __list_del(struct list_head * prev, struct list_head * next);
extern void __list_del_entry(struct list_head *entry);
extern void list_move(struct list_head *list, struct list_head *head);	
extern void list_move_tail(struct list_head *list, struct list_head *head);
extern int list_empty(const struct list_head *head);
	
#endif



