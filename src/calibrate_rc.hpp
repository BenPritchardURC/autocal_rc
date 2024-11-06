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

#include "autocal_rc.hpp"

namespace ACPRO2_RG
{

    // parameters used to perform an arbitrary calibration of some certain channels
    typedef struct _ArbitraryCalibrationParams
    {

        bool valid;

        bool Do_LowGain_Not_HighGain;
        int High_Gain_Point; // ignored if Do_LowGain_Not_HighGain
                             // "0 = _CALIBRATION_REQUEST_HI_GAIN_0_5"
                             // "1 = _CALIBRATION_REQUEST_HI_GAIN_1_0"
                             // "2 = _CALIBRATION_REQUEST_HI_GAIN_1_5"
                             // "3 = _CALIBRATION_REQUEST_HI_GAIN_2_0"

        bool Do_Channel_A;
        bool Do_Channel_B;
        bool Do_Channel_C;
        bool Do_Channel_N;

        bool send_DG1000Z_Commands;
        bool send_BK9801_Commands;
        bool Use50HZ;
        float voltageToCommandRMS;

    } ArbitraryCalibrationParams;

    // parameters passed to the routine to do a full calibration: DoFullTripUnitCAL()
    typedef struct _FullCalibrationParams
    {

        bool valid;

        float lo_gain_voltages_rms[4];
        float hi_gain_voltages_rms[4];

        bool do50hz;
        bool do60hz;

        // only one of these should be true
        bool use_bk_precision_9801;
        bool use_rigol_dg1000z;

        bool doHighGain;
        bool doLowGain;

    } FullCalibrationParams;

    // private routines

    static bool CalibrateChannel(HANDLE hTripUnit, CalibrationRequest &calRequest, CalibrationResults &calResults);
    static bool SetSystemAndDeviceSettings(HANDLE hTripUnit, SetSettingsFuncPtr funcPtr);

    // public routines

    bool DoFullTripUnitCAL(
        HANDLE hTripUnit, HANDLE hKeithley, const FullCalibrationParams &params);

    bool Time_CalibrateChannel(
        HANDLE hTripUnit, CalibrationRequest &calRequest, CalibrationResults &calResults, long long &duration);

    bool TripUnitisACPro2_RC(HANDLE hTripUnit);

    void WriteArbitraryCalibrationParamsToINI(const std::string INIFileName, const ArbitraryCalibrationParams &params);
    void ReadArbitraryParamsFromINI(const std::string INIFileName, ArbitraryCalibrationParams &params);

    void WriteFullCalibrationParamsToINI(const std::string INIFileName, const FullCalibrationParams &params);
    void ReadFullCalibrationParamsFromINI(const std::string INIFileName, FullCalibrationParams &params);

    void setDefaultRCFullCalParams(FullCalibrationParams &params);

    void DumpCalibrationResults(CalibrationResults *results);
    bool DoCalibrationCommand(HANDLE hTripUnit, int CalCommand);
    bool InitCalibration(HANDLE hTripUnit);
    bool SetCalToDefault(HANDLE hTripUnit);
    bool DeCommissionTripUnit(HANDLE hTripUnit);
    bool CalibrateChannel(HANDLE hTripUnit, int16_t Command, int16_t RealRMSIndex, uint16_t RMSValue, CalibrationResults *calResults);
    bool WriteCalibrationToFlash(HANDLE hTripUnit);
    bool VirginizeTripUnit(HANDLE hTripUnit);
    bool CheckTripUnitCalibration(HANDLE hTripUnit);
    
    bool TripUnitIsV4(HANDLE hTripUnit);
    bool Enable50hzPersonality(HANDLE hTripUnit);
    bool DoCalibrationCommand(HANDLE hTripUnit, int CalCommand);
    bool Uncalibrate(HANDLE hTripUnit);
    bool DoFullTripUnitCAL(
        HANDLE hTripUnit, HANDLE hKeithley, const FullCalibrationParams& params);

}