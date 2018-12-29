/*
----------------------------------------------------------------------
File        : fs_port.h 
Purpose     : Architecture dependend defines for the file system
              This header defines basic data types and access macros
              used by the file system. Depending on your CPU/compiler,
              you may have to modify this file.
----------------------------------------------------------------------
Known problems or limitations with current version
----------------------------------------------------------------------
None.
---------------------------END-OF-HEADER------------------------------
*/

#ifndef _FS_PORT_H_
#define _FS_PORT_H_

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************************
*
*             define global data types
*
**********************************************************************

  These defines will work for most 8/16/32 bit CPUs. Please check
  your compiler manual if you are not sure.
*/

#define FS_size_t       unsigned long       /* 32 bit unsigned */
#define FS_u64	     unsigned long long	/*64 bit unsigned*/
#define FS_i64          signed long long	/*64 bit signed*/
#define FS_u32          unsigned long       /* 32 bit unsigned */
#define FS_i32          signed long         /* 32 bit signed */
#define FS_u16          unsigned short      /* 16 bit unsigned */
#define FS_i16          signed short        /* 16 bit signed */
#define FS_u8           unsigned char       /*  8 bit unsigned */

/* 
  Some CPUs have default character pointers, which are unable to access
  the complete CPU address space. The following define is used for character
  pointers, which must be able to access the complete address space.
*/
#define FS_FARCHARPTR   char *              /* far character pointer */

#define FS_CMD_GET_SDMMC_CID         1011  	/* use this to get SDMMC Register CID. */


/*********************************************************************
*
*             #define access macros
*
**********************************************************************

  Version V1.30 and older require data access macros

    FS__r_u32
    FS__w_u32
    FS__r_u16
    FS__w_u16

  and a constant

    FS_LITTLE_ENDIAN

  These are obsolote. Of course it does not hurt, if you leave
  an old fs_port.h unchanged.
*/

#ifdef __cplusplus
}
#endif

#endif  /* _FS_PORT_H_ */

