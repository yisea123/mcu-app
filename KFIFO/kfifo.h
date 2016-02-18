#ifndef __KFIFO_H
#define __KFIFO_H	 
#include "uart_command.h"

enum {
	HEAD1 = 0, //MUST 0
	HEAD2,
	LEN,
	CMD,
	DATA,
	CHECK,
};

#define uint16 unsigned short
#define uint8  unsigned char
	
#define DEBUG_KFIFO_LEN 1024	
//#define uint32_t unsigned int
//512  256  128
#define UART_KFIFO_LEN 1024
//2048
#define CAN_KFIFO_LEN 2048
//1024
#define UART_4G_KFIFO_LEN 2048
//256
#define CMD_LEN 64
#define is_power_of_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define min(a, b) (((a) < (b)) ? (a) : (b))

#define ENABLE_INT()	__set_PRIMASK(0)
#define DISABLE_INT()	__set_PRIMASK(1)

struct kfifo {
    void         *buffer;     /* the buffer holding the data */
    unsigned int size;         /* the size of the allocated buffer */
    unsigned int in;           /* data is added at offset (in % size) */
    unsigned int out;          /* data is extracted from off. (out % size) */
	  unsigned int lostBytes;
	  unsigned int errorCmds;
	  unsigned long acceptCmds;
	  char 				mPrint;
};

struct kfifo* kfifo_init(void *buffer, unsigned int size, void *f_lock);
struct kfifo *kfifo_alloc(unsigned int size, void *lock);
void kfifo_free(struct kfifo *ring_buf);
unsigned int kfifo_len(const struct kfifo *ring_buf);
unsigned int kfifo_get(struct kfifo *ring_buf, void *buffer, unsigned int size);
unsigned int kfifo_put(struct kfifo *ring_buf, void *buffer, unsigned int size);
extern  void reset_fifo(struct kfifo *ring_buf);
void kfifo_clean(struct kfifo *ring_buf);
void printf_kfifo_info(struct kfifo *ring_buf);
void* memcpy(void *des, const void *src, unsigned int n) ;
extern struct kfifo* uart3_fifo;
extern struct kfifo* uart6_fifo;
extern struct kfifo* can1_fifo;
extern struct kfifo* can2_fifo;
extern struct kfifo* debug_fifo;
#endif

