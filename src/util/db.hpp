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

// code for interacting with production database

namespace DB
{

    bool Connect(const std::string &database_file);
    void Disconnect();

    namespace TLThreshold
    {

        typedef struct _LTThresholdsRecord
        {
            int ID;
            std::string SerialNum;

            int LTNoPu_A;
            int LTPu_A;
            int LTNoPu_B;
            int LTPu_B;
            int LTNoPu_C;
            int LTPu_C;
        } LTThresholdRecord;

        bool Read(LTThresholdRecord &record, const std::string &serial_num);
        bool Write(const LTThresholdRecord &record);
    }

    namespace Personality
    {

        typedef struct _PersonalityRecord
        {
            int ID;
            std::string SerialNum;

            bool LTPUOnOffPersonalitity;
            bool LTPUto120PercentPersonality;
            bool LTDlyto50secPersonality;
            bool InstOverridePersonality;
            bool InstClosePersonality;
            bool InstBlockPersonality;
            bool GFOnlyPersonality;
        } PersonalityRecord;

        bool Read(PersonalityRecord &record, const std::string &serial_num);
        bool Write(const PersonalityRecord &record);

    }

}