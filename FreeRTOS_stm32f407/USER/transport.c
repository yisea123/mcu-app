#include "transport.h"
#include "usart.h"
#include <string.h>
#include "uartprotocol.h"
#include "rfifo.h"
#include "stdio.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "amrfile.h"
#include "ff.h"
#include "sys.h"
#include "ymodem.h"

#define CMD_BURN_C_ID 				(0xfffe)
#define CMD_BURN_N_ID 				(0xfffd)
#define CMD_BURN_DONE_ID 			(0xfffc)
#define CMD_ERR_PACKET_INDEX        (0xfffb)
#define CMD_ERR_SIZE_EXT            (0xfffa)
#define CMD_ERR_FLASH_RW            (0xfff9)
#define CMD_ERR_TRANSMISS_START     (0xfff8)
#define CMD_BURN_ACK_ID 			(0xfff7)
#define CMD_ERR_UPDATE_REQUEST      (0xfff6)
#define CMD_ERR_PACKET_SIZES        (0xfff5)

#define TRANSPORT_CONTINUE 			1
#define TRANSPORT_FINISH			2
#define TRANSPORT_ERROR				3

#define TRANSPORT_LAST_PACKET 		0

typedef struct pmodeState
{
		char			mBurnBinStart;
		unsigned int	packetNum;
		unsigned int	packetTotal;
		unsigned int	tagetNum;
		char			packetSize;
		unsigned int	binSize;
		unsigned char	deviceNum;
		unsigned char 	aesKey[32];	
		char 			filename[20];
		unsigned int 	totalsize;
}PmodemState;	

static PmodemState state;
Ringfifo tspfifo;
xSemaphoreHandle xTspSemaphore = NULL ;
QueueHandle_t xTransportQueue =  NULL;
QueueHandle_t xTranTaskQueue =  NULL;

void init_status(void)
{
	printf("%s\r\n", __func__);
	//mPos = 0;	
	//binSize = 0;
	//fileSize = 0;

	state.mBurnBinStart = 0;
	state.packetNum = 0;
	state.packetTotal = 0;
	state.tagetNum = 0;
	state.packetSize = 0;
	state.binSize = 0;
	state.deviceNum = 0;	
}

void int2str(unsigned char* str, unsigned int intnum)
{
  unsigned int i;
  unsigned int j = 0;  
  unsigned int Status = 0;
  unsigned int Div = 1000000000;

  for (i = 0; i < 10; i++) {
    str[j++] = (intnum / Div) + 48;
    intnum = intnum % Div;
    Div /= 10;
    if ((str[j-1] == '0') & (Status == 0)) {
      j = 0;
    } else {
      Status++;
    }
  }
}

unsigned int get_file_size()
{
	unsigned int size;

	//size = file.size + md5;
	//state.binSize = file.size;
	return size;
}

void send_update_packet(unsigned char devNum) 
{
	int 	len;
	char 	data[2];
	char 	command[ 2 + N_LEN + N_MSGID + N_TYPE + 2 + N_CRC ];

	data[0] = UPDATE;
	data[1] = devNum;	
	printf("%s: update rom request, devNum = %d", __func__, devNum);
	
	len = compose_protocol_command( 2, MSG_ANDOIRD_OTA, data, command );
	add_command_to_stream( command,  len, WAIT_BLOCK );	
	state.deviceNum = devNum;		
}

void send_info_packet( const unsigned char* fileName, unsigned int binSize,
					unsigned char *key ) 
{   
	unsigned short i, j;
	unsigned char file_ptr[10];
	
	char data[166];
	//aa bb 130    0b 00   01   00 0f d0~d127    crc1 crc2
	data[0] = 0xAA;
	data[1] = 0xBB;

	*((LEN_Type *)&data[2]) = state.packetSize + 2;
	*((MSGID_Type *)&data[2+N_LEN]) = MSG_ANDOIRD_OTA;  	
	data[2+N_LEN+N_MSGID] = TYPE_CMD;
	
	data[3+N_LEN+N_MSGID] = 0x00;
	data[4+N_LEN+N_MSGID] = 0xff;
	/* Filename packet has valid data */ //LED.bin 6553523 33333
	for ( i = 3+N_LEN+N_MSGID; (fileName[i - 3 -N_LEN-N_MSGID] != '\0') && 
			(i < FILE_NAME_LENGTH + 3+N_LEN+N_MSGID); i++ ) 
	{
	 	data[i + PACKET_HEADER] = fileName[i - 3-N_LEN-N_MSGID];
		//printf("(%d)", i + PACKET_HEADER);
	}
	data[i + PACKET_HEADER] = 0x00;
	//printf("(%d)", i + PACKET_HEADER);
	memset(file_ptr, '\0', sizeof(file_ptr));
	int2str(file_ptr, binSize);
	printf("file_name = [%s]", fileName);
	printf("file_size = [%s]", file_ptr);
	for ( j = 0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; ) {
		//printf("(%d)", i);
	 	data[i++] = file_ptr[j++];
	}
	//printf("(%d)", i);
	data[i++] = '\0';
	j = i;
	for ( i = 0; i < sizeof(state.aesKey)/sizeof(state.aesKey[0]); i++, j++ ) {
		data[j] = key[i];
		//printf("(%d)", j);
	}
	for ( ; j < state.packetSize + PACKET_HEADER + 3+N_LEN+N_MSGID; j++) {
		data[j] = 0;
		//printf("(%d)", j);
		//it is important to set 0!
	}

	*((MSGID_Type *)&data[2+N_LEN+N_MSGID+N_TYPE+ *((LEN_Type *)&data[2])]) = 
		calculate_crc((unsigned char *)data + 2, N_LEN + N_MSGID + N_TYPE + *((LEN_Type *)&data[2]));
	add_command_to_stream(data, *((LEN_Type *)&data[2]) + N_COMMAND_TOTAL, WAIT_BLOCK);	
}

void send_data_packet(unsigned char *SourceBuf,  
					unsigned char pktNo, unsigned int sizeBlk) 
{
   
	unsigned short i, size, packetSize;
	unsigned char* file_ptr;
	char data[166];

	//if(sizeBlk != 128) ALOGD("%s: sizeBlk = %d", __func__, sizeBlk);

	/* Make first three packet */
	packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : state.packetSize;
	size = (sizeBlk < packetSize ? sizeBlk :packetSize);
	if (packetSize == PACKET_1K_SIZE) 
	{
		;//NO SUPPORT....
	} 
	else 
	{
	  data[0] = 0xAA;
	  data[1] = 0xBB;
	  *((LEN_Type *)&data[2]) = state.packetSize + 2;
	  *((MSGID_Type *)&data[2+N_LEN]) = MSG_ANDOIRD_OTA;	  
	  data[2+N_LEN+N_MSGID] = TYPE_CMD;
	}
	data[3+N_LEN+N_MSGID] = pktNo;
	data[4+N_LEN+N_MSGID] = (~pktNo);	
	file_ptr = SourceBuf;
	/* Filename packet has valid data */
	for ( i = PACKET_HEADER + 3+N_LEN+N_MSGID; i < size + PACKET_HEADER + 3+N_LEN+N_MSGID; i++ ) {
	 	data[i] = *file_ptr++;
	}
	if ( size  <= packetSize) 
	{
		for (i = size + PACKET_HEADER + 3+N_LEN+N_MSGID; i < packetSize + 
				PACKET_HEADER + 3+N_LEN+N_MSGID; i++) 
		{
		  	data[i] = 0x00; 
		}
	}
	
	*((MSGID_Type *)&data[2+N_LEN+N_MSGID+N_TYPE+ *((LEN_Type *)&data[2])]) = 
		calculate_crc((unsigned char *)data + 2, N_LEN + N_MSGID + N_TYPE + *((LEN_Type *)&data[2]));
	add_command_to_stream(data, *((LEN_Type *)&data[2]) + N_COMMAND_TOTAL, WAIT_BLOCK);		
}

void send_ca_packet( void )
{
	char data[50];
	
	//AA BB 2  0x05 EOT CRC1 CRC2
	data[0] = 0xAA;
	data[1] = 0xBB;
	*((LEN_Type *)&data[2]) = 1;
	*((MSGID_Type *)&data[2+N_LEN]) = MSG_ANDOIRD_OTA;		
	data[2+N_LEN+N_MSGID] = TYPE_CMD;
	data[3+N_LEN+N_MSGID] = CA;

	*((MSGID_Type *)&data[2+N_LEN+N_MSGID+N_TYPE+ *((LEN_Type *)&data[2])]) = 
		calculate_crc((unsigned char *)data + 2, N_LEN + N_MSGID + N_TYPE + *((LEN_Type *)&data[2]));
	add_command_to_stream(data, *((LEN_Type *)&data[2]) + N_COMMAND_TOTAL, WAIT_BLOCK);	
}

void send_eot_packet( void )
{
	char data[50];

	//AA BB 2  0x05 EOT CRC1 CRC2
	data[0] = 0xAA;
	data[1] = 0xBB;
	*((LEN_Type *)&data[2]) = 1;
	*((MSGID_Type *)&data[2+N_LEN]) = MSG_ANDOIRD_OTA;		
	data[2+N_LEN+N_MSGID] = TYPE_CMD;
	data[3+N_LEN+N_MSGID] = EOT;

	*((MSGID_Type *)&data[2+N_LEN+N_MSGID+N_TYPE+ *((LEN_Type *)&data[2])]) = 
		calculate_crc((unsigned char *)data + 2, N_LEN + N_MSGID + N_TYPE + *((LEN_Type *)&data[2]));
	add_command_to_stream(data, *((LEN_Type *)&data[2]) + N_COMMAND_TOTAL, WAIT_BLOCK);	

}

void send_end_packet( void )
{
	int i;
	char data[166];

	//AA BB 2  0x05 EOT CRC1 CRC2
	data[0] = 0xAA;
	data[1] = 0xBB;
	*((LEN_Type *)&data[2]) = state.packetSize + 2;
	*((MSGID_Type *)&data[2+N_LEN]) = MSG_ANDOIRD_OTA;		
	data[2+N_LEN+N_MSGID] = TYPE_CMD;
	data[3+N_LEN+N_MSGID] = 0x00;
	data[4+N_LEN+N_MSGID] = 0xff;

	for (i = PACKET_HEADER + 3 + N_LEN + N_MSGID; i < state.packetSize + 
			PACKET_HEADER + 3+ N_LEN + N_MSGID; i++ ) 
	{
		data[i] = 0x00;
	}
	*((MSGID_Type *)&data[2+N_LEN+N_MSGID+N_TYPE+ *((LEN_Type *)&data[2])]) = 
		calculate_crc((unsigned char *)data + 2, N_LEN + N_MSGID + N_TYPE + *((LEN_Type *)&data[2]));
	add_command_to_stream(data, *((LEN_Type *)&data[2]) + N_COMMAND_TOTAL, WAIT_BLOCK);			
}

void compose_data_packet( int num, char isCA )
{
	PmodemState *mState = &state;
	static unsigned int mSaveSize = 0;
	static unsigned char mData[256+4];
	unsigned int size, ret = mState->packetSize;
	unsigned char pktNo=0;
	Ringfifo *mFifo = &tspfifo;
	
	if( isCA ) 
	{ 
		printf("%s: MSG_SEND_BIN_ERROR, send CA packet for Tmodem!", __func__);	
		send_ca_packet();
		return;
	}
	if(!mState->mBurnBinStart) 
	{ 
		printf("%s: mBurnBinStart = %d error!", __func__, mState->mBurnBinStart); 
		return;
	}
	if((num == 1) && (mState->packetTotal == 0))
	{
		/*while(romSize==0) usleep(1000);ALOGD("usleep out...");*/ 
		size = get_file_size();
		printf("%s: READ_mcu_bin  size=[%d]", __func__, size); 
		if(size >0 && (mState->packetSize != 0)) 
		{
			mState->packetTotal = (size/mState->packetSize);
			if (size%mState->packetSize != 0) 
				mState->packetTotal++;
			printf("%s: PacketTotal=[%d], packet_size=%d", __func__, mState->packetTotal, mState->packetSize);
			send_info_packet((const unsigned char*)"first.arm", mState->binSize, mState->aesKey);
			mSaveSize = mState->packetSize;
			/*add for fix bug.*/
		} 
		else
		{
			printf("%s: packet_size=0 or FilePath error.", __func__); 
		}
	}
	else if(num > 1 && num < (mState->packetTotal+2)) 
	{
		if(num != mState->tagetNum) 
		{
			ret = rfifo_get(mFifo, mData, mState->packetSize);
			if(ret != mState->packetSize) 
			{
				mSaveSize = ret;
				printf("%s:************************************************", __func__);
				printf("%s: packetTotal= %d, num=%d, ret=%d, packet_size=%d", 
					__func__, mState->packetTotal, num, ret, mState->packetSize);
			} 
			mState->tagetNum = num;
		} 
		pktNo = (unsigned char)( num - 1 ) % 256;
		//ALOGD("%s: pktNo = %d, ret=%d", __func__, pktNo, ret);
		if(num == (mState->packetTotal+1)) 
		{
			printf("%s: fifo len=%d, [if the len !=0, ***brun error*** happened]", 
				__func__, rfifo_len(mFifo));
			printf("%s: packetTotal = %d, num=%d, mSaveSize = %d", __func__, mState->packetTotal, num, mSaveSize);
			send_data_packet(mData,	pktNo, mSaveSize);
		} 
		else 
		{ 
			send_data_packet(mData, pktNo, ret);
		}
	}
	else if(num == (mState->packetTotal+2) || num == (mState->packetTotal+3)) 
	{
		send_eot_packet();
	}
	else if(num == (mState->packetTotal+4)) 
	{
		send_end_packet();
	}

	//if( rfifo_len(mFifo) < 129)
	//{
	//	readDataFromBin(srcFile);
	//}
}

//vSemaphoreCreateBinary( xTspSemaphore );	
//xSemaphoreTake( xTspSemaphore, 0 );
void TransportTask( void * pvParameters )
{
	int 	  result;
	int		  res;
	//int 	  ret;
	//short 	  len;
	//char 	  buff[300];
	Ringfifo* mfifo = &tspfifo;
	PmodemState *mState = &state;

	mState->packetSize = 128;
	rfifo_init( mfifo, 2048 );	
	vSetTaskLogLevel( NULL, eLogLevel_3 );	
	xTransportQueue = xQueueCreate( 5, sizeof(int));
	xTranTaskQueue = xQueueCreate( 5, sizeof(int));

	vTaskDelay( 5 / portTICK_RATE_MS );
	printf("%s: start...\r\n", __func__);
	
	while( 1 )
	{
		init_status();
		rfifo_clean( mfifo );
		//ulTaskNotifyTake( pdFALSE, portMAX_DELAY );
		if( xQueueReceive( xTranTaskQueue, &( res ), portMAX_DELAY ))
		{
			//wait for start singal
			printf("res=%d\r\n", res);
			//res = bin size;
		}		
		send_update_packet(1);
		if( xQueueReceive( xTransportQueue, &( result ), ( TickType_t ) 10000 / portTICK_RATE_MS ) )
		{
			if( result != TRANSPORT_CONTINUE )
			{
				printf("error happend!\r\n");
				continue;
			}
		}
		else
		{
			printf("time out!\r\n");
			continue;
		}
		while( mState->mBurnBinStart ) 
		{
			while( mState->mBurnBinStart )
			{
				if( rfifo_len( mfifo ) >= mState->packetSize  || res == TRANSPORT_LAST_PACKET )
				{
					break;
				}
				else
				{
					//xSemaphoreTake( xTspSemaphore,  6000 / portTICK_RATE_MS   );
					//ulTaskNotifyTake( pdFALSE, 6000 / portTICK_RATE_MS );
					if( xQueueReceive( xTranTaskQueue, &( res ), portMAX_DELAY ))
					{
						//wait for process or last packet singal
						printf("res=%d\r\n", res);
					}					
				}
			}

			compose_data_packet(mState->packetNum, 0);
			
			if( xQueueReceive( xTransportQueue, &( result ), ( TickType_t ) 10000 / portTICK_RATE_MS ) )
			{
				if( result == TRANSPORT_CONTINUE )
				{
					continue;
				}
				else if( result == TRANSPORT_CONTINUE )
				{
					printf("%s: finish...\r\n", __func__);
					break;
				}
				else if( result == TRANSPORT_CONTINUE )
				{
					compose_data_packet(mState->packetNum, 1);
					compose_data_packet(mState->packetNum, 1);			
					printf("%s: error happend...\r\n", __func__);					
					break;		
				}		
			}
			else
			{
				printf("time out!\r\n");			
				break;
			}
		}
	}
}

int decode_message(const char *c, const char *c_)//start at d0....
{
	int id = -1;
	PmodemState *mState = &state;
	//aa bb 02 0e C xx xx
	//ALOGD("%s:0e->*(cmd+4)=%x, C=%x", __func__, *(cmd+4), 'C');
	switch(*c) {
		
		case ACK:
			if (mState->mBurnBinStart) {
				if ((unsigned char)(mState->packetNum)%256 == *c_) {
					mState->packetNum++;
					//ALOGD("packetNum(%d) packetTotal(%d)", packetNum, packetTotal);
					if ((mState->packetNum != 1) && (mState->packetNum != 2) &&
						(mState->packetNum != (mState->packetTotal+3))
						&& (mState->packetNum != (mState->packetTotal+4)) && 
						!(mState->packetNum >= (mState->packetTotal+5))) {
						id = CMD_BURN_ACK_ID;
					}
				} else {
					// fix mutil ack problem
					printf("%s:error! (packetNum)%x , packets_received=%x\r\n"
						, __func__, (unsigned char)((mState->packetNum)%256), *c_ );

				}
			}

			break;
			
		case EOT_ACK:
			printf("%s:***EOT_ACK***\r\n", __func__);
			
		case LAST_ACK:
			if (mState->mBurnBinStart) {				
				 mState->packetNum++;
				 //ALOGD("packetNum(%d) packetTotal(%d)", packetNum, packetTotal);
				 if ((mState->packetNum != 1) && (mState->packetNum != 2) &&
					 (mState->packetNum != (mState->packetTotal+3))
					 && (mState->packetNum != (mState->packetTotal+4)) && 
					 !(mState->packetNum >= (mState->packetTotal+5))) {
					 id = CMD_BURN_ACK_ID;
				 }
			}
			break;
			
		case CRC16:
			if (mState->mBurnBinStart && mState->packetNum == 1 || mState->packetNum == 2 ||
				(mState->packetNum == (mState->packetTotal+4))) {
				printf("%s:***CMD_BURN_C_ID***\r\n", __func__);
				id = CMD_BURN_C_ID;
			} else {
				printf("%s:***no CMD_BURN_C_ID***\r\n", __func__);
			}
			//report service continue
			break;
			
		case NAK:
			if (mState->mBurnBinStart && mState->packetNum == (mState->packetTotal +3)) {
				id = CMD_BURN_N_ID;
				printf("%s:***CMD_BURN_N_ID***\r\n", __func__);
			} else {
				printf("%s:***NO CMD_BURN_N_ID***\r\n", __func__);
			}
			//report service continue
			break;

		case ERR_PACKET_INDEX:
			printf("%s:***ERR_PACKET_INDEX***\r\n", __func__);
			id = CMD_ERR_PACKET_INDEX;
			mState->deviceNum = 0;			
			break;
			
		case ERR_SIZE_EXT:
			printf("%s:***no ERR_SIZE_EXT***\r\n", __func__);
			id = CMD_ERR_SIZE_EXT;
			mState->deviceNum = 0;			
			break;
			
		case ERR_FLASH_RW:
			printf("%s:***ERR_FLASH_RW***\r\n", __func__);
			id = CMD_ERR_FLASH_RW;
			mState->deviceNum = 0;			
			break;
			
		case ERR_TRANSMISS_START:
			printf("%s:***ERR_TRANSMISS_START***\r\n", __func__);
			id = CMD_ERR_TRANSMISS_START;			
			mState->deviceNum = 0;
			break;
			
		case ERR_UPDATE_REQUEST:
			printf("%s:***ERR_UPDATE_REQUEST***\r\n", __func__);
			id = CMD_ERR_UPDATE_REQUEST;			
			mState->deviceNum = 0;
			break;
			
		case UPDATE_DONE:
			printf("%s:***UPDATE_DONE***\r\n", __func__);
			id = CMD_BURN_DONE_ID;			
			mState->deviceNum = 0;
			break;

		case UPDATE:
			mState->packetSize = *c_;
			printf("%s:***MCU UPDATE ACK***, PS=[%d]\r\n", __func__, mState->packetSize);
			if ((mState->packetSize != 0) && (mState->packetSize%16==0)) {			
				init_status();
				mState->packetSize = *c_; //get again!
				mState->mBurnBinStart = 1;
				printf("%s:packet_size=%d, mBurnBinStart=%d\r\n", __func__, 
					mState->packetSize, mState->mBurnBinStart);
			} else {
				printf("%s:***CMD_ERR_PACKET_SIZES***\r\n", __func__);
				id = CMD_ERR_PACKET_SIZES;
			}
			break;
		default:
			break;
	
	}

	return id;
}

void deliver_message(char *data, int result )
{
	int ulVar;
	int percent;
	static int mProgress = -1;
	PmodemState *mState = &state;

	switch (result) 
	{
		case CMD_BURN_ACK_ID:
		case CMD_BURN_C_ID:
		case CMD_BURN_N_ID:
			if (mState->packetTotal == 0) {
				percent = 0;
			} else {
				percent = (int)
				((mState->packetNum*1.0*100)/(mState->packetTotal+5*1.0));
			}
			if (mState->mBurnBinStart) {
				//printf("%s: xSemaphoreGive...\r\n", __func__);
				//xSemaphoreGive( xTspSemaphore );
				ulVar = TRANSPORT_CONTINUE;
				if( xQueueSend( xTransportQueue, ( void * ) &ulVar, ( TickType_t ) 100 ) != pdPASS )
				{
					// Failed to post the message, even after 10 ticks.
					printf("xQueueSend Fail!\r\n");
				}				
				//mJniObserver->OnEvent(rData->data, percent, RECEIVE_BURN_CONTINUE_INDEX, percent);
			}
			if (percent%10==0 && mProgress != percent) {
				mProgress = percent;
				printf("***burn continue, progress(%d%%)\r\n", percent);
			}
			break;

		case CMD_BURN_DONE_ID:
			if (mState->packetTotal == 0) {
				percent = 0;
			} else { 
				percent = (int)
				((mState->packetNum*1.0*100)/(mState->packetTotal+5*1.0));
			}
			if (mState->mBurnBinStart) {
				//xSemaphoreGive( xTspSemaphore );
				ulVar = TRANSPORT_FINISH;
				if( xQueueSend( xTransportQueue, ( void * ) &ulVar, ( TickType_t ) 100 ) != pdPASS )
				{
					// Failed to post the message, even after 10 ticks.
					printf("xQueueSend Fail!\r\n");
				}				
				//mJniObserver->OnEvent(rData->data, percent, RECEIVE_BURN_FINISH_INDEX, percent); 
			}
			//pModem->resetPmodemStatus();
			init_status();
			mProgress = -1;
			printf("***burn finish, progress(%d%%)\r\n", percent);	
			break;

		case CMD_ERR_PACKET_INDEX:
		case CMD_ERR_SIZE_EXT:
		case CMD_ERR_FLASH_RW:
		case CMD_ERR_TRANSMISS_START:
		case CMD_ERR_PACKET_SIZES:
			if (mState->packetTotal == 0) 
			{
				percent = 0;
			}
			else 
			{
				percent = (int)
				((mState->packetNum*1.0*100)/(mState->packetTotal+5*1.0));
			}
			printf("***RECEIVE_TMODEM_ERR_INDEX\r\n");
			//pModem->resetPmodemStatus();
			init_status();
			//xSemaphoreGive( xTspSemaphore );
			ulVar = TRANSPORT_ERROR;
			if( xQueueSend( xTransportQueue, ( void * ) &ulVar, ( TickType_t ) 100 ) != pdPASS )
			{
				// Failed to post the message, even after 10 ticks.
				printf("xQueueSend Fail!\r\n");
			}				
			//mJniObserver->OnEvent(rData->data, percent, RECEIVE_TMODEM_ERR_INDEX, percent);
			break;

		case CMD_ERR_UPDATE_REQUEST:
			printf("***CMD_ERR_UPDATE_REQUEST\r\n");
			//pModem->resetPmodemStatus();
			init_status();
			//xSemaphoreGive( xTspSemaphore );
			ulVar = TRANSPORT_ERROR;
			if( xQueueSend( xTransportQueue, ( void * ) &ulVar, ( TickType_t ) 100 ) != pdPASS )
			{
				// Failed to post the message, even after 10 ticks.
				printf("xQueueSend Fail!\r\n");
			}				
			//mJniObserver->OnEvent(rData->data, rData->head.len, RECEIVE_TMODEM_ERR_INDEX, 0);	
			break;
			
		default:			
			printf("error!\r\n");
			break;	
	}

}

void handler_file_message( char *data)
{
	int result;
	result = decode_message(data, data + 1);
	deliver_message(data, result);
}

