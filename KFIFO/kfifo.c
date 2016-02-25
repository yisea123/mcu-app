#include "kfifo.h"

void* memcpy(void *des, const void *src, unsigned int n)  
{  
  u8 *xdes=des;
	u8 *xsrc=(u8 *)src; 
  while(n--)*xdes++=*xsrc++;  
	
	return des;
}  

struct kfifo* kfifo_init(void *buffer, uint32_t size, void *f_lock)
 {
 //    assert(buffer);
     struct kfifo *ring_buf = (void*)(0);
     if (!is_power_of_2(size))
     {
				 printf("size must be power of 2.\n");
         return ring_buf;
     }
     ring_buf = (struct kfifo *)mymalloc(0, sizeof(struct kfifo));
     if (!ring_buf)
     {
         printf("Failed to malloc struct kfifo\r\n");
         return ring_buf;
     }
 //    memset(ring_buf, 0, sizeof(struct kfifo));
     ring_buf->buffer = buffer;
     ring_buf->size = size;
     ring_buf->in = 0;
     ring_buf->out = 0;
		 ring_buf->lostBytes = 0;
		 ring_buf->acceptCmds = 0;
		 ring_buf->errorCmds = 0;
//     ring_buf->f_lock = f_lock;
     return ring_buf;
 }
//SIZE = 1024  512  256 
 struct kfifo *kfifo_alloc(unsigned int size, void *lock)
 {
     void   *buffer;
     struct kfifo *ret;
     if (!is_power_of_2(size)) {
			printf("kfifo alloc fail. is power of 2 fail!\r\n");
     }
     buffer = (void *)mymalloc(0, size);
     if (!buffer) {
				 printf("kfifo alloc fail. buffer = NULL\r\n");
         return (void*)(0);
     	}
     ret = kfifo_init(buffer, size, lock);
 
     if (ret == (void*)(0))
         myfree(0, buffer);
     return ret;
 }

 void kfifo_free(struct kfifo *ring_buf)
 {
     if (ring_buf)
     {
			 if (ring_buf->buffer)
			 {
					myfree(0, ring_buf->buffer);
			 }
      myfree(0, ring_buf);
     }
 }

 uint32_t __kfifo_len(const struct kfifo *ring_buf)
 {
     return (ring_buf->in - ring_buf->out);
 }
 
 uint32_t __kfifo_get(struct kfifo *ring_buf, void * buffer, uint32_t size)
 {
	   uint32_t len = 0;
		 /*��һ����ʱ����ȥ��������������ʡȥ���жϵĴ���*/
		 uint32_t temp = 0;
	   temp = ring_buf->in - ring_buf->out;
	  /*******************************************************************************************************
			�Ƚ�size��fifo�д�ŵ��ֽ����ĸ��࣬ȡ��С���Ǹ�
			(ring_buf->out & (ring_buf->size - 1)) ����ring_buf->out��ring_buf->buffer��ƫ���� ��Χ0-ring_buf->size
			ring_buf->buffer���һ���ֽ� �� ring_buf->out�ľ��룬�˾�����Ҫ��ȡ��size�Ƚϣ���˭С
	    
		*********************************************************************************************************/
		// DISABLE_INT(); //fix bug....
     size  = min(size, temp);  
		// ENABLE_INT();	 
	 
     /* first get the data from fifo->out until the end of the buffer */
     len = min(size, ring_buf->size - (ring_buf->out & (ring_buf->size - 1)));
     memcpy(buffer, ((char*)ring_buf->buffer + (ring_buf->out & (ring_buf->size - 1))), len);
     /* then get the rest (if any) from the beginning of the buffer */
     memcpy((char*)buffer + len, (char*)ring_buf->buffer, size - len);
     ring_buf->out += size;
     return size;
 }

 uint32_t __kfifo_put(struct kfifo *ring_buf, void *buffer, uint32_t size)
 {
	   uint32_t len = 0;
		 //assert(ring_buf || buffer);
     size = min(size, ring_buf->size - ring_buf->in + ring_buf->out);
     /* first put the data starting from fifo->in to buffer end */
     len  = min(size, ring_buf->size - (ring_buf->in & (ring_buf->size - 1)));
  /***********************************************************************************************************
	 (ring_buf->in & (ring_buf->size - 1)) ����in��ring_buf->buffer��ƫ����������in�Ƿ����ring_buf->size
	 ���統ring_buf->size��8ʱ�� ����inΪ6�� sizeΪ3�� ��ring_buf->in & (ring_buf->size - 1) = 6 & 0x07 = 6
	 ��len = min(3, 2) = 2; �����Ͱ�buffer��ǰ����byte ���Ƶ�ring_buf->buffer����������ֽڣ�
	 �ٰ�buffer�����һ���ֽڸ��Ƶ�ring_buf->buffer �ĵ�һ���ֽڣ� in��Ϊ9�� ������ring_buf->buffer��ÿһ���ֽڡ�
	************************************************************************************************************/
     memcpy((char*)ring_buf->buffer + (ring_buf->in & (ring_buf->size - 1)), buffer, len);
     /* then put the rest (if any) at the beginning of the buffer */
     memcpy(ring_buf->buffer, (char*)buffer + len, size - len);
     ring_buf->in += size;
     return size;
 }
 
 uint32_t kfifo_len(const struct kfifo *ring_buf)
 {
     uint32_t len = 0;
 //    pthread_mutex_lock(ring_buf->f_lock);
     len = __kfifo_len(ring_buf);
 //    pthread_mutex_unlock(ring_buf->f_lock);
     return len;
 }
 
 uint32_t kfifo_get(struct kfifo *ring_buf, void *buffer, uint32_t size)
 {
     uint32_t ret;
 //    pthread_mutex_lock(ring_buf->f_lock);
     ret = __kfifo_get(ring_buf, buffer, size);
		 //if(ret > size) printf("%s->  ret=%d, size=%d, %d\r\n", __func__, ret, size, ring_buf->size);
//    pthread_mutex_unlock(ring_buf->f_lock);
     return ret;
 }
 
 void reset_fifo(struct kfifo *ring_buf)
 {
		if (ring_buf->in == ring_buf->out && (ring_buf->out > 2*ring_buf->size)) {
			ring_buf->in = ring_buf->out = 0;	
		}
 }
 
 uint32_t kfifo_put(struct kfifo *ring_buf, void *buffer, uint32_t size)
 {
     uint32_t ret;
 //    pthread_mutex_lock(ring_buf->f_lock);
     ret = __kfifo_put(ring_buf, buffer, size);
 //    pthread_mutex_unlock(ring_buf->f_lock);
		 //if(ret != size) printf("%s->  ret=%d, size=%d, %d\r\n", __func__, ret, size, ring_buf->size);
		 if(ret != size) ring_buf->mLost = 1;
	 
     return ret;
 }

void kfifo_clean(struct kfifo *ring_buf)
{
//	pthread_mutex_lock(ring_buf->f_lock);
	ring_buf->in = 0;
	ring_buf->out = 0;
//	pthread_mutex_unlock(ring_buf->f_lock);
}

void printf_kfifo_info(struct kfifo *ring_buf)
{
     uint32_t len = 0;
		 printf("********************");
//     pthread_mutex_lock(ring_buf->f_lock);
 	   printf("kififo in=%d, out=%d", ring_buf->in, ring_buf->out);	 
     len = (ring_buf->in - ring_buf->out);	 
//     pthread_mutex_unlock(ring_buf->f_lock);
		 printf("kififo data len = %d", len);
	   printf("********************");	 
}

/**
		 ����in out�洦������Ҳ���ᶪʧ������char in, char out, size=128����
     if (ring_buf->in == ring_buf->out && (ring_buf->out > 5000*ring_buf->size)) {
			  //�ر��ж�   ���ж��л��޸�in��ֵ�����Ա���ر��ж�
//			 ring_buf->in = ring_buf->out = 0;
				DISABLE_INT();
			  if (ring_buf->in == ring_buf->out) {
					// �ٴμ���Ƿ����
			    //printf("i=%d o=%d, setZERO.\r\n", ring_buf->in, ring_buf->out);
					ring_buf->in = ring_buf->out = 0;
				}
				ENABLE_INT();
				//���ж�
		 }

*/



