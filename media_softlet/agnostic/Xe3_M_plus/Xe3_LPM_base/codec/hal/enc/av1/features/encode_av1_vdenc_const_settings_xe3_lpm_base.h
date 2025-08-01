/*
* Copyright (c) 2022, Intel Corporation
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
//! \file     encode_av1_vdenc_const_settings_xe3_lpm_base.h
//! \brief    Defines the common interface for av1 vdenc const settings
//! \details  The default setting is further sub-divided by platform type
//!           this file is for the base interface which is shared by all components.
//!

#ifndef __ENCODE_AV1_VDENC_CONST_SETTINGS_XE3_LPM_H__
#define __ENCODE_AV1_VDENC_CONST_SETTINGS_XE3_LPM_H__

#include "encode_av1_vdenc_const_settings.h"

namespace encode
{

struct Av1VdencTUConstSettingsXe3_Lpm_Base
{
    static const uint8_t  vdencCmd2Par4[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par38[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par39[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par67[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par83Table2[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par83Table1[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par83Table0[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par84Table0[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par84Table1[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par84Table2[NUM_TARGET_USAGE_MODES];
    static const uint32_t vdencCmd2Par85Table0[NUM_TARGET_USAGE_MODES];
    static const uint32_t vdencCmd2Par85Table1[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par86[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par87Table3[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par87Table2[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par87Table1[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par87Table0[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table13[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table12[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table11[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table10[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table23[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table22[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table21[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table20[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table03[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table02[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table01[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par88Table00[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par89[NUM_TARGET_USAGE_MODES];
    static const uint16_t vdencCmd2Par92[NUM_TARGET_USAGE_MODES];
    static const uint16_t vdencCmd2Par93[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par151[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par100[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par94[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par95[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par96[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par97[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par98[NUM_TARGET_USAGE_MODES];
    static const uint16_t vdencCmd2Par12[NUM_TARGET_USAGE_MODES];
    static const uint16_t vdencCmd2Par13[NUM_TARGET_USAGE_MODES];
    static const uint16_t vdencCmd2Par14[NUM_TARGET_USAGE_MODES];
    static const uint8_t  temporalMvp[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par18[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par15[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par23[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par133[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par102[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par101[NUM_TARGET_USAGE_MODES];
    static const bool     vdencCmd2Par109[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par142[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par143[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par144[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par145[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par146[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par147[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par148[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par149[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par138[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par150[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par152[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par139[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par153[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par154[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par155[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par156[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par140[NUM_TARGET_USAGE_MODES];
    static const uint8_t  vdencCmd2Par141[NUM_TARGET_USAGE_MODES];
    static const uint8_t  av1EnableIntraEdgeFilter[NUM_TARGET_USAGE_MODES];
};

class EncodeAv1VdencConstSettingsXe3_Lpm_Base : public EncodeAv1VdencConstSettings
{
public:

    //!
    //! \brief  EncodeAv1VdencConstSettingsXe3_Lpm_Base constructor
    //!
    EncodeAv1VdencConstSettingsXe3_Lpm_Base(PMOS_INTERFACE osInterface) :
        EncodeAv1VdencConstSettings (osInterface) {}

    //!
    //! \brief  EncodeAv1VdencConstSettingsXe3_Lpm_Base deconstructor
    //!
    virtual ~EncodeAv1VdencConstSettingsXe3_Lpm_Base() {}

    //!
    //! \brief  Prepare CMD1 TU related settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetVdencCmd1Settings() override;

    //!
    //! \brief  Prepare CMD2 TU related settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetVdencCmd2Settings() override;

    //!
    //! \brief  Prepare StreamIn TU related settings
    //! \return MOS_STATUS
    //!         MOS_STATUS_SUCCESS if success, else fail reason
    //!
    virtual MOS_STATUS SetVdencStreaminStateSettings() override;

MEDIA_CLASS_DEFINE_END(encode__EncodeAv1VdencConstSettingsXe3_Lpm_Base)
};


}
#endif // !__ENCODE_AV1_VDENC_CONST_SETTINGS_XE3_LPM_H__
