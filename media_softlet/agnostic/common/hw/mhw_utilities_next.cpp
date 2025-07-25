/*
* Copyright (c) 2014-2022, Intel Corporation
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
* OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/
//!
//! \file      mhw_utilities.c
//! \brief         This modules implements utilities which are shared by both the HW interface     and the state heap interface.
//!

#include <math.h>
#include <set>
#include "mhw_utilities_next.h"
#include "mhw_state_heap.h"
#include "mos_interface.h"
#include "hal_oca_interface_next.h"
#include "media_skuwa_specific.h"
#include "mhw_itf.h"
#include "mhw_mi.h"
#include "mhw_mi_cmdpar.h"
#include "mhw_mi_itf.h"

#define MHW_NS_PER_TICK_RENDER_ENGINE 80  // 80 nano seconds per tick in render engine

static const std::set<MOS_HW_COMMAND> s_OCAResourceInfoType = {
    MOS_MI_BATCH_BUFFER_START,
    MOS_VEBOX_STATE,
    MOS_VEBOX_DI_IECP,
    MOS_VEBOX_TILING_CONVERT,
    MOS_SFC_STATE,
    MOS_STATE_BASE_ADDR,
    MOS_SURFACE_STATE,
    MOS_SURFACE_STATE_ADV,
    MOS_MFX_PIPE_BUF_ADDR,
    MOS_MFX_INDIRECT_OBJ_BASE_ADDR,
    MOS_MFX_BSP_BUF_BASE_ADDR,
    MOS_MFX_AVC_DIRECT_MODE,
    MOS_MFX_VP8_PIC,
    MOS_HUC_IND_OBJ_BASE_ADDR,
    MOS_HUC_DMEM,
    MOS_HUC_VIRTUAL_ADDR,
    MOS_VDENC_PIPE_BUF_ADDR,
    MOS_BINDLESS_STATELESS_SURFACE,
};

//!
//! \brief    Set mocs index
//! \details  Set mocs index
//!           command buffer or indirect state
//! \param    PMOS_INTERFACE osInterface
//!           [in] OS interface
//! \param    PMOS_RESOURCE resource
//!           [in] resource
//! \param    MHW_MOCS_PARAMS &mocsParams
//!           [in] mocsParams
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_SetMocsTableIndex(
    PMOS_INTERFACE       osInterface,
    PMOS_RESOURCE        resource,
    MHW_MOCS_PARAMS      &mocsParams)
{
    MHW_CHK_NULL_RETURN(resource);
    MHW_CHK_NULL_RETURN(osInterface);

    // Index is defined in bit 1:6
    const uint8_t indexBitFieldLow      = 1;
    const uint8_t indexMask             = 0x3F;
    uint32_t      memObjCtrlStateValue  = 0;

    uint32_t    *data                   = mocsParams.mocsTableIndex;
    uint32_t    value                   = 0;
    uint32_t    mask                    = 0;
    uint8_t     bitFieldLow             = mocsParams.bitFieldLow;
    uint8_t     bitFieldHigh            = mocsParams.bitFieldHigh;

    if (data == nullptr)
    {
        MHW_NORMALMESSAGE("skip to set the mocs");
        return MOS_STATUS_SUCCESS;
    }

    if (bitFieldLow > bitFieldHigh || bitFieldHigh > 31)
    {
        MOS_OS_ASSERTMESSAGE("invalid bit field");
        return MOS_STATUS_INVALID_PARAMETER;
    }

    value = *data;

    auto memObjCtrlState = osInterface->pfnGetResourceCachePolicyMemoryObject(osInterface, resource);
    memObjCtrlStateValue = (memObjCtrlState.DwordValue >> indexBitFieldLow) & indexMask;

    if (bitFieldHigh == 31)
    {
        mask = (1 << bitFieldLow) - 1;
    }
    else
    {
        mask = (~((1 << (bitFieldHigh + 1)) - 1)) | ((1 << bitFieldLow) - 1);
    }
    value = value & mask;
    *data = value | (memObjCtrlStateValue << bitFieldLow);

    return MOS_STATUS_SUCCESS;
}

//!
//! \brief    Adds graphics address of a resource to the command buffer or indirect state
//! \details  Internal MHW function to add the graphics address of resources to the
//!           command buffer or indirect state
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] OS interface
//! \param    PMOS_COMMAND_BUFFER pCmdBuffer
//!           [in] Command buffer
//! \param    PMHW_RESOURCE_PARAMS pParams
//!           [in] Parameters necessary to insert the graphics address
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_AddResourceToCmd_GfxAddress(
    PMOS_INTERFACE              pOsInterface,
    PMOS_COMMAND_BUFFER         pCmdBuffer,
    PMHW_RESOURCE_PARAMS        pParams)
{
    uint32_t                dwAlign, dwMask;
    uint32_t                dwGfxAddrBottom, dwGfxAddrTop = 0;
    uint64_t                ui64GfxAddress, ui64GfxAddressUpperBound;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;
    int32_t                 iAllocationIndex = 0;
    uint32_t                uiPatchOffset = 0;
    uint8_t                 *pbCmdBufBase = nullptr;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pParams);
    MHW_CHK_NULL_RETURN(pParams->presResource);
    MHW_CHK_NULL_RETURN(pCmdBuffer);
    MHW_CHK_NULL_RETURN(pCmdBuffer->pCmdBase);

    pbCmdBufBase = (uint8_t*)pCmdBuffer->pCmdBase;

    MHW_CHK_STATUS_RETURN(pOsInterface->pfnRegisterResource(
        pOsInterface,
        pParams->presResource,
        pParams->bIsWritable ? true : false,
        pParams->bIsWritable ? true : false));

    dwAlign = ( 1 << pParams->dwLsbNum);
    dwMask  = (-1 << pParams->dwLsbNum);

    pParams->dwOffset = MOS_ALIGN_CEIL(pParams->dwOffset, dwAlign);
    ui64GfxAddress =
        pOsInterface->pfnGetResourceGfxAddress(pOsInterface, pParams->presResource) + pParams->dwOffset;
    MHW_CHK_COND(ui64GfxAddress == 0, "Driver can't add resource with ui64GfxAddress == 0. DW location in cmd == %d.", pParams->dwLocationInCmd);

    dwGfxAddrBottom = (uint32_t)(ui64GfxAddress & 0x00000000FFFFFFFF);
    dwGfxAddrTop = (uint32_t)((ui64GfxAddress & 0xFFFFFFFF00000000) >> 32);

    *pParams->pdwCmd = (*pParams->pdwCmd & ~dwMask) | (dwGfxAddrBottom & dwMask);
    // this is next DW for top part of the address
    *(pParams->pdwCmd + 1) = dwGfxAddrTop;

    Mhw_SetMocsTableIndex(pOsInterface, pParams->presResource, pParams->mocsParams);

#if (_DEBUG || _RELEASE_INTERNAL)
    {
        uint32_t evtData[4] ={(uint32_t)pParams->HwCommandType, pParams->dwLocationInCmd, pParams->dwOffset, pParams->dwSize};
        MOS_TraceEventExt(EVENT_RESOURCE_REGISTER, EVENT_TYPE_INFO2, evtData, sizeof(evtData), &ui64GfxAddress, sizeof(ui64GfxAddress));
    }
#endif

    if (pParams->dwOffsetInSSH > 0)
    {
        // Calculate the patch offset to command buffer
        uiPatchOffset = pParams->dwOffsetInSSH + (pParams->dwLocationInCmd * sizeof(uint32_t));
    }
    else
    {
        // Calculate the patch offset to command buffer
        uiPatchOffset = pCmdBuffer->iOffset + (pParams->dwLocationInCmd * sizeof(uint32_t));
    }

    MOS_PATCH_ENTRY_PARAMS PatchEntryParams;

    iAllocationIndex = pOsInterface->pfnGetResourceAllocationIndex(pOsInterface, pParams->presResource);
    MOS_ZeroMemory(&PatchEntryParams, sizeof(PatchEntryParams));
    PatchEntryParams.uiAllocationIndex = iAllocationIndex;
    PatchEntryParams.uiResourceOffset = pParams->dwOffset;
    PatchEntryParams.uiPatchOffset    = uiPatchOffset;
    PatchEntryParams.bWrite           = pParams->bIsWritable;
    PatchEntryParams.HwCommandType    = pParams->HwCommandType;
    PatchEntryParams.forceDwordOffset = pParams->dwSharedMocsOffset;
    PatchEntryParams.cmdBufBase       = pbCmdBufBase;
    PatchEntryParams.presResource     = pParams->presResource;
    PatchEntryParams.cmdBuffer        = pCmdBuffer;

    // Add patch entry
    MHW_CHK_STATUS_RETURN(pOsInterface->pfnSetPatchEntry(
        pOsInterface,
        &PatchEntryParams));

    if (pParams->dwUpperBoundLocationOffsetFromCmd > 0)
    {
        pParams->dwSize = MOS_ALIGN_CEIL(pParams->dwSize, dwAlign);

        ui64GfxAddressUpperBound = ui64GfxAddress + pParams->dwSize;
        dwGfxAddrBottom          = (uint32_t)(ui64GfxAddressUpperBound & 0x00000000FFFFFFFF);
        dwGfxAddrTop             = (uint32_t)((ui64GfxAddressUpperBound & 0xFFFFFFFF00000000) >> 32);

        pParams->pdwCmd += pParams->dwUpperBoundLocationOffsetFromCmd;
        *pParams->pdwCmd = (*pParams->pdwCmd & ~dwMask) | (dwGfxAddrBottom & dwMask);
        // this is next DW for top part of the address
        *(pParams->pdwCmd + 1) = dwGfxAddrTop;

        MOS_PATCH_ENTRY_PARAMS PatchEntryParams;

        // Calculate the patch offset to command buffer
        uiPatchOffset += pParams->dwUpperBoundLocationOffsetFromCmd * sizeof(uint32_t);

        MOS_ZeroMemory(&PatchEntryParams, sizeof(PatchEntryParams));
        PatchEntryParams.uiAllocationIndex = iAllocationIndex;
        PatchEntryParams.uiResourceOffset = pParams->dwOffset + pParams->dwSize;
        PatchEntryParams.uiPatchOffset    = uiPatchOffset;
        PatchEntryParams.bUpperBoundPatch = true;
        PatchEntryParams.presResource     = pParams->presResource;

        // Add patch entry (CP won't register this patch point since bUpperBoundPatch = true)
        MHW_CHK_STATUS_RETURN(pOsInterface->pfnSetPatchEntry(
            pOsInterface,
            &PatchEntryParams));
    }

    if (s_OCAResourceInfoType.count(pParams->HwCommandType))
    {
        HalOcaInterfaceNext::DumpResourceInfo(*pCmdBuffer, *pOsInterface, *pParams->presResource, pParams->HwCommandType,
            pParams->dwLocationInCmd, pParams->dwOffset);
    }

    return eStatus;
}

//!
//! \brief    Adds resources to a patch list
//! \details  Internal MHW function to put resources to be added to the command
//!           buffer or indirect state into a patch list for patch later
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] OS interface
//! \param    PMOS_COMMAND_BUFFER pCmdBuffer
//!           [in] Command buffer
//! \param    PMHW_RESOURCE_PARAMS pParams
//!           [in] Parameters necessary to add the resource to the patch list
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_AddResourceToCmd_PatchList(
    PMOS_INTERFACE              pOsInterface,
    PMOS_COMMAND_BUFFER         pCmdBuffer,
    PMHW_RESOURCE_PARAMS        pParams)
{
    MOS_GPU_CONTEXT         GpuContext;
    int32_t                 iAllocationIndex;
    uint32_t                dwLsbNum, dwUpperBoundOffset;
    uint32_t                dwOffset;
    uint32_t                uiPatchOffset;
    MOS_PATCH_ENTRY_PARAMS  PatchEntryParams;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pParams);
    MHW_CHK_NULL_RETURN(pParams->presResource);
    MHW_CHK_NULL_RETURN(pCmdBuffer);

    MOS_TraceEventExt(EVENT_RESOURCE_REGISTER, EVENT_TYPE_INFO2, &pParams->HwCommandType, sizeof(uint32_t), &pParams->dwLocationInCmd, sizeof(uint32_t));

    MHW_CHK_STATUS_RETURN(pOsInterface->pfnRegisterResource(
        pOsInterface,
        pParams->presResource,
        pParams->bIsWritable ? true : false,
        pParams->bIsWritable ? true : false));

    GpuContext = pOsInterface->pfnGetGpuContext(pOsInterface);
    iAllocationIndex = pOsInterface->pfnGetResourceAllocationIndex(pOsInterface, pParams->presResource);
    dwLsbNum = pParams->dwLsbNum;

    // Offset and command LSB parameters
    dwOffset = pParams->dwOffset | ((*pParams->pdwCmd) & ((1 << dwLsbNum) - 1));

    Mhw_SetMocsTableIndex(pOsInterface, pParams->presResource, pParams->mocsParams);
    
    if (pParams->dwOffsetInSSH > 0)
    {
        // Calculate the patch offset to command buffer
        uiPatchOffset = pParams->dwOffsetInSSH + (pParams->dwLocationInCmd * sizeof(uint32_t));
    }
    else
    {
        // Calculate the patch offset to command buffer
        uiPatchOffset = pCmdBuffer->iOffset + (pParams->dwLocationInCmd * sizeof(uint32_t));
    }

    MOS_ZeroMemory(&PatchEntryParams, sizeof(PatchEntryParams));
    PatchEntryParams.uiAllocationIndex = iAllocationIndex;
    if(pParams->patchType == MOS_PATCH_TYPE_UV_Y_OFFSET ||
       pParams->patchType == MOS_PATCH_TYPE_PITCH ||
       pParams->patchType == MOS_PATCH_TYPE_V_Y_OFFSET)
    {
        PatchEntryParams.uiResourceOffset = *pParams->pdwCmd;
    }
    else
    {
        PatchEntryParams.uiResourceOffset = dwOffset;
    }
    PatchEntryParams.uiPatchOffset    = uiPatchOffset;
    PatchEntryParams.bWrite           = pParams->bIsWritable;
    PatchEntryParams.HwCommandType    = pParams->HwCommandType;
    PatchEntryParams.forceDwordOffset = pParams->dwSharedMocsOffset;
    PatchEntryParams.cmdBufBase       = (uint8_t*)pCmdBuffer->pCmdBase;
    PatchEntryParams.presResource     = pParams->presResource;
    PatchEntryParams.patchType        = pParams->patchType;
    PatchEntryParams.shiftAmount      = pParams->shiftAmount;
    PatchEntryParams.shiftDirection   = pParams->shiftDirection;
    PatchEntryParams.offsetInSSH      = pParams->dwOffsetInSSH;
    PatchEntryParams.cmdBuffer        = pCmdBuffer;

    // Add patch entry to patch the address field for this command
    MHW_CHK_STATUS_RETURN(pOsInterface->pfnSetPatchEntry(
        pOsInterface,
        &PatchEntryParams));

    if (pParams->dwUpperBoundLocationOffsetFromCmd > 0)
    {
        pParams->pdwCmd += pParams->dwUpperBoundLocationOffsetFromCmd;
        dwUpperBoundOffset = pParams->dwUpperBoundLocationOffsetFromCmd;

        // Offset and command LSB parameters
        dwOffset = MOS_ALIGN_CEIL((pParams->dwOffset + pParams->dwSize), (1 << dwLsbNum));
        dwOffset = dwOffset | ((*pParams->pdwCmd) & ((1 << dwLsbNum) - 1));

        // Calculate the patch offset to command buffer
        uiPatchOffset += dwUpperBoundOffset * sizeof(uint32_t);

        MOS_ZeroMemory(&PatchEntryParams, sizeof(PatchEntryParams));
        PatchEntryParams.uiAllocationIndex = iAllocationIndex;
        PatchEntryParams.uiResourceOffset = dwOffset;
        PatchEntryParams.uiPatchOffset    = uiPatchOffset;
        PatchEntryParams.bUpperBoundPatch = true;
        PatchEntryParams.presResource     = pParams->presResource;
        PatchEntryParams.patchType        = pParams->patchType;
        PatchEntryParams.shiftAmount      = pParams->shiftAmount;
        PatchEntryParams.shiftDirection   = pParams->shiftDirection;
        PatchEntryParams.offsetInSSH      = pParams->dwOffsetInSSH;
        PatchEntryParams.cmdBuffer        = pCmdBuffer;

        if(dwLsbNum)
        {
            PatchEntryParams.shiftAmount = dwLsbNum;
            PatchEntryParams.shiftDirection = 0;
        }

        // Add patch entry to patch the address field for this command
        MHW_CHK_STATUS_RETURN(pOsInterface->pfnSetPatchEntry(
            pOsInterface,
            &PatchEntryParams));
    }

    if (s_OCAResourceInfoType.count(pParams->HwCommandType))
    {
        HalOcaInterfaceNext::DumpResourceInfo(*pCmdBuffer, *pOsInterface, *pParams->presResource, pParams->HwCommandType,
            pParams->dwLocationInCmd, pParams->dwOffset);
    }

    return eStatus;
}

//!
//! \brief    Derive Surface Type from Surface Format
//! \details  Internal MHW function to dervie surface type from surface format
//! \param    uint32_t dwForceSurfaceFormat
//!           [in] surface format information
//! \param    PMOS_SURFACE psSurface
//!           [in] surface which have depth information
//! \param    uint32_t* pdwSurfaceType
//!           [out] Surface type
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_SurfaceFormatToType(
    uint32_t                       dwForceSurfaceFormat,
    PMOS_SURFACE                   psSurface,
    uint32_t                       *pdwSurfaceType)
{
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(psSurface);
    MHW_CHK_NULL_RETURN(pdwSurfaceType);

    switch ( dwForceSurfaceFormat )
    {
        // 1D Surface
        case MHW_GFX3DSTATE_SURFACEFORMAT_RAW:
        case MHW_GFX3DSTATE_SURFACEFORMAT_R32_UINT:
        case MHW_GFX3DSTATE_SURFACEFORMAT_L8_UNORM:
            *pdwSurfaceType = GFX3DSTATE_SURFACETYPE_BUFFER;
            break;

        // 2D Surface for Codec: GFX3DSTATE_SURFACEFORMAT_YCRCB_NORMAL, GFX3DSTATE_SURFACEFORMAT_YCRCB_SWAPY
        // GFX3DSTATE_SURFACEFORMAT_R32_UNORM, GFX3DSTATE_SURFACEFORMAT_R16_UNORM, GFX3DSTATE_SURFACEFORMAT_R8_UNORM

        // 2D & 3D Surface
        default:
            (psSurface->dwDepth > 1) ?
                *pdwSurfaceType = GFX3DSTATE_SURFACETYPE_3D:
                *pdwSurfaceType = GFX3DSTATE_SURFACETYPE_2D;
    }

    return eStatus;
}

//!
//! \brief    Inserts the generic prologue command for a command buffer
//! \details  Client facing function to add the generic prologue commands:
//!               - the command buffer header (if necessary)
//!               - flushes for the read/write caches (MI_FLUSH_DW or PIPE_CONTROL)
//!               - CP prologue if necessary
//! \param    PMOS_COMMAND_BUFFER pCmdBuffer
//!           [in] Command buffer
//! \param    PMHW_GENERIC_PROLOG_PARAMS pParams
//!           [in] Parameters necessary to add the generic prologue commands
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_SendGenericPrologCmdNext(
    PMOS_COMMAND_BUFFER           pCmdBuffer,
    PMHW_GENERIC_PROLOG_PARAMS    pParams,
    std::shared_ptr<void>         pMiItf,
    MHW_MI_MMIOREGISTERS          *pMmioReg)
{
    PMOS_INTERFACE                  pOsInterface;
    MEDIA_FEATURE_TABLE             *pSkuTable;
    MEDIA_WA_TABLE                  *pWaTable;
    MOS_GPU_CONTEXT                 GpuContext;
    MHW_PIPE_CONTROL_PARAMS         PipeControlParams;
    MHW_MI_FLUSH_DW_PARAMS          FlushDwParams;
    bool                            bRcsEngineUsed = false;
    MOS_STATUS                      eStatus = MOS_STATUS_SUCCESS;
    std::shared_ptr<mhw::mi::Itf>   miItf = std::static_pointer_cast<mhw::mi::Itf>(pMiItf);
    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(pCmdBuffer);
    MHW_CHK_NULL_RETURN(pParams);
    MHW_CHK_NULL_RETURN(pParams->pOsInterface);
    MHW_CHK_NULL_RETURN(miItf);

    pOsInterface = pParams->pOsInterface;

    pSkuTable = pOsInterface->pfnGetSkuTable(pOsInterface);
    MHW_CHK_NULL_RETURN(pSkuTable);
    pWaTable = pOsInterface->pfnGetWaTable(pOsInterface);
    MHW_CHK_NULL_RETURN(pWaTable);
    GpuContext = pOsInterface->pfnGetGpuContext(pOsInterface);
    if (pOsInterface->pfnIsGpuSyncByCmd(pOsInterface, pOsInterface->CurrentGpuContextHandle) && pCmdBuffer->syncMhwBatchBuffer != nullptr)  // Some gpu context may not support sync with batch buffer
    {
        //Reset params
        auto &miBatchBufferStartParams = miItf->MHW_GETPAR_F(MI_BATCH_BUFFER_START)();
        uint64_t gfxAddr = pOsInterface->pfnGetResourceGfxAddress(pOsInterface, &pCmdBuffer->OsResource);
        pOsInterface->pfnOnNativeFenceSyncBBAdded(pCmdBuffer, gfxAddr);
        miBatchBufferStartParams       = {};
        MHW_CHK_STATUS_RETURN(miItf->MHW_ADDCMD_F(MI_BATCH_BUFFER_START)(pCmdBuffer, pCmdBuffer->syncMhwBatchBuffer));
    }

    if ( pOsInterface->Component != COMPONENT_CM )
    {
        if (    GpuContext == MOS_GPU_CONTEXT_RENDER        ||
                GpuContext == MOS_GPU_CONTEXT_RENDER2       ||
                GpuContext == MOS_GPU_CONTEXT_RENDER3       ||
                GpuContext == MOS_GPU_CONTEXT_RENDER4       ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO         ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO2        ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO3        ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO4        ||
                GpuContext == MOS_GPU_CONTEXT_VDBOX2_VIDEO  ||
                GpuContext == MOS_GPU_CONTEXT_VDBOX2_VIDEO2 ||
                GpuContext == MOS_GPU_CONTEXT_VDBOX2_VIDEO3 ||
                GpuContext == MOS_GPU_CONTEXT_VEBOX         ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO5        ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO6        ||
                GpuContext == MOS_GPU_CONTEXT_VIDEO7        )
        {
            MHW_CHK_STATUS_RETURN(miItf->AddWatchdogTimerStartCmd(pCmdBuffer));
        }
    }

    bRcsEngineUsed = MOS_RCS_ENGINE_USED(GpuContext);

    if (bRcsEngineUsed)
    {
        auto& par = miItf->MHW_GETPAR_F(PIPE_CONTROL)();
        par = {};
        par.dwFlushMode = MHW_FLUSH_WRITE_CACHE;
        MHW_CHK_STATUS_RETURN(miItf->MHW_ADDCMD_F(PIPE_CONTROL)(pCmdBuffer));

        auto& par1 = miItf->MHW_GETPAR_F(PIPE_CONTROL)();
        par1 = {};
        par1.dwFlushMode = MHW_FLUSH_READ_CACHE;
        par1.presDest = pParams->presStoreData;
        par1.dwResourceOffset = pParams->dwStoreDataOffset;
        par1.dwPostSyncOp = MHW_FLUSH_WRITE_IMMEDIATE_DATA;
        MHW_CHK_STATUS_RETURN(miItf->MHW_ADDCMD_F(PIPE_CONTROL)(pCmdBuffer));

        if(pCmdBuffer->Attributes.bUmdSSEUEnable)
        {
            MHW_MI_LOAD_REGISTER_IMM_PARAMS MiLoadRegImmParams;
            MHW_RENDER_PWR_CLK_STATE_PARAMS params;

            MOS_ZeroMemory(&params, sizeof(params));
            params.PowerClkStateEn  = true;
            params.SCountEn         = true;
            params.SSCountEn        = true;
            params.SliceCount       = pCmdBuffer->Attributes.dwNumRequestedEUSlices;
            params.SubSliceCount    = pCmdBuffer->Attributes.dwNumRequestedSubSlices;
            params.EUmax            = pCmdBuffer->Attributes.dwNumRequestedEUs;
            params.EUmin            = pCmdBuffer->Attributes.dwNumRequestedEUs;

            auto& par = miItf->MHW_GETPAR_F(MI_LOAD_REGISTER_IMM)();
            par = {};
            par.dwRegister = MHW__PWR_CLK_STATE_REG;
            par.dwData = params.Data;
            MHW_CHK_STATUS_RETURN(miItf->MHW_ADDCMD_F(MI_LOAD_REGISTER_IMM)(pCmdBuffer));
        }
    }
    else
    {
        // Send MI_FLUSH with protection bit off, which will FORCE exit protected mode for MFX
        auto& params = miItf->MHW_GETPAR_F(MI_FLUSH_DW)();
        params = {};
        params.bVideoPipelineCacheInvalidate = true;
        params.pOsResource = pParams->presStoreData;
        params.dwResourceOffset = pParams->dwStoreDataOffset;
        params.dwDataDW1 = pParams->dwStoreDataValue;
        MHW_CHK_STATUS_RETURN(miItf->MHW_ADDCMD_F(MI_FLUSH_DW)(pCmdBuffer));
    }

    MHW_CHK_STATUS_RETURN(miItf->AddProtectedProlog(pCmdBuffer));

    if (pMmioReg)
    {
        HalOcaInterfaceNext::On1stLevelBBStart(
            *pCmdBuffer,
            (MOS_CONTEXT_HANDLE)pOsInterface->pOsContext,
            pOsInterface->CurrentGpuContextHandle,
            miItf,
            *pMmioReg);
    }

    return eStatus;
}

//!
//! \brief      Sets Nearest Mode Table for Gen75/9, across SFC and Render engine to set the sampler states
//! \details    This function sets Coefficients for Nearest Mode
//! \param      int32_t*   iCoefs
//!             [out]   Polyphase Table to fill
//! \param      uint32_t   dwPlane
//!             [in]    Number of Polyphase tables
//! \param      bool    bBalancedFilter
//!             [in]    If Filter is balanced, set true
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_SetNearestModeTable(
    int32_t         *iCoefs,
    uint32_t        dwPlane,
    bool            bBalancedFilter)
{
    uint32_t                dwNumEntries;
    uint32_t                dwOffset;
    uint32_t                i;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(iCoefs);

    if (dwPlane == MHW_GENERIC_PLANE || dwPlane == MHW_Y_PLANE)
    {
        dwNumEntries = NUM_POLYPHASE_Y_ENTRIES;
        dwOffset = 3;
    }
    else // if (dwPlane == MHW_U_PLANE || dwPlane == MHW_V_PLANE)
    {
        dwNumEntries = NUM_POLYPHASE_UV_ENTRIES;
        dwOffset = 1;
    }

    for (i = 0; i <= NUM_HW_POLYPHASE_TABLES / 2; i++)
    {
        iCoefs[i * dwNumEntries + dwOffset] = 0x40;
    }

    if (bBalancedFilter)
    {
        // Fix offset so that filter is balanced
        for (i = (NUM_HW_POLYPHASE_TABLES / 2 + 1); i < NUM_HW_POLYPHASE_TABLES; i++)
        {
            iCoefs[i * dwNumEntries + dwOffset + 1] = 0x40;
        }
    }

    return eStatus;
}

//!
//! \brief      Calculate Polyphase tables for Y , across SFC and Render engine to set the sampler states
//! \details    Calculate Polyphase tables for Y
//!             This function uses 17 phases.
//!             MHW_NUM_HW_POLYPHASE_TABLES reflects the phases to program coefficients in HW, and
//!             NUM_POLYPHASE_TABLES reflects the number of phases used for internal calculations.
//! \param      int32_t*   iCoefs
//!             [out]   Polyphase Table to fill
//! \param      float   fScaleFactor
//!             [in]    Scaling factor
//! \param      uint32_t   dwPlane
//!             [in]    Plane Info
//! \param      MOS_FORMAT srcFmt
//!             [in]    Source Format
//! \param      float   fHPStrength
//!             [in]    High Pass Strength
//! \param      bool    bUse8x8Filter
//!             [in]    is 8x8 Filter used
//! \param      uint32_t   dwHwPhase
//!             [in]    Number of phases in HW
//! \param      float      fLanczosT
//!             [in]    Lanczos factor
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_CalcPolyphaseTablesY(
    int32_t         *iCoefs,
    float           fScaleFactor,
    uint32_t        dwPlane,
    MOS_FORMAT      srcFmt,
    float           fHPStrength,
    bool            bUse8x8Filter,
    uint32_t        dwHwPhase,
    float           fLanczosT)
{
    uint32_t                dwNumEntries;
    uint32_t                dwTableCoefUnit;
    uint32_t                i, j;
    int32_t                 k;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;
    float                   fPhaseCoefs[NUM_POLYPHASE_Y_ENTRIES];
    float                   fPhaseCoefsCopy[NUM_POLYPHASE_Y_ENTRIES];
    float                   fStartOffset;
    float                   fHPFilter[3], fHPSum, fHPHalfPhase; // Only used for Y_PLANE
    float                   fBase, fPos, fSumCoefs;
    int32_t                 iCenterPixel;
    int32_t                 iSumQuantCoefs;

    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(iCoefs);
    MHW_ASSERT((dwHwPhase == MHW_NUM_HW_POLYPHASE_TABLES) || (dwHwPhase == NUM_HW_POLYPHASE_TABLES));

    if (dwPlane == MHW_GENERIC_PLANE || dwPlane == MHW_Y_PLANE)
    {
        dwNumEntries = NUM_POLYPHASE_Y_ENTRIES;
    }
    else // if (dwPlane == MHW_U_PLANE || dwPlane == MHW_V_PLANE)
    {
        dwNumEntries = NUM_POLYPHASE_UV_ENTRIES;
    }

    MOS_ZeroMemory(fPhaseCoefs    , sizeof(fPhaseCoefs));
    MOS_ZeroMemory(fPhaseCoefsCopy, sizeof(fPhaseCoefsCopy));

    dwTableCoefUnit = 1 << MHW_AVS_TBL_COEF_PREC;
    iCenterPixel = dwNumEntries / 2 - 1;
    fStartOffset = (float)(-iCenterPixel);

    if ((IS_YUV_FORMAT(srcFmt)    &&
        dwPlane != MHW_U_PLANE    &&
        dwPlane != MHW_V_PLANE)   ||
        ((IS_RGB32_FORMAT(srcFmt) ||
        srcFmt == Format_Y410     ||
        srcFmt == Format_AYUV)    &&
        dwPlane == MHW_Y_PLANE))
    {
        if (fScaleFactor < 1.0F)
        {
            fLanczosT = 4.0F;
        }
        else
        {
            fLanczosT = 8.0F;
        }
    }
    else // if (dwPlane == MHW_U_PLANE || dwPlane == MHW_V_PLANE || (IS_RGB_FORMAT(srcFmt) && dwPlane != MHW_V_PLANE))
    {
        fLanczosT = 2.0F;
    }

    for (i = 0; i < dwHwPhase; i++)
    {
        fBase = fStartOffset - (float)i / (float)NUM_POLYPHASE_TABLES;
        fSumCoefs = 0.0F;

        for (j = 0; j < dwNumEntries; j++)
        {
            fPos = fBase + (float)j;

            if (bUse8x8Filter)
            {
                fPhaseCoefs[j] = fPhaseCoefsCopy[j] = MosUtilities::MosLanczos(fPos * fScaleFactor, dwNumEntries, fLanczosT);
            }
            else
            {
                fPhaseCoefs[j] = fPhaseCoefsCopy[j] = MosUtilities::MosLanczosG(fPos * fScaleFactor, NUM_POLYPHASE_5x5_Y_ENTRIES, fLanczosT);
            }

            fSumCoefs += fPhaseCoefs[j];
        }

        // Convolve with HP
        if (dwPlane == MHW_GENERIC_PLANE || dwPlane == MHW_Y_PLANE)
        {
            if (i <= NUM_POLYPHASE_TABLES / 2)
            {
                fHPHalfPhase = (float)i / (float)NUM_POLYPHASE_TABLES;
            }
            else
            {
                fHPHalfPhase = (float)(NUM_POLYPHASE_TABLES - i) / (float)NUM_POLYPHASE_TABLES;
            }
            fHPFilter[0] = fHPFilter[2] = -fHPStrength * MosUtilities::MosSinc(fHPHalfPhase * MOS_PI);
            fHPFilter[1] = 1.0F + 2.0F * fHPStrength;

            for (j = 0; j < dwNumEntries; j++)
            {
                fHPSum = 0.0F;
                for (k = -1; k <= 1; k++)
                {
                    if ((((long)j + k) >= 0) && (j + k < dwNumEntries))
                    {
                        fHPSum += fPhaseCoefsCopy[(int32_t)j+k] * fHPFilter[k+1];
                    }
                    fPhaseCoefs[j] = fHPSum;
                }
            }
        }

        // Normalize coefs and save
        iSumQuantCoefs = 0;
        for (j = 0; j < dwNumEntries; j++)
        {
            iCoefs[i * dwNumEntries + j] = (int32_t)floor(0.5F + (float)dwTableCoefUnit * fPhaseCoefs[j] / fSumCoefs);
            iSumQuantCoefs += iCoefs[i * dwNumEntries + j];
        }

        // Fix center coef so that filter is balanced
        if (i <= NUM_POLYPHASE_TABLES / 2)
        {
            iCoefs[i * dwNumEntries + iCenterPixel] -= iSumQuantCoefs - dwTableCoefUnit;
        }
        else
        {
            iCoefs[i * dwNumEntries + iCenterPixel + 1] -= iSumQuantCoefs - dwTableCoefUnit;
        }
    }

    return eStatus;
}

//!
//! \brief      Calculate Polyphase tables for UV for Gen9, across SFC and Render engine to set the sampler states
//! \details    Calculate Polyphase tables for UV
//! \param      int32_t*   piCoefs
//!             [out]   Polyphase Table to fill
//! \param      float   fLanczosT
//!             [in]    Lanczos modifying factor
//! \param      float   fInverseScaleFactor
//!             [in]    Inverse scaling factor
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_CalcPolyphaseTablesUV(
    int32_t    *piCoefs,
    float      fLanczosT,
    float      fInverseScaleFactor)
{
    int32_t     phaseCount, tableCoefUnit, centerPixel, sumQuantCoefs;
    double      phaseCoefs[MHW_SCALER_UV_WIN_SIZE];
    double      startOffset, sf, base, sumCoefs, pos;
    int32_t     minCoef[MHW_SCALER_UV_WIN_SIZE];
    int32_t     maxCoef[MHW_SCALER_UV_WIN_SIZE];
    int32_t     i, j;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(piCoefs);

    phaseCount      = MHW_TABLE_PHASE_COUNT;
    centerPixel     = (MHW_SCALER_UV_WIN_SIZE / 2) - 1;
    startOffset     = (double)(-centerPixel);
    tableCoefUnit   = 1 << MHW_TBL_COEF_PREC;
    sf              = MOS_MIN(1.0, fInverseScaleFactor); // Sf isn't used for upscaling

    MOS_ZeroMemory(piCoefs, sizeof(int32_t) * MHW_SCALER_UV_WIN_SIZE * phaseCount);
    MOS_ZeroMemory(minCoef, sizeof(minCoef));
    MOS_ZeroMemory(maxCoef, sizeof(maxCoef));

    if (sf < 1.0F)
    {
        fLanczosT = 2.0F;
    }

    for(i = 0; i < phaseCount; ++i, piCoefs += MHW_SCALER_UV_WIN_SIZE)
    {
        // Write all
        // Note - to shift by a half you need to a half to each phase.
        base     = startOffset - (double)(i) / (double)(phaseCount);
        sumCoefs = 0.0;

        for(j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            pos             = base + (double) j;
            phaseCoefs[j]   = MosUtilities::MosLanczos((float)(pos * sf), MHW_SCALER_UV_WIN_SIZE, fLanczosT);
            sumCoefs        += phaseCoefs[j];
        }
        // Normalize coefs and save
        for(j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            piCoefs[j] = (int32_t) floor((0.5 + (double)(tableCoefUnit) * (phaseCoefs[j] / sumCoefs)));

            //For debug purposes:
            minCoef[j] = MOS_MIN(minCoef[j], piCoefs[j]);
            maxCoef[j] = MOS_MAX(maxCoef[j], piCoefs[j]);
        }

        // Recalc center coef
        sumQuantCoefs = 0;
        for(j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            sumQuantCoefs += piCoefs[j];
        }

        // Fix center coef so that filter is balanced
        if (i <= phaseCount/2)
        {
            piCoefs[centerPixel]     -= sumQuantCoefs - tableCoefUnit;
        }
        else
        {
            piCoefs[centerPixel + 1] -= sumQuantCoefs - tableCoefUnit;
        }
    }

    return eStatus;
}

//!
//! \brief      Calculate polyphase tables UV offset for Gen9, across SFC and Render engine to set the sampler states
//! \details    Calculate Polyphase tables for UV with chroma siting for
//!             420 to 444 conversion
//! \param      int32_t*   piCoefs
//!             [out]   Polyphase Table to fill
//! \param      float   fLanczosT
//!             [in]    Lanczos modifying factor
//! \param      float   fInverseScaleFactor
//!             [in]    Inverse scaling factor
//! \param      int32_t     iUvPhaseOffset
//!             [in]    UV Phase Offset
//! \return   MOS_STATUS
//!           MOS_STATUS_SUCCESS if success, else fail reason
//!
MOS_STATUS Mhw_CalcPolyphaseTablesUVOffset(
    int32_t     *piCoefs,
    float       fLanczosT,
    float       fInverseScaleFactor,
    int32_t     iUvPhaseOffset)
{
    int32_t     phaseCount, tableCoefUnit, centerPixel, sumQuantCoefs;
    double      phaseCoefs[MHW_SCALER_UV_WIN_SIZE];
    double      startOffset, sf, pos, sumCoefs, base;
    int32_t     minCoef[MHW_SCALER_UV_WIN_SIZE];
    int32_t     maxCoef[MHW_SCALER_UV_WIN_SIZE];
    int32_t     i, j;
    int32_t     adjusted_phase;
    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_FUNCTION_ENTER;

    MHW_CHK_NULL_RETURN(piCoefs);

    phaseCount = MHW_TABLE_PHASE_COUNT;
    centerPixel = (MHW_SCALER_UV_WIN_SIZE / 2) - 1;
    startOffset = (double)(-centerPixel +
        (double)iUvPhaseOffset / (double)(phaseCount));
    tableCoefUnit = 1 << MHW_TBL_COEF_PREC;

    MOS_ZeroMemory(minCoef, sizeof(minCoef));
    MOS_ZeroMemory(maxCoef, sizeof(maxCoef));
    MOS_ZeroMemory(piCoefs, sizeof(int32_t)* MHW_SCALER_UV_WIN_SIZE * phaseCount);

    sf = MOS_MIN(1.0, fInverseScaleFactor); // Sf isn't used for upscaling
    if (sf < 1.0)
    {
        fLanczosT = 3.0;
    }

    for (i = 0; i < phaseCount; ++i, piCoefs += MHW_SCALER_UV_WIN_SIZE)
    {
        // Write all
        // Note - to shift by a half you need to a half to each phase.
        base = startOffset - (double)(i) / (double)(phaseCount);
        sumCoefs = 0.0;

        for (j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            pos = base + (double)j;
            phaseCoefs[j] = MosUtilities::MosLanczos((float)(pos * sf), 6/*MHW_SCALER_UV_WIN_SIZE*/, fLanczosT);
            sumCoefs += phaseCoefs[j];
        }
        // Normalize coefs and save
        for (j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            piCoefs[j] = (int32_t)floor((0.5 + (double)(tableCoefUnit)* (phaseCoefs[j] / sumCoefs)));

            // For debug purposes:
            minCoef[j] = MOS_MIN(minCoef[j], piCoefs[j]);
            maxCoef[j] = MOS_MAX(maxCoef[j], piCoefs[j]);
        }

        // Recalc center coef
        sumQuantCoefs = 0;
        for (j = 0; j < MHW_SCALER_UV_WIN_SIZE; ++j)
        {
            sumQuantCoefs += piCoefs[j];
        }

        // Fix center coef so that filter is balanced
        adjusted_phase = i - iUvPhaseOffset;
        if (adjusted_phase <= phaseCount / 2)
        {
            piCoefs[centerPixel] -= sumQuantCoefs - tableCoefUnit;
        }
        else // if(adjusted_phase < phaseCount)
        {
            piCoefs[centerPixel + 1] -= sumQuantCoefs - tableCoefUnit;
        }
    }

    return eStatus;
}

//!
//! \brief    Allocate BB
//! \details  Allocated Batch Buffer
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] Pointer to OS Interface
//! \param    PMHW_BATCH_BUFFER pBatchBuffer
//!           [in/out] Pointer to Batch Buffer
//! \param    PMHW_BATCH_BUFFER pBatchBufferList
//!           [in/out] If valid, represents the batch buffer list maintained by the client
//! \param    uint32_t dwSize
//!           [in] Sixe of the batch vuffer to be allocated
//! \param    bool notLockable
//!           [in] Indicate if the batch buffer not lockable, by default is false
//! \param    bool inSystemMem
//!           [in] Indicate if the batch buffer in system memory, by default is false
//! \return   MOS_STATUS
//!           true  if the Batch Buffer was successfully allocated
//!           false if failed
//!
MOS_STATUS Mhw_AllocateBb(
    PMOS_INTERFACE          pOsInterface,
    PMHW_BATCH_BUFFER       pBatchBuffer,
    PMHW_BATCH_BUFFER       pBatchBufferList,
    uint32_t                dwSize,
    uint32_t                batchCount,
    bool                    notLockable,
    bool                    inSystemMem)
{
    MHW_FUNCTION_ENTER;

    MOS_RESOURCE                        OsResource;
    MOS_ALLOC_GFXRES_PARAMS             AllocParams;
    uint32_t                            allocSize;
    MOS_STATUS                          eStatus = MOS_STATUS_SUCCESS;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pBatchBuffer);
    MHW_ASSERT(!(notLockable && inSystemMem)); // Notlockable system memory doesn't make sense

    dwSize += 8 * MHW_CACHELINE_SIZE;
    dwSize = MOS_ALIGN_CEIL(dwSize, MOS_PAGE_SIZE);
    allocSize = dwSize * batchCount;

    MOS_ZeroMemory(&OsResource, sizeof(OsResource));
    MOS_ZeroMemory(&AllocParams, sizeof(AllocParams));
    AllocParams.Type     = MOS_GFXRES_BUFFER;
    AllocParams.TileType = MOS_TILE_LINEAR;
    AllocParams.Format   = Format_Buffer;
    AllocParams.dwBytes  = allocSize;
    AllocParams.pBufName = "BatchBuffer";
    AllocParams.ResUsageType = MOS_HW_RESOURCE_USAGE_MEDIA_BATCH_BUFFERS;
    AllocParams.Flags.bNotLockable = notLockable ? 1 : 0;
    if (notLockable)
    {
        AllocParams.dwMemType = MOS_MEMPOOL_DEVICEMEMORY;
    }
    else if (inSystemMem)
    {
        AllocParams.dwMemType = MOS_MEMPOOL_SYSTEMMEMORY;
    }
    else
    {
        AllocParams.dwMemType = MOS_MEMPOOL_VIDEOMEMORY;
    }

    MHW_CHK_STATUS_RETURN(pOsInterface->pfnAllocateResource(
        pOsInterface,
        &AllocParams,
        &OsResource));

    // Reset Allocation
    pOsInterface->pfnResetResourceAllocationIndex(pOsInterface, &OsResource);

    pBatchBuffer->OsResource        = OsResource;
    pBatchBuffer->iSize             = (int32_t)dwSize;
    pBatchBuffer->count             = batchCount;
    pBatchBuffer->iRemaining        = pBatchBuffer->iSize;
    pBatchBuffer->iCurrent          = 0;
    pBatchBuffer->bLocked           = false;
#if (_DEBUG || _RELEASE_INTERNAL)
    pBatchBuffer->iLastCurrent      = 0;
#endif

    // Link BB for synchronization
    pBatchBuffer->bBusy             = false;
    pBatchBuffer->dwCmdBufId        = 0;

    if (pBatchBufferList)
    {
        pBatchBuffer->pNext = pBatchBufferList;
        pBatchBufferList = pBatchBuffer;
        if (pBatchBuffer->pNext)
        {
            pBatchBuffer->pNext->pPrev = pBatchBuffer;
        }
    }

    return eStatus;
}

//!
//! \brief    Free BB
//! \details  Frees Batch Buffer
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] Pointer to OS Interface
//! \param    PMHW_BATCH_BUFFER pBatchBuffer
//!           [in] Pointer to Batch Buffer
//! \param    PMHW_BATCH_BUFFER pBatchBufferList
//!           [in/out] If valid, represents the batch buffer list maintained by the client
//! \return   MOS_STATUS
//!
MOS_STATUS Mhw_FreeBb(
    PMOS_INTERFACE          pOsInterface,
    PMHW_BATCH_BUFFER       pBatchBuffer,
    PMHW_BATCH_BUFFER       pBatchBufferList)
{
    MHW_FUNCTION_ENTER;

    MOS_STATUS              eStatus = MOS_STATUS_SUCCESS;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pBatchBuffer);

    if (pBatchBuffer->bLocked)
    {
        MHW_CHK_STATUS_RETURN(Mhw_UnlockBb(pOsInterface, pBatchBuffer, true));
    }

    pOsInterface->pfnFreeResource(pOsInterface, &pBatchBuffer->OsResource);

    pBatchBuffer->dwCmdBufId       = 0;
    pBatchBuffer->iSize            = 0;
    pBatchBuffer->count            = 0;
    pBatchBuffer->iCurrent         = 0;
#if (_DEBUG || _RELEASE_INTERNAL)
    pBatchBuffer->iLastCurrent     = 0;
#endif

    if (pBatchBufferList)
    {
        // Unlink BB from synchronization list
        if (pBatchBuffer->pNext)
        {
            pBatchBuffer->pNext->pPrev = pBatchBuffer->pPrev;
        }

        if (pBatchBuffer->pPrev)
        {
            pBatchBuffer->pPrev->pNext = pBatchBuffer->pNext;
        }
        else
        {
            pBatchBufferList = pBatchBuffer->pNext;
        }

        pBatchBuffer->pPrev = pBatchBuffer->pNext = nullptr;
    }

    return eStatus;
}

//!
//! \brief    Lock BB
//! \details  Locks Batch Buffer
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] Pointer to OS Interface
//! \param    PMHW_BATCH_BUFFER pBatchBuffer
//!           [in] Pointer to Batch Buffer
//! \return   MOS_STATUS
//!
MOS_STATUS Mhw_LockBb(
    PMOS_INTERFACE          pOsInterface,
    PMHW_BATCH_BUFFER       pBatchBuffer)
{
    MHW_FUNCTION_ENTER;

    MOS_LOCK_PARAMS         LockFlags;
    MOS_STATUS              eStatus;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pBatchBuffer);

    eStatus         = MOS_STATUS_UNKNOWN;

    if (pBatchBuffer->bLocked)
    {
        MHW_ASSERTMESSAGE("Batch Buffer is already locked.");
        return eStatus;
    }

    MOS_ZeroMemory(&LockFlags, sizeof(MOS_LOCK_PARAMS));
    LockFlags.WriteOnly = 1;
    pBatchBuffer->pData = (uint8_t*)pOsInterface->pfnLockResource(
        pOsInterface,
        &pBatchBuffer->OsResource,
        &LockFlags);

    MHW_CHK_NULL_RETURN(pBatchBuffer->pData);

    pBatchBuffer->bLocked   = true;
    eStatus                 = MOS_STATUS_SUCCESS;

    return eStatus;
}

//!
//! \brief    Unlock BB
//! \details  Unlocks Batch Buffer
//! \param    PMOS_INTERFACE pOsInterface
//!           [in] Pointer to OS Interface
//! \param    PMHW_BATCH_BUFFER pBatchBuffer
//!           [in] Pointer to Batch Buffer
//! \param    bool bResetBuffer
//!           [in] Reset BB information concerning current offset and size
//! \return   MOS_STATUS
//!
MOS_STATUS Mhw_UnlockBb(
    PMOS_INTERFACE          pOsInterface,
    PMHW_BATCH_BUFFER       pBatchBuffer,
    bool                    bResetBuffer)
{
    MHW_FUNCTION_ENTER;

    MOS_STATUS              eStatus;

    MHW_CHK_NULL_RETURN(pOsInterface);
    MHW_CHK_NULL_RETURN(pBatchBuffer);

    eStatus = MOS_STATUS_UNKNOWN;

    if (!pBatchBuffer->bLocked)
    {
        MHW_ASSERTMESSAGE("Batch buffer is locked.");
        return eStatus;
    }

    if (bResetBuffer)
    {
        pBatchBuffer->iRemaining    = pBatchBuffer->iSize;
        pBatchBuffer->iCurrent      = 0;
    }

    MHW_CHK_STATUS_RETURN(pOsInterface->pfnUnlockResource(
        pOsInterface,
        &pBatchBuffer->OsResource));

    pBatchBuffer->bLocked = false;
    pBatchBuffer->pData   = nullptr;

    eStatus = MOS_STATUS_SUCCESS;

    return eStatus;
}

//!
//! \brief    Convert To Nano Seconds
//! \details  Convert to Nano Seconds
//! \param    PVPHAL_HW_int32_tERFACE pHwInterface
//!           [in] Pointer to Hardware Interface Structure
//! \param    uint64_t iTicks
//!           [in] Ticks
//! \param    uint64_t* piNs
//!           [in] Nano Seconds
//! \return   MOS_STATUS
//!
MOS_STATUS Mhw_ConvertToNanoSeconds(
    uint64_t                              iTicks,
    uint64_t                              *piNs)
{
    MOS_STATUS eStatus = MOS_STATUS_SUCCESS;

    MHW_CHK_NULL_RETURN(piNs);

    *piNs = iTicks * MHW_NS_PER_TICK_RENDER_ENGINE;

    return eStatus;
}

//!
//! \brief    Convert Mos Tile Type to TR Mode
//! \details  Convert Mos Tile Type to TR Mode
//! \param    MOS_TILE_TYPE  Type
//!           [in] MOS tile type
//! \return   uint32_t
//!           Tile Resouece Mode
//!
uint32_t Mhw_ConvertToTRMode(MOS_TILE_TYPE  Type)
{
    switch (Type)
    {
    case MOS_TILE_YS:
        return TRMODE_TILEYS;
    case MOS_TILE_YF:
        return TRMODE_TILEYF;
    default:
        return TRMODE_NONE;
    }
}
