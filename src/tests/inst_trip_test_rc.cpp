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
#include "inst_trip_test_rc.hpp"

extern bool ArduinoAbortTimingTest;

namespace INST_TRIP_TEST_RC
{

    // per David's spreadsheet:
    constexpr int LT_PICKUP_AMPS = 800;
    constexpr float LT_DELAY_SECONDS = 20;

    bool SetupTripUnit(
        HANDLE hTripUnit,
        testParams &params)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

        SetSettingsFuncPtr funcptr;

        funcptr = [&params](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
        {
            // hard-coded values
            DevSettings4->LTPickup = LT_PICKUP_AMPS * 10;
            DevSettings4->LTDelay = LT_DELAY_SECONDS * 10;
            DevSettings4->STEnabled = false;
            DevSettings4->GFPickup = 0;

            // FIXME: these values need to be more thoroughly defined
            // per spreadsheet

            DevSettings4->InstantEnabled = true;
            DevSettings4->InstantPickup = params.InstPickup;
            DevSettings4->QTInstPickup = params.QTInstPickup;

            return true; // indicate we changed the values
        };

        return SetSystemAndDeviceSettings(hTripUnit, funcptr);
    }

    bool SetupQuickTrip(HANDLE hTripUnit, bool Enable)
    {

        if (!SendSetQTStatus(hTripUnit, Enable))
        {
            PrintToScreen("Error getting LED status from trip unit");
            return false;
        }

        PrintToScreen("waiting 5 seconds for QT to enable/disable...");
        Sleep(1000 * 5);

        // grab LEDs; make sure trip unit reports QT is in the right state
        URCMessageUnion rsp = {0};
        SendURCCommand(hTripUnit, MSG_GET_LEDS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
        GetURCResponse(hTripUnit, &rsp);

        // NOTE: sizeof(MsgRspLEDs) somehow is not packing correctly?
        //		 it seems to be 5 bytes + header
        //		 but trip unit says length is 6 bytes + header

        if (!VerifyMessageIsOK(&rsp, MSG_RSP_LEDS, 16 - sizeof(MsgHdr)))
        {
            PrintToScreen("Error getting LED status from trip unit");
            return false;
        }

        if (Enable)
            return rsp.msgRspLEDs.State[LED_QT];
        else
            return !rsp.msgRspLEDs.State[LED_QT];
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
                PrintToScreen("delaying 30 seconds between tests ...");
                Sleep(1000 * 30);
            }

            PrintToScreen("Running test point " + std::to_string(TestPoint++));

            if (!SetupTripUnit(hTripUnit, testParam))
            {
                PrintToScreen("Error setting up trip unit");
                return false;
            }

            PrintToScreen("waiting 2 seconds for trip unit to reboot after sending MSG_SET_USR_SETTINGS_4 ...");
            Sleep(2000);

            // setup the QT status, based on the test parameters

            if (!SetupQuickTrip(hTripUnit, testParam.QTEnabled))
            {
                PrintToScreen("Error setting QT status on trip unit");
                return false;
            }

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

            PrintToScreen("Using Hard-coded value LT_PICKUP_AMPS: " + std::to_string(LT_PICKUP_AMPS));
            PrintToScreen("Using Hard-coded value LT_DELAY_SECONDS: " + FloatToString(LT_DELAY_SECONDS, 2));
            PrintToScreen("InstPickup: " + std::to_string(testParam.InstPickup));
            PrintToScreen("QTInstPickup: " + std::to_string(testParam.QTInstPickup));
            PrintToScreen("QTEnabled: " + std::to_string(testParam.QTEnabled));
            PrintToScreen("tripTimeThresholdMS: " + std::to_string(testParam.tripTimeThresholdMS));
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
            msgRspTimingTestResults = nullptr; // just to prevent compiler warning

            auto start = std::chrono::high_resolution_clock::now();
            auto elapsed = std::chrono::milliseconds(0);

            // TODO:
            // FIXME:
            // use MSG_ARDUINO_GET_STATUS

            while (!ArduinoAbortTimingTest)
            {
                Sleep(200);

                auto now = std::chrono::high_resolution_clock::now();
                elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start);

                if (elapsed.count() >= 1000)
                {
                    PrintToScreen("Timed out after 1 second waiting for instantanious trip");
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

            if (!CheckForCorrectTrip(
                    hTripUnit,
                    testParam.QTEnabled ? _TRIP_TYPE_QT_I_DIGITAL : _TRIP_TYPE_INST_DIGITAL,
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
            r.measuredTimeToTripMS = msgRspTimingTestResults->timingTestResults.elapsedTime;
            r.expectedTripType = testParam.QTEnabled ? _TRIP_TYPE_QT_I_DIGITAL : _TRIP_TYPE_INST_DIGITAL;
            r.tripTypeIsAsExpected = tripTypeIsAsExpected;
            r.TripTimeIsBelowThreshold = r.measuredTimeToTripMS < testParam.tripTimeThresholdMS;

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

    // note: this doesn't work on Inst_TripType
    // it is for the trip unit firmware constants [used in these INST tests]
    std::string INSTExpectedTripTypeToString(int type)
    {
        switch (type)
        {
        case _TRIP_TYPE_QT_I_DIGITAL:
            return "QT-I";
            break;
        case _TRIP_TYPE_INST_DIGITAL:
            return "Inst";
            break;
        default:
            return "OTHER";
        }
    }

    void PrintResults(
        const std::vector<testParams> &params,
        const std::vector<testResults> &results)
    {
        PrintToScreen("R.C. INSTANTANIOUS TRIP TESTS");

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
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)LT_PICKUP_AMPS") + std::to_string(LT_PICKUP_AMPS));
            PrintToScreen(Tab(2) + Dots(35, "(Hard-coded)LT_DELAY_SECONDS") + FloatToString(LT_DELAY_SECONDS, 2));
            PrintToScreen(Tab(2) + Dots(35, "InstPickup") + std::to_string(param.InstPickup));
            PrintToScreen(Tab(2) + Dots(35, "QTInstPickup") + std::to_string(param.QTInstPickup));
            PrintToScreen(Tab(2) + Dots(35, "AmpsRMSToApply") + FloatToString(param.AmpsRMSToApply, 2));
            PrintToScreen(Tab(2) + Dots(35, "QTEnabled") + std::string(param.QTEnabled ? "Yes" : "No"));

            PrintToScreen(Tab(1) + "Results:");
            PrintToScreen(Tab(2) + Dots(35, "Test Ran") + std::string(result.testRan ? "Yes" : "No"));
            PrintToScreen(Tab(2) + Dots(35, "Calculated Current Amps") + std::to_string(result.CalculatedCurrentAmps));
            PrintToScreen(Tab(2) + Dots(35, "Measured Time to Trip (ms)") + std::to_string(result.measuredTimeToTripMS));
            PrintToScreen(Tab(2) + Dots(35, "Expected Trip Type") + INSTExpectedTripTypeToString(result.expectedTripType));
            PrintToScreen(Tab(2) + Dots(35, "Trip Type Is As Expected?") + std::string(result.tripTypeIsAsExpected ? "Yes" : "No"));
            PrintToScreen(Tab(2) + Dots(35, "Trip Time Is Below Threshold?") + std::string(result.TripTimeIsBelowThreshold ? "Yes" : "No"));

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

            // Read InstPickup
            if (!std::getline(iss, token, ','))
                return false;
            p.InstPickup = std::stoi(token);

            // Read QTInstPickup
            if (!std::getline(iss, token, ','))
                return false;
            p.QTInstPickup = std::stof(token);

            // Read QTEnabled
            if (!std::getline(iss, token, ','))
                return false;
            p.QTEnabled = (token == "1");

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