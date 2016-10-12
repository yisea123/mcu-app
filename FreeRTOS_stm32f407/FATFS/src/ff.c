/*----------------------------------------------------------------------------/
/  FatFs - FAT file system module  R0.10b                (C)ChaN, 2014
/-----------------------------------------------------------------------------/
/ FatFs module is a generic FAT file system module for small embedded systems.
/ This is a free software that opened for education, research and commercial
/ developments under license policy of following terms.
/
/  Copyright (C) 2014, ChaN, all right reserved.
/
/ * The FatFs module is a free software and there is NO WARRANTY.
/ * No restriction on use. You can use, modify and redistribute it for
/   personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
/ * Redistributions of source code must retain the above copyright notice.
/
/-----------------------------------------------------------------------------/
/ Feb 26,'06 R0.00  Prototype.
/
/ Apr 29,'06 R0.01  First stable version.
/
/ Jun 01,'06 R0.02  Added FAT12 support.
/                   Removed unbuffered mode.
/                   Fixed a problem on small (<32M) partition.
/ Jun 10,'06 R0.02a Added a configuration option (_FS_MINIMUM).
/
/ Sep 22,'06 R0.03  Added f_rename().
/                   Changed option _FS_MINIMUM to _FS_MINIMIZE.
/ Dec 11,'06 R0.03a Improved cluster scan algorithm to write files fast.
/                   Fixed f_mkdir() creates incorrect directory on FAT32.
/
/ Feb 04,'07 R0.04  Supported multiple drive system.
/                   Changed some interfaces for multiple drive system.
/                   Changed f_mountdrv() to f_mount().
/                   Added f_mkfs().
/ Apr 01,'07 R0.04a Supported multiple partitions on a physical drive.
/                   Added a capability of extending file size to f_lseek().
/                   Added minimization level 3.
/                   Fixed an endian sensitive code in f_mkfs().
/ May 05,'07 R0.04b Added a configuration option _USE_NTFLAG.
/                   Added FSINFO support.
/                   Fixed DBCS name can result FR_INVALID_NAME.
/                   Fixed short seek (<= csize) collapses the file object.
/
/ Aug 25,'07 R0.05  Changed arguments of f_read(), f_write() and f_mkfs().
/                   Fixed f_mkfs() on FAT32 creates incorrect FSINFO.
/                   Fixed f_mkdir() on FAT32 creates incorrect directory.
/ Feb 03,'08 R0.05a Added f_truncate() and f_utime().
/                   Fixed off by one error at FAT sub-type determination.
/                   Fixed btr in f_read() can be mistruncated.
/                   Fixed cached sector is not flushed when create and close without write.
/
/ Apr 01,'08 R0.06  Added fputc(), fputs(), fprintf() and fgets().
/                   Improved performance of f_lseek() on moving to the same or following cluster.
/
/ Apr 01,'09 R0.07  Merged Tiny-FatFs as a configuration option. (_FS_TINY)
/                   Added long file name feature.
/                   Added multiple code page feature.
/                   Added re-entrancy for multitask operation.
/                   Added auto cluster size selection to f_mkfs().
/                   Added rewind option to f_readdir().
/                   Changed result code of critical errors.
/                   Renamed string functions to avoid name collision.
/ Apr 14,'09 R0.07a Separated out OS dependent code on reentrant cfg.
/                   Added multiple sector size feature.
/ Jun 21,'09 R0.07c Fixed f_unlink() can return FR_OK on error.
/                   Fixed wrong cache control in f_lseek().
/                   Added relative path feature.
/                   Added f_chdir() and f_chdrive().
/                   Added proper case conversion to extended character.
/ Nov 03,'09 R0.07e Separated out configuration options from ff.h to ffconf.h.
/                   Fixed f_unlink() fails to remove a sub-directory on _FS_RPATH.
/                   Fixed name matching error on the 13 character boundary.
/                   Added a configuration option, _LFN_UNICODE.
/                   Changed f_readdir() to return the SFN with always upper case on non-LFN cfg.
/
/ May 15,'10 R0.08  Added a memory configuration option. (_USE_LFN = 3)
/                   Added file lock feature. (_FS_SHARE)
/                   Added fast seek feature. (_USE_FASTSEEK)
/                   Changed some types on the API, XCHAR->TCHAR.
/                   Changed .fname in the FILINFO structure on Unicode cfg.
/                   String functions support UTF-8 encoding files on Unicode cfg.
/ Aug 16,'10 R0.08a Added f_getcwd().
/                   Added sector erase feature. (_USE_ERASE)
/                   Moved file lock semaphore table from fs object to the bss.
/                   Fixed a wrong directory entry is created on non-LFN cfg when the given name contains ';'.
/                   Fixed f_mkfs() creates wrong FAT32 volume.
/ Jan 15,'11 R0.08b Fast seek feature is also applied to f_read() and f_write().
/                   f_lseek() reports required table size on creating CLMP.
/                   Extended format syntax of f_printf().
/                   Ignores duplicated directory separators in given path name.
/
/ Sep 06,'11 R0.09  f_mkfs() supports multiple partition to complete the multiple partition feature.
/                   Added f_fdisk().
/ Aug 27,'12 R0.09a Changed f_open() and f_opendir() reject null object pointer to avoid crash.
/                   Changed option name _FS_SHARE to _FS_LOCK.
/                   Fixed assertion failure due to OS/2 EA on FAT12/16 volume.
/ Jan 24,'13 R0.09b Added f_setlabel() and f_getlabel().
/
/ Oct 02,'13 R0.10  Added selection of character encoding on the file. (_STRF_ENCODE)
/                   Added f_closedir().
/                   Added forced full FAT scan for f_getfree(). (_FS_NOFSINFO)
/                   Added forced mount feature with changes of f_mount().
/                   Improved behavior of volume auto detection.
/                   Improved write throughput of f_puts() and f_printf().
/                   Changed argument of f_chdrive(), f_mkfs(), disk_read() and disk_write().
/                   Fixed f_write() can be truncated when the file size is close to 4GB.
/                   Fixed f_open(), f_mkdir() and f_setlabel() can return incorrect error code.
/ Jan 15,'14 R0.10a Added arbitrary strings as drive number in the path name. (_STR_VOLUME_ID)
/                   Added a configuration option of minimum sector size. (_MIN_SS)
/                   2nd argument of f_rename() can have a drive number and it will be ignored.
/                   Fixed f_mount() with forced mount fails when drive number is >= 1.
/                   Fixed f_close() invalidates the file object without volume lock.
/                   Fixed f_closedir() returns but the volume lock is left acquired.
/                   Fixed creation of an entry with LFN fails on too many SFN collisions.
/ May 19,'14 R0.10b Fixed a hard error in the disk I/O layer can collapse the directory entry.
/                   Fixed LFN entry is not deleted on delete/rename an object with lossy converted SFN.
/---------------------------------------------------------------------------*/

#include "ff.h"			/* Declarations of FatFs API */
#include "diskio.h"		/* Declarations of disk I/O functions */
#include <stdio.h>



/*--------------------------------------------------------------------------

   Module Private Definitions

---------------------------------------------------------------------------*/

#if _FATFS != 8051	/* Revision ID */
#error Wrong include file (ff.h).
#endif


/* Reentrancy related */
#if _FS_REENTRANT
#if _USE_LFN == 1
#error Static LFN work area cannot be used at thread-safe configuration.
#endif
/*互斥锁的申请与释放
针对不同的操作系统，有不同的实现*/
#define	ENTER_FF(fs)		{ if (!lock_fs(fs)) return FR_TIMEOUT; }
#define	LEAVE_FF(fs, res)	{ unlock_fs(fs, res); return res; }
#else
#define	ENTER_FF(fs)
#define LEAVE_FF(fs, res)	return res
#endif

#define	ABORT(fs, res)		{ fp->err = (BYTE)(res); LEAVE_FF(fs, res); }


/* Definitions of sector size */
#if (_MAX_SS < _MIN_SS) || (_MAX_SS != 512 && _MAX_SS != 1024 && _MAX_SS != 2048 && _MAX_SS != 4096) || (_MIN_SS != 512 && _MIN_SS != 1024 && _MIN_SS != 2048 && _MIN_SS != 4096)
#error Wrong sector size configuration.
#endif
#if _MAX_SS == _MIN_SS
#define	SS(fs)	((UINT)_MAX_SS)	/* Fixed sector size */
#else
#define	SS(fs)	((fs)->ssize)	/* Variable sector size */
#endif


/* File access control feature */
#if _FS_LOCK
#if _FS_READONLY
#error _FS_LOCK must be 0 at read-only cfg.
#endif
typedef struct {
	FATFS *fs;		/* Object ID 1, volume (NULL:blank entry) */
	DWORD clu;		/* Object ID 2, directory (0:root) */
	WORD idx;		/* Object ID 3, directory index */
	WORD ctr;		/* Object open counter, 0:none, 0x01..0xFF:read mode open count, 0x100:write mode */
} FILESEM;
#endif



/* DBCS code ranges and SBCS extend character conversion table */

#if _CODE_PAGE == 932	/* Japanese Shift-JIS */
#define _DF1S	0x81	/* DBC 1st byte range 1 start */
#define _DF1E	0x9F	/* DBC 1st byte range 1 end */
#define _DF2S	0xE0	/* DBC 1st byte range 2 start */
#define _DF2E	0xFC	/* DBC 1st byte range 2 end */
#define _DS1S	0x40	/* DBC 2nd byte range 1 start */
#define _DS1E	0x7E	/* DBC 2nd byte range 1 end */
#define _DS2S	0x80	/* DBC 2nd byte range 2 start */
#define _DS2E	0xFC	/* DBC 2nd byte range 2 end */

#elif _CODE_PAGE == 936	/* Simplified Chinese GBK */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x40
#define _DS1E	0x7E
#define _DS2S	0x80
#define _DS2E	0xFE

#elif _CODE_PAGE == 949	/* Korean */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x41
#define _DS1E	0x5A
#define _DS2S	0x61
#define _DS2E	0x7A
#define _DS3S	0x81
#define _DS3E	0xFE

#elif _CODE_PAGE == 950	/* Traditional Chinese Big5 */
#define _DF1S	0x81
#define _DF1E	0xFE
#define _DS1S	0x40
#define _DS1E	0x7E
#define _DS2S	0xA1
#define _DS2E	0xFE

#elif _CODE_PAGE == 437	/* U.S. (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0x41,0x8E,0x41,0x8F,0x80,0x45,0x45,0x45,0x49,0x49,0x49,0x8E,0x8F,0x90,0x92,0x92,0x4F,0x99,0x4F,0x55,0x55,0x59,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 720	/* Arabic (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x45,0x41,0x84,0x41,0x86,0x43,0x45,0x45,0x45,0x49,0x49,0x8D,0x8E,0x8F,0x90,0x92,0x92,0x93,0x94,0x95,0x49,0x49,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 737	/* Greek (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x92,0x92,0x93,0x94,0x95,0x96,0x97,0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87, \
				0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0xAA,0x92,0x93,0x94,0x95,0x96,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0x97,0xEA,0xEB,0xEC,0xE4,0xED,0xEE,0xE7,0xE8,0xF1,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 775	/* Baltic (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x91,0xA0,0x8E,0x95,0x8F,0x80,0xAD,0xED,0x8A,0x8A,0xA1,0x8D,0x8E,0x8F,0x90,0x92,0x92,0xE2,0x99,0x95,0x96,0x97,0x97,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xE0,0xA3,0xA3,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xB5,0xB6,0xB7,0xB8,0xBD,0xBE,0xC6,0xC7,0xA5,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE3,0xE8,0xE8,0xEA,0xEA,0xEE,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 850	/* Multilingual Latin 1 (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xB7,0x8F,0x80,0xD2,0xD3,0xD4,0xD8,0xD7,0xDE,0x8E,0x8F,0x90,0x92,0x92,0xE2,0x99,0xE3,0xEA,0xEB,0x59,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9F, \
				0xB5,0xD6,0xE0,0xE9,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE7,0xE7,0xE9,0xEA,0xEB,0xED,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 852	/* Latin 2 (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xDE,0x8F,0x80,0x9D,0xD3,0x8A,0x8A,0xD7,0x8D,0x8E,0x8F,0x90,0x91,0x91,0xE2,0x99,0x95,0x95,0x97,0x97,0x99,0x9A,0x9B,0x9B,0x9D,0x9E,0x9F, \
				0xB5,0xD6,0xE0,0xE9,0xA4,0xA4,0xA6,0xA6,0xA8,0xA8,0xAA,0x8D,0xAC,0xB8,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBD,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC6,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD1,0xD1,0xD2,0xD3,0xD2,0xD5,0xD6,0xD7,0xB7,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE3,0xD5,0xE6,0xE6,0xE8,0xE9,0xE8,0xEB,0xED,0xED,0xDD,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xEB,0xFC,0xFC,0xFE,0xFF}

#elif _CODE_PAGE == 855	/* Cyrillic (OEM) */
#define _DF1S	0
#define _EXCVT {0x81,0x81,0x83,0x83,0x85,0x85,0x87,0x87,0x89,0x89,0x8B,0x8B,0x8D,0x8D,0x8F,0x8F,0x91,0x91,0x93,0x93,0x95,0x95,0x97,0x97,0x99,0x99,0x9B,0x9B,0x9D,0x9D,0x9F,0x9F, \
				0xA1,0xA1,0xA3,0xA3,0xA5,0xA5,0xA7,0xA7,0xA9,0xA9,0xAB,0xAB,0xAD,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB6,0xB6,0xB8,0xB8,0xB9,0xBA,0xBB,0xBC,0xBE,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD1,0xD1,0xD3,0xD3,0xD5,0xD5,0xD7,0xD7,0xDD,0xD9,0xDA,0xDB,0xDC,0xDD,0xE0,0xDF, \
				0xE0,0xE2,0xE2,0xE4,0xE4,0xE6,0xE6,0xE8,0xE8,0xEA,0xEA,0xEC,0xEC,0xEE,0xEE,0xEF,0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF8,0xFA,0xFA,0xFC,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 857	/* Turkish (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xB7,0x8F,0x80,0xD2,0xD3,0xD4,0xD8,0xD7,0x98,0x8E,0x8F,0x90,0x92,0x92,0xE2,0x99,0xE3,0xEA,0xEB,0x98,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9E, \
				0xB5,0xD6,0xE0,0xE9,0xA5,0xA5,0xA6,0xA6,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xDE,0x59,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 858	/* Multilingual Latin 1 + Euro (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x9A,0x90,0xB6,0x8E,0xB7,0x8F,0x80,0xD2,0xD3,0xD4,0xD8,0xD7,0xDE,0x8E,0x8F,0x90,0x92,0x92,0xE2,0x99,0xE3,0xEA,0xEB,0x59,0x99,0x9A,0x9D,0x9C,0x9D,0x9E,0x9F, \
				0xB5,0xD6,0xE0,0xE9,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC7,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD1,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE5,0xE5,0xE6,0xE7,0xE7,0xE9,0xEA,0xEB,0xED,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 862	/* Hebrew (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x41,0x49,0x4F,0x55,0xA5,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0x21,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 866	/* Russian (OEM) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0x90,0x91,0x92,0x93,0x9d,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,0xF0,0xF0,0xF2,0xF2,0xF4,0xF4,0xF6,0xF6,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 874	/* Thai (OEM, Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 1250 /* Central Europe (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x8A,0x9B,0x8C,0x8D,0x8E,0x8F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xA3,0xB4,0xB5,0xB6,0xB7,0xB8,0xA5,0xAA,0xBB,0xBC,0xBD,0xBC,0xAF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xF7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xFF}

#elif _CODE_PAGE == 1251 /* Cyrillic (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x82,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x80,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x8A,0x9B,0x8C,0x8D,0x8E,0x8F, \
				0xA0,0xA2,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB2,0xA5,0xB5,0xB6,0xB7,0xA8,0xB9,0xAA,0xBB,0xA3,0xBD,0xBD,0xAF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF}

#elif _CODE_PAGE == 1252 /* Latin 1 (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0xAd,0x9B,0x8C,0x9D,0xAE,0x9F, \
				0xA0,0x21,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xF7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0x9F}

#elif _CODE_PAGE == 1253 /* Greek (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xA2,0xB8,0xB9,0xBA, \
				0xE0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xF2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xFB,0xBC,0xFD,0xBF,0xFF}

#elif _CODE_PAGE == 1254 /* Turkish (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x8A,0x9B,0x8C,0x9D,0x9E,0x9F, \
				0xA0,0x21,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xF7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0x9F}

#elif _CODE_PAGE == 1255 /* Hebrew (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0x21,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 1256 /* Arabic (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x8C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0x41,0xE1,0x41,0xE3,0xE4,0xE5,0xE6,0x43,0x45,0x45,0x45,0x45,0xEC,0xED,0x49,0x49,0xF0,0xF1,0xF2,0xF3,0x4F,0xF5,0xF6,0xF7,0xF8,0x55,0xFA,0x55,0x55,0xFD,0xFE,0xFF}

#elif _CODE_PAGE == 1257 /* Baltic (Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F, \
				0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xA8,0xB9,0xAA,0xBB,0xBC,0xBD,0xBE,0xAF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xF7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xFF}

#elif _CODE_PAGE == 1258 /* Vietnam (OEM, Windows) */
#define _DF1S	0
#define _EXCVT {0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0xAC,0x9D,0x9E,0x9F, \
				0xA0,0x21,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF, \
				0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xEC,0xCD,0xCE,0xCF,0xD0,0xD1,0xF2,0xD3,0xD4,0xD5,0xD6,0xF7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xFE,0x9F}

#elif _CODE_PAGE == 1	/* ASCII (for only non-LFN cfg) */
#if _USE_LFN
#error Cannot use LFN feature without valid code page.
#endif
#define _DF1S	0

#else
#error Unknown code page

#endif


/* Character code support macros */
#define IsUpper(c)	(((c)>='A')&&((c)<='Z'))
#define IsLower(c)	(((c)>='a')&&((c)<='z'))
#define IsDigit(c)	(((c)>='0')&&((c)<='9'))

#if _DF1S		/* Code page is DBCS */

#ifdef _DF2S	/* Two 1st byte areas */
#define IsDBCS1(c)	(((BYTE)(c) >= _DF1S && (BYTE)(c) <= _DF1E) || ((BYTE)(c) >= _DF2S && (BYTE)(c) <= _DF2E))
#else			/* One 1st byte area */
#define IsDBCS1(c)	((BYTE)(c) >= _DF1S && (BYTE)(c) <= _DF1E)
#endif

#ifdef _DS3S	/* Three 2nd byte areas */
#define IsDBCS2(c)	(((BYTE)(c) >= _DS1S && (BYTE)(c) <= _DS1E) || ((BYTE)(c) >= _DS2S && (BYTE)(c) <= _DS2E) || ((BYTE)(c) >= _DS3S && (BYTE)(c) <= _DS3E))
#else			/* Two 2nd byte areas */
#define IsDBCS2(c)	(((BYTE)(c) >= _DS1S && (BYTE)(c) <= _DS1E) || ((BYTE)(c) >= _DS2S && (BYTE)(c) <= _DS2E))
#endif

#else			/* Code page is SBCS */

#define IsDBCS1(c)	0
#define IsDBCS2(c)	0

#endif /* _DF1S */


/* Name status flags */
#define NS			11		/* Index of name status byte in fn[] */
#define NS_LOSS		0x01	/* Out of 8.3 format */
#define NS_LFN		0x02	/* Force to create LFN entry */
#define NS_LAST		0x04	/* Last segment */
#define NS_BODY		0x08	/* Lower case flag (body) */
#define NS_EXT		0x10	/* Lower case flag (ext) */
#define NS_DOT		0x20	/* Dot entry */


/* FAT sub-type boundaries */
#define MIN_FAT16	4086U	/* Minimum number of clusters for FAT16 */
#define	MIN_FAT32	65526U	/* Minimum number of clusters for FAT32 */


/* FatFs refers the members in the FAT structures as byte array instead of
/ structure member because the structure is not binary compatible between
/ different platforms */

#define BS_jmpBoot			0		/* Jump instruction (3) */
#define BS_OEMName			3		/* OEM name (8) */
#define BPB_BytsPerSec		11		/* Sector size [byte] (2) */
#define BPB_SecPerClus		13		/* Cluster size [sector] (1) */
#define BPB_RsvdSecCnt		14		/* Size of reserved area [sector] (2) */
#define BPB_NumFATs			16		/* Number of FAT copies (1) */
#define BPB_RootEntCnt		17		/* Number of root directory entries for FAT12/16 (2) */
#define BPB_TotSec16		19		/* Volume size [sector] (2) */
#define BPB_Media			21		/* Media descriptor (1) */
#define BPB_FATSz16			22		/* FAT size [sector] (2) */
#define BPB_SecPerTrk		24		/* Track size [sector] (2) */
#define BPB_NumHeads		26		/* Number of heads (2) */
#define BPB_HiddSec			28		/* Number of special hidden sectors (4) */
#define BPB_TotSec32		32		/* Volume size [sector] (4) */
#define BS_DrvNum			36		/* Physical drive number (2) */
#define BS_BootSig			38		/* Extended boot signature (1) */
#define BS_VolID			39		/* Volume serial number (4) */
#define BS_VolLab			43		/* Volume label (8) */
#define BS_FilSysType		54		/* File system type (1) */
#define BPB_FATSz32			36		/* FAT size [sector] (4) */
#define BPB_ExtFlags		40		/* Extended flags (2) */
#define BPB_FSVer			42		/* File system version (2) */
#define BPB_RootClus		44		/* Root directory first cluster (4) */
#define BPB_FSInfo			48		/* Offset of FSINFO sector (2) */
#define BPB_BkBootSec		50		/* Offset of backup boot sector (2) */
#define BS_DrvNum32			64		/* Physical drive number (2) */
#define BS_BootSig32		66		/* Extended boot signature (1) */
#define BS_VolID32			67		/* Volume serial number (4) */
#define BS_VolLab32			71		/* Volume label (8) */
#define BS_FilSysType32		82		/* File system type (1) */
#define	FSI_LeadSig			0		/* FSI: Leading signature (4) */
#define	FSI_StrucSig		484		/* FSI: Structure signature (4) */
#define	FSI_Free_Count		488		/* FSI: Number of free clusters (4) */
#define	FSI_Nxt_Free		492		/* FSI: Last allocated cluster (4) */
#define MBR_Table			446		/* MBR: Partition table offset (2) */
#define	SZ_PTE				16		/* MBR: Size of a partition table entry */
#define BS_55AA				510		/* Signature word (2) */

#define	DIR_Name			0		/* Short file name (11) */
#define	DIR_Attr			11		/* Attribute (1) */
#define	DIR_NTres			12		/* NT flag (1) */
#define DIR_CrtTimeTenth	13		/* Created time sub-second (1) */
#define	DIR_CrtTime			14		/* Created time (2) */
#define	DIR_CrtDate			16		/* Created date (2) */
#define DIR_LstAccDate		18		/* Last accessed date (2) */
#define	DIR_FstClusHI		20		/* Higher 16-bit of first cluster (2) */
#define	DIR_WrtTime			22		/* Modified time (2) */
#define	DIR_WrtDate			24		/* Modified date (2) */
#define	DIR_FstClusLO		26		/* Lower 16-bit of first cluster (2) */
#define	DIR_FileSize		28		/* File size (4) */
#define	LDIR_Ord			0		/* LFN entry order and LLE flag (1) */
#define	LDIR_Attr			11		/* LFN attribute (1) */
#define	LDIR_Type			12		/* LFN type (1) */
#define	LDIR_Chksum			13		/* Sum of corresponding SFN entry */
#define	LDIR_FstClusLO		26		/* Filled by zero (0) */
#define	SZ_DIR				32		/* Size of a directory entry */
#define	LLE					0x40	/* Last long entry flag in LDIR_Ord */
#define	DDE					0xE5	/* Deleted directory entry mark in DIR_Name[0] */
#define	NDDE				0x05	/* Replacement of the character collides with DDE */




/*------------------------------------------------------------*/
/* Module private work area                                   */
/*------------------------------------------------------------*/
/* Note that uninitialized variables with static duration are
/  guaranteed zero/null as initial value. If not, either the
/  linker or start-up routine is out of ANSI-C standard.
*/

#if _VOLUMES >= 1 || _VOLUMES <= 10
static
FATFS *FatFs[_VOLUMES];		/* Pointer to the file system objects (logical drives) */
#else
#error Number of volumes must be 1 to 10.
#endif

static
WORD Fsid;					/* File system mount ID */

#if _FS_RPATH && _VOLUMES >= 2
static
BYTE CurrVol;				/* Current drive */
#endif

#if _FS_LOCK
static
FILESEM	Files[_FS_LOCK];	/* Open object lock semaphores */
#endif

#if _USE_LFN == 0			/* No LFN feature */
#define	DEF_NAMEBUF			BYTE sfn[12]
#define INIT_BUF(dobj)		(dobj).fn = sfn
#define	FREE_BUF()

#elif _USE_LFN == 1			/* LFN feature with static working buffer */
static
WCHAR LfnBuf[_MAX_LFN+1];
/*定义成全局变量，所以有可能会导致多任务进入
安全的问题*/
#define	DEF_NAMEBUF			BYTE sfn[12]
#define INIT_BUF(dobj)		{ (dobj).fn = sfn; (dobj).lfn = LfnBuf; }
#define	FREE_BUF()

#elif _USE_LFN == 2 		/* LFN feature with dynamic working buffer on the stack */
#define	DEF_NAMEBUF			BYTE sfn[12]; WCHAR lbuf[_MAX_LFN+1]
/*使用任务的栈开辟lfn的空间，对多任务来说
安全的，但是消耗栈，要注意多分配点栈给任务*/
#define INIT_BUF(dobj)		{ (dobj).fn = sfn; (dobj).lfn = lbuf; }
#define	FREE_BUF()

#elif _USE_LFN == 3 		/* LFN feature with dynamic working buffer on the heap */
#define	DEF_NAMEBUF			BYTE sfn[12]; WCHAR *lfn
/*使用动态内存分配*/
#define INIT_BUF(dobj)		{ lfn = ff_memalloc((_MAX_LFN + 1) * 2); \
							  if (!lfn) LEAVE_FF((dobj).fs, FR_NOT_ENOUGH_CORE); \
							  (dobj).lfn = lfn;	(dobj).fn = sfn; }
#define	FREE_BUF()			ff_memfree(lfn)

#else
#error Wrong LFN configuration.
#endif


#ifdef _EXCVT
static
const BYTE ExCvt[] = _EXCVT;	/* Upper conversion table for extended characters */
#endif






/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------*/
/* String functions                                                      */
/*-----------------------------------------------------------------------*/

/* Copy memory to memory */
static
void mem_cpy (void* dst, const void* src, UINT cnt) {
	BYTE *d = (BYTE*)dst;
	const BYTE *s = (const BYTE*)src;

#if _WORD_ACCESS == 1
	while (cnt >= sizeof (int)) {
		*(int*)d = *(int*)s;
		d += sizeof (int); s += sizeof (int);
		cnt -= sizeof (int);
	}
#endif
	while (cnt--)
		*d++ = *s++;
}

/* Fill memory */
static
void mem_set (void* dst, int val, UINT cnt) {
	BYTE *d = (BYTE*)dst;

	while (cnt--)
		*d++ = (BYTE)val;
}

/* Compare memory to memory */
static
int mem_cmp (const void* dst, const void* src, UINT cnt) {
	const BYTE *d = (const BYTE *)dst, *s = (const BYTE *)src;
	int r = 0;

	while (cnt-- && (r = *d++ - *s++) == 0) ;
	return r;
}

/* Check if chr is contained in the string */
static
int chk_chr (const char* str, int chr) {
	while (*str && *str != chr) str++;
	return *str;
}




/*-----------------------------------------------------------------------*/
/* Request/Release grant to access the volume                            */
/*-----------------------------------------------------------------------*/
#if _FS_REENTRANT
static
int lock_fs (
	FATFS* fs		/* File system object */
)
{
	return ff_req_grant(fs->sobj);
}


static
void unlock_fs (
	FATFS* fs,		/* File system object */
	FRESULT res		/* Result code to be returned */
)
{
	if (fs &&
		res != FR_NOT_ENABLED &&
		res != FR_INVALID_DRIVE &&
		res != FR_INVALID_OBJECT &&
		res != FR_TIMEOUT) {
		ff_rel_grant(fs->sobj);
	}
}
#endif




/*-----------------------------------------------------------------------*/
/* File lock control functions                                           */
/*-----------------------------------------------------------------------*/
#if _FS_LOCK

static
FRESULT chk_lock (	/* Check if the file can be accessed */
	DIR* dp,		/* Directory object pointing the file to be checked */
	int acc			/* Desired access type (0:Read, 1:Write, 2:Delete/Rename) */
)
{
	UINT i, be;

	/* Search file semaphore table */
	for (i = be = 0; i < _FS_LOCK; i++) {
		if (Files[i].fs) {	/* Existing entry */
			if (Files[i].fs == dp->fs &&	 	/* Check if the object matched with an open object */
				Files[i].clu == dp->sclust &&
				Files[i].idx == dp->index) break;
		} else {			/* Blank entry */
			be = 1;
		}
	}
	if (i == _FS_LOCK)	/* The object is not opened */
		return (be || acc == 2) ? FR_OK : FR_TOO_MANY_OPEN_FILES;	/* Is there a blank entry for new object? */

	/* The object has been opened. Reject any open against writing file and all write mode open */
	return (acc || Files[i].ctr == 0x100) ? FR_LOCKED : FR_OK;
}


static
int enq_lock (void)	/* Check if an entry is available for a new object */
{
	UINT i;

	for (i = 0; i < _FS_LOCK && Files[i].fs; i++) ;
	return (i == _FS_LOCK) ? 0 : 1;
}


static
UINT inc_lock (	/* Increment object open counter and returns its index (0:Internal error) */
	DIR* dp,	/* Directory object pointing the file to register or increment */
	int acc		/* Desired access (0:Read, 1:Write, 2:Delete/Rename) */
)
{
	UINT i;


	for (i = 0; i < _FS_LOCK; i++) {	/* Find the object */
		if (Files[i].fs == dp->fs &&
			Files[i].clu == dp->sclust &&
			Files[i].idx == dp->index) break;
	}

	if (i == _FS_LOCK) {				/* Not opened. Register it as new. */
		for (i = 0; i < _FS_LOCK && Files[i].fs; i++) ;
		if (i == _FS_LOCK) return 0;	/* No free entry to register (int err) */
		Files[i].fs = dp->fs;
		Files[i].clu = dp->sclust;
		Files[i].idx = dp->index;
		Files[i].ctr = 0;
	}

	if (acc && Files[i].ctr) return 0;	/* Access violation (int err) */

	Files[i].ctr = acc ? 0x100 : Files[i].ctr + 1;	/* Set semaphore value */

	return i + 1;
}


static
FRESULT dec_lock (	/* Decrement object open counter */
	UINT i			/* Semaphore index (1..) */
)
{
	WORD n;
	FRESULT res;


	if (--i < _FS_LOCK) {	/* Shift index number origin from 0 */
		n = Files[i].ctr;
		if (n == 0x100) n = 0;		/* If write mode open, delete the entry */
		if (n) n--;					/* Decrement read mode open count */
		Files[i].ctr = n;
		if (!n) Files[i].fs = 0;	/* Delete the entry if open count gets zero */
		res = FR_OK;
	} else {
		res = FR_INT_ERR;			/* Invalid index nunber */
	}
	return res;
}


static
void clear_lock (	/* Clear lock entries of the volume */
	FATFS *fs
)
{
	UINT i;

	for (i = 0; i < _FS_LOCK; i++) {
		if (Files[i].fs == fs) Files[i].fs = 0;
	}
}
#endif




/*-----------------------------------------------------------------------*/
/* Move/Flush disk access window in the file system object               */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
static
FRESULT sync_window (
	FATFS* fs		/* File system object */
)
{
	DWORD wsect;
	UINT nf;


	if (fs->wflag) {	/* Write back the sector if it is dirty */
		wsect = fs->winsect;	/* Current sector number */
		if (disk_write(fs->drv, fs->win, wsect, 1))
			return FR_DISK_ERR;
		fs->wflag = 0;
		if (wsect - fs->fatbase < fs->fsize) {		/* Is it in the FAT area? */
			//如果写的是fat表，把备份的fat也回写
			for (nf = fs->n_fats; nf >= 2; nf--) {	/* Reflect the change to all FAT copies */
				wsect += fs->fsize;
				disk_write(fs->drv, fs->win, wsect, 1);
			}
		}
	}
	return FR_OK;
}
#endif


static
FRESULT move_window (
	FATFS* fs,		/* File system object */
	DWORD sector	/* Sector number to make appearance in the fs->win[] */
)
{
	/*当sector与当前win缓存中的数据不是同个
	扇区，重新去读flash的扇区。*/
	if (sector != fs->winsect) {	/* Changed current window */
		//printf("%s: sector(%d) != fs->winsect(%d)\r\n", __func__, 
		// sector, fs->winsect);
#if !_FS_READONLY
		if (sync_window(fs) != FR_OK)
			return FR_DISK_ERR;
#endif
		if (disk_read(fs->drv, fs->win, sector, 1))
			return FR_DISK_ERR;
		fs->winsect = sector;
	}

	return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Synchronize file system and strage device                             */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
static
FRESULT sync_fs (	/* FR_OK: successful, FR_DISK_ERR: failed */
	FATFS* fs		/* File system object */
)
{
	FRESULT res;


	res = sync_window(fs);
	if (res == FR_OK) {
		/* Update FSINFO sector if needed */
		if (fs->fs_type == FS_FAT32 && fs->fsi_flag == 1) {
			/* Create FSINFO structure */
			mem_set(fs->win, 0, SS(fs));
			ST_WORD(fs->win+BS_55AA, 0xAA55);
			ST_DWORD(fs->win+FSI_LeadSig, 0x41615252);
			ST_DWORD(fs->win+FSI_StrucSig, 0x61417272);
			ST_DWORD(fs->win+FSI_Free_Count, fs->free_clust);
			ST_DWORD(fs->win+FSI_Nxt_Free, fs->last_clust);
			/* Write it into the FSINFO sector */
			fs->winsect = fs->volbase + 1;
			disk_write(fs->drv, fs->win, fs->winsect, 1);
			fs->fsi_flag = 0;
		}
		/* Make sure that no pending write process in the physical drive */
		if (disk_ioctl(fs->drv, CTRL_SYNC, 0) != RES_OK)
			res = FR_DISK_ERR;
	}

	return res;
}
#endif




/*-----------------------------------------------------------------------*/
/* Get sector# from cluster#                                             */
/*-----------------------------------------------------------------------*/

/*族到扇区的转化*/
DWORD clust2sect (	/* !=0: Sector number, 0: Failed - invalid cluster# */
	FATFS* fs,		/* File system object */
	DWORD clst		/* Cluster# to be converted */
)
{
	clst -= 2;//why do this?
	if (clst >= (fs->n_fatent - 2)) return 0;		/* Invalid cluster# */
	return clst * fs->csize + fs->database;
}




/*-----------------------------------------------------------------------*/
/* FAT access - Read value of a FAT entry                                */
/*-----------------------------------------------------------------------*/

/*  Get the cluster status 
就是读出fat表中当前族下的值，
可能代表一下族，也可能代表clst是最后一个族*/
DWORD get_fat (	/* 0xFFFFFFFF:Disk error, 1:Internal error, Else:Cluster status */
	FATFS* fs,	/* File system object */
	DWORD clst	/* Cluster# to get the link information */
)
{
	UINT wc, bc;
	BYTE *p;


	if (clst < 2 || clst >= fs->n_fatent)	/* Check range */
		return 1;

	switch (fs->fs_type) {
	case FS_FAT12 :
		bc = (UINT)clst; bc += bc / 2;
		if (move_window(fs, fs->fatbase + (bc / SS(fs)))) break;
		wc = fs->win[bc % SS(fs)]; bc++;
		if (move_window(fs, fs->fatbase + (bc / SS(fs)))) break;
		wc |= fs->win[bc % SS(fs)] << 8;
		return clst & 1 ? wc >> 4 : (wc & 0xFFF);

	case FS_FAT16 :
		if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 2)))) break;
		p = &fs->win[clst * 2 % SS(fs)];
		return LD_WORD(p);

	case FS_FAT32 :
		if (move_window(fs, fs->fatbase + (clst / (SS(fs) / 4)))) break;
		p = &fs->win[clst * 4 % SS(fs)];
		return LD_DWORD(p) & 0x0FFFFFFF;

	default:
		return 1;
	}

	return 0xFFFFFFFF;	/* An error occurred at the disk I/O layer */
}




/*-----------------------------------------------------------------------*/
/* FAT access - Change value of a FAT entry                              */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
/*把clst位置的族，设置成val，也就是指向了val族*/
FRESULT put_fat (
	FATFS* fs,	/* File system object */
	DWORD clst,	/* Cluster# to be changed in range of 2 to fs->n_fatent - 1 */
	DWORD val	/* New value to mark the cluster */
)
{
	UINT bc;
	BYTE *p;
	FRESULT res;


	if (clst < 2 || clst >= fs->n_fatent) {	/* Check range */
		res = FR_INT_ERR;

	} else {
		switch (fs->fs_type) {
		case FS_FAT12 :
			bc = (UINT)clst; bc += bc / 2;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != FR_OK) break;
			p = &fs->win[bc % SS(fs)];
			*p = (clst & 1) ? ((*p & 0x0F) | ((BYTE)val << 4)) : (BYTE)val;
			bc++;
			fs->wflag = 1;
			res = move_window(fs, fs->fatbase + (bc / SS(fs)));
			if (res != FR_OK) break;
			p = &fs->win[bc % SS(fs)];
			*p = (clst & 1) ? (BYTE)(val >> 4) : ((*p & 0xF0) | ((BYTE)(val >> 8) & 0x0F));
			break;

		case FS_FAT16 :
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 2)));
			if (res != FR_OK) break;
			p = &fs->win[clst * 2 % SS(fs)];
			ST_WORD(p, (WORD)val);
			break;

		case FS_FAT32 :
			res = move_window(fs, fs->fatbase + (clst / (SS(fs) / 4)));
			if (res != FR_OK) break;
			p = &fs->win[clst * 4 % SS(fs)];
			val |= LD_DWORD(p) & 0xF0000000;
			ST_DWORD(p, val);
			break;

		default :
			res = FR_INT_ERR;
		}
		fs->wflag = 1;
	}

	return res;
}
#endif /* !_FS_READONLY */




/*-----------------------------------------------------------------------*/
/* FAT handling - Remove a cluster chain                                 */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
/*删除clst开始的族链*/
static
FRESULT remove_chain (
	FATFS* fs,			/* File system object */
	DWORD clst			/* Cluster# to remove a chain from */
)
{
	FRESULT res;
	DWORD nxt;
#if _USE_ERASE
	DWORD scl = clst, ecl = clst, rt[2];
#endif

	if (clst < 2 || clst >= fs->n_fatent) {	/* Check range */
		res = FR_INT_ERR;

	} else {
		res = FR_OK;
		while (clst < fs->n_fatent) {			/* Not a last link? */
			nxt = get_fat(fs, clst);			/* Get cluster status */
			if (nxt == 0) break;				/* Empty cluster? */
			if (nxt == 1) { res = FR_INT_ERR; break; }	/* Internal error? */
			if (nxt == 0xFFFFFFFF) { res = FR_DISK_ERR; break; }	/* Disk error? */
			/*把当前族设置为空闲族*/
			res = put_fat(fs, clst, 0);			/* Mark the cluster "empty" */
			if (res != FR_OK) break;
			if (fs->free_clust != 0xFFFFFFFF) {	/* Update FSINFO */
				fs->free_clust++;
				fs->fsi_flag |= 1;
			}
#if _USE_ERASE
			if (ecl + 1 == nxt) {	/* Is next cluster contiguous? */
				ecl = nxt;
			} else {				/* End of contiguous clusters */ 
				rt[0] = clust2sect(fs, scl);					/* Start sector */
				rt[1] = clust2sect(fs, ecl) + fs->csize - 1;	/* End sector */
				disk_ioctl(fs->drv, CTRL_ERASE_SECTOR, rt);		/* Erase the block */
				scl = ecl = nxt;
			}
#endif
			clst = nxt;	/* Next cluster */
		}
	}

	return res;
}
#endif




/*-----------------------------------------------------------------------*/
/* FAT handling - Stretch or Create a cluster chain                      */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
/*当clst为0时，找出一个未分配的族，作为
链的第一个族头
当clst不会0时，clst代表当前族链的最后一个族，分配一个
新族，然后添加到clst，使新族成为族链尾巴*/
static
DWORD create_chain (	/* 0:No free cluster, 1:Internal error, 0xFFFFFFFF:Disk error, >=2:New cluster# */
	FATFS* fs,			/* File system object */
	DWORD clst			/* Cluster# to stretch. 0 means create a new chain. */
)
{
	DWORD cs, ncl, scl;
	FRESULT res;


	if (clst == 0) {		/* Create a new chain */
		printf("%s: create a new cluster chain for sub dir or file!\r\n", __func__);
		scl = fs->last_clust;			/* Get suggested start point */
		if (!scl || scl >= fs->n_fatent) scl = 1;
	}
	else {					/* Stretch the current chain */
		cs = get_fat(fs, clst);			/* Check the cluster status */
		if (cs < 2) return 1;			/* Invalid value */
		if (cs == 0xFFFFFFFF) return cs;	/* A disk error occurred */
		if (cs < fs->n_fatent) return cs;	/* It is already followed by next cluster */
		scl = clst;
	}

	ncl = scl;				/* Start cluster */
	for (;;) {
		ncl++;							/* Next cluster */
		if (ncl >= fs->n_fatent) {		/* Check wrap around */
			ncl = 2;
			if (ncl > scl) return 0;	/* No free cluster */
		}
		cs = get_fat(fs, ncl);			/* Get the cluster status */
		if (cs == 0) break;				/* Found a free cluster */
		if (cs == 0xFFFFFFFF || cs == 1)/* An error occurred */
			return cs;
		if (ncl == scl) return 0;		/* No free cluster */
	}

	res = put_fat(fs, ncl, 0x0FFFFFFF);	/* Mark the new cluster "last link" */
	if (res == FR_OK && clst != 0) {
		printf("%s: ###############################\r\n", __func__);
		printf("%s: ###############################Link cluster(%d)->(%d)\r\n", __func__, clst, ncl);
		res = put_fat(fs, clst, ncl);	/* Link it to the previous one if needed */
	}
	if (res == FR_OK) {
		fs->last_clust = ncl;			/* Update FSINFO */
		if (fs->free_clust != 0xFFFFFFFF) {
			fs->free_clust--;
			fs->fsi_flag |= 1;
		}
	} else {
		ncl = (res == FR_DISK_ERR) ? 0xFFFFFFFF : 1;
	}

	return ncl;		/* Return new cluster number or error code */
}
#endif /* !_FS_READONLY */




/*-----------------------------------------------------------------------*/
/* FAT handling - Convert offset into cluster with link map table        */
/*-----------------------------------------------------------------------*/

#if _USE_FASTSEEK
static
DWORD clmt_clust (	/* <2:Error, >=2:Cluster number */
	FIL* fp,		/* Pointer to the file object */
	DWORD ofs		/* File offset to be converted to cluster# */
)
{
	DWORD cl, ncl, *tbl;


	tbl = fp->cltbl + 1;	/* Top of CLMT */
	cl = ofs / SS(fp->fs) / fp->fs->csize;	/* Cluster order from top of the file */
	for (;;) {
		ncl = *tbl++;			/* Number of cluters in the fragment */
		if (!ncl) return 0;		/* End of table? (error) */
		if (cl < ncl) break;	/* In this fragment? */
		cl -= ncl; tbl++;		/* Next fragment */
	}
	return cl + *tbl;	/* Return the cluster number */
}
#endif	/* _USE_FASTSEEK */




/*-----------------------------------------------------------------------*/
/* Directory handling - Set directory index                              */
/*-----------------------------------------------------------------------*/
//static //让外部函数调用
/*根据dir的sclust获取到指写index的目录项的信息
也就是让dp->dir指向win缓存中的正确位置
还有设置了每index个目录项对应的dp->sect、dp->clust
可是此时win缓存可能还没有更新， 所以一般
都会再调用 一下move_window*/

/*找到目录下，第index个目录项，从0开始算
子目录下，第0个是".", 每1个是".."代表了父亲目录，
目录项里面的信息，包含了父亲目录的开始族
开始族为0时，代表到了root目录*/
FRESULT dir_sdi (
	DIR* dp,		/* Pointer to directory object */
	UINT idx		/* Index of directory table */
)
{
	DWORD clst, sect;
	UINT ic;

	//printf("in %s: dp->index(%d), dp->sect(%d), dp->sclust(%d)\r\n", 
	//	__func__, dp->index, dp->sect, dp->sclust);
	dp->index = (WORD)idx;	/* Current index */
	clst = dp->sclust;		/* Table start cluster (0:root) */
	if (clst == 1 || clst >= dp->fs->n_fatent)	/* Check start cluster range */
		return FR_INT_ERR;
	if (!clst && dp->fs->fs_type == FS_FAT32)	/* Replace cluster# 0 with root cluster# if in FAT32 */
		clst = dp->fs->dirbase;

	if (clst == 0) {	/* Static table (root-directory in FAT12/16) */
		if (idx >= dp->fs->n_rootdir)	/* Is index out of range? */
			return FR_INT_ERR;
		sect = dp->fs->dirbase;
	}
	else {				/* Dynamic table (root-directory in FAT32 or sub-directory) */
		ic = SS(dp->fs) / SZ_DIR * dp->fs->csize;	/* Entries per cluster */
		/*一个cluster有ic个目录项(代表目录跟文件)
		而文件的sclust代表文件内容的开始地址
		而子目录的sclust代表一颗新的目录树的开始
		遍历一个目录下的文件跟目录，就是从此目录
		的sclust开始遍历目录项，除了root下对目录项有限制，
		其它的子目录下族开始的目录项没有数目的限制*/
		while (idx >= ic) {	/* Follow cluster chain */
			clst = get_fat(dp->fs, clst);				/* Get next cluster */
			/*clst 就是族链中的下一个族，如果有效的话，
				就是因为这个子目录下有太多的目录项了，
				也就是说有太多的文件跟目录了*/
			if (clst == 0xFFFFFFFF) return FR_DISK_ERR;	/* Disk error */
			if (clst < 2 || clst >= dp->fs->n_fatent)	/* Reached to end of table or internal error */
				return FR_INT_ERR;
			idx -= ic;
		}
		sect = clust2sect(dp->fs, clst);
		//printf("%s: sub dir, clst(%d), sect=%d\r\n", __func__, clst, sect);
	}
	dp->clust = clst;	/* Current cluster# */
	if (!sect) return FR_INT_ERR;
	dp->sect = sect + idx / (SS(dp->fs) / SZ_DIR);					/* Sector# of the directory entry */
	/*当前目录项在所在扇区的偏移地址*/
	dp->dir = dp->fs->win + (idx % (SS(dp->fs) / SZ_DIR)) * SZ_DIR;	/* Ptr to the entry in the sector */
	//printf("out %s: dp->index(%d), dp->sect(%d)\r\n", 
	//	__func__, dp->index, dp->sect);

	return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Directory handling - Move directory table index next                  */
/*-----------------------------------------------------------------------*/

/*根据dp->sclust代表的目录，并且在第dp->index个
目录项的状态下，向后移动一个目录项，
stretch为0，代表超出一族时，报错误，
stretch为1时，代表当目录项的空间满一族后，
再分配多一族给目录的目录项*/
static
FRESULT dir_next (	/* FR_OK:Succeeded, FR_NO_FILE:End of table, FR_DENIED:Could not stretch */
	DIR* dp,		/* Pointer to the directory object */
	int stretch		/* 0: Do not stretch table, 1: Stretch table if needed */
)
{
	DWORD clst;
	UINT i;


	i = dp->index + 1;
	if (!(i & 0xFFFF) || !dp->sect)	/* Report EOT when index has reached 65535 */
		return FR_NO_FILE;

	if (!(i % (SS(dp->fs) / SZ_DIR))) {	/* Sector changed? */
		dp->sect++;					/* Next sector */
		//printf("%s: Sector changed, dp->sect = %d\r\n", __func__, dp->sect);
		if (!dp->clust) {		/* Static table */
			/*如果是在0族，也就是root目录族
			root目录族最多只能有目录项n_rootdir 个*/
			if (i >= dp->fs->n_rootdir)	/* Report EOT if it reached end of static table */
				return FR_NO_FILE;
		}
		else {					/* Dynamic table */
			/*如果不是在root目录下，而是在子目录下*/
			if (((i / (SS(dp->fs) / SZ_DIR)) & (dp->fs->csize - 1)) == 0) {	/* Cluster changed? */
				/*产生族的变化,子目录下文件太多*/
				printf("%s: in sub dir, and it dir has too much file and dir"	\
							"exzit an Cluster(has %d sector size)\r\n", __func__, dp->fs->csize);
				clst = get_fat(dp->fs, dp->clust);				/* Get next cluster */
				if (clst <= 1) return FR_INT_ERR;
				if (clst == 0xFFFFFFFF) return FR_DISK_ERR;
				/*判断是否超出了fat表所能表示的族*/
				if (clst >= dp->fs->n_fatent) {					/* If it reached end of dynamic table, */
					//判断已经是族链的尾端
#if !_FS_READONLY
					UINT c;
					if (!stretch) return FR_NO_FILE;			/* If do not stretch, report EOT */
					/*找出一个新的族，放到族链尾端*/
					clst = create_chain(dp->fs, dp->clust);		/* Stretch cluster chain */
					if (clst == 0) return FR_DENIED;			/* No free cluster */
					if (clst == 1) return FR_INT_ERR;
					if (clst == 0xFFFFFFFF) return FR_DISK_ERR;
					/* Clean-up stretched table */
					if (sync_window(dp->fs)) return FR_DISK_ERR;/* Flush disk access window */
					mem_set(dp->fs->win, 0, SS(dp->fs));		/* Clear window buffer */
					dp->fs->winsect = clust2sect(dp->fs, clst);	/* Cluster start sector */
					for (c = 0; c < dp->fs->csize; c++) {		/* Fill the new cluster with 0 */
						dp->fs->wflag = 1;
						if (sync_window(dp->fs)) return FR_DISK_ERR;
						dp->fs->winsect++;
					}
					dp->fs->winsect -= c;						/* Rewind window offset */
#else
					if (!stretch) return FR_NO_FILE;			/* If do not stretch, report EOT (this is to suppress warning) */
					return FR_NO_FILE;							/* Report EOT */
#endif
				}
				dp->clust = clst;				/* Initialize data for new cluster */
				dp->sect = clust2sect(dp->fs, clst);
				printf("%s: Cluster changed  dp->sect=%d\r\n", __func__, dp->sect);
			}
		}
	}

	dp->index = (WORD)i;	/* Current index */
	dp->dir = dp->fs->win + (i % (SS(dp->fs) / SZ_DIR)) * SZ_DIR;	/* Current entry in the window */

	return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Directory handling - Reserve directory entry                          */
/*-----------------------------------------------------------------------*/

#if !_FS_READONLY
/*在dp->sclust   所代表的目录下，分配nent个目录项*/
static
FRESULT dir_alloc (
	DIR* dp,	/* Pointer to the directory object */
	UINT nent	/* Number of contiguous entries to allocate (1-21) */
)
{
	FRESULT res;
	UINT n;


	res = dir_sdi(dp, 0);
	if (res == FR_OK) {
		n = 0;
		do {
			printf("%s: dp->dir = %p\r\n", __func__, dp->dir);
			res = move_window(dp->fs, dp->sect);
			if (res != FR_OK) break;
			if (dp->dir[0] == DDE || dp->dir[0] == 0) {	/* Is it a blank entry? */
				if (++n == nent) {
					printf("%s: dp->index = %d\r\n", __func__, dp->index);
					break;	/* A block of contiguous entries is found */
				}
			} else {
				n = 0;					/* Not a blank entry. Restart to search */
			}
			/*只有在分配 时，参数才传入1*/
			res = dir_next(dp, 1);		/* Next entry with table stretch enabled */
		} while (res == FR_OK);
	}
	if (res == FR_NO_FILE) res = FR_DENIED;	/* No directory entry to allocate */
	return res;
}
#endif




/*-----------------------------------------------------------------------*/
/* Directory handling - Load/Store start cluster number                  */
/*-----------------------------------------------------------------------*/

/*获取到子目录树的开始族
其中dir为子目录所在的目录项的指针*/
static
DWORD ld_clust (
	FATFS* fs,	/* Pointer to the fs object */
	BYTE* dir	/* Pointer to the directory entry */
)
{
	DWORD cl;
	/*目录项中保存的
	    文件数据开始的 cluster
	    
	    或者子目录的开始cluster */
	    
	/*root 目录项的cluster 为0
	/user/user1/ff.txt*/
	cl = LD_WORD(dir+DIR_FstClusLO);
	if (fs->fs_type == FS_FAT32)
		cl |= (DWORD)LD_WORD(dir+DIR_FstClusHI) << 16;

	return cl;
}


#if !_FS_READONLY
static
void st_clust (
	BYTE* dir,	/* Pointer to the directory entry */
	DWORD cl	/* Value to be set */
)
{
	ST_WORD(dir+DIR_FstClusLO, cl);
	ST_WORD(dir+DIR_FstClusHI, cl >> 16);
}
#endif




/*-----------------------------------------------------------------------*/
/* LFN handling - Test/Pick/Fit an LFN segment from/to directory entry   */
/*-----------------------------------------------------------------------*/
#if _USE_LFN
static
const BYTE LfnOfs[] = {1,3,5,7,9,14,16,18,20,22,24,28,30};	/* Offset of LFN characters in the directory entry */


static
int cmp_lfn (			/* 1:Matched, 0:Not matched */
	WCHAR* lfnbuf,		/* Pointer to the LFN to be compared */
	BYTE* dir			/* Pointer to the directory entry containing a part of LFN */
)
{
	UINT i, s;
	WCHAR wc, uc;


	i = ((dir[LDIR_Ord] & ~LLE) - 1) * 13;	/* Get offset in the LFN buffer */
	s = 0; wc = 1;
	do {
		uc = LD_WORD(dir+LfnOfs[s]);	/* Pick an LFN character from the entry */
		if (wc) {	/* Last character has not been processed */
			wc = ff_wtoupper(uc);		/* Convert it to upper case */
			if (i >= _MAX_LFN || wc != ff_wtoupper(lfnbuf[i++]))	/* Compare it */
				return 0;				/* Not matched */
		} else {
			if (uc != 0xFFFF) return 0;	/* Check filler */
		}
	} while (++s < 13);				/* Repeat until all characters in the entry are checked */

	if ((dir[LDIR_Ord] & LLE) && wc && lfnbuf[i])	/* Last segment matched but different length */
		return 0;

	return 1;						/* The part of LFN matched */
}



static
int pick_lfn (			/* 1:Succeeded, 0:Buffer overflow */
	WCHAR* lfnbuf,		/* Pointer to the Unicode-LFN buffer */
	BYTE* dir			/* Pointer to the directory entry */
)
{
	UINT i, s;
	WCHAR wc, uc;


	i = ((dir[LDIR_Ord] & 0x3F) - 1) * 13;	/* Offset in the LFN buffer */

	s = 0; wc = 1;
	do {
		uc = LD_WORD(dir+LfnOfs[s]);		/* Pick an LFN character from the entry */
		if (wc) {	/* Last character has not been processed */
			if (i >= _MAX_LFN) return 0;	/* Buffer overflow? */
			lfnbuf[i++] = wc = uc;			/* Store it */
		} else {
			if (uc != 0xFFFF) return 0;		/* Check filler */
		}
	} while (++s < 13);						/* Read all character in the entry */

	if (dir[LDIR_Ord] & LLE) {				/* Put terminator if it is the last LFN part */
		if (i >= _MAX_LFN) return 0;		/* Buffer overflow? */
		lfnbuf[i] = 0;
	}

	return 1;
}


#if !_FS_READONLY
static
void fit_lfn (
	const WCHAR* lfnbuf,	/* Pointer to the LFN buffer */
	BYTE* dir,				/* Pointer to the directory entry */
	BYTE ord,				/* LFN order (1-20) */
	BYTE sum				/* SFN sum */
)
{
	UINT i, s;
	WCHAR wc;


	dir[LDIR_Chksum] = sum;			/* Set check sum */
	dir[LDIR_Attr] = AM_LFN;		/* Set attribute. LFN entry */
	dir[LDIR_Type] = 0;
	ST_WORD(dir+LDIR_FstClusLO, 0);

	i = (ord - 1) * 13;				/* Get offset in the LFN buffer */
	s = wc = 0;
	do {
		if (wc != 0xFFFF) wc = lfnbuf[i++];	/* Get an effective character */
		ST_WORD(dir+LfnOfs[s], wc);	/* Put it */
		if (!wc) wc = 0xFFFF;		/* Padding characters following last character */
	} while (++s < 13);
	if (wc == 0xFFFF || !lfnbuf[i]) ord |= LLE;	/* Bottom LFN part is the start of LFN sequence */
	dir[LDIR_Ord] = ord;			/* Set the LFN order */
}

#endif
#endif




/*-----------------------------------------------------------------------*/
/* Create numbered name                                                  */
/*-----------------------------------------------------------------------*/
#if _USE_LFN
static
void gen_numname (
	BYTE* dst,			/* Pointer to the buffer to store numbered SFN */
	const BYTE* src,	/* Pointer to SFN */
	const WCHAR* lfn,	/* Pointer to LFN */
	UINT seq			/* Sequence number */
)
{
	BYTE ns[8], c;
	UINT i, j;


	mem_cpy(dst, src, 11);

	if (seq > 5) {	/* On many collisions, generate a hash number instead of sequential number */
		WCHAR wc;
		DWORD sr = seq;

		while (*lfn) {	/* Create a CRC */
			wc = *lfn++;
			for (i = 0; i < 16; i++) {
				sr = (sr << 1) + (wc & 1);
				wc >>= 1;
				if (sr & 0x10000) sr ^= 0x11021;
			}
		}
		seq = (UINT)sr;
	}

	/* itoa (hexdecimal) */
	i = 7;
	do {
		c = (seq % 16) + '0';
		if (c > '9') c += 7;
		ns[i--] = c;
		seq /= 16;
	} while (seq);
	ns[i] = '~';

	/* Append the number */
	for (j = 0; j < i && dst[j] != ' '; j++) {
		if (IsDBCS1(dst[j])) {
			if (j == i - 1) break;
			j++;
		}
	}
	do {
		dst[j++] = (i < 8) ? ns[i++] : ' ';
	} while (j < 8);
}
#endif




/*-----------------------------------------------------------------------*/
/* Calculate sum of an SFN                                               */
/*-----------------------------------------------------------------------*/
#if _USE_LFN
static
BYTE sum_sfn (
	const BYTE* dir		/* Pointer to the SFN entry */
)
{
	BYTE sum = 0;
	UINT n = 11;

	do sum = (sum >> 1) + (sum << 7) + *dir++; while (--n);
	return sum;
}
#endif




/*-----------------------------------------------------------------------*/
/* Directory handling - Find an object in the directory                  */
/*-----------------------------------------------------------------------*/

/*根据文件名，从flash读取dir结构结构

根据dp->sclust代表的当前目录下，找到dp->fn为名字
的文件、目录的目录项，原理就是利用move_window把
目录项信息，读取到FATFS win缓存中，从32个bytes中
对比dp->fn是否一样，一样就中了，
重要的就是dir指针，指向了目录项信息地址*/
static
FRESULT dir_find (
	DIR* dp			/* Pointer to the directory object linked to the file name */
)
{
	FRESULT res;
	BYTE c, *dir;
#if _USE_LFN
	BYTE a, ord, sum;
#endif

	//printf("%s: need to find file name = %s, dp->sect=%d\r\n", __func__, dp->fn, dp->sect);
	/* Set directory index */
	/*从第0个目录表项开始找*/
	res = dir_sdi(dp, 0);			/* Rewind directory object */
	if (res != FR_OK) return res;

#if _USE_LFN
	ord = sum = 0xFF; dp->lfn_idx = 0xFFFF;	/* Reset LFN sequence */
#endif
	do {
		/*读取sector的数据，如果sector有变化，*/
		res = move_window(dp->fs, dp->sect);
		if (res != FR_OK) break;
		dir = dp->dir;					/* Ptr to the directory entry of current index */
		c = dir[DIR_Name];
		if (c == 0) { 
			/*dp->sclust代表的当前目录下已经没有其它
				目录项了*/
			res = FR_NO_FILE; 
			printf("%s: Reached to end of table Directory! not find (%s) file or dir\r\n", 
				__func__, dp->fn);
			break; 
		}	/* Reached to end of table */
#if _USE_LFN	/* LFN configuration */
		a = dir[DIR_Attr] & AM_MASK;
		if (c == DDE || ((a & AM_VOL) && a != AM_LFN)) {	/* An entry without valid data */
			ord = 0xFF; dp->lfn_idx = 0xFFFF;	/* Reset LFN sequence */
		} else {
			if (a == AM_LFN) {			/* An LFN entry is found */
				if (dp->lfn) {
					if (c & LLE) {		/* Is it start of LFN sequence? */
						sum = dir[LDIR_Chksum];
						c &= ~LLE; ord = c;	/* LFN start order */
						dp->lfn_idx = dp->index;	/* Start index of LFN */
					}
					/* Check validity of the LFN entry and compare it with given name */
					ord = (c == ord && sum == dir[LDIR_Chksum] && cmp_lfn(dp->lfn, dir)) ? ord - 1 : 0xFF;
				}
			} else {					/* An SFN entry is found */
				if (!ord && sum == sum_sfn(dir)) break;	/* LFN matched? */
				if (!(dp->fn[NS] & NS_LOSS) && !mem_cmp(dir, dp->fn, 11)) break;	/* SFN matched? */
				ord = 0xFF; dp->lfn_idx = 0xFFFF;	/* Reset LFN sequence */
			}
		}
#else		/* Non LFN configuration */
		if (!(dir[DIR_Attr] & AM_VOL) && !mem_cmp(dir, dp->fn, 11)) /* Is it a valid entry? */
		{
			//printf("%s, find the file or dir scuess!\r\n", __func__);
			//printf("%s: out, dp->fn(%s), dp->sect=%d\r\n", 
			//	__func__, dp->fn, dp->sect);			
			break;
		}
#endif
		/*改变sector , 如果目录项的移动超出当前sector*/
		res = dir_next(dp, 0);		/* Next entry */
		/*dir_next(dp, 0) 中的参数0，因为我们只是查询
		目录下的目录项，遍历它，所以不为1，
		在创建目录时，用dir_next(dp, 1)*/
	} while (res == FR_OK);

	return res;
}




/*-----------------------------------------------------------------------*/
/* Read an object from the directory                                     */
/*-----------------------------------------------------------------------*/
#if _FS_MINIMIZE <= 1 || _USE_LABEL || _FS_RPATH >= 2
/*读dp->sect扇区中一个一个的目录项，读到一个存在的
目录项，或者读不到，都break然后返回*/
static
FRESULT dir_read (
	DIR* dp,		/* Pointer to the directory object */
	int vol			/* Filtered by 0:file/directory or 1:volume label */
)
{
	FRESULT res;
	BYTE a, c, *dir;
#if _USE_LFN
	BYTE ord = 0xFF, sum = 0xFF;
#endif

	res = FR_NO_FILE;
	while (dp->sect) {
		res = move_window(dp->fs, dp->sect);
		if (res != FR_OK) break;
		dir = dp->dir;					/* Ptr to the directory entry of current index */
		c = dir[DIR_Name];
		if (c == 0) { res = FR_NO_FILE; break; }	/* Reached to end of table */
		a = dir[DIR_Attr] & AM_MASK;
#if _USE_LFN	/* LFN configuration */
		if (c == DDE || (!_FS_RPATH && c == '.') || (int)(a == AM_VOL) != vol) {	/* An entry without valid data */
			ord = 0xFF;
		} else {
			if (a == AM_LFN) {			/* An LFN entry is found */
				if (c & LLE) {			/* Is it start of LFN sequence? */
					sum = dir[LDIR_Chksum];
					c &= ~LLE; ord = c;
					dp->lfn_idx = dp->index;
				}
				/* Check LFN validity and capture it */
				ord = (c == ord && sum == dir[LDIR_Chksum] && pick_lfn(dp->lfn, dir)) ? ord - 1 : 0xFF;
			} else {					/* An SFN entry is found */
				if (ord || sum != sum_sfn(dir))	/* Is there a valid LFN? */
					dp->lfn_idx = 0xFFFF;		/* It has no LFN. */
				break;
			}
		}
#else		/* Non LFN configuration */
		if (c != DDE && (_FS_RPATH || c != '.') && a != AM_LFN && (int)(a == AM_VOL) == vol)	/* Is it a valid entry? */
		{
			//printf("%s: get an dir or file: (%s)\r\n", __func__, (char *)dp->dir[DIR_Name]);
			break;
		}
#endif
		res = dir_next(dp, 0);				/* Next entry */
		if (res != FR_OK) break;
	}

	if (res != FR_OK) dp->sect = 0;

	return res;
}
#endif	/* _FS_MINIMIZE <= 1 || _USE_LABEL || _FS_RPATH >= 2 */




/*-----------------------------------------------------------------------*/
/* Register an object to the directory                                   */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY
/*给dp->fn指定的文件名，在dp->sclust所代表的目录下，
分配一个新的目录项给此文件，并且把文件的名字，
写入到缓存中，标志回写，等待目录项信息被回写到
flash中
dp->fn也可能是一个目录名
*/
static
FRESULT dir_register (	/* FR_OK:Successful, FR_DENIED:No free entry or too many SFN collision, FR_DISK_ERR:Disk error */
	DIR* dp				/* Target directory with object name to be created */
)
{
	FRESULT res;

#if _USE_LFN	/* LFN configuration */
	UINT n, nent;
	BYTE sn[12], *fn, sum;
	WCHAR *lfn;


	fn = dp->fn; lfn = dp->lfn;
	mem_cpy(sn, fn, 12);

	if (_FS_RPATH && (sn[NS] & NS_DOT))		/* Cannot create dot entry */
		return FR_INVALID_NAME;

	if (sn[NS] & NS_LOSS) {			/* When LFN is out of 8.3 format, generate a numbered name */
		fn[NS] = 0; dp->lfn = 0;			/* Find only SFN */
		for (n = 1; n < 100; n++) {
			gen_numname(fn, sn, lfn, n);	/* Generate a numbered name */
			res = dir_find(dp);				/* Check if the name collides with existing SFN */
			if (res != FR_OK) break;
		}
		if (n == 100) return FR_DENIED;		/* Abort if too many collisions */
		if (res != FR_NO_FILE) return res;	/* Abort if the result is other than 'not collided' */
		fn[NS] = sn[NS]; dp->lfn = lfn;
	}

	if (sn[NS] & NS_LFN) {			/* When LFN is to be created, allocate entries for an SFN + LFNs. */
		for (n = 0; lfn[n]; n++) ;
		nent = (n + 25) / 13;
	} else {						/* Otherwise allocate an entry for an SFN  */
		nent = 1;
	}
	res = dir_alloc(dp, nent);		/* Allocate entries */

	if (res == FR_OK && --nent) {	/* Set LFN entry if needed */
		res = dir_sdi(dp, dp->index - nent);
		if (res == FR_OK) {
			sum = sum_sfn(dp->fn);	/* Sum value of the SFN tied to the LFN */
			do {					/* Store LFN entries in bottom first */
				res = move_window(dp->fs, dp->sect);
				if (res != FR_OK) break;
				fit_lfn(dp->lfn, dp->dir, (BYTE)nent, sum);
				dp->fs->wflag = 1;
				res = dir_next(dp, 0);	/* Next entry */
			} while (res == FR_OK && --nent);
		}
	}
#else	/* Non LFN configuration */
	printf("%s: dp->sclust(%d)\r\n", __func__, dp->sclust);	
	/*分配一个目录项空间，也就是32bytes
		在此时dp所代表的当前目录下*/
	res = dir_alloc(dp, 1);		/* Allocate an entry for SFN */
#endif

	if (res == FR_OK) {				/* Set SFN entry */
		res = move_window(dp->fs, dp->sect);
		if (res == FR_OK) {
			/*清除分配到的32bytes目录项内容*/
			mem_set(dp->dir, 0, SZ_DIR);	/* Clean the entry */
			/*拷贝文件名字到目录项空间中*/
			mem_cpy(dp->dir, dp->fn, 11);	/* Put SFN */
#if _USE_LFN
			dp->dir[DIR_NTres] = dp->fn[NS] & (NS_BODY | NS_EXT);	/* Put NT flag */
#endif
			/*目录项信息被污染，请求回写*/
			dp->fs->wflag = 1;
		}
	}

	return res;
}
#endif /* !_FS_READONLY */




/*-----------------------------------------------------------------------*/
/* Remove an object from the directory                                   */
/*-----------------------------------------------------------------------*/
#if !_FS_READONLY && !_FS_MINIMIZE
static
FRESULT dir_remove (	/* FR_OK: Successful, FR_DISK_ERR: A disk error */
	DIR* dp				/* Directory object pointing the entry to be removed */
)
{
	FRESULT res;
#if _USE_LFN	/* LFN configuration */
	UINT i;

	i = dp->index;	/* SFN index */
	res = dir_sdi(dp, (dp->lfn_idx == 0xFFFF) ? i : dp->lfn_idx);	/* Goto the SFN or top of the LFN entries */
	if (res == FR_OK) {
		do {
			res = move_window(dp->fs, dp->sect);
			if (res != FR_OK) break;
			mem_set(dp->dir, 0, SZ_DIR);	/* Clear and mark the entry "deleted" */
			*dp->dir = DDE;
			dp->fs->wflag = 1;
			if (dp->index >= i) break;	/* When reached SFN, all entries of the object has been deleted. */
			res = dir_next(dp, 0);		/* Next entry */
		} while (res == FR_OK);
		if (res == FR_NO_FILE) res = FR_INT_ERR;
	}

#else			/* Non LFN configuration */
	res = dir_sdi(dp, dp->index);
	if (res == FR_OK) {
		res = move_window(dp->fs, dp->sect);
		if (res == FR_OK) {
			mem_set(dp->dir, 0, SZ_DIR);	/* Clear and mark the entry "deleted" */
			/*在目录项的第一个字节放0xE5
			用于标示一个被删除的目录项信息*/
			*dp->dir = DDE;
			dp->fs->wflag = 1;
		}
	}
#endif

	return res;
}
#endif /* !_FS_READONLY */




/*-----------------------------------------------------------------------*/
/* Get file information from directory entry                             */
/*-----------------------------------------------------------------------*/
#if _FS_MINIMIZE <= 1 || _FS_RPATH >= 2
static
void get_fileinfo (		/* No return code */
	DIR* dp,			/* Pointer to the directory object */
	FILINFO* fno	 	/* Pointer to the file information to be filled */
)
{
	UINT i;
	TCHAR *p, c;


	p = fno->fname;
	if (dp->sect) {		/* Get SFN */
		BYTE *dir = dp->dir;

		i = 0;
		while (i < 11) {		/* Copy name body and extension */
			c = (TCHAR)dir[i++];
			if (c == ' ') continue;			/* Skip padding spaces */
			if (c == NDDE) c = (TCHAR)DDE;	/* Restore replaced DDE character */
			if (i == 9) *p++ = '.';			/* Insert a . if extension is exist */
#if _USE_LFN
			if (IsUpper(c) && (dir[DIR_NTres] & (i >= 9 ? NS_EXT : NS_BODY)))
				c += 0x20;			/* To lower */
#if _LFN_UNICODE
			if (IsDBCS1(c) && i != 8 && i != 11 && IsDBCS2(dir[i]))
				c = c << 8 | dir[i++];
			c = ff_convert(c, 1);	/* OEM -> Unicode */
			if (!c) c = '?';
#endif
#endif
			*p++ = c;
		}
		fno->fattrib = dir[DIR_Attr];				/* Attribute */
		fno->fsize = LD_DWORD(dir+DIR_FileSize);	/* Size */
		fno->fdate = LD_WORD(dir+DIR_WrtDate);		/* Date */
		fno->ftime = LD_WORD(dir+DIR_WrtTime);		/* Time */
	}
	*p = 0;		/* Terminate SFN string by a \0 */

#if _USE_LFN
	if (fno->lfname) {
		WCHAR w, *lfn;

		i = 0; p = fno->lfname;
		if (dp->sect && fno->lfsize && dp->lfn_idx != 0xFFFF) {	/* Get LFN if available */
			lfn = dp->lfn;
			while ((w = *lfn++) != 0) {		/* Get an LFN character */
#if !_LFN_UNICODE
				w = ff_convert(w, 0);		/* Unicode -> OEM */
				if (!w) { i = 0; break; }	/* No LFN if it could not be converted */
				if (_DF1S && w >= 0x100)	/* Put 1st byte if it is a DBC (always false on SBCS cfg) */
					p[i++] = (TCHAR)(w >> 8);
#endif
				if (i >= fno->lfsize - 1) { i = 0; break; }	/* No LFN if buffer overflow */
				p[i++] = (TCHAR)w;
			}
		}
		p[i] = 0;	/* Terminate LFN string by a \0 */
	}
#endif
}
#endif /* _FS_MINIMIZE <= 1 || _FS_RPATH >= 2*/




/*-----------------------------------------------------------------------*/
/* Pick a segment and create the object name in directory form           */
/*-----------------------------------------------------------------------*/

static
FRESULT create_name (
	DIR* dp,			/* Pointer to the directory object */
	const TCHAR** path	/* Pointer to pointer to the segment in the path string */
)
{
#if _USE_LFN	/* LFN configuration */
	BYTE b, cf;
	WCHAR w, *lfn;
	UINT i, ni, si, di;
	const TCHAR *p;

	/* Create LFN in Unicode */
	for (p = *path; *p == '/' || *p == '\\'; p++) ;	/* Strip duplicated separator */
	lfn = dp->lfn;
	si = di = 0;
	for (;;) {
		w = p[si++];					/* Get a character */
		if (w < ' ' || w == '/' || w == '\\') break;	/* Break on end of segment */
		if (di >= _MAX_LFN)				/* Reject too long name */
			return FR_INVALID_NAME;
#if !_LFN_UNICODE
		w &= 0xFF;
		if (IsDBCS1(w)) {				/* Check if it is a DBC 1st byte (always false on SBCS cfg) */
			b = (BYTE)p[si++];			/* Get 2nd byte */
			if (!IsDBCS2(b))
				return FR_INVALID_NAME;	/* Reject invalid sequence */
			w = (w << 8) + b;			/* Create a DBC */
		}
		w = ff_convert(w, 1);			/* Convert ANSI/OEM to Unicode */
		if (!w) return FR_INVALID_NAME;	/* Reject invalid code */
#endif
		if (w < 0x80 && chk_chr("\"*:<>\?|\x7F", w)) /* Reject illegal characters for LFN */
			return FR_INVALID_NAME;
		lfn[di++] = w;					/* Store the Unicode character */
	}
	*path = &p[si];						/* Return pointer to the next segment */
	cf = (w < ' ') ? NS_LAST : 0;		/* Set last segment flag if end of path */
#if _FS_RPATH
	if ((di == 1 && lfn[di-1] == '.') || /* Is this a dot entry? */
		(di == 2 && lfn[di-1] == '.' && lfn[di-2] == '.')) {
		lfn[di] = 0;
		for (i = 0; i < 11; i++)
			dp->fn[i] = (i < di) ? '.' : ' ';
		dp->fn[i] = cf | NS_DOT;		/* This is a dot entry */
		return FR_OK;
	}
#endif
	while (di) {						/* Strip trailing spaces and dots */
		w = lfn[di-1];
		if (w != ' ' && w != '.') break;
		di--;
	}
	if (!di) return FR_INVALID_NAME;	/* Reject nul string */

	lfn[di] = 0;						/* LFN is created */

	/* Create SFN in directory form */
	mem_set(dp->fn, ' ', 11);
	for (si = 0; lfn[si] == ' ' || lfn[si] == '.'; si++) ;	/* Strip leading spaces and dots */
	if (si) cf |= NS_LOSS | NS_LFN;
	while (di && lfn[di - 1] != '.') di--;	/* Find extension (di<=si: no extension) */

	b = i = 0; ni = 8;
	for (;;) {
		w = lfn[si++];					/* Get an LFN character */
		if (!w) break;					/* Break on end of the LFN */
		if (w == ' ' || (w == '.' && si != di)) {	/* Remove spaces and dots */
			cf |= NS_LOSS | NS_LFN; continue;
		}

		if (i >= ni || si == di) {		/* Extension or end of SFN */
			if (ni == 11) {				/* Long extension */
				cf |= NS_LOSS | NS_LFN; break;
			}
			if (si != di) cf |= NS_LOSS | NS_LFN;	/* Out of 8.3 format */
			if (si > di) break;			/* No extension */
			si = di; i = 8; ni = 11;	/* Enter extension section */
			b <<= 2; continue;
		}

		if (w >= 0x80) {				/* Non ASCII character */
#ifdef _EXCVT
			w = ff_convert(w, 0);		/* Unicode -> OEM code */
			if (w) w = ExCvt[w - 0x80];	/* Convert extended character to upper (SBCS) */
#else
			w = ff_convert(ff_wtoupper(w), 0);	/* Upper converted Unicode -> OEM code */
#endif
			cf |= NS_LFN;				/* Force create LFN entry */
		}

		if (_DF1S && w >= 0x100) {		/* Double byte character (always false on SBCS cfg) */
			if (i >= ni - 1) {
				cf |= NS_LOSS | NS_LFN; i = ni; continue;
			}
			dp->fn[i++] = (BYTE)(w >> 8);
		} else {						/* Single byte character */
			if (!w || chk_chr("+,;=[]", w)) {	/* Replace illegal characters for SFN */
				w = '_'; cf |= NS_LOSS | NS_LFN;/* Lossy conversion */
			} else {
				if (IsUpper(w)) {		/* ASCII large capital */
					b |= 2;
				} else {
					if (IsLower(w)) {	/* ASCII small capital */
						b |= 1; w -= 0x20;
					}
				}
			}
		}
		dp->fn[i++] = (BYTE)w;
	}

	if (dp->fn[0] == DDE) dp->fn[0] = NDDE;	/* If the first character collides with deleted mark, replace it with 0x05 */

	if (ni == 8) b <<= 2;
	if ((b & 0x0C) == 0x0C || (b & 0x03) == 0x03)	/* Create LFN entry when there are composite capitals */
		cf |= NS_LFN;
	if (!(cf & NS_LFN)) {						/* When LFN is in 8.3 format without extended character, NT flags are created */
		if ((b & 0x03) == 0x01) cf |= NS_EXT;	/* NT flag (Extension has only small capital) */
		if ((b & 0x0C) == 0x04) cf |= NS_BODY;	/* NT flag (Filename has only small capital) */
	}

	dp->fn[NS] = cf;	/* SFN is created */

	return FR_OK;


#else	/* Non-LFN configuration */
	BYTE b, c, d, *sfn;
	UINT ni, si, i;
	const char *p;

	/* Create file name in directory form */
	for (p = *path; *p == '/' || *p == '\\'; p++) ;	/* Strip duplicated separator */
	sfn = dp->fn;
	mem_set(sfn, ' ', 11);
	si = i = b = 0; ni = 8;
#if _FS_RPATH
	if (p[si] == '.') { /* Is this a dot entry? */
		for (;;) {
			c = (BYTE)p[si++];
			if (c != '.' || si >= 3) break;
			sfn[i++] = c;
		}
		if (c != '/' && c != '\\' && c > ' ') return FR_INVALID_NAME;
		*path = &p[si];									/* Return pointer to the next segment */
		sfn[NS] = (c <= ' ') ? NS_LAST | NS_DOT : NS_DOT;	/* Set last segment flag if end of path */
		return FR_OK;
	}
#endif
	for (;;) {
		c = (BYTE)p[si++];
		if (c <= ' ' || c == '/' || c == '\\') break;	/* Break on end of segment */
		if (c == '.' || i >= ni) {
			if (ni != 8 || c != '.') return FR_INVALID_NAME;
			i = 8; ni = 11;
			b <<= 2; continue;
		}
		if (c >= 0x80) {				/* Extended character? */
			b |= 3;						/* Eliminate NT flag */
#ifdef _EXCVT
			c = ExCvt[c - 0x80];		/* To upper extended characters (SBCS cfg) */
#else
#if !_DF1S
			return FR_INVALID_NAME;		/* Reject extended characters (ASCII cfg) */
#endif
#endif
		}
		if (IsDBCS1(c)) {				/* Check if it is a DBC 1st byte (always false on SBCS cfg) */
			d = (BYTE)p[si++];			/* Get 2nd byte */
			if (!IsDBCS2(d) || i >= ni - 1)	/* Reject invalid DBC */
				return FR_INVALID_NAME;
			sfn[i++] = c;
			sfn[i++] = d;
		} else {						/* Single byte code */
			if (chk_chr("\"*+,:;<=>\?[]|\x7F", c))	/* Reject illegal chrs for SFN */
				return FR_INVALID_NAME;
			if (IsUpper(c)) {			/* ASCII large capital? */
				b |= 2;
			} else {
				if (IsLower(c)) {		/* ASCII small capital? */
					b |= 1; c -= 0x20;
				}
			}
			sfn[i++] = c;
		}
	}
	*path = &p[si];						/* Return pointer to the next segment */
	c = (c <= ' ') ? NS_LAST : 0;		/* Set last segment flag if end of path */

	if (!i) return FR_INVALID_NAME;		/* Reject nul string */
	if (sfn[0] == DDE) sfn[0] = NDDE;	/* When first character collides with DDE, replace it with 0x05 */

	if (ni == 8) b <<= 2;
	if ((b & 0x03) == 0x01) c |= NS_EXT;	/* NT flag (Name extension has only small capital) */
	if ((b & 0x0C) == 0x04) c |= NS_BODY;	/* NT flag (Name body has only small capital) */

	sfn[NS] = c;		/* Store NT flag, File name is created */

	return FR_OK;
#endif
}




/*-----------------------------------------------------------------------*/
/* Follow a file path                                                    */
/*-----------------------------------------------------------------------*/

/*从patch找到最后一个目录，或者文件的目录项
信息，并把信息放到dp中，重点是返回值跟dp->dir

返回后，如果找到文件or目录，dp->sclust代表的是
文件or目录的父亲目录的族，如果是父亲目录是
root，dp->sclust = 0*/
static
FRESULT follow_path (	/* FR_OK(0): successful, !=0: error code */
	DIR* dp,			/* Directory object to return last directory and found object */
	const TCHAR* path	/* Full-path string to find a file or directory */
)
{
	FRESULT res;
	BYTE *dir, ns;

	//printf("%s: path = %s\r\n", __func__, path);
#if _FS_RPATH
	if (*path == '/' || *path == '\\') {	/* There is a heading separator */
		/*如果是根目录*/
		path++;	dp->sclust = 0;				/* Strip it and start from the root directory */
		//printf("%s: (have root dir /) dp->sclust=%d\r\n", __func__, dp->sclust);		
	} else {								/* No heading separator */
		/*如果不是根目录*/
		dp->sclust = dp->fs->cdir;			/* Start from the current directory */
		//printf("%s: (!!!not have root dir /) dp->sclust=%d\r\n", __func__, dp->sclust);
	}
#else
	if (*path == '/' || *path == '\\')		/* Strip heading separator if exist */
		path++;
	dp->sclust = 0;							/* Always start from the root directory */
#endif

	if ((UINT)*path < ' ') {				/* Null path name is the origin directory itself */
		/*只有根目录*/
		//printf("%s: just /\r\n", __func__);
		res = dir_sdi(dp, 0);
		dp->dir = 0;
	} else {								/* Follow path */
		//	USER/yang.txt
		//printf("%s: path = %s\r\n", __func__, path);
		for (;;) {
			/*格式化出一个文件or目录名,放在dp->fn
				同时修改path(不断的减少最前的目录)*/
			res = create_name(dp, &path);	/* Get a segment name of the path */
			// 			path = yang.txt, dp->fn=USER
			//printf("%s: create_name after, path(%s), dp->fn(%s)\r\n", 
			//	__func__, path, dp->fn);
			if (res != FR_OK) break;
			res = dir_find(dp);				/* Find an object with the sagment name */
			ns = dp->fn[NS];
			if (res != FR_OK) {				/* Failed to find the object */
				if (res == FR_NO_FILE) {	/* Object is not found */
					if (_FS_RPATH && (ns & NS_DOT)) {	/* If dot entry is not exist, */
						/*根目录下没有"." 目录*/
						dp->sclust = 0; dp->dir = 0;	/* it is the root directory and stay there */
						if (!(ns & NS_LAST)) continue;	/* Continue to follow if not last segment */
						res = FR_OK;					/* Ended at the root directroy. Function completed. */
					} else {							/* Could not find the object */
						if (!(ns & NS_LAST)) res = FR_NO_PATH;	/* Adjust error code if not last segment */
					}
				}
				//printf("%s: Failed to find the object File or Dir\r\n", __func__);				
				break;
			}
			if (ns & NS_LAST) {
				/*到了寻找路径的最后一个文件或者目录了*/
				//printf("%s: Last segment matched. Function completed. \r\n", __func__);
				break;			/* Last segment matched. Function completed. */
			}
			dir = dp->dir;						/* Follow the sub-directory */
			if (!(dir[DIR_Attr] & AM_DIR)) {	/* It is not a sub-directory and cannot follow */
				res = FR_NO_PATH; break;
			}
			/*开始遍历下一个子目录的开始族*/
			dp->sclust = ld_clust(dp->fs, dir);
			//printf("%s: *** dp->sclust=%d\r\n", __func__, dp->sclust);
		}
	}

	return res;
}

/*
	root 的族为0， root目录 下的目录user ，其目录项位于
	root所在的族0中，user的族信息从目录项中得到，也就是
	子目录user也有自己的族，user子目录下的文件目录项就是
	保存在user子目录所在的开始族中。

	dp->sclust代表目录所在的开始族Table start cluster (0:Root dir) 。
*/


/*-----------------------------------------------------------------------*/
/* Get logical drive number from path name                               */
/*-----------------------------------------------------------------------*/

static
int get_ldnumber (		/* Returns logical drive number (-1:invalid drive) */
	const TCHAR** path	/* Pointer to pointer to the path name */
)
{
	const TCHAR *tp, *tt;
	UINT i;
	int vol = -1;


	if (*path) {	/* If the pointer is not a null */
		for (tt = *path; (UINT)*tt >= (_USE_LFN ? ' ' : '!') && *tt != ':'; tt++) ;	/* Find ':' in the path */
		if (*tt == ':') {	/* If a ':' is exist in the path name */
			tp = *path;
			i = *tp++ - '0'; 
			if (i < 10 && tp == tt) {	/* Is there a numeric drive id? */
				if (i < _VOLUMES) {	/* If a drive id is found, get the value and strip it */
					vol = (int)i;
					*path = ++tt;
				}
			} else {	/* No numeric drive number */
#if _STR_VOLUME_ID		/* Find string drive id */
				static const char* const str[] = {_VOLUME_STRS};
				const char *sp;
				char c;
				TCHAR tc;

				i = 0; tt++;
				do {
					sp = str[i]; tp = *path;
					do {	/* Compare a string drive id with path name */
						c = *sp++; tc = *tp++;
						if (IsLower(tc)) tc -= 0x20;
					} while (c && (TCHAR)c == tc);
				} while ((c || tp != tt) && ++i < _VOLUMES);	/* Repeat for each id until pattern match */
				if (i < _VOLUMES) {	/* If a drive id is found, get the value and strip it */
					vol = (int)i;
					*path = tt;
				}
#endif
			}
			return vol;
		}
#if _FS_RPATH && _VOLUMES >= 2
		vol = CurrVol;	/* Current drive */
#else
		vol = 0;		/* Drive 0 */
#endif
	}
	return vol;
}




/*-----------------------------------------------------------------------*/
/* Load a sector and check if it is an FAT boot sector                   */
/*-----------------------------------------------------------------------*/
/*load sect then check the VBR*/
static
BYTE check_fs (	/* 0:FAT boor sector, 1:Valid boor sector but not FAT, 2:Not a boot sector, 3:Disk error */
	FATFS* fs,	/* File system object */
	DWORD sect	/* Sector# (lba) to check if it is an FAT boot record or not */
)
{
	fs->wflag = 0; fs->winsect = 0xFFFFFFFF;	/* Invaidate window */
	if (move_window(fs, sect) != FR_OK)			/* Load boot record */
		return 3;

	if (LD_WORD(&fs->win[BS_55AA]) != 0xAA55)	/* Check boot record signature (always placed at offset 510 even if the sector size is >512) */
		return 2;

	if ((LD_DWORD(&fs->win[BS_FilSysType]) & 0xFFFFFF) == 0x544146)		/* Check "FAT" string */
		return 0;
	if ((LD_DWORD(&fs->win[BS_FilSysType32]) & 0xFFFFFF) == 0x544146)	/* Check "FAT" string */
		return 0;

	return 1;
}




/*-----------------------------------------------------------------------*/
/* Find logical drive and check if the volume is mounted                 */
/*-----------------------------------------------------------------------*/
/*find the FATFS data, and save the FATFS information for any volume*/
/*主要就是得到存储设备代表的FATFS信息
如果FATFS还没读到，就从扇区中读出来，
如果已经存在，就返回就可以了*/
static
FRESULT find_volume (	/* FR_OK(0): successful, !=0: any error occurred */
	FATFS** rfs,		/* Pointer to pointer to the found file system object */
	const TCHAR** path,	/* Pointer to pointer to the path name (drive number) */
	BYTE wmode			/* !=0: Check write protection for write access */
)
{
	BYTE fmt;
	int vol;
	DSTATUS stat;
	DWORD bsect, fasize, tsect, sysect, nclst, szbfat;
	WORD nrsv;
	FATFS *fs;


	/* Get logical drive number from the path name */
	*rfs = 0;
	vol = get_ldnumber(path);
	if (vol < 0) return FR_INVALID_DRIVE;

	/* Check if the file system object is valid or not */
	fs = FatFs[vol];					/* Get pointer to the file system object */
	if (!fs) return FR_NOT_ENABLED;		/* Is the file system object available? */

	ENTER_FF(fs);						/* Lock the volume */
	*rfs = fs;							/* Return pointer to the file system object */

	if (fs->fs_type) {					/* If the volume has been mounted */
		stat = disk_status(fs->drv);
		if (!(stat & STA_NOINIT)) {		/* and the physical drive is kept initialized */
			if (!_FS_READONLY && wmode && (stat & STA_PROTECT))	/* Check write protection if needed */
				return FR_WRITE_PROTECTED;
			return FR_OK;				/* The file system object is valid */
		}
	}

	/* The file system object is not valid. */
	/* Following code attempts to mount the volume. (analyze BPB and initialize the fs object) */

	fs->fs_type = 0;					/* Clear the file system object */
	fs->drv = LD2PD(vol);				/* Bind the logical drive and a physical drive */
	stat = disk_initialize(fs->drv);	/* Initialize the physical drive */
	if (stat & STA_NOINIT)				/* Check if the initialization succeeded */
		return FR_NOT_READY;			/* Failed to initialize due to no medium or hard error */
	if (!_FS_READONLY && wmode && (stat & STA_PROTECT))	/* Check disk write protection if needed */
		return FR_WRITE_PROTECTED;
#if _MAX_SS != _MIN_SS						/* Get sector size (multiple sector size cfg only) */
	if (disk_ioctl(fs->drv, GET_SECTOR_SIZE, &SS(fs)) != RES_OK
		|| SS(fs) < _MIN_SS || SS(fs) > _MAX_SS) return FR_DISK_ERR;
#endif
	/* Find an FAT partition on the drive. Supports only generic partitioning, FDISK and SFD. */
	bsect = 0;
	fmt = check_fs(fs, bsect);					/* Load sector 0 and check if it is an FAT boot sector as SFD */
	if (fmt == 1 || (!fmt && (LD2PT(vol)))) {	/* Not an FAT boot sector or forced partition number */
		UINT i;
		DWORD br[4];

		for (i = 0; i < 4; i++) {			/* Get partition offset */
			BYTE *pt = fs->win+MBR_Table + i * SZ_PTE;
			br[i] = pt[4] ? LD_DWORD(&pt[8]) : 0;
		}
		i = LD2PT(vol);						/* Partition number: 0:auto, 1-4:forced */
		if (i) i--;
		do {								/* Find an FAT volume */
			bsect = br[i];
			fmt = bsect ? check_fs(fs, bsect) : 2;	/* Check the partition */
		} while (!LD2PT(vol) && fmt && ++i < 4);
	}
	if (fmt == 3) return FR_DISK_ERR;		/* An error occured in the disk I/O layer */
	if (fmt) return FR_NO_FILESYSTEM;		/* No FAT volume is found */

	/* An FAT volume is found. Following code initializes the file system object */

	if (LD_WORD(fs->win+BPB_BytsPerSec) != SS(fs))		/* (BPB_BytsPerSec must be equal to the physical sector size) */
		return FR_NO_FILESYSTEM;

	fasize = LD_WORD(fs->win+BPB_FATSz16);				/* Number of sectors per FAT */
	if (!fasize) fasize = LD_DWORD(fs->win+BPB_FATSz32);
	fs->fsize = fasize;

	fs->n_fats = fs->win[BPB_NumFATs];					/* Number of FAT copies */
	if (fs->n_fats != 1 && fs->n_fats != 2)				/* (Must be 1 or 2) */
		return FR_NO_FILESYSTEM;
	fasize *= fs->n_fats;								/* Number of sectors for FAT area */
	printf("%s: Number of FAT copies fs->n_fats(%d)\r\n", __func__, fs->n_fats);
	printf("%s: Number of sectors for FAT area fasize(%d)\r\n", __func__, fasize);

	fs->csize = fs->win[BPB_SecPerClus];				/* Number of sectors per cluster */
	printf("%s: Number of sectors per cluster fs->csize(%d)\r\n", __func__, fs->csize);

	if (!fs->csize || (fs->csize & (fs->csize - 1)))	/* (Must be power of 2) */
		return FR_NO_FILESYSTEM;

	fs->n_rootdir = LD_WORD(fs->win+BPB_RootEntCnt);	/* Number of root directory entries */
	printf("%s: fs->n_rootdir(%d)\r\n", __func__, fs->n_rootdir);
	if (fs->n_rootdir % (SS(fs) / SZ_DIR))				/* (Must be sector aligned) */
		return FR_NO_FILESYSTEM;

	tsect = LD_WORD(fs->win+BPB_TotSec16);				/* Number of sectors on the volume */
	if (!tsect) tsect = LD_DWORD(fs->win+BPB_TotSec32);

	nrsv = LD_WORD(fs->win+BPB_RsvdSecCnt);				/* Number of reserved sectors */
	if (!nrsv) return FR_NO_FILESYSTEM;					/* (Must not be 0) */

	/* Determine the FAT sub type */
	sysect = nrsv + fasize + fs->n_rootdir / (SS(fs) / SZ_DIR);	/* RSV+FAT+DIR */
	if (tsect < sysect) return FR_NO_FILESYSTEM;		/* (Invalid volume size) */
	nclst = (tsect - sysect) / fs->csize;				/* Number of clusters */
	if (!nclst) return FR_NO_FILESYSTEM;				/* (Invalid volume size) */
	fmt = FS_FAT12;
	if (nclst >= MIN_FAT16) fmt = FS_FAT16;
	if (nclst >= MIN_FAT32) fmt = FS_FAT32;
	printf("%s: fmt(%d), FS_FAT12=1 FS_FAT16=2, FS_FAT32=3\r\n", __func__, fmt);

	/* Boundaries and Limits */
	fs->n_fatent = nclst + 2;							/* Number of FAT entries */
	fs->volbase = bsect;								/* Volume start sector */
	fs->fatbase = bsect + nrsv; 						/* FAT start sector */
	fs->database = bsect + sysect;						/* Data start sector */
	if (fmt == FS_FAT32) {
		if (fs->n_rootdir) return FR_NO_FILESYSTEM;		/* (BPB_RootEntCnt must be 0) */
		fs->dirbase = LD_DWORD(fs->win+BPB_RootClus);	/* Root directory start cluster */
		szbfat = fs->n_fatent * 4;						/* (Needed FAT size) */
	} else {
		if (!fs->n_rootdir)	return FR_NO_FILESYSTEM;	/* (BPB_RootEntCnt must not be 0) */
		fs->dirbase = fs->fatbase + fasize;				/* Root directory start sector */
		szbfat = (fmt == FS_FAT16) ?					/* (Needed FAT size) */
			fs->n_fatent * 2 : fs->n_fatent * 3 / 2 + (fs->n_fatent & 1);
	}
	if (fs->fsize < (szbfat + (SS(fs) - 1)) / SS(fs))	/* (BPB_FATSz must not be less than needed) */
		return FR_NO_FILESYSTEM;

	printf("%s: Number of FAT entries(=Number of clusters+2)" \
		"fs->n_fatent(%d)\r\n", __func__, fs->n_fatent);
	printf("%s: Volume start sector fs->volbase(%d)\r\n", __func__, fs->volbase);
	printf("%s: FAT start sector fs->fatbase(%d)\r\n", __func__, fs->fatbase);
	printf("%s: Root directory start sector fs->dirbase(%d)\r\n", __func__, fs->dirbase);
	printf("%s: Data start sector fs->database(%d)\r\n", __func__, fs->database);

#if !_FS_READONLY
	/* Initialize cluster allocation information */
	fs->last_clust = fs->free_clust = 0xFFFFFFFF;

	/* Get fsinfo if available */
	fs->fsi_flag = 0x80;
#if (_FS_NOFSINFO & 3) != 3
	if (fmt == FS_FAT32				/* Enable FSINFO only if FAT32 and BPB_FSInfo is 1 */
		&& LD_WORD(fs->win+BPB_FSInfo) == 1
		&& move_window(fs, bsect + 1) == FR_OK)
	{
		fs->fsi_flag = 0;
		if (LD_WORD(fs->win+BS_55AA) == 0xAA55	/* Load FSINFO data if available */
			&& LD_DWORD(fs->win+FSI_LeadSig) == 0x41615252
			&& LD_DWORD(fs->win+FSI_StrucSig) == 0x61417272)
		{
#if (_FS_NOFSINFO & 1) == 0
			fs->free_clust = LD_DWORD(fs->win+FSI_Free_Count);
#endif
#if (_FS_NOFSINFO & 2) == 0
			fs->last_clust = LD_DWORD(fs->win+FSI_Nxt_Free);
#endif
		}
	}
#endif
#endif
	fs->fs_type = fmt;	/* FAT sub-type */
	fs->id = ++Fsid;	/* File system mount ID */
#if _FS_RPATH
	fs->cdir = 0;		/* Set current directory to root */
#endif
#if _FS_LOCK			/* Clear file lock semaphores */
	clear_lock(fs);
#endif

/*
1、mf_fmkfs("1:", 1, 4096):

f_mount: path = 1:
f_mount: vol = 1, path = 1:
W25QXX_Init: W25QXX_TYPE = 0XEF14
move_window: sector(0) != fs->winsect(-1)
find_volume: Number of FAT copies fs->n_fats(1)
find_volume: Number of sectors for FAT area fasize(7)
find_volume: Number of sectors per cluster fs->csize(8)
find_volume: fs->n_rootdir(512)
find_volume: fmt(1), FS_FAT12=1 FS_FAT16=2, FS_FAT32=3
find_volume: Number of FAT entries(=Number of clusters+2)fs->n_fatent(509)
find_volume: Volume start sector fs->volbase(0)
find_volume: FAT start sector fs->fatbase(1)
find_volume: Root directory start sector fs->dirbase(8)
find_volume: Data start sector fs->database(40)


2、mf_fmkfs("1:", 1, 1024):

move_window: sector(0) != fs->winsect(-1)
find_volume: Number of FAT copies fs->n_fats(1)
find_volume: Number of sectors for FAT area fasize(7)
find_volume: Number of sectors per cluster fs->csize(2)
find_volume: fs->n_rootdir(512)
find_volume: fmt(1), FS_FAT12=1 FS_FAT16=2, FS_FAT32=3
find_volume: Number of FAT entries(=Number of clusters+2)fs->n_fatent(2030)
find_volume: Volume start sector fs->volbase(0)
find_volume: FAT start sector fs->fatbase(1)
find_volume: Root directory start sector fs->dirbase(8)
find_volume: Data start sector fs->database(40)

*/
//以上代码主要是根据引导扇区里的数据，填
//充文件系统对象结构体的信息：包括FAT表大小、
//数目、根目录区起始地址、数据区起始扇区、
//文件系统类型等等。


/*

DBR :

find_volume: Number of FAT copies fs->n_fats(1)
find_volume: Number of sectors for FAT area fasize(2)
find_volume: Number of sectors per cluster fs->csize(2)
find_volume: fs->n_rootdir(16)
find_volume: fmt(1), FS_FAT12=1 FS_FAT16=2, FS_FAT32=3
find_volume: Number of FAT entries(=Number of clusters+2)fs->n_fatent(10)
find_volume: Volume start sector fs->volbase(0)
find_volume: FAT start sector fs->fatbase(1)
find_volume: Root directory start sector fs->dirbase(3)
find_volume: Data start sector fs->database(4)

Start Addr: 2000eb60, sector(0)
          00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15

00:       eb  fe 90 4d 53 44 4f 53 35 2e 30 (00 02) (02) 01 00
01:       (01) (10 00) 14 00 f0  02 00 3f 00 ff  00 00 00 00 00
02:       00 00 00 00 80 00 29 95 ae 4c 49 4e 4f 20 4e 41
03:       4d 45 20 20 20 20 46 41 54 20 20 20 20 20 00 00
04:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
05:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
06:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
07:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
08:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
09:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
10:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
11:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
12:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
13:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
14:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
15:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
16:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
17:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
18:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
19:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 55 aa

FAT:

root@2:/ #dump 1

Start Addr: 2000ed60, sector(1)
          00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15

00:       f0  ff  ff 00 00 00 00 00 00 00 00 00 00 00 00 00
01:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
02:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
03:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
04:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
05:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
06:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
07:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
08:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
09:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
10:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
11:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
12:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
13:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
14:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
15:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
16:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
17:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
18:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
19:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

root:

root@2:/ #dump 3

Start Addr: 2000f160, sector(3)
          00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15

00:       52 41 4d 44 49 53 4b 20 20 20 20 08 00 00 00 00
01:       00 00 00 00 00 00 95 ae 4c 49 00 00 00 00 00 00
02:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
03:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
04:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
05:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
06:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
07:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
08:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
09:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
10:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
11:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
12:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
13:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
14:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
15:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
16:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
17:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
18:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
19:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

root@2:/ #mkdir sys
root@2:/ #dump 3

Start Addr: 2000f160, sector(3)
            00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 

00:       52 41 4d 44 49 53 4b 20 20 20 20 (08) 00 00 00 00
01:       00 00 00 00 00 00 95 ae 4c 49 00 00 00 00 00 00
02:       53 59 53 20 20 20 20 20 20 20 20 (10) 00 00 00 00
03:       00 00 00 00 00 00 8b b0 4c 49 (02 00) 00 00 00 00
04:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
05:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
06:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
07:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
08:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
09:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
0F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
10:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
11:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
12:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
13:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
14:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
15:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
16:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
17:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
18:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
19:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1A:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1B:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1C:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1D:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1E:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
1F:       00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00

*/

	return FR_OK;
}




/*-----------------------------------------------------------------------*/
/* Check if the file/directory object is valid or not                    */
/*-----------------------------------------------------------------------*/

static
FRESULT validate (	/* FR_OK(0): The object is valid, !=0: Invalid */
	void* obj		/* Pointer to the object FIL/DIR to check validity */
)
{
	FIL *fil = (FIL*)obj;	/* Assuming offset of .fs and .id in the FIL/DIR structure is identical */


	if (!fil || !fil->fs || !fil->fs->fs_type || fil->fs->id != fil->id)
		return FR_INVALID_OBJECT;

	ENTER_FF(fil->fs);		/* Lock file system */

	if (disk_status(fil->fs->drv) & STA_NOINIT)
		return FR_NOT_READY;

	return FR_OK;
}




/*--------------------------------------------------------------------------

   Public Functions

--------------------------------------------------------------------------*/



/*-----------------------------------------------------------------------*/
/* Mount/Unmount a Logical Drive                                         */
/*-----------------------------------------------------------------------*/

//res = f_mount( fs[0], "1:", 1 );

FRESULT f_mount (
	FATFS* fs,			/* Pointer to the file system object (NULL:unmount)*/
	const TCHAR* path,	/* Logical drive number to be mounted/unmounted */
	BYTE opt			/* 0:Do not mount (delayed mount), 1:Mount immediately */
)
{
	FATFS *cfs;
	int vol;
	FRESULT res;
	const TCHAR *rp = path;

	printf("%s: path = %s\r\n", __func__, path);
	vol = get_ldnumber(&rp);
	printf("%s: vol = %d, path = %s\r\n", __func__, vol, path);
	if (vol < 0) return FR_INVALID_DRIVE;
	cfs = FatFs[vol];					/* Pointer to fs object */

	if (cfs) {
#if _FS_LOCK
		clear_lock(cfs);
#endif
#if _FS_REENTRANT						/* Discard sync object of the current volume */
		if (!ff_del_syncobj(cfs->sobj)) return FR_INT_ERR;
#endif
		cfs->fs_type = 0;				/* Clear old fs object */
	}

	if (fs) {
		fs->fs_type = 0;				/* Clear new fs object */
#if _FS_REENTRANT						/* Create sync object for the new volume */
		if (!ff_cre_syncobj((BYTE)vol, &fs->sobj)) return FR_INT_ERR;
#endif
	}
	FatFs[vol] = fs;					/* Register new fs object */

	if (!fs || opt != 1) return FR_OK;	/* Do not mount now, it will be mounted later */

	res = find_volume(&fs, &path, 0);	/* Force mounted the volume */
	LEAVE_FF(fs, res);
}




/*-----------------------------------------------------------------------*/
/* Open or Create a File                                                 */
/*-----------------------------------------------------------------------*/

/*
FAT文件系统根据根目录来寻址其他文件(包括文件夹)，
故而根目录的位置必须在磁盘存取数据之前得以确定
FAT文件系统就是根据分区的相关DBR参数与DBR中存放的
已经计算好的FAT表(2份)的大小来确定的。格式化以后，
跟目录的大小和位置其实都已经确定下来了：位置紧随
FAT2之后，大小通常为32个扇区。根目录之后便是数据区
第2簇。
*/

/*
FAT32的另一项重大改革是根目录的文件化，
即将根目录等同于普通的文件。这样根目录便没有了
FAT16中512个目录项的限制
不够用的时候增加簇链，分配空簇即可。
而且，根目录的位置也不再硬性地固定了，
可以存储在分区内可寻址的任意簇内，
不过通常根目录是最早建立的(格式化就生成了)目录表。


表11   FAT16目录项32个字节的表示定义
字节偏移(16进制)			字节数				定义
0x0~0x7							8						文件名
0x8~0xA							3						扩展名
0xB								1						属性字节	00000000(读写)
																		00000001(只读)
																		00000010(隐藏)
																		00000100(系统)
																		00001000(卷标)
																		  00010000(子目录)
																		00100000(归档)
0xC~0x15						10										系统保留
0x16~0x17						2										文件的最近修改时间
0x18~0x19						2										文件的最近修改日期
0x1A~0x1B						2										表示文件的首簇号
0x1C~0x1F						4										表示文件的长度



图4.3.11 Fat16的组织形式
引导扇区	FAT1					FAT2(重复的)			
(1扇区)		(实际情况取大小)		(同FAT1	)				
 

根文件夹	其他文件夹及所有文件		剩余扇区
(32个扇区)	(开始簇编号(从2开始))			(不足一簇)

*/

/*
[root]1:/#
[root]1:/#cat qq.txt
follow_path: path = qq.txt
follow_path: 1 dp->sect=536879052
follow_path: 2 dp->sect=536879052
follow_path: create_name after, path = , dp->fn=QQ      TXTX
dir_find: need to find file name = QQ      TXT, dp->sect=536879052
move_window: sector(8) != fs->winsect(9)
dir_find, find the file !!! scuess!
dir_find: out, dp->fn(QQ      TXT), dp->sect=8
follow_path: Last segment matched. Function completed. 

f_open: path(1:/qq.txt)
f_open: after find_volume(), path(/qq.txt)
follow_path: path = qq.txt
follow_path: 1 dp->sect=134218791
follow_path: 2 dp->sect=134218791
follow_path: create_name after, path = , dp->fn=QQ      TXTX
dir_find: need to find file name = QQ      TXT, dp->sect=134218791
dir_find, find the file !!! scuess!
dir_find: out, dp->fn(QQ      TXT), dp->sect=8
follow_path: Last segment matched. Function completed. 
f_open: after follow_path, path=/qq.txt
f_open: fp init!!! dj.index = 2
78efd


f_open: path(1:/qq.txt)
f_open: after find_volume(), path(/qq.txt)
follow_path: path = qq.txt
follow_path: 1 dp->sect=134218791
follow_path: 2 dp->sect=134218791
follow_path: create_name after, path = , dp->fn=QQ      TXTX
dir_find: need to find file name = QQ      TXT, dp->sect=134218791
in dir_sdi: dp->index(8192), dp->sect(134218791), dp->sclust(0)
out dir_sdi: dp->index(0), dp->sect(8)
dir_find, find the file !!! scuess!
dir_find: out, dp->fn(QQ      TXT), dp->sect=8
follow_path: Last segment matched. Function completed. 
f_open: after follow_path, path=/qq.txt
f_open: fp init!!! dj.index = 2
78efd


f_open: path(1:/USER./yang.txt)
f_open: after find_volume(), path(/USER./yang.txt)
follow_path: path = USER./yang.txt
follow_path: 1 dp->sect=134218791
follow_path: 2 dp->sect=134218791
follow_path: create_name after, path = yang.txt, dp->fn=USER       
dir_find: need to find file name = USER       , dp->sect=134218791
in dir_sdi: dp->index(8192), dp->sect(134218791), dp->sclust(0)
out dir_sdi: dp->index(0), dp->sect(8)
move_window: sector(8) != fs->winsect(64)
dir_find, find the file !!! scuess!
dir_find: out, dp->fn(USER       ), dp->sect=8
follow_path: 1 dp->sect=8
follow_path: 2 dp->sect=8
follow_path: create_name after, path = , dp->fn=YANG    TXTX
dir_find: need to find file name = YANG    TXT, dp->sect=8
in dir_sdi: dp->index(1), dp->sect(8), dp->sclust(5)
out dir_sdi: dp->index(0), dp->sect(64)
move_window: sector(64) != fs->winsect(8)
dir_find, find the file !!! scuess!
dir_find: out, dp->fn(YANG    TXT), dp->sect=64
follow_path: Last segment matched. Function completed. 
f_open: after follow_path, path=/USER./yang.txt
f_open: fp init!!! dj.index = 3
df


*/
FRESULT f_open (
	FIL* fp,			/* Pointer to the blank file object */
	const TCHAR* path,	/* Pointer to the file name */
	BYTE mode			/* Access mode and file open mode flags */
)
{
	FRESULT res;
	DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;

	
	if (!fp) return FR_INVALID_OBJECT;
	fp->fs = 0;			/* Clear file object */
	//	1:/USER/yang.txt
	//printf("%s: path(%s)\r\n", __func__, path);
	/* Get logical drive number */
#if !_FS_READONLY
	mode &= FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW;
	res = find_volume(&dj.fs, &path, (BYTE)(mode & ~FA_READ));
#else
	mode &= FA_READ;
	res = find_volume(&dj.fs, &path, 0);
#endif
	// 	/USER/yang.txt
	//printf("%s: after find_volume(), path(%s)\r\n", __func__, path);

	if (res == FR_OK) {
		INIT_BUF(dj);
		/*根据文件路径， 得到所对应的dir结构体
		follow_path从文件目录项表中找到文件名字为path
		的文件，把对应的扇区读出来，然后计算出
		此文件目录项在扇区的偏移地址，然后把
		dir.dir指向这个地址，从这个地址我们就要以读出
		文件的大小，起始cluster*/
		res = follow_path(&dj, path);	/* Follow the file path */
		//printf("%s: after follow_path, path=%s\r\n", __func__, path);
		dir = dj.dir;
#if !_FS_READONLY	/* R/W configuration */
		if (res == FR_OK) {
			if (!dir)	/* Default directory itself */
				res = FR_INVALID_NAME;
#if _FS_LOCK
			else
				res = chk_lock(&dj, (mode & ~FA_READ) ? 1 : 0);
#endif
		}
		/* Create or Open a file */
		if (mode & (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW)) {
			DWORD dw, cl;

			if (res != FR_OK) {					/* No file, create new */
				if (res == FR_NO_FILE)			/* There is no file to open, create a new entry */
#if _FS_LOCK
					res = enq_lock() ? dir_register(&dj) : FR_TOO_MANY_OPEN_FILES;
#else
					//printf("%s: file is't ezixt! dir_register a file, dj.sclust(%d)\r\n", __func__, dj.sclust);
					/*此时dj->sclust应该代表该文件要被创建的目录
					如果是root 则dj->sclust = 0， 如果为子目录，肯定不
					是为0， 也就是dj->sclust != 0*/
					res = dir_register(&dj);
#endif
				//mode |= FA_CREATE_ALWAYS;		/* File is created */
				dir = dj.dir;					/* New entry */
			}
			else {								/* Any object is already existing */
				if (dir[DIR_Attr] & (AM_RDO | AM_DIR)) {	/* Cannot overwrite it (R/O or DIR) */
					res = FR_DENIED;
				} else {
					/*if use FA_CREATE_NEW and file exist, return FR_EXIST*/
					if (mode & FA_CREATE_NEW)	/* Cannot create as new file */
						res = FR_EXIST;
				}
			}
			if (res == FR_OK && (mode & FA_CREATE_ALWAYS)) {	
				/* Truncate it if overwrite mode */
				/*如果是覆盖式的打开文件
					有可能导致，旧文件被移除，也就是
					要移除the directory item of old file, and it's cluster chain
				*/
				dw = get_fattime();				/* Created time */
				ST_DWORD(dir+DIR_CrtTime, dw);
				dir[DIR_Attr] = 0;				/* Reset attribute */
				/*设置文件的大小为0*/
				ST_DWORD(dir+DIR_FileSize, 0);	/* size = 0 */
				/*读取当前目录项的开始族，如果
					是旧文件有内容，则它不为0*/
				cl = ld_clust(dj.fs, dir);		/* Get start cluster */
				/*设置新文件的开始族为0，文件在没有内容
				时，它的大小为0，开始族也为0*/
				st_clust(dir, 0);				/* cluster = 0 */
				/*目录项信息被修改，要回写*/
				dj.fs->wflag = 1;
				if (cl) {						/* Remove the cluster chain if exist */
					dw = dj.fs->winsect;
					res = remove_chain(dj.fs, cl);
					/*如果移除族链成功，会导致
						dj.fs->winsect被修改，所以此时的
						dj.fs->winsect  != dw	也就是，
						move_window(dj.fs, dw)可以把被新文件
						的目录项覆盖的缓存fs->win回写到flash中
					*/
					if (res == FR_OK) {
						dj.fs->last_clust = cl - 1;	/* Reuse the cluster hole */
						res = move_window(dj.fs, dw);
					}
				}
			}
		}
		else {	/* Open an existing file */
			if (res == FR_OK) {					/* Follow succeeded */
				if (dir[DIR_Attr] & AM_DIR) {	/* It is a directory */
					res = FR_NO_FILE;
				} else {
					/*只读文件，不能被写打开*/
					if ((mode & FA_WRITE) && (dir[DIR_Attr] & AM_RDO)) /* R/O violation */
						res = FR_DENIED;
				}
			}
		}
		if (res == FR_OK) {
			if (mode & FA_CREATE_ALWAYS)		/* Set file change flag if created or overwritten */
				mode |= FA__WRITTEN;
			/*只读的话，不用保存，因为不会去更新
			文件信息，大小、属性等*/
			fp->dir_sect = dj.fs->winsect;		/* Pointer to the directory entry */
			fp->dir_ptr = dir;
#if _FS_LOCK
			fp->lockid = inc_lock(&dj, (mode & ~FA_READ) ? 1 : 0);
			if (!fp->lockid) res = FR_INT_ERR;
#endif
		}

#else				/* R/O configuration */
		if (res == FR_OK) {					/* Follow succeeded */
			dir = dj.dir;
			if (!dir) {						/* Current directory itself */
				res = FR_INVALID_NAME;
			} else {
				if (dir[DIR_Attr] & AM_DIR)	/* It is a directory */
					res = FR_NO_FILE;
			}
		}
#endif
		FREE_BUF();

		if (res == FR_OK) {
			//printf("%s: fp init!!! dj.index = %d\r\n", __func__, dj.index);			
			fp->flag = mode;					/* File access mode */
			fp->err = 0;						/* Clear error flag */
			fp->sclust = ld_clust(dj.fs, dir);	/* File start cluster */
			fp->fsize = LD_DWORD(dir+DIR_FileSize);	/* File size */
			fp->fptr = 0;						/* File pointer */
			fp->dsect = 0;
#if _USE_FASTSEEK
			fp->cltbl = 0;						/* Normal seek mode */
#endif
			fp->fs = dj.fs;	 					/* Validate file object */
			fp->id = fp->fs->id;
		}
	}

	LEAVE_FF(dj.fs, res);
}




/*-----------------------------------------------------------------------*/
/* Read File                                                             */
/*-----------------------------------------------------------------------*/

FRESULT f_read (
	FIL* fp, 		/* Pointer to the file object */
	void* buff,		/* Pointer to data buffer */
	UINT btr,		/* Number of bytes to read */
	UINT* br		/* Pointer to number of bytes read */
)
{
	FRESULT res;
	DWORD clst, sect, remain;
	UINT rcnt, cc;
	BYTE csect, *rbuff = (BYTE*)buff;


	*br = 0;	/* Clear read byte counter */

	res = validate(fp);							/* Check validity */
	if (res != FR_OK) LEAVE_FF(fp->fs, res);
	if (fp->err)								/* Check error */
		LEAVE_FF(fp->fs, (FRESULT)fp->err);
	if (!(fp->flag & FA_READ)) 					/* Check access mode */
		LEAVE_FF(fp->fs, FR_DENIED);
	remain = fp->fsize - fp->fptr;
	if (btr > remain) btr = (UINT)remain;		/* Truncate btr by remaining bytes */

	for ( ;  btr;								/* Repeat until all data read */
		rbuff += rcnt, fp->fptr += rcnt, *br += rcnt, btr -= rcnt) {
		if ((fp->fptr % SS(fp->fs)) == 0) {		/* On the sector boundary? */
			csect = (BYTE)(fp->fptr / SS(fp->fs) & (fp->fs->csize - 1));	/* Sector offset in the cluster */
			if (!csect) {						/* On the cluster boundary? */
				if (fp->fptr == 0) {			/* On the top of the file? */
					clst = fp->sclust;			/* Follow from the origin */
				} else {						/* Middle or end of the file */
#if _USE_FASTSEEK
					if (fp->cltbl)
						clst = clmt_clust(fp, fp->fptr);	/* Get cluster# from the CLMT */
					else
#endif
						clst = get_fat(fp->fs, fp->clust);	/* Follow cluster chain on the FAT */
				}
				if (clst < 2) ABORT(fp->fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
				fp->clust = clst;				/* Update current cluster */
			}
			sect = clust2sect(fp->fs, fp->clust);	/* Get current sector */
			if (!sect) ABORT(fp->fs, FR_INT_ERR);
			sect += csect;
			cc = btr / SS(fp->fs);				/* When remaining bytes >= sector size, */
			if (cc) {							/* Read maximum contiguous sectors directly */
				if (csect + cc > fp->fs->csize)	/* Clip at cluster boundary */
					cc = fp->fs->csize - csect;
				if (disk_read(fp->fs->drv, rbuff, sect, cc))
					ABORT(fp->fs, FR_DISK_ERR);
#if !_FS_READONLY && _FS_MINIMIZE <= 2			/* Replace one of the read sectors with cached data if it contains a dirty sector */
#if _FS_TINY
				if (fp->fs->wflag && fp->fs->winsect - sect < cc)
					mem_cpy(rbuff + ((fp->fs->winsect - sect) * SS(fp->fs)), fp->fs->win, SS(fp->fs));
#else
				if ((fp->flag & FA__DIRTY) && fp->dsect - sect < cc)
					mem_cpy(rbuff + ((fp->dsect - sect) * SS(fp->fs)), fp->buf, SS(fp->fs));
#endif
#endif
				rcnt = SS(fp->fs) * cc;			/* Number of bytes transferred */
				continue;
			}
#if !_FS_TINY
			if (fp->dsect != sect) {			/* Load data sector if not in cache */
#if !_FS_READONLY
				if (fp->flag & FA__DIRTY) {		/* Write-back dirty sector cache */
					if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
						ABORT(fp->fs, FR_DISK_ERR);
					fp->flag &= ~FA__DIRTY;
				}
#endif
				if (disk_read(fp->fs->drv, fp->buf, sect, 1))	/* Fill sector cache */
					ABORT(fp->fs, FR_DISK_ERR);
			}
#endif
			fp->dsect = sect;
		}
		rcnt = SS(fp->fs) - ((UINT)fp->fptr % SS(fp->fs));	/* Get partial sector data from sector buffer */
		if (rcnt > btr) rcnt = btr;
#if _FS_TINY
		if (move_window(fp->fs, fp->dsect))		/* Move sector window */
			ABORT(fp->fs, FR_DISK_ERR);
		mem_cpy(rbuff, &fp->fs->win[fp->fptr % SS(fp->fs)], rcnt);	/* Pick partial sector */
#else
		mem_cpy(rbuff, &fp->buf[fp->fptr % SS(fp->fs)], rcnt);	/* Pick partial sector */
#endif
	}

	LEAVE_FF(fp->fs, FR_OK);
}




#if !_FS_READONLY
/*-----------------------------------------------------------------------*/
/* Write File                                                            */
/*-----------------------------------------------------------------------*/

FRESULT f_write (
	FIL* fp,			/* Pointer to the file object */
	const void *buff,	/* Pointer to the data to be written */
	UINT btw,			/* Number of bytes to write */
	UINT* bw			/* Pointer to number of bytes written */
)
{
	FRESULT res;
	DWORD clst, sect;
	UINT wcnt, cc;
	const BYTE *wbuff = (const BYTE*)buff;
	BYTE csect;


	*bw = 0;	/* Clear write byte counter */

	res = validate(fp);						/* Check validity */
	if (res != FR_OK) LEAVE_FF(fp->fs, res);
	if (fp->err)							/* Check error */
		LEAVE_FF(fp->fs, (FRESULT)fp->err);
	if (!(fp->flag & FA_WRITE))				/* Check access mode */
		LEAVE_FF(fp->fs, FR_DENIED);
	/*溢出*/
	if (fp->fptr + btw < fp->fptr) btw = 0;	/* File size cannot reach 4GB */

	for ( ;  btw;							/* Repeat until all data written */
		wbuff += wcnt, fp->fptr += wcnt, *bw += wcnt, btw -= wcnt) {
		if ((fp->fptr % SS(fp->fs)) == 0) {	/* On the sector boundary? */
			csect = (BYTE)(fp->fptr / SS(fp->fs) & (fp->fs->csize - 1));	/* Sector offset in the cluster */
			if (!csect) {					/* On the cluster boundary? */
				if (fp->fptr == 0) {		/* On the top of the file? */
					clst = fp->sclust;		/* Follow from the origin */
					if (clst == 0)			/* When no cluster is allocated, */
					{	
						/*when the file is created in f_open, fp->sclust is set to 0
							it just open the file, alloc the directory item*/
						clst = create_chain(fp->fs, 0);	/* Create a new cluster chain */
					}
				} else {					/* Middle or end of the file */
#if _USE_FASTSEEK
					if (fp->cltbl)
						clst = clmt_clust(fp, fp->fptr);	/* Get cluster# from the CLMT */
					else
#endif
						clst = create_chain(fp->fs, fp->clust);	/* Follow or stretch cluster chain on the FAT */
				}
				if (clst == 0) break;		/* Could not allocate a new cluster (disk full) */
				if (clst == 1) ABORT(fp->fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
				fp->clust = clst;			/* Update current cluster */
				if (fp->sclust == 0) fp->sclust = clst;	/* Set start cluster if the first write */
			}
#if _FS_TINY
			if (fp->fs->winsect == fp->dsect && sync_window(fp->fs))	/* Write-back sector cache */
				ABORT(fp->fs, FR_DISK_ERR);
#else
			if (fp->flag & FA__DIRTY) {		/* Write-back sector cache */
				if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
					ABORT(fp->fs, FR_DISK_ERR);
				fp->flag &= ~FA__DIRTY;
			}
#endif
			sect = clust2sect(fp->fs, fp->clust);	/* Get current sector */
			if (!sect) ABORT(fp->fs, FR_INT_ERR);
			sect += csect;
			cc = btw / SS(fp->fs);			/* When remaining bytes >= sector size, */
			if (cc) {						/* Write maximum contiguous sectors directly */
				if (csect + cc > fp->fs->csize)	/* Clip at cluster boundary */
					cc = fp->fs->csize - csect;
				if (disk_write(fp->fs->drv, wbuff, sect, cc))
					ABORT(fp->fs, FR_DISK_ERR);
#if _FS_MINIMIZE <= 2
#if _FS_TINY
				if (fp->fs->winsect - sect < cc) {	/* Refill sector cache if it gets invalidated by the direct write */
					mem_cpy(fp->fs->win, wbuff + ((fp->fs->winsect - sect) * SS(fp->fs)), SS(fp->fs));
					fp->fs->wflag = 0;
				}
#else
				if (fp->dsect - sect < cc) { /* Refill sector cache if it gets invalidated by the direct write */
					mem_cpy(fp->buf, wbuff + ((fp->dsect - sect) * SS(fp->fs)), SS(fp->fs));
					fp->flag &= ~FA__DIRTY;
				}
#endif
#endif
				wcnt = SS(fp->fs) * cc;		/* Number of bytes transferred */
				continue;
			}
#if _FS_TINY
			if (fp->fptr >= fp->fsize) {	/* Avoid silly cache filling at growing edge */
				if (sync_window(fp->fs)) ABORT(fp->fs, FR_DISK_ERR);
				fp->fs->winsect = sect;
			}
#else
			if (fp->dsect != sect) {		/* Fill sector cache with file data */
				if (fp->fptr < fp->fsize &&
					disk_read(fp->fs->drv, fp->buf, sect, 1))
						ABORT(fp->fs, FR_DISK_ERR);
			}
#endif
			fp->dsect = sect;
		}
		wcnt = SS(fp->fs) - ((UINT)fp->fptr % SS(fp->fs));/* Put partial sector into file I/O buffer */
		if (wcnt > btw) wcnt = btw;
#if _FS_TINY
		if (move_window(fp->fs, fp->dsect))	/* Move sector window */
			ABORT(fp->fs, FR_DISK_ERR);
		mem_cpy(&fp->fs->win[fp->fptr % SS(fp->fs)], wbuff, wcnt);	/* Fit partial sector */
		fp->fs->wflag = 1;
#else
		mem_cpy(&fp->buf[fp->fptr % SS(fp->fs)], wbuff, wcnt);	/* Fit partial sector */
		fp->flag |= FA__DIRTY;
#endif
	}

	if (fp->fptr > fp->fsize) fp->fsize = fp->fptr;	/* Update file size if needed */
	fp->flag |= FA__WRITTEN;						/* Set file change flag */

	LEAVE_FF(fp->fs, FR_OK);
}




/*-----------------------------------------------------------------------*/
/* Synchronize the File                                                  */
/*-----------------------------------------------------------------------*/

FRESULT f_sync (
	FIL* fp		/* Pointer to the file object */
)
{
	FRESULT res;
	DWORD tm;
	BYTE *dir;


	res = validate(fp);					/* Check validity of the object */
	if (res == FR_OK) {
		if (fp->flag & FA__WRITTEN) {	/* Has the file been written? */
			/* Write-back dirty buffer */
#if !_FS_TINY
			if (fp->flag & FA__DIRTY) {
				if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
					LEAVE_FF(fp->fs, FR_DISK_ERR);
				fp->flag &= ~FA__DIRTY;
			}
#endif
			/* Update the directory entry */
			res = move_window(fp->fs, fp->dir_sect);
			if (res == FR_OK) {
				dir = fp->dir_ptr;
				dir[DIR_Attr] |= AM_ARC;					/* Set archive bit */
				ST_DWORD(dir+DIR_FileSize, fp->fsize);		/* Update file size */
				st_clust(dir, fp->sclust);					/* Update start cluster */
				tm = get_fattime();							/* Update updated time */
				ST_DWORD(dir+DIR_WrtTime, tm);
				ST_WORD(dir+DIR_LstAccDate, 0);
				fp->flag &= ~FA__WRITTEN;
				fp->fs->wflag = 1;
				res = sync_fs(fp->fs);
			}
		}
	}

	LEAVE_FF(fp->fs, res);
}

#endif /* !_FS_READONLY */




/*-----------------------------------------------------------------------*/
/* Close File                                                            */
/*-----------------------------------------------------------------------*/

FRESULT f_close (
	FIL *fp		/* Pointer to the file object to be closed */
)
{
	FRESULT res;


#if !_FS_READONLY
	res = f_sync(fp);					/* Flush cached data */
	if (res == FR_OK)
#endif
	{
		res = validate(fp);				/* Lock volume */
		if (res == FR_OK) {
#if _FS_REENTRANT
			FATFS *fs = fp->fs;
#endif
#if _FS_LOCK
			res = dec_lock(fp->lockid);	/* Decrement file open counter */
			if (res == FR_OK)
#endif
				fp->fs = 0;				/* Invalidate file object */
#if _FS_REENTRANT
			unlock_fs(fs, FR_OK);		/* Unlock volume */
#endif	
		}
		LEAVE_FF(fp->fs,res);			//FATFS新版本的bug,必须加上这句,否则在使用OS的时候,可能导致关闭中断后,一直不开启,从而假死.
	}
	return res;
}




/*-----------------------------------------------------------------------*/
/* Change Current Directory or Current Drive, Get Current Directory      */
/*-----------------------------------------------------------------------*/

#if _FS_RPATH >= 1
#if _VOLUMES >= 2
FRESULT f_chdrive (
	const TCHAR* path		/* Drive number */
)
{
	int vol;


	vol = get_ldnumber(&path);
	if (vol < 0) return FR_INVALID_DRIVE;

	/*获取当前路径是，会用到当前
	卷号*/
	CurrVol = (BYTE)vol;

	return FR_OK;
}
#endif


FRESULT f_chdir (
	const TCHAR* path	/* Pointer to the directory path */
)
{
	FRESULT res;
	DIR dj;
	DEF_NAMEBUF;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 0);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);		/* Follow the path */
		FREE_BUF();
		if (res == FR_OK) {					/* Follow completed */
			if (!dj.dir) {
				/*root directory ? */
				dj.fs->cdir = dj.sclust;	/* Start directory itself */
			} else {
				if (dj.dir[DIR_Attr] & AM_DIR)	/* Reached to the directory */
					dj.fs->cdir = ld_clust(dj.fs, dj.dir);
					/*找到子目录，把子目录的开始族
						赋值给cdir*/
				else
					res = FR_NO_PATH;		/* Reached but a file */
			}
		}
		if (res == FR_NO_FILE) res = FR_NO_PATH;
	}

	LEAVE_FF(dj.fs, res);
}


#if _FS_RPATH >= 2
FRESULT f_getcwd (
	TCHAR* buff,	/* Pointer to the directory path */
	UINT len		/* Size of path */
)
{
	FRESULT res;
	DIR dj;
	UINT i, n;
	DWORD ccl;
	TCHAR *tp;
	FILINFO fno;
	DEF_NAMEBUF;


	*buff = 0;
	/* Get logical drive number */
	res = find_volume(&dj.fs, (const TCHAR**)&buff, 0);	/* Get current volume */
	if (res == FR_OK) {
		INIT_BUF(dj);
		i = len;			/* Bottom of buffer (directory stack base) */
		dj.sclust = dj.fs->cdir;			/* Start to follow upper directory from current directory */
		//利用当前目录的开始族，往上遍历目录，
		//到根目录为止
		while ((ccl = dj.sclust) != 0) {	/* Repeat while current directory is a sub-directory */
			/*获取子目录下的第1 个目录项信息，从0开始*/
			res = dir_sdi(&dj, 1);			/* Get parent directory */
			if (res != FR_OK) break;
			/*读取第1个目录项的信息的缓存空间*/
			res = dir_read(&dj, 0);
			if (res != FR_OK) break;
			/*获取到父亲目录的开始族*/
			dj.sclust = ld_clust(dj.fs, dj.dir);	/* Goto parent directory */
			/*或取父亲目录的第0个目录项的sector还有
			所在扇区的偏移地址*/
			res = dir_sdi(&dj, 0);
			if (res != FR_OK) break;
			do {							/* Find the entry links to the child directory */
				/*遍历父亲目录，从父亲目录
				中找到子目录的目录项，做这一步
				是因为子目录的目录名都是放在这个目录
				项中*/
				res = dir_read(&dj, 0);
				if (res != FR_OK) break;
				if (ccl == ld_clust(dj.fs, dj.dir)) break;	/* Found the entry */
				res = dir_next(&dj, 0);	
			} while (res == FR_OK);
			if (res == FR_NO_FILE) res = FR_INT_ERR;/* It cannot be 'not found'. */
			if (res != FR_OK) break;
#if _USE_LFN
			fno.lfname = buff;
			fno.lfsize = i;
#endif
			/*从这个父亲目录中的
			目录项信息中获取dir的信息，放到FILINFO中*/
			get_fileinfo(&dj, &fno);		/* Get the directory name and push it to the buffer */
			tp = fno.fname;
#if _USE_LFN
			if (*buff) tp = buff;
#endif
			for (n = 0; tp[n]; n++) ;
			if (i < n + 3) {
				res = FR_NOT_ENOUGH_CORE; break;
			}
			while (n) buff[--i] = tp[--n];
			buff[--i] = '/';
		}
		tp = buff;
		if (res == FR_OK) {
#if _VOLUMES >= 2
			*tp++ = '0' + CurrVol;			/* Put drive number */
			*tp++ = ':';
#endif
			if (i == len) {					/* Root-directory */
				*tp++ = '/';
			} else {						/* Sub-directroy */
				do		/* Add stacked path str */
					*tp++ = buff[i++];
				while (i < len);
			}
		}
		*tp = 0;
		FREE_BUF();
	}

	LEAVE_FF(dj.fs, res);
}
#endif /* _FS_RPATH >= 2 */
#endif /* _FS_RPATH >= 1 */



#if _FS_MINIMIZE <= 2
/*-----------------------------------------------------------------------*/
/* Seek File R/W Pointer                                                 */
/*-----------------------------------------------------------------------*/

FRESULT f_lseek (
	FIL* fp,		/* Pointer to the file object */
	DWORD ofs		/* File pointer from top of file */
)
{
	FRESULT res;


	res = validate(fp);					/* Check validity of the object */
	if (res != FR_OK) LEAVE_FF(fp->fs, res);
	if (fp->err)						/* Check error */
		LEAVE_FF(fp->fs, (FRESULT)fp->err);

#if _USE_FASTSEEK
	if (fp->cltbl) {	/* Fast seek */
		DWORD cl, pcl, ncl, tcl, dsc, tlen, ulen, *tbl;

		if (ofs == CREATE_LINKMAP) {	/* Create CLMT */
			tbl = fp->cltbl;
			tlen = *tbl++; ulen = 2;	/* Given table size and required table size */
			cl = fp->sclust;			/* Top of the chain */
			if (cl) {
				do {
					/* Get a fragment */
					tcl = cl; ncl = 0; ulen += 2;	/* Top, length and used items */
					do {
						pcl = cl; ncl++;
						cl = get_fat(fp->fs, cl);
						if (cl <= 1) ABORT(fp->fs, FR_INT_ERR);
						if (cl == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
					} while (cl == pcl + 1);
					if (ulen <= tlen) {		/* Store the length and top of the fragment */
						*tbl++ = ncl; *tbl++ = tcl;
					}
				} while (cl < fp->fs->n_fatent);	/* Repeat until end of chain */
			}
			*fp->cltbl = ulen;	/* Number of items used */
			if (ulen <= tlen)
				*tbl = 0;		/* Terminate table */
			else
				res = FR_NOT_ENOUGH_CORE;	/* Given table size is smaller than required */

		} else {						/* Fast seek */
			if (ofs > fp->fsize)		/* Clip offset at the file size */
				ofs = fp->fsize;
			fp->fptr = ofs;				/* Set file pointer */
			if (ofs) {
				fp->clust = clmt_clust(fp, ofs - 1);
				dsc = clust2sect(fp->fs, fp->clust);
				if (!dsc) ABORT(fp->fs, FR_INT_ERR);
				dsc += (ofs - 1) / SS(fp->fs) & (fp->fs->csize - 1);
				if (fp->fptr % SS(fp->fs) && dsc != fp->dsect) {	/* Refill sector cache if needed */
#if !_FS_TINY
#if !_FS_READONLY
					if (fp->flag & FA__DIRTY) {		/* Write-back dirty sector cache */
						if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
							ABORT(fp->fs, FR_DISK_ERR);
						fp->flag &= ~FA__DIRTY;
					}
#endif
					if (disk_read(fp->fs->drv, fp->buf, dsc, 1))	/* Load current sector */
						ABORT(fp->fs, FR_DISK_ERR);
#endif
					fp->dsect = dsc;
				}
			}
		}
	} else
#endif

	/* Normal Seek */
	{
		DWORD clst, bcs, nsect, ifptr;

		if (ofs > fp->fsize					/* In read-only mode, clip offset with the file size */
#if !_FS_READONLY
			 && !(fp->flag & FA_WRITE)
#endif
			) ofs = fp->fsize;

		ifptr = fp->fptr;
		fp->fptr = nsect = 0;
		if (ofs) {
			bcs = (DWORD)fp->fs->csize * SS(fp->fs);	/* Cluster size (byte) */
			if (ifptr > 0 &&
				(ofs - 1) / bcs >= (ifptr - 1) / bcs) {	/* When seek to same or following cluster, */
				fp->fptr = (ifptr - 1) & ~(bcs - 1);	/* start from the current cluster */
				ofs -= fp->fptr;
				clst = fp->clust;
			} else {									/* When seek to back cluster, */
				clst = fp->sclust;						/* start from the first cluster */
#if !_FS_READONLY
				if (clst == 0) {						/* If no cluster chain, create a new chain */
					clst = create_chain(fp->fs, 0);
					if (clst == 1) ABORT(fp->fs, FR_INT_ERR);
					if (clst == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
					fp->sclust = clst;
				}
#endif
				fp->clust = clst;
			}
			if (clst != 0) {
				while (ofs > bcs) {						/* Cluster following loop */
#if !_FS_READONLY
					if (fp->flag & FA_WRITE) {			/* Check if in write mode or not */
						clst = create_chain(fp->fs, clst);	/* Force stretch if in write mode */
						if (clst == 0) {				/* When disk gets full, clip file size */
							ofs = bcs; break;
						}
					} else
#endif
						clst = get_fat(fp->fs, clst);	/* Follow cluster chain if not in write mode */
					if (clst == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
					if (clst <= 1 || clst >= fp->fs->n_fatent) ABORT(fp->fs, FR_INT_ERR);
					fp->clust = clst;
					fp->fptr += bcs;
					ofs -= bcs;
				}
				fp->fptr += ofs;
				if (ofs % SS(fp->fs)) {
					nsect = clust2sect(fp->fs, clst);	/* Current sector */
					if (!nsect) ABORT(fp->fs, FR_INT_ERR);
					nsect += ofs / SS(fp->fs);
				}
			}
		}
		if (fp->fptr % SS(fp->fs) && nsect != fp->dsect) {	/* Fill sector cache if needed */
#if !_FS_TINY
#if !_FS_READONLY
			if (fp->flag & FA__DIRTY) {			/* Write-back dirty sector cache */
				if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
					ABORT(fp->fs, FR_DISK_ERR);
				fp->flag &= ~FA__DIRTY;
			}
#endif
			if (disk_read(fp->fs->drv, fp->buf, nsect, 1))	/* Fill sector cache */
				ABORT(fp->fs, FR_DISK_ERR);
#endif
			fp->dsect = nsect;
		}
#if !_FS_READONLY
		if (fp->fptr > fp->fsize) {			/* Set file change flag if the file size is extended */
			fp->fsize = fp->fptr;
			fp->flag |= FA__WRITTEN;
		}
#endif
	}

	LEAVE_FF(fp->fs, res);
}



#if _FS_MINIMIZE <= 1
/*-----------------------------------------------------------------------*/
/* Create a Directory Object                                             */
/*-----------------------------------------------------------------------*/

FRESULT f_opendir (
	DIR* dp,			/* Pointer to directory object to create */
	const TCHAR* path	/* Pointer to the directory path */
)
{
	FRESULT res;
	FATFS* fs;
	DEF_NAMEBUF;


	if (!dp) return FR_INVALID_OBJECT;

	/* Get logical drive number */
	res = find_volume(&fs, &path, 0);
	if (res == FR_OK) {
		dp->fs = fs;
		INIT_BUF(*dp);
		res = follow_path(dp, path);			/* Follow the path to the directory */
		FREE_BUF();
		if (res == FR_OK) {						/* Follow completed */
			if (dp->dir) {						/* It is not the origin directory itself */
				//printf("%s: not only just /\r\n", __func__);
				if (dp->dir[DIR_Attr] & AM_DIR)	/* The object is a sub directory */
				{
					dp->sclust = ld_clust(fs, dp->dir);
					//printf("%s: The object is a sub directory, sclust(%d) \r\n", 
					//	__func__, dp->sclust);
				}
				else							/* The object is a file */
					res = FR_NO_PATH;
			}
			if (res == FR_OK) {
				dp->id = fs->id;
				res = dir_sdi(dp, 0);			/* Rewind directory */
				//printf("%s: dp->sclust(%d), dp->index(%d), dp->sect(%d)\r\n", 
				//	__func__, dp->sclust, dp->index, dp->sect);				
#if _FS_LOCK
				if (res == FR_OK) {
					if (dp->sclust) {
						dp->lockid = inc_lock(dp, 0);	/* Lock the sub directory */
						if (!dp->lockid)
							res = FR_TOO_MANY_OPEN_FILES;
					} else {
						dp->lockid = 0;	/* Root directory need not to be locked */
					}
				}
#endif
			}
		}
		if (res == FR_NO_FILE) res = FR_NO_PATH;
	}
	if (res != FR_OK) dp->fs = 0;		/* Invalidate the directory object if function faild */

	LEAVE_FF(fs, res);
}




/*-----------------------------------------------------------------------*/
/* Close Directory                                                       */
/*-----------------------------------------------------------------------*/

FRESULT f_closedir (
	DIR *dp		/* Pointer to the directory object to be closed */
)
{
	FRESULT res;


	res = validate(dp);
	if (res == FR_OK) {
#if _FS_REENTRANT
		FATFS *fs = dp->fs;
#endif			
#if _FS_LOCK
		if (dp->lockid)				/* Decrement sub-directory open counter */
			res = dec_lock(dp->lockid);
		if (res == FR_OK)
#endif
			dp->fs = 0;				/* Invalidate directory object */
#if _FS_REENTRANT
		//unlock_fs(fs, FR_OK);		/* Unlock volume */
		/*释放并且返回*/
		LEAVE_FF(fs, FR_OK);
#endif
	}
	/*释放并且返回*/
	LEAVE_FF(dp->fs,res);		
	//FATFS新版本的bug,必须加上这句,否则在使用OS的时候,可能导致关闭中断后,一直不开启,从而假死.
	//return res;
}




/*-----------------------------------------------------------------------*/
/* Read Directory Entries in Sequence                                    */
/*-----------------------------------------------------------------------*/

FRESULT f_readdir (
	DIR* dp,			/* Pointer to the open directory object */
	FILINFO* fno		/* Pointer to file information to return */
)
{
	FRESULT res;
	DEF_NAMEBUF;


	res = validate(dp);						/* Check validity of the object */
	if (res == FR_OK) {
		if (!fno) {
			res = dir_sdi(dp, 0);			/* Rewind the directory object */
		} else {
			INIT_BUF(*dp);
			res = dir_read(dp, 0);			/* Read an item */
			if (res == FR_NO_FILE) {		/* Reached end of directory */
				dp->sect = 0;
				res = FR_OK;
			}
			if (res == FR_OK) {				/* A valid entry is found */
				get_fileinfo(dp, fno);		/* Get the object information */
				res = dir_next(dp, 0);		/* Increment index for next */
				if (res == FR_NO_FILE) {
					dp->sect = 0;
					res = FR_OK;
				}
			}
			FREE_BUF();
		}
	}

	LEAVE_FF(dp->fs, res);
}



#if _FS_MINIMIZE == 0
/*-----------------------------------------------------------------------*/
/* Get File Status                                                       */
/*-----------------------------------------------------------------------*/

FRESULT f_stat (
	const TCHAR* path,	/* Pointer to the file path */
	FILINFO* fno		/* Pointer to file information to return */
)
{
	FRESULT res;
	DIR dj;
	DEF_NAMEBUF;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 0);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);	/* Follow the file path */
		if (res == FR_OK) {				/* Follow completed */
			if (dj.dir) {		/* Found an object */
				if (fno) get_fileinfo(&dj, fno);
			} else {			/* It is root directory */
				res = FR_INVALID_NAME;
			}
		}
		FREE_BUF();
	}

	LEAVE_FF(dj.fs, res);
}



#if !_FS_READONLY
/*-----------------------------------------------------------------------*/
/* Get Number of Free Clusters                                           */
/*-----------------------------------------------------------------------*/

FRESULT f_getfree (
	const TCHAR* path,	/* Path name of the logical drive number */
	DWORD* nclst,		/* Pointer to a variable to return number of free clusters */
	FATFS** fatfs		/* Pointer to return pointer to corresponding file system object */
)
{
	FRESULT res;
	FATFS *fs;
	DWORD n, clst, sect, stat;
	UINT i;
	BYTE fat, *p;


	/* Get logical drive number */
	res = find_volume(fatfs, &path, 0);
	fs = *fatfs;
	if (res == FR_OK) {
		/* If free_clust is valid, return it without full cluster scan */
		if (fs->free_clust <= fs->n_fatent - 2) {
			/*如果fs->free_clust有效，直接返回，不进行扫描*/
			*nclst = fs->free_clust;
		} else {
			/* Get number of free clusters */
			fat = fs->fs_type;
			n = 0;
			if (fat == FS_FAT12) {
				/*对于fat12从fat表中的第二项开始
				直到最后的n_fatent，原理是查看
				第一族是否有下一族，如果是0,带个空闲
				族，进行n++*/
				clst = 2;
				do {
					stat = get_fat(fs, clst);
					if (stat == 0xFFFFFFFF) { res = FR_DISK_ERR; break; }
					if (stat == 1) { res = FR_INT_ERR; break; }
					if (stat == 0) n++;
				} while (++clst < fs->n_fatent);
			} else {
				clst = fs->n_fatent;
				sect = fs->fatbase;
				i = 0; p = 0;
				do {
					if (!i) {
						res = move_window(fs, sect++);
						if (res != FR_OK) break;
						p = fs->win;
						i = SS(fs);
					}
					if (fat == FS_FAT16) {
						if (LD_WORD(p) == 0) n++;
						p += 2; i -= 2;
					} else {
						if ((LD_DWORD(p) & 0x0FFFFFFF) == 0) n++;
						p += 4; i -= 4;
					}
				} while (--clst);
			}
			fs->free_clust = n;
			fs->fsi_flag |= 1;
			*nclst = n;
		}
	}
	LEAVE_FF(fs, res);
}




/*-----------------------------------------------------------------------*/
/* Truncate File                                                         */
/*-----------------------------------------------------------------------*/

/**
The f_truncate function truncates the file size to the current file read/write pointer. 
This function has no effect if the file read/write pointer is already pointing end of the file.
这个函数是用来截断文件当前读写位置fptr后面的文件
数据，如果当前文件fptr在文件最后，则对文件没什么影响。
**/
FRESULT f_truncate (
	FIL* fp		/* Pointer to the file object */
)
{
	FRESULT res;
	DWORD ncl;


	res = validate(fp);						/* Check validity of the object */
	if (res == FR_OK) {
		if (fp->err) {						/* Check error */
			res = (FRESULT)fp->err;
		} else {
			if (!(fp->flag & FA_WRITE))		/* Check access mode */
				res = FR_DENIED;
		}
	}
	if (res == FR_OK) {
		if (fp->fsize > fp->fptr) {
			/*把当前文件的大小设置了读写位置*/
			fp->fsize = fp->fptr;	/* Set file size to current R/W point */
			fp->flag |= FA__WRITTEN;
			if (fp->fptr == 0) {	/* When set file size to zero, remove entire cluster chain */
				/*当文件内容全被删除
				删除整条文件的族链*/
				res = remove_chain(fp->fs, fp->sclust);
				fp->sclust = 0;
			} else {				/* When truncate a part of the file, remove remaining clusters */
				ncl = get_fat(fp->fs, fp->clust);
				res = FR_OK;
				if (ncl == 0xFFFFFFFF) res = FR_DISK_ERR;
				if (ncl == 1) res = FR_INT_ERR;
				if (res == FR_OK && ncl < fp->fs->n_fatent) {
					/*把当前族设置成最后一族0x0FFFFFFF->0xFFFF FF0F*/
					res = put_fat(fp->fs, fp->clust, 0x0FFFFFFF);
					if (res == FR_OK) res = remove_chain(fp->fs, ncl);
				}
			}
#if !_FS_TINY
			if (res == FR_OK && (fp->flag & FA__DIRTY)) {
				if (disk_write(fp->fs->drv, fp->buf, fp->dsect, 1))
					res = FR_DISK_ERR;
				else
					fp->flag &= ~FA__DIRTY;
			}
#endif
		}
		if (res != FR_OK) fp->err = (FRESULT)res;
	}

	LEAVE_FF(fp->fs, res);
}




/*-----------------------------------------------------------------------*/
/* Delete a File or Directory                                            */
/*-----------------------------------------------------------------------*/

FRESULT f_unlink (
	const TCHAR* path		/* Pointer to the file or directory path */
)
{
	FRESULT res;
	DIR dj, sdj;
	BYTE *dir;
	DWORD dclst;
	DEF_NAMEBUF;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 1);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);		/* Follow the file path */
		if (_FS_RPATH && res == FR_OK && (dj.fn[NS] & NS_DOT))
			res = FR_INVALID_NAME;			/* Cannot remove dot entry */
#if _FS_LOCK
		if (res == FR_OK) res = chk_lock(&dj, 2);	/* Cannot remove open file */
#endif
		if (res == FR_OK) {					/* The object is accessible */
			dir = dj.dir;
			if (!dir) {
				res = FR_INVALID_NAME;		/* Cannot remove the start directory */
			} else {
				if (dir[DIR_Attr] & AM_RDO)
					res = FR_DENIED;		/* Cannot remove R/O object */
			}
			dclst = ld_clust(dj.fs, dir);
			if (res == FR_OK && (dir[DIR_Attr] & AM_DIR)) {	/* Is it a sub-dir? */
				if (dclst < 2) {
					res = FR_INT_ERR;
				} else {
					mem_cpy(&sdj, &dj, sizeof (DIR));	/* Check if the sub-directory is empty or not */
					sdj.sclust = dclst;
					/*接下来查看子目录下，是否
					有文件或者目录，因为index=1被占用，
					只需要查看index=2是否有目录项存在
					就可是知道子目录下是否有东西了*/
					res = dir_sdi(&sdj, 2);		/* Exclude dot entries */
					if (res == FR_OK) {
						res = dir_read(&sdj, 0);	/* Read an item */
						if (res == FR_OK		/* Not empty directory */
#if _FS_RPATH
						|| dclst == dj.fs->cdir	/* Current directory */
#endif
						) res = FR_DENIED;
						if (res == FR_NO_FILE) res = FR_OK;	/* Empty */
					}
				}
			}
			if (res == FR_OK) {
				res = dir_remove(&dj);		/* Remove the directory entry */
				if (res == FR_OK) {
					if (dclst)				/* Remove the cluster chain if exist */
						res = remove_chain(dj.fs, dclst);
					if (res == FR_OK) res = sync_fs(dj.fs);
				}
			}
		}
		FREE_BUF();
	}

	LEAVE_FF(dj.fs, res);
}




/*-----------------------------------------------------------------------*/
/* Create a Directory                                                    */
/*-----------------------------------------------------------------------*/

FRESULT f_mkdir (
	const TCHAR* path		/* Pointer to the directory path */
)
{
	FRESULT res;
	DIR dj;
	BYTE *dir, n;
	DWORD dsc, dcl, pcl, tm = get_fattime();
	DEF_NAMEBUF;

	printf("%s: create dir(%s) start...\r\n", __func__, path);
	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 1);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);			/* Follow the file path */
		if (res == FR_OK) res = FR_EXIST;		/* Any object with same name is already existing */
		if (_FS_RPATH && res == FR_NO_FILE && (dj.fn[NS] & NS_DOT))
			res = FR_INVALID_NAME;
		if (res == FR_NO_FILE) {				/* Can create a new directory */
			dcl = create_chain(dj.fs, 0);		/* Allocate a cluster for the new directory table */
			printf("%s: Allocate a cluster for the new directory(%d)\r\n", 
				__func__, dcl);
			/*dcl为新的族头*/
			res = FR_OK;
			if (dcl == 0) res = FR_DENIED;		/* No space to allocate a new cluster */
			if (dcl == 1) res = FR_INT_ERR;
			if (dcl == 0xFFFFFFFF) res = FR_DISK_ERR;
			if (res == FR_OK)					/* Flush FAT */
				res = sync_window(dj.fs);
			if (res == FR_OK) {					/* Initialize the new directory table */
				dsc = clust2sect(dj.fs, dcl);
				dir = dj.fs->win;
				/*清除缓存win空间，dir代表新目录下的
				第0个目录项，就是被"." 目录用去了*/
				mem_set(dir, 0, SS(dj.fs));
				mem_set(dir+DIR_Name, ' ', 11);	/* Create "." entry */
				dir[DIR_Name] = '.';
				dir[DIR_Attr] = AM_DIR;
				ST_DWORD(dir+DIR_WrtTime, tm);
				st_clust(dir, dcl);
				/*新目录下的第二个目录项
				被".." 占用去*/
				mem_cpy(dir+SZ_DIR, dir, SZ_DIR); 	/* Create ".." entry */
				dir[SZ_DIR+1] = '.'; pcl = dj.sclust;
				/*pcl应该代表新目录的父亲目录的sclust
					因为follow_path 后，dir.sclust一直没有被修改*/
				if (dj.fs->fs_type == FS_FAT32 && pcl == dj.fs->dirbase)
					pcl = 0;
				/*把父亲目录的开始族，放到".."目录项的空间
				中，也就是，通过".."的目录项，可以查到父亲
				目录的族，知道了目录的开始族，也就是可以
				回到父亲目录了*/
				st_clust(dir+SZ_DIR, pcl);
				for (n = dj.fs->csize; n; n--) {	/* Write dot entries and clear following sectors */
					dj.fs->winsect = dsc++;
					dj.fs->wflag = 1;
					res = sync_window(dj.fs);
					/*第一次回写时，前64bytes包含了"."和".."的
					目录项信息，逻辑扇区中其它的空间为0
					，第二次开始，缓存中的数据都是0*/
					if (res != FR_OK) break;
					mem_set(dir, 0, SS(dj.fs));
				}
			}
			if (res == FR_OK) res = dir_register(&dj);	/* Register the object to the directoy */
			if (res != FR_OK) {
				remove_chain(dj.fs, dcl);			/* Could not register, remove cluster chain */
			} else {
				dir = dj.dir;
				dir[DIR_Attr] = AM_DIR;				/* Attribute */
				ST_DWORD(dir+DIR_WrtTime, tm);		/* Created time */
				/*当前目录项信息包含dcl 新的族头*/
				st_clust(dir, dcl);					/* Table start cluster */
				dj.fs->wflag = 1;
				res = sync_fs(dj.fs);
			}
		}
		FREE_BUF();
	}

	LEAVE_FF(dj.fs, res);
}




/*-----------------------------------------------------------------------*/
/* Change Attribute                                                      */
/*-----------------------------------------------------------------------*/

FRESULT f_chmod (
	const TCHAR* path,	/* Pointer to the file path */
	BYTE value,			/* Attribute bits */
	BYTE mask			/* Attribute mask to change */
)
{
	FRESULT res;
	DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 1);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);		/* Follow the file path */
		FREE_BUF();
		if (_FS_RPATH && res == FR_OK && (dj.fn[NS] & NS_DOT))
			res = FR_INVALID_NAME;
		if (res == FR_OK) {
			dir = dj.dir;
			if (!dir) {						/* Is it a root directory? */
				res = FR_INVALID_NAME;
			} else {						/* File or sub directory */
				mask &= AM_RDO|AM_HID|AM_SYS|AM_ARC;	/* Valid attribute mask */
				dir[DIR_Attr] = (value & mask) | (dir[DIR_Attr] & (BYTE)~mask);	/* Apply attribute change */
				dj.fs->wflag = 1;
				res = sync_fs(dj.fs);
			}
		}
	}

	LEAVE_FF(dj.fs, res);
}




/*-----------------------------------------------------------------------*/
/* Change Timestamp                                                      */
/*-----------------------------------------------------------------------*/

FRESULT f_utime (
	const TCHAR* path,	/* Pointer to the file/directory name */
	const FILINFO* fno	/* Pointer to the time stamp to be set */
)
{
	FRESULT res;
	DIR dj;
	BYTE *dir;
	DEF_NAMEBUF;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 1);
	if (res == FR_OK) {
		INIT_BUF(dj);
		res = follow_path(&dj, path);	/* Follow the file path */
		FREE_BUF();
		if (_FS_RPATH && res == FR_OK && (dj.fn[NS] & NS_DOT))
			res = FR_INVALID_NAME;
		if (res == FR_OK) {
			dir = dj.dir;
			if (!dir) {					/* Root directory */
				res = FR_INVALID_NAME;
			} else {					/* File or sub-directory */
				ST_WORD(dir+DIR_WrtTime, fno->ftime);
				ST_WORD(dir+DIR_WrtDate, fno->fdate);
				dj.fs->wflag = 1;
				res = sync_fs(dj.fs);
			}
		}
	}

	LEAVE_FF(dj.fs, res);
}




/*-----------------------------------------------------------------------*/
/* Rename File/Directory                                                 */
/*-----------------------------------------------------------------------*/

FRESULT f_rename (
	const TCHAR* path_old,	/* Pointer to the object to be renamed */
	const TCHAR* path_new	/* Pointer to the new name */
)
{
	FRESULT res;
	DIR djo, djn;
	BYTE buf[21], *dir;
	DWORD dw;
	DEF_NAMEBUF;


	/* Get logical drive number of the source object */
	res = find_volume(&djo.fs, &path_old, 1);
	if (res == FR_OK) {
		djn.fs = djo.fs;
		INIT_BUF(djo);
		res = follow_path(&djo, path_old);		/* Check old object */
		/*dp->sclust代表的是
		文件or目录的父亲目录的族*/		
		if (_FS_RPATH && res == FR_OK && (djo.fn[NS] & NS_DOT))
			res = FR_INVALID_NAME;
#if _FS_LOCK
		if (res == FR_OK) res = chk_lock(&djo, 2);
#endif
		if (res == FR_OK) {						/* Old object is found */
			if (!djo.dir) {						/* Is root dir? */
				res = FR_NO_FILE;
			} else {
				mem_cpy(buf, djo.dir+DIR_Attr, 21);		/* Save the object information except name */
				mem_cpy(&djn, &djo, sizeof (DIR));		/* Duplicate the directory object */
				if (get_ldnumber(&path_new) >= 0)		/* Snip drive number off and ignore it */
					res = follow_path(&djn, path_new);	/* and check if new object is exist */
				else
					res = FR_INVALID_DRIVE;
				if (res == FR_OK) res = FR_EXIST;		/* The new object name is already existing */
				if (res == FR_NO_FILE) { 				/* Is it a valid path and no name collision? */
/* Start critical section that any interruption can cause a cross-link */
					res = dir_register(&djn);			/* Register the new entry */
					if (res == FR_OK) {
						dir = djn.dir;					/* Copy object information except name */
						mem_cpy(dir+13, buf+2, 19);
						dir[DIR_Attr] = buf[0] | AM_ARC;
						djo.fs->wflag = 1;
						if (djo.sclust != djn.sclust && (dir[DIR_Attr] & AM_DIR)) {		/* Update .. entry in the directory if needed */
							/*如果是目录，要更新父亲目录的目录项信息*/
							dw = clust2sect(djo.fs, ld_clust(djo.fs, dir));
							if (!dw) {
								res = FR_INT_ERR;
							} else {
								res = move_window(djo.fs, dw);
								dir = djo.fs->win+SZ_DIR;	/* .. entry */
								/*此时，dir为父亲目录的目录项信息，
								保存了父亲目录的开始族*/
								if (res == FR_OK && dir[1] == '.') {
									dw = (djo.fs->fs_type == FS_FAT32 && djn.sclust == djo.fs->dirbase) ? 0 : djn.sclust;
									//修改父亲目录的开始族，
									//也就是新目录跟旧目录的父亲目录不一样
									//证明这个目录的修改，不是发生在同一级目录下
									//比如如mv 	/user/abc	/abc
									printf("%s: mv 	/user/abc  /abc !\r\n", __func__);
									printf("%s: djo.sclust(%d) djn.sclust(%d)\r\n", __func__, djo.sclust, djn.sclust);
									st_clust(dir, dw);
									djo.fs->wflag = 1;
								}
							}
						}
						else if (djo.sclust == djn.sclust && (dir[DIR_Attr] & AM_DIR))
						{
							//add for debug..
							printf("%s: mv	/user/abc  /user/abc1 !!!!!\r\n", __func__);
							printf("%s: djo.sclust(%d) djn.sclust(%d)\r\n", __func__, djo.sclust, djn.sclust);
						}
						if (res == FR_OK) {
							/*删除旧的目录项信息*/
							res = dir_remove(&djo);		/* Remove old entry */
							if (res == FR_OK)
								res = sync_fs(djo.fs);
						}
					}
/* End critical section */
				}
			}
		}
		FREE_BUF();
	}

	LEAVE_FF(djo.fs, res);
}

#endif /* !_FS_READONLY */
#endif /* _FS_MINIMIZE == 0 */
#endif /* _FS_MINIMIZE <= 1 */
#endif /* _FS_MINIMIZE <= 2 */



#if _USE_LABEL
/*-----------------------------------------------------------------------*/
/* Get volume label                                                      */
/*-----------------------------------------------------------------------*/

FRESULT f_getlabel (
	const TCHAR* path,	/* Path name of the logical drive number */
	TCHAR* label,		/* Pointer to a buffer to return the volume label */
	DWORD* vsn			/* Pointer to a variable to return the volume serial number */
)
{
	FRESULT res;
	DIR dj;
	UINT i, j;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &path, 0);

	/* Get volume label */
	if (res == FR_OK && label) {
		dj.sclust = 0;					/* Open root directory */
		res = dir_sdi(&dj, 0);
		if (res == FR_OK) {
			res = dir_read(&dj, 1);		/* Get an entry with AM_VOL */
			if (res == FR_OK) {			/* A volume label is exist */
#if _USE_LFN && _LFN_UNICODE
				WCHAR w;
				i = j = 0;
				do {
					w = (i < 11) ? dj.dir[i++] : ' ';
					if (IsDBCS1(w) && i < 11 && IsDBCS2(dj.dir[i]))
						w = w << 8 | dj.dir[i++];
					label[j++] = ff_convert(w, 1);	/* OEM -> Unicode */
				} while (j < 11);
#else
				mem_cpy(label, dj.dir, 11);
#endif
				j = 11;
				do {
					label[j] = 0;
					if (!j) break;
				} while (label[--j] == ' ');
			}
			if (res == FR_NO_FILE) {	/* No label, return nul string */
				label[0] = 0;
				res = FR_OK;
			}
		}
	}

	/* Get volume serial number */
	if (res == FR_OK && vsn) {
		res = move_window(dj.fs, dj.fs->volbase);
		if (res == FR_OK) {
			i = dj.fs->fs_type == FS_FAT32 ? BS_VolID32 : BS_VolID;
			/* Volume serial number (4 bytes ) 
				卷标的串码放在DBR扇区中*/			
			*vsn = LD_DWORD(&dj.fs->win[i]);
		}
	}

	LEAVE_FF(dj.fs, res);
}



#if !_FS_READONLY
/*-----------------------------------------------------------------------*/
/* Set volume label                                                      */
/*-----------------------------------------------------------------------*/

FRESULT f_setlabel (
	const TCHAR* label	/* Pointer to the volume label to set */
)
{
	FRESULT res;
	DIR dj;
	BYTE vn[11];
	UINT i, j, sl;
	WCHAR w;
	DWORD tm;


	/* Get logical drive number */
	res = find_volume(&dj.fs, &label, 1);
	if (res) LEAVE_FF(dj.fs, res);

	/* Create a volume label in directory form */
	vn[0] = 0;
	for (sl = 0; label[sl]; sl++) ;				/* Get name length */
	for ( ; sl && label[sl-1] == ' '; sl--) ;	/* Remove trailing spaces */
	if (sl) {	/* Create volume label in directory form */
		i = j = 0;
		do {
#if _USE_LFN && _LFN_UNICODE
			w = ff_convert(ff_wtoupper(label[i++]), 0);
#else
			w = (BYTE)label[i++];
			if (IsDBCS1(w))
				w = (j < 10 && i < sl && IsDBCS2(label[i])) ? w << 8 | (BYTE)label[i++] : 0;
#if _USE_LFN
			w = ff_convert(ff_wtoupper(ff_convert(w, 1)), 0);
#else
			if (IsLower(w)) w -= 0x20;			/* To upper ASCII characters */
#ifdef _EXCVT
			if (w >= 0x80) w = ExCvt[w - 0x80];	/* To upper extended characters (SBCS cfg) */
#else
			if (!_DF1S && w >= 0x80) w = 0;		/* Reject extended characters (ASCII cfg) */
#endif
#endif
#endif
			if (!w || chk_chr("\"*+,.:;<=>\?[]|\x7F", w) || j >= (UINT)((w >= 0x100) ? 10 : 11)) /* Reject invalid characters for volume label */
				LEAVE_FF(dj.fs, FR_INVALID_NAME);
			if (w >= 0x100) vn[j++] = (BYTE)(w >> 8);
			vn[j++] = (BYTE)w;
		} while (i < sl);
		while (j < 11) vn[j++] = ' ';
	}

	/* Set volume label */
	dj.sclust = 0;					/* Open root directory */
	/*把卷标放在root目录下的一个目录项中
	第11个字节为0x08，卷名字放第0字节开始
	11个字节中*/
	res = dir_sdi(&dj, 0);
	if (res == FR_OK) {
		res = dir_read(&dj, 1);		/* Get an entry with AM_VOL */
		if (res == FR_OK) {			/* A volume label is found */
			if (vn[0]) {
				mem_cpy(dj.dir, vn, 11);	/* Change the volume label name */
				tm = get_fattime();
				ST_DWORD(dj.dir+DIR_WrtTime, tm);
			} else {
				dj.dir[0] = DDE;			/* Remove the volume label */
			}
			dj.fs->wflag = 1;
			res = sync_fs(dj.fs);
		} else {					/* No volume label is found or error */
			if (res == FR_NO_FILE) {
				res = FR_OK;
				if (vn[0]) {				/* Create volume label as new */
					res = dir_alloc(&dj, 1);	/* Allocate an entry for volume label */
					if (res == FR_OK) {
						mem_set(dj.dir, 0, SZ_DIR);	/* Set volume label */
						mem_cpy(dj.dir, vn, 11);
						dj.dir[DIR_Attr] = AM_VOL;
						tm = get_fattime();
						ST_DWORD(dj.dir+DIR_WrtTime, tm);
						dj.fs->wflag = 1;
						res = sync_fs(dj.fs);
					}
				}
			}
		}
	}

	LEAVE_FF(dj.fs, res);
}

#endif /* !_FS_READONLY */
#endif /* _USE_LABEL */



/*-----------------------------------------------------------------------*/
/* Forward data to the stream directly (available on only tiny cfg)      */
/*-----------------------------------------------------------------------*/
#if _USE_FORWARD && _FS_TINY

FRESULT f_forward (
	FIL* fp, 						/* Pointer to the file object */
	UINT (*func)(const BYTE*,UINT),	/* Pointer to the streaming function */
	UINT btf,						/* Number of bytes to forward */
	UINT* bf						/* Pointer to number of bytes forwarded */
)
{
	FRESULT res;
	DWORD remain, clst, sect;
	UINT rcnt;
	BYTE csect;


	*bf = 0;	/* Clear transfer byte counter */

	res = validate(fp);								/* Check validity of the object */
	if (res != FR_OK) LEAVE_FF(fp->fs, res);
	if (fp->err)									/* Check error */
		LEAVE_FF(fp->fs, (FRESULT)fp->err);
	if (!(fp->flag & FA_READ))						/* Check access mode */
		LEAVE_FF(fp->fs, FR_DENIED);

	remain = fp->fsize - fp->fptr;
	if (btf > remain) btf = (UINT)remain;			/* Truncate btf by remaining bytes */

	for ( ;  btf && (*func)(0, 0);					/* Repeat until all data transferred or stream becomes busy */
		fp->fptr += rcnt, *bf += rcnt, btf -= rcnt) {
		csect = (BYTE)(fp->fptr / SS(fp->fs) & (fp->fs->csize - 1));	/* Sector offset in the cluster */
		if ((fp->fptr % SS(fp->fs)) == 0) {			/* On the sector boundary? */
			if (!csect) {							/* On the cluster boundary? */
				clst = (fp->fptr == 0) ?			/* On the top of the file? */
					fp->sclust : get_fat(fp->fs, fp->clust);
				if (clst <= 1) ABORT(fp->fs, FR_INT_ERR);
				if (clst == 0xFFFFFFFF) ABORT(fp->fs, FR_DISK_ERR);
				fp->clust = clst;					/* Update current cluster */
			}
		}
		sect = clust2sect(fp->fs, fp->clust);		/* Get current data sector */
		if (!sect) ABORT(fp->fs, FR_INT_ERR);
		sect += csect;
		if (move_window(fp->fs, sect))				/* Move sector window */
			ABORT(fp->fs, FR_DISK_ERR);
		fp->dsect = sect;
		rcnt = SS(fp->fs) - (WORD)(fp->fptr % SS(fp->fs));	/* Forward data from sector window */
		if (rcnt > btf) rcnt = btf;
		rcnt = (*func)(&fp->fs->win[(WORD)fp->fptr % SS(fp->fs)], rcnt);
		if (!rcnt) ABORT(fp->fs, FR_INT_ERR);
	}

	LEAVE_FF(fp->fs, FR_OK);
}
#endif /* _USE_FORWARD */



#if _USE_MKFS && !_FS_READONLY
/*-----------------------------------------------------------------------*/
/* Create File System on the Drive                                       */
/*-----------------------------------------------------------------------*/
#define N_ROOTDIR	16/*512*/		/* Number of root directory entries for FAT12/16 */
#define N_FATS		1		/* Number of FAT copies (1 or 2) */

/*
[root]1:/#mf_fmkfs("1:", 1, 1024)
W25QXX_Init: W25QXX_TYPE = 0XEF14
f_mkfs: n_vol = 4096
f_mkfs: Number of sectors per cluster = 2
f_mkfs: fmt = 1, n_clst = 2048, FS_FAT12=1, FS_FAT16=2, FS_FAT32=3
f_mkfs: n_fat(7), b_fat(1), b_dir(8), b_data(40)
f_mkfs: b_fat(1)
f_mkfs: write VBR to 0 sector!

*/
FRESULT f_mkfs (
	const TCHAR* path,	/* Logical drive number */
	BYTE sfd,			/* Partitioning rule 0:FDISK, 1:SFD */
	UINT au				/* Allocation unit [bytes] */
)
{
	static const WORD vst[] = { 1024,   512,  256,  128,   64,    32,   16,    8,    4,    2,   0};
	static const WORD cst[] = {32768, 16384, 8192, 4096, 2048, 16384, 8192, 4096, 2048, 1024, 512};
	int vol;
	BYTE fmt, md, sys, *tbl, pdrv, part;
	DWORD n_clst, vs, n, wsect;
	UINT i;
	DWORD b_vol, b_fat, b_dir, b_data;	/* LBA */
	DWORD n_vol, n_rsv, n_fat, n_dir;	/* Size */
	FATFS *fs;
	DSTATUS stat;


	/* Check mounted drive and clear work area */
	vol = get_ldnumber(&path);
	if (vol < 0) return FR_INVALID_DRIVE;
	if (sfd > 1) return FR_INVALID_PARAMETER;
	if (au & (au - 1)) return FR_INVALID_PARAMETER;
	fs = FatFs[vol];
	if (!fs) return FR_NOT_ENABLED;
	fs->fs_type = 0;
	pdrv = LD2PD(vol);	/* Physical drive */
	part = LD2PT(vol);	/* Partition (0:auto detect, 1-4:get from partition table)*/

	/* Get disk statics */
	stat = disk_initialize(pdrv);
	if (stat & STA_NOINIT) return FR_NOT_READY;
	if (stat & STA_PROTECT) return FR_WRITE_PROTECTED;
#if _MAX_SS != _MIN_SS		/* Get disk sector size */
	if (disk_ioctl(pdrv, GET_SECTOR_SIZE, &SS(fs)) != RES_OK || SS(fs) > _MAX_SS || SS(fs) < _MIN_SS)
		return FR_DISK_ERR;
#endif
	if (_MULTI_PARTITION && part) {
		/* Get partition information from partition table in the MBR */
		if (disk_read(pdrv, fs->win, 0, 1)) return FR_DISK_ERR;
		if (LD_WORD(fs->win+BS_55AA) != 0xAA55) return FR_MKFS_ABORTED;
		tbl = &fs->win[MBR_Table + (part - 1) * SZ_PTE];
		if (!tbl[4]) return FR_MKFS_ABORTED;	/* No partition? */
		b_vol = LD_DWORD(tbl+8);	/* Volume start sector */
		n_vol = LD_DWORD(tbl+12);	/* Volume size */
	} else {
		/* Create a partition in this function */
		printf("GET_SECTOR_COUNT\r\n");
		if (disk_ioctl(pdrv, GET_SECTOR_COUNT, &n_vol) != RES_OK || n_vol < /*128*/6)
			return FR_DISK_ERR;
		b_vol = (sfd) ? 0 : 63;		/* Volume start sector */
		/*没有引导分区， b_vol = 0,    n_vol 为卷的扇区数目*/
		n_vol -= b_vol;				/* Volume size */
		printf("b_vol=%d\r\n", b_vol);
	}

	printf("%s: n_vol = %d\r\n", __func__ , n_vol );
	
	if (!au) {				/* AU auto selection */
		vs = n_vol / (2000 / (SS(fs) / 512));
		for (i = 0; vs < vst[i]; i++) ;
		au = cst[i];
	}
	au /= SS(fs);		/* Number of sectors per cluster */
	if (au == 0) au = 1;
	if (au > 128) au = 128;
	printf("%s: Number of sectors per cluster = %d\r\n", __func__, au);
	/* Pre-compute number of clusters and FAT sub-type */
	n_clst = n_vol / au;
	fmt = FS_FAT12;
	if (n_clst >= MIN_FAT16) fmt = FS_FAT16;
	if (n_clst >= MIN_FAT32) fmt = FS_FAT32;

	printf("%s: fmt = %d, n_clst = %d, FS_FAT12=1, FS_FAT16=2, FS_FAT32=3\r\n",
		__func__, fmt, n_clst);

	/* Determine offset and size of FAT structure */
	if (fmt == FS_FAT32) {
		n_fat = ((n_clst * 4) + 8 + SS(fs) - 1) / SS(fs);
		n_rsv = 32;
		n_dir = 0;
	} else {
		n_fat = (fmt == FS_FAT12) ? (n_clst * 3 + 1) / 2 + 3 : (n_clst * 2) + 4;
		n_fat = (n_fat + SS(fs) - 1) / SS(fs);
		n_rsv = 1;
		/*for fat12\fat16 , it has reserved 1 sector for VBR?*/
		n_dir = (DWORD)N_ROOTDIR * SZ_DIR / SS(fs);
	}
	/*文件分配表is FAT ?*/
	b_fat = b_vol + n_rsv;				/* FAT area start sector */
	b_dir = b_fat + n_fat * N_FATS;		/* Directory area start sector */
	b_data = b_dir + n_dir;				/* Data area start sector */
	/*目录区+ FAT  可以计算出---->file addr in flash?*/
	printf("%s: n_fat(%d), b_fat(%d), b_dir(%d), b_data(%d)\r\n", 
		__func__, n_fat, b_fat, b_dir, b_data );

	/*
		[root]1:/#mf_fmkfs("1:", 1, 4096)
		W25QXX_Init: W25QXX_TYPE = 0XEF14
		f_mkfs: n_vol = 4096
		f_mkfs: Number of sectors per cluster = 8
		f_mkfs: fmt = 1, n_clst = 512, FS_FAT12=1, FS_FAT16=2, FS_FAT32=3
		f_mkfs: n_fat(2), b_fat(1), b_dir(3), b_data(35)
		f_mkfs: b_fat(1)
	*/
	if (n_vol < b_data + au - b_vol) return FR_MKFS_ABORTED;	/* Too small volume */

	/* Align data start sector to erase block boundary (for flash memory media) */
	if (disk_ioctl(pdrv, GET_BLOCK_SIZE, &n) != RES_OK || !n || n > 32768) n = 1;
	n = (b_data + n - 1) & ~(n - 1);	/* Next nearest erase block from current data start */
	n = (n - b_data) / N_FATS;
	if (fmt == FS_FAT32) {		/* FAT32: Move FAT offset */
		n_rsv += n;
		b_fat += n;
	} else {					/* FAT12/16: Expand FAT size */
		n_fat += n;
	}
	printf("%s: b_fat(%d)\r\n", __func__, b_fat);

	/* Determine number of clusters and final check of validity of the FAT sub-type */
	n_clst = (n_vol - n_rsv - n_fat * N_FATS - n_dir) / au;
	if (   (fmt == FS_FAT16 && n_clst < MIN_FAT16)
		|| (fmt == FS_FAT32 && n_clst < MIN_FAT32))
		return FR_MKFS_ABORTED;

	/* Determine system ID in the partition table */
	if (fmt == FS_FAT32) {
		sys = 0x0C;		/* FAT32X */
	} else {
		if (fmt == FS_FAT12 && n_vol < 0x10000) {
			sys = 0x01;	/* FAT12(<65536) */
		} else {
			sys = (n_vol < 0x10000) ? 0x04 : 0x06;	/* FAT16(<65536) : FAT12/16(>=65536) */
		}
	}

	if (_MULTI_PARTITION && part) {
		/* Update system ID in the partition table */
		tbl = &fs->win[MBR_Table + (part - 1) * SZ_PTE];
		tbl[4] = sys;
		if (disk_write(pdrv, fs->win, 0, 1))	/* Write it to teh MBR */
			return FR_DISK_ERR;
		md = 0xF8;
	} else {
		if (sfd) {	/* No partition table (SFD) */
			md = 0xF0;
		} else {	/* Create partition table (FDISK) */
			mem_set(fs->win, 0, SS(fs));
			tbl = fs->win+MBR_Table;	/* Create partition table for single partition in the drive */
			tbl[1] = 1;						/* Partition start head */
			tbl[2] = 1;						/* Partition start sector */
			tbl[3] = 0;						/* Partition start cylinder */
			tbl[4] = sys;					/* System type */
			tbl[5] = 254;					/* Partition end head */
			n = (b_vol + n_vol) / 63 / 255;
			tbl[6] = (BYTE)(n >> 2 | 63);	/* Partition end sector */
			tbl[7] = (BYTE)n;				/* End cylinder */
			ST_DWORD(tbl+8, 63);			/* Partition start in LBA */
			ST_DWORD(tbl+12, n_vol);		/* Partition size in LBA */
			ST_WORD(fs->win+BS_55AA, 0xAA55);	/* MBR signature */
/*MBR （Main Boot Record）,按其字面上的理解即为主引导记录区
不过,在总共512字节的主引导扇区中,
MBR只占用了其中的 446个字节（偏移0--偏移1BDH）,
另外的64个字节（偏移1BEH--偏移1FDH）交给了
DPT(Disk Partition Table硬盘分区表)（见下表）,最后两个字节
"55,AA"（偏移1FEH- 偏移1FFH）是分区的结束标志。
这个整体构成了硬盘的主引导扇区。
*/
			if (disk_write(pdrv, fs->win, 0, 1))	/* Write it to the MBR */
				return FR_DISK_ERR;
			md = 0xF8;
		}
	}

//卷引导记录(VBR, Volume Boot Record), 又称为：
//卷引导扇区(Volume Boot Sector), DBR,
// BIOS Parameter Block (BPB)

	/* Create BPB in the VBR */
	tbl = fs->win;							/* Clear sector */
	mem_set(tbl, 0, SS(fs));
	mem_cpy(tbl, "\xEB\xFE\x90" "MSDOS5.0", 11);/* Boot jump code, OEM name */
	i = SS(fs);								/* Sector size */
	ST_WORD(tbl+BPB_BytsPerSec, i);
	tbl[BPB_SecPerClus] = (BYTE)au;			/* Sectors per cluster */
	ST_WORD(tbl+BPB_RsvdSecCnt, n_rsv);		/* Reserved sectors */
	tbl[BPB_NumFATs] = N_FATS;				/* Number of FATs */
	i = (fmt == FS_FAT32) ? 0 : N_ROOTDIR;	/* Number of root directory entries */
	/*目录项个数，对于fat12/16 为N_ROOTDIR个*/
	ST_WORD(tbl+BPB_RootEntCnt, i);
	if (n_vol < 0x10000) {					/* Number of total sectors */
		ST_WORD(tbl+BPB_TotSec16, n_vol);
	} else {
		ST_DWORD(tbl+BPB_TotSec32, n_vol);
	}
	tbl[BPB_Media] = md;					/* Media descriptor */
	ST_WORD(tbl+BPB_SecPerTrk, 63);			/* Number of sectors per track */
	ST_WORD(tbl+BPB_NumHeads, 255);			/* Number of heads */
	/*隐藏的分区，SFD b_vol = 0, 	 FDISK: b_vol = x*/
	ST_DWORD(tbl+BPB_HiddSec, b_vol);		/* Hidden sectors */
	n = get_fattime();						/* Use current time as VSN */
	if (fmt == FS_FAT32) {
		ST_DWORD(tbl+BS_VolID32, n);		/* VSN */
		ST_DWORD(tbl+BPB_FATSz32, n_fat);	/* Number of sectors per FAT */
		ST_DWORD(tbl+BPB_RootClus, 2);		/* Root directory start cluster (2) */
		ST_WORD(tbl+BPB_FSInfo, 1);			/* FSINFO record offset (VBR+1) */
		ST_WORD(tbl+BPB_BkBootSec, 6);		/* Backup boot record offset (VBR+6) */
		tbl[BS_DrvNum32] = 0x80;			/* Drive number */
		tbl[BS_BootSig32] = 0x29;			/* Extended boot signature */
		mem_cpy(tbl+BS_VolLab32, "NO NAME    " "FAT32   ", 19);	/* Volume label, FAT signature */
	} else {
		ST_DWORD(tbl+BS_VolID, n);			/* VSN */
		ST_WORD(tbl+BPB_FATSz16, n_fat);	/* Number of sectors per FAT */
		tbl[BS_DrvNum] = 0x80;				/* Drive number */
		tbl[BS_BootSig] = 0x29;				/* Extended boot signature */
		mem_cpy(tbl+BS_VolLab, "NO NAME    " "FAT     ", 19);	/* Volume label, FAT signature */
	}
	ST_WORD(tbl+BS_55AA, 0xAA55);			/* Signature (Offset is fixed here regardless of sector size) */
	printf("%s: write VBR to %d sector!\r\n", __func__ , b_vol );
	if (disk_write(pdrv, tbl, b_vol, 1))	/* Write it to the VBR sector */
		return FR_DISK_ERR;
	if (fmt == FS_FAT32)					/* Write backup VBR if needed (VBR+6) */
		disk_write(pdrv, tbl, b_vol + 6, 1);
/*
FATFS文件系统剖析1：?
FAT16：?
数据按照其不同的特点和作用大致可分为5部分：
MBR区、DBR区、FAT区、DIR区和DATA区，
相比fat12多了DBR区?????Main?boot?record:?MBR（0--1bdh）????????
磁盘参数存放??????????????????????????????????DPT（1beh--1fdh）???
磁盘分区表??????????????????????????????????55，aa?????????????????????
分区结束标志?????DBR（Dos?Boot?Record）
是操作系统引导记录区的意思?????
FAT区（有两个，一个备份）：对于fat16，
每一个fat项16位，所以可寻址的簇项数为
65535（2的16次方）。而其每簇大小不超?过32k，
所以其每个分区最大容量为2G。fat32，每一个fat项32位，
可寻址簇数目为2的32次方。?????DIR区（根目录区）：
紧接着第二FAT表（即备份的FAT表）之后，记录着根
目录下每个文件（目录）的起始单元，文件的属性等。
定位文件位置时，操作系统根据DIR中的起始单元，
结合FAT表就可以知道文件在硬盘中的具体位置和大小了。
?????
DATA区：实际文件内容存放区。

*/
	/* Initialize FAT area */
	wsect = b_fat;
	for (i = 0; i < N_FATS; i++) {		/* Initialize each FAT copy */
		mem_set(tbl, 0, SS(fs));			/* 1st sector of the FAT  */
		n = md;								/* Media descriptor byte */
		if (fmt != FS_FAT32) {
			//The FAT is an array of up to 65,536 16-bit unsigned integers.	 for fat16
			//其中FAT16是指文件分配表使用16位数字
			/*cluster 与FAT的关系， 族cluster*/
			n |= (fmt == FS_FAT12) ? 0x00FFFF00 : 0xFFFFFF00;
			ST_DWORD(tbl+0, n);				/* Reserve cluster #0-1 (FAT12/16) */
		} else {
			n |= 0xFFFFFF00;
			ST_DWORD(tbl+0, n);				/* Reserve cluster #0-1 (FAT32) */
			ST_DWORD(tbl+4, 0xFFFFFFFF);
			ST_DWORD(tbl+8, 0x0FFFFFFF);	/* Reserve cluster #2 for root directory */
		}
		if (disk_write(pdrv, tbl, wsect++, 1))
			return FR_DISK_ERR;

		//接下来所有的扇区都清0，表示该簇未被占用		
		mem_set(tbl, 0, SS(fs));			/* Fill following FAT entries with zero */
		for (n = 1; n < n_fat; n++) {		/* This loop may take a time on FAT32 volume due to many single sector writes */
			if (disk_write(pdrv, tbl, wsect++, 1))
				return FR_DISK_ERR;
		}
	}
/*
在磁盘上的每一个可用的簇在FAT中就只有一个登记项，
通过在对应簇号的登记项内填入“表项值”来表明数据
区中的该簇是否占用、空闲或是已损坏的。损坏的簇是
在格式化的过程中，通过FORMAT命令发现。在一个簇中，
只要有一个扇区有问题，该簇就不能够使用
*/
	/* Initialize root directory */
	i = (fmt == FS_FAT32) ? au : (UINT)n_dir;
	do {
		if (disk_write(pdrv, tbl, wsect++, 1))
			return FR_DISK_ERR;
	} while (--i);

#if _USE_ERASE	/* Erase data area if needed */
	{
/*用于格式化卷时，探险data sectors*/	
		DWORD eb[2];

		eb[0] = wsect; eb[1] = wsect + (n_clst - ((fmt == FS_FAT32) ? 1 : 0)) * au - 1;
		disk_ioctl(pdrv, CTRL_ERASE_SECTOR, eb);
	}
#endif

	/* Create FSINFO if needed */
	if (fmt == FS_FAT32) {
		ST_DWORD(tbl+FSI_LeadSig, 0x41615252);
		ST_DWORD(tbl+FSI_StrucSig, 0x61417272);
		ST_DWORD(tbl+FSI_Free_Count, n_clst - 1);	/* Number of free clusters */
		ST_DWORD(tbl+FSI_Nxt_Free, 2);				/* Last allocated cluster# */
		ST_WORD(tbl+BS_55AA, 0xAA55);
		disk_write(pdrv, tbl, b_vol + 1, 1);	/* Write original (VBR+1) */
		disk_write(pdrv, tbl, b_vol + 7, 1);	/* Write backup (VBR+7) */
	}

	return (disk_ioctl(pdrv, CTRL_SYNC, 0) == RES_OK) ? FR_OK : FR_DISK_ERR;
}



#if _MULTI_PARTITION
/*-----------------------------------------------------------------------*/
/* Divide Physical Drive                                                 */
/*-----------------------------------------------------------------------*/

FRESULT f_fdisk (
	BYTE pdrv,			/* Physical drive number */
	const DWORD szt[],	/* Pointer to the size table for each partitions */
	void* work			/* Pointer to the working buffer */
)
{
	UINT i, n, sz_cyl, tot_cyl, b_cyl, e_cyl, p_cyl;
	BYTE s_hd, e_hd, *p, *buf = (BYTE*)work;
	DSTATUS stat;
	DWORD sz_disk, sz_part, s_part;


	stat = disk_initialize(pdrv);
	if (stat & STA_NOINIT) return FR_NOT_READY;
	if (stat & STA_PROTECT) return FR_WRITE_PROTECTED;
	if (disk_ioctl(pdrv, GET_SECTOR_COUNT, &sz_disk)) return FR_DISK_ERR;

	/* Determine CHS in the table regardless of the drive geometry */
	for (n = 16; n < 256 && sz_disk / n / 63 > 1024; n *= 2) ;
	if (n == 256) n--;
	e_hd = n - 1;
	sz_cyl = 63 * n;
	tot_cyl = sz_disk / sz_cyl;

	/* Create partition table */
	mem_set(buf, 0, _MAX_SS);
	p = buf + MBR_Table; b_cyl = 0;
	for (i = 0; i < 4; i++, p += SZ_PTE) {
		p_cyl = (szt[i] <= 100U) ? (DWORD)tot_cyl * szt[i] / 100 : szt[i] / sz_cyl;
		if (!p_cyl) continue;
		s_part = (DWORD)sz_cyl * b_cyl;
		sz_part = (DWORD)sz_cyl * p_cyl;
		if (i == 0) {	/* Exclude first track of cylinder 0 */
			s_hd = 1;
			s_part += 63; sz_part -= 63;
		} else {
			s_hd = 0;
		}
		e_cyl = b_cyl + p_cyl - 1;
		if (e_cyl >= tot_cyl) return FR_INVALID_PARAMETER;

		/* Set partition table */
		p[1] = s_hd;						/* Start head */
		p[2] = (BYTE)((b_cyl >> 2) + 1);	/* Start sector */
		p[3] = (BYTE)b_cyl;					/* Start cylinder */
		p[4] = 0x06;						/* System type (temporary setting) */
		p[5] = e_hd;						/* End head */
		p[6] = (BYTE)((e_cyl >> 2) + 63);	/* End sector */
		p[7] = (BYTE)e_cyl;					/* End cylinder */
		ST_DWORD(p + 8, s_part);			/* Start sector in LBA */
		ST_DWORD(p + 12, sz_part);			/* Partition size */

		/* Next partition */
		b_cyl += p_cyl;
	}
	ST_WORD(p, 0xAA55);

	/* Write it to the MBR */
	return (disk_write(pdrv, buf, 0, 1) || disk_ioctl(pdrv, CTRL_SYNC, 0)) ? FR_DISK_ERR : FR_OK;
}


#endif /* _MULTI_PARTITION */
#endif /* _USE_MKFS && !_FS_READONLY */




#if _USE_STRFUNC
/*-----------------------------------------------------------------------*/
/* Get a string from the file                                            */
/*-----------------------------------------------------------------------*/

/*
从文件结构体指针stream中读取数据，每次读取一行。
读取的数据保存在buf指向的字符数组中，每次最多
读取bufsize-1个字符（第bufsize个字符赋'\0'），如果文件
中的该行，不足bufsize个字符，则读完该行就结束。
如若该行（包括最后一个换行符）的字符数超过
bufsize-1，则fgets只返回一个不完整的行，但是，
缓冲区总是以NULL字符结尾，对fgets的下一次调用会
继续读该行。函数成功将返回buf，失败或读到文件
结尾返回NULL。因此我们不能直接通过fgets的返回值
来判断函数是否是出错而终止的，应该借助feof函数
或者ferror函数来判断。

*/
//f_eof
TCHAR* f_gets (
	TCHAR* buff,	/* Pointer to the string buffer to read */
	int len,		/* Size of string buffer (characters) */
	FIL* fp			/* Pointer to the file object */
)
{
	int n = 0;
	TCHAR c, *p = buff;
	BYTE s[2];
	UINT rc;

	/*读字符直到buffer填满*/
	while (n < len - 1) {	/* Read characters until buffer gets filled */
#if _USE_LFN && _LFN_UNICODE
#if _STRF_ENCODE == 3		/* Read a character in UTF-8 */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
		if (c >= 0x80) {
			if (c < 0xC0) continue;	/* Skip stray trailer */
			if (c < 0xE0) {			/* Two-byte sequence */
				f_read(fp, s, 1, &rc);
				if (rc != 1) break;
				c = (c & 0x1F) << 6 | (s[0] & 0x3F);
				if (c < 0x80) c = '?';
			} else {
				if (c < 0xF0) {		/* Three-byte sequence */
					f_read(fp, s, 2, &rc);
					if (rc != 2) break;
					c = c << 12 | (s[0] & 0x3F) << 6 | (s[1] & 0x3F);
					if (c < 0x800) c = '?';
				} else {			/* Reject four-byte sequence */
					c = '?';
				}
			}
		}
#elif _STRF_ENCODE == 2		/* Read a character in UTF-16BE */
		f_read(fp, s, 2, &rc);
		if (rc != 2) break;
		c = s[1] + (s[0] << 8);
#elif _STRF_ENCODE == 1		/* Read a character in UTF-16LE */
		f_read(fp, s, 2, &rc);
		if (rc != 2) break;
		c = s[0] + (s[1] << 8);
#else						/* Read a character in ANSI/OEM */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
		if (IsDBCS1(c)) {
			f_read(fp, s, 1, &rc);
			if (rc != 1) break;
			c = (c << 8) + s[0];
		}
		c = ff_convert(c, 1);	/* OEM -> Unicode */
		if (!c) c = '?';
#endif
#else						/* Read a character without conversion */
		f_read(fp, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];
#endif
		if (_USE_STRFUNC == 2 && c == '\r') continue;	/* Strip '\r' */
		*p++ = c;
		n++;
		if (c == '\n') break;		/* Break on EOL */
	}
	*p = 0;
	/*当读到了文件末尾，或者发生error*/
	/*是不读到了文件的末尾，用f_eof(fp)去判断，
	所以当返回null时，要用f_eof(fp)才能判断是否
	发生了读error*/
	return n ? buff : 0;			/* When no data read (eof or error), return with error. */
}



#if !_FS_READONLY
#include <stdarg.h>
/*-----------------------------------------------------------------------*/
/* Put a character to the file                                           */
/*-----------------------------------------------------------------------*/

/*
	输出缓冲区
	idx代表buf的哪个偏移上
	nchr应该是buf上有多少个字符+ 已经写
	入文件的字节数
*/
typedef struct {
	FIL* fp;
	int idx, nchr;
	BYTE buf[64];
} putbuff;


static
void putc_bfd (
	putbuff* pb,
	TCHAR c
)
{
	UINT bw;
	int i;


	if (_USE_STRFUNC == 2 && c == '\n')	 /* LF -> CRLF conversion */
		putc_bfd(pb, '\r');

	i = pb->idx;	/* Buffer write index (-1:error) */
	if (i < 0) return;

#if _USE_LFN && _LFN_UNICODE
#if _STRF_ENCODE == 3			/* Write a character in UTF-8 */
	if (c < 0x80) {				/* 7-bit */
		pb->buf[i++] = (BYTE)c;
	} else {
		if (c < 0x800) {		/* 11-bit */
			pb->buf[i++] = (BYTE)(0xC0 | c >> 6);
		} else {				/* 16-bit */
			pb->buf[i++] = (BYTE)(0xE0 | c >> 12);
			pb->buf[i++] = (BYTE)(0x80 | (c >> 6 & 0x3F));
		}
		pb->buf[i++] = (BYTE)(0x80 | (c & 0x3F));
	}
#elif _STRF_ENCODE == 2			/* Write a character in UTF-16BE */
	pb->buf[i++] = (BYTE)(c >> 8);
	pb->buf[i++] = (BYTE)c;
#elif _STRF_ENCODE == 1			/* Write a character in UTF-16LE */
	pb->buf[i++] = (BYTE)c;
	pb->buf[i++] = (BYTE)(c >> 8);
#else							/* Write a character in ANSI/OEM */
	c = ff_convert(c, 0);	/* Unicode -> OEM */
	if (!c) c = '?';
	if (c >= 0x100)
		pb->buf[i++] = (BYTE)(c >> 8);
	pb->buf[i++] = (BYTE)c;
#endif
#else							/* Write a character without conversion */
	pb->buf[i++] = (BYTE)c;
#endif

	if (i >= (int)(sizeof pb->buf) - 3) {	/* Write buffered characters to the file */
		f_write(pb->fp, pb->buf, (UINT)i, &bw);
		i = (bw == (UINT)i) ? 0 : -1;
		//当写错误，idx = -1, 否则为0
	}
	pb->idx = i;
	pb->nchr++;
}



int f_putc (
	TCHAR c,	/* A character to be output */
	FIL* fp		/* Pointer to the file object */
)
{
	putbuff pb;
	UINT nw;


	pb.fp = fp;			/* Initialize output buffer */
	pb.nchr = pb.idx = 0;

	putc_bfd(&pb, c);	/* Put a character */

	if (   pb.idx >= 0	/* Flush buffered characters to the file */
		&& f_write(pb.fp, pb.buf, (UINT)pb.idx, &nw) == FR_OK
		&& (UINT)pb.idx == nw) return pb.nchr;
	return EOF;
}




/*-----------------------------------------------------------------------*/
/* Put a string to the file                                              */
/*-----------------------------------------------------------------------*/

int f_puts (
	const TCHAR* str,	/* Pointer to the string to be output */
	FIL* fp				/* Pointer to the file object */
)
{
	putbuff pb;
	UINT nw;


	pb.fp = fp;				/* Initialize output buffer */
	pb.nchr = pb.idx = 0;

	while (*str)			/* Put the string */
		putc_bfd(&pb, *str++);

	if (   pb.idx >= 0		/* Flush buffered characters to the file */
		&& f_write(pb.fp, pb.buf, (UINT)pb.idx, &nw) == FR_OK
		&& (UINT)pb.idx == nw) return pb.nchr;
	return EOF;
}




/*-----------------------------------------------------------------------*/
/* Put a formatted string to the file                                    */
/*-----------------------------------------------------------------------*/

/*
	把打印输出到文件fp中,可以用来保存log到flash中
*/

int f_printf (
	FIL* fp,			/* Pointer to the file object */
	const TCHAR* fmt,	/* Pointer to the format string */
	...					/* Optional arguments... */
)
{
	va_list arp;
	BYTE f, r;
	UINT nw, i, j, w;
	DWORD v;
	TCHAR c, d, s[16], *p;
	putbuff pb;


	pb.fp = fp;				/* Initialize output buffer */
	pb.nchr = pb.idx = 0;

	va_start(arp, fmt);

	for (;;) {
		c = *fmt++;
		if (c == 0) break;			/* End of string */
		if (c != '%') {				/* Non escape character */
			putc_bfd(&pb, c);
			continue;
		}
		w = f = 0;
		c = *fmt++;
		if (c == '0') {				/* Flag: '0' padding */
			f = 1; c = *fmt++;
		} else {
			if (c == '-') {			/* Flag: left justified */
				f = 2; c = *fmt++;
			}
		}
		while (IsDigit(c)) {		/* Precision */
			w = w * 10 + c - '0';
			c = *fmt++;
		}
		if (c == 'l' || c == 'L') {	/* Prefix: Size is long int */
			f |= 4; c = *fmt++;
		}
		if (!c) break;
		d = c;
		if (IsLower(d)) d -= 0x20;
		switch (d) {				/* Type is... */
		case 'S' :					/* String */
			p = va_arg(arp, TCHAR*);
			for (j = 0; p[j]; j++) ;
			if (!(f & 2)) {
				while (j++ < w) putc_bfd(&pb, ' ');
			}
			while (*p) putc_bfd(&pb, *p++);
			while (j++ < w) putc_bfd(&pb, ' ');
			continue;
		case 'C' :					/* Character */
			putc_bfd(&pb, (TCHAR)va_arg(arp, int)); continue;
		case 'B' :					/* Binary */
			r = 2; break;
		case 'O' :					/* Octal */
			r = 8; break;
		case 'D' :					/* Signed decimal */
		case 'U' :					/* Unsigned decimal */
			r = 10; break;
		case 'X' :					/* Hexdecimal */
			r = 16; break;
		default:					/* Unknown type (pass-through) */
			putc_bfd(&pb, c); continue;
		}

		/* Get an argument and put it in numeral */
		v = (f & 4) ? (DWORD)va_arg(arp, long) : ((d == 'D') ? (DWORD)(long)va_arg(arp, int) : (DWORD)va_arg(arp, unsigned int));
		if (d == 'D' && (v & 0x80000000)) {
			v = 0 - v;
			f |= 8;
		}
		i = 0;
		do {
			d = (TCHAR)(v % r); v /= r;
			if (d > 9) d += (c == 'x') ? 0x27 : 0x07;
			s[i++] = d + '0';
		} while (v && i < sizeof s / sizeof s[0]);
		if (f & 8) s[i++] = '-';
		j = i; d = (f & 1) ? '0' : ' ';
		while (!(f & 2) && j++ < w) putc_bfd(&pb, d);
		do putc_bfd(&pb, s[--i]); while (i);
		while (j++ < w) putc_bfd(&pb, d);
	}

	va_end(arp);

	if (   pb.idx >= 0		/* Flush buffered characters to the file */
		&& f_write(pb.fp, pb.buf, (UINT)pb.idx, &nw) == FR_OK
		&& (UINT)pb.idx == nw) return pb.nchr;
	return EOF;
}

#endif /* !_FS_READONLY */
#endif /* _USE_STRFUNC */









