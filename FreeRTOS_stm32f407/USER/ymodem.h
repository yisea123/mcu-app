#ifndef __YMODEM_H
#define __YMODEM_H	 

#define NOMAL										(0)

#define UD0											(1)   /* start update mcu rom */
#define UD1											(2)
#define UD2											(3)
#define UD3											(4)
#define UD4											(5)   
#define UD5											(6)   /* update rom finish */
#define UDS											(7)   /*   update stop     */

#define E0											(-10)
#define E1											(-9)
#define E2											(-8)
#define E3											(-7)
#define E4											(-6)
#define E5											(-5)
#define E6											(-4)
#define E7                      					(-3)
#define E8                      					(-2)

#define CMD_PACKET_LEN								(1)
#define CMD_UPDATE_PACKET_LEN						(2)

#define SEQNO_INDEX      							(0)
#define SEQNO_COMP_INDEX 							(1)
#define PACKET_HEADER           					(2)

#define PACKET_TRAILER          					(2)
#define PACKET_OVERHEAD         					(PACKET_HEADER + PACKET_TRAILER)

/*必须可被16整除*/
#define PACKET_SIZE             					(128) 
/*128  [64 80 96 112 128] 
PACKET_SIZE%16=0 && 128=>PACKET_SIZE>32+FILE_NAME_LENGTH+FILE_SIZE_LENGTH+2*/

#define PACKET_1K_SIZE          (1024)

#define DATA_PACKET_LEN0        (PACKET_SIZE+3)
#define DATA_PACKET_LEN1        (PACKET_1K_SIZE+3)

#define FILE_NAME_LENGTH        (20)
#define FILE_SIZE_LENGTH        (8)

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define UPDATE                  (0x08)  /* start update rom of mcu */
#define UPDATE_DONE             (0x09)  /* update rom of mcu done*/
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* one of these in succession aborts transfer */

#define LAST_ACK				(0x16)
#define EOT_ACK					(0x17)

#define ERR_PACKET_INDEX        (0x20)
#define ERR_SIZE_EXT            (0x21)
#define ERR_FLASH_RW            (0x22)
#define ERR_TRANSMISS_START     (0x23)
#define ERR_UPDATE_REQUEST      (0x24)

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
#define CONVERTDEC(c)  ( c - '0' )
#define CONVERTHEX_alpha(c)  (IS_AF(c) ? (c - 'A' + 10) : (c - 'a' + 10))
#define CONVERTHEX(c)   	 (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))

extern int handle_ymodem_command( char* rpacket, int len );

#endif
