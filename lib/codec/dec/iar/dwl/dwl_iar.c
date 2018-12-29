/*------------------------------------------------------------------------------
--                                                                            --
--       This software is confidential and proprietary and may be used        --
--        only as expressly authorized by a licensing agreement from          --
--                                                                            --
--                            Hantro Products Oy.                             --
--                                                                            --
--                   (C) COPYRIGHT 2006 HANTRO PRODUCTS OY                    --
--                            ALL RIGHTS RESERVED                             --
--                                                                            --
--                 The entire notice above must be reproduced                 --
--                  on all copies and should not be removed.                  --
--                                                                            --
--------------------------------------------------------------------------------
--
--  Description :  dwl common part
--
------------------------------------------------------------------------------
--
--  Version control information, please leave untouched.
--
--  $RCSfile: dwl_linux.c,v $
--  $Revision: 1.54 $
--  $Date: 2011/01/17 13:18:22 $
--
------------------------------------------------------------------------------*/

#include "basetype.h"
#include "dwl.h"
//#include "dwl_linux.h"
#include "dwl_iar.h"
#include "rtos.h"
//#include "memalloc.h"
//#include "dwl_linux_lock.h"

//#include "hx170dec.h"   /* This DWL uses the kernel module */

//#include <sys/syscall.h>
//#include <sys/types.h>
//#include <sys/stat.h>
//#include <sys/mman.h>
//#include <sys/ioctl.h>
//#include <sys/timeb.h>
//#include <fcntl.h>
//#include <unistd.h>

//#include <errno.h>

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "xm_kernel_malloc.h"
#include "xm_core.h"

void dma_inv_range (unsigned int ulStart, unsigned int ulEnd);
unsigned int vaddr_to_page_addr (unsigned int addr);


#define DWL_MPEG2_E         31  /* 1 bit */
#define DWL_VC1_E           29  /* 2 bits */
#define DWL_JPEG_E          28  /* 1 bit */
#define DWL_MPEG4_E         26  /* 2 bits */
#define DWL_H264_E          24  /* 2 bits */
#define DWL_VP6_E           23  /* 1 bit */
#define DWL_PJPEG_E         22  /* 1 bit */
#define DWL_REF_BUFF_E      20  /* 1 bit */

#define DWL_JPEG_EXT_E          31  /* 1 bit */
#define DWL_REF_BUFF_ILACE_E    30  /* 1 bit */
#define DWL_MPEG4_CUSTOM_E      29  /* 1 bit */
#define DWL_REF_BUFF_DOUBLE_E   28  /* 1 bit */
#define DWL_RV_E            26  /* 2 bits */
#define DWL_VP7_E           24  /* 1 bit */
#define DWL_VP8_E           23  /* 1 bit */
#define DWL_AVS_E           22  /* 1 bit */
#define DWL_MVC_E           20  /* 2 bits */
#define DWL_WEBP_E          19  /* 1 bit */
#define DWL_DEC_TILED_L     17  /* 2 bits */

#define DWL_CFG_E           24  /* 4 bits */
#define DWL_PP_E            16  /* 1 bit */
#define DWL_PP_IN_TILED_L   14  /* 2 bits */

#define DWL_SORENSONSPARK_E 11  /* 1 bit */

#define DWL_H264_FUSE_E          31 /* 1 bit */
#define DWL_MPEG4_FUSE_E         30 /* 1 bit */
#define DWL_MPEG2_FUSE_E         29 /* 1 bit */
#define DWL_SORENSONSPARK_FUSE_E 28 /* 1 bit */
#define DWL_JPEG_FUSE_E          27 /* 1 bit */
#define DWL_VP6_FUSE_E           26 /* 1 bit */
#define DWL_VC1_FUSE_E           25 /* 1 bit */
#define DWL_PJPEG_FUSE_E         24 /* 1 bit */
#define DWL_CUSTOM_MPEG4_FUSE_E  23 /* 1 bit */
#define DWL_RV_FUSE_E            22 /* 1 bit */
#define DWL_VP7_FUSE_E           21 /* 1 bit */
#define DWL_VP8_FUSE_E           20 /* 1 bit */
#define DWL_AVS_FUSE_E           19 /* 1 bit */
#define DWL_MVC_FUSE_E           18 /* 1 bit */

#define DWL_DEC_MAX_1920_FUSE_E  15 /* 1 bit */
#define DWL_DEC_MAX_1280_FUSE_E  14 /* 1 bit */
#define DWL_DEC_MAX_720_FUSE_E   13 /* 1 bit */
#define DWL_DEC_MAX_352_FUSE_E   12 /* 1 bit */
#define DWL_REF_BUFF_FUSE_E       7 /* 1 bit */


#define DWL_PP_FUSE_E				31  /* 1 bit */
#define DWL_PP_DEINTERLACE_FUSE_E   30  /* 1 bit */
#define DWL_PP_ALPHA_BLEND_FUSE_E   29  /* 1 bit */
#define DWL_PP_MAX_1920_FUSE_E		15  /* 1 bit */
#define DWL_PP_MAX_1280_FUSE_E		14  /* 1 bit */
#define DWL_PP_MAX_720_FUSE_E		13  /* 1 bit */
#define DWL_PP_MAX_352_FUSE_E		12  /* 1 bit */

#ifdef _DWL_FAKE_HW_TIMEOUT
static void DWLFakeTimeout(u32 * status);
#endif

static OS_RSEMA dwl_pp_semid;		// PP
static OS_RSEMA dwl_dec_semid;	// dec

static u32 getpagesize (void)
{
	return 1024;
}

/*------------------------------------------------------------------------------
    Function name   : DWLMapRegisters
    Description     :

    Return type     : u32 - the HW ID
------------------------------------------------------------------------------*/
u32 *DWLMapRegisters(int mem_dev, unsigned int base,
                     unsigned int regSize, u32 write)
{
    return (u32 *) base;
}

void DWLUnmapRegisters(const void *io, unsigned int regSize)
{
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicID
    Description     : Read the HW ID. Does not need a DWL instance to run

    Return type     : u32 - the HW ID
------------------------------------------------------------------------------*/
u32 DWLReadAsicID()
{
    u32 *io, id = ~0;

    DWL_DEBUG("DWLReadAsicID\n");

	 io = (u32 *)HX170DEC_IO_BASE;

    id = io[0];

    return id;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicConfig
    Description     : Read HW configuration. Does not need a DWL instance to run

    Return type     : DWLHwConfig_t - structure with HW configuration
------------------------------------------------------------------------------*/
void DWLReadAsicConfig(DWLHwConfig_t * pHwCfg)
{
    const u32 *io = NULL;
    u32 configReg;
    u32 asicID;
    unsigned long base;
    unsigned int regSize;

    DWL_DEBUG("DWLReadAsicConfig\n");

    memset(pHwCfg, 0, sizeof(*pHwCfg));

    io = (u32 *)HX170DEC_IO_BASE;

    /* Decoder configuration */
    configReg = io[HX170DEC_SYNTH_CFG];

    pHwCfg->h264Support = (configReg >> DWL_H264_E) & 0x3U;
    /* check jpeg */
    pHwCfg->jpegSupport = (configReg >> DWL_JPEG_E) & 0x01U;
    if(pHwCfg->jpegSupport && ((configReg >> DWL_PJPEG_E) & 0x01U))
        pHwCfg->jpegSupport = JPEG_PROGRESSIVE;
	 pHwCfg->jpegSupport = JPEG_PROGRESSIVE;
    pHwCfg->mpeg4Support = (configReg >> DWL_MPEG4_E) & 0x3U;
    pHwCfg->vc1Support = (configReg >> DWL_VC1_E) & 0x3U;
    pHwCfg->mpeg2Support = (configReg >> DWL_MPEG2_E) & 0x01U;
    pHwCfg->sorensonSparkSupport = (configReg >> DWL_SORENSONSPARK_E) & 0x01U;
    pHwCfg->refBufSupport = (configReg >> DWL_REF_BUFF_E) & 0x01U;
    pHwCfg->vp6Support = (configReg >> DWL_VP6_E) & 0x01U;
#ifdef DEC_X170_APF_DISABLE
    if(DEC_X170_APF_DISABLE)
    {
        pHwCfg->tiledModeSupport = 0;
    }
#endif /* DEC_X170_APF_DISABLE */

    pHwCfg->maxDecPicWidth = configReg & 0x07FFU;

    /* 2nd Config register */
    configReg = io[HX170DEC_SYNTH_CFG_2];
    if(pHwCfg->refBufSupport)
    {
        if((configReg >> DWL_REF_BUFF_ILACE_E) & 0x01U)
            pHwCfg->refBufSupport |= 2;
        if((configReg >> DWL_REF_BUFF_DOUBLE_E) & 0x01U)
            pHwCfg->refBufSupport |= 4;
    }

    pHwCfg->customMpeg4Support = (configReg >> DWL_MPEG4_CUSTOM_E) & 0x01U;
    pHwCfg->vp7Support = (configReg >> DWL_VP7_E) & 0x01U;
    pHwCfg->vp8Support = (configReg >> DWL_VP8_E) & 0x01U;
    pHwCfg->avsSupport = (configReg >> DWL_AVS_E) & 0x01U;

    /* JPEG xtensions */
    asicID = DWLReadAsicID();    
#if 0
    if(((asicID >> 16) >= 0x8190U) ||
       ((asicID >> 16) == 0x6731U) )
        pHwCfg->jpegESupport = (configReg >> DWL_JPEG_EXT_E) & 0x01U;
    else
        pHwCfg->jpegESupport = JPEG_EXT_NOT_SUPPORTED;    
#else
	 pHwCfg->jpegESupport = 1;
#endif
    if(((asicID >> 16) >= 0x9170U) ||
       ((asicID >> 16) == 0x6731U) )
        pHwCfg->rvSupport = (configReg >> DWL_RV_E) & 0x03U;
    else
        pHwCfg->rvSupport = RV_NOT_SUPPORTED;

    pHwCfg->mvcSupport = (configReg >> DWL_MVC_E) & 0x03U;

    pHwCfg->webpSupport = (configReg >> DWL_WEBP_E) & 0x01U;
    pHwCfg->tiledModeSupport = (configReg >> DWL_DEC_TILED_L) & 0x03U;

    if(pHwCfg->refBufSupport &&
       (asicID >> 16) == 0x6731U )
    {
        pHwCfg->refBufSupport |= 8; /* enable HW support for offset */
    }

    /* Pp configuration */
    configReg = io[HX170PP_SYNTH_CFG];

    if((configReg >> DWL_PP_E) & 0x01U)
    {
        pHwCfg->ppSupport = 1;
        pHwCfg->maxPpOutPicWidth = configReg & 0x07FFU;
        /*pHwCfg->ppConfig = (configReg >> DWL_CFG_E) & 0x0FU; */
        pHwCfg->ppConfig = configReg;
    }
    else
    {
        pHwCfg->ppSupport = 0;
        pHwCfg->maxPpOutPicWidth = 0;
        pHwCfg->ppConfig = 0;
    }

    {
        /* check the HW versio */
        asicID = DWLReadAsicID();
        if(((asicID >> 16) >= 0x8190U) ||
           ((asicID >> 16) == 0x6731U) )
        {
            u32 deInterlace;
            u32 alphaBlend;
            u32 deInterlaceFuse;
            u32 alphaBlendFuse;
            DWLHwFuseStatus_t hwFuseSts;

            /* check fuse status */
            DWLReadAsicFuseStatus(&hwFuseSts);

            /* Maximum decoding width supported by the HW */
            if(pHwCfg->maxDecPicWidth > hwFuseSts.maxDecPicWidthFuse)
                pHwCfg->maxDecPicWidth = hwFuseSts.maxDecPicWidthFuse;
            /* Maximum output width of Post-Processor */
            if(pHwCfg->maxPpOutPicWidth > hwFuseSts.maxPpOutPicWidthFuse)
                pHwCfg->maxPpOutPicWidth = hwFuseSts.maxPpOutPicWidthFuse;
            /* h264 */
            if(!hwFuseSts.h264SupportFuse)
                pHwCfg->h264Support = H264_NOT_SUPPORTED;
            /* mpeg-4 */
            if(!hwFuseSts.mpeg4SupportFuse)
                pHwCfg->mpeg4Support = MPEG4_NOT_SUPPORTED;
            /* custom mpeg-4 */
            if(!hwFuseSts.customMpeg4SupportFuse)
                pHwCfg->customMpeg4Support = MPEG4_CUSTOM_NOT_SUPPORTED;
            /* jpeg (baseline && progressive) */
            if(!hwFuseSts.jpegSupportFuse)
                pHwCfg->jpegSupport = JPEG_NOT_SUPPORTED;
            if((pHwCfg->jpegSupport == JPEG_PROGRESSIVE) &&
               !hwFuseSts.jpegProgSupportFuse)
                pHwCfg->jpegSupport = JPEG_BASELINE;
            /* mpeg-2 */
            if(!hwFuseSts.mpeg2SupportFuse)
                pHwCfg->mpeg2Support = MPEG2_NOT_SUPPORTED;
            /* vc-1 */
            if(!hwFuseSts.vc1SupportFuse)
                pHwCfg->vc1Support = VC1_NOT_SUPPORTED;
            /* vp6 */
            if(!hwFuseSts.vp6SupportFuse)
                pHwCfg->vp6Support = VP6_NOT_SUPPORTED;
            /* vp7 */
            if(!hwFuseSts.vp7SupportFuse)
                pHwCfg->vp7Support = VP7_NOT_SUPPORTED;
            /* vp6 */
            if(!hwFuseSts.vp8SupportFuse)
                pHwCfg->vp8Support = VP8_NOT_SUPPORTED;
            /* pp */
            if(!hwFuseSts.ppSupportFuse)
                pHwCfg->ppSupport = PP_NOT_SUPPORTED;
            /* check the pp config vs fuse status */
            if((pHwCfg->ppConfig & 0xFC000000) &&
               ((hwFuseSts.ppConfigFuse & 0xF0000000) >> 5))
            {
                /* config */
                deInterlace = ((pHwCfg->ppConfig & PP_DEINTERLACING) >> 25);
                alphaBlend = ((pHwCfg->ppConfig & PP_ALPHA_BLENDING) >> 24);
                /* fuse */
                deInterlaceFuse =
                    (((hwFuseSts.ppConfigFuse >> 5) & PP_DEINTERLACING) >> 25);
                alphaBlendFuse =
                    (((hwFuseSts.ppConfigFuse >> 5) & PP_ALPHA_BLENDING) >> 24);

                /* check if */
                if(deInterlace && !deInterlaceFuse)
                    pHwCfg->ppConfig &= 0xFD000000;
                if(alphaBlend && !alphaBlendFuse)
                    pHwCfg->ppConfig &= 0xFE000000;
            }
            /* sorenson */
            if(!hwFuseSts.sorensonSparkSupportFuse)
                pHwCfg->sorensonSparkSupport = SORENSON_SPARK_NOT_SUPPORTED;
            /* ref. picture buffer */
            if(!hwFuseSts.refBufSupportFuse)
                pHwCfg->refBufSupport = REF_BUF_NOT_SUPPORTED;

            /* rv */
            if(!hwFuseSts.rvSupportFuse)
                pHwCfg->rvSupport = RV_NOT_SUPPORTED;
            /* avs */
            if(!hwFuseSts.avsSupportFuse)
                pHwCfg->avsSupport = AVS_NOT_SUPPORTED;
            /* mvc */
            if(!hwFuseSts.mvcSupportFuse)
                pHwCfg->mvcSupport = MVC_NOT_SUPPORTED;
        }
    }

}

/*------------------------------------------------------------------------------
    Function name   : DWLReadAsicFuseStatus
    Description     : Read HW fuse configuration. Does not need a DWL instance to run

    Returns     : DWLHwFuseStatus_t * pHwFuseSts - structure with HW fuse configuration
------------------------------------------------------------------------------*/
void DWLReadAsicFuseStatus(DWLHwFuseStatus_t * pHwFuseSts)
{
    const u32 *io = NULL;
    u32 configReg;
    u32 fuseReg;
    u32 fuseRegPp;
    unsigned long base;
    unsigned int regSize;

    DWL_DEBUG("DWLReadAsicFuseStatus\n");

    memset(pHwFuseSts, 0, sizeof(*pHwFuseSts));


    io = (u32 *)HX170DEC_IO_BASE;

    /* Decoder fuse configuration */
    fuseReg = io[HX170DEC_FUSE_CFG];

    pHwFuseSts->h264SupportFuse = (fuseReg >> DWL_H264_FUSE_E) & 0x01U;
    pHwFuseSts->mpeg4SupportFuse = (fuseReg >> DWL_MPEG4_FUSE_E) & 0x01U;
    pHwFuseSts->mpeg2SupportFuse = (fuseReg >> DWL_MPEG2_FUSE_E) & 0x01U;
    pHwFuseSts->sorensonSparkSupportFuse =
        (fuseReg >> DWL_SORENSONSPARK_FUSE_E) & 0x01U;
    pHwFuseSts->jpegSupportFuse = (fuseReg >> DWL_JPEG_FUSE_E) & 0x01U;
    pHwFuseSts->vp6SupportFuse = (fuseReg >> DWL_VP6_FUSE_E) & 0x01U;
    pHwFuseSts->vc1SupportFuse = (fuseReg >> DWL_VC1_FUSE_E) & 0x01U;
    pHwFuseSts->jpegProgSupportFuse = (fuseReg >> DWL_PJPEG_FUSE_E) & 0x01U;
    pHwFuseSts->rvSupportFuse = (fuseReg >> DWL_RV_FUSE_E) & 0x01U;
    pHwFuseSts->avsSupportFuse = (fuseReg >> DWL_AVS_FUSE_E) & 0x01U;
	pHwFuseSts->vp7SupportFuse = (fuseReg >> DWL_VP7_FUSE_E) & 0x01U;
	pHwFuseSts->vp8SupportFuse = (fuseReg >> DWL_VP8_FUSE_E) & 0x01U;
    pHwFuseSts->customMpeg4SupportFuse = (fuseReg >> DWL_CUSTOM_MPEG4_FUSE_E) & 0x01U;
    pHwFuseSts->mvcSupportFuse = (fuseReg >> DWL_MVC_FUSE_E) & 0x01U;

    /* check max. decoder output width */
    if(fuseReg & 0x8000U)
        pHwFuseSts->maxDecPicWidthFuse = 1920;
    else if(fuseReg & 0x4000U)
        pHwFuseSts->maxDecPicWidthFuse = 1280;
    else if(fuseReg & 0x2000U)
        pHwFuseSts->maxDecPicWidthFuse = 720;
    else if(fuseReg & 0x1000U)
        pHwFuseSts->maxDecPicWidthFuse = 352;

    pHwFuseSts->refBufSupportFuse = (fuseReg >> DWL_REF_BUFF_FUSE_E) & 0x01U;

    /* Pp configuration */
    configReg = io[HX170PP_SYNTH_CFG];

    if((configReg >> DWL_PP_E) & 0x01U)
    {
        /* Pp fuse configuration */
        fuseRegPp = io[HX170PP_FUSE_CFG];

        if((fuseRegPp >> DWL_PP_FUSE_E) & 0x01U)
        {
            pHwFuseSts->ppSupportFuse = 1;

            /* check max. pp output width */
            if(fuseRegPp & 0x8000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 1920;
            else if(fuseRegPp & 0x4000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 1280;
            else if(fuseRegPp & 0x2000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 720;
            else if(fuseRegPp & 0x1000U)
                pHwFuseSts->maxPpOutPicWidthFuse = 352;

            pHwFuseSts->ppConfigFuse = fuseRegPp;
        }
        else
        {
            pHwFuseSts->ppSupportFuse = 0;
            pHwFuseSts->maxPpOutPicWidthFuse = 0;
            pHwFuseSts->ppConfigFuse = 0;
        }
    }
}

/*------------------------------------------------------------------------------
    Function name   : DWLMallocRefFrm
    Description     : Allocate a frame buffer (contiguous linear RAM memory)

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - DWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : void *info - place where the allocated memory buffer
                        parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocRefFrm(const void *instance, u32 size, DWLLinearMem_t * info)
{

#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocRefFrm\t%8d bytes\n", size);
#endif

    return DWLMallocLinear(instance, size, info);

}

/*------------------------------------------------------------------------------
    Function name   : DWLFreeRefFrm
    Description     : Release a frame buffer previously allocated with
                        DWLMallocRefFrm.

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : void *info - frame buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeRefFrm(const void *instance, DWLLinearMem_t * info)
{
    DWLFreeLinear(instance, info);
}

/*------------------------------------------------------------------------------
    Function name   : DWLMallocLinear
    Description     : Allocate a contiguous, linear RAM  memory buffer

    Return type     : i32 - 0 for success or a negative error code

    Argument        : const void * instance - DWL instance
    Argument        : u32 size - size in bytes of the requested memory
    Argument        : void *info - place where the allocated memory buffer
                        parameters are returned
------------------------------------------------------------------------------*/
i32 DWLMallocLinear(const void *instance, u32 size, DWLLinearMem_t * info)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    u32 pgsize = getpagesize();
    //MemallocParams params;

    assert(dec_dwl != NULL);
    assert(info != NULL);

#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocLinear\t%8d bytes \n", size);
#endif

    size = (size + (pgsize - 1)) & (~(pgsize - 1));

    info->size = size;
    info->virtualAddress = NULL;
    info->busAddress = 0;
	 
	 info->base = (char *)kernel_malloc (pgsize * 2 + size);
	 if(info->base == NULL)
    {
        DWL_DEBUG("DWLMallocLinear: ERROR! No linear buffer available\n");
        return DWL_ERROR;
    }
		 
	 //dma_inv_range ((u32)info->base, (u32)info->base + pgsize * 2 + size);
    /* get memory linear memory buffers */

	 info->virtualAddress = (u32 *)((unsigned int)(info->base + pgsize - 1) & ~(pgsize - 1)); 
	 
	 dma_inv_range ((u32)info->virtualAddress, (u32)info->virtualAddress + info->size);
	 
	 // 将虚拟地址转换为页地址
	 info->virtualAddress = (u32 *)vaddr_to_page_addr ((unsigned int)info->virtualAddress);
	 
	// dma_inv_range ((u32)info->virtualAddress, (u32)info->virtualAddress + size);
	 
    /* Map the bus address to virtual address */

    /* ASIC might be in different address space */
    //buff->busAddress = (u32)BUS_CPU_TO_ASIC(buff->virtualAddress);
	 info->busAddress = (u32)info->virtualAddress;
	 
#ifdef MEMORY_USAGE_TRACE
    printf("DWLMallocLinear 0x%08x virtualAddress: 0x%08x\n",
           info->busAddress, (unsigned) info->virtualAddress);
#endif

    if(info->virtualAddress == NULL)
        return DWL_ERROR;

    return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLFreeLinear
    Description     : Release a linera memory buffer, previously allocated with
                        DWLMallocLinear.

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : void *info - linear buffer memory information
------------------------------------------------------------------------------*/
void DWLFreeLinear(const void *instance, DWLLinearMem_t * info)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
	 if(dec_dwl == NULL || info == NULL)
	 {
		 printf ("DWLFreeLinear failed, instance %08x, info %08x\n", instance, info);
		 return;
	 }

    assert(dec_dwl != NULL);
    assert(info != NULL);

    if(info->busAddress != 0)
    {
		 kernel_free (info->base);
		 info->base = 0;
		 info->busAddress = 0;
	 }

}

/*------------------------------------------------------------------------------
    Function name   : DWLWriteReg
    Description     : Write a value to a hardware IO register

    Return type     : void

    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLWriteReg(const void *instance, u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    DWL_DEBUG("DWL: Write reg %d at offset 0x%02X --> %08X\n", offset / 4,
              offset, value);

#ifdef INTERNAL_TEST
    u32 swreg = offset / 4;
    u32 tmp_val = ((*(dec_dwl->pRegBase + 3)) >> 27) & 0x01;

#if 0
    if(swreg == 5)
    {
        /* check if RLC mode -> swreg12 contains base address */
        if(tmp_val)
        {
            fprintf(dec_dwl->regDump,
                    "-\nW STREAM START BIT:       N.A. (RLC MODE ENABLED)\n");
        }
        else
        {
            fprintf(dec_dwl->regDump, "-\nW STREAM START BIT:       %u\n",
                    (*(dec_dwl->pRegBase + swreg)) >> 26);
        }
    }
#endif
    if(swreg == 12)
    {
        /* check if RLC mode -> swreg12 contains base address */
		  char msg[128];
        if(tmp_val)
        {
            sprintf(msg,
                    "-\nW STREAM START ADDRESS:       N.A. (RLC MODE ENABLED)\n");
        }
        else
        {
            sprintf(msg, "-\nW STREAM START ADDRESS:       %u\n",
                    value);
        }
		  FS_Write (dec_dwl->regDump, msg, strlen(msg));
		  FS_FSeek (dec_dwl->regDump, 0, SEEK_SET);
        //fflush(dec_dwl->regDump);
    }
#endif

/*#ifdef _DWL_DEBUG
    fprintf(dec_dwl->regDump, "write:%d:0x%08x:0x%08x\n", offset / 4, offset,
            value);
#endif*/

    assert((dec_dwl->clientType != DWL_CLIENT_TYPE_PP &&
            offset < HX170PP_REG_START) ||
           (dec_dwl->clientType == DWL_CLIENT_TYPE_PP &&
            offset >= HX170PP_REG_START));

    assert(dec_dwl != NULL);
    assert(offset < dec_dwl->regSize);

    offset = offset / 4;

#ifdef _DWL_HW_PERFORMANCE
    if(offset == HX170DEC_REG_START / 4 && value & DWL_HW_ENABLE_BIT)
    {
        DwlDecoderEnable();
    }
    if(offset == HX170PP_REG_START / 4 && value & DWL_HW_ENABLE_BIT)
    {
        DwlPpEnable();
    }
#endif

#ifdef _DWL_FAKE_HW_TIMEOUT
    /* Compensate for hacking the timeout bit in the read */
    if(offset == HX170DEC_REG_START / 4 && value & DWL_HW_TIMEOUT_BIT)
        *(dec_dwl->pRegBase + offset) = value & ~DWL_HW_TIMEOUT_BIT;
    else
        *(dec_dwl->pRegBase + offset) = value;
#else
    *(dec_dwl->pRegBase + offset) = value;
#endif
}

/*------------------------------------------------------------------------------
    Function name   : DWLReadReg
    Description     : Read the value of a hardware IO register

    Return type     : u32 - the value stored in the register

    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be read
------------------------------------------------------------------------------*/
u32 DWLReadReg(const void *instance, u32 offset)
{
	 char msg[512];
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;
    u32 val;

    assert((dec_dwl->clientType != DWL_CLIENT_TYPE_PP &&
            offset < HX170PP_REG_START) ||
           (dec_dwl->clientType == DWL_CLIENT_TYPE_PP &&
            offset >= HX170PP_REG_START) || (offset == 0) ||
           (offset == HX170PP_SYNTH_CFG));

    assert(dec_dwl != NULL);
    assert(offset < dec_dwl->regSize);

    offset = offset / 4;
    val = *(dec_dwl->pRegBase + offset);

    DWL_DEBUG("DWL: Read reg %d at offset 0x%02X --> %08X\n", offset,
              offset * 4, val);
#ifdef INTERNAL_TEST
    if(offset == 1 || offset == 12 || offset == 52 || offset == 53 ||
       offset == 60 || offset == 56)
    {
        if(offset == 1)
        {
            /* write out the interrupt status bit */
            u32 tmp_val = (val >> 12) & 0xFF;

            if(tmp_val & 0x01)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            PICTURE DECODED\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            else if(tmp_val & 0x04)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            BUFFER EMPTY\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            else if(tmp_val & 0x08)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            ASO DETECTED\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            else if(tmp_val & 0x10)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            ERROR DETECTED\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            else if(tmp_val & 0x20)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            SLICE DECODED\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            else if(tmp_val & 0x40)
            {
                sprintf(msg,
                        "R INTERRUPT STATUS:            TIMEOUT\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            /*else
             * {
             * fprintf(dec_dwl->regDump, "R SWREG%d:                     %08X\n", offset, val);
             * } */
        }
        else if(offset == 12 || offset == 52 || offset == 53 || offset == 56)
        {
            u32 tmp_val = *(dec_dwl->pRegBase + 1);
            /* check if ASO detected */
            if(((tmp_val >> 12) & 0xFF & 0x8) && offset == 12)
            {
                sprintf(msg,
                        "R STREAM END ADDRESS:          N.A. (ASO DETECTED)\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            /* check if error detected */
            else if(((tmp_val >> 12) & 0xFF & 0x10) && offset == 12)
            {
                sprintf(msg,
                        "R STREAM END ADDRESS:          N.A. (ERROR DETECTED)\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
            /* check if timeout detected */    
            else if (((tmp_val >> 12) & 0xFF & 0x40) && offset == 12)
            {
                sprintf(msg,
                        "R STREAM END ADDRESS:          N.A. (TIMEOUT DETECTED)\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }    
            else
            {
                if(offset == 12)
                {
                    u32 tmp_val2 =
                        ((*(dec_dwl->pRegBase + 3)) >> 27) & 0x01;
                    /* check if RLC mode -> swreg12 contains base address */
                    if(tmp_val2)
                    {
                        sprintf(msg,
                                "R STREAM END ADDRESS:          N.A. (RLC MODE ENABLED)\n");
								FS_Write (dec_dwl->regDump, msg, strlen(msg));
                    }
                    else
                    {
                        sprintf(msg,
                                "R STREAM END ADDRESS:          %u\n", val);
								FS_Write (dec_dwl->regDump, msg, strlen(msg));
                    }
                }
                else
                {
                    u32 tmp_val2 =
                        *(dec_dwl->pRegBase + 51) & 0x80000000;
                    if (tmp_val2)
                    {
                        if(offset == 52)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER HIT SUM:    %u\n",
                                    (val >> 16) & 0xFFFF);
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            sprintf(msg,
                                    "R REFERENCE BUFFER INTRA SUM:  %u\n",
                                    val & 0xFFFF);
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                        }
                        else if(offset == 53)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER Y_MV SUM:   %u\n",
                                    val & 0x3FFFFF);
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                        }
                        else if(offset == 56)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER TOP SUM:    %u\n",
                                    (val >> 16) & 0xFFFF);
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            sprintf(msg,
                                    "R REFERENCE BUFFER BOTTOM SUM: %u\n",
                                    val & 0xFFFF);
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            /* repeat the start address for JPEG slice mode */
                            if((tmp_val >> 12) & 0xFF & 0x20)
                            {
                                u32 tmp_val3 = (*(dec_dwl->pRegBase + 12)) >> 2;

                                sprintf(msg,
                                        "-\nW STREAM START ADDRESS:        %u\n",
                                        tmp_val3);
										  FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            }
                        }
                    }
                    else
                    {
                        if(offset == 52)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER HIT SUM:    N.A.\n");
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            sprintf(msg,
                                    "R REFERENCE BUFFER INTRA SUM:  N.A.\n");
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                        }
                        else if(offset == 53)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER Y_MV SUM:   N.A.\n");
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                        }
                        else if(offset == 56)
                        {
                            sprintf(msg,
                                    "R REFERENCE BUFFER TOP SUM:    N.A.\n");
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            sprintf(msg,
                                    "R REFERENCE BUFFER BOTTOM SUM: N.A.\n");
									 FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            /* repeat the start address for JPEG slice mode */
                            if((tmp_val >> 12) & 0xFF & 0x20)
                            {
                                u32 tmp_val3 = (*(dec_dwl->pRegBase + 12)) >> 2;

                                sprintf(msg,
                                        "-\nW STREAM START ADDRESS:        %u\n",
                                        tmp_val3);
										  FS_Write (dec_dwl->regDump, msg, strlen(msg));
                            }
                        }
                    }
                }
            }
        }
        else if(offset == 60)
        {
            u32 tmp_val = (val >> 12) & 0x01;

            if(tmp_val)
            {
                sprintf(msg,
                        "R PP INTERRUPT STATUS:         PICTURE PROCESSED\n");
					 FS_Write (dec_dwl->regDump, msg, strlen(msg));
            }
        }
        // fflush(dec_dwl->regDump);
		  FS_FSeek (dec_dwl->regDump, 0, SEEK_SET);
    }
#endif

#ifdef _DWL_FAKE_HW_TIMEOUT
    if(offset == HX170DEC_REG_START / 4)
    {
        DWLFakeTimeout(&val);
    }
#endif
    return val;
}

/*------------------------------------------------------------------------------
    Function name   : DWLEnableHW
    Description     : Enable hw by writing to register
    Return type     : void
    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLEnableHW(const void *instance, u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    DWLWriteReg(dec_dwl, offset, value);
    DWL_DEBUG("DWLEnableHW (by previous DWLWriteReg)\n");
}

/*------------------------------------------------------------------------------
    Function name   : DWLDisableHW
    Description     : Disable hw by writing to register
    Return type     : void
    Argument        : const void * instance - DWL instance
    Argument        : u32 offset - byte offset of the register to be written
    Argument        : u32 value - value to be written out
------------------------------------------------------------------------------*/
void DWLDisableHW(const void *instance, u32 offset, u32 value)
{
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    DWLWriteReg(dec_dwl, offset, value);
    DWL_DEBUG("DWLDisableHW (by previous DWLWriteReg)\n");
}

/*------------------------------------------------------------------------------
    Function name   : DWLWaitHwReady
    Description     : Wait until hardware has stopped running.
                      Used for synchronizing software runs with the hardware.
                      The wait could succed, timeout, or fail with an error.

    Return type     : i32 - one of the values DWL_HW_WAIT_OK
                                              DWL_HW_WAIT_TIMEOUT
                                              DWL_HW_WAIT_ERROR

    Argument        : const void * instance - DWL instance
------------------------------------------------------------------------------*/
i32 DWLWaitHwReady(const void *instance, u32 timeout)
{
    const hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    i32 ret;

    assert(dec_dwl);

    switch (dec_dwl->clientType)
    {
    case DWL_CLIENT_TYPE_H264_DEC:
    case DWL_CLIENT_TYPE_MPEG4_DEC:
    case DWL_CLIENT_TYPE_JPEG_DEC:
    case DWL_CLIENT_TYPE_VC1_DEC:
    case DWL_CLIENT_TYPE_MPEG2_DEC:
    case DWL_CLIENT_TYPE_RV_DEC:
    case DWL_CLIENT_TYPE_VP6_DEC:
    case DWL_CLIENT_TYPE_VP8_DEC:
    case DWL_CLIENT_TYPE_AVS_DEC:
        {
            ret = DWLWaitDecHwReady(dec_dwl, timeout);
            break;
        }
    case DWL_CLIENT_TYPE_PP:
        {
            ret = DWLWaitPpHwReady(dec_dwl, timeout);
            break;
        }
    default:
        {
            assert(0);  /* should not happen */
            ret = DWL_HW_WAIT_ERROR;
        }
    }

    return ret;
}

/*------------------------------------------------------------------------------
    Function name   : DWLmalloc
    Description     : Allocate a memory block. Same functionality as
                      the ANSI C kernel_malloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

    Argument        : u32 n - Bytes to allocate
------------------------------------------------------------------------------*/
void *DWLmalloc(u32 n)
{
#ifdef MEMORY_USAGE_TRACE
    printf("DWLmalloc\t%8d bytes\n", n);
#endif
    return kernel_malloc((size_t) n);
}

/*------------------------------------------------------------------------------
    Function name   : DWLfree
    Description     : Deallocates or frees a memory block. Same functionality as
                      the ANSI C kernel_free()

    Return type     : void

    Argument        : void *p - Previously allocated memory block to be freed
------------------------------------------------------------------------------*/
void DWLfree(void *p)
{
    if(p != NULL)
        kernel_free(p);
}

/*------------------------------------------------------------------------------
    Function name   : DWLcalloc
    Description     : Allocates an array in memory with elements initialized
                      to 0. Same functionality as the ANSI C calloc()

    Return type     : void pointer to the allocated space, or NULL if there
                      is insufficient memory available

    Argument        : u32 n - Number of elements
    Argument        : u32 s - Length in bytes of each element.
------------------------------------------------------------------------------*/
void *DWLcalloc(u32 n, u32 s)
{
#ifdef MEMORY_USAGE_TRACE
    printf("DWLcalloc\t%8d bytes\n", n * s);
#endif
    return kernel_calloc((size_t) n, (size_t) s);
}

/*------------------------------------------------------------------------------
    Function name   : DWLmemcpy
    Description     : Copies characters between buffers. Same functionality as
                      the ANSI C memcpy()

    Return type     : The value of destination d

    Argument        : void *d - Destination buffer
    Argument        : const void *s - Buffer to copy from
    Argument        : u32 n - Number of bytes to copy
------------------------------------------------------------------------------*/
void *DWLmemcpy(void *d, const void *s, u32 n)
{
    return memcpy(d, s, (size_t) n);
}

/*------------------------------------------------------------------------------
    Function name   : DWLmemset
    Description     : Sets buffers to a specified character. Same functionality
                      as the ANSI C memset()

    Return type     : The value of destination d

    Argument        : void *d - Pointer to destination
    Argument        : i32 c - Character to set
    Argument        : u32 n - Number of characters
------------------------------------------------------------------------------*/
void *DWLmemset(void *d, i32 c, u32 n)
{
    return memset(d, (int) c, (size_t) n);
}

extern int arkn141_codec_release_hw (int b_is_decoder);
extern int arkn141_codec_reserve_hw (int b_is_decoder);


/*------------------------------------------------------------------------------
    Function name   : DWLReserveHw
    Description     :
    Return type     : i32
    Argument        : const void *instance
------------------------------------------------------------------------------*/
i32 DWLReserveHw(const void *instance)
{
    i32 ret = 0;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

#if 1
	 if(arkn141_codec_reserve_hw (1))
	 {
		 printf ("DWLReserveHw error\n");
		 return DWL_ERROR;
	 }
#else
	 if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
	 {
		 DWL_DEBUG("DWL: PP locked by PID %d\n", OS_GetTaskID());
		 OS_Use(&dwl_pp_semid); 
	 }
	 else
	 {
		 DWL_DEBUG("DWL: Dec locked by PID %d\n", OS_GetTaskID());
		 OS_Use(&dwl_dec_semid); 
	 }

    DWL_DEBUG("DWL: DWLReserveHw success\n");
#endif

    //if(ret)
    //    return DWL_ERROR;
    //else
        return DWL_OK;
}

/*------------------------------------------------------------------------------
    Function name   : DWLReleaseHw
    Description     :
    Return type     : void
    Argument        : const void *instance
------------------------------------------------------------------------------*/
void DWLReleaseHw(const void *instance)
{
    i32 ret;
    hX170dwl_t *dec_dwl = (hX170dwl_t *) instance;

    /* dec_dwl->clientType -- identifies the client type */
#if 1
	 if(arkn141_codec_release_hw (1))
	 {
		 printf ("DWLReleaseHw error\n");
		 return;
	 }
#else
    DWL_DEBUG("DWL: HW trying to release by %d\n", OS_GetTaskID());
	 /*
    do
    {

        if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
            ret = binary_semaphore_post(dec_dwl->semid, 1);
        else
            ret = binary_semaphore_post(dec_dwl->semid, 0);

    }
    while(ret != 0 && errno == EINTR);*/
	 if(dec_dwl->clientType == DWL_CLIENT_TYPE_PP)
		 OS_Unuse(&dwl_pp_semid); 
	 else
		 OS_Unuse(&dwl_dec_semid); 
	 

    DWL_DEBUG("DWL: HW released by PID %d\n", OS_GetTaskID());
#endif
}

/*------------------------------------------------------------------------------
    Function name   : DWLFakeTimeout
    Description     : Testing help function that changes HW stream errors info
                        HW timeouts. You can check how the SW behaves or not.
    Return type     : void
    Argument        : void
------------------------------------------------------------------------------*/

#ifdef _DWL_FAKE_HW_TIMEOUT
void DWLFakeTimeout(u32 * status)
{

    if((*status) & DWL_STREAM_ERROR_BIT)
    {
        *status &= ~DWL_STREAM_ERROR_BIT;
        *status |= DWL_HW_TIMEOUT_BIT;
        printf("\nDWL: Change stream error to hw timeout\n");
    }
}
#endif

void dwl_init (void)
{
	OS_CREATERSEMA(&dwl_pp_semid);
	OS_CREATERSEMA(&dwl_dec_semid);
}

void dwl_exit (void)
{
	OS_DeleteRSema(&dwl_pp_semid);
	OS_DeleteRSema(&dwl_dec_semid);
}