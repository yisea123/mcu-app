#ifndef __T_MODEM_H
#define __T_MODEM_H	 


#define MCU_NUM									(5)
#define SCU_NUM									(6)
#define BOOTLOADER_NUM					(109)

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
#define E7                      (-3)
#define E8                      (-2)

#define CMD_PACKET_LEN					(2)
#define CMD_UPDATE_PACKET_LEN		(3)


#define PACKET_SEQNO_INDEX      (4)
#define PACKET_SEQNO_COMP_INDEX (5)

#define PACKET_HEADER           (6)
#define PACKET_TRAILER          (2)
#define PACKET_OVERHEAD         (PACKET_HEADER + PACKET_TRAILER)

/*必须可被8整除*/
#define PACKET_SIZE             (40) 
/*128  [40 48 56 64 72 80 88 96 104 112 120 128] PACKET_SIZE%8=0 && 128=>PACKET_SIZE>=40*/

#define PACKET_1K_SIZE          (1024)

#define DATA_PACKET_LEN0        (PACKET_SIZE+3)
#define DATA_PACKET_LEN1        (PACKET_1K_SIZE+3)

#define FILE_NAME_LENGTH        (20)
#define FILE_SIZE_LENGTH        (8)

#define SOH                     (0x11)  /* start of 128-byte data packet */
#define STX                     (0x12)  /* start of 1024-byte data packet */
#define EOT                     (0x14)  /* end of transmission */
#define ACK                     (0x16)  /* acknowledge */
#define UPDATE                  (0x18)  /* start update rom of mcu */
#define UPDATE_DONE             (0x19)  /* update rom of mcu done*/
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* one of these in succession aborts transfer */

#define LAST_ACK								(0x26)
#define EOT_ACK									(0x27)

#define ERR_PACKET_INDEX        (0x30)
#define ERR_SIZE_EXT            (0x31)
#define ERR_FLASH_RW            (0x32)
#define ERR_TRANSMISS_START     (0x33)
#define ERR_UPDATE_REQUEST      (0x34)

#define CRC16                   (0x43)  /* 'C' == 0x43, request 16-bit CRC */

#define ABORT1                  (0x41)  /* 'A' == 0x41, abort by user */
#define ABORT2                  (0x51)  /* 'a' == 0x61, abort by user */

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

#define BOOTLOADER							(unsigned int)0x08000000
#define BOOTLOADER_ADDRESS			(unsigned int)0x08010004
#define BOOTLOADER_ADDRESS_END 	(0x08020000-1)
#define BOOTLOADER_SIZE_MAX 		(BOOTLOADER_ADDRESS_END-BOOTLOADER_ADDRESS)

#define APPLICATION_ADDRESS   (unsigned int)0x080A0004 
#define USER_FLASH_END_ADDRESS        0x080FFFFF
#define USER_FLASH_SIZE 	(USER_FLASH_END_ADDRESS-APPLICATION_ADDRESS)

typedef  void (*pFunction)(void);

extern int handle_update_bin(const char* packet_data, int len);
extern void handle_tmodem_result(int result, const char* ack, int ack_len);
extern void report_tmodem_packet(char C);
extern void report_tmodem_packet0(char C, char C0);
extern void handle_scu_rom_update(void);
extern void handle_booloader_rom_update(void);
extern void reset_tmodem_status(void);


extern unsigned int romSize;
extern char session_begin, mTickCount;
#endif
