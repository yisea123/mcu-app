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

#define SOH                     (0x01)  /* start of 128-byte data packet */
#define STX                     (0x02)  /* start of 1024-byte data packet */
#define EOT                     (0x04)  /* end of transmission */
#define ACK                     (0x06)  /* acknowledge */
#define UPDATE                  (0x08)  /* start update rom of mcu */
#define UPDATE_DONE             (0x09)  /* update rom of mcu done*/
#define NAK                     (0x15)  /* negative acknowledge */
#define CA                      (0x18)  /* one of these in succession aborts transfer */

#define LAST_ACK								(0x16)
#define EOT_ACK									(0x17)

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
#define CONVERTDEC(c)  (c - '0')

#define CONVERTHEX_alpha(c)  (IS_AF(c) ? (c - 'A'+10) : (c - 'a'+10))
#define CONVERTHEX(c)   (IS_09(c) ? (c - '0') : CONVERTHEX_alpha(c))

/**

#define ADDR_FLASH_SECTOR_0     ((u32)0x08000000) 	//扇区0起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_1     ((u32)0x08004000) 	//扇区1起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_2     ((u32)0x08008000) 	//扇区2起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_3     ((u32)0x0800C000) 	//扇区3起始地址, 16 Kbytes  
#define ADDR_FLASH_SECTOR_4     ((u32)0x08010000) 	//扇区4起始地址, 64 Kbytes  
#define ADDR_FLASH_SECTOR_5     ((u32)0x08020000) 	//扇区5起始地址, 128 Kbytes  

#define ADDR_FLASH_SECTOR_6     ((u32)0x08040000) 	//扇区6起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_7     ((u32)0x08060000) 	//扇区7起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_8     ((u32)0x08080000) 	//扇区8起始地址, 128 Kbytes  

#define ADDR_FLASH_SECTOR_9     ((u32)0x080A0000) 	//扇区9起始地址, 128 Kbytes  
#define ADDR_FLASH_SECTOR_10    ((u32)0x080C0000) 	//扇区10起始地址,128 Kbytes  
#define ADDR_FLASH_SECTOR_11    ((u32)0x080E0000) 	//扇区11起始地址,128 Kbytes  

**/
#define BOOTLOADER							(unsigned int)0x08000000
#define BOOTLOADER_ADDRESS			(unsigned int)0x08010000
#define BOOTLOADER_ADDRESS_END 	(0x08020000-1)
#define BOOTLOADER_SIZE_MAX 		(BOOTLOADER_ADDRESS_END-BOOTLOADER_ADDRESS)

#define APPLICATION_ADDRESS   (unsigned int)0x080A0000 
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
extern unsigned int FLASH_If_Erase_Sector(unsigned int StartSector);
extern unsigned char md5[32];
extern unsigned int romSize;
extern char session_begin, mTmodemTickCount;
#endif
