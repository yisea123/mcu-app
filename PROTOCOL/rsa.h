#ifndef __RSA_H
#define __RSA_H	 

#include "sys.h"
/* RSA_N RSADΪ������Կ������ܹ̼�����Կ��ԣ����ɶ��⹫��*/
#define RSA_N					65369				//31301
#define RSA_D					151					//71

extern unsigned char* decode_packet(unsigned char *packet_data, int packet_length);
#endif
