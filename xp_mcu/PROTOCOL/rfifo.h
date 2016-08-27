#ifndef __RFIFO_H
#define __RFIFO_H	
#include "sys.h"
#include <stdlib.h>
#include <string.h>

#define MAX_RINGBUFFER_LEN			512	
#define IS_POWER_OF_2(x) ((x) != 0 && (((x) & ((x) - 1)) == 0))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))

typedef struct rfifo {
    unsigned char buffer[MAX_RINGBUFFER_LEN];  /* the buffer holding the data*/
    unsigned int size;         /* the size of the allocated buffer */
    unsigned int in;           /* data is added at offset (in % size) */
    unsigned int out;          /* data is extracted from off. (out % size) */
	char 		 lost;
} Ringfifo;

extern struct rfifo* rfifo_init(struct rfifo *ring);
extern unsigned int rfifo_len(struct rfifo *ring);
extern unsigned int rfifo_get(struct rfifo *ring, void *buffer, unsigned int size);
extern unsigned int rfifo_put(struct rfifo *ring, void *buffer, unsigned int size);
extern void rfifo_clean(struct rfifo *ring);

#endif



