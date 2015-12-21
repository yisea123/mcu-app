#ifndef __RSA_H
#define __RSA_H	 

#include "sys.h"

#define RSA_N									31301
#define RSA_D									71

extern unsigned char* decode_packet(unsigned char *packet_data, int packet_length);
#endif
