/*
* Copyright (c) 2017-2019, Intel Corporation
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
//! \file     codechal_encode_jpeg_g12.h
//! \brief    Defines the encode interface extension for JPEG.
//! \details  Defines all types, macros, and functions required by CodecHal for JPEG encoding. Definitions are not externally facing.
//!

#ifndef __CODECHAL_ENCODER_JPEG_G12_H__
#define __CODECHAL_ENCODER_JPEG_G12_H__

#include "codechal_encode_jpeg.h"
#include "codechal_encode_singlepipe_virtualengine.h"

class CodechalEncodeJpegStateG12: public CodechalEncodeJpegState
{
public:
    PCODECHAL_ENCODE_SINGLEPIPE_VIRTUALENGINE_STATE m_sinlgePipeVeState;  //!< single pipe virtual engine state
    //!
    //! \brief    Constructor
    //!
    CodechalEncodeJpegStateG12(
            CodechalHwInterface* hwInterface,
            CodechalDebugInterface* debugInterface,
            PCODECHAL_STANDARD_INFO standardInfo);
    //!
    //! \brief    Destructor
    //!
    ~CodechalEncodeJpegStateG12();

    //!
    //! \brief    Help function to send prolog with frame tracking information
    //!
    //! \param    [in] cmdBuffer
    //!           Pointer to command buffer
    //! \param    [in] frameTrackingRequested
    //!           True if frame tracking info is needed, false otherwise
    //!
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SendPrologWithFrameTracking(
        PMOS_COMMAND_BUFFER cmdBuffer,
        bool frameTrackingRequested,
        MHW_MI_MMIOREGISTERS *mmioRegister = nullptr) override;

    //!
    //! \brief   Create and Initialize MMC state 
    //!
    MOS_STATUS InitMmcState();

    //derived from base class
    MOS_STATUS Initialize(CodechalSetting *settings) override;

    MOS_STATUS SubmitCommandBuffer(
        PMOS_COMMAND_BUFFER cmdBuffer,
        bool             bNullRendering) override;

    //!
    //! \brief    Set And Populate VE Hint parameters
    //! \details  Set Virtual Engine hint parameter and populate it to primary cmd buffer attributes
    //! \param    [in] cmdBuffer
    //!               Pointer to primary cmd buffer
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS  SetAndPopulateVEHintParams(
        PMOS_COMMAND_BUFFER  cmdBuffer);

    //!
    //! \brief    Encode User Feature Key Report.
    //! \details  Report user feature values set by encode.
    //! 
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success
    //!
    virtual MOS_STATUS UserFeatureKeyReport() override;

    //!
    //! \brief  Set up params for gpu context creation
    //! \return   MOS_STATUS
    //!           MOS_STATUS_SUCCESS if success, else fail reason
    //!
    MOS_STATUS SetGpuCtxCreatOption() override;
};
#endif //__CODECHAL_ENCODER_JPEG_G12_H__
