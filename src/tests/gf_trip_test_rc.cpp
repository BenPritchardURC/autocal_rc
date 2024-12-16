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

#include <windows.h>
#include <chrono>

#include "..\autocal_rc.hpp"
#include "..\util\settings.hpp"
#include "..\devices\arduino.hpp"
#include "gf_trip_test_rc.hpp"

extern bool ArduinoAbortTimingTest;

namespace GF_TRIP_TEST_RC
{

    // per David's spreadsheet
    // we setup the LT and ST like this during the GF tests:

    constexpr float LT_DELAY_SECONDS = 24;
    constexpr float ST_DELAY_SECONDS = 0.4;

    bool SetupTripUnit(
        HANDLE hTripUnit,
        testParams &params)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

        SetSettingsFuncPtr funcptr;

        funcptr = [&params](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
        {
            // Settings->CTRating = params.CTRating;

            // hard-coded values
            DevSettings4->LTPickup = params.CTRating * 10;
            DevSettings4->LTDelay = LT_DELAY_SECONDS * 10;
            DevSettings4->LTEnabled = 1;

            DevSettings4->STPickup = params.CTRating * 6;
            DevSettings4->STDelay = ST_DELAY_SECONDS * 100;
            DevSettings4->STI2T = 0;
            DevSettings4->STEnabled = true;

            DevSettings4->InstantEnabled = false;

            DevSettings4->GFPickup = params.GFPickup;
            DevSettings4->GFDelay = params.GFDelay * 100;
            DevSettings4->GFI2T = params.GFSlope;
            DevSettings4->GFType = params.GFType;

            return true; // indicate we changed the values
        };

        return SetSystemAndDeviceSettings(hTripUnit, funcptr);
    }

    bool FloatCompare(float a, float b)
    {
        return std::abs(a - b) < 0.0001;
    }

    // Ground Fault:                                                      //  12
    // uint32_t GFPickup;	 // Ground Fault Pickup
    // uint16_t GFDelay;	 // Ground Fault Delay * 100
    // uint8_t GFI2T;		 // Ground Fault 1=I2T or 2=I5T Ramp
    // uint8_t GFType;		 // Ground Fault Pickup Type
    // uint8_t GFI2TAmps;	 // GF I2T transition to "definite time".

    // GFSlope 1=I2T or 2=I5T Ramp
    //
    uint32_t TimeTimeToTripMS(
        int CTRating, int GFPickup, float GFDelay, int GFSlope, int GFCurrent)
    {

        // special case
        if (0 == GFDelay)
        {
            return 0;
        }

        if (GFSlope < 0 || GFSlope > 2)
        {
            PrintToScreen("GF_TRIP_TEST_RC::TimeTimeToTripMS() - Invalid GFSlope: " + std::to_string(GFSlope));
            return 0;
        }

        // apply definitions
        if (GFSlope == 0)
        {
            return GFDelay * 1000; // convert to MS
        }

        if (GFSlope == 1)
        {
            if (GFCurrent > 0.6 * CTRating)
            {
                return GFDelay * 1000; // convert to MS
            }
        }

        if (GFSlope == 2)
        {
            if (GFCurrent > 4 * GFPickup)
            {
                return GFDelay * 1000; // convert to MS
            }
        }

        // otherwise, use formulas
        float TB2C;
        float TB5C;
        float TimeToTripSeconds;

        if (GFSlope == 1) // I^2T
        {
            if (FloatCompare(GFDelay, 0.5))
                TB2C = 0.5;
            else if (FloatCompare(GFDelay, 0.4))
                TB2C = 0.4;
            else if (FloatCompare(GFDelay, 0.3))
                TB2C = 0.3;
            else if (FloatCompare(GFDelay, 0.2))
                TB2C = 0.2;
            else if (FloatCompare(GFDelay, 0.1))
                TB2C = 0.01;
            else if (FloatCompare(GFDelay, 0.05))
                TB2C = 0.0;

            else
            {
                PrintToScreen("GF_TRIP_TEST_RC::TimeTimeToTripMS() - Invalid GFDelay: " + std::to_string(GFDelay));
                return 0;
            }

            // use formula from ACPro2 manual
            float xGF = static_cast<float>(GFCurrent) / static_cast<float>(CTRating);
            TimeToTripSeconds = static_cast<float>(TB2C) / static_cast<float>(xGF * xGF);

            // convert to MS
            return TimeToTripSeconds * 1000;
        }
        // I^5T
        {

            if (FloatCompare(GFDelay, 0.5))
                TB5C = 512;
            else if (FloatCompare(GFDelay, 0.4))
                TB5C = 409.6;
            else if (FloatCompare(GFDelay, 0.3))
                TB5C = 307.2;
            else
            {
                PrintToScreen("GF_TRIP_TEST_RC::TimeTimeToTripMS() - Invalid GFDelay: " + std::to_string(GFDelay));
                return 0;
            }

            // use formula from ACPro2 manual
            float x5GF = static_cast<float>(GFCurrent) / static_cast<float>(GFPickup);
            TimeToTripSeconds = static_cast<float>(TB5C) / static_cast<float>(x5GF * x5GF * x5GF * x5GF * x5GF);

            // convert to MS
            return TimeToTripSeconds * 1000;
        }

        PrintToScreen("GF_TRIP_TEST_RC::TimeTimeToTripMS() - Logic Error");
        return 0;
    }

    // returns true if all tests were successfully run
    static bool CheckTripTime_Internal(
        HANDLE hTripUnit, HANDLE hKeithley, HANDLE hArduino,
        const std::vector<testParams> &params,
        std::vector<testResults> &results)
    {
        int SensitivityAmpsPerVolt = 3800;

        // (MsgRspTimingTestResults is not in URCMessageUnion)
        ARDUINO::MsgRspTimingTestResults *msgRspTimingTestResults;

        URCMessageUnion rsp = {0};
        bool retval;

        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        _ASSERT(hArduino != INVALID_HANDLE_VALUE);

        // tell the Arduino to abort any timing tests in progress
        SendURCCommand(hArduino,
                       ARDUINO::MSG_ABORT_TIMING_TEST,
                       ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

        if (!(GetURCResponse(hArduino, &rsp) && MessageIsACK(&rsp)))
        {
            PrintToScreen("error sending MSG_ABORT_TIMING_TEST to the arduino");
            return false;
        }

        int TestPoint = 1;

        for (auto testParam : params)
        {
            if (TestPoint > 1)
            {
                PrintToScreen("waiting for trip unit to reboot after last trip ...");
                Sleep(2000);
            }

            PrintToScreen("Running test point " + std::to_string(TestPoint++));

            if (!SetupTripUnit(hTripUnit, testParam))
            {
                PrintToScreen("Error setting up trip unit");
                return false;
            }

            PrintToScreen("waiting 2 seconds for trip unit to reboot after sending MSG_SET_USR_SETTINGS_4 ...");
            Sleep(2000);

            // clear the trip history
            if (!SendClearTripHistory(hTripUnit))
            {
                PrintToScreen("Error clearing trip history on trip unit");
                return false;
            }

            // tell the Arduino to start timing
            SendURCCommand(hArduino, ARDUINO::MSG_START_TIMING_TEST, ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);
            retval = GetURCResponse(hArduino, &rsp) && MessageIsACK(&rsp);

            if (!retval)
            {
                PrintToScreen("cannot start timing test on the arduino; (test probably already in progress)");
                return false;
            }

            // determine how many voltsRMS to apply to the Rigol
            float RigolVoltage = (testParam.AmpsRMSToApply / SensitivityAmpsPerVolt);

            // compensate for voltage drop
            RigolVoltage *= 1.125;

            PrintToScreen("Amps To Apply: " + FloatToString(testParam.AmpsRMSToApply, 2));
            PrintToScreen("Commanding Rigol to output " + std::to_string(RigolVoltage) + " volts RMS");

            RIGOL_DG1000Z::SetupToApplySINWave(false, std::to_string(RigolVoltage));
            RIGOL_DG1000Z::EnableOutput();

            PrintToScreen("Taking Keithley voltage reading ...");
            double KeithleyReadingVoltsRMS = KEITHLEY::GetVoltageForAutoRange(hKeithley);
            if (KeithleyReadingVoltsRMS == std::numeric_limits<double>::quiet_NaN())
            {
                PrintToScreen("Keithley voltage reading failed; aborted");
                return false;
            }

            int CalculatedCurrentAmps = SensitivityAmpsPerVolt * KeithleyReadingVoltsRMS;

            PrintToScreen("Waiting for test results to become valid....");

            ArduinoAbortTimingTest = false;
            msgRspTimingTestResults = nullptr;

            auto expectedTripTimeMS = TimeTimeToTripMS(
                testParam.CTRating,
                testParam.GFPickup,
                testParam.GFDelay,
                testParam.GFSlope,
                CalculatedCurrentAmps);

            uint32_t TimeToWaitMS = 1.5 * expectedTripTimeMS;

            // if we are dealing with a short trip time, make sure our loop
            // runs for at least 10 seconds
            if (TimeToWaitMS < 1000 * 10)
                TimeToWaitMS = 1000 * 10;

            PrintToScreen("Expected time to trip (ms): " + std::to_string(expectedTripTimeMS));
            PrintToScreen("Max loop time (ms): " + std::to_string(TimeToWaitMS));
            PrintToScreen("Use menu item 'Abort Timing Test' under Arduino menu to abort...");

            // this timing is just so we can time out if the trip doesn't occur.
            // so we can just start counting from this point, even though
            // actually voltage has already been turned on a while ago.
            auto start = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::milliseconds(0);

            // TODO:
            // FIXME:
            // use MSG_ARDUINO_GET_STATUS

            while (!ArduinoAbortTimingTest)
            {
                Sleep(200);

                // did we time out waiting for the trip to occur?
                auto now = std::chrono::high_resolution_clock::now();
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

                if (elapsed.count() >= TimeToWaitMS)
                {
                    PrintToScreen("Timed out waiting for trip after " + std::to_string(elapsed.count()) + " ms");
                    PrintToScreen("Expected trip time was " + std::to_string(expectedTripTimeMS) + " ms");

                    RIGOL_DG1000Z::DisableOutput();
                    return false;
                }

                // send message to the arduino, asking for the timing test results

                SendURCCommand(hArduino, ARDUINO::MSG_GET_TIMING_TEST_RESULTS, ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

                // get MSG_RSP_TIMING_TEST_RESULTS
                retval = GetURCResponse(hArduino, &rsp);

                if (!retval)
                {
                    RIGOL_DG1000Z::DisableOutput();
                    PrintToScreen("cannot get timing results from the Arduino");
                    return false;
                }

                // arduino will NAK us if the test is still in progress...
                if (MSG_NAK == rsp.msgHdr.Type)
                    continue;

                retval = VerifyMessageIsOK(&rsp, ARDUINO::MSG_RSP_TIMING_TEST_RESULTS,
                                           sizeof(ARDUINO::MsgRspTimingTestResults) - sizeof(MsgHdr));

                if (!retval)
                {
                    RIGOL_DG1000Z::DisableOutput();
                    PrintToScreen("cannot get timing results from the Arduino");
                    return false;
                }

                // check to see if the timing tests are valid yet...
                msgRspTimingTestResults = reinterpret_cast<ARDUINO::MsgRspTimingTestResults *>(&rsp);
                if (msgRspTimingTestResults->timingTestResults.ResultsAreValid)
                    break;
            }

            if (ArduinoAbortTimingTest)
            {
                PrintToScreen("Timing test aborted");
                RIGOL_DG1000Z::DisableOutput();
                return false;
            }

            RIGOL_DG1000Z::DisableOutput();

            PrintToScreen("waiting 5 seconds after trip ...");
            Sleep(5000);

            bool tripTypeIsAsExpected; // this is filled in by CheckForExactlyOneTrip

            // the return value from CheckForCorrectTrip() means if we could get the trip history
            // from the trip unit
            if (!CheckForCorrectTrip(
                    hTripUnit,
                    _TRIP_TYPE_GF,
                    tripTypeIsAsExpected))
            {
                // NOTE: this doesn't mean the trip type is wrong
                // it means: we could not determine it due to failure with MSG_GET_TRIP_HISTORY
                PrintToScreen("error sending MSG_GET_TRIP_HISTORY");
                // RIGOL_DG1000Z::DisableOutput(); (already disabled above)
                return false;
            }

            testResults r;

            r.testRan = true;
            r.expectedTimeToTripMS = expectedTripTimeMS;
            r.CalculatedCurrentAmps = CalculatedCurrentAmps;
            r.measuredTimeToTripMS = msgRspTimingTestResults->timingTestResults.elapsedTime;
            r.expectedTripType = _TRIP_TYPE_GF;
            r.tripTypeIsAsExpected = tripTypeIsAsExpected;

            r.errorPercent =
                PercentDifference(
                    r.expectedTimeToTripMS,
                    msgRspTimingTestResults->timingTestResults.elapsedTime);

            results.push_back(r);
        }

        // we return true if all the tests were ran and completed
        bool allTestRan = params.size() == results.size();
        if (allTestRan)
            for (const auto &r : results)
                allTestRan &= r.testRan;

        return allTestRan;
    }

    bool CheckTripTime(
        HANDLE hTripUnit, HANDLE hKeithley, HANDLE hArduino,
        const std::vector<testParams> &params, std::vector<testResults> &results)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
        _ASSERT(hKeithley != INVALID_HANDLE_VALUE);
        _ASSERT(hArduino != INVALID_HANDLE_VALUE);

        bool retval = CheckTripTime_Internal(hTripUnit, hKeithley, hArduino, params, results);

        // extral level of protection, to guarantee that the rigol is not left on
        RIGOL_DG1000Z::DisableOutput();

        return retval;
    }

    void PrintResults(
        const std::vector<testParams> &params,
        const std::vector<testResults> &results)
    {

        if (params.size() != results.size())
        {
            PrintToScreen("Error: params and results are not the same size");
            return;
        }

        PrintToScreen("R.C. INSTANTANIOUS TRIP TESTS");

        for (int i = 0; i < params.size(); ++i)
        {
            const auto &param = params[i];
            const auto &result = results[i];

            PrintToScreen("--------------------------------------------------------------");
            PrintToScreen(Tab(0) + "Test " + std::to_string(i + 1) + ":");
            PrintToScreen("--------------------------------------------------------------");
            PrintToScreen(Tab(1) + "Parameters:");
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)LT_PICKUP_AMPS") + std::to_string(param.CTRating));
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)LT_DELAY_SECONDS") + std::to_string(LT_DELAY_SECONDS));
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)ST_PICKUP_AMPS") + std::to_string(6 * param.CTRating));
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)ST_DELAY_SECONDS") + std::to_string(ST_DELAY_SECONDS));
            PrintToScreen(Tab(2) + Dots(35, "CTRating") + std::to_string(param.CTRating));
            PrintToScreen(Tab(2) + Dots(35, "GF_TYPE") + std::to_string(param.GFType) + " (0=none, 1=Residential, 2=Return)");
            PrintToScreen(Tab(2) + Dots(35, "GF_PICKUP_AMPS") + std::to_string(param.GFPickup));
            PrintToScreen(Tab(2) + Dots(35, "GF_DELAY_SECONDS") + std::to_string(param.GFDelay));
            PrintToScreen(Tab(2) + Dots(35, "GF_SLOPE") + std::to_string(param.GFSlope) + " (0=OFF, 1=I2T, 2=I5T)");

            PrintToScreen(Tab(1) + "Results:");
            PrintToScreen(Tab(2) + Dots(35, "Test Ran") + std::string(result.testRan ? "Yes" : "No"));
            PrintToScreen(Tab(2) + Dots(35, "Calculated Current Amps") + std::to_string(result.CalculatedCurrentAmps));
            PrintToScreen(Tab(2) + Dots(35, "Expected Time to Trip (ms)") + std::to_string(result.expectedTimeToTripMS));
            PrintToScreen(Tab(2) + Dots(35, "Measured Time to Trip (ms)") + std::to_string(result.measuredTimeToTripMS));
            PrintToScreen(Tab(2) + Dots(35, "Expected Trip Type:") + "GF");
            PrintToScreen(Tab(2) + Dots(35, "Trip Type Is As Expected?") + std::string(result.tripTypeIsAsExpected ? "Yes" : "No"));
            PrintToScreen(Tab(2) + Dots(35, "Error Percent") + FloatToString(result.errorPercent, 4));

            PrintToScreen("");
            PrintToScreen("");
        }
    }

    // reads in a file like this:
    //  1500, 1500, OFF, 2500
    // and puts results into params
    bool ReadTestFile(std::string scriptFile, std::vector<testParams> &params)
    {
        std::ifstream file(scriptFile);
        if (!file.is_open())
        {
            PrintToScreen("Error opening file: " + scriptFile);
            return false;
        }

        std::string line;
        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string token;
            testParams p;

            // Skip blank lines
            if (line.empty())
            {
                continue;
            }

            // CT
            if (!std::getline(iss, token, ','))
                return false;
            p.CTRating = std::stoi(token);

            // PU
            if (!std::getline(iss, token, ','))
                return false;
            p.GFPickup = std::stof(token);

            // Delay
            if (!std::getline(iss, token, ','))
                return false;
            p.GFDelay = std::stof(token);

            // GF Slope
            if (!std::getline(iss, token, ','))
                return false;
            p.GFSlope = std::stof(token);

            // GF Type
            if (!std::getline(iss, token, ','))
                return false;
            p.GFType = std::stof(token);

            // Amps To Apply
            if (!std::getline(iss, token, ','))
                return false;
            p.AmpsRMSToApply = std::stof(token);

            // Add the parsed parameters to the vector
            params.push_back(p);
        }

        file.close();
        return true;
    }
}