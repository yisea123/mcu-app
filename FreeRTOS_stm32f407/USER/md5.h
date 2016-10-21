#ifndef __MD5_H
#define __MD5_H	 

#define LEN_MD5					(16)

extern int compare_flashdata_md5( unsigned int addrStart, unsigned int size,
							const unsigned char* md5 );
extern void caculate_buffer_md5( unsigned char *buffer, int len, char* md5 );
extern int caculate_file_md5( const char *fileName, unsigned char md5[] );
#endif
