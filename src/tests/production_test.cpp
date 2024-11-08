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
#include "..\resource\resource.h"

typedef struct _DeviceConnection
{
    HANDLE handle;
    int commPort;
} DeviceConnection;

extern HWND hModelessDlg;
extern DeviceConnection hTripUnit;

namespace ACPRO2_RG
{
    LRESULT CALLBACK ProductionDialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_INITDIALOG:
            // Set the text of the big text label
            SetDlgItemText(hDlg, IDC_BIG_TEXT_LABEL, (LPCTSTR)lParam);
            return true;

        case WM_COMMAND:
            if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
            {
                EndDialog(hDlg, LOWORD(wParam));
                return true;
            }
            break;
        }
        return false;
    }

    static bool AllEquipmentConnected()
    {
        return true;

        /*
        return hKeithley.handle != INVALID_HANDLE_VALUE &&
                hArduino.handle != INVALID_HANDLE_VALUE &&
                RIGOL_DG1000Z_Connected;
        */
    }

    static void SetUIText(std::string text)
    {
        SetDlgItemText(
            hModelessDlg,
            IDC_BIG_TEXT_LABEL,
            text.c_str());
    }

    bool DidItWork(std::string text)
    {
        int ret = MessageBoxA(hModelessDlg, text.c_str(), "Succeed?", MB_YESNO);
        return ret == IDYES;
    }

    bool TryAgain(std::string text)
    {
        int ret = MessageBoxA(hModelessDlg, text.c_str(), "Retry?", MB_ICONERROR | MB_YESNO);
        return ret == IDYES;
    }

    // returns true if func() succeeds
    // returns false if user cancles
    bool OptionallyRetry(bool (*func)(), std::string ErrorStr)
    {
        while (1)
        {
            if (func())
                return true;
            if (!TryAgain(ErrorStr))
                return false;
        }
    }

    void ShowErrorStartOver(std::string ErrorStr)
    {
        MessageBoxA(hModelessDlg, ErrorStr.c_str(), "Error", MB_ICONERROR);
        DisconnectFromTripUnit();
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool TripUnitCalibrated()
    {
        SetUIText("Checking calibration status");
        return DidItWork("Is Trip Unit calibrated?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool CalibrateTripUnit_Attempt()
    {
        SetUIText("Calibrating Trip Unit");
        return DidItWork("Trip Unit Calibration OK?");
    }

    bool CalibrateTripUnit()
    {
        return OptionallyRetry(CalibrateTripUnit_Attempt, "Calibration failed; try again?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool Test1_Attempt()
    {
        SetUIText("Doing Test 1");
        return DidItWork("Test One OK?");
    }

    bool Test1()
    {
        return OptionallyRetry(Test1_Attempt, "Test1 failed; try again?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool Test2_Attempt()
    {
        SetUIText("Doing Test 2");
        return DidItWork("Test One OK?");
    }

    bool Test2()
    {
        return OptionallyRetry(Test1_Attempt, "Test2 failed; try again?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool Test3_Attempt()
    {
        SetUIText("Doing Test 3");
        return DidItWork("Test One OK?");
    }

    bool Test3()
    {
        return OptionallyRetry(Test1_Attempt, "Test3 failed; try again?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    bool ProgramFTDIChip_Attempt()
    {
        SetUIText("Programming the FTDI Chip");
        return DidItWork("Program FTDI Chip OK?");
    }

    bool ProgramFTDIChip()
    {
        return OptionallyRetry(ProgramFTDIChip_Attempt, "Programming FTDI Chip failed; try again?");
    }

    ///////////////////////////////////////////////////////////////////////////////

    void DoProductionLoop()
    {
        while (1)
        {
            Sleep(1000);

            // nothing we can do until all equipment is connected
            if (!AllEquipmentConnected())
            {
                // just hang out forever; user must exit this program
                SetUIText("Cannot continue; equipment not connected.");
                continue;
            }

            // just hang out until we get the trip unit
            if (hTripUnit.handle == INVALID_HANDLE_VALUE)
            {
                SetUIText("Waiting for Trip Unit");
                continue;
            }

            // program the trip unit FTDI trip
            if (!ProgramFTDIChip())
            {
                ShowErrorStartOver("Error programming FTDI chip");
                continue;
            }

            // is trip unit calibrated?
            if (!TripUnitCalibrated())
            {
                // calibrate it
                if (!CalibrateTripUnit())
                {
                    ShowErrorStartOver("Error calibrating trip unit");
                    continue;
                }
            }

            if (!Test1())
            {
                ShowErrorStartOver("Error Running Test 1");
                continue;
            }

            if (!Test2())
            {
                ShowErrorStartOver("Error Running Test 2");
                continue;
            }

            if (!Test3())
            {
                ShowErrorStartOver("Error Running Test 3");
                continue;
            }

            // we are done!
            MessageBoxA(hModelessDlg, "Test Sequence Complete; disconnect trip unit", "Nice!", MB_OK);
            DisconnectFromTripUnit();
        }
    }

}
