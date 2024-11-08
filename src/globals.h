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

// global variables
// no pragma once... we want to include this only from autocal_lite.cpp

#include <unordered_map>

#define WM_CALIBRATION_UPDATE_MSG (WM_APP + 1) // Custom message for updating UI

#define ID_EDITCHILD 100
#define ID_TOOL_BAR 101
#define ID_TIMER_1 1
#define ID_TIMER_2 2 //

const char g_szClassName[] = "urc_autocal_lite";

std::vector<int> commPorts;
TripUnitType tripUnitType;

// represents a connection to a device that uses a comm port
typedef struct _DeviceConnection
{
	HANDLE handle;
	int commPort;
} DeviceConnection;

DeviceConnection hTripUnit = {INVALID_HANDLE_VALUE, 0};
DeviceConnection hKeithley = {INVALID_HANDLE_VALUE, 0};
DeviceConnection hArduino = {INVALID_HANDLE_VALUE, 0};

int TripUnitFTDIIndex = FTDI::NO_FTDI_INDEX;
bool logData = false;

bool BK_PRECISION_9801_Connected = false;
bool RIGOL_DG1000Z_Connected = false;

//////////////////////////////////////////////////////////////////////////

// this is just stuff having to do with InputDlgProc()

std::string inputBoxPrompt; // prompt show in the dialog box
std::unordered_map<std::string, std::string> lastInputValues;

#define InputBox_ReturnValue_Cancel 0
#define InputBox_ReturnValue_OK 1

int InputBox_ReturnValue;
float ReturnedInputValue_Float;
std::string ReturnedInputValue_String;

////////////////////////////////////////////////////////////////////////////

HWND hwndMain;
HWND hwndEdit;
HWND hwndStatusBar;
HWND hwndManualModeDialog;

int ATB_Phase_Selected = -1;
long double rValue_For_ATB_Phase;

enum ConnectionEnum
{
	DEVICE_TRIP_UNIT,
	DEVICE_KEITHLEY,
	DEVICE_RIGOL,
	DEVICE_ARDUINO
};

char tu_serial[12] = {0};

std::string database_file;

ACPRO2_RG::ArbitraryCalibrationParams arbitraryRCCalibrationParams;
ACPRO2_RG::FullCalibrationParams fullRCCalibrationParams;

// we have already made sure this directory exists
std::string iniFile = "C:\\urc\\apps\\autocal_rc\\autocal_rc.ini";
