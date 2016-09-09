#include "xmalloc.h"	    
#include "FreeRTOS.h"

extern void vTaskSuspendAll( void );
extern BaseType_t xTaskResumeAll( void );
//�ڴ��(32�ֽڶ���)
__align(32) u8 mem1base[MEM1_MAX_SIZE];													//�ڲ�SRAM�ڴ��
__align(32) u8 mem2base[MEM2_MAX_SIZE] __attribute__((at(0X68000000)));					//�ⲿSRAM�ڴ��
//�ڴ�����
u16 mem1mapbase[MEM1_ALLOC_TABLE_SIZE];													//�ڲ�SRAM�ڴ��MAP
u16 mem2mapbase[MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0X68000000+MEM2_MAX_SIZE)));	//�ⲿSRAM�ڴ��MAP
//�ڴ�������	   
const u32 memtblsize[SRAMBANK]={MEM1_ALLOC_TABLE_SIZE,MEM2_ALLOC_TABLE_SIZE};			//�ڴ���С
const u32 memblksize[SRAMBANK]={MEM1_BLOCK_SIZE,MEM2_BLOCK_SIZE};						//�ڴ�ֿ��С
const u32 memsize[SRAMBANK]={MEM1_MAX_SIZE,MEM2_MAX_SIZE};								//�ڴ��ܴ�С


//�ڴ���������
struct _m_mallco_dev mallco_dev=
{
	x_mem_init,				//�ڴ��ʼ��
	xmem_used_pecent,				//�ڴ�ʹ����
	mem1base, mem2base,			//�ڴ��
	mem1mapbase, mem2mapbase,	//�ڴ����״̬��
	0, 0,  		 				//�ڴ����δ����
};

//�����ڴ�
//*des:Ŀ�ĵ�ַ
//*src:Դ��ַ
//n:��Ҫ���Ƶ��ڴ泤��(�ֽ�Ϊ��λ)
void xmemcpy(void *des, void *src, u32 n)  
{  
    u8 *xdes=des;
	u8 *xsrc=src; 
    while(n--)*xdes++=*xsrc++;  
}  
//�����ڴ�
//*s:�ڴ��׵�ַ
//c :Ҫ���õ�ֵ
//count:��Ҫ���õ��ڴ��С(�ֽ�Ϊ��λ)
void xmemset(void *s, u8 c, u32 count)  
{  
    u8 *xs = s;  
    while(count--)*xs++=c;  
}	   
//�ڴ�����ʼ��  
//memx:�����ڴ��
void x_mem_init(u8 memx)  
{  
    xmemset(mallco_dev.memmap[memx], 0,memtblsize[memx]*2);//�ڴ�״̬����������  
	xmemset(mallco_dev.membase[memx], 0,memsize[memx]);	//�ڴ��������������  
	mallco_dev.memrdy[memx]=1;								//�ڴ�����ʼ��OK  
}  
//��ȡ�ڴ�ʹ����
//memx:�����ڴ��
//����ֵ:ʹ����(0~100)
u8 xmem_used_pecent(u8 memx)  
{  
    u32 used=0;  
    u32 i;  
    for (i=0; i < memtblsize[memx]; i++)  
    {  
        if (mallco_dev.memmap[memx][i]) used++; 
    } 
    return (used*100)/(memtblsize[memx]);  
}  

//�ڴ����(�ڲ�����)
//memx:�����ڴ��
//size:Ҫ������ڴ��С(�ֽ�)
//����ֵ:0XFFFFFFFF,�������;����,�ڴ�ƫ�Ƶ�ַ 
u32 x_mem_malloc(u8 memx, u32 size)  
{  
    signed long offset=0;  
    u32 nmemb;	//��Ҫ���ڴ����  
	u32 cmemb=0;//�������ڴ����
    u32 i;  
    if (!mallco_dev.memrdy[memx]) mallco_dev.init(memx);//δ��ʼ��,��ִ�г�ʼ�� 
    if (size==0) return 0XFFFFFFFF; //����Ҫ����
    nmemb = size/memblksize[memx];  	//��ȡ��Ҫ����������ڴ����
    if (size % memblksize[memx]) nmemb++;  
    for(offset = memtblsize[memx]-1; offset>=0; offset--)//���������ڴ������  
    {     
		if (!mallco_dev.memmap[memx][offset]) cmemb++;//�������ڴ��������
		else cmemb = 0;								//�����ڴ������
		if(cmemb == nmemb)							//�ҵ�������nmemb�����ڴ��
		{
            for(i=0;i<nmemb;i++)  					//��ע�ڴ��ǿ� 
            {  
                mallco_dev.memmap[memx][offset+i]=nmemb;  
            }  
            return (offset*memblksize[memx]);//����ƫ�Ƶ�ַ  
		}
    }  
    return 0XFFFFFFFF;//δ�ҵ����Ϸ����������ڴ��  
}  
//�ͷ��ڴ�(�ڲ�����) 
//memx:�����ڴ��
//offset:�ڴ��ַƫ��
//����ֵ:0,�ͷųɹ�;1,�ͷ�ʧ��;  
u8 x_mem_free(u8 memx,u32 offset)  
{  
    int i;  
    if(!mallco_dev.memrdy[memx])//δ��ʼ��,��ִ�г�ʼ��
	{
		mallco_dev.init(memx);    
        return 1;//δ��ʼ��  
    }  
    if(offset < memsize[memx])//ƫ�����ڴ����. 
    {  
        int index = offset/memblksize[memx];			//ƫ�������ڴ�����  
        int nmemb = mallco_dev.memmap[memx][index];	//�ڴ������
        for(i=0; i<nmemb; i++)  						//�ڴ������
        {  
            mallco_dev.memmap[memx][index+i]=0;  
        }  
        return 0;  
    } else {
		return 2;//ƫ�Ƴ�����.  
	}
}  
//�ͷ��ڴ�(�ⲿ����) 
//memx:�����ڴ��
//ptr:�ڴ��׵�ַ 

//�ж��в��ɵ���xfree

void xfree(u8 memx, void *ptr)  
{  
	u32 offset;   
	if (ptr == NULL) {
		return;//��ַΪ0.  
 	}
	vTaskSuspendAll();
	offset = (u32)ptr - (u32)mallco_dev.membase[memx];     
    x_mem_free(memx, offset);	//�ͷ��ڴ�    
    ( void )xTaskResumeAll();
}  
//�����ڴ�(�ⲿ����)
//memx:�����ڴ��
//size:�ڴ��С(�ֽ�)
//����ֵ:���䵽���ڴ��׵�ַ.

//�ж��в��ɵ���xmalloc, ���� �Ŀռ�Խ��
//���ĵ�ʱ��Խ��
void *xmalloc(u8 memx, u32 size)  
{  
    u32 offset;   
	vTaskSuspendAll();	//��ֹ�����л�
	offset = x_mem_malloc(memx, size);
	( void )xTaskResumeAll();
    if (offset == 0XFFFFFFFF) {	
		return NULL;  
    } else {
    	return (void*)((u32)mallco_dev.membase[memx]+offset); 
    }
}  
//���·����ڴ�(�ⲿ����)
//memx:�����ڴ��
//*ptr:���ڴ��׵�ַ
//size:Ҫ������ڴ��С(�ֽ�)
//����ֵ:�·��䵽���ڴ��׵�ַ.

//�ж��в��ɵ���xrealloc

void *xrealloc(u8 memx, void *ptr, u32 size)  
{  
    u32 offset;
	vTaskSuspendAll();
    offset = x_mem_malloc(memx, size);
	( void )xTaskResumeAll();
    if (offset == 0XFFFFFFFF) {
		return NULL;     
    } else {
    	vTaskSuspendAll();
	    xmemcpy((void*)((u32)mallco_dev.membase[memx]+offset), ptr, size);	//�������ڴ����ݵ����ڴ�   
        xfree(memx, ptr);  											  		//�ͷž��ڴ�
		( void )xTaskResumeAll();
        return (void*)((u32)mallco_dev.membase[memx]+offset);  				//�������ڴ��׵�ַ
    }  
}












