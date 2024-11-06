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

#include <windows.h>
#include <vector>

namespace GF_TRIP_TEST_RC
{

    struct testParams
    {
        int CTRating;
        int GFType;
        int GFPickup;
        float GFDelay;
        int GFSlope;
        float AmpsRMSToApply;
    };

    struct testResults
    {
        bool testRan;
        int CalculatedCurrentAmps;
        int32_t expectedTimeToTripMS;
        int32_t measuredTimeToTripMS;
        float errorPercent;
        int expectedTripType;
        bool tripTypeIsAsExpected;
    };

    bool CheckTripTime(
        HANDLE hTripUnit, HANDLE hKeithley, HANDLE hArduino,
        const std::vector<testParams> &params, std::vector<testResults> &results);

    void PrintResults(
        const std::vector<testParams> &params,
        const std::vector<testResults> &results);

    bool ReadTestFile(std::string scriptFile, std::vector<testParams> &params);

    // fixme
    // these are just here temporarily

    bool SetupTripUnit(
        HANDLE hTripUnit,
        testParams &params);

    uint32_t TimeTimeToTripMS(
        int CTRating, int GFPickup, float GFDelay, int GFSlope, int GFCurrent);

}