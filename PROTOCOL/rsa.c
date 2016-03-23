#include "rsa.h"
#include "iwdg.h"

static unsigned int rsa_n=RSA_N, rsa_d=RSA_D;
/*此两个密钥不可删除！*/

unsigned char buf_decode[128] = { 0 };

unsigned char rsa_decode(unsigned short data, unsigned int d, unsigned int n)
{
 	unsigned long long value=1;
	d=d+1;
	
	while(d!=1)
	{
		value=value*data;
		value=value%n;
		d--;
	}
	
	return (unsigned char)value;
}


unsigned char* decode_packet(unsigned char *packet_data, int packet_length)
{
	int t;
	
	for(t=0; t<(packet_length/2); t++) 
	{
		buf_decode[t] = rsa_decode(*((unsigned short*)packet_data), rsa_d, rsa_n);
		packet_data = packet_data + 2;
		IWDG_Feed();
	}
	
	return buf_decode;
}







