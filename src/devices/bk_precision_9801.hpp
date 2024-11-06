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

namespace BK_PRECISION_9801
{

    bool OpenVISAInterface();
    void CloseVISAInterface();
    bool WriteCommand(const std::string &command);
    bool WriteCommandReadResponse(const std::string &command, std::string &response);
    bool Initialize();
    bool CheckForDevice();
    bool EnableOutput();
    bool DisableOutput();
    bool SetupToApplySINWave(bool use50Hz, const std::string &voltsRMSAsString);

}