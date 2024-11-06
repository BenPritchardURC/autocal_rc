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

#include "..\autocal_rc.hpp"

namespace KEITHLEY
{

    constexpr int KEITHLEY_TIMEOUT_MS = 200;

    bool Connect(HANDLE *hKeithley, int port)
    {
        bool retval = true;

        retval = InitCommPort(hKeithley, port, 19200);

        if (retval)
        {
            WriteToCommPort_Str(*hKeithley, "*IDN?");
            retval = GetExpectedString(*hKeithley, KEITHLEY_TIMEOUT_MS, "KEITHLEY");
        }

        if (retval)
        {
            Initialize(*hKeithley);
        }

        if (!retval)
        {
            CloseCommPort(hKeithley);
        }

        return retval;
    }

    void Initialize(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);

        WriteToCommPort_Str(hKeithley, "*RST;:SYST:BEEP:STAT 0");
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":FORM:DATA ASCII");
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":SENS:FUNC 'VOLT:AC'");
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":VOLT:NPLC 10"); // set to meduim sample speed: 120 samples takes 120 ms
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":SENS:VOLT:AC:RANG 0"); // Set Keithley to mVolt range (expected default)
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":TRIG:COUNT 1");
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":TRIG:SOUR IMM");
        Sleep(400);
        WriteToCommPort_Str(hKeithley, ":SENS:VOLT:AC:DET:BAND 30"); // MEDium Rate: bandwidth  30 - 300KHz  @  120 ms rate
    }

    double GetVoltage(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);

        uint8_t rsp[1024];

        WriteToCommPort_Str(hKeithley, ":INIT");
        Sleep(200);
        WriteToCommPort_Str(hKeithley, ":FETC?");

        if (GetRawResponse(hKeithley, rsp, KEITHLEY_TIMEOUT_MS, -1))
        {
            return atof((const char *)rsp);
        }
        else
        {
            return 0;
        }
    }

    double GetAverageVoltage(HANDLE hKeithley, int samples)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);

        double sum = 0;

        for (int i = 0; i < samples; i++)
        {
            sum += GetVoltage(hKeithley);
        }

        return sum / samples;
    }

    void RANGE0(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        WriteToCommPort_Str(hKeithley, ":SENS:VOLT:AC:RANG 0");
    }

    void RANGE1(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        WriteToCommPort_Str(hKeithley, ":SENS:VOLT:AC:RANG 1");
    }

    void RANGE10(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        WriteToCommPort_Str(hKeithley, ":SENS:VOLT:AC:RANG 10");
    }

    void RANGE_AUTO(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        WriteToCommPort_Str(hKeithley, ":SENSE:VOLT:AC:RANG:AUTO ON");
    }

    double GetVoltageForAutoRange(HANDLE hKeithley)
    {
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);

        // PrintToScreen("taking Keithley voltage reading....");
        KEITHLEY::RANGE_AUTO(hKeithley);
        Sleep(400);
        double KeithleyVoltageRMS = KEITHLEY::GetVoltage(hKeithley);

        // try to get a good reading... this logic seems required when doing auto-ranging
        for (int i = 0; i < 10; i++)
        {
            if (KeithleyVoltageRMS >= 1000 || KeithleyVoltageRMS <= 0)
            {
                // PrintToScreen("invalid Keithley RMS voltage: " + std::to_string(KeithleyVoltageRMS) + "; retrying");
                Sleep(200);
                KeithleyVoltageRMS = KEITHLEY::GetVoltage(hKeithley);
            }
            else
            {
                // PrintToScreen("got good keithley after " + std::to_string(i) + " tries");
                return KeithleyVoltageRMS;
            }
        }

        PrintToScreen("Could not get valid Keithley voltage reading within 10 tries; aborting");
        return std::numeric_limits<double>::quiet_NaN();
    }

    bool VoltageOnKeithleyIsStable(HANDLE hKeithley, bool quietMode)
    {
        double KeithleyVoltageRMS[5];

        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);

        // first reading might be needed
        double IgnoreME = KEITHLEY::GetVoltageForAutoRange(hKeithley);

        for (int i = 0; i < 5; i++)
        {
            auto loop_iteration_start = std::chrono::high_resolution_clock::now();
            KeithleyVoltageRMS[i] = KEITHLEY::GetVoltageForAutoRange(hKeithley);
            auto loop_iteration_end = std::chrono::high_resolution_clock::now();
            auto loop_iteration_duration = std::chrono::duration_cast<std::chrono::milliseconds>(loop_iteration_end - loop_iteration_start).count();

            if (1000 - loop_iteration_duration > 0)
                Sleep(1000 - loop_iteration_duration);

            if (!quietMode)
                PrintToScreen("sample #: " + std::to_string(i) + ", Keithley Voltage RMS: " + std::to_string(KeithleyVoltageRMS[i]));
        }

        // calc min, max, average, and spread between min and max
        float minValue = KeithleyVoltageRMS[0];
        float maxValue = KeithleyVoltageRMS[0];
        float sumValue = 0;

        for (int i = 0; i < 5; i++)
        {
            if (KeithleyVoltageRMS[i] < minValue)
                minValue = KeithleyVoltageRMS[i];
            if (KeithleyVoltageRMS[i] > maxValue)
                maxValue = KeithleyVoltageRMS[i];
            sumValue += KeithleyVoltageRMS[i];
        }

        float averageValue = sumValue / 5;
        float spreadValue = maxValue - minValue;

        // Print the calculated values
        if (!quietMode)
            PrintToScreen("Min Value: " + std::to_string(minValue));
        if (!quietMode)
            PrintToScreen("Max Value: " + std::to_string(maxValue));
        if (!quietMode)
            PrintToScreen("Average Value: " + std::to_string(averageValue));
        if (!quietMode)
            PrintToScreen("Spread Value: " + std::to_string(spreadValue));

        return spreadValue < 0.001;
    }

    bool VoltageOnKeithleyIsStable(HANDLE hKeithley)
    {
        return VoltageOnKeithleyIsStable(hKeithley, false);
    }

}

namespace ASYNC_KEITHLEY
{
    static bool _keepMonitoringKeithley;
    static bool _keithleyIsStable;
    static bool _MonitoringInProgress;

    static void MonitorKeithleyDuringCAL(HANDLE hKeithley)
    {

        _MonitoringInProgress = true;
        _keithleyIsStable = true;

        // just keep going until somebody tells us to stop
        while (_keepMonitoringKeithley)
        {
            _keithleyIsStable &= KEITHLEY::VoltageOnKeithleyIsStable(hKeithley, true);
            if (!_keithleyIsStable)
            {
                PrintToScreen("Keithley is not stable");
            }
            Sleep(1000);
        }

        _MonitoringInProgress = false;
    }

    void StartMonitoringKeithley(HANDLE hKeithley)
    {
        _keepMonitoringKeithley = true;
        std::thread t(MonitorKeithleyDuringCAL, hKeithley);
        t.detach();
    }

    void StopMonitoringKeithley()
    {
        _keepMonitoringKeithley = false;

        while (_MonitoringInProgress)
        {
            Sleep(1000);
        }
    }

    bool KeithleyIsStable()
    {
        return _keithleyIsStable;
    }

}