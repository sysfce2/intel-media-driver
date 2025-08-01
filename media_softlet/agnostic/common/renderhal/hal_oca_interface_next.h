/*
* Copyright (c) 2021, Intel Corporation
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
//! \file     hal_oca_interface_next.h
//! \brief    Implementation of functions for HAL OCA Interface
//!

#ifndef __HAL_OCA_INTERFACE_NEXT_H__
#define __HAL_OCA_INTERFACE_NEXT_H__

#include "mhw_mi.h"
#include "mos_os_hw.h"
#include "mhw_mi_itf.h"
#include "media_defs.h"
/****************************************************************************************************/
/*                                      HalOcaInterface                                             */
/****************************************************************************************************/

class HalOcaInterfaceNext
{
public:
    //!
    //! \brief  Oca operation which should be called at the beginning of 1st level batch buffer start.
    //! \param  [in/out] cmdBuffer
    //!         Command buffer for current BB. hOcaBuf in cmdBuffer will be updated.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] gpuContextHandle
    //!         Gpu context handle
    //! \param  [in] miItf
    //!         Reference to Mhw MiItf.
    //! \param  [in] mmioRegisters
    //!         mmio registers for current engine.
    //! \param  [in] offsetOf1stLevelBB
    //!         Offset for current BB in cmdBuffer.
    //! \param  [in] bUseSizeOfCmdBuf
    //!         If true, use size of cmdBuffer for batch buffer, else use sizeOf1stLevelBB.
    //! \param  [in] sizeOf1stLevelBB
    //!         Size of BB. Ignore if bUseSizeOfResource == true.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void On1stLevelBBStart(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext,
        uint32_t gpuContextHandle, std::shared_ptr<mhw::mi::Itf> miItf, MHW_MI_MMIOREGISTERS &mmioRegisters,
        uint32_t offsetOf1stLevelBB = 0, bool bUseSizeOfCmdBuf = true, uint32_t sizeOf1stLevelBB = 0);

    //!
    //! \brief  Oca operation which should be called at the beginning of 1st level batch buffer start.
    //! \param  [in/out] cmdBuffer
    //!         Command buffer for current BB. hOcaBuf in cmdBuffer will be updated.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] gpuContextHandle
    //!         Gpu context handle
    //! \param  [in] miItf
    //!         Reference to Mhw MiItf.
    //! \param  [in] mmioRegisters
    //!         mmio registers for current engine.
    //! \param  [in] offsetOf1stLevelBB
    //!         Offset for current BB in cmdBuffer.
    //! \param  [in] bUseSizeOfCmdBuf
    //!         If true, use size of cmdBuffer for batch buffer, else use sizeOf1stLevelBB.
    //! \param  [in] sizeOf1stLevelBB
    //!         Size of BB. Ignore if bUseSizeOfResource == true.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void On1stLevelBBStart(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext,
        uint32_t gpuContextHandle, std::shared_ptr<mhw::mi::Itf> miItf, MmioRegistersMfx &mmioRegisters,
        uint32_t offsetOf1stLevelBB = 0, bool bUseSizeOfCmdBuf = true, uint32_t sizeOf1stLevelBB = 0);

    //!
    //! \brief  Oca operation which should be called before adding batch buffer end command for 1st
    //!         level batch buffer.
    //! \param  [in/out] cmdBuffer
    //!         Command buffer for current BB. hOcaBuf in cmdBuffer will be updated.
    //! \param  [in] osInterface
    //!         Reference to MOS_INTERFACE.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void On1stLevelBBEnd(MOS_COMMAND_BUFFER &cmdBuffer, MOS_INTERFACE &osInterface);

    //!
    //! \brief  Oca operation which should be called before sending start sub level batch buffer command.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] pMosResource
    //!         Pointer to the MOS_RESOURCE.
    //! \param  [in] offsetOfSubLevelBB
    //!         Offset for current BB in pMosResource.
    //! \param  [in] bUseSizeOfResource
    //!         If true, use size of pMosResource for batch buffer, else use sizeOfIndirectState.
    //! \param  [in] sizeOfSubLevelBB
    //!         Size of BB. Ignore if bUseSizeOfResource == true.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void OnSubLevelBBStart(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, void *pMosResource, uint32_t offsetOfSubLevelBB, bool bUseSizeOfResource, uint32_t sizeOfSubLevelBB);

    //!
    //! \brief  Oca operation which should be called when indirect states being added.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] pMosResource
    //!         Pointer to the MOS_RESOURCE.
    //! \param  [in] offsetOfIndirectState
    //!         Offset for current state in pMosResource.
    //! \param  [in] bUseSizeOfResource
    //!         If true, use size of pMosResource for indirect state, else use sizeOfIndirectState.
    //! \param  [in] sizeOfIndirectState
    //!         Size of indirect state. Ignore if bUseSizeOfResource == true.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void OnIndirectState(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, void *pMosResource, uint32_t offsetOfIndirectState, bool bUseSizeOfResource, uint32_t sizeOfIndirectState);

    //!
    //! \brief  Oca operation which should be called before adding dispatch states,
    //!         e.g. VEB_DI_IECP_STATE and MEDIA_OBJECT_WALKER.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] miItf
    //!         Reference to Mhw MiItf.
    //! \param  [in] mmioRegisters
    //!         mmio registers for current engine.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void OnDispatch(MOS_COMMAND_BUFFER &cmdBuffer, MOS_INTERFACE &osInterface, std::shared_ptr<mhw::mi::Itf> miItf, MHW_MI_MMIOREGISTERS &mmioRegisters);

    //!
    //! \brief  Add string to oca log section
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] str
    //!         string to be added.
    //! \param  [in] maxCount
    //!         size of the buffer pointed by str.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void TraceMessage(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, const char *str, uint32_t maxCount);

    //!
    //! \brief  Add vp kernel info to oca log section.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] osInterface
    //!         Reference to MOS_INTERFACE.
    //! \param  [in] res
    //!         Reference to MOS_RESOURCE.
    //! \param  [in] hwCmdType
    //!         Hw command type.
    //! \param  [in] locationInCmd
    //!         Location in command.
    //! \param  [in] offsetInRes
    //!         Offset in resource.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpResourceInfo(MOS_COMMAND_BUFFER &cmdBuffer, MOS_INTERFACE &osInterface, MOS_RESOURCE &res, MOS_HW_COMMAND hwCmdType, uint32_t locationInCmd, uint32_t offsetInRes);

    //!
    //! \brief  Insert bindless/stateless resource state heap into OcaBufferHandlerMap
    //! \param  [in] key
    //!         The base address of resource state heap
    //! \param  [in] osInterface
    //!         Reference to PMOS_INTERFACE.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void InsertResourceHeapToCurrentCmdBufferOcaBufferHandle(uint32_t *key, PMOS_INTERFACE osInterface, PMOS_COMMAND_BUFFER _cmdBuffer);

    //!
    //! \brief  Trace OCA Sku Value.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] osInterface
    //!         Reference to MOS_INTERFACE.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void TraceOcaSkuValue(MOS_COMMAND_BUFFER &cmdBuffer, MOS_INTERFACE &osInterface);

    //!
    //! \brief  Add vp kernel info to oca log section.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] vpKernelID
    //!         Value of enum VpKernelID.
    //! \param  [in] fcKernelCount
    //!         If vpKernelID == kernelCombinedFc, fcKernelCount is the kernel count for fc, otherwise, it's not used.
    //! \param  [in] fcKernelList
    //!         If vpKernelID == kernelCombinedFc, fcKernelList is the kernel list for fc, otherwise, it's not used.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpVpKernelInfo(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, int vpKernelID, int fcKernelCount, int *fcKernelList, int aiKernelFeatureType = 0);

    //!
    //! \brief  Add vp kernel info to oca log section.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] pControlValues
    //!         Value of user features.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpVpUserFeautreControlInfo(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, PMOS_OCA_LOG_USER_FEATURE_CONTROL_INFO pControlValues);

    //!
    //! \brief  Add vphal parameters to oca log section.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] pVphalDumper
    //!         Pointer to vphal dumper object.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpVphalParam(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, void *pVphalDumper);

    //!
    //! \brief  Add codechal parameters to oca log section.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] pCodechalDumper
    //!         Pointer to codechal dumper object.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpCodechalParam(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, void *pCodechalDumper, CODECHAL_STANDARD codec);

    //!
    //! \brief  Get large resource dump support state
    //! \return bool
    //!         Return true when support large reource dump
    //!
    static bool IsLargeResouceDumpSupported();

    //!
    //! \brief  Add cp IO Message to oca log section.
    //! \param  [in] ocaInterface
    //!         Reference to MosOcaInterface.
    //! \param  [in] hOcaBuf
    //!         Reference to MOS_OCA_BUFFER_HANDLE.
    //! \param  [in] mosCtx
    //!         DDI device context.
    //! \param  [in] pCpDumper
    //!         Pointer to cp dumper object.
    //! \param  [in] type
    //!         Cp message type.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpCpIoMsg(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext, void *pCpDumper, int type);

protected:
    static MOS_STATUS MhwMiLoadRegisterImmCmd(
        std::shared_ptr<mhw::mi::Itf>    miItf,
        PMOS_COMMAND_BUFFER              pCmdBuffer,
        MHW_MI_LOAD_REGISTER_IMM_PARAMS  *params);

    //!
    //! \brief  Error handle function.
    //! \param  [in] mosCtx
    //!         the ddi device context.
    //! \param  [in] status
    //!         The MOS_STATUS for current error.
    //! \param  [in] funcName
    //!         The failure function name.
    //! \param  [in] lineNumber
    //!         The line number where OnOcaError being called in failure function.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void OnOcaError(MOS_CONTEXT_HANDLE mosContext, MOS_STATUS status, const char *functionName, uint32_t lineNumber);
    //!
    //! \brief  Get OCA buffer handle from pool.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \return MOS_OCA_BUFFER_HANDLE
    //!         MOS_OCA_BUFFER_HANDLE.
    //!
    static MOS_OCA_BUFFER_HANDLE GetOcaBufferHandle(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT_HANDLE mosContext);

    //!
    //! \brief  Remove OCA buffer handle from pool.
    //! \param  [in] cmdBuffer
    //!         Command buffer for current BB.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void RemoveOcaBufferHandle(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT &mosContext);

    //!
    //! \brief  Add cp parameters to oca log section.
    //! \param  [in] ocaInterface
    //!         Reference to MosOcaInterface.
    //! \param  [in] hOcaBuf
    //!         Reference to MOS_OCA_BUFFER_HANDLE.
    //! \param  [in] mosCtx
    //!         DDI device context.
    //! \param  [in] pCpDumper
    //!         Pointer to cp dumper object.
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void DumpCpParam(MosOcaInterface &ocaInterface, MOS_OCA_BUFFER_HANDLE &hOcaBuf, PMOS_CONTEXT mosCtx, void *pCpDumper);

    //!
    //! \brief  Oca operation which should be called at the beginning of 1st level batch buffer start.
    //! \param  [in/out] cmdBuffer
    //!         Command buffer for current BB. ocaBufHandle in cmdBuffer will be updated.
    //! \param  [in] mosContext
    //!         Reference to MOS_CONTEXT.
    //! \param  [in] gpuContextHandle
    //!         Gpu context handle 
    //! \return void
    //!         No return value. Handle all exception inside the function.
    //!
    static void On1stLevelBBStart(MOS_COMMAND_BUFFER &cmdBuffer, MOS_CONTEXT &mosContext, uint32_t gpuContextHandle);

    static void AddRTLogReource(MOS_COMMAND_BUFFER &cmdBuffer,
                               MOS_CONTEXT_HANDLE  mosContext,
                               MOS_INTERFACE &osInterface);

    // Private functions to ensure class singleton.
    HalOcaInterfaceNext();
    HalOcaInterfaceNext(HalOcaInterfaceNext &);
    HalOcaInterfaceNext& operator= (HalOcaInterfaceNext &);

MEDIA_CLASS_DEFINE_END(HalOcaInterfaceNext)
};


#endif // __RHAL_OCA_INTERFACE_H__
