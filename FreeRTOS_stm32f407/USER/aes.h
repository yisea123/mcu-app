#ifndef __AES_H
#define __AES_H	 

#include <stdio.h>

#include <stdlib.h>

#include <stdint.h>

#include <string.h>

#define AES_DECODE_LEN			(16)

extern unsigned char * aes_create_key( unsigned char key[], int len );

extern void aes_destory_key( unsigned char ** w );

extern unsigned char* aes_decode_packet( unsigned char* key, unsigned char *pdata, int plength);

extern unsigned char* aes_encode_packet( unsigned char* key, unsigned char *pdata, int plength);

#endif




