#ifndef __RSA_H
#define __RSA_H	 

#include "sys.h"
/* RSA_N RSAD为解密密钥，与加密固件的密钥配对，不可对外公布*/
#define RSA_N					65369				//31301
#define RSA_D					151					//71

extern unsigned char* decode_packet(unsigned char *packet_data, int packet_length);
#endif
