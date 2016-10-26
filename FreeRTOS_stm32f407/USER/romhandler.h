#ifndef __ROMHANDLER_H
#define __ROMHANDLER_H	

#include <stdlib.h>
#include <string.h>

#define MCU_NUM									(5)
#define SCU_NUM									(6)
#define BOOTLOADER_NUM							(109)

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
#define APPLICATION_ADDRESS_1   (unsigned int)0x080C0000
#define APPLICATION_ADDRESS_2   (unsigned int)0x080E0000	
#define USER_FLASH_END_ADDRESS        0x080FFFFF
#define USER_FLASH_SIZE 	(USER_FLASH_END_ADDRESS-APPLICATION_ADDRESS)

/*
	index is the packet number in order,  first packet index is zero
	int type as return, 0 is handle pack scuess, none-zero is error happended!
*/
extern int rom_packet_handler(	int devNum, unsigned int index, const char *packet, int plen  );
extern int rom_error_handler( int devNum, int error );
extern int rom_finish_handler( int devNum );

extern int amr_packet_handler(	int devNum, unsigned int index, const char *packet, int plen  );
extern int amr_error_handler( int devNum, int error );
extern int amr_finish_handler( int devNum );

extern void register_rom_handlers( void );

#endif



