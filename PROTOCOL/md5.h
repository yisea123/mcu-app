#ifndef __MD5_H
#define __MD5_H	 
#include "sys.h"

#define LEN_MD5					(16)
extern int check_md5(u32 src, unsigned int binSize, unsigned char* md5);
extern void cal_md5(uint8_t *data, int len, char* md5);
#endif
