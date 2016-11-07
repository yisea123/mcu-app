#ifndef __XMALLOC_H
#define __XMALLOC_H
#include "stm32f4xx.h"
#include "FreeRTOSConfig.h"

#ifndef NULL
#define NULL 0
#endif

//���������ڴ��
#define SRAMIN	 0		//�ڲ��ڴ��
#define SRAMEX   1		//�ⲿ�ڴ��(��ӢSTM32�����岻֧���ⲿ�ڴ�) 

#define SRAMBANK 	2	//����֧�ֵ�SRAM����. ��Ӣ��ʵ����ֻ֧��1���ڴ�����,���ڲ��ڴ�.	


//mem1�ڴ�����趨.mem1��ȫ�����ڲ�SRAM����.
#define MEM1_BLOCK_SIZE			32  	  						//�ڴ���СΪ32�ֽ�
#define MEM1_MAX_SIZE			10*1024 			//�������ڴ� 40K
#define MEM1_ALLOC_TABLE_SIZE	MEM1_MAX_SIZE/MEM1_BLOCK_SIZE 	//�ڴ���С

//mem2�ڴ�����趨.mem2���ڴ�ش����ⲿSRAM����
#define MEM2_BLOCK_SIZE			32  	  						//�ڴ���СΪ32�ֽ�
#define MEM2_MAX_SIZE			1 *32  							//��Ϊ��Ӣ��û�������ڴ�,����������һ����Сֵ
#define MEM2_ALLOC_TABLE_SIZE	MEM2_MAX_SIZE/MEM2_BLOCK_SIZE 	//�ڴ���С 
		 
 
//�ڴ���������
struct _m_mallco_dev
{
	void (*init)(u8);					//��ʼ��
	u8 (*perused)(u8);		  	    	//�ڴ�ʹ����
	u8 	*membase[SRAMBANK];				//�ڴ�� ����SRAMBANK��������ڴ�
	u16 *memmap[SRAMBANK]; 				//�ڴ����״̬��
	u8  memrdy[SRAMBANK]; 				//�ڴ�����Ƿ����
};
extern struct _m_mallco_dev mallco_dev;	 //��mallco.c���涨��

void xmemset(void *s,u8 c,u32 count);	//�����ڴ�
void xmemcpy(void *des,void *src,u32 n);//�����ڴ�     
void x_mem_init(u8 memx);				//�ڴ�����ʼ������(��/�ڲ�����)
u32 x_mem_malloc(u8 memx,u32 size);	//�ڴ����(�ڲ�����)
u8 x_mem_free(u8 memx,u32 offset);		//�ڴ��ͷ�(�ڲ�����)
extern unsigned char xmem_used_pecent(unsigned char memx);				//����ڴ�ʹ����(��/�ڲ�����) 
////////////////////////////////////////////////////////////////////////////////
//�û����ú���
void xfree(u8 memx,void *ptr);  			//�ڴ��ͷ�(�ⲿ����)
void *xmalloc(u8 memx,u32 size);			//�ڴ����(�ⲿ����)
void *xrealloc(u8 memx,void *ptr,u32 size);//���·����ڴ�(�ⲿ����)
#endif













