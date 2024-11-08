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
#include "lt_trip_test_rc.hpp"

extern bool ArduinoAbortTimingTest;

namespace LT_TRIP_TEST_RC
{

    bool SetupTripUnit(HANDLE hTripUnit, testParams &params)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

        SetSettingsFuncPtr funcptr;

        funcptr = [params](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
        {
            DevSettings4->LTEnabled = true;

            DevSettings4->LTPickup = params.LTPickupAMPS * 10;
            DevSettings4->LTDelay = params.LT_Delay_Seconds * 10;

            DevSettings4->STPickup = (params.LTPickupAMPS * 10);
            DevSettings4->STDelay = 0.4 * 100;
            DevSettings4->STEnabled = true;

            // we can't have ST and Inst both disabled at the same time
            DevSettings4->InstantEnabled = false;

            return true; // indicate we changed the values
        };

        return SetSystemAndDeviceSettings(hTripUnit, funcptr);
    }

    int TimeTimeToTripMS(int LT_Pickup_AmpsRMS, float LT_Delay_Seconds, int AppliedCurrentAmpsRMS)
    {
        float X = (float)AppliedCurrentAmpsRMS / LT_Pickup_AmpsRMS;
        int TBLC_lt = 36 * LT_Delay_Seconds;
        float T = TBLC_lt / (X * X);
        return T * 1000;
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

            PrintToScreen("LTPickupAMPS: " + std::to_string(testParam.LTPickupAMPS));
            PrintToScreen("LT_Delay_Seconds: " + FloatToString(testParam.LT_Delay_Seconds, 2));
            PrintToScreen("Amps To Apply: " + FloatToString(testParam.AmpsRMSToApply, 2));
            PrintToScreen("Commanding Rigol to output " + std::to_string(RigolVoltage) + " volts RMS");

            RIGOL_DG1000Z::SetupToApplySINWave(false, std::to_string(RigolVoltage));
            RIGOL_DG1000Z::EnableOutput();

            double KeithleyReadingVoltsRMS = KEITHLEY::GetVoltageForAutoRange(hKeithley);
            if (KeithleyReadingVoltsRMS == std::numeric_limits<double>::quiet_NaN())
            {
                PrintToScreen("Keithley voltage reading failed; aborted");
                return false;
            }

            int CalculatedCurrentAmps = SensitivityAmpsPerVolt * KeithleyReadingVoltsRMS;

            PrintToScreen("Waiting for test results to become valid....");
            PrintToScreen("This may take a few minutes; use menu item 'Abort Timing Test' under Arduino menu to abort...");

            ArduinoAbortTimingTest = false;
            msgRspTimingTestResults = nullptr;

            auto expectedTripTimeMS = TimeTimeToTripMS(
                testParam.LTPickupAMPS,
                testParam.LT_Delay_Seconds,
                CalculatedCurrentAmps);

            // this timing is just so we can time out if the trip doesn't occur.
            // so we can just start counting from this point, even though
            // actually voltage has already been turned on a while ago.
            auto start = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::milliseconds(0);

            while (!ArduinoAbortTimingTest)
            {
                Sleep(200);

                // did we time out waiting for the trip to occur?
                // (we wait 1.5 times the expected trip time)

                auto now = std::chrono::high_resolution_clock::now();
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

                if (elapsed.count() >= 1.5 * expectedTripTimeMS)
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

            bool tripTypeIsAsExpected;

            if (!CheckForExactlyOneTrip(
                    hTripUnit,
                    _TRIP_TYPE_LT,
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
            r.CalculatedCurrentAmps = CalculatedCurrentAmps;
            r.expectedTimeToTripMS = expectedTripTimeMS;
            r.measuredTimeToTripMS = msgRspTimingTestResults->timingTestResults.elapsedTime;
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

        PrintToScreen("R.C. LONG TIME TRIP TESTS");

        if (results.size() > params.size())
        {
            PrintToScreen("Error: Logic error; more results than parameters");
            return;
        }

        // it is OK to have less results than params, because the tests could have
        // been aborted.
        // we handle this by just looping through the results we have
        for (int i = 0; i < results.size(); ++i)
        {
            const auto &param = params[i];
            const auto &result = results[i];

            PrintToScreen("--------------------------------------------------------------");
            PrintToScreen(Tab(0) + "Test " + std::to_string(i + 1) + ":");
            PrintToScreen("--------------------------------------------------------------");
            PrintToScreen(Tab(1) + "Parameters:");
            PrintToScreen(Tab(2) + Dots(35, "LTPickupAMPS") + std::to_string(param.LTPickupAMPS));
            PrintToScreen(Tab(2) + Dots(35, "LT_Delay_Seconds") + FloatToString(param.LT_Delay_Seconds, 2));
            PrintToScreen(Tab(2) + Dots(35, "AmpsRMSToApply") + FloatToString(param.AmpsRMSToApply, 2));

            PrintToScreen(Tab(1) + "Results:");
            PrintToScreen(Tab(2) + Dots(35, "Test Ran") + std::string(result.testRan ? "Yes" : "No"));
            PrintToScreen(Tab(2) + Dots(35, "Calculated Current Amps") + std::to_string(result.CalculatedCurrentAmps));
            PrintToScreen(Tab(2) + Dots(35, "Expected Time to Trip (ms)") + std::to_string(result.expectedTimeToTripMS));
            PrintToScreen(Tab(2) + Dots(35, "Measured Time to Trip (ms)") + std::to_string(result.measuredTimeToTripMS));
            PrintToScreen(Tab(2) + Dots(35, "Error Percent") + FloatToString(result.errorPercent, 4));

            PrintToScreen("");
            PrintToScreen("");
        }
    }

    // reads in a file like this:
    //  415, 3.5, 2250
    //  625, 24.0, 2250
    //  735, 2.0, 2250
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

            // Read LTPickupAMPS
            if (!std::getline(iss, token, ','))
                return false;
            p.LTPickupAMPS = std::stoi(token);

            // Read LT_Delay_Seconds
            if (!std::getline(iss, token, ','))
                return false;
            p.LT_Delay_Seconds = std::stof(token);

            // Read AmpsRMSToApply
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