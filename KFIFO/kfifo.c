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
		 /*用一个临时变量去保存结果，这样可省去关中断的代码*/
		 uint32_t temp = 0;
	   temp = ring_buf->in - ring_buf->out;
	  /*******************************************************************************************************
			比较size与fifo中存放的字节数哪个多，取最小的那个
			(ring_buf->out & (ring_buf->size - 1)) 计算ring_buf->out到ring_buf->buffer的偏移量 范围0-ring_buf->size
			ring_buf->buffer最后一个字节 到 ring_buf->out的距离，此距离与要读取的size比较，看谁小
	    
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
	 (ring_buf->in & (ring_buf->size - 1)) 计算in到ring_buf->buffer的偏移量，不管in是否大于ring_buf->size
	 比如当ring_buf->size是8时， 假设in为6， size为3， 则ring_buf->in & (ring_buf->size - 1) = 6 & 0x07 = 6
	 则len = min(3, 2) = 2; 这样就把buffer的前两个byte 复制到ring_buf->buffer的最后两个字节，
	 再把buffer的最后一个字节复制到ring_buf->buffer 的第一个字节！ in变为9， 覆盖了ring_buf->buffer的每一个字节。
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
		 就算in out益处，数据也不会丢失，可用char in, char out, size=128测试
     if (ring_buf->in == ring_buf->out && (ring_buf->out > 5000*ring_buf->size)) {
			  //关闭中断   在中断中会修改in的值，所以必须关闭中断
//			 ring_buf->in = ring_buf->out = 0;
				DISABLE_INT();
			  if (ring_buf->in == ring_buf->out) {
					// 再次检查是否相等
			    //printf("i=%d o=%d, setZERO.\r\n", ring_buf->in, ring_buf->out);
					ring_buf->in = ring_buf->out = 0;
				}
				ENABLE_INT();
				//打开中断
		 }

*/



