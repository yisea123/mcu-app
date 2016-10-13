#include "fattester.h"	 
#include "sdio_sdcard.h"
#include "usmart.h"
#include "usart.h"
#include "exfuns.h"
#include "FreeRTOS.h"
#include "task.h"  
#include "ff.h"
#include "string.h"
    
//Ϊ����ע�Ṥ����	 
//path:����·��������"0:"��"1:"
//mt:0��������ע�ᣨ�Ժ�ע�ᣩ��1������ע��
//����ֵ:ִ�н��
u8 mf_mount(u8* path,u8 mt)
{		   
	return f_mount(fs[2], (const TCHAR*)path, mt); 
}
//��·���µ��ļ�
//path:·��+�ļ���
//mode:��ģʽ
//����ֵ:ִ�н��
u8 mf_open(u8*path, u8 mode)
{
	u8 res;	 
	res=f_open(file,(const TCHAR*)path,mode);//���ļ���
	return res;
} 
//�ر��ļ�
//����ֵ:ִ�н��
u8 mf_close(void)
{
	f_close(file);
	return 0;
}
//��������
//len:�����ĳ���
//����ֵ:ִ�н��
u8 mf_read(u16 len)
{
	u16 i,t;
	u8 res=0;
	u16 tlen=0;
	printf("\r\nRead file data is:\r\n");
	for(i=0;i<len/512;i++)
	{
		res=f_read(file,fatbuf,512,&br);
		if(res)
		{
			printf("Read Error:%d\r\n",res);
			break;
		}else
		{
			tlen+=br;
			for(t=0;t<br;t++)printf("%c",fatbuf[t]); 
		}
	}
	if(len%512)
	{
		res=f_read(file,fatbuf,len%512,&br);
		if(res)	//�����ݳ�����
		{
			printf("\r\nRead Error:%d\r\n",res);   
		}else
		{
			tlen+=br;
			for(t=0;t<br;t++)printf("%c",fatbuf[t]); 
		}	 
	}
	if(tlen)printf("\r\nReaded data len:%d\r\n",tlen);//���������ݳ���
	printf("Read data over\r\n");	 
	return res;
}
//д������
//dat:���ݻ�����
//len:д�볤��
//����ֵ:ִ�н��
u8 mf_write(u8*dat,u16 len)
{			    
	u8 res;	   					   

	printf("\r\nBegin Write file...\r\n");
	printf("Write data len:%d\r\n",len);	 
	res=f_write(file,dat,len,&bw);
	if(res)
	{
		printf("Write Error:%d\r\n",res);   
	}else printf("Writed data len:%d\r\n",bw);
	printf("Write data over.\r\n");
	return res;
}

//��Ŀ¼
 //path:·��
//����ֵ:ִ�н��
u8 mf_opendir(u8* path)
{
	return f_opendir(&dir,(const TCHAR*)path);	
}
//�ر�Ŀ¼ 
//����ֵ:ִ�н��
u8 mf_closedir(void)
{
	return f_closedir(&dir);	
}

u8 mf_chdir(u8* path)
{
	return f_chdir((const TCHAR*)path);
}

u8 mf_chdrive(u8* path)
{
	return f_chdrive((const TCHAR*)path);
}

char pDir[128];
u8 mf_getcwd( void )
{	
	f_getcwd((TCHAR*)pDir, sizeof(pDir));
	//printf("\r\n%s\r\n", pDir);
	return 0;
}

u8 mf_getcwd_( char*buff, int len )
{
	f_getcwd((TCHAR*)buff, len);
	//printf("\r\n%s\r\n", buff);
	return 0;
}
//mf_stat("1:/hello.txt")
//mf_scan_files("1:")
//mf_chdrive("1:")
//mf_getcwd()
//mf_chdir("1:/first")
//mf_mkdir()
//mf_showfree("1:")

u8 mf_stat( u8* path )
{
	FILINFO fno;
	u8 ret;
	UBaseType_t pre;
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 1 );	
	memset(&fno, '\0', sizeof(fno));
	ret  = f_stat ((const TCHAR*)path, &fno);
	vTaskPrioritySet( NULL, pre );
	
	printf("\r\n");
	if( ret == FR_OK )
	{
		printf("File Name is:%s\r\n",fno.fname);
		printf("File Size is:%d\r\n",fno.fsize);
		printf("File data is:%d\r\n",fno.fdate);
		printf("File time is:%d\r\n",fno.ftime);
		printf("File Attr is:%d\r\n",fno.fattrib);
		
	#if _USE_LFN	
		if( fno.lfname != NULL)
		{
			printf("File lfname is:%s\r\n",fno.lfname);
			printf("File lfsize is:%d\r\n",fno.lfsize);	
		}
	#endif	
		printf("\r\n");	
	}
	else
	{
		printf("%s: error!\r\n", __func__);
	}
	return ret;
}

u8 mf_stat_( u8* path , FILINFO *fno )
{
	//FILINFO fno;
	u8 ret;
	UBaseType_t pre;
	pre = uxTaskPriorityGet( NULL );
	
	vTaskPrioritySet( NULL, configMAX_PRIORITIES - 1 );	
	memset(fno, '\0', sizeof(FILINFO));
	ret  = f_stat ((const TCHAR*)path, fno);
	vTaskPrioritySet( NULL, pre );
	
	printf("\r\n");
	if( ret == FR_OK )
	{
		//printf("File Name is:%s\r\n",fno->fname);
		//printf("File Size is:%d\r\n",fno->fsize);
		//printf("File data is:%d\r\n",fno->fdate);
		//printf("File time is:%d\r\n",fno->ftime);
		//printf("File Attr is:%d\r\n",fno->fattrib);
		
	#if _USE_LFN	
		if( fno->lfname != NULL)
		{
			//printf("File lfname is:%s\r\n",fno->lfname);
			//printf("File lfsize is:%d\r\n",fno->lfsize);	
		}
	#endif	
		//printf("\r\n");	
	}
	else
	{
		printf("%s: error!\r\n", __func__);
	}
	return ret;
}


//���ȡ�ļ���
//����ֵ:ִ�н��
u8 mf_readdir(void)
{
	u8 res;
	char *fn;			 
#if _USE_LFN
 	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = pvPortMalloc( fileinfo.lfsize );
#endif		  
	res=f_readdir(&dir,&fileinfo);//��ȡһ���ļ�����Ϣ
	if(res!=FR_OK||fileinfo.fname[0]==0)
	{
#if _USE_LFN		
		vPortFree( fileinfo.lfname );
#endif		
		return res;//������.
	}
#if _USE_LFN
	fn=*fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else
	fn=fileinfo.fname;;
#endif	
	printf("\r\n DIR info:\r\n");

	printf("dir.id:%d\r\n",dir.id);
	printf("dir.index:%d\r\n",dir.index);
	printf("dir.sclust:%d\r\n",dir.sclust);
	printf("dir.clust:%d\r\n",dir.clust);
	printf("dir.sect:%d\r\n",dir.sect);	  

	printf("\r\n");
	printf("File Name is:%s\r\n",fn);
	printf("File Size is:%d\r\n",fileinfo.fsize);
	printf("File data is:%d\r\n",fileinfo.fdate);
	printf("File time is:%d\r\n",fileinfo.ftime);
	printf("File Attr is:%d\r\n",fileinfo.fattrib);
	printf("\r\n");
#if _USE_LFN		
	vPortFree(fileinfo.lfname);
#endif	
	return 0;
}			 

 //�����ļ�
 //path:·��
 //����ֵ:ִ�н��
u8 mf_scan_files(u8 * path)
{
#if !_USE_LFN 
	int i = 0;
#endif
	DIR dir;
	FILINFO fileinfo;
	FRESULT res;	  
    char *fn;   /* This function is assuming non-Unicode cfg. */
#if _USE_LFN
 	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = pvPortMalloc( fileinfo.lfsize );
#endif		  

    res = f_opendir(&dir,(const TCHAR*)path); //��һ��Ŀ¼
    if (res == FR_OK) 
	{	
		printf("\r\n"); 
		while(1)
		{
	        res = f_readdir(&dir, &fileinfo);                   //��ȡĿ¼�µ�һ���ļ�
	        if (res != FR_OK || fileinfo.fname[0] == 0) break;  //������/��ĩβ��,�˳�
	        //if (fileinfo.fname[0] == '.') continue;             //�����ϼ�Ŀ¼
#if _USE_LFN
        	fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else							   
        	fn = fileinfo.fname;
#endif	                                              /* It is a file. */
#if !_USE_LFN 
			i = 0;
			while( path[i] != '\0' )
			{
				if( path[i] >= 0x41 && path[i] <= 0x5a )
					path[i] += 0x20;
				i++;
				if( i >= 200 )
				{
					printf("%s: error!!\r\n", __func__);
					return 3;
				}
			}
			i = 0;
			while( fn[i] != '\0' )
			{
				if( fn[i] >= 0x41 && fn[i] <= 0x5a )
					fn[i] += 0x20;
				i++;
				if( i >= 13 )
				{
					printf("%s: error!!\r\n", __func__);
					return 3;
				}
			}
#endif
			if( dir.sclust == 0 )
			{
				printf("%s", path);
			}
			else
			{
				printf("%s/", path);//��ӡ·��	
			}
			printf("%s\r\n",  fn);//��ӡ�ļ���	  
		} 
    }	  
#if _USE_LFN		
		vPortFree( fileinfo.lfname );
#endif		
    return res;	  
}

static char *prvWritePathToBuffer( char *pcBuffer, const char *pcPath , const char *pcFn )
{
	int x;
	if( strlen( pcPath ) + strlen( pcFn ) < 30 )
		strcpy( pcBuffer, pcPath );
	strcpy( pcBuffer + strlen( pcBuffer ), pcFn );
	for( x = strlen( pcBuffer ); x < ( int ) ( 30 ); x++ )
	{
		pcBuffer[ x ] = ' ';
	}
	pcBuffer[ x ] = 0x00;
	return &( pcBuffer[ x ] );
}

u8 mf_scan_files_( char * path, char * buffer, int len )
{
	int i = 0;
	DIR dir;
	FILINFO fileinfo;
	FRESULT res;	  
    char *fn = NULL, *pbuf = NULL;
	
#if _USE_LFN 
 	fileinfo.lfsize = _MAX_LFN * 2 + 1;
	fileinfo.lfname = pvPortMalloc( fileinfo.lfsize );
#endif		  

    res = f_opendir( &dir,(const TCHAR*) path );
    if ( res == FR_OK ) 
	{	
		pbuf = buffer;
		printf("\r\n"); 
		while( 1 )
		{
	        res = f_readdir( &dir, &fileinfo ); 
	        if ( res != FR_OK || fileinfo.fname[0] == 0 ) break; 
#if _USE_LFN
        	fn = *fileinfo.lfname ? fileinfo.lfname : fileinfo.fname;
#else							   
        	fn = fileinfo.fname;
#endif
			memset( buffer, '\0', len );
			if( dir.sclust != 0 )
			{
				path[ strlen( path ) ] = '/';
				path[ strlen( path ) ] = '\0';
				buffer = prvWritePathToBuffer( buffer, path, fn );
				/*we must resume the path string*/
				path[ strlen( path ) - 1 ] = '\0';				
			}
			else
			{
				buffer = prvWritePathToBuffer( buffer, path, fn );
			}
			i = 0;
			while( pbuf[i] != '\0' )
			{
				if( pbuf[i] >= 0x41 && pbuf[i] <= 0x5a )
					pbuf[i] += 0x20;
				i++;
				if( i >= len )
				{
					printf("%s: error!!\r\n", __func__);
					return 3;
				}
			}
			
			sprintf( buffer, "\t%8d bytes\t %s\t "
					"%02d-%02d-%02d %02d:%02d:%02d\r\n",
					fileinfo.fsize, 
					( ( fileinfo.fattrib >> 4 ) & 0x01 ) == 1 ? 
					"DIR" : ( ( fileinfo.fattrib >> 5 & 0x01 ) == 1 ? "FIL" : "unknow" ), 
					( fileinfo.fdate >> 9 ) + 1980,
					( fileinfo.fdate & 0x1ff ) >> 5, 
					fileinfo.fdate & 0x1f,
					fileinfo.ftime >> 11,
					( fileinfo.ftime & 0x7ff ) >> 5,
					( fileinfo.ftime & 0x1f )*2 );
			printf("%s", pbuf);
			buffer = pbuf;
			
		} 
    }	  
	
#if _USE_LFN		
		vPortFree( fileinfo.lfname );
#endif		

    return res;	  
} 


//��ʾʣ������
//drv:�̷�
//����ֵ:ʣ������(�ֽ�)
u32 mf_showfree(u8 *drv)
{
	FATFS *fs1;
	u8 res;
    u32 fre_clust=0, fre_sect=0, tot_sect=0;
    //�õ�������Ϣ�����д�����
    res = f_getfree((const TCHAR*)drv,(DWORD*)&fre_clust, &fs1);
    if(res==0)
	{											   
	    tot_sect = (fs1->n_fatent - 2) * fs1->csize;//�õ���������
	    fre_sect = fre_clust * fs1->csize;			//�õ�����������	   
#if _MAX_SS!=512
		tot_sect*=fs1->ssize/512;
		fre_sect*=fs1->ssize/512;
#endif	  
		if(tot_sect<20480)//������С��10M
		{
		    /* Print free space in unit of KB (assuming 512 bytes/sector) */
		    printf("����������:%d KB\r\n"
		           "���ÿռ�:%d KB\r\n",
		           tot_sect>>1,fre_sect>>1);
		}else
		{
		    /* Print free space in unit of KB (assuming 512 bytes/sector) */
		    printf("����������:%d MB\r\n"
		           "���ÿռ�:%d MB\r\n",
		           tot_sect>>11,fre_sect>>11);
		}
	}
	return fre_sect;
}		    
//�ļ���дָ��ƫ��
//offset:����׵�ַ��ƫ����
//����ֵ:ִ�н��.
u8 mf_lseek(u32 offset)
{
	return f_lseek(file,offset);
}
//��ȡ�ļ���ǰ��дָ���λ��.
//����ֵ:λ��
u32 mf_tell(void)
{
	return f_tell(file);
}
//��ȡ�ļ���С
//����ֵ:�ļ���С
u32 mf_size(void)
{
	return f_size(file);
} 
//����Ŀ¼
//pname:Ŀ¼·��+����
//����ֵ:ִ�н��
u8 mf_mkdir(u8*pname)
{
	return f_mkdir((const TCHAR *)pname);
}
//��ʽ��
//path:����·��������"0:"��"1:"
//mode:ģʽ
//au:�ش�С
//����ֵ:ִ�н��
u8 mf_fmkfs(u8* path,u8 mode,u16 au)
{
	return f_mkfs((const TCHAR*)path,mode,au);//��ʽ��,drv:�̷�;mode:ģʽ;au:�ش�С
} 
//ɾ���ļ�/Ŀ¼
//pname:�ļ�/Ŀ¼·��+����
//����ֵ:ִ�н��
u8 mf_unlink(u8 *pname)
{
	return  f_unlink((const TCHAR *)pname);
}

//�޸��ļ�/Ŀ¼����(���Ŀ¼��ͬ,�������ƶ��ļ�Ŷ!)
//oldname:֮ǰ������
//newname:������
//����ֵ:ִ�н��
u8 mf_rename(u8 *oldname,u8* newname)
{
	return  f_rename((const TCHAR *)oldname,(const TCHAR *)newname);
}
//��ȡ�̷����������֣�
//path:����·��������"0:"��"1:"  
void mf_getlabel(u8 *path)
{
	u8 buf[20];
	u32 sn=0;
	u8 res;
	res=f_getlabel ((const TCHAR *)path,(TCHAR *)buf,(DWORD*)&sn);
	if(res==FR_OK)
	{
		printf("\r\n����%s ���̷�Ϊ:%s\r\n",path,buf);
		printf("����%s �����к�:%X\r\n\r\n",path,sn); 
	}else printf("\r\n��ȡʧ�ܣ�������:%X\r\n",res);
}
//�����̷����������֣����11���ַ�������֧�����ֺʹ�д��ĸ����Լ����ֵ�
//path:���̺�+���֣�����"0:ALIENTEK"��"1:OPENEDV"  
void mf_setlabel(u8 *path)
{
	u8 res;
	res=f_setlabel ((const TCHAR *)path);
	if(res==FR_OK)
	{
		printf("\r\n�����̷����óɹ�:%s\r\n",path);
	}else printf("\r\n�����̷�����ʧ�ܣ�������:%X\r\n",res);
} 

//���ļ������ȡһ���ַ���
//size:Ҫ��ȡ�ĳ���
void mf_gets(u16 size)
{
 	TCHAR* rbuf;
	rbuf=f_gets((TCHAR*)fatbuf,size,file);
	if(*rbuf==0)return  ;//û�����ݶ���
	else
	{
		printf("\r\nThe String Readed Is:%s\r\n",rbuf);  	  
	}			    	
}
//��Ҫ_USE_STRFUNC>=1
//дһ���ַ����ļ�
//c:Ҫд����ַ�
//����ֵ:ִ�н��
u8 mf_putc(u8 c)
{
	return f_putc((TCHAR)c,file);
}
//д�ַ������ļ�
//c:Ҫд����ַ���
//����ֵ:д����ַ�������
u8 mf_puts(u8*c)
{
	return f_puts((TCHAR*)c,file);
}













