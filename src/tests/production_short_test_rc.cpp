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

namespace PRODUCTION_SHORT_TEST_RC
{
    const int NUM_PHASES = 4;
    const int NUM_TEST_POINTS = 2;
    const int NUM_FREQUENCIES = 2;

    const int TEST_POINT_1 = 0;
    const int TEST_POINT_2 = 1;

    typedef struct _ResultsAtPoint
    {
        int32_t idealCurrent_AmpsRMS;  // the current we want to output
        int32_t outputCurrent_AmpsRMS; // current we actually output; is pretty close to idealCurrent_AmpsRMS
        int32_t upperLimit_AmpsRMS;
        int32_t lowerLimit_AmpsRMS;

        int32_t tripUnitCurrent_AmpsRMS[NUM_PHASES];
        float percentError[NUM_PHASES];
        bool passed[NUM_PHASES];

        // this point passed if all the phases passed
        bool pointPassed;

    } ResultsAtPoint;

    /*
    typedef struct _ResultsAtFreq
    {


        bool frequencyTested;
        bool frequencyPassed;
    } ResultsAtFreq;
    */

    typedef struct _TestResults
    {
        // ResultsAtFreq resultsAtFreq[NUM_FREQUENCIES];
        ResultsAtPoint resultsAtPoint[NUM_TEST_POINTS]; // 0 = point 1  (1000 amps)

        bool passed;
        TimeDate4 timeTested;
    } TestResults;

    // how to run this test
    typedef struct _TestParams
    {
        int setPointRMSCurrent[NUM_TEST_POINTS];
        bool TestPhasesSeperately;
        bool Test50hz;
        bool Test60hz;
    } TestParams;

    static std::string PointToAmpString(TestParams &TestParams, int point)
    {
        if (point >= 0 && point < NUM_TEST_POINTS)
            return std::to_string(TestParams.setPointRMSCurrent[point]) + " ampsRMS";
        else if (point == 0)
            return "invalid index passed to PointToAmpString()";
    }

    static void SaveTestResults()
    {
    }

    static void LoadTestResults()
    {
    }

    static void PrintTestResults(
        const TestResults &testResults, const TestParams &testParams)
    {
        const char *freqNames[] = {"50hz", "60hz"};
        const char *phaseNames[] = {"Phase A", "Phase B", "Phase C", "Phase N"};

        PrintToScreen("=========================================");
        PrintToScreen("R.C. Production Short Test Parameters:");
        PrintToScreen("=========================================");

        for (int i = 0; i < NUM_TEST_POINTS; ++i)
        {
            PrintToScreen("Point " + std::to_string(i) + " (amps RMS)" + std::to_string(testParams.setPointRMSCurrent[i]));
        }

        PrintToScreen("Phases Tested Seperately? " + BoolToYesNo(testParams.TestPhasesSeperately));
        PrintToScreen("Test 50hz? " + BoolToYesNo(testParams.Test50hz));
        PrintToScreen("Test 60hz? " + BoolToYesNo(testParams.Test60hz));

        PrintToScreen("=========================================");
        PrintToScreen("R.C. Production Short Test Results:");
        PrintToScreen("=========================================");

        // loop through the frequencies
        for (int freq_index = 0; freq_index < NUM_FREQUENCIES; ++freq_index)
        {
            PrintToScreen(Tab(0) + "Results at " + std::string(freqNames[freq_index]));
            PrintToScreen(Tab(0) + "Frequency Tested? " + BoolToYesNo(freqNames[freq_index]));

            // loop through all the configured points
            for (int point_index = 0; point_index < NUM_TEST_POINTS; ++point_index)
            {
                PrintToScreen(Tab(2) + "Results at point " + std::to_string(testParams.setPointRMSCurrent[point_index]));

                const ResultsAtPoint &resultsAtPoint =
                    testResults.resultsAtPoint[point_index];

                PrintToScreen(Tab(3) + "Ideal Current (ampsRMS)......." + std::to_string(resultsAtPoint.idealCurrent_AmpsRMS));
                PrintToScreen(Tab(3) + "Output Current (ampsRMS)......" + std::to_string(resultsAtPoint.outputCurrent_AmpsRMS));

                for (int i = 0; i < 4; ++i)
                {
                    PrintToScreen(Tab(3) + "Trip Unit Current (ampsRMS)..." + std::to_string(resultsAtPoint.tripUnitCurrent_AmpsRMS[i]));
                    PrintToScreen(Tab(3) + "Percent Error (%)............." + std::to_string(resultsAtPoint.percentError[i]));
                    PrintToScreen(Tab(3) + "Result at this point.........." + BoolToPassFail(resultsAtPoint.passed[i]));
                }
            }
        }

        // PrintToScreen(Tab(1) + "Overall Test Result at point " + std::string(BoolToPassFail(TestResults.pointPassed)));
    }

    static bool TestAtPoint(
        HANDLE hATB, HANDLE hTripUnit, HANDLE hKeithley, int setPointRMSCurrent, ResultsAtPoint &resultsAtPoint)
    {

        double KeithleyVoltagevRMS;
        int calculatedRMSCurrent;
        int lowerLimit, upperLimit;
        URCMessageUnion Rsp;
        bool OperationOK;

        float voltsToCommandvRMS = setPointRMSCurrent / 3800;

        OperationOK = RIGOL_DG1000Z::SetupToApplySINWave(false, FloatToString(voltsToCommandvRMS, 3));

        if (OperationOK)
            OperationOK = RIGOL_DG1000Z::EnableOutput();

        if (!OperationOK)
        {
            PrintToScreen("ERROR: could not setup RIGOL; aborting");
            return false;
        }

        // about setPointRMSCurrent amps are now flowing...

        // calculate amps ourselves using the keithley

        KeithleyVoltagevRMS = KEITHLEY::GetVoltage(hKeithley);
        calculatedRMSCurrent = KeithleyVoltagevRMS * 3800;

        lowerLimit = calculatedRMSCurrent - (calculatedRMSCurrent * 0.02);
        upperLimit = calculatedRMSCurrent + (calculatedRMSCurrent * 0.02);

        resultsAtPoint.idealCurrent_AmpsRMS = setPointRMSCurrent;
        resultsAtPoint.idealCurrent_AmpsRMS = setPointRMSCurrent;
        resultsAtPoint.idealCurrent_AmpsRMS = setPointRMSCurrent;
        resultsAtPoint.outputCurrent_AmpsRMS = calculatedRMSCurrent;
        resultsAtPoint.upperLimit_AmpsRMS = upperLimit;
        resultsAtPoint.lowerLimit_AmpsRMS = lowerLimit;

        // grab readings from the trip unit
        if (!GetDynamics(hTripUnit, &Rsp))
        {
            PrintToScreen("ERROR: could not get reading from trip unit; aborting");
            RIGOL_DG1000Z::DisableOutput();
            return false;
        }

        // grab all the readings at the same time...
        resultsAtPoint.tripUnitCurrent_AmpsRMS[0] = Rsp.msgRspDynamics4.Dynamics.Measurements.Ia;
        resultsAtPoint.tripUnitCurrent_AmpsRMS[1] = Rsp.msgRspDynamics4.Dynamics.Measurements.Ib;
        resultsAtPoint.tripUnitCurrent_AmpsRMS[2] = Rsp.msgRspDynamics4.Dynamics.Measurements.Ic;
        resultsAtPoint.tripUnitCurrent_AmpsRMS[3] = Rsp.msgRspDynamics4.Dynamics.Measurements.In;

        for (int i = 0; i < 4; ++i)
        {
            resultsAtPoint.percentError[i] =
                PercentDifference(resultsAtPoint.tripUnitCurrent_AmpsRMS[i], calculatedRMSCurrent);

            // did we pass?
            resultsAtPoint.passed[i] = (resultsAtPoint.tripUnitCurrent_AmpsRMS[i] >= lowerLimit) &&
                                       (resultsAtPoint.tripUnitCurrent_AmpsRMS[i] <= upperLimit);
        }

        RIGOL_DG1000Z::DisableOutput();
        return true;
    }

    bool SetupTripUnitForShortTests(HANDLE hTripUnit)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

        SetSettingsFuncPtr funcptr;

        // short tests are always ran at 60hz
        funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
        {
            Settings->CTNeutralRating = 1000;
            Settings->CTRating = 1000;
            Settings->Frequency = 60;
            DevSettings4->SBThreshold = 0;
            DevSettings4->LTPickup = 9000; // x10
            DevSettings4->LTDelay = 300;   // x10

            return true;
        };

        return SetSystemAndDeviceSettings(hTripUnit, funcptr);
    }

    static void CheckForTrip(HANDLE hTripUnit, TestResults &TestResults)
    {
        _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

        URCMessageUnion rsp = {0};

        if (!GetTripHistory(hTripUnit, &rsp))
        {
            PrintToScreen("Error sending MSG_GET_TRIP_HIST");
            return;
        }

        int total = 0;

        for (int i = 0; i < _TRIP_TYPE_MAX; ++i)
            total += rsp.msgRspTripHist4.Trips.Counter.trip[i];

        if (total > 0)
        {
            TestResults.passed = false;
            PrintToScreen("**** ERROR **** unexpected trip detected during short test");
        }
    }

    void DeterminePassFail(TestResults &testResults)
    {
        // the point passed if all the phases in the point passed
        for (int point_index = 0; point_index < NUM_TEST_POINTS; ++point_index)
        {
            testResults.resultsAtPoint[point_index].pointPassed = true;

            testResults.resultsAtPoint[point_index].pointPassed &=
                (testResults.resultsAtPoint[point_index].passed[0] &&
                 testResults.resultsAtPoint[point_index].passed[1] &&
                 testResults.resultsAtPoint[point_index].passed[2] &&
                 testResults.resultsAtPoint[point_index].passed[3]);
        }

        // overall test passed if each point passed
        testResults.passed = true;

        for (int point_index = 0; point_index < NUM_TEST_POINTS; ++point_index)
            testResults.passed &=
                (testResults.resultsAtPoint[point_index].pointPassed);
    }

    void RunTest(HANDLE hATB, HANDLE hTripUnit, HANDLE hKeithley)
    {
        bool retval = true;
        TestResults testResults = {0};
        TestParams testParams = {0};

        // setup which points we want to test at
        testParams.setPointRMSCurrent[TEST_POINT_1] = 1000;
        testParams.setPointRMSCurrent[TEST_POINT_2] = 6000;

        auto StartTime = std::chrono::system_clock::now();

        PrintToScreen("selecting 60hz frequency on trip unit");
        retval = SetupTripUnitForShortTests(hTripUnit);

        if (!retval)
        {
            scr_printf("cannot setup trip unit parameters for calibration; aborting short tests");
            return;
        }

        scr_printf("waiting 2 seconds for trip unit to reboot...");
        Sleep(2000);

        PrintToScreen("Clearing trip history....");

        // start off by clearing the trip history
        if (!SendClearTripHistory(hTripUnit))
        {
            PrintToScreen("Error clearing trip history");
            return;
        }

        // loop through configured points
        for (int point_index = 0; point_index < NUM_TEST_POINTS; ++point_index)
        {
            PrintToScreen("Testing point " + PointToAmpString(testParams, point_index));

            ResultsAtPoint &resultsAtPoint =
                testResults.resultsAtPoint[point_index];

            bool retval = TestAtPoint(hATB, hTripUnit, hKeithley,
                                      testParams.setPointRMSCurrent[point_index],
                                      resultsAtPoint);

            if (retval)
            {
                PrintToScreen("Delaying 1 seconds after testing point...");
                Sleep(1000);
            }
            else
            {
                PrintToScreen("TestAtPoint() failed; aborting short tests");
                return;
            }
        }

        DeterminePassFail(testResults);

        // test fails if trip unit somehow tripped during testing
        CheckForTrip(hTripUnit, testResults);

        PrintTestResults(testResults, testParams);

        PrintToScreen("Short Tests completed in " +
                      std::to_string(NumSecondsElapsed(StartTime)) + " seconds");
    }
}