/*******************************************************************************

 * Copyright 2024  Utility Relay Company (URC) Chagrin Falls, Ohio.
 * All Rights Reserved.
 *
 * The information contained herein is confidential property of URC.  All uasge,
 * copying, transfer, or disclosure of this information is prohibited by law.
 *
 *  AutoCAL_RC - ACPro2-RC testing and calibration software
 *  Original Author: Benjamin Pritchard
 *
 *******************************************************************************/

#pragma once

#include "..\ftdi\ftd2xx.h"

namespace FTDI
{

    constexpr int NO_FTDI_INDEX = -1;

    void DumpEEProm(FT_HANDLE handle);
    void EnumerateFTDI();
    bool ProgramEEPROM(const char *mfg, const char *description, int FTDI_Index);
    int FindTripUnitFTDIIndex(int TripUnitCommPort);
    bool VerifyEEPROM(const char *expected_mfg, const char *expected_description, int FTDI_Index);

}
