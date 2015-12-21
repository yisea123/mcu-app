#ifndef __MD5_H
#define __MD5_H	 
#include "sys.h"

#define LEN_MD5					(16)
extern int check_md5(u32 src, int binSize, unsigned char* md5);
#endif
