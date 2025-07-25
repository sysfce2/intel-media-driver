/*
* Copyright (c) 2009-2023, Intel Corporation
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
//! \file     mos_interface.h
//! \brief    MOS interface definition
//!
//! Device level: OsDeviceContext in device/Libva context.
//!               The global level of entire media driver instance in user space.
//!               There could be multiple devices in a single process.
//! Stream level: OsStreamState in Hal instances (Codec pipeline, VP pipeline, CM device, CP session, etc)
//!               Each Hal instance can have an OsStreamState to indicate that stream's state specific to OS.
//!               Each Device can have multiple streams.
//!               All OsStreamStates must be binded with a valid OsDeviceContext to indicate the inclusion relationship
//!               between device and stream in DDI
//!
//! MOS interface provide following OS services: (OS services are abstracted and diff OS behavior is tranparent to MOS customers)
//! 1) Workload scheduling (GPU context, cmdbuffer, sync, virtual engine, etc)
//! 2) Resource managment (Graphic resource, external resource)
//! 3) Utilities (Abstraction of generalized system call)
//! 4) Performance interface
//! 5) Debug interface
//!
//! Caller: DDI, Media interface, HAL, MHW
//! Any interface func returning MOS_STATUS_UNKNOWN mean Device level is go into unstable situation.
//! Caller needs to make sure exiting properly.


#ifndef __MOS_INTERFACE_H__
#define __MOS_INTERFACE_H__

#include "mos_defs.h"
#include "mos_oca_rtlog_mgr_defs.h"
#include "mos_os.h"
#include "media_class_trace.h"

class GpuContextSpecificNext;
struct _MOS_VIRTUALENGINE_SET_PARAMS;
struct _MOS_VIRTUALENGINE_INIT_PARAMS;
struct SYNC_FENCE_INFO_TRINITY;

typedef struct _MOS_VIRTUALENGINE_SET_PARAMS  MOS_VIRTUALENGINE_SET_PARAMS, *PMOS_VIRTUALENGINE_SET_PARAMS;
typedef struct _MOS_VIRTUALENGINE_INIT_PARAMS MOS_VIRTUALENGINE_INIT_PARAMS, *PMOS_VIRTUALENGINE_INIT_PARAMS;
typedef struct _MOS_CMD_BUF_ATTRI_VE MOS_CMD_BUF_ATTRI_VE, *PMOS_CMD_BUF_ATTRI_VE;
typedef struct _MHW_VDBOX_GPUNODE_LIMIT *PMHW_VDBOX_GPUNODE_LIMIT;
#if !EMUL
inline bool IsGMMMapped(GMM_CLIENT_CONTEXT *gmmClientContext, PADAPTER_INFO adapterInfo, GMM_RESOURCE_FLAG& flags)
{
    if (gmmClientContext                            &&
        adapterInfo->SkuTable.FtrE2ECompression     &&
        !adapterInfo->SkuTable.FtrFlatPhysCCS       &&
        !adapterInfo->WaTable.WaAuxTable64KGranular &&
        flags.Info.MediaCompressed)
    {
        return true;
    }
    else
    {
        return false;
    }
}
#endif
class MosInterface
{
protected:
    //!
    //! \brief   Destructor
    //! \details There is no members in Mos Interface, it's pure interface.
    //!          Never call the Destructor of Mos interface
    //!
    ~MosInterface() = default;
    
    //!
    //! \brief   Constructor
    //! \details There is no members in Mos Interface, it's pure interface.
    //!          Never call the Constructor of Mos interface
    //!
    MosInterface() = default;

public:
    //!
    //! \brief    Init Os Utilities
    //! \details  Include Utilities, user settings key, mem ninja etc
    //! \details  Must be first called MOS interface before CreateOsDeviceContext
    //! \details  Caller: DDI only.
    //!
    //! \param    [in] ddiDeviceContext
    //!           Pointer of device context in DDI to init Os Device Context
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS InitOsUtilities(DDI_DEVICE_CONTEXT ddiDeviceContext);

    //!
    //! \brief    Close Os Utilities
    //! \details  Include Utilities, user settings key, mem ninja etc
    //! \details  Must be last called MOS interface after DestroyOsDeviceContext
    //! \details  Caller: DDI only.
    //!
    //! \param    [in] mosCtx
    //!           Pointer of device context in DDI for reg ops
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CloseOsUtilities(PMOS_CONTEXT mosCtx);

    //!
    //! \brief    Init Os context interface
    //! \details  Init Os context interface
    //!
    //! \param    [in/out] ctxInterface
    //!           Pointer of MOS_CONTEXT_INTERFACE
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS InitCtxInterface(MOS_CONTEXT_INTERFACE *ctxInterface);

    //!
    //! \brief    Create Os Device Context
    //! \details  Create the Os Device Context in device level.
    //! \details  Caller: DDI only.
    //! \details  The Os Device Context is a singleton in the device, DDI must make sure call this only once.
    //!           If the creation failed, DDI must yield to continue the initialization of device.
    //!
    //! \param    [in] ddiDeviceContext
    //!           Pointer of device context in DDI to init Os Device Context
    //! \param    [out] deviceContext
    //!           Handle of Os Device Context to create. If creation failed, it is INVALID_HANLE.
    //!           OsDeviceContext is a device level singleton which stores the states, info specific to OS.
    //!           It contain sub modules of MOS to transfer OS specific services to OS agnositic abstractions.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CreateOsDeviceContext(DDI_DEVICE_CONTEXT ddiDeviceContext, MOS_DEVICE_HANDLE *deviceContext);
    
    //!
    //! \brief    Destroy Os Device Context
    //! \details  Destroy the Os Device Context in device level
    //! \details  Caller: DDI only.
    //!
    //! \param    [in] deviceContext
    //!           Handle of Os Device Context to Destroy
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DestroyOsDeviceContext(MOS_DEVICE_HANDLE deviceContext);

    //!
    //! \brief    Create Os Stream State
    //! \details  Create the Os Stream State in stream level.
    //! \details  Caller: DDI
    //! \details  In DDI, one stream (Hal instance) can only create one Os Stream State corresponding to it.
    //!           Os Stream state directly created by DDI is not corresponding any streams (Hal instances)
    //!
    //! \param    [out] streamState
    //!           Handle of Os Stream State to create. If creation failed, it is INVALID_HANLE.
    //!           OsStreamState is a stream level state which stores the flags, info specific to OS specfic to that stream.
    //!           It is be binded with a valid OsDeviceContext to indicate the inclusion relationship between device and stream.
    //! \param    [in] deviceContext
    //!           Device context to init streamState
    //! \param    [in] osInterface
    //!           Os interface to store streamState
    //! \param    [in] component
    //!           Indicate which component the stream state to create belongs to
    //! \param    [in] extraParams
    //!           Additional parameters needed to init streamstate
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CreateOsStreamState(
        MOS_STREAM_HANDLE       *streamState,
        MOS_DEVICE_HANDLE       deviceContext,
        MOS_INTERFACE_HANDLE    osInterface,
        MOS_COMPONENT           component,
        EXTRA_PARAMS            extraParams = nullptr);

    //!
    //! \brief    Destroy Os Stream State
    //! \details  Destroy the Os Stream State in stream level
    //! \details  Caller: DDI
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State to Destroy
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!     
    static MOS_STATUS DestroyOsStreamState(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get OS runtime interface version
    //! \details  [System info Interface] Get OS runtime interface version
    //! \details  Caller: DDI only
    //! \details  Only DDI can derive diff behavior due to OS runtime interface version
    //!
    //! \param    [in] deviceContext
    //!           Handle of Os Device Context
    //!
    //! \return   uint32_t
    //!           Read-only OS runtime interface version, it's meaning diff from OS and API
    //!
    static uint32_t GetInterfaceVersion(MOS_DEVICE_HANDLE deviceContext);

    //!
    //! \brief    Get Platform
    //! \details  [System info Interface] Get Get Platform information
    //! \details  Caller: DDI & HAL & MHW
    //! \details  This func is called in DDI only to generate hal instance stand for specific platform.
    //!           This func can be used in HAL & MHW to get platfrom detailed info to judge the path of different behavior.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   PLATFORM
    //!           Gfx driver shared enum of platform got. Read-only.
    //!
    static PLATFORM *GetPlatform(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get SkuTable
    //! \details  [System info Interface] Get Sku Table
    //! \details  Caller: DDI & HAL & MHW
    //! \details  This func is called to differentiate the behavior according to SKU table.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   MEDIA_FEATURE_TABLE*
    //!           Read-only SKU table got, nullptr if failed to get
    //!
    static MEDIA_FEATURE_TABLE *GetSkuTable(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get WaTable
    //! \details  [System info Interface] Get WA Table
    //! \details  Caller: DDI & HAL & MHW
    //! \details  This func is called to differentiate the behavior according to WA table.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   MEDIA_WA_TABLE*
    //!           Read-only WA table got, nullptr if failed to get
    //!
    static MEDIA_WA_TABLE *GetWaTable(MOS_STREAM_HANDLE streamState);
    
    //!
    //! \brief    Get Gt System Info
    //! \details  [System info Interface] Get Gt System Info
    //! \details  Caller: HAL & MHW
    //! \details  This func is called to differentiate the behavior according to Gt System Info.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   MEDIA_SYSTEM_INFO*
    //!           Read-only GT system info got, nullptr if failed to get
    //!
    static MEDIA_SYSTEM_INFO *GetGtSystemInfo(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get Media Engine Info
    //! \details  [System info Interface] Get Media Engine Info
    //! \details  Caller: HAL & MHW
    //! \details  This func is called to differentiate the behavior according to Media Engine Info.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] info
    //!           MEDIA_SYS_INFO
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetMediaEngineInfo(MOS_STREAM_HANDLE streamState, MEDIA_ENGINE_INFO &info);

    //!
    //! \brief    Get Adapter Info
    //! \details  [System info Interface] Get Adapter Info
    //! \details  Caller: DDI & HAL
    //! \details  This func is called to differentiate the behavior according to Adapter Info.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   ADAPTER_INFO*
    //!           Read-only Adapter Info got, nullptr if failed to get
    //!
    static ADAPTER_INFO *GetAdapterInfo(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get current gmmclientcontext
    //! \details  Get current gmmclientcontext
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   GMM_CLIENT_CONTEXT
    //!           Current gmmclientcontext
    //!
    static GMM_CLIENT_CONTEXT *GetGmmClientContext(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief  Get PAT index from gmm
    //!
    //! \param  [in] gmmClient
    //!         GMM client context
    //! \param  [in] gmmResourceInfo
    //!         gmm resource info
    //!
    //! \return unsigned int
    //!         Pat index
    //!
    static unsigned int GetPATIndexFromGmm(
        GMM_CLIENT_CONTEXT *gmmClient,
        GMM_RESOURCE_INFO *gmmResourceInfo);

    //!
    //! \brief    Get current Gpu context priority
    //! \details  Get current Gpu context priority
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //!           [out]
    //!           Current Gpu context priority
    //!
    static void GetGpuPriority(MOS_STREAM_HANDLE streamState, int32_t* priority);

    //!
    //! \brief    Set current Gpu context priority
    //! \details  Set current Gpu context priority
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!           [in] priority
    //!           priority to set for gpu context
    //!
    static void SetGpuPriority(MOS_STREAM_HANDLE streamState, int32_t priority);

    //!
    //! \brief    Get AuxTable base address
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   uint64_t
    //!           64bit base address value of AuxTable
    //!
    static uint64_t GetAuxTableBaseAddr(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Create Gpu Context
    //! \details  [GPU Context Interface] Create Gpu Context to submit cmdbuffers
    //! \details  Caller: HAL (Media Context) only
    //! \details  This func is called when a stream (Hal instance) needs a SW queue to submit cmd buffers programmed with GPU cmds.
    //! \details  This queue contain options to indicate the properties of virtual GPU engine to execute these cmds.
    //! \details  Caller can use Usage & option & GPU_CONTEXT_HANDLE to track and re-use the GPU contexts.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] createOption
    //!           Properties of Gpu context to create. They stand for the request from HAL on the Gpu context.
    //!           The request include engine type, pipe count, restrictions, etc.
    //! \param    [out] gpuContext
    //!           Handle of gpu Context created. If creation failed, it is INVALID_HANLE
    //!           GPU context stands for a SW queue in user space to submit cmd buffers FIFO.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CreateGpuContext(
        MOS_STREAM_HANDLE streamState,
        GpuContextCreateOption &createOption,
        GPU_CONTEXT_HANDLE &gpuContext);
       
    //!
    //! \brief    Destroy Gpu Context
    //! \details  [GPU Context Interface] Destroy Gpu Context to submit cmdbuffers
    //! \details  Caller: HAL (Media Context) only
    //! \details  This func is called when a stream (Hal instance) never needs this SW queue to submit cmd buffers
    //! \details  This func is called only in the destruction stage of Hal instance. 
    //!           Never should be SetGpuContext called to set destroied Gpu Context.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           Handle of gpu Context to destroy.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DestroyGpuContext(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuContext);

    //!
    //! \brief    Set Gpu Context
    //! \details  [GPU Context Interface] Set current Gpu Context to submit cmd buffers for the stream(Hal instance)
    //! \details  Caller: HAL (Media Context) only
    //! \details  This func is called when a stream (Hal instance) needs an existing GPU context to submit cmd buffers.
    //! \details  Current GPU context is the major state of Os Stream State.
    //! \details  Before getting a cmd buffer to program GPU cmds, a valid GPU context must be setted into the stream.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer -> AddCommand ...
    //!           -> ReturnCommandBuffer -> SubmitCommandBuffer -> SetGpuContext ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           Current handle of gpu Context to set.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetGpuContext(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuContext);

    //! \brief    Sets the object capture flags for Linux OCA dump
    //! \details  Sets the object capture flags for Linux OCA dump
    //!
    //! \param    PMOS_RESOURCE osResource
    //!           [in] osResource
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if success else failure reason
    //!
    static MOS_STATUS SetObjectCapture(
        PMOS_RESOURCE osResource);

    //!
    //! \brief   Get GpuContext
    //! \details MOS internal toolset func to get GPU context instance
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           MOS GPU Context handle
    //!
    //! \return   GpuContextSpecificNext
    //!           GPU Context instance got by GPU context handle, nullptr if get failed
    //!
    static GpuContextSpecificNext *GetGpuContext(MOS_STREAM_HANDLE streamState, GPU_CONTEXT_HANDLE handle);

    //!
    //! \brief    Add Command
    //! \details  [Cmd Buffer Interface] Add gpu commands into cmd buffer
    //! \details  Caller: MHW only
    //! \details  It is not device stated function and can be used in both APO MHW and NON-APO MOS.
    //! \details  This func is called when a stream (Hal instance) adds gpu cmds into cmd buffer.
    //! \details  Before getting a cmd buffer to program GPU cmds, a valid GPU context must be setted into the stream.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer -> AddCommand ...
    //!           -> ReturnCommandBuffer -> SubmitCommandBuffer -> SetGpuContext ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //!
    //! \param    [in] cmdBuffer
    //!           Handle of cmd buffer to add cmd. cmd buffer handle can be get by calling GetCommandBuffer.
    //! \param    [in] cmd
    //!           Pointer to the memory to indicate cmd, caller must make sure it's valid.
    //! \param    [in] cmdSize
    //!           Size of cmd to program.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS AddCommand(
        COMMAND_BUFFER_HANDLE cmdBuffer,
        const void *cmd,
        uint32_t   cmdSize);

#if MOS_COMMAND_BUFFER_DUMP_SUPPORTED
    //!
    //! \brief    Dump Indirect state in Command Buffer
    //!
    static MOS_STATUS DumpIndirectStates(
        MOS_STREAM_HANDLE streamState,
        const char        *filePathPrefix,
        std::time_t       currentTime);
    //!
    //! \brief    Dump Indirect state in Command Buffer
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] cmdBuffer
    //!           Handle of cmd buffer to add cmd. cmd buffer handle can be get by calling GetCommandBuffer.
    //! \param    [in] gpuNode
    //!           Gpu node.
    //! \param    [in] filePathPrefix
    //!           The prefix for indirect state dump file.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DumpBindingTable(
        MOS_STREAM_HANDLE     streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer,
        MOS_GPU_NODE          gpuNode,
        const char            *filePathPrefix);

    //!
    //! \brief    Dump Command Buffer
    //! \details  [Cmd Buffer Interface] Dump an existing cmd buffer
    //! \details  Caller: HAL only
    //! \details  This func is called when a stream (Hal instance) needs to dump cmd buffer.
    //! \details  Only after ReturnCommandBuffer can Command Buffer being dumped
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] cmdBuffer
    //!           Handle of cmd buffer to add cmd. cmd buffer handle can be get by calling GetCommandBuffer.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DumpCommandBuffer(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer);

    static MOS_STATUS DumpSyncCommandBuffer(
        OsContextNext          *osContext,
        GpuContextSpecificNext *gpuContext);
#endif  // MOS_COMMAND_BUFFER_DUMP_SUPPORTED

    //!
    //! \brief    Get Command Buffer
    //! \details  [Cmd Buffer Interface] Get current cmd buffer to program based on streamState
    //! \details  Caller: HAL only
    //! \details  This func is called when a stream (Hal instance) needs to get a cmd buffer corresponding to current GPU context in os stream state.
    //! \details  Before getting a cmd buffer to program GPU cmds, a valid GPU context must be setted into the stream.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer -> AddCommand ...
    //!           -> ReturnCommandBuffer -> SubmitCommandBuffer -> SetGpuContext ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] cmdBuffer
    //!           Handle of cmd buffer to get. If get failed, it is INVALID_HANLE.
    //! \param    [in] pipeIdx
    //!           Pipe index to indicate which pipe's cmdbuffer to get.
    //!           In frame split, pipe index is indicated to get secondary cmd buffers. They are cmd buffers split to different engines.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetCommandBuffer(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE &cmdBuffer,
        uint32_t pipeIdx = 0);

    //!
    //! \brief    Return Command Buffer
    //! \details  [Cmd Buffer Interface] Return current cmd buffer to the MOS
    //! \details  Caller: HAL only
    //! \details  This func is called when a stream (Hal instance) finished add cmds into a cmd buffer.
    //! \details  ReturnCommandBuffer must be called before submit cmd buffer. MOS will do necessary operations in this interface.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer -> AddCommand ...
    //!           -> ReturnCommandBuffer -> SubmitCommandBuffer -> SetGpuContext ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] cmdBuffer
    //!           Handle of cmd buffer to return.
    //! \param    [in] pipeIdx
    //!           Pipe index to indicate which pipe's cmdbuffer to get.
    //!           In frame split, pipe index is indicated to get secondary cmd buffers. They are cmd buffers split to different engines.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ReturnCommandBuffer(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer,
        uint32_t pipeIdx = 0);

    //!
    //! \brief    Submit Command Buffer
    //! \details  [Cmd Buffer Interface] Submit current cmd buffer to current GPU context queue.
    //! \details  Caller: HAL only
    //! \details  When a stream (Hal instance) call this interface, cmd buffer is enqueued into current GPU context in streamState.
    //! \details  OS runtime and KMD will schedule the workload. HW cmds in the cmd buffer will be executed in HW engines.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer -> AddCommand ...
    //!           -> ReturnCommandBuffer -> SubmitCommandBuffer -> SetGpuContext ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //! \details  Cmd buffer execution in GPU context is async with Hal programming. Return of this interface does not guarantee finish executing actual cmds.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] cmdBuffer
    //!           Handle of cmd buffer to Submit. 
    //!           If there is frame split case (more than 1 pipe) in current GPU context queue, this is primary cmd buffer handle.
    //! \param    [in] nullRendering
    //!           Flag to indicate if not actually submit workload into HW.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SubmitCommandBuffer(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer,
        bool nullRendering = false);

    //!
    //! \brief    Reset Command Buffer
    //! \details  [Cmd Buffer Interface] Reset cmd buffer to the initialized state.
    //! \details  Caller: HAL only
    //! \details  ResetCommandBuffer can be called after a stream (Hal instance) call GetCommandBuffer.
    //! \details  OS runtime and KMD will schedule the workload. HW cmds in the cmd buffer will be executed in HW engines.
    //! \details  Calling sequence is like: SetGpuContext -> GetCommandBuffer (-> ResetCommandBuffer) -> AddCommand ...
    //! \details  If Current GPU context is never set, all command buffer / resource interfaces cannot be used. 
    //!           (They will return MOS_STATUS_INVALID_GPU_CONTEXT)
    //! \details  Cmd buffer reset means stream starts to program a new set of cmds into a cmd buffer got.
    //!           This interface must not be called when the cmd buffer already programed cmds and not submitted unless the stream needs to drop these cmds.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] cmdBuffer
    //!           Handle of cmd buffer to reset. 
    //!           If there is frame split case (more than 1 pipe) in current GPU context queue, this is primary cmd buffer handle.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ResetCommandBuffer(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer);

    //!
    //! \brief    Verify Command Buffer Size
    //! \details  [Cmd Buffer Interface] Check if cmd buffer size is larger than the requested size
    //! \details  Caller: HAL only
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] cmdBuffer
    //!           Handle of cmd buffer to verify size
    //! \param    [in] requestedSize
    //!           Requested size
    //! \param    [in] pipeIdx
    //!           Pipe index to indicate which pipe's cmdbuffer to verify.
    //!           In frame split, pipe index is indicated to get secondary cmd buffers. They are cmd buffers split to different engines.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, MOS_STATUS_UNKNOWN if size does not meet the requirment, otherwise failed
    //!
    static MOS_STATUS VerifyCommandBufferSize(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer,
        uint32_t requestedSize,
        uint32_t pipeIdx = 0);

    //!
    //! \brief    Resize Command Buffer and Patch List
    //! \details  [Cmd Buffer Interface] Resize the cmd buffer to contain more cmds. Resize the patch list to have more resource.s
    //! \details  Caller: HAL only
    //! \details  ResizeCommandBuffer can be called at any time if providing valid a cmd buffer.
    //!           When cmds number to be added is increased, this interface needs to be called.
    //!           MOS will make sure the existing cmds copied to the resized cmd buffer.
    //!           Patch list contain the entries to patch cmds. When cmds number to be added is increased, this interface needs to be called.
    //! \details  Recommand to call this interface only once for a specific cmd buffer with a conservative requestedSize.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] cmdBuffer
    //!           Handle of cmd buffer to resize.
    //! \param    [in] requestedSize
    //!           Requested size. If the size already larger than the requirement, no operations is done to cmd buffer.
    //! \param    [in] requestedPatchListSize
    //!           Requested patch list size. If the size already larger than the requirement, no operations is done to cmd buffer.
    //! \param    [in] pipeIdx
    //!           Pipe index to indicate which pipe's cmdbuffer to resize.
    //!           In frame split, pipe index is indicated to get secondary cmd buffers. They are cmd buffers split to different engines.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ResizeCommandBufferAndPatchList(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer,
        uint32_t requestedSize,
        uint32_t requestedPatchListSize,
        uint32_t pipeIdx = 0);

    //!
    //! \brief    Set Patch Entry
    //! \details  [Cmd Buffer Interface] Set a patch entry in cmd buffer.
    //! \details  Caller: MHW only
    //! \details  This interface is called only when adding a resource into a cmd.
    //!           The entries in cmd buffer indicate the gfx address to be patched.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] params
    //!           Pointer to the patch entry parameters.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetPatchEntry(
        MOS_STREAM_HANDLE streamState,
        PMOS_PATCH_ENTRY_PARAMS params);

    //!
    //! \brief    Get Indirect State
    //! \details  [Cmd Buffer Interface] Get the indirect state in cmd buffer.
    //! \details  Caller: MHW only
    //! \details  This interface is called when preparing indirect state data in cmd buffer.
    //!           Indirect state is a reserved region in cmd buffer which contains the data needed by execute Media kernel.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] indirectState
    //!           Pointer to pointer to indirectState data. MHW can use this ptr to set data.
    //! \param    [out] offset
    //!           Offset of indirect state in the cmd buffer.
    //! \param    [out] size
    //!           Size of indirect state in the cmd buffer.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetIndirectState(
        MOS_STREAM_HANDLE streamState,
        uint8_t **indirectState,
        uint32_t &offset,
        uint32_t &size);

    //!
    //! \brief    Setup indirect state
    //! \details  [Cmd Buffer Interface] Setup the indirect state region in cmd buffer.
    //! \details  Caller: MHW only
    //! \details  This interface is called to reserve the region of indirect state data in cmd buffer.
    //! \details  Indirect state is a reserved region in cmd buffer which contains the data needed by execute Media kernel.
    //!           The region is at the end of cmd buffer, size is only needed. Between each SubmitCommandBuffer, this interface should only be call once.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] size
    //!           Size of indirect state in the cmd buffer.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetupIndirectState(
        MOS_STREAM_HANDLE streamState,
        uint32_t size);

    //!
    //! \brief    Setup commandlist and command pool
    //! \details  Set the commandlist and commandPool used in this stream.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] cmdList
    //!           pointer to the command list.
    //! \param    [in] cmdBufMgr
    //!           pointer to the command buffer manager.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetupCurrentCmdListAndPool(
        MOS_STREAM_HANDLE   streamState,
        CommandList         *cmdList,
        CmdBufMgrNext       *cmdBufMgr);

    //!
    //! \brief    Setup commandlist and command pool from os interface
    //! \details  Set the commandlist and commandPool used in this stream from os interface.
    //!
    //! \param    [in] pMosInterface
    //!           pointer to the mos interface
    //! \param    [out] streamStateDst
    //!           Handle of Os Stream State.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetupCurrentCmdListAndPoolFromOsInterface(
        PMOS_INTERFACE      pMosInterface,
        MOS_STREAM_HANDLE   streamState);

    //!
    //! \brief    Is Device Async or not
    //! \details  Is Device Async or not.
    //!
    //! \param    [in] streamStateDst
    //!           Handle of Os Stream State.
    //!
    //! \return   bool
    //!           Return true if is async, otherwise false
    //!
    static bool IsAsyncDevice(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Setup VE Attribute Buffer
    //! \details  [Cmd Buffer Interface] Setup VE Attribute Buffer into cmd buffer.
    //! \details  Caller: MHW only
    //! \details  This interface is called to setup into cmd buffer.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] cmdBuffer
    //!           Cmd buffer to setup VE attribute.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetupAttributeVeBuffer(
        MOS_STREAM_HANDLE     streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer);
        
    //!
    //! \brief    Get VE Attribute Buffer
    //! \details  [Cmd Buffer Interface] Get VE Attribute Buffer from cmd buffer.
    //! \details  Caller: HAL only
    //! \details  This interface is called to get VE attribute buffer from cmd buffer if it contains one.
    //!           If there is no VE attribute buffer returned, it means the cmd buffer has no such buffer
    //!           in current MOS module. It is not error state if it is nullptr.
    //!
    //! \param    [out] cmdBuffer
    //!           Cmd buffer to setup VE attribute.
    //!
    //! \return   MOS_CMD_BUF_ATTRI_VE*
    //!           Return pointer of VE attribute buffer, nullptr if current cmdBuffer didn't contain attribute.
    //!
    static MOS_CMD_BUF_ATTRI_VE *GetAttributeVeBuffer(
        COMMAND_BUFFER_HANDLE cmdBuffer);

    //!
    //! \brief    Get Cache Policy Memory Object
    //! \details  [Resource Interface] Get Cache Policy Memory Object in GMM corresponding to the resource usage
    //!           Caller: HAL & MHW
    //!
    //! \param    [in] mosUsage
    //!           Resource usage as index to the memory object table
    //!           If prociding unknown usage, default state will be returned
    //!
    //! \return   MEMORY_OBJECT_CONTROL_STATE
    //!           The cache policy memory object got from MOS interface
    //!
    static GMM_RESOURCE_USAGE_TYPE GetGmmResourceUsageType(
        MOS_HW_RESOURCE_DEF mosUsage);

    //!
    //! \brief    Get MOS_HW_RESOURCE_DEF
    //! \details  [Resource Interface] Get Mos HW Resource DEF
    //!           Caller: HAL & MHW
    //!
    //! \param    [in] gmmResUsage
    //!           Gmm Resource usage as index
    //!
    //! \return   MOS_HW_RESOURCE_DEF
    //!           Mos HW resource definition
    //!
    static MOS_HW_RESOURCE_DEF GmmToMosResourceUsageType(
        GMM_RESOURCE_USAGE_TYPE gmmResUsage);

    //!
    //! \brief    Get Cache Policy Memory Object
    //! \details  [Resource Interface] Get Cache Policy Memory Object in GMM corresponding to the resource usage
    //!           Caller: HAL & MHW
    //!
    //! \param    [in] gmmClientContext
    //!           Handle of gmmClientContext
    //! \param    [in] mosUsage
    //!           Resource usage as index to the memory object table
    //!           If prociding unknown usage, default state will be returned 
    //!
    //! \return   MEMORY_OBJECT_CONTROL_STATE
    //!           The cache policy memory object got from MOS interface
    //!     
    static MEMORY_OBJECT_CONTROL_STATE GetCachePolicyMemoryObject(
        GMM_CLIENT_CONTEXT *gmmClientContext,
        MOS_HW_RESOURCE_DEF mosUsage);

    //!
    //! \brief    Get default Cache Policy Memory Object
    //! \details  [Resource Interface] Get Cache Policy Memory Object in GMM corresponding to the resource usage
    //!           Caller: HAL & MHW
    //!
    //! \param    [in] gmmClientContext
    //!           Handle of gmmClientContext
    //!           If prociding unknown usage, default state will be returned
    //!
    //! \return   MEMORY_OBJECT_CONTROL_STATE
    //!           The cache policy memory object got from MOS interface
    //!
    static MEMORY_OBJECT_CONTROL_STATE GetDefaultCachePolicyMemoryObject(
        GMM_CLIENT_CONTEXT *gmmClientContext);

    //!
    //! \brief    Get Cache Policy Memory Object
    //! \details  [Resource Interface] Get Cache Policy Memory Object in GMM corresponding to the resource usage
    //!           Caller: MOS
    //!
    //! \param    [in] gmmClientContext
    //!           Handle of gmmClientContext
    //! \param    [in] gmmUsage
    //!           Resource usage value defined in gmm
    //!           If prociding unknown usage, default state will be returned
    //!
    //! \return   MEMORY_OBJECT_CONTROL_STATE
    //!           The cache policy memory object got from MOS interface
    //!
    static MEMORY_OBJECT_CONTROL_STATE GetGmmCachePolicyMemoryObject(
        GMM_CLIENT_CONTEXT      *gmmClientContext,
        GMM_RESOURCE_USAGE_TYPE gmmUsage);

    //!
    //! \brief    Get Cache Policy L1 Config
    //! \details  [Resource Interface] Get L1 Cache Config in GMM corresponding to the resource usage
    //!           Caller: HAL & MHW
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] mosUsage
    //!           Resource usage as index to the memory object table
    //!           If prociding unknown usage, default state will be returned
    //!
    //! \return   uint8_t
    //!           The L1_Cache_Config got from MOS interface
    //! 
    static uint8_t GetCachePolicyL1Config(
        MOS_STREAM_HANDLE streamState,
        MOS_HW_RESOURCE_DEF mosUsage);

    //!
    //! \brief    Get Reserved info from resource
    //! \details
    //!
    //! \param    [in] resource
    //!           Handle of resource
    //! \param    [out] val
    //!           result of info.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetReservedFromResource(MOS_RESOURCE_HANDLE resource, uint32_t &val);

    //!
    //! \brief    Get Reserved info from Stream
    //! \details
    //!
    //! \param    [in] stream
    //!           Handle of stream
    //! \param    [out] val
    //!           result of info.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetReservedFromStream(MOS_STREAM_HANDLE stream, uint32_t &val);

    //!
    //! \brief    Get Reserved info from Device
    //! \details
    //!
    //! \param    [in] osDeivceContext
    //!           Handle of device
    //! \param    [out] val
    //!           result of info.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetReservedFromDevice(MOS_DEVICE_HANDLE device, uint32_t &val);

    //!
    //! \brief    Get preStreamParameters(mos context) info from streamState
    //! \details
    //!
    //! \param    [in] stream
    //!           Handle of stream
    //! \param    [out] perStreamParameters
    //!           pointer of mos conxtex.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetperStreamParameters(MOS_STREAM_HANDLE stream, void **perStreamParameters);

    //!
    //! \brief    Convert Resource From Ddi
    //! \details  [Resource Interface] Convert Resource structure From OS/API specific to MOS reource.
    //! \details  Caller: DDI only
    //! \details  It is not device stated function and can be used in both APO DDI and NON-APO MOS.
    //! \details  MOS resoure is the structure inside MOS module. DDI specific resource depends on OS/API verison.
    //!           DDI call this to convert external resources (not created by hal) to Mos resources so that HAL & MHW can use them.
    //!
    //! \param    [in] osResource
    //!           OS/API specific resource structure to convert.
    //! \param    [out] resource
    //!           Handle of Mos resource convert.
    //! \param    UINT firstArraySlice
    //!           [in] resource special info
    //! \param    UINT mipSlice
    //!           [in] resource special info
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ConvertResourceFromDdi(
        OsSpecificRes osResource,
        MOS_RESOURCE_HANDLE &resource,
        uint32_t firstArraySlice,
        uint32_t mipSlice);
    
    //!
    //! \brief    Create Os Specific Resource Info
    //! \details  [Resource Interface] Create OS/API specific resource info structures.
    //! \details  Caller: DDI only
    //! \details  Os Specific resource info must be created before uing in DDI or converting to MOS resource.
    //!           This interface doesn't allocate Os Specific Resource. It only create the decorated structure of that resource.
    //!           
    //! \param    [in, out] resource
    //!           OS/API specific resource structure to initialize.
    //! \param    [in] isInternal
    //!           Indicate if the resource is media internal.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CreateOsSpecificResourceInfo(OsSpecificRes resource, bool isInternal = false);

    //!
    //! \brief    Destroy Os Specific Resource Info
    //! \details  [Resource Interface] Destroy OS/API specific resource structure.
    //! \details  Caller: DDI only
    //! \details  It is not device stated function and can be used in both APO DDI and NON-APO MOS.
    //! \details  Os Specific resource info must be destroied if the resource is not used anymore.
    //!
    //! \param    [in, out] resource
    //!           OS/API specific resource structure to initialize.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DestroySpecificResourceInfo(OsSpecificRes resource);

    //!
    //! \brief    Allocate Resource
    //! \details  [Resource Interface] Allocate a graphic resource.
    //! \details  Caller: HAL only
    //! \details  Graphic resource is a buffer contain data used in the HW cmds.
    //!           This interface allocates the gfx resource and its internal data structure.
    //!           RegisterResource must be called when cmds in cmd buffer programmed are using this resource. 
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] params
    //!           Pointer to the parameters for allocating resource
    //! \param    [out] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS AllocateResource(
        MOS_STREAM_HANDLE           streamState,
        PMOS_ALLOC_GFXRES_PARAMS    params,    // user provided va
        MOS_RESOURCE_HANDLE         &resource
#if MOS_MESSAGES_ENABLED
        ,
        const char                  *functionName,
        const char                  *filename,
        int32_t                     line
#endif
    );

    //!
    //! \brief    Convert HAL free flags to OS free flags
    //!
    //! \param    [in] halFreeFlag
    //!           bit definition in MOS_GFXRES_FREE_FLAGS
    //!
    //! \return   uint32_t
    //!           OS resource deallc flags
    //!
    static uint32_t ConvertHalFreeFlagsToOsFreeFlags(
        uint32_t halFreeFlag
    );

    //!
    //! \brief    Free Resource
    //! \details  [Resource Interface] Free a graphic resource.
    //! \details  Caller: HAL only
    //! \details  Graphic resource is a buffer contain data used in the HW cmds.
    //!           This interface frees the gfx resource and its internal data structure.
    //!           This interface must be called when the resource is not used anymore. 
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //! \param    [in] flag
    //!           User defined free flag of the resource.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS FreeResource(
        MOS_STREAM_HANDLE       streamState,
        MOS_RESOURCE_HANDLE     resource,
        uint32_t                flag
#if MOS_MESSAGES_ENABLED
        ,
        const char              *functionName,
        const char              *filename,
        int32_t                 line
#endif  // MOS_MESSAGES_ENABLED
    );

    static MOS_STATUS FreeResource(
        OsDeviceContext       *osDeviceContext,
        MOS_RESOURCE_HANDLE    resource,
        uint32_t               flag
#if MOS_MESSAGES_ENABLED
        ,
        const char            *functionName,
        const char            *filename,
        int32_t                line
#endif  // MOS_MESSAGES_ENABLED
    );

    //!
    //! \brief    Get Resource Info
    //! \details  [Resource Interface] Get the info of a graphic resource.
    //! \details  Caller: HAL only
    //! \details  This interface gets the read-only detailed info of a graphic resource. 
    //!           Any modification of details provided by this interface will not impact the actual resource.
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //! \param    [out] details
    //!           Resource detailed info got.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetResourceInfo(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        MosResourceInfo &details);

    //!
    //! \brief    Lock Resource
    //! \details  [Resource Interface] Lock the gfx resource for CPU to access
    //! \details  Caller: HAL only
    //! \details  A sys memory ptr will be provided by this interface if executed successfully.
    //! \details  The sys memory is mapped to the gfx memory inside MOS module.
    //! \details  This interface is usually for driver to read/write data into a resource directly (without program HW cmd).
    //! \details  This interface will call the overloading LockMosResource for MOS.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the resource to lock.
    //! \param    [in] flags
    //!           Control flags of locking resource.
    //!
    //! \return   void *
    //!           Locked memory data pointer, nullptr if lock failed.
    //!
    static void *LockMosResource(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE resource,
        PMOS_LOCK_PARAMS    flags);
    //!
    //! \brief    Lock Resource
    //! \details  [Resource Interface] Lock the gfx resource for CPU to access
    //! \details  Caller: MOS only
    //! \details  A sys memory ptr will be provided by this interface if executed successfully.
    //! \details  The sys memory is mapped to the gfx memory inside MOS module.
    //! \details  This interface is usually for driver to read/write data into a resource directly (without program HW cmd).
    //! \details  Caller must make sure no access out of bound of the locked out data. UnlockResource must be called when finished access the locked data.
    //!           A resource already been locked cannot be locked again.
    //!           This is a blocking call if the resource is used by the cmdbuffer which already submitted to an existing GPU context.
    //!           Unless SkipResourceSync is called. This interface will make sure the sync of Lock.
    //! \details  If the resource is compressed, gfx memory decompression will be triggered.
    //!
    //! \param    [in] OsDeviceContext
    //!           Os Device Context
    //! \param    [in] resource
    //!           MOS Resource handle of the resource to lock.
    //! \param    [in] flags
    //!           Control flags of locking resource.
    //!
    //! \return   void *
    //!           Locked memory data pointer, nullptr if lock failed.
    //!
    static void *LockMosResource(
        OsDeviceContext       *osDeviceContext,
        MOS_RESOURCE_HANDLE    resource,
        PMOS_LOCK_PARAMS       flags,
        bool                   isDumpPacket=0);

    //!
    //! \brief    Unlock Resource
    //! \details  [Resource Interface] Unlock the gfx resource which is locked out.
    //! \details  Caller: HAL only
    //! \details  UnlockResource must be called when finished access the locked data of the resource.
    //!           A resource already been unlocked cannot be unlocked again. 
    //! \details  Unlock resource will not trigger compressing or changing the layout of the resource.
    //! \details  This interface will call the overloading UnlockMosResource for MOS.
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS UnlockMosResource(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource);
    //!
    //! \brief    Unlock Resource
    //! \details  [Resource Interface] Unlock the gfx resource which is locked out.
    //! \details  Caller: MOS only
    //! \details  UnlockResource must be called when finished access the locked data of the resource.
    //!           A resource already been unlocked cannot be unlocked again.
    //! \details  Unlock resource will not trigger compressing or changing the layout of the resource.
    //!
    //! \param    [in] OsDeviceContext
    //!           Os Device Context
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS UnlockMosResource(
        OsDeviceContext    *osDeviceContext,
        MOS_RESOURCE_HANDLE    resource);
    //!
    //! \brief    Update resource usage type
    //! \details  update the resource usage for cache policy
    //! \param    PMOS_RESOURCE pOsResource
    //!           [in/out] Pointer to OS Resource
    //! \param    MOS_HW_RESOURCE_DEF resUsageType
    //!           [in] MOS resosuce usage type
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS UpdateResourceUsageType(
        PMOS_RESOURCE           pOsResource,
        MOS_HW_RESOURCE_DEF     resUsageType);

    //!
    //! \brief    Register Resource
    //! \details  [Resource Interface] Register the resource to current streamState.
    //! \details  Caller: MHW only
    //! \details  Register resource to inform MOS that the resource is read/written by current cmd buffer being programmed
    //!           and this cmd buffer will be submitted into current GPU context in streamState.
    //! \details  RegisterResource must be called when cmds in cmd buffer programmed are using this resource. 
    //! \details  This interface is to make the residency of the resource and handle resource sync harzad between GPU contexts.
    //! \details  Calling sequence is like:  SetGpuContext -> RegisterResource... -> SubmitCommandBuffer ->
    //!           SetGpuContext(another) -> RegisterResource(another or same resource)... -> SubmitCommandBuffer
    //! \details  If Register same resource to different GPU context when calling SetGpuContext, sync harzad will be handled.
    //!           RegisterResource for the same resource can be called repeatedly. MOS will make sure no duplicated residency making and sync.
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] resource
    //!           MOS Resource handle of the allocated resource.
    //! \param    [in] write
    //!           Indicate if the resource is written by HW or just read.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS RegisterResource(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        bool write);
        
    //!
    //! \brief    Get Resource Gfx Address
    //! \details  [Resource Interface] Get the graphic virtual address of the resource.
    //! \details  Caller: MHW only
    //! \details  Only use this interface to add resource's address directly into cmd field. 
    //!           If MHW needs patch the address in the cmd field, GetResourceAllocationIndex should be called.
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   uint64_t
    //!           64bit virtual graphic address got. 0x00000000 if execution failed.
    //!
    static uint64_t GetResourceGfxAddress(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource);

    //!
    //! \brief    Get Resource Allocation Handle
    //! \details  [Resource Interface] Get the allocation handle of a graphic resource.
    //! \details  Caller: MHW Only
    //! \details  This interface gets the read-only detailed info of a graphic resource.
    //!           Any modification of details provided by this interface will not impact the actual resource.
    //!
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   uint32_t
    //!           Allocation Handle. 0 if execution failed.
    //!
    static uint32_t GetResourceAllocationHandle(
        MOS_RESOURCE_HANDLE resource);

    //!
    //! \brief    Get Resource Allocation index
    //! \details  [Resource Interface] Get the allocation index of the resource.
    //! \details  Caller: MHW only
    //! \details  Allocation index is used when calling SetPatchEntry to add resource into cmd.
    //!           If MHW needs patch the address in the cmd field, GetResourceAllocationIndex should be called.
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   uint32_t
    //!           Allocation index got. 0 if execution failed.
    //!
    static uint32_t GetResourceAllocationIndex(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource);
    
    //!
    //! \brief    Skip Resource Sync
    //! \details  [Resource Interface] Skip the sync handling of the resource 
    //! \details  Caller: HAL only
    //! \details  It is not device stated function and can be used in both APO HAL and NON-APO MOS.
    //! \details  Indicate the resource provided needn't to be synced. 
    //!           The resource skipping sync can be accessed by different cmd buffers on different GPU contexts at the same time.
    //! \details  RegisterResource and LockResource will not handling the sync of the resources between different GPU cotnexts.
    //! \details  Usually the resource skipping sync is for the case like: 
    //!           Different cmd buffers at the same time access the non-overlapped region of the resource
    //!           
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] params
    //!           Pointer to the parameters for allocating resource
    //! \param    [out] resource
    //!           MOS Resource handle of the allocated resource.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SkipResourceSync(
        MOS_RESOURCE_HANDLE resource);

    //!
    //! \brief    Sync on resource
    //! \details  [Resource Interface] Explicit sync on resource
    //! \details  Caller: HAL only
    //! \details  Resource is shared by different cmd buffers on different GPU contexts.
    //!           Adding sync object into requestor GPU context queue to resolve the hazard if necessary.
    //!           This func is called by hal to declare the resource to consider the sync explicitly.
    //!           It is a strong sync request for the resource.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle for the resource contain hazard of sync
    //! \param    [in] writeOperation
    //!           Indicate the current programming is to write resource or not
    //! \param    [in] requsetorGpuContext
    //!           GpuContext which programming the resource. Recommand not setting it and use current GPU context.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SyncOnResource(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        bool writeOperation,
        GPU_CONTEXT_HANDLE requsetorGpuContext = MOS_GPU_CONTEXT_INVALID_HANDLE);
        
    //!
    //! \brief    Resource Sync call back between Media and 3D for resource Sync
    //! \details  [Resource Interface] Sync Call Back based on resource
    //! \details  Caller: DDI only
    //! \details  Resource is shared by different cmd buffers on different GPU contexts.
    //!           Adding sync object into requestor GPU context queue to resolve the hazard if necessary.
    //!           If there is a hazard, one cmd buffer in requestor GPU context queue will wait for the other cmd buffer in busy GPU context.
    //!           
    //! \param    [in] resource
    //!           OS specific resource handle for the resource contain hazard of sync
    //! \param    [in] deviceContext
    //!           Handle of Os Device Context
    //! \param    [in] index
    //!           Sub-resource index
    //! \param    [in] hazardType
    //!           Type of hazard: RAW, WAR, WAR
    //! \param    [in] busyCtx
    //!           GPU Context handle of the queue being waiting for.
    //! \param    [in] requestorCtx
    //!           GPU Context handle of current GPU which requesting to use the resoure and find the hazard to wait the busy context.
    //! \param    [in] osRequestorHandle
    //!           OS runtime handle of requestor context
    //! \param    [in,out] fenceInfoTrinity
    //!           if need to sync, it is fence handle and fence value
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ResourceSyncCallback(
        OsSpecificRes          resource,
        MOS_DEVICE_HANDLE      deviceContext,
        uint32_t               index,
        SYNC_HAZARD            hazardType,
        GPU_CONTEXT_HANDLE     busyCtx,
        GPU_CONTEXT_HANDLE     requestorCtx,
        OS_HANDLE              osRequestorHandle,
        SYNC_FENCE_INFO_TRINITY *fenceInfoTrinity);

    //!
    //! \brief    Lock Sync Callback between Media and 3D
    //! \details  [Resource Interface] Lock Sync Call Back
    //! \details  Caller: DDI only
    //! \details  Resource is used in a cmd buffer on an existing GPU context.
    //!           Before Locking the resource, make sure the resource finished used by all GPU contexts which are using this resource.
    //!           If there is a hazard, CPU side will wait for the cmd buffer in busy GPU context.
    //!           
    //! \param    [in] resource
    //!           OS specific resource handle for the resource contain hazard of sync
    //! \param    [in] deviceContext
    //!           Handle of Os Device Context
    //! \param    [in] index
    //!           Sub-resource index
    //! \param    [in] hazardType
    //!           Type of hazard: RAW, WAR, WAR
    //! \param    [in] busyCtx
    //!           GPU Context handle of the queue being waiting for.
    //! \param    [in] doNotWait
    //!           Indicate this is blocking call or not. When set to true, possibly return MOS_STATUS_STILL_DRAWING
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, MOS_STATUS_STILL_DRAWING if doNotWait
    //!           is set to true and resoure is still being used in HW, otherwise failed
    //!        
    static MOS_STATUS LockSyncCallback(    
        OsSpecificRes           resource,
        MOS_DEVICE_HANDLE       deviceContext,
        uint32_t                index,
        SYNC_HAZARD             hazardType,
        GPU_CONTEXT_HANDLE      busyCtx,
        bool                    doNotWait);

    //!
    //! \brief    Wait For cmd Completion
    //! \details  [GPU Context Interface] Waiting for the completion of cmd in provided GPU context
    //! \details  Caller: HAL only
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuCtx
    //!           GpuContext handle of the gpu context to wait cmd completion
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS WaitForCmdCompletion(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuCtx);
    
    //!
    //! \brief    Trim Residency
    //!
    //! \param    [in] device
    //!           MOS device handle
    //! \param    [in] periodicTrim
    //!           Indicate if the trim is periodic
    //! \param    [in] restartPeriodicTrim
    //!           Indicate if restarting periodic trim
    //! \param    [in] numBytesToTrim
    //!           Number bytes to trim
    //! \param    [in] trimToMinimum
    //!           Indicate if trim to minimum
    //! \param    [in] trimOnlyMediaResources
    //!           Indicate if only trim media resources.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!        
    static MOS_STATUS TrimResidency(
        MOS_DEVICE_HANDLE device,
        bool periodicTrim, 
        bool restartPeriodicTrim, 
        uint64_t &numBytesToTrim, 
        bool trimToMinimum,
        bool trimOnlyMediaResources);

    //!
    //! \brief    Update Residency
    //!
    //! \param    [in] device
    //!           MOS device handle
    //! \param    [in] resInfo
    //!           Os specific resource info
    //! \param    [in] index
    //!           Resource index
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!  
    static MOS_STATUS UpdateResidency(
        MOS_DEVICE_HANDLE device,
        OsSpecificRes     resInfo,
        uint32_t          index,
        bool              bypassAuxTableUpdate = false);
    

    // Memory compression interfaces

    //!
    //! \brief    Decompress resource
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the resource to decompress.
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DecompResource(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource);

    //!
    //! \brief    Decompress resource
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] mosDecompression
    //!           MosDecompression in stramStatate or in osDeviceContext.
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetMosDecompressionFromStreamState(
        MOS_STREAM_HANDLE streamState,
        MosDecompression* & mosDecompression);

    //!
    //! \brief    Set auxiliary resource to sync with decompression
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle of the resource.
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetDecompSyncRes(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE syncResource);

    //!
    //! \brief  Set Memory Compression Mode
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] resource
    //!           MOS Resource handle
    //! \param    [in] resMmcMode
    //!           MMC mode
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetMemoryCompressionMode(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        MOS_MEMCOMP_STATE resMmcMode);
    
    //!
    //! \brief  Get Memory Compression Mode
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] resource
    //!           MOS Resource handle
    //! \param    [out] resMmcMode
    //!           MMC mode
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetMemoryCompressionMode(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        MOS_MEMCOMP_STATE &resMmcMode); 
    
    //!
    //! \brief  Set Memory Compression Hint
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] resource
    //!           MOS Resource handle
    //! \param    [in] hintOn
    //!           Flag to set hint on or off
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS SetMemoryCompressionHint(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        bool                hintOn);

    //!
    //! \brief  Get Memory Compression Format
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in, out] resource
    //!           MOS Resource handle
    //! \param    [out] resMmcFormat
    //!           MMC format got
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS GetMemoryCompressionFormat(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE resource,
        uint32_t *resMmcFormat);

    //!
    //! \brief    Double buffer copy resource
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] inputResource
    //!           Input resource to copy.
    //! \param    [out] outputResource
    //!           Output resource.
    //! \param    [in] outputCompressed
    //!           Insdicate if output resource is compressed.
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DoubleBufferCopyResource(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE inputResource,
        MOS_RESOURCE_HANDLE outputResource,
        bool                outputCompressed);

    //!
    //! \brief    Use media copy to copy resource
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] inputResource
    //!           Source resource.
    //! \param    [out] outputResource
    //!           Destination resource.
    //! \param    [in] preferMethod
    //!           Preferred copy engine.
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS UnifiedMediaCopyResource(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE inputResource,
        MOS_RESOURCE_HANDLE outputResource,
        int                 preferMethod);

    //!
    //! \brief    Copy Resource to Another Buffer
    //! \details  Decompress and Copy Resource to Another 2D Buffer
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    inputResource
    //!           [in] Input Resource object
    //! \param    outputResource
    //!           [out] output Resource object
    //! \param    [in] copyPitch
    //!           The 2D surface pitch
    //! \param    [in] copyHeight
    //!           The 2D surface height
    //! \param    [in] copyInputOffset
    //!           The offset of copied surface from
    //! \param    [in] copyOutputOffset
    //!           The offset of copied to
    //! \param    [in] outputCompressed
    //!           True means apply compression on output surface, else output uncompressed surface
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if successful
    //!
    static MOS_STATUS MediaCopyResource2D(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE inputResource,
        MOS_RESOURCE_HANDLE outputResource,
        uint32_t            copyPitch,
        uint32_t            copyHeight,
        uint32_t            bpp,
        bool                outputCompressed);

    //!
    //! \brief    Copy Mono Resource to Another Buffer
    //! \details  Decompress and Copy Mono Resource to Another 2D Buffer
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    inputResource
    //!           [in] Input Resource object
    //! \param    outputResource
    //!           [out] output Resource object
    //! \param    [in] copyWidth
    //!           The 2D surface Width
    //! \param    [in] copyHeight
    //!           The 2D surface height
    //! \param    [in] copyInputOffset
    //!           The offset of copied surface from
    //! \param    [in] copyOutputOffset
    //!           The offset of copied to
    //! \param    [in] outputCompressed
    //!           True means apply compression on output surface, else output uncompressed surface
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if successful
    //!
    static MOS_STATUS MonoSurfaceCopy(
        MOS_STREAM_HANDLE   streamState,
        MOS_RESOURCE_HANDLE inputResource,
        MOS_RESOURCE_HANDLE outputResource,
        uint32_t            copyPitch,
        uint32_t            copyHeight,
        uint32_t            copyInputOffset,
        uint32_t            copyOutputOffset,
        bool                outputCompressed);

    //!
    //! \brief   Check whether the parameter of mos surface is valid for copy
    //!
    //! \param    [in] mosSurface
    //!           Pointer to MosSurface
    //!
    //! \return   bool
    //!           Whether the paramter of mosSurface is valid
    //!
    static MOS_STATUS VerifyMosSurface(
        PMOS_SURFACE mosSurface,
        bool        &bIsValid);

    // GPU Status interfaces
    //!
    //! \brief   Get Gpu Status Tag
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           MOS GPU Context handle
    //!
    //! \return   uint32_t
    //!           Tag got from GPU Context indicated, 0 if failed to get the tag
    //!
    static uint32_t GetGpuStatusTag(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuContext);

    //!
    //! \brief   Increment Gpu Status Tag
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           MOS GPU Context handle
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS IncrementGpuStatusTag(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuContext);

    //!
    //! \brief   Get Gpu Status Sync Tag
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] gpuContext
    //!           MOS GPU Context handle
    //!
    //! \return   uint64_t
    //!           HW tag got from GPU context, 0 if get failed
    //!
    static uint64_t GetGpuStatusSyncTag(
        MOS_STREAM_HANDLE streamState,
        GPU_CONTEXT_HANDLE gpuContext);
    
    //!
    //! \brief   Get Gpu Status Buffer Resource
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] resource
    //!           MOS resource handle of GPU status buffer got from current GPU context
    //! \param    [in] gpuContext
    //!           MOS GPU Context handle
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!

    static MOS_STATUS GetGpuStatusBufferResource(
        MOS_STREAM_HANDLE streamState,
        MOS_RESOURCE_HANDLE &resource,
        GPU_CONTEXT_HANDLE gpuContext);
    
    //!
    //! \brief   Get CP Interface
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //!
    //! \return   MosCpInterface
    //!           CP Interface got from stream State, nullptr if get failed
    //!
    static MosCpInterface *GetCpInterface(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Maps the specified executable module into the address space of
    //!           the calling process.
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] A handle to OS interface.  This can be nullptr which allows a caller to
    //!           always get library from specified library path (function will never check
    //!           driver store) which is useful if there's a constant static path of a library
    //! \param    const PCCHAR lpLibFileName
    //!           [in] String containing resource name to load.  Absolute path is given here
    //!           if pOsInterface is nullptr, else only lib path is given, and driver will check store for path
    //! \param    PHMODULE phModule
    //!           [out] Handle to library given back to the caller
    //! \return   MOS_STATUS
    //!           Returns one of the MOS_STATUS error codes if failed,
    //!           else MOS_STATUS_SUCCESS
    //!
    static MOS_STATUS MosLoadLibrary(
        MOS_STREAM_HANDLE           streamState,
        PCCHAR                      pFileName,
        PHMODULE                    phModule);

    //!
    //! \brief    Free the loaded dynamic-link library
    //! \details  Free the loaded dynamic-link library
    //! \param    [in] hLibModule
    //!           A handle to the loaded DLL module
    //! \return   int32_t
    //!           true if success else false
    //!
    static MOS_STATUS MosFreeLibrary(HMODULE hLibModule);

    //!
    //! \brief    Create Virtual Engine State
    //! \details  [Virtual Engine Interface] Create Virtual Engine State of provided streamState
    //! \details  Caller: Hal (Scalability) only
    //! \details  This func is called when a stream (Hal instance) need to create a VE state
    //! \details  into provided stream.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] veInitParms
    //!           Pointer of parameters to init ve staet
    //! \param    [out] veState
    //!           Reference of the handle of Virtual Engine State to created
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS CreateVirtualEngineState(
        MOS_STREAM_HANDLE streamState,
        PMOS_VIRTUALENGINE_INIT_PARAMS veInitParms,
        MOS_VE_HANDLE    &veState);

    //!
    //! \brief    Destroy Virtual Engine State
    //! \details  [Virtual Engine Interface] Destroy Virtual Engine State of provided streamState
    //! \details  Caller: Hal (Scalability) only
    //! \details  This func is called when a stream (Hal instance) need to destroy a VE state
    //! \details  into provided stream.
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] veState
    //!           Reference of the handle of Virtual Engine State to created
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DestroyVirtualEngineState(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Set hint parameters
    //!
    //! \details  [Virtual Engine Interface] Set hint parameters into Virtual Engine State in provided stream
    //! \details  Caller: Hal (Scalability) only
    //! \details  Set hint parameters for virtual engine state
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] veParams
    //!           pointer to VE parameter data structure to set
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    static MOS_STATUS SetVeHintParams(
        MOS_STREAM_HANDLE streamState,
        PMOS_VIRTUALENGINE_SET_PARAMS veParams);

    //!
    //! \brief    Get hint parameters
    //!
    //! \details  [Virtual Engine Interface] Get hint parameters from Virtual Engine State in provided stream
    //! \details  Caller: Hal (Scalability) only
    //! \details  Get hint parameters from virtual engine state
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] scalableMode
    //!           flag to indicate if scalability mode
    //! \param    [out] hintParams
    //!           pointer to VE hint parameter address
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    static MOS_STATUS GetVeHintParams(
        MOS_STREAM_HANDLE streamState,
        bool scalableMode,
        PMOS_VIRTUALENGINE_HINT_PARAMS *hintParams);

    //!
    //! \brief    Get Adapter BDF
    //! \details  [System info Interface] Get Adapter BDF
    //! \details  Caller: DDI & HAL
    //! \details  This func is called to differentiate the behavior according to Adapter BDF.
    //!
    //! \param    [in] mosCtx
    //!           Pointer of Mos context
    //! \param    [out] adapterBDF
    //!           Adapter BDF info
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    static MOS_STATUS GetAdapterBDF(PMOS_CONTEXT mosCtx, ADAPTER_BDF *adapterBDF);
    
    //!
    //! \brief    Set Hybrid Cmd To GpuContext
    //! \details  Set Hybrid Cmd To GpuContext
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] ptr to pOsInterface
    //! \param    uint64_t gpuCtxOnHybridCmd
    //!           gpuCtxOnHybridCmd
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS
    //!
    static MOS_STATUS SetHybridCmdMgrToGpuContext(
        PMOS_INTERFACE pOsInterface,
        uint64_t       gpuCtxOnHybridCmd);

    //!
    //! \brief    Set Hybrid Cmd Submit Mode
    //! \details  Set Hybrid Cmd Submit Mode
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] ptr to pOsInterface
    //! \param    uint64_t hybridMgrSubmitMode
    //!           hybridMgrSubmitMode
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS
    //!
    static MOS_STATUS SetHybridCmdMgrSubmitMode(
        PMOS_INTERFACE pOsInterface,
        uint64_t       hybridMgrSubmitMode);

    //!
    //! \brief    Start the Cmd Consumer
    //! \details  Start the Cmd Consumer
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] ptr to pOsInterface
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS
    //!
    static MOS_STATUS StartHybridCmdMgr(
        PMOS_INTERFACE pOsInterface);

    //!
    //! \brief    Stop the Cmd Consumer
    //! \details  Stop the Cmd Consumer
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] ptr to pOsInterface
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS
    //!
    static MOS_STATUS StopHybridCmdMgr(
        PMOS_INTERFACE pOsInterface);

    //!
    //! \brief    Submit Cmd Package to Cmd Consumer
    //! \details  Submit Cmd Package to Cmd Consumer
    //! \param    PMOS_INTERFACE pOsInterface
    //!           [in] ptr to pOsInterface
    //! \param    CmdPackage& cmdPackage
    //!           [in] reference to cmdPackage
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS
    //!
    static MOS_STATUS SubmitPackage(
        PMOS_INTERFACE pOsInterface,
        CmdPackage    &cmdPackage);

#if _DEBUG || _RELEASE_INTERNAL
    //!
    //! \brief    Get engine count
    //!
    //! \details  [Virtual Engine Interface] Get engine count from Virtual Engine State in provided stream
    //! \details  Caller: Hal (Scalability) only
    //! \details  Get engine count from virtual engine state
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   uint8_t
    //!           Engine count
    //!
    static uint8_t GetVeEngineCount(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get Engine Logic Id
    //! \details  [Virtual Engine Interface] Get engine Logic Id from Virtual Engine State in provided stream
    //! \details  Caller: Hal (Scalability) only
    //! \details  Get engine Logic Id from virtual engine state
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] instanceIdx
    //!           Engine instance index
    //! \return   uint8_t
    //!
    static uint8_t GetEngineLogicId(
        MOS_STREAM_HANDLE streamState,
        uint32_t instanceIdx);

    //!
    //! \brief    Set Gpu Virtual Address for Debug
    //! \details  Manually make page fault
    //!
    //! \param    [in] pResource
    //!           Resource to set Gpu Address
    //! \param    [in] address
    //!           Address to set
    //! \return   MOS_STATUS
    //!
    static MOS_STATUS SetGpuVirtualAddress(
        PMOS_RESOURCE pResource, 
        uint64_t      address);

#endif // _DEBUG || _RELEASE_INTERNAL

    //!
    //! \brief    Sets the perf tag
    //! \details  Sets the perf tag
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    uint32_t perfTag
    //!           [in] Perf tag
    //! \return   void
    //!
    static void SetPerfTag(
        MOS_STREAM_HANDLE streamState,
        uint32_t       perfTag);

    //!
    //! \brief    Gets the perf tag
    //! \details  Gets the perf tag
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   uint32_t
    //!           Return perf tag
    //!
    static uint32_t GetPerfTag(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Check if Perf Tag is already set
    //! \details  Check if Perf Tag is already set
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   int32_t
    //!
    static int32_t IsPerfTagSet(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Increase performance data frame ID
    //! \details  Increase performance data frame ID
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   void
    //!
    static void IncPerfFrameID(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Set Hybrid Kernel ID
    //! \details  Set Hybrid Kernel ID
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    uint32_t kernelID
    //!           [in] Hybrid Decoder kernel ID
    //! \return   void
    //!
    static void SetPerfHybridKernelID(
        MOS_STREAM_HANDLE streamState,
        uint32_t          kernelID);

    //!
    //! \brief    Reset performance data buffer ID
    //! \details  Reset performance data buffer ID
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   void
    //!
    static void ResetPerfBufferID(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Increase performance data buffer ID
    //! \details  Increase performance data buffer ID
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   VOID
    //!
    static void IncPerfBufferID(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Determines if the GPU Hung
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   int32_t
    //!           Return if the GPU Hung
    //!
    static int32_t IsGPUHung(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get SetMarker enabled flag
    //! \details  Get SetMarker enabled flag from streamState
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   bool
    //!           SetMarker enabled flag
    //!
    static bool IsSetMarkerEnabled(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Get SetMarker resource address
    //! \details  Get SetMarker resource address from streamState
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \return   PMOS_RESOURCE
    //!           SetMarker resource address
    //!
    static PMOS_RESOURCE GetMarkerResource(
        MOS_STREAM_HANDLE streamState);

    //!
    //! \brief    Check if OS resource is nullptr
    //! \details  Check if OS resource is nullptr
    //! \param    PMOS_RESOURCE pOsResource
    //!           [in] Pointer to OS Resource
    //! \return   int32_t
    //!           Return true if nullptr, otherwise false
    //!
    static bool MosResourceIsNull(PMOS_RESOURCE   resource);

    //!
    //! \brief    OS reset resource
    //! \details  Resets the OS resource
    //! \param    PMOS_RESOURCE pOsResource
    //!           [in] Pointer to OS Resource
    //! \return   void
    //!           Return NONE
    //!
    static void MosResetResource(PMOS_RESOURCE   resource);

    //!
    //! \brief    Get Gmm Resource Info
    //! \details  Get Gmm Resource Info
    //! \param    PMOS_RESOURCE resource
    //!           [in/out] pointer to OS resource
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if successful
    //!
    static MOS_STATUS GetGmmResourceInfo(PMOS_RESOURCE resource);

    //!
    //! \brief    Get plane offset inside surface
    //! \details  Returns the offset
    //! \param    MOS_PLANE_OFFSET planeOffset
    //!           [in] Reference to MOS_PLANE_OFFSET structure
    //! \return   int - offset of the plane
    //!
    static int GetPlaneSurfaceOffset(
        const MOS_PLANE_OFFSET &planeOffset);

    //!
    //! \brief    Get Resource array index
    //! \details  Returns the array index
    //! \param    PMOS_RESOURCE
    //!           [in] Pointer to  MOS_RESOURCE
    //! \return   uint32_t - array index
    //!
    static uint32_t GetResourceArrayIndex(
        PMOS_RESOURCE resource);

    //!
    //! \brief  Translate MOS_FORMAT into GMM_RESOURCE_FORMAT
    //!
    static GMM_RESOURCE_FORMAT MosFmtToGmmFmt(MOS_FORMAT format);

    //!
    //! \brief  Translate GMM_RESOURCE_FORMAT into MOS_FORMAT
    //!
    static MOS_FORMAT GmmFmtToMosFmt(GMM_RESOURCE_FORMAT format);

    //!
    //! \brief  Translate MOS_FORMAT into MOS_OS_FORMAT
    //!
    static uint32_t MosFmtToOsFmt(MOS_FORMAT format);

    //!
    //! \brief  Translate MOS_OS_FORMT into MOS_FORMAT
    //!
    static MOS_FORMAT OsFmtToMosFmt(uint32_t format);

    //! \brief    Get usersetting instance for each stream
    //! \details  the user setting instance
    //! \details  call the overloading MosGetUserSettingInstance for osDeviceContext
    //! \param    MOS_STREAM_HANDLE streamState
    //!           [in] streamState
    //! \return   MediaUserSettingSharedPtr - user setting instance
    //!
    static MediaUserSettingSharedPtr MosGetUserSettingInstance(
        MOS_STREAM_HANDLE streamState);

    //! \brief    Get usersetting instance for each stream
    //! \details  the user setting instance
    //! \param    OsDeviceContext osDeviceContext
    //!           [in] osDeviceContext
    //! \return   MediaUserSettingSharedPtr - user setting instance
    //!
    static MediaUserSettingSharedPtr MosGetUserSettingInstance(
        OsDeviceContext *osDeviceContext);

    //! \brief    Get usersetting instance for each stream
    //! \details  the user setting instance
    //! \param    PMOS_CONTEXT mosCtx
    //!           [in] mosCtx
    //! \return   MediaUserSettingSharedPtr - user setting instance
    //!
    static MediaUserSettingSharedPtr MosGetUserSettingInstance(
        PMOS_CONTEXT mosCtx);

    //!
    //! \brief  Translate MOS_OS_FORMT into MOS_FORMAT
    //!
    static bool IsCompressibelSurfaceSupported(MEDIA_FEATURE_TABLE *skuTable);

    //!
    //! \brief  Check if Mismatch Order Programming model is supported
    //!
    static bool IsMismatchOrderProgrammingSupported();

    //!
    //! \brief  Translate GMM_TILE_TYPE to MOS_TILE_TYPE
    //!
    static MOS_TILE_TYPE MapTileType(GMM_RESOURCE_FLAG flags, GMM_TILE_TYPE type);

    //!
    //! \brief  Check if Multiple Codec Devices is in use
    //!
    static bool IsMultipleCodecDevicesInUse(PMOS_INTERFACE osInterface);


    static MOS_STATUS SetMultiEngineEnabled(
        PMOS_INTERFACE pOsInterface,
        MOS_COMPONENT  component,
        bool           enabled);

    static MOS_STATUS GetMultiEngineStatus(
        PMOS_INTERFACE pOsInterface,
        PLATFORM      *platform,
        MOS_COMPONENT  component,
        bool          &isMultiDevices,
        bool          &isMultiEngine);

    //!
    //! \brief  get latest virtual node for encoder and decoder
    //!
    static MOS_GPU_NODE GetLatestVirtualNode(MOS_STREAM_HANDLE streamState, MOS_COMPONENT component);

    //!
    //! \brief  set latest virtual node for encoder and decoder
    //!
    static void SetLatestVirtualNode(MOS_STREAM_HANDLE streamState, MOS_GPU_NODE node);

    //!
    //! \brief  get virtual node for each decoder stream
    //!
    static MOS_GPU_NODE GetDecoderVirtualNodePerStream(MOS_STREAM_HANDLE streamState);

    //!
    //! \brief  set virtual node for each decoder stream
    //!
    static void SetDecoderVirtualNodePerStream(MOS_STREAM_HANDLE streamState, MOS_GPU_NODE node);

    //! \brief    Wait for the created Batch Buffer completion event
    //! \details  Wait for the created Batch Buffer completion event, we will be 
    //!           woken up for every Batch Buffer completion or when TimeOut expires
    //! \param    MOS_STREAM_HANDLE streamState
    //!           [in] Pointer to streamState
    //! \param    GPU_CONTEXT_HANDLE gpuContextHandle
    //!           [in] gpuContextHandle
    //! \param    uint32_t uiTimeOut
    //!           [in] Wait until signaled or TimeOut ms
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if success else failure reason
    //!
    static MOS_STATUS WaitForBBCompleteNotifyEvent(
        MOS_STREAM_HANDLE       streamState,
        GPU_CONTEXT_HANDLE      gpuContextHandle,
        uint32_t                uiTimeOut);

    //!
    //! \brief    Register GPU Context with KMD BB Complete Event
    //! \details  Register GPU Context with KMD BB Complete Event
    //! \param    MOS_STREAM_HANDLE streamState
    //!           [in] Pointer to streamState
    //! \param    GPU_CONTEXT_HANDLE gpuContextHandle
    //!           [in] GPU context handle
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success else fail reason
    //!
    static MOS_STATUS RegisterBBCompleteNotifyEvent(
        MOS_STREAM_HANDLE   streamState,
        GPU_CONTEXT_HANDLE  gpuContextHandle);

    static void GetRtLogResourceInfo(
        PMOS_INTERFACE osInterface,
        PMOS_RESOURCE &osResource,
        uint32_t &size);

    static bool IsPooledResource(MOS_STREAM_HANDLE streamState, PMOS_RESOURCE osResource);

    static uint64_t GetResourceHandle(MOS_STREAM_HANDLE streamState, PMOS_RESOURCE osResource);

    static void SetIsTrinityEnabled(bool bTrinity);

    static bool IsGpuSyncByCmd(MOS_STREAM_HANDLE streamState, GPU_CONTEXT_HANDLE gpuContextHandle);

private:
    //!
    //! \brief    Init per stream parameters
    //! \details  Init per stream parameters
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [in] extraParams
    //!           Additional parameters needed to init streamstate
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS InitStreamParameters(
        MOS_STREAM_HANDLE   streamState,
        EXTRA_PARAMS        extraParams = nullptr);

    //!
    //! \brief    Compose Cmd buffer header
    //! \details  Compose Cmd buffer header if it contains header
    //!
    //! \param    [in] streamState
    //!           Handle of Os Stream State
    //! \param    [out] cmdBuffer
    //!           Cmd buffer to compose header.
    //!
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS ComposeCommandBufferHeader(
        MOS_STREAM_HANDLE streamState,
        COMMAND_BUFFER_HANDLE cmdBuffer);

#if MOS_COMMAND_BUFFER_DUMP_SUPPORTED
    //! \brief    Unified dump command buffer initialization
    //! \details  check if dump command buffer was enabled and create the output directory
    //! \param    [in/out] streamState
    //!           Os stream state to init cmd buffer dump
    //! \return   MOS_STATUS
    //!           Return MOS_STATUS_SUCCESS if successful, otherwise failed
    //!
    static MOS_STATUS DumpCommandBufferInit(
        MOS_STREAM_HANDLE streamState);
#endif  // MOS_COMMAND_BUFFER_DUMP_SUPPORTED

#if (_DEBUG || _RELEASE_INTERNAL)

    enum OS_API_FAIL_TYPE
    {
        OS_API_FAIL_TYPE_NONE    = 0,
        OS_FAIL_ALLOC_GFX_RES    = 1,
        OS_FAIL_REGISTER_GFX_RES = 1 << 1,
        OS_API_FAIL_TYPE_MAX     = OS_FAIL_ALLOC_GFX_RES | OS_FAIL_REGISTER_GFX_RES,
    };

    enum OS_API_FAIL_SIMULATE_MODE
    {
        OS_API_FAIL_SIMULATE_MODE_DEFAULT  = 0,
        OS_API_FAIL_SIMULATE_MODE_RANDOM   = 1,
        OS_API_FAIL_SIMULATE_MODE_TRAVERSE = 1 << 1,
        OS_API_FAIL_SIMULATE_MODE_MAX      = OS_API_FAIL_SIMULATE_MODE_RANDOM | OS_API_FAIL_SIMULATE_MODE_TRAVERSE,
    };

    #define MIN_OS_API_FAIL_FREQ (1)      //max memory allcation fail rate 100%
    #define MAX_OS_API_FAIL_FREQ (10000)  //min memory allcation fail rate 1/10000

    #define MosOsApiFailSimulationEnabled(OsApiType)                  \
        (m_mosOsApiFailSimulateType == OsApiType &&                   \
         m_mosOsApiFailSimulateMode &  OS_API_FAIL_SIMULATE_MODE_MAX)

    //!
    //! \brief    Init OS API fail simulate flags
    //! \details  Init OS API fail simulate flags according user feature value
    //! \param    [in] mosCtx
    //!           os device ctx handle
    //! \return   void
    //!
    static void MosInitOsApiFailSimulateFlag(MediaUserSettingSharedPtr userSettingPtr);

    //!
    //! \brief    Deinit OS API fail simulate flags
    //! \details  Reset OS API fail simulate flags
    //! \param    none
    //! \return   void
    //!
    static void MosDeinitOsApiFailSimulateFlag();

    static bool MosSimulateOsApiFail(
        OS_API_FAIL_TYPE type,
        const char *functionName,
        const char *filename,
        int32_t     line);

    static uint32_t m_mosOsApiFailSimulateType;
    static uint32_t m_mosOsApiFailSimulateMode;
    static uint32_t m_mosOsApiFailSimulateFreq;
    static uint32_t m_mosOsApiFailSimulateHint;
    static uint32_t m_mosOsApiFailSimulateCounter;
#endif
    static bool m_bTrinity;
MEDIA_CLASS_DEFINE_END(MosInterface)
};

#define Mos_ResetResource(resource)     MosInterface::MosResetResource(resource)

#endif  // __MOS_INTERFACE_H__
