#ifndef __AES_H
#define __AES_H	 

#include <stdio.h>

#include <stdlib.h>

#include <stdint.h>

#include "malloc.h"

#define AES_DECODE_LEN			(16)

extern uint8_t * aes_create_key( uint8_t key[], int len );

extern void aes_destory_key(uint8_t * w);

extern uint8_t* aes_decode_packet(uint8_t* key, uint8_t *packet_data, int packet_length);

extern uint8_t* aes_encode_packet(uint8_t* key, uint8_t *packet_data, int packet_length);

#endif




