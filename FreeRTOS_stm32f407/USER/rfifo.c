#include "rfifo.h"
#include "stdio.h"

/*
* author: yangjianzhou
* function: init ring buffer.
**/
struct rfifo* rfifo_init(struct rfifo *ring)
{
	int size = MAX_RBUFFER_LEN;
	
	if (!ring) {
		 printf("ring_buf is NULL\r\n");
		 return NULL;
	}
	if (!IS_POWER_OF_2(size)) {
		 printf("size must be power of 2\r\n");
		 return NULL;
	}
	ring->hasInit = 1;
	ring->size = MAX_RBUFFER_LEN;
	ring->in = 0;
	ring->out = 0;
	ring->lost = 0;
  ring->lostBytes = 0;
	
	return ring;
}

/*
*	author: yangjianzhou
* function: __rfifo_len.
**/
unsigned int __rfifo_len(const struct rfifo *ring)
{
	return (ring->in - ring->out);
}
 
/*
*	author: yangjianzhou
* function: __rfifo_get.
**/
unsigned int __rfifo_get(struct rfifo *ring, void * buffer, unsigned int size)
{
	 unsigned int len = 0;
	 unsigned int tmp = 0;
	 
	 tmp = ring->in - ring->out;
	 size  = MIN(size, tmp);  
	 len = MIN(size, ring->size - (ring->out & (ring->size - 1)));
	 memcpy(buffer, ((char*)ring->buffer + (ring->out & (ring->size - 1))), len);
	 memcpy((char*)buffer + len, (char*)ring->buffer, size - len);
	 ring->out += size;
	 
	 return size;
}

/*
*	author: yangjianzhou
* function: __rfifo_put.
**/
unsigned int __rfifo_put(struct rfifo *ring, void *buffer, unsigned int size)
{
	unsigned int len = 0;
	
	size = MIN(size, ring->size - ring->in + ring->out);
	len = MIN(size, ring->size - (ring->in & (ring->size - 1)));
	memcpy((char*)ring->buffer + (ring->in & (ring->size - 1)), buffer, len);
	memcpy(ring->buffer, (char*)buffer + len, size - len);
	ring->in += size;
	
	return size;
}

/*
* author: yangjianzhou
* function: return the len of data in the fifo.
**/ 
unsigned int rfifo_len(struct rfifo *ring)
{
	unsigned int len = 0;
	len = __rfifo_len(ring);
	return len;
}

 /*
* author: yangjianzhou
* function: get buffer from fifo.
**/ 
unsigned int rfifo_get(struct rfifo *ring, void *buffer, unsigned int size)
{
	unsigned int ret;
	ret = __rfifo_get(ring, buffer, size);
	
	return ret;
}

 /*
* author: yangjianzhou
* function: put buffer to fifo.
**/ 
unsigned int rfifo_put(struct rfifo *ring, void *buffer, unsigned int size)
{
	unsigned int ret;
	ret = __rfifo_put(ring, buffer, size);
	if (ret != size) {
		ring->lost = 1;
	}
	
	return ret;
}

/*
*	author: yangjianzhou
* function: rfifo_clean.
**/
void rfifo_clean(struct rfifo *ring)
{
	ring->in = 0;
	ring->out = 0;
}



