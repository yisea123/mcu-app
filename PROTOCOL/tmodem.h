#ifndef __T_MODEM_H
#define __T_MODEM_H	 

//0xaa 0xbb len mCmd d0 d1 d2 d3... check01 check02

//AA BB 131  0x05 00 FF foo.c 102400 CRC1 CRC2
//								ACK
//								C
//AA BB 131  0x05 01 FE Data[128] CRC1 CRC2
//								ACK
//AA BB 131  0x05 02 FD Data[128] CRC1 CRC2		
//								ACK
//.....
//AA BB 2  0x05 EOT CRC1 CRC2
//								NAK
//AA BB 2  0x05 EOT CRC1 CRC2
//								ACK
//								C
//AA BB 131  cmd 00 FF NULL[128] CRC1 CRC2
//								ACKS


#define PACKET_SEQNO_INDEX      (4)
#define PACKET_SEQNO_COMP_INDEX (5)

#define PACKET_HEADER           (6)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)
#define PACKET_SIZE             (128)
#define PACKET_1K_SIZE          (1024)

#define FILE_NAME_LENGTH        (256)
#define FILE_SIZE_LENGTH        (16)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* one of these in succession aborts transfer */
#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x61)  /* 'a' == 0x61, abort by user */

#define NAK_TIMEOUT             (0x100000)
#define MAX_ERRORS              (5)


#define IS_AF(c)  ((c >= 'A') && (c <= 'F'))
#define IS_af(c)  ((c >= 'a') && (c <= 'f'))
#define IS_09(c)  ((c >= '0') && (c <= '9'))
#define ISVALIDHEX(c)  IS_AF(c) || IS_af(c) || IS_09(c)
#define ISVALIDDEC(c)  IS_09(c)
#define CONVERTDEC(c)  (c - '0')

#define CONVERTHEX_alpha(c)  (IS_AF(c) ? (c - 'A'+10) : (c - 'a'+10))
#define CONVERTHEX(c)   (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))

#define APPLICATION_ADDRESS   (unsigned int)0x08080000 
#define USER_FLASH_END_ADDRESS        0x080FFFFF

typedef  void (*pFunction)(void);

extern int handle_update_bin(const char* packet_data, int len);

#endif
