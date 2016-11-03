#include "ff.h"
#include "md5.h"
#include <stdio.h>
#include <string.h>

/*
	d0:index = 01-> start,  d1~dn: fileName,size,md5
	d0:index = 02...ff, d1~dn: music data
	d0:index = 00->end of file, d1~dn: music data.
*/
//static  unsigned int k = 0;
static  FIL	 mfile;
static	int  fileSize;
static	char fileMd5[ 16 ];
static	char fileName[ 11 ];
extern  char amrPlayRequest;

int handle_amr_voice( char *data, unsigned short len )
{	
	unsigned char md5[16];
	unsigned int bw;
	static unsigned char index = 0;
	static int transfer = 0;
	printf("%s: \r\n", __func__);

	if( *data == 1 || *data == 0 )
	{
		index = *data;
	}
	else if( *data == ( index + 1 ) )
	{
		index++;
	}
	else
	{
		printf("%s: Error, pre(%02x)+1 != now(%02x)!\r\n", __func__, index, *data);
		return 0;
	}

	if( index == 1 && !transfer )
	{
		transfer = 1;
		memcpy( fileName, data + 1, 11 );
		fileSize = *( ( int * )( data + 12 ) );
		memcpy( fileMd5, data + 16, 16 );
		
		if( f_open( &mfile,(const TCHAR*) "0:/voice/1.amr", 
					FA_CREATE_ALWAYS | FA_WRITE ) == FR_OK )
		{
			printf("file 0:/voice/1.amr create sucess!\r\n");
			f_close(&mfile);
		}		
	}
	else if( index == 0 && transfer )
	{
		if( f_open( &mfile,(const TCHAR*) "0:/voice/1.amr", 
					FA_WRITE ) == FR_OK )
		{
			f_write( &mfile, data + 1, len - 1, &bw );		
			f_sync( &mfile );
			f_close( &mfile );
		}
		else
		{
			printf("%s: file no exzit error!\r\n", __func__);
			return -1;
		}
		caculate_file_md5( "0:/voice/1.amr", md5);
		if( memcmp( fileMd5, md5, 16 ) == 0 )
		{
			printf("%s: transfer file ok!\r\n", __func__);
			amrPlayRequest = 1;
		}
		else
		{
			printf("%s: transfer file fail!\r\n", __func__);
		}
		transfer = 0;
	}
	else if( transfer )
	{
		if( f_open( &mfile,(const TCHAR*) "0:/voice/1.amr", 
					FA_WRITE ) == FR_OK )
		{
			f_write( &mfile, data + 1, len - 1, &bw );
			f_sync( &mfile );
			f_close( &mfile );
		}
		else
		{
			printf("%s: file no exzit error!\r\n", __func__);
			return -1;
		}
	}
	else
	{
		printf("%s: error!\r\n", __func__);
		return -1;
	}
	
	return 0;
}

