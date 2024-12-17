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

#include "autocal_rc.hpp"
#include "resource\resource.h"
#include "Dbt.h"

#include <CommCtrl.h>
#include <vector>
#include <fstream>
#include <string>
#include <crtdbg.h>
#include <iomanip>
#include <sstream>

#include "util\ld.hpp" // local display
#include "devices\arduino.hpp"
#include "globals.h"
#include "util\settings.hpp"
#include "tests\lt_trip_test_rc.hpp"
#include "tests\st_trip_test_rc.hpp"
#include "tests\inst_trip_test_rc.hpp"
#include "tests\gf_trip_test_rc.hpp"

// undefine the UNICODE macro, so that we can use the non-unicode versions of the windows API
// (all the strings we use are ASCII for this program)
#undef UNICODE

// 1.0 - original
constexpr float AUTOCAL_RC_VERSION = 1.0;

// private function prototypes
bool ProgramTripUnitEEProm(const char *mfg, const char *description);
void Shutdown();
std::string TripUnitTypeToString(TripUnitType type);
static void EnumeratePorts();
HWND CreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE hinst, int cParts);
void SetStatusBarText(const char *msg);
void StatusBarReady();
void SetConnectionStatus(ConnectionEnum connection, bool isConnected);
HANDLE GetHandleForTripUnit();
static void UpdateRemoteSoftkeys(HANDLE hTripUnit, HWND hDlg, uint8_t Key);
bool GetInputValue(const std::string &prompt, int defaultValue);
void PrintConnectionStatus();
static void BuildCurrentVoltageScreen(HANDLE hTripUnit, HWND hDlg);
static void stepBK9801(HANDLE hTripUnit, HANDLE hKeithley, bool do50hz);
static void stepRIGOL(HANDLE hTripUnit, HANDLE hKeithley, bool do50hz);
static void CenterDialog(HWND hwnd);

// global variables
static std::ofstream log_file;
bool ArduinoAbortTimingTest = false;
bool RigolDualChannelMode = false;

static void OpenURL(const char *url)
{
	ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
}

static void ClearTextBox()
{
	SendMessageA(hwndEdit, WM_SETTEXT, 0, (LPARAM) "");
}

// returns -1 if the value is not an int
int readIntValueFromINIFile(const char *filePath, const char *section, const char *key)
{

	char value_buf[255];
	GetPrivateProfileStringA(section, key, "-1", value_buf, sizeof(value_buf), filePath);
	int retval = -1;

	// Convert the string to an integer
	try
	{
		retval = std::stoi(value_buf);
	}
	catch (std::exception &)
	{
		// If the conversion fails, return -1
		retval = -1;
	}

	return retval;
}

// returns false if the value is not a boolean
bool readBoolValueFromINIFile(const char *filePath, const char *section, const char *key)
{

	char value_buf[255];
	GetPrivateProfileStringA(section, key, "false", value_buf, sizeof(value_buf), filePath);
	bool retval = false;

	// Convert the string to a boolean
	try
	{
		std::string s = std::string(value_buf);
		if (s == "TRUE" || s == "true" || s == "True")
			retval = true;
	}
	catch (std::exception &)
	{
		// If the conversion fails, return false
		retval = false;
	}

	return retval;
}

std::string ReadStringFromINIFile(
	const std::string &filePath,
	const std::string &section,
	const std::string &key)
{
	char value_buf[255] = {0};
	GetPrivateProfileStringA(
		section.c_str(), key.c_str(), "", value_buf, sizeof(value_buf), filePath.c_str());

	return std::string(value_buf);
}

void ReadInConfigurationFile()
{
	database_file = ReadStringFromINIFile(iniFile.c_str(), "database", "file");
}

void WriteConfigurationFile()
{
}

// everybody should call this to post a message saying to update the UI
void PrintToScreen(const std::string &msg)
{
	// Dynamically allocate a copy of the message string to send it to the UI thread
	std::string *message = new std::string(msg);

	// Post the message to the UI thread
	// The LPARAM will be a pointer to the dynamically allocated std::string
	PostMessage(hwndMain, WM_CALIBRATION_UPDATE_MSG, 0, reinterpret_cast<LPARAM>(message));

	// log everything we print to the screen to the log file as well
	if (log_file.is_open())
	{
		log_file.write(msg.c_str(), msg.size());
		log_file.write("\n", 1);
	}
}

// prints out a message to our edit box, which is a scrollable list of all the
// text we've entered
static void _PrintToScreen(const std::string &message)
{
	std::string messageToPrint;

	// append a newline to the message, unless it ends with our
	// special sequence of two pipe characters
	if (message.rfind("||") != message.length() - 2)
		messageToPrint = message + "\r\n";
	else
	{
		// don't add a newline, but remove the special sequence
		messageToPrint = message;
		messageToPrint.erase(messageToPrint.length() - 2);
	}

	SendMessageA(hwndEdit, EM_SETSEL, 0, -1);									// Select all
	SendMessageA(hwndEdit, EM_SETSEL, -1, -1);									// Unselect and stay at the end pos
	SendMessageA(hwndEdit, EM_REPLACESEL, 0, (LPARAM)(messageToPrint.c_str())); // append text to current pos and scroll down
}

void scr_printf(const char *format, ...)
{
	char buffer[1024];
	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);
	va_end(args);

	PrintToScreen(buffer);
}

void ExitWithError(const std::string &message)
{
	std::string ToDisplay = message + "; terminating...";
	MessageBoxA(hwndMain, ToDisplay.c_str(), "Error", MB_ICONERROR);
	exit(1);
}

// programs the mfg and description of the trip unit's FTDI chip
// note: it is necessary to temporarily close the com port
// 	     while doing this, because we have to open the device
//		 using FTD2XX.DLL
bool ProgramTripUnitEEProm(const char *mfg, const char *description)
{

	bool retval;

	PrintToScreen("Programming Trip Unit EEPROM with mfg: " + std::string(mfg) + " and description: " + std::string(description));
	return true;

	// first just close the comm port, so we can open it up with FT_Open()
	CloseCommPort(&hTripUnit.handle);

	TripUnitFTDIIndex = FTDI::FindTripUnitFTDIIndex(hTripUnit.commPort);
	retval = (FTDI::NO_FTDI_INDEX != TripUnitFTDIIndex);

	if (retval)
	{
		retval = (FTDI::ProgramEEPROM(mfg, description, TripUnitFTDIIndex));
	}

	if (retval)
	{
		// read back what we just wrote
		// make sure it was programmed correctly
		retval = FTDI::VerifyEEPROM(mfg, description, TripUnitFTDIIndex);

		if (!retval)
		{
			PrintToScreen("EEPROM verification failed");
		}
	}

	// now just reconnect to the trip unit via comm port, no matter what
	retval = ConnectTripUnit(&hTripUnit.handle, hTripUnit.commPort, &tripUnitType);

	return retval;
}

/*
bool DumpFTDIEEprom(const char *mfg, const char *description)
{

	bool retval;

	// first just close the comm port, so we can open it up with FT_Open()
	CloseCommPort(&hTripUnit.handle);

	TripUnitFTDIIndex = FTDI::FindTripUnitFTDIIndex(hTripUnit.commPort);
	retval = (FTDI::NO_FTDI_INDEX != TripUnitFTDIIndex);

	if (retval)
	{
		retval = (FTDI::ProgramEEPROM(mfg, description, TripUnitFTDIIndex));
	}

	if (retval)
	{
		// read back what we just wrote
		// make sure it was programmed correctly
		retval = FTDI::VerifyEEPROM(mfg, description, TripUnitFTDIIndex);

		if (!retval)
		{
			PrintToScreen("EEPROM verification failed");
		}
	}

	// now just reconnect to the trip unit via comm port, no matter what
	retval = ConnectTripUnit(&hTripUnit.handle, hTripUnit.commPort, &tripUnitType);

	return retval;
}
*/

bool ProgramTripUnitSerialNumber()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return false;
	}

	if (strlen(tu_serial) != 10)
	{
		PrintToScreen("Serial numbers must be 10 characters long");
		return false;
	}

	if (!SetSerialNumber(hHandleForTripUnit, tu_serial))
	{
		PrintToScreen("Failed to set serial numbers");
		return false;
	}

	// read back the serial number

	char new_tu_serial[12] = {0};

	if (!GetSerialNumber(hHandleForTripUnit, new_tu_serial, sizeof(new_tu_serial)))
	{
		PrintToScreen("Failed to read back serial number");
		return false;
	}

	if (strcmp(tu_serial, new_tu_serial) != 0)
	{
		PrintToScreen("Failed to verify serial numbers");
		return false;
	}

	PrintToScreen("Trip Unit Serial number set to: " + std::string(tu_serial));
	return true;
}

// returns true if "port" is not already open
static bool CommPortIsAvailable(int port)
{
	auto handles = {hTripUnit, hKeithley, hArduino};

	for (auto &device : handles)
	{
		if (device.commPort == port && device.handle != INVALID_HANDLE_VALUE)
			return false;
	}

	return true;
}

void FindEquipment(bool findKeithley, bool findTripUnit, bool findArduino)
{
	SetStatusBarText("Searching for devices...");
	EnumeratePorts();

	if (findKeithley)
	{
		hKeithley.commPort = 0;
		for (int i = 0; i < commPorts.size(); i++)
		{
			if (CommPortIsAvailable(commPorts[i]))
			{
				PrintToScreen("searching for Keithley on com: " + std::to_string(commPorts[i]));
				if (KEITHLEY::Connect(&hKeithley.handle, commPorts[i]))
				{
					hKeithley.commPort = commPorts[i];
					PrintToScreen("Keithley found on comm port " + std::to_string(commPorts[i]));
					break;
				}
				else
				{
					CloseCommPort(&hKeithley.handle);
				}
			}
		}
		if (0 == hKeithley.commPort)
			PrintToScreen("Keithley not found");
	}

	if (findTripUnit)
	{
		hTripUnit.commPort = 0;
		for (int i = 0; i < commPorts.size(); i++)
		{
			if (CommPortIsAvailable(commPorts[i]))
			{
				PrintToScreen("searching for trip unit on com: " + std::to_string(commPorts[i]));
				if (ConnectTripUnit(&hTripUnit.handle, commPorts[i], &tripUnitType))
				{
					hTripUnit.commPort = commPorts[i];
					PrintToScreen("Trip Unit found on comm port " + std::to_string(commPorts[i]));
					break;
				}
				else
				{
					CloseCommPort(&hTripUnit.handle);
				}
			}
		}

		if (0 == hTripUnit.commPort)
			PrintToScreen("Trip Unit not found");
	}

	if (findArduino)
	{
		hArduino.commPort = 0;

		for (int i = 0; i < commPorts.size(); i++)
		{
			if (CommPortIsAvailable(commPorts[i]))
			{
				PrintToScreen("searching for Arduino on com: " + std::to_string(commPorts[i]));
				if (ARDUINO::Connect(&hArduino.handle, commPorts[i]))
				{
					hArduino.commPort = commPorts[i];
					PrintToScreen("Arduino found on comm port " + std::to_string(commPorts[i]));
					break;
				}
				else
				{
					CloseCommPort(&hArduino.handle);
				}
			}
		}
	}

	PrintConnectionStatus();
	StatusBarReady();
}

// ignore errors, because we are shutting down
void Shutdown()
{
	WriteConfigurationFile();
	DB::Disconnect();

	CloseCommPort(&hTripUnit.handle);
	CloseCommPort(&hKeithley.handle);
}

bool SendGetBaudRate(HANDLE hComm)
{
	return SendURCCommand(hComm, MSG_GET_BAUD_RATES, ADDR_DISP_LOCAL, ADDR_CAL_APP);
}

bool SendSetBaudRate(HANDLE hComm, uint32_t baud)
{

	URCMessageUnion msg;
	memset(&msg, 0, sizeof(msg));

	msg.msgHdr.Type = MSG_SET_BAUD_RATE;
	msg.msgHdr.Version = PROTOCOL_VERSION;
	msg.msgHdr.Length = 4;
	msg.msgHdr.Seq = SequenceNumber();
	msg.msgHdr.Dst = ADDR_DISP_LOCAL;
	msg.msgHdr.Src = ADDR_CAL_APP;

	msg.msgSetBaudRate.BaudRate = baud;

	msg.msgHdr.ChkSum = CalcChecksum((uint8_t *)&msg, sizeof(msg.msgHdr) + msg.msgHdr.Length);

	return WriteToCommPort(hComm, (uint8_t *)&msg, sizeof(msg.msgHdr) + msg.msgHdr.Length);
}

// this only needs to run if we are connected to an AC-PRO2_v4 Trip Unit directly
void SetHighestTripUnitBaudRate()
{
	URCMessageUnion rsp = {0};
	URCMessageUnion msg = {0};
	bool retval;

	HANDLE hHandleForTripUnit;

	_ASSERT(hTripUnit.handle != INVALID_HANDLE_VALUE);

	PrintToScreen("enumerating trip unit baud rates...");

	SendGetBaudRate(hTripUnit.handle);
	retval = GetURCResponse(hTripUnit.handle, &rsp) &&
			 VerifyMessageIsOK(&rsp, MSG_RSP_BAUD_RATES, sizeof(MsgRspBaudRates) - sizeof(MsgHdr));

	if (!retval)
	{
		PrintToScreen("error getting trip unit baud rate");
		return;
	}

	PrintToScreen("received supported baud rates from trip unit:");
	for (int i = 0; i < 8; i++)
	{
		// PrintToScreen(std::to_string(rsp.msgRspBaudRates.BaudRates[i]));
	}

	// just pick the fastest one...
	uint32_t HighestBaud = rsp.msgRspBaudRates.BaudRates[0];

	PrintToScreen("setting trip unit baud rate to " + std::to_string(HighestBaud));

	SendSetBaudRate(hTripUnit.handle, HighestBaud);
	retval = GetURCResponse(hTripUnit.handle, &rsp) &&
			 VerifyMessageIsOK(&rsp, MSG_ACK, sizeof(MsgACK) - sizeof(MsgHdr));

	if (!retval)
	{
		PrintToScreen("error setting trip unit baud rate");
		return;
	}
	PrintToScreen("successfully set trip unit baud rate to: " + std::to_string(HighestBaud));

	PrintToScreen("setting local com port to to: " + std::to_string(HighestBaud));
	retval = SetLocalBaudRate(hTripUnit.handle, HighestBaud);

	if (!retval)
	{
		PrintToScreen("error setting local baud rate\n");
		return;
	}

	PrintToScreen("successfully set local com port to " + std::to_string(HighestBaud));
}

std::string TripUnitTypeToString(TripUnitType type)
{
	switch (type)
	{
	case TripUnitType::AC_PRO2_GFO:
		return "AC_PRO2_GFO";
	case TripUnitType::AC_PRO2_NW:
		return "AC_PRO2_NW";
	case TripUnitType::AC_PRO2_V4:
		return "AC_PRO2_V4";
	case TripUnitType::AC_PRO2_RC:
		return "AC_PRO2_RC";
	default:
		return "<unknown trip unit type>";
	}
}

// returns NaN if the string is not a float
float FloatFromString(const std::string &input_text)
{
	float retval;

	std::istringstream iss;
	// convert the input string to a float
	iss.str(input_text);
	iss >> std::noskipws >> retval;

	// Check if the entire string was consumed and if there was no error
	if (iss.eof() && !iss.fail())
	{
		// input_text is a float; stored in retval
	}
	else
	{
		// input_text is not a float
		retval = std::numeric_limits<float>::quiet_NaN();
	}

	return retval;
}

// this sets the global variable ReturnedInputValue_Int to the value
// the user types into the input box, as long as what they
// type in is numeric.
// otherwise, if non numeric, sets ReturnedInputValue_Int to -1
// or if the user presses cancel, sets ReturnedInputValue_Int to -1
// also: sets ReturnedInputValue_String to the string the user types in
INT_PTR CALLBACK InputDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	char input_text[1024] = {0};
	int xPos;
	int yPos;
	RECT rc;

	switch (message)
	{
	case WM_INITDIALOG:
		GetWindowRect(hwnd, &rc);

		xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
		yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

		SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
		SetDlgItemTextA(hwnd, IDC_PROMPT, inputBoxPrompt.c_str());
		SetFocus(GetDlgItem(hwnd, IDC_EDIT1));

		// default to using the same value as last time
		SetDlgItemTextA(hwnd, IDC_EDIT1, lastInputValues[inputBoxPrompt].c_str());

		// select all the text, so if the user starts typing, it will replace the current value
		SendDlgItemMessage(hwnd, IDC_EDIT1, EM_SETSEL, 0, -1);
		return false; // return false meaning we set the focus ourselves

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
		{
			GetDlgItemTextA(hwnd, IDC_EDIT1, input_text, sizeof(input_text));

			// save the text they typed in, associated with the inputbox prompt.
			// that way, each type of dialogbox will have its own last value

			lastInputValues[inputBoxPrompt] = input_text;

			// return what the user typed in as a string
			ReturnedInputValue_String = std::string(input_text);

			if (ReturnedInputValue_String.size() != 0)
			{
				// return what the user typed in as an int (used in lots of places)
				ReturnedInputValue_Float = FloatFromString(input_text);
				InputBox_ReturnValue = InputBox_ReturnValue_OK;
			}
			else
			{
				// return what the user typed in as an int (used in lots of places)
				ReturnedInputValue_Float = std::numeric_limits<float>::quiet_NaN();
				InputBox_ReturnValue = InputBox_ReturnValue_Cancel;
			}

			EndDialog(hwnd, 0);
			return TRUE;
		}
		case IDCANCEL:
		case IDC_BUTTON2:
			ReturnedInputValue_String = "";
			ReturnedInputValue_Float = std::numeric_limits<float>::quiet_NaN();
			InputBox_ReturnValue = InputBox_ReturnValue_Cancel;
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// returns TRUE if the user pressed OK
bool GetInputValue(const std::string &prompt, int defaultValue = -1)
{
	inputBoxPrompt = prompt;
	DialogBox(NULL, MAKEINTRESOURCE(IDD_INPUTBOX), hwndMain, InputDlgProc);
	return (InputBox_ReturnValue == InputBox_ReturnValue_OK);
}

INT_PTR CALLBACK FTDI_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	char mfg[1024] = {0};
	char desc[1024] = {0};

	switch (message)
	{
	case WM_INITDIALOG:
		SetFocus(GetDlgItem(hwnd, IDC_FTDI_MFG));
		return false; // return false meaning we set the focus ourselves
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			GetDlgItemTextA(hwnd, IDC_FTDI_MFG, mfg, sizeof(mfg));
			GetDlgItemTextA(hwnd, IDC_FTDI_DESC, desc, sizeof(desc));
			ProgramTripUnitEEProm(mfg, desc);
			EndDialog(hwnd, 0);
			return TRUE;
		}
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK SerialNum_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		SetDlgItemTextA(hwnd, IDC_TU_SERIAL, tu_serial);
		SetFocus(GetDlgItem(hwnd, IDC_LD_SERIAL));
		return false; // return false meaning we set the focus ourselves
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		{
			GetDlgItemTextA(hwnd, IDC_TU_SERIAL, tu_serial, sizeof(tu_serial));
			if (!ProgramTripUnitSerialNumber())
			{
				MessageBoxA(hwnd, "Failed to program trip unit serial number", "Error", MB_ICONERROR);
			}
			EndDialog(hwnd, 0);
			return TRUE;
		}
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// grab the values from the dialog box and put them into arbitraryRCCalibrationParams
void GatherRCArbitraryCalParams(HWND hwnd)
{
	char tmpBuf[1024] = {0};

	arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RADIO_LOGAIN));
	arbitraryRCCalibrationParams.Do_Channel_A = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_A));
	arbitraryRCCalibrationParams.Do_Channel_B = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_B));
	arbitraryRCCalibrationParams.Do_Channel_C = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_C));
	arbitraryRCCalibrationParams.Do_Channel_N = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_N));

	// Get the handle to the combo box.
	HWND combo_hwnd = GetDlgItem(hwnd, IDC_LB_HIGH_GAIN_TYPE);
	LRESULT tmp = SendMessage(combo_hwnd, LB_GETCURSEL, 0, 0);

	if (CB_ERR != tmp)
		arbitraryRCCalibrationParams.High_Gain_Point = (int)tmp;
	else
	{
		PrintToScreen("Error getting high gain point from drop down, defaulting to 0.5");
		arbitraryRCCalibrationParams.High_Gain_Point = 0;
	}

	arbitraryRCCalibrationParams.send_BK9801_Commands = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_SEND_BK9801));
	arbitraryRCCalibrationParams.send_DG1000Z_Commands = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_SEND_DG1000z));
	arbitraryRCCalibrationParams.Use50HZ = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_USE50Hz));

	arbitraryRCCalibrationParams.valid =
		arbitraryRCCalibrationParams.Do_Channel_A ||
		arbitraryRCCalibrationParams.Do_Channel_B ||
		arbitraryRCCalibrationParams.Do_Channel_C ||
		arbitraryRCCalibrationParams.Do_Channel_N;

	if (!arbitraryRCCalibrationParams.valid)
	{
		PrintToScreen("Error: no channels selected");
	}

	GetDlgItemTextA(hwnd, IDC_RIGOL_V, tmpBuf, sizeof(tmpBuf));
	arbitraryRCCalibrationParams.voltageToCommandRMS = atof(tmpBuf);
	arbitraryRCCalibrationParams.valid = arbitraryRCCalibrationParams.valid && (arbitraryRCCalibrationParams.voltageToCommandRMS >= 0);
}

// populate the dialog box with the values from arbitraryRCCalibrationParams
void ScatterArbitraryCalParams(const HWND hwnd)
{
	HWND hwndComboBox = GetDlgItem(hwnd, IDC_LB_HIGH_GAIN_TYPE);

	SendMessage(GetDlgItem(hwnd, IDC_RADIO_HIGAIN), BM_SETCHECK,
				!arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_RADIO_LOGAIN), BM_SETCHECK,
				arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_CHANNEL_A), BM_SETCHECK,
				arbitraryRCCalibrationParams.Do_Channel_A ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_CHANNEL_B), BM_SETCHECK,
				arbitraryRCCalibrationParams.Do_Channel_B ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_CHANNEL_C), BM_SETCHECK,
				arbitraryRCCalibrationParams.Do_Channel_C ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_CHANNEL_N), BM_SETCHECK,
				arbitraryRCCalibrationParams.Do_Channel_N ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(hwndComboBox, LB_SETCURSEL, arbitraryRCCalibrationParams.High_Gain_Point, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_SEND_BK9801), BM_SETCHECK,
				arbitraryRCCalibrationParams.send_BK9801_Commands ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_SEND_DG1000z), BM_SETCHECK,
				arbitraryRCCalibrationParams.send_DG1000Z_Commands ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_USE50Hz), BM_SETCHECK,
				arbitraryRCCalibrationParams.Use50HZ ? BST_CHECKED : BST_UNCHECKED, 0);

	SetDlgItemTextA(hwnd, IDC_RIGOL_V, std::to_string(arbitraryRCCalibrationParams.voltageToCommandRMS).c_str());
}

// i removed all the logic about trying to enable/disable controls
// for now, everything is just always enabled
INT_PTR CALLBACK RC_ArbitraryCal_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// populate the high gain drop down
	// Get the handle to the combo box.
	HWND hwndComboBox;

	switch (message)
	{
	case WM_INITDIALOG:
		CenterDialog(hwnd);
		hwndComboBox = GetDlgItem(hwnd, IDC_LB_HIGH_GAIN_TYPE);
		SendMessage(hwndComboBox, LB_ADDSTRING, 0, (LPARAM) "HI_GAIN_0_5");
		SendMessage(hwndComboBox, LB_ADDSTRING, 0, (LPARAM) "HI_GAIN_1_0");
		SendMessage(hwndComboBox, LB_ADDSTRING, 0, (LPARAM) "HI_GAIN_1_5");
		SendMessage(hwndComboBox, LB_ADDSTRING, 0, (LPARAM) "HI_GAIN_2_0");

		ScatterArbitraryCalParams(hwnd);
		SetFocus(GetDlgItem(hwnd, IDC_RADIO_HIGAIN));
		return false; // return false meaning we set the focus ourselves
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO_LOGAIN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				// when we go to low gain, default the rigol to 5Vrms
				// SetDlgItemTextA(hwnd, IDC_RIGOL_V, "5.0");
			}
			break;
		case IDC_RADIO_HIGAIN:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				// high gain is selected, so enable the high gain list box
				EnableWindow(GetDlgItem(hwnd, IDC_LB_HIGH_GAIN_TYPE), TRUE);
				// SetDlgItemTextA(hwnd, IDC_RIGOL_V, "0.5");
			}
			break;
		case IDC_CHECK_SEND_DG1000z:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				// The IDC_CHECK_SEND_DG1000z checkbox was clicked.
				// Get the new check state.
				BOOL isChecked = (BST_CHECKED == SendMessage(GetDlgItem(hwnd, IDC_CHECK_SEND_DG1000z), BM_GETCHECK, 0, 0));
				// Enable or disable the IDC_CHECK_USE50Hz checkbox based on the new check state.

				/*
				EnableWindow(GetDlgItem(hwnd, IDC_CHECK_USE50Hz), isChecked);
				EnableWindow(GetDlgItem(hwnd, IDC_RIGOL_V), isChecked);
				*/
			}
			break;
		case ID_DO_CAL:
			// here we have to grab all the data from the screen
			GatherRCArbitraryCalParams(hwnd);
			EndDialog(hwnd, 0);
			return TRUE;
		case IDCANCEL:
			arbitraryRCCalibrationParams = {0};
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// grab the values from the dialog box and put them into the fullRCCalibrationParams struct
void GatherRCFullCalibrationParams(HWND hwnd)
{
	char tmpBuf[1024] = {0};

	fullRCCalibrationParams.do50hz = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_DO50hz));
	fullRCCalibrationParams.do60hz = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_DO60hz));
	fullRCCalibrationParams.useRigolDualChannelMode = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_USE_RIGOL_DUALCHANNEL));

	// 0.5
	GetDlgItemTextA(hwnd, IDC_V1, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.hi_gain_voltages_rms[0] = atof(tmpBuf);
	GetDlgItemTextA(hwnd, IDC_V2, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.lo_gain_voltages_rms[0] = atof(tmpBuf);

	// 1.0
	GetDlgItemTextA(hwnd, IDC_V3, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.hi_gain_voltages_rms[1] = atof(tmpBuf);
	GetDlgItemTextA(hwnd, IDC_V4, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.lo_gain_voltages_rms[1] = atof(tmpBuf);

	// 1.5
	GetDlgItemTextA(hwnd, IDC_V5, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.hi_gain_voltages_rms[2] = atof(tmpBuf);
	GetDlgItemTextA(hwnd, IDC_V6, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.lo_gain_voltages_rms[2] = atof(tmpBuf);

	// 2.0
	GetDlgItemTextA(hwnd, IDC_V7, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.hi_gain_voltages_rms[3] = atof(tmpBuf);
	GetDlgItemTextA(hwnd, IDC_V8, tmpBuf, sizeof(tmpBuf));
	fullRCCalibrationParams.lo_gain_voltages_rms[3] = atof(tmpBuf);

	// figure out which voltage source they selected
	HWND combo_hwnd = GetDlgItem(hwnd, IDC_VOLTAGE_SOURCE);
	LRESULT tmp = SendMessage(combo_hwnd, CB_GETCURSEL, 0, 0);

	if (CB_ERR == tmp)
	{
		PrintToScreen("Error getting voltage source from drop down, defaulting to rigol dg1000z");
		fullRCCalibrationParams.use_rigol_dg1000z = true;
		fullRCCalibrationParams.use_bk_precision_9801 = false;
	}
	else
	{
		if (0 == tmp)
		{
			fullRCCalibrationParams.use_rigol_dg1000z = false;
			fullRCCalibrationParams.use_bk_precision_9801 = true;
		}
		else
		{
			fullRCCalibrationParams.use_rigol_dg1000z = true;
			fullRCCalibrationParams.use_bk_precision_9801 = false;
		}
	}
}

bool CheckForNumericFieldGreaterThanZero(HWND hwnd, int ID)
{

	char tmpBuf[1024] = {0};
	char *end;
	double value;
	bool retval;

	GetDlgItemTextA(hwnd, ID, tmpBuf, sizeof(tmpBuf));
	value = std::strtod(tmpBuf, &end);
	retval = (end != tmpBuf && *end == '\0');

	if (retval)
	{
		retval = (value >= 0.0);
	}

	if (!retval)
	{
		SetFocus(GetDlgItem(hwnd, ID));
		return false;
	}

	return true;
}

// check the values on the screen are all OK before letting the user proceed
bool VerifyRCFullCalibrationParams(HWND hwnd)
{
	bool retval;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V1);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V2);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V3);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V4);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V5);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V6);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V7);
	if (!retval)
		return false;

	retval = CheckForNumericFieldGreaterThanZero(hwnd, IDC_V8);
	if (!retval)
		return false;

	bool tmp =
		(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_DO50hz)) ||
		(BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_DO60hz));

	if (!tmp)
	{
		MessageBoxA(hwnd, "You must select either 50Hz or 60Hz", "Error", MB_ICONERROR);
		return false;
	}

	return retval;
}

std::string formatFloat(float value, int precision)
{
	std::ostringstream stream;
	stream << std::fixed << std::setprecision(precision) << value;
	return stream.str();
}

// populate the dialog box with the values from fullRCCalibrationParams
void ScatterRCFullCalibrationParams(HWND hwnd)
{
	SendMessage(GetDlgItem(hwnd, IDC_CHECK_DO50hz), BM_SETCHECK,
				fullRCCalibrationParams.do50hz ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_DO60hz), BM_SETCHECK,
				fullRCCalibrationParams.do60hz ? BST_CHECKED : BST_UNCHECKED, 0);

	SendMessage(GetDlgItem(hwnd, IDC_CHECK_USE_RIGOL_DUALCHANNEL), BM_SETCHECK,
				fullRCCalibrationParams.useRigolDualChannelMode ? BST_CHECKED : BST_UNCHECKED, 0);

	// 0.5
	SetDlgItemTextA(hwnd, IDC_V1, formatFloat(fullRCCalibrationParams.hi_gain_voltages_rms[0], 3).c_str());
	SetDlgItemTextA(hwnd, IDC_V2, formatFloat(fullRCCalibrationParams.lo_gain_voltages_rms[0], 3).c_str());

	// 1.0
	SetDlgItemTextA(hwnd, IDC_V3, formatFloat(fullRCCalibrationParams.hi_gain_voltages_rms[1], 3).c_str());
	SetDlgItemTextA(hwnd, IDC_V4, formatFloat(fullRCCalibrationParams.lo_gain_voltages_rms[1], 3).c_str());

	// 1.5
	SetDlgItemTextA(hwnd, IDC_V5, formatFloat(fullRCCalibrationParams.hi_gain_voltages_rms[2], 3).c_str());
	SetDlgItemTextA(hwnd, IDC_V6, formatFloat(fullRCCalibrationParams.lo_gain_voltages_rms[2], 3).c_str());

	// 2.0
	SetDlgItemTextA(hwnd, IDC_V7, formatFloat(fullRCCalibrationParams.hi_gain_voltages_rms[3], 3).c_str());
	SetDlgItemTextA(hwnd, IDC_V8, formatFloat(fullRCCalibrationParams.lo_gain_voltages_rms[3], 3).c_str());

	// populate the combo box for which voltage source was selected
	HWND hwndComboBox = GetDlgItem(hwnd, IDC_VOLTAGE_SOURCE);

	SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM) "rigol dg1000z");
	SendMessage(hwndComboBox, CB_ADDSTRING, 0, (LPARAM) "bk precision 9801");

	if (fullRCCalibrationParams.use_rigol_dg1000z)
		SendMessage(hwndComboBox, CB_SETCURSEL, 1, 0);
	else
		SendMessage(hwndComboBox, CB_SETCURSEL, 0, 0);
}

static void CenterDialog(HWND hwnd)
{
	RECT rc;
	GetWindowRect(hwnd, &rc);

	int xPos = (GetSystemMetrics(SM_CXSCREEN) - rc.right) / 2;
	int yPos = (GetSystemMetrics(SM_CYSCREEN) - rc.bottom) / 2;

	SetWindowPos(hwnd, 0, xPos, yPos, 0, 0, SWP_NOZORDER | SWP_NOSIZE);
}

INT_PTR CALLBACK RC_FULLCAL_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{

	switch (message)
	{
	case WM_INITDIALOG:
		CenterDialog(hwnd);
		ScatterRCFullCalibrationParams(hwnd);
		SetFocus(GetDlgItem(hwnd, IDC_V1));
		return false; // return false meaning we set the focus ourselves

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		// default here refers to the "default" button on the screen
		case ID_DEFAULT:
			ACPRO2_RG::setDefaultRCFullCalParams(fullRCCalibrationParams);
			ScatterRCFullCalibrationParams(hwnd);
			return TRUE;
		case IDOK:
			if (VerifyRCFullCalibrationParams(hwnd))
			{
				GatherRCFullCalibrationParams(hwnd);
				fullRCCalibrationParams.valid = true;
				EndDialog(hwnd, 0);
			}
			else
			{
				MessageBoxA(hwnd, "Invalid input", "Error", MB_ICONERROR);
			}
			return TRUE;
		case IDCANCEL:
			fullRCCalibrationParams = {0};
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// void PrintCalMatrix(CalibrationResults &calResults)
void PrintCalMatrix(uint16_t calChannels)
{
	PrintToScreen("-----------------------------------------------------");
	PrintToScreen("A_HI | A_LO | B_HI | B_LO | C_HI | C_LO | N_HI | N_LO");

	std::string TmpStr = "[ ]  | [ ]  | [ ]  | [ ]  | [ ]  | [ ]  | [ ]  | [ ]";

	if (calChannels & 1)
		TmpStr[1] = 'X';
	if (calChannels & 2)
		TmpStr[8] = 'X';
	if (calChannels & 4)
		TmpStr[15] = 'X';
	if (calChannels & 8)
		TmpStr[22] = 'X';
	if (calChannels & 16)
		TmpStr[29] = 'X';
	if (calChannels & 32)
		TmpStr[36] = 'X';
	if (calChannels & 64)
		TmpStr[43] = 'X';
	if (calChannels & 128)
		TmpStr[50] = 'X';

	PrintToScreen(TmpStr);

	PrintToScreen("-----------------------------------------------------");
}

void GatherParamsAndDoACPro2v4Cal(HWND hwnd)
{
	_ASSERT(hTripUnit.handle != INVALID_HANDLE_VALUE);

	int calibrationCommand;
	int RMSIndex;
	CalibrationResults calResults;

	// Buffer to hold the text from the edit box
	char szBuffer[256];
	// Variable to hold the converted value
	uint16_t RMSCurrentValue = 0;

	// Get the text from the edit box
	GetDlgItemText(hwnd, IDC_TestSet_AmpsRMS, szBuffer, sizeof(szBuffer));

	float tmpFloat = convertToFloat(szBuffer);

	if (std::isnan(tmpFloat))
	{
		MessageBox(hwnd, "Invalid input. Please enter a valid number.", "Error", MB_ICONERROR);
		return;
	}

	if (tmpFloat > 14.0)
	{
		MessageBox(hwnd, "Invalid input. Test set only supports up to 13.xx amps", "Error", MB_ICONERROR);
		return;
	}

	RMSCurrentValue = 1000 * tmpFloat;

	bool DoA = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_A));
	bool DoB = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_B));
	bool DoC = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_C));
	bool DoN = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_CHECK_CHANNEL_N));

	if (!(DoA || DoB || DoC || DoN))
	{
		MessageBox(hwnd, "No channels selected. Please select at least one channel.", "Error", MB_ICONERROR);
		return;
	}

	bool IsHigh = (BST_CHECKED == IsDlgButtonChecked(hwnd, IDC_RADIO_HIGAIN));

	// setup command
	if (IsHigh)
	{
		if (DoA)
			calibrationCommand = _CALIBRATION_REQUEST_IA_HI;
		else if (DoB)
			calibrationCommand = _CALIBRATION_REQUEST_IB_HI;
		else if (DoC)
			calibrationCommand = _CALIBRATION_REQUEST_IC_HI;
		else if (DoN)
			calibrationCommand = _CALIBRATION_REQUEST_IN_HI;
	}
	else
	{
		if (DoA)
			calibrationCommand = _CALIBRATION_REQUEST_IA_LO;
		else if (DoB)
			calibrationCommand = _CALIBRATION_REQUEST_IB_LO;
		else if (DoC)
			calibrationCommand = _CALIBRATION_REQUEST_IC_LO;
		else if (DoN)
			calibrationCommand = _CALIBRATION_REQUEST_IN_LO;
	}

	// setup RMSINdex
	if (IsHigh)
	{
		if (DoA)
			RMSIndex = i_calibrationIaHI;
		else if (DoB)
			RMSIndex = i_calibrationIbHI;
		else if (DoC)
			RMSIndex = i_calibrationIcHI;
		else if (DoN)
			RMSIndex = i_calibrationInHI;
	}
	else
	{
		if (DoA)
			RMSIndex = i_calibrationIaLO;
		else if (DoB)
			RMSIndex = i_calibrationIbLO;
		else if (DoC)
			RMSIndex = i_calibrationIcLO;
		else if (DoN)
			RMSIndex = i_calibrationInLO;
	}

	bool retval;

	retval = ACPRO2_RG::CalibrateChannel(hTripUnit.handle, calibrationCommand, RMSIndex, RMSCurrentValue, &calResults);

	if (!retval)
	{
		MessageBox(hwnd, "MSG_EXE_CALIBRATE_AD failed", "Error", MB_ICONERROR);
		return;
	}

	if (retval)
		retval = ((calResults.CalibratedChannels & calibrationCommand) == calibrationCommand);

	if (!retval)
	{
		MessageBox(hwnd, "Calibration failed; channel does not report being calibrated", "Error", MB_ICONERROR);
		return;
	}

	PrintCalMatrix(calResults.CalibratedChannels);
}

// Global variable to store the original window procedure of the edit control
WNDPROC g_OldEditProc = NULL;

// New window procedure for subclassing the edit control
LRESULT CALLBACK MyEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	LRESULT retVal;
	// Buffer to hold the text from the edit box
	char szBuffer[256] = {0};
	// Variable to hold the converted value
	uint16_t RMSCurrentValue = 0;
	float tmpFloat = 0;

	switch (msg)
	{
	case WM_CHAR:
		// Call the original window procedure for default processing
		retVal = CallWindowProc(g_OldEditProc, hwnd, msg, wParam, lParam);

		// Get the text from the edit box
		GetDlgItemText(hwndManualModeDialog, IDC_TestSet_AmpsRMS, szBuffer, sizeof(szBuffer));
		tmpFloat = convertToFloat(szBuffer);

		if (!std::isnan(tmpFloat))
		{
			// test-set can only go up to 13.xx amps
			if (tmpFloat < 14)
			{
				RMSCurrentValue = 1000 * tmpFloat;
				SetDlgItemText(hwndManualModeDialog, IDC_STATIC4, std::to_string(RMSCurrentValue).c_str());
			}
			else
			{
				SetDlgItemText(hwndManualModeDialog, IDC_STATIC4, "n/a");
			}
		}
		else
			SetDlgItemText(hwndManualModeDialog, IDC_STATIC4, "n/a");

		return retVal;
	default:
		// Call the original window procedure for default processing
		return CallWindowProc(g_OldEditProc, hwnd, msg, wParam, lParam);
	}
}

INT_PTR CALLBACK ManualCal_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// populate the high gain drop down
	// Get the handle to the combo box.
	HWND hwndComboBox;
	HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndManualModeDialog = hwnd;
		CenterDialog(hwnd);
		SendMessage(GetDlgItem(hwnd, IDC_RADIO_HIGAIN), BM_SETCHECK, BST_CHECKED, 0);
		SendMessage(GetDlgItem(hwnd, IDC_CHECK_CHANNEL_A), BM_SETCHECK, BST_CHECKED, 0);

		// subclass IDC_TestSet_AmpsRMS edit text so we can grab its keystrokes to
		// update the screen
		hwndEdit = GetDlgItem(hwnd, IDC_TestSet_AmpsRMS);
		g_OldEditProc = (WNDPROC)SetWindowLongPtr(hwndEdit, GWLP_WNDPROC, (LONG_PTR)MyEditProc);

		return true; // return false meaning we set the focus ourselves
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_SET_TRIP_50:
			if (IDYES == MessageBoxA(hwndMain, "WARNING- this will also modify the trip unit parameters, such as CR-RATIO. Continue?", "Warning", MB_ICONWARNING | MB_YESNO))
			{
				if (!SetupTripUnitForCalibration(hTripUnit.handle, true))
				{
					MessageBoxA(hwndMain, "Failed to set trip unit for 60hz calibration", "Error", MB_ICONERROR);
					return false;
				}

				// for whatever reason, the AC-PRO-2 wants us to do _CALIBRATION_INITIALIZE when we switch freqencies
				if (!ACPRO2_RG::InitCalibration(hTripUnit.handle))
				{
					MessageBoxA(hwndMain, "_CALIBRATION_INITIALIZE failed", "Error", MB_ICONERROR);
					return false;
				}

				MessageBoxA(hwndMain, "Trip unit set for 50hz calibration", "Success", MB_ICONINFORMATION);
			}
			break;
		case ID_SET_TRIP_60:
			if (IDYES == MessageBoxA(hwndMain, "WARNING- this will also modify the trip unit parameters, such as CR-RATIO. Continue?", "Warning", MB_ICONWARNING | MB_YESNO))
			{
				if (!SetupTripUnitForCalibration(hTripUnit.handle, false))
				{
					MessageBoxA(hwndMain, "Failed to set trip unit for 60hz calibration", "Error", MB_ICONERROR);
					return false;
				}

				// for whatever reason, the AC-PRO-2 wants us to do _CALIBRATION_INITIALIZE when we switch freqencies
				if (!ACPRO2_RG::InitCalibration(hTripUnit.handle))
				{
					MessageBoxA(hwndMain, "_CALIBRATION_INITIALIZE failed", "Error", MB_ICONERROR);
					return false;
				}

				MessageBoxA(hwndMain, "Trip unit set for 60hz calibration", "Success", MB_ICONINFORMATION);
			}
			break;
		case ID_DO_CAL: // do the calibration
			// do the calibration
			// this is a blocking call
			GatherParamsAndDoACPro2v4Cal(hwnd);
			break;
		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// perform a full calibration procedure on a ACPRO2-RC trip unit
bool LoopDoFullTripUnitCAL(
	HANDLE hTripUnit, HANDLE hKeithley, const ACPRO2_RG::FullCalibrationParams &params)
{
	for (int i = 0; i < 32; i++)
	{
		PrintToScreen("Starting cal number: " + std::to_string(i + 1));
		ACPRO2_RG::DoFullTripUnitCAL(hTripUnit, hKeithley, params);

		// the trip unit will be rebooted in DoFullTripUnitCAL()
		// so we do not have to do it here.

		// however:
		// it is important that the trip unit does in fact reboot
		// after being calibrated, because otherwise somehow its RMS
		// calculations are not correct

		// for now, just do 60hz
		stepVoltageSource(hTripUnit, hKeithley, params.use_bk_precision_9801, false);
	}

	TurnOffVoltageSource(params.use_bk_precision_9801);

	return true;
}

// functions that start with Async_ execute the function in a separate thread
// NOTE: this has a parameter shouldDoLoopingCal that is normally set to false.
// NOTE: it is there so we can do the calibration in a loop during development (to look for repeatability)
static void Async_FullACPro2_RG(bool shouldDoLoopingCal)
{
	HANDLE hHandleForTripUnit;
	int msgboxID;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (fullRCCalibrationParams.use_rigol_dg1000z)
	{
		if (!RIGOL_DG1000Z_Connected)
		{
			PrintToScreen("Rigol DG1000z AWG not connected");
			return;
		}
	}

	if (fullRCCalibrationParams.use_bk_precision_9801)
	{
		if (!BK_PRECISION_9801_Connected)
		{
			PrintToScreen("BK Precision 9801 not connected");
			return;
		}
	}

	if (tripUnitType != TripUnitType::AC_PRO2_RC)
	{
		msgboxID = MessageBoxA(hwndMain, "Trip Unit is not an AC-PRO-2 RC; click OK to proceed anyway...", "Error", MB_ICONERROR | MB_OKCANCEL);
		if (msgboxID == IDCANCEL)
		{
			PrintToScreen("User cancelled operation");
			return;
		}
		PrintToScreen("Trip Unit is not an AC-PRO-2 RC");
	}

	// these are hard-coded for now
	// i don't think we really need a way to prevent the full call routine from just doing both HIGH and LOW
	fullRCCalibrationParams.doHighGain = true;
	fullRCCalibrationParams.doLowGain = true;

	std::thread thread;

	if (shouldDoLoopingCal)
		thread = std::thread(LoopDoFullTripUnitCAL, hHandleForTripUnit, hKeithley.handle, fullRCCalibrationParams);
	else
		thread = std::thread(ACPRO2_RG::DoFullTripUnitCAL, hHandleForTripUnit, hKeithley.handle, fullRCCalibrationParams);

	thread.detach();
}

// enumerate available comm ports by querying registry
// print comm ports to screen
// populate commPorts vector with available comm ports
static void EnumeratePorts()
{
	PrintToScreen("------------------------------------");
	PrintToScreen("available com ports");
	PrintToScreen("------------------------------------");

	commPorts.clear();

	// Open the serialcomm key
	HKEY key;
	LONG result = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
								"HARDWARE\\DEVICEMAP\\SERIALCOMM",
								0,
								KEY_READ,
								&key);

	// Check for errors
	if (result != ERROR_SUCCESS)
		PrintToScreen("Error opening reg key");

	// Get the number of values in the key
	DWORD num_values;
	result = RegQueryInfoKeyA(key,
							  NULL,
							  NULL,
							  NULL,
							  NULL,
							  NULL,
							  NULL,
							  &num_values,
							  NULL,
							  NULL,
							  NULL,
							  NULL);

	// Check for errors
	if (result != ERROR_SUCCESS)
	{
		PrintToScreen("Error reading reg key");
		return;
	}

	// Iterate over the values in the key
	for (DWORD i = 0; i < num_values; i++)
	{
		// Get the value name and data
		DWORD value_name_size = 256;
		char value_name[256];
		DWORD value_data_size = 256;
		BYTE value_data[256];
		DWORD value_type;
		result = RegEnumValueA(key,
							   i,
							   value_name,
							   &value_name_size,
							   NULL,
							   &value_type,
							   value_data,
							   &value_data_size);

		// Check for errors
		if (result != ERROR_SUCCESS)
		{
			PrintToScreen("Error opening reg key");
			return;
		}

		switch (value_type)
		{
		case REG_SZ:
			PrintToScreen(reinterpret_cast<char *>(value_data));
			// strip off word COM
			// and convert to an int
			// and add to our list of comm ports
			commPorts.push_back(atoi(reinterpret_cast<char *>(value_data) + 3));
			break;
		default:
			break;
		}
	}

	RegCloseKey(key);
}

std::string ConnectedStatus(const DeviceConnection &connection)
{
	if (connection.handle != INVALID_HANDLE_VALUE)
		return std::to_string(connection.commPort);
	else
		return "DISCONNECTED";
}

std::string boolToString(bool b)
{
	return b ? "true" : "false";
}

void PrintConnectionColumns(std::string column1, std::string column2, std::string column3)
{
	PrintToScreen(
		Dots(25, column1) +
		Dots(25, column2) +
		column3);
}

void PrintConnectionStatus()
{
	PrintToScreen("-----------------------------------------------------------");
	PrintToScreen("A U T O C A L   L I T E  C O N N E C T I O N   S T A T U S");
	PrintToScreen("-----------------------------------------------------------");
	PrintToScreen("Device                   Interface                Status");
	PrintToScreen("-----------------------------------------------------------");

	PrintConnectionColumns("Keithley", "Virtual Comm", ConnectedStatus(hKeithley));
	PrintConnectionColumns("Trip Unit", "Virtual Comm", ConnectedStatus(hTripUnit));
	PrintToScreen(Dots(25, "Trip Unit Type") + TripUnitTypeToString(tripUnitType));
	PrintConnectionColumns("Arduino_AutoCAL_Lite", "Virtual Comm", ConnectedStatus(hArduino));
	PrintConnectionColumns("Rigol_DG1000z", "USB-TCM", (RIGOL_DG1000Z_Connected ? "CONNECTED" : "DISCONNECTED"));
	PrintConnectionColumns("BK Precision-9801", "USB-TCM", (BK_PRECISION_9801_Connected ? "CONNECTED" : "DISCONNECTED"));

	PrintToScreen("");

	if (INVALID_HANDLE_VALUE != GetHandleForTripUnit())
		menu_ID_ACPRO2_PRINT_VERSION();

	if (INVALID_HANDLE_VALUE != hArduino.handle)
		menu_ID_ARDUINO_PRINTVERSION();

	PrintToScreen("Rigol Dual Channel Mode: " + boolToString(RigolDualChannelMode));
}

static void PrintStatus(uint32_t status)
{
	PrintToScreen(Tab(1) + Dots(35, "STS_CALIBRATED") + ZeroOrOne((status & STS_CALIBRATED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_COMMISSIONED") + ZeroOrOne((status & STS_COMMISSIONED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_VDM_ATTACHED") + ZeroOrOne((status & STS_VDM_ATTACHED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_VDM_NOT_POWERED") + ZeroOrOne((status & STS_VDM_NOT_POWERED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_ACTUATOR_OPEN") + ZeroOrOne((status & STS_ACTUATOR_OPEN) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_SLUGGISH_BKR") + ZeroOrOne((status & STS_SLUGGISH_BKR) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_INTERNAL_ERR") + ZeroOrOne((status & STS_INTERNAL_ERR) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_POWER_FRON_CT_OR_24V") + ZeroOrOne((status & STS_POWER_FRON_CT_OR_24V) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_FATAL_ERROR") + ZeroOrOne((status & STS_FATAL_ERROR) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_GF_BLOCKED") + ZeroOrOne((status & STS_GF_BLOCKED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_00000400") + ZeroOrOne((status & STS_00000400) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_00000800") + ZeroOrOne((status & STS_00000800) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_TEST_MODE_ACTIVE") + ZeroOrOne((status & STS_TEST_MODE_ACTIVE) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_TEST_MODE_WARNING") + ZeroOrOne((status & STS_TEST_MODE_WARNING) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_TEST_MODE_TIMEOUT") + ZeroOrOne((status & STS_TEST_MODE_TIMEOUT) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_QT_ON") + ZeroOrOne((status & STS_QT_ON) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_BKR_POS_CONTACT_ERR") + ZeroOrOne((status & STS_BKR_POS_CONTACT_ERR) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_00020000") + ZeroOrOne((status & STS_00020000) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_VOLTAGE_NA") + ZeroOrOne((status & STS_VOLTAGE_NA) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_GF_I5T_SUPPORT") + ZeroOrOne((status & STS_GF_I5T_SUPPORT) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_EEPOT_ERROR_WIPER_INST") + ZeroOrOne((status & STS_EEPOT_ERROR_WIPER_INST) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_EEPOT_ERROR_WIPER_QT_I") + ZeroOrOne((status & STS_EEPOT_ERROR_WIPER_QT_I) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_TRIP_UNIT_POWER_UP") + ZeroOrOne((status & STS_TRIP_UNIT_POWER_UP) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_REV_PHASE_CT_POLARITY") + ZeroOrOne((status & STS_REV_PHASE_CT_POLARITY) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_NO_COMMUNICATIONS") + ZeroOrOne((status & STS_NO_COMMUNICATIONS) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_BOOT_STRAP_LOADER") + ZeroOrOne((status & STS_BOOT_STRAP_LOADER) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_SET_USER_SETTINGS") + ZeroOrOne((status & STS_SET_USER_SETTINGS) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_LOW_BATTERY") + ZeroOrOne((status & STS_LOW_BATTERY) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_GF_DEFEATED_BY_INPUT") + ZeroOrOne((status & STS_GF_DEFEATED_BY_INPUT) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_BREAKER_OPENED") + ZeroOrOne((status & STS_BREAKER_OPENED) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_REV_NEUTRAL_CT_POLARITY") + ZeroOrOne((status & STS_REV_NEUTRAL_CT_POLARITY) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_GF_TRIP_TEST_POWER") + ZeroOrOne((status & STS_GF_TRIP_TEST_POWER) > 0));
	PrintToScreen(Tab(1) + Dots(35, "STS_OPEN_CIRCUIT_CT") + ZeroOrOne((status & STS_OPEN_CIRCUIT_CT) > 0));
}

static std::string TimeDate4ToString(const TimeDate4 &date)
{
	return std::to_string(date.Month) + "/" +
		   std::to_string(date.Day) + "/" +
		   std::to_string(date.Year) + " " +
		   std::to_string(date.Hour) + ":" +
		   std::to_string(date.Minute) + ":" +
		   std::to_string(date.Second);
}

static void PrintSystemSettings(const SystemSettings4 &sysSettings)
{
	PrintToScreen(Tab(1) + Dots(35, "Spare00000000") + std::to_string(sysSettings.Spare00000000));
	PrintToScreen(Tab(1) + Dots(35, "SpareFF") + std::to_string(sysSettings.SpareFF));
	PrintToScreen(Tab(1) + Dots(35, "BreakerL") + std::to_string(sysSettings.BreakerL));
	PrintToScreen(Tab(1) + Dots(35, "CTNeutralRating") + std::to_string(sysSettings.CTNeutralRating));
	PrintToScreen(Tab(1) + Dots(35, "CTRating") + std::to_string(sysSettings.CTRating));
	PrintToScreen(Tab(1) + Dots(35, "CTSecondary") + std::to_string(sysSettings.CTSecondary));
	PrintToScreen(Tab(1) + Dots(35, "CTNeutralSecondary") + std::to_string(sysSettings.CTNeutralSecondary));
	PrintToScreen(Tab(1) + Dots(35, "RatioPT") + std::to_string(sysSettings.RatioPT));
	PrintToScreen(Tab(1) + Dots(35, "CoilSensitivity") + std::to_string(sysSettings.CoilSensitivity));
	PrintToScreen(Tab(1) + Dots(35, "SensorType") + std::to_string(sysSettings.SensorType));
	PrintToScreen(Tab(1) + Dots(35, "Frequency") + std::to_string(sysSettings.Frequency));
	PrintToScreen(Tab(1) + Dots(35, "BkrPosCntType") + std::to_string(sysSettings.BkrPosCntType));
	PrintToScreen(Tab(1) + Dots(35, "PowerReversed") + std::to_string(sysSettings.PowerReversed));
	PrintToScreen(Tab(1) + Dots(35, "AutoPolarityCT") + std::to_string(sysSettings.AutoPolarityCT));
	PrintToScreen(Tab(1) + Dots(35, "TripUnitMode") + std::to_string(sysSettings.TripUnitMode));
	PrintToScreen(Tab(1) + Dots(35, "TypePT") + std::to_string(sysSettings.TypePT));
	PrintToScreen(Tab(1) + Dots(35, "HPCdevice") + std::to_string(sysSettings.HPCdevice));
	PrintToScreen(Tab(1) + Dots(35, "ChangeSource") + std::to_string(sysSettings.ChangeSource));
	PrintToScreen(Tab(1) + Dots(35, "LastChanged") + TimeDate4ToString(sysSettings.LastChanged));
}

static void PrintDeviceSettings(const DeviceSettings4 &devSettings)
{
	PrintToScreen(Tab(1) + Dots(35, "LTPickupx10") + std::to_string(devSettings.LTPickup));
	PrintToScreen(Tab(1) + Dots(35, "LTDelayx10") + std::to_string(devSettings.LTDelay));
	PrintToScreen(Tab(1) + Dots(35, "LTThermMem") + std::to_string(devSettings.LTThermMem));
	PrintToScreen(Tab(1) + Dots(35, "LTEnabled") + std::to_string(devSettings.LTEnabled));
	PrintToScreen(Tab(1) + Dots(35, "STPickup") + std::to_string(devSettings.STPickup));
	PrintToScreen(Tab(1) + Dots(35, "STDelay") + std::to_string(devSettings.STDelay));
	PrintToScreen(Tab(1) + Dots(35, "STI2T") + std::to_string(devSettings.STI2T));
	PrintToScreen(Tab(1) + Dots(35, "STEnabled") + std::to_string(devSettings.STEnabled));
	PrintToScreen(Tab(1) + Dots(35, "GFPickup") + std::to_string(devSettings.GFPickup));
	PrintToScreen(Tab(1) + Dots(35, "GFDelay") + std::to_string(devSettings.GFDelay));
	PrintToScreen(Tab(1) + Dots(35, "GFI2T") + std::to_string(devSettings.GFI2T));
	PrintToScreen(Tab(1) + Dots(35, "GFType") + std::to_string(devSettings.GFType));
	PrintToScreen(Tab(1) + Dots(35, "GFI2TAmps") + std::to_string(devSettings.GFI2TAmps));
	PrintToScreen(Tab(1) + Dots(35, "QTInstPickup") + std::to_string(devSettings.QTInstPickup));
	PrintToScreen(Tab(1) + Dots(35, "InstantPickup") + std::to_string(devSettings.InstantPickup));
	PrintToScreen(Tab(1) + Dots(35, "InstantEnabled") + std::to_string(devSettings.InstantEnabled));
	PrintToScreen(Tab(1) + Dots(35, "GFQTPickup") + std::to_string(devSettings.GFQTPickup));
	PrintToScreen(Tab(1) + Dots(35, "GFQTType") + std::to_string(devSettings.GFQTType));
	PrintToScreen(Tab(1) + Dots(35, "GFQTSpare8") + std::to_string(devSettings.GFQTSpare8));
	PrintToScreen(Tab(1) + Dots(35, "UBPickup") + std::to_string(devSettings.UBPickup));
	PrintToScreen(Tab(1) + Dots(35, "UBDelay") + std::to_string(devSettings.UBDelay));
	PrintToScreen(Tab(1) + Dots(35, "UBEnabled") + std::to_string(devSettings.UBEnabled));
	PrintToScreen(Tab(1) + Dots(35, "UBSpare8") + std::to_string(devSettings.UBSpare8));
	PrintToScreen(Tab(1) + Dots(35, "OVTripPickupLL") + std::to_string(devSettings.OVTripPickupLL));
	PrintToScreen(Tab(1) + Dots(35, "OVAlarmPickupLL") + std::to_string(devSettings.OVAlarmPickupLL));
	PrintToScreen(Tab(1) + Dots(35, "OVTripDelayLL") + std::to_string(devSettings.OVTripDelayLL));
	PrintToScreen(Tab(1) + Dots(35, "OVAlarmDelayLL") + std::to_string(devSettings.OVAlarmDelayLL));
	PrintToScreen(Tab(1) + Dots(35, "OVTripEnabledLL") + std::to_string(devSettings.OVTripEnabledLL));
	PrintToScreen(Tab(1) + Dots(35, "OVSpare8") + std::to_string(devSettings.OVSpare8));
	PrintToScreen(Tab(1) + Dots(35, "UVTripPickupLL") + std::to_string(devSettings.UVTripPickupLL));
	PrintToScreen(Tab(1) + Dots(35, "UVAlarmPickupLL") + std::to_string(devSettings.UVAlarmPickupLL));
	PrintToScreen(Tab(1) + Dots(35, "UVTripDelayLL") + std::to_string(devSettings.UVTripDelayLL));
	PrintToScreen(Tab(1) + Dots(35, "UVAlarmDelayLL") + std::to_string(devSettings.UVAlarmDelayLL));
	PrintToScreen(Tab(1) + Dots(35, "UVTripEnabledLL") + std::to_string(devSettings.UVTripEnabledLL));
	PrintToScreen(Tab(1) + Dots(35, "UVSpare8") + std::to_string(devSettings.UVSpare8));
	PrintToScreen(Tab(1) + Dots(35, "OFValueTrip") + std::to_string(devSettings.OFValueTrip));
	PrintToScreen(Tab(1) + Dots(35, "OFValueAlarm") + std::to_string(devSettings.OFValueAlarm));
	PrintToScreen(Tab(1) + Dots(35, "OFEnabled") + std::to_string(devSettings.OFEnabled));
	PrintToScreen(Tab(1) + Dots(35, "UFValueTrip") + std::to_string(devSettings.UFValueTrip));
	PrintToScreen(Tab(1) + Dots(35, "UFValueTrip") + std::to_string(devSettings.UFValueTrip));
	PrintToScreen(Tab(1) + Dots(35, "UFValueAlarm") + std::to_string(devSettings.UFValueAlarm));
	PrintToScreen(Tab(1) + Dots(35, "UFEnabled") + std::to_string(devSettings.UFEnabled));
	PrintToScreen(Tab(1) + Dots(35, "UFSpare8") + std::to_string(devSettings.UFSpare8));
	PrintToScreen(Tab(1) + Dots(35, "PVLossEnabled") + std::to_string(devSettings.PVLossEnabled));
	PrintToScreen(Tab(1) + Dots(35, "PVLossDelay") + std::to_string(devSettings.PVLossDelay));
	PrintToScreen(Tab(1) + Dots(35, "SystemRotation") + std::to_string(devSettings.SystemRotation));
	PrintToScreen(Tab(1) + Dots(35, "NSOVPickupPercent") + std::to_string(devSettings.NSOVPickupPercent));
	PrintToScreen(Tab(1) + Dots(35, "RPPickup") + std::to_string(devSettings.RPPickup));
	PrintToScreen(Tab(1) + Dots(35, "RPDelay") + std::to_string(devSettings.RPDelay));
	PrintToScreen(Tab(1) + Dots(35, "RPEnabled") + std::to_string(devSettings.RPEnabled));
	PrintToScreen(Tab(1) + Dots(35, "AlarmRelayTU") + std::to_string(devSettings.AlarmRelayTU));
	PrintToScreen(Tab(1) + Dots(35, "AlarmRelayHI") + std::to_string(devSettings.AlarmRelayHI));
	PrintToScreen(Tab(1) + Dots(35, "AlarmRelayRIU1") + std::to_string(devSettings.AlarmRelayRIU1));
	PrintToScreen(Tab(1) + Dots(35, "AlarmRelayRIU2") + std::to_string(devSettings.AlarmRelayRIU2));
	PrintToScreen(Tab(1) + Dots(35, "SBThreshold") + std::to_string(devSettings.SBThreshold));
	PrintToScreen(Tab(1) + Dots(35, "DateFormat") + std::to_string(devSettings.DateFormat));
	PrintToScreen(Tab(1) + Dots(35, "LanguageID") + std::to_string(devSettings.LanguageID));
	PrintToScreen(Tab(1) + Dots(35, "NeutralProtectionPercent") + std::to_string(devSettings.NeutralProtectionPercent));
	PrintToScreen(Tab(1) + Dots(35, "ZoneBlockBits[0]") + std::to_string(devSettings.ZoneBlockBits[0]));
	PrintToScreen(Tab(1) + Dots(35, "ZoneBlockBits[1]") + std::to_string(devSettings.ZoneBlockBits[1]));
	PrintToScreen(Tab(1) + Dots(35, "ProgrammableRelay[0]") + std::to_string(devSettings.ProgrammableRelay[0]));
	PrintToScreen(Tab(1) + Dots(35, "ProgrammableRelay[1]") + std::to_string(devSettings.ProgrammableRelay[1]));
	PrintToScreen(Tab(1) + Dots(35, "USBTripEnabled") + std::to_string(devSettings.USBTripEnabled));
	PrintToScreen(Tab(1) + Dots(35, "ThresholdPhaseCT") + std::to_string(devSettings.ThresholdPhaseCT));
	PrintToScreen(Tab(1) + Dots(35, "ThresholdNeutralCT") + std::to_string(devSettings.ThresholdNeutralCT));
	PrintToScreen(Tab(1) + Dots(35, "DINF") + std::to_string(devSettings.DINF));
	PrintToScreen(Tab(1) + Dots(35, "ModbusForcedTripEnabled") + std::to_string(devSettings.ModbusForcedTripEnabled));
	PrintToScreen(Tab(1) + Dots(35, "ModbusChangeUserSettingsEnabled") + std::to_string(devSettings.ModbusChangeUserSettingsEnabled));
	PrintToScreen(Tab(1) + Dots(35, "ModbusSoftQuickTripSwitchEnabled") + std::to_string(devSettings.ModbusSoftQuickTripSwitchEnabled));
	PrintToScreen(Tab(1) + Dots(35, "ChangeSource") + std::to_string(devSettings.ChangeSource));
	PrintToScreen(Tab(1) + Dots(35, "LastChanged") + TimeDate4ToString(devSettings.LastChanged));
}

static void PrintPersonality(const Personality4 &personality)
{
	PrintToScreen(Tab(1) + Dots(35, "structID") + std::to_string(personality.structID));
	PrintToScreen(Tab(1) + Dots(35, "options32") + std::to_string(personality.options32));
	PrintToScreen(Tab(1) + Dots(35, "options16") + std::to_string(personality.options16));
	PrintToScreen(Tab(1) + Dots(35, "optionsTrip") + std::to_string(personality.optionsTrip));
	PrintToScreen(Tab(1) + Dots(35, "maxRMS") + std::to_string(personality.maxRMS));
	PrintToScreen(Tab(1) + Dots(35, "maxCTRatingX") + std::to_string(personality.maxCTRatingX));
	PrintToScreen(Tab(1) + Dots(35, "minCTRating") + std::to_string(personality.minCTRating));
	PrintToScreen(Tab(1) + Dots(35, "maxCTSecondary") + std::to_string(personality.maxCTSecondary));
	PrintToScreen(Tab(1) + Dots(35, "minCTSecondary") + std::to_string(personality.minCTSecondary));
	PrintToScreen(Tab(1) + Dots(35, "maxCTNeutral") + std::to_string(personality.maxCTNeutral));
	PrintToScreen(Tab(1) + Dots(35, "minCTNeutral") + std::to_string(personality.minCTNeutral));
	PrintToScreen(Tab(1) + Dots(35, "absLTPickup") + std::to_string(personality.absLTPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxLTPickup") + std::to_string(personality.maxLTPickup));
	PrintToScreen(Tab(1) + Dots(35, "minLTPickupX") + std::to_string(personality.minLTPickupX));
	PrintToScreen(Tab(1) + Dots(35, "maxLTDelay") + std::to_string(personality.maxLTDelay));
	PrintToScreen(Tab(1) + Dots(35, "minLTDelay") + std::to_string(personality.minLTDelay));
	PrintToScreen(Tab(1) + Dots(35, "absSTPickup") + std::to_string(personality.absSTPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxSTPickup") + std::to_string(personality.maxSTPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxSTPickup") + std::to_string(personality.maxSTPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxSTDelay") + std::to_string(personality.maxSTDelay));
	PrintToScreen(Tab(1) + Dots(35, "minSTDelay") + std::to_string(personality.minSTDelay));
	PrintToScreen(Tab(1) + Dots(35, "absInstPickup") + std::to_string(personality.absInstPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxInstPickup") + std::to_string(personality.maxInstPickup));
	PrintToScreen(Tab(1) + Dots(35, "minInstPickup") + std::to_string(personality.minInstPickup));
	PrintToScreen(Tab(1) + Dots(35, "absGFPickup") + std::to_string(personality.absGFPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxGFPickup") + std::to_string(personality.maxGFPickup));
	PrintToScreen(Tab(1) + Dots(35, "minGFPickup") + std::to_string(personality.minGFPickup));
	PrintToScreen(Tab(1) + Dots(35, "maxGFDelay") + std::to_string(personality.maxGFDelay));
	PrintToScreen(Tab(1) + Dots(35, "minGFDelay") + std::to_string(personality.minGFDelay));
	PrintToScreen(Tab(1) + Dots(35, "reduceRailPercentPhase") + std::to_string(personality.reduceRailPercentPhase));
	PrintToScreen(Tab(1) + Dots(35, "reduceRailPercentNeutral") + std::to_string(personality.reduceRailPercentNeutral));
	PrintToScreen(Tab(1) + Dots(35, "CC_Options") + std::to_string(personality.CC_Options));
}

static void PrintBkrClrTimeTH(const BkrClrTimeTH *bct)
{
	scr_printf("\tBreaker Clearing Times (0.1 ms):");

	for (int i = 0; i < _NUM_PHASES_ABC; ++i)
	{
		scr_printf("\t\tPhase %d: %u", i + 1, bct->Phase[i]);
	}
}

static void PrintTripHistoryCounts(const CounterTH4 *counter)
{
	scr_printf("Trip Count Information:");

	for (int i = 0; i < _TRIP_TYPE_MAX; ++i)
	{
		scr_printf("\tTrip type %d: %u", i + 1, counter->trip[i]);
	}

	scr_printf("Sluggish Breaker Events: %u", counter->sluggishBreaker);
	scr_printf("Next Available Offset: %u", counter->nextAvailableOffset);
}

static void PrintTripData4(const TripData4 *data)
{
	// PrintToScreen(TimeDate4ToString((const DateTime4&)&data->TimeStamp));
	scr_printf("Trip Data:");

	PrintBkrClrTimeTH(&data->BCT);

	scr_printf("\tTrip Status: %u", data->TripStatus);
	for (int i = 0; i < _NUM_PHASES_ABCN; ++i)
	{
		scr_printf("\tCurrent (Phase/Neutral) %d: %u", i + 1, data->I[i]);
	}
	scr_printf("\tGround Fault Current: %u", data->I_GF);
	for (int i = 0; i < _NUM_PHASES_ABC; ++i)
	{
		scr_printf("\tVoltage Line-Line %d: %u", i + 1, data->VLL[i]);
		scr_printf("\tVoltage Line-Neutral %d: %u", i + 1, data->VLN[i]);
	}
	scr_printf("\tCT Rating: %u", data->CTRating);
	scr_printf("\tCT Secondary: %u", data->CTSecondary);
	scr_printf("\tCT Neutral Secondary: %u", data->CTNeutralSec);
	scr_printf("\tUnbalance Percent: %u", data->UnbalancePercent);
	scr_printf("\tFrequency: %u Hz", data->Frequency);
	scr_printf("\tNSOV percent: %u", data->NSOVpercent);
	scr_printf("\tTrip Type: %u", data->TripType);
	scr_printf("\tThreshold Sluggish Breaker: %u", data->ThresholdSB);
	scr_printf("\tPeak RMS: %u", data->PeakRMS);
}

void PrintTripHist4(const TripHist4 *hist)
{

	PrintTripHistoryCounts(&hist->Counter);
	scr_printf("Number of Buffers: %u", hist->NumBufs);

	for (int i = 0; i < hist->NumBufs; ++i)
	{
		scr_printf("Trip %d:", i + 1);
		PrintTripData4(&hist->Data[i]);
	}
}

// use over-loaded functions here...

static void PrintDBRecord(const DB::TLThreshold::LTThresholdRecord &record)
{
	PrintToScreen(Tab(1) + Dots(35, "ID") + std::to_string(record.ID));
	PrintToScreen(Tab(1) + Dots(35, "SerialNum") + record.SerialNum);
	PrintToScreen(Tab(1) + Dots(35, "LTNoPu_A") + std::to_string(record.LTNoPu_A));
	PrintToScreen(Tab(1) + Dots(35, "LTPu_A") + std::to_string(record.LTPu_A));
	PrintToScreen(Tab(1) + Dots(35, "LTNoPu_B") + std::to_string(record.LTNoPu_B));
	PrintToScreen(Tab(1) + Dots(35, "LTPu_B") + std::to_string(record.LTPu_B));
	PrintToScreen(Tab(1) + Dots(35, "LTNoPu_C") + std::to_string(record.LTNoPu_C));
	PrintToScreen(Tab(1) + Dots(35, "LTPu_C") + std::to_string(record.LTPu_C));
}

static void PrintDBRecord(const DB::Personality::PersonalityRecord &record)
{
	PrintToScreen(Tab(1) + Dots(35, "ID") + std::to_string(record.ID));
	PrintToScreen(Tab(1) + Dots(35, "SerialNum") + record.SerialNum);
	PrintToScreen(Tab(1) + Dots(35, "LTPUOnOffPersonalitity") + std::to_string(record.LTPUOnOffPersonalitity));
	PrintToScreen(Tab(1) + Dots(35, "LTPUto120PercentPersonality") + std::to_string(record.LTPUto120PercentPersonality));
	PrintToScreen(Tab(1) + Dots(35, "LTDlyto50secPersonality") + std::to_string(record.LTDlyto50secPersonality));
	PrintToScreen(Tab(1) + Dots(35, "InstOverridePersonality") + std::to_string(record.InstOverridePersonality));
	PrintToScreen(Tab(1) + Dots(35, "InstClosePersonality") + std::to_string(record.InstClosePersonality));
	PrintToScreen(Tab(1) + Dots(35, "GFOnlyPersonality") + std::to_string(record.GFOnlyPersonality));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Menu commands: all these commands are invoked from the application's menu. they should not return
//				  anything. they should just print out to the screen the results of the command
//////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////
// file menu
//////////////////////////////////////////////////////

void SaveEditControlContentToFile(HWND hwndEdit, const std::string &filename)
{

	// Step 1: Retrieve the length of the text
	int textLength = GetWindowTextLength(hwndEdit);
	if (textLength == 0)
	{
		return; // No text to save
	}

	// Allocate buffer for text (+1 for null terminator)
	char *buffer = new char[textLength + 1];

	// Step 3: Get the text from the edit control
	GetWindowText(hwndEdit, buffer, textLength + 1);

	// Step 4: Open a file
	std::ofstream file(filename, std::ios::out | std::ios::binary);
	if (!file.is_open())
	{
		delete[] buffer; // Clean up buffer before returning
		PrintToScreen("Unable to open file for writing: " + filename);
		return; // Unable to open file
	}

	// Step 5: Write the text to the file
	file.write(buffer, textLength);

	// Clean up
	file.close();
	delete[] buffer; // Don't forget to free the allocated memory
}

static void menu_ID_FILE_SAVE()
{
	auto fileToSave = SelectFileToSave(hwndMain);
	if (fileToSave != "")
	{
		PrintToScreen("Saving content to file " + fileToSave);
		SaveEditControlContentToFile(hwndEdit, fileToSave);
	}
}

static void menu_ID_FILE_CLEARSCREEN()
{
	ClearTextBox();
}

static void menu_ID_FILE_EXIT()
{
	PostQuitMessage(0);
}

//////////////////////////////////////////////////////
// connection menu
//////////////////////////////////////////////////////

static void menu_ID_CONNECTION_CONNECT_KEITHLEY()
{

	if (!GetInputValue("Enter Keithley comm port:"))
		return;

	hKeithley.commPort = ReturnedInputValue_Float;

	if (KEITHLEY::Connect(&hKeithley.handle, hKeithley.commPort))
		PrintToScreen("Connected to connect to KEITHLEY on comm " + std::to_string(hKeithley.commPort));
	else
		PrintToScreen("Failed to connect to KEITHLEY on comm " + std::to_string(hKeithley.commPort));
}

static void menu_ID_CONNECTION_DISCONNECT_KEITHLEY()
{
	CloseCommPort(&hKeithley.handle);
	PrintToScreen("Disconnected from Keithley");
}

static void menu_ID_CONNECTION_FIND_KEITHLEY()
{
	if (hKeithley.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley already connected; please disconnect first");
		return;
	}
	std::thread thread(FindEquipment, true, false, false);
	thread.detach();
}

static void menu_ID_CONNECTION_CONNECT_RIGOL_DG1000Z()
{
	RIGOL_DG1000Z_Connected = RIGOL_DG1000Z::OpenVISAInterface();

	if (RIGOL_DG1000Z_Connected)
		RIGOL_DG1000Z_Connected = RIGOL_DG1000Z::Initialize();

	if (RIGOL_DG1000Z_Connected)
		PrintToScreen("RIGOL_DG1000Z connected successfully");
	else
		PrintToScreen("RIGOL_DG1000Z not found");

	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_DISCONNECT_RIGOL_DG1000Z()
{

	RIGOL_DG1000Z_Connected = false;
	PrintToScreen("RIGOL_DG1000Z AWG Disconnected");

	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_CONNECT_TRIPUNIT()
{
	if (!GetInputValue("Enter Trip Unit comm port:"))
		return;

	int tmpPort = ReturnedInputValue_Float;

	if (ConnectTripUnit(&hTripUnit.handle, tmpPort, &tripUnitType))
	{
		hTripUnit.commPort = tmpPort;
		PrintToScreen("Connected to trip unit on comm " + std::to_string(hTripUnit.commPort));
		PrintToScreen("Trip Unit Type: " + TripUnitTypeToString(tripUnitType));
	}
	else
		PrintToScreen("Failed to connect to trip unit on comm " + std::to_string(hTripUnit.commPort));

	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_DISCONNECT_TRIPUNIT()
{
	DisconnectFromTripUnit();
}

static void menu_ID_CONNECTION_FIND_TRIPUNIT()
{
	if (hTripUnit.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Trip Unit already connected; please disconnect first");
		return;
	}

	std::thread thread(FindEquipment, false, true, false);
	thread.detach();
}

// BK_PRECISION_9801 is a VISA device
static void menu_ID_CONNECTION_CONNECT_BK_PRECISION_9801()
{

	BK_PRECISION_9801_Connected = BK_PRECISION_9801::OpenVISAInterface();

	if (BK_PRECISION_9801_Connected)
		BK_PRECISION_9801_Connected = BK_PRECISION_9801::Initialize();

	if (BK_PRECISION_9801_Connected)
		PrintToScreen("BK_PRECISION_9801 connected successfully");
	else
		PrintToScreen("BK_PRECISION_9801 not found");

	PrintConnectionStatus();
}

// BK_PRECISION_9801 is a VISA device
static void menu_ID_CONNECTION_DISCONNECT_BK_PRECISION_9801()
{
	BK_PRECISION_9801_Connected = false;
	PrintToScreen("BK_PRECISION_9801 Disconnected");
	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_CONNECT_ARDUINO()
{
	if (hArduino.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino connected; please disconnect first");
		return;
	}

	if (!GetInputValue("Enter Arduino comm port:"))
		return;

	hArduino.commPort = ReturnedInputValue_Float;

	if (ARDUINO::Connect(&hArduino.handle, hArduino.commPort))
		PrintToScreen("Connected to Arduino on comm " + std::to_string(hArduino.commPort));
	else
		PrintToScreen("Failed to connect to Arduino on comm " + std::to_string(hArduino.commPort));

	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_DISCONNECT_ARDUINO()
{
	CloseCommPort(&hArduino.handle);
	PrintToScreen("Disconnected from Arduino");
	PrintConnectionStatus();
}

static void menu_ID_CONNECTION_FIND_ARUINO()
{
	if (hArduino.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino already connected; please disconnect first");
		return;
	}

	std::thread thread(FindEquipment, false, false, true);
	thread.detach();
}

static void menu_ID_CONNECTION_ENUMERATECOMMPORTS()
{
	EnumeratePorts();
}

static void menu_ID_CONNECTION_PRINTCONNECTIONSTATUS()
{
	PrintConnectionStatus();
}

static void menu_ID_FIND_DEVICES_2()
{
	bool findKeithley = true;
	bool findTripUnit = true;
	bool findArduino = true;

	if (hKeithley.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley already connected; skipping...");
		findKeithley = false;
	}

	if (hTripUnit.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Trip Unit already connected; skipping");
		findTripUnit = false;
	}

	if (hArduino.handle != INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino already connected; skipping");
		findArduino = false;
	}

	if (!RIGOL_DG1000Z_Connected)
		menu_ID_CONNECTION_CONNECT_RIGOL_DG1000Z();
	else
		PrintToScreen("RIGOL_DG1000Z already connected skipping...");

	if (findKeithley || findTripUnit || findArduino)
	{
		std::thread thread(FindEquipment, findKeithley, findTripUnit, findArduino);
		thread.detach();
	}
	else
	{
		PrintToScreen("Keithley and Trip Unit already connected");
	}
}

//////////////////////////////////////////////////////
// database menu
//////////////////////////////////////////////////////

static void menu_ID_DUMP_DB_LT_THRESHOLD()
{

	DB::TLThreshold::LTThresholdRecord record = {0};

	// get the record number
	if (!GetInputValue("Enter serial number:"))
		return;

	std::string serial_num = ReturnedInputValue_String;

	// get the record
	if (!DB::TLThreshold::Read(record, serial_num))
	{
		PrintToScreen("Error reading database record");
		return;
	}

	// print the record
	PrintDBRecord(record);

	// for testing, we can write the record back into the database
	// and read it back to make sure all this code works
#if true

	record.LTNoPu_A = 10;
	record.LTPu_A = 11;
	record.LTNoPu_B = 12;
	record.LTPu_B = 13;
	record.LTNoPu_C = 14;
	record.LTPu_C = 15;

	// update the record
	if (!DB::TLThreshold::Write(record))
	{
		PrintToScreen("Error writing database record");
		return;
	}

	record = {0};

	// now re-read the record
	if (!DB::TLThreshold::Read(record, serial_num))
	{
		PrintToScreen("Error re-reading database record");
		return;
	}

	bool testPassed = true;

	testPassed &= (record.LTNoPu_A == 10);
	testPassed &= (record.LTPu_A == 11);
	testPassed &= (record.LTNoPu_B == 12);
	testPassed &= (record.LTPu_B == 13);
	testPassed &= (record.LTNoPu_C == 14);
	testPassed &= (record.LTPu_C == 15);

	if (testPassed)
		PrintToScreen("Database record write/read-back test passed");
	else
		PrintToScreen("Database record write/read-back test failed");

#endif
}

void menu_ID_DUMP_DB_PERSONALITY()
{

	DB::Personality::PersonalityRecord record = {0};

	// get the record number
	if (!GetInputValue("Enter serial number:"))
		return;

	std::string serial_num = ReturnedInputValue_String;

	// get the record
	if (!DB::Personality::Read(record, serial_num))
	{
		PrintToScreen("Error reading database record");
		return;
	}

	// print the record
	PrintDBRecord(record);

	// for testing, we can write the record back into the database
	// and read it back to make sure all this code works
#if true

	record.LTPUOnOffPersonalitity = false; // make a pattern
	record.LTPUto120PercentPersonality = true;
	record.LTDlyto50secPersonality = false;
	record.InstOverridePersonality = true;
	record.InstClosePersonality = false;
	record.InstBlockPersonality = true;
	record.GFOnlyPersonality = false;

	// update the record
	if (!DB::Personality::Write(record))
	{
		PrintToScreen("Error writing database record");
		return;
	}

	record = {0};

	// now re-read the record
	if (!DB::Personality::Read(record, serial_num))
	{
		PrintToScreen("Error re-reading database record");
		return;
	}

	bool testPassed = true;

	testPassed &= (record.LTPUOnOffPersonalitity == false); // same pattern as above
	testPassed &= (record.LTPUto120PercentPersonality == true);
	testPassed &= (record.LTDlyto50secPersonality == false);
	testPassed &= (record.InstOverridePersonality == true);
	testPassed &= (record.InstClosePersonality == false);
	testPassed &= (record.InstBlockPersonality == true);
	testPassed &= (record.GFOnlyPersonality == false);

	if (testPassed)
		PrintToScreen("Database record read/write test passed");
	else
		PrintToScreen("Database record read/write test failed");

#endif
}

//////////////////////////////////////////////////////
// ACPRO2 menu
//////////////////////////////////////////////////////

HANDLE GetHandleForTripUnit()
{
	return hTripUnit.handle;
}

static void menu_ID_CALIBRATION_SET_TO_DEFAULT()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (ACPRO2_RG::SetCalToDefault(hHandleForTripUnit))
		PrintToScreen("InitCalibration() successful");
	else
		PrintToScreen("Error sending MSG_EXE_INIT_CAL command to trip unit");
}

static void menu_ID_ACPRO2_NEGOTIATE_BAUD()
{

	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == hTripUnit.handle)
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	SetHighestTripUnitBaudRate();
}

static void menu_ID_ACPro2_UNCALIBRATE()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (ACPRO2_RG::Uncalibrate(hHandleForTripUnit))
		PrintToScreen("_CALIBRATION_UNCALIBRATE successful");
	else
		PrintToScreen("Error sending _CALIBRATION_UNCALIBRATE command to trip unit");
}

static void menu_ID_ACPro2_INITCAL()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (ACPRO2_RG::InitCalibration(hHandleForTripUnit))
		PrintToScreen("InitCalibration() successful");
	else
		PrintToScreen("Error sending MSG_EXE_INIT_CAL command to trip unit");
}

void PrintCalCommand(int16_t Command)
{

	PrintToScreen("calRequest.Commands " + std::to_string(Command));

	if (Command & _CALIBRATION_REQUEST_IA_HI)
		PrintToScreen("_CALIBRATION_REQUEST_IA_HI");
	if (Command & _CALIBRATION_REQUEST_IB_HI)
		PrintToScreen("_CALIBRATION_REQUEST_IB_HI");
	if (Command & _CALIBRATION_REQUEST_IC_HI)
		PrintToScreen("_CALIBRATION_REQUEST_IC_HI");
	if (Command & _CALIBRATION_REQUEST_IN_HI)
		PrintToScreen("_CALIBRATION_REQUEST_IN_HI");

	if (Command & _CALIBRATION_REQUEST_IA_LO)
		PrintToScreen("_CALIBRATION_REQUEST_IA_LO");
	if (Command & _CALIBRATION_REQUEST_IB_LO)
		PrintToScreen("_CALIBRATION_REQUEST_IB_LO");
	if (Command & _CALIBRATION_REQUEST_IC_LO)
		PrintToScreen("_CALIBRATION_REQUEST_IC_LO");
	if (Command & _CALIBRATION_REQUEST_IN_LO)
		PrintToScreen("_CALIBRATION_REQUEST_IN_LO");

	if ((Command & _CALIBRATION_REQUEST_HI_GAIN_MASK) == _CALIBRATION_REQUEST_HI_GAIN_2_0)
		PrintToScreen("_CALIBRATION_REQUEST_HARDWARE_GAIN_2_0");
	if ((Command & _CALIBRATION_REQUEST_HI_GAIN_MASK) == _CALIBRATION_REQUEST_HI_GAIN_0_5)
		PrintToScreen("_CALIBRATION_REQUEST_HARDWARE_GAIN_0_5");
	if ((Command & _CALIBRATION_REQUEST_HI_GAIN_MASK) == _CALIBRATION_REQUEST_HI_GAIN_1_0)
		PrintToScreen("_CALIBRATION_REQUEST_HARDWARE_GAIN_1_0");
	if ((Command & _CALIBRATION_REQUEST_HI_GAIN_MASK) == _CALIBRATION_REQUEST_HI_GAIN_1_5)
		PrintToScreen("_CALIBRATION_REQUEST_HARDWARE_GAIN_1_5");
}

bool CommandRigolDG100zVoltage(bool Do50hz, float VoltageRMS)
{

	std::string value_to_command;
	bool operationOK;

	if (!RIGOL_DG1000Z_Connected)
		return false;

	value_to_command = std::to_string(VoltageRMS);

	operationOK = RIGOL_DG1000Z::SetupToApplySINWave(false, value_to_command);

	if (operationOK)
	{
		operationOK = RIGOL_DG1000Z::EnableOutput();
	}

	if (operationOK)
	{
		PrintToScreen("Sine wave applied RIGOL_DG1000Z");
		PrintToScreen("RMS Voltage selected " + value_to_command + "vRMS");
		if (Do50hz)
			PrintToScreen("Frequency selected 50hz ");
		else
			PrintToScreen("Frequency selected 60hz ");
	}
	else
	{
		// just make sure the output is disabled no matter what
		// if something went wrong
		RIGOL_DG1000Z::DisableOutput();
		PrintToScreen("Error applying sine wave to RIGOL_DG1000Z; output disabled");
	}

	return operationOK;
}

bool CommandBK9801Voltage(bool Do50hz, float VoltageRMS)
{

	std::string value_to_command;
	bool operationOK;

	if (!BK_PRECISION_9801_Connected)
		return false;

	value_to_command = std::to_string(VoltageRMS);

	operationOK = BK_PRECISION_9801::SetupToApplySINWave(false, value_to_command);

	if (operationOK)
		operationOK = BK_PRECISION_9801::EnableOutput();

	if (operationOK)
	{
		PrintToScreen("Sine wave applied BK_PRECISION_9801");
		PrintToScreen("RMS Voltage selected" + value_to_command + "vRMS");
		if (Do50hz)
			PrintToScreen("Frequency selected 50hz ");
		else
			PrintToScreen("Frequency selected 60hz ");
	}
	else
	{
		// just make sure the output is disabled no matter what
		// if something went wrong
		BK_PRECISION_9801::DisableOutput();
		PrintToScreen("Error applying sine wave to BK_PRECISION_9801; output disabled");
	}

	return operationOK;
}

static void DumpCalCommands(ACPRO2_RG::ArbitraryCalibrationParams cal_params)
{

	PrintToScreen(Dots(20, "Valid?") + boolToString(cal_params.valid));
	PrintToScreen(Dots(20, "Channel A?") + boolToString(cal_params.Do_Channel_A));
	PrintToScreen(Dots(20, "Channel B?") + boolToString(cal_params.Do_Channel_B));
	PrintToScreen(Dots(20, "Channel C?") + boolToString(cal_params.Do_Channel_C));
	PrintToScreen(Dots(20, "Channel N?") + boolToString(cal_params.Do_Channel_N));
	PrintToScreen(Dots(20, "Do Low Gain?") + boolToString(cal_params.Do_LowGain_Not_HighGain));
	PrintToScreen(Dots(20, "Send DG1000Z cmds?") + boolToString(cal_params.send_DG1000Z_Commands));
	PrintToScreen(Dots(20, "Send BK9801 cmds?") + boolToString(cal_params.send_BK9801_Commands));
	PrintToScreen(Dots(20, "Use 50hz?") + boolToString(cal_params.Use50HZ));
	PrintToScreen(Dots(20, "Rigol Volts RMS") + std::to_string(cal_params.voltageToCommandRMS));

	std::string tmpString;
	switch (cal_params.High_Gain_Point)
	{
	case 0:
		tmpString = "_CALIBRATION_REQUEST_HI_GAIN_0_5";
		break;
	case 1:
		tmpString = "_CALIBRATION_REQUEST_HI_GAIN_1_0";
		break;
	case 2:
		tmpString = "_CALIBRATION_REQUEST_HI_GAIN_1_5";
		break;
	case 3:
		tmpString = "_CALIBRATION_REQUEST_HI_GAIN_2_0";
		break;
	default:
		tmpString = "invalid";
	}

	PrintToScreen(Dots(20, "High Cal Point") + tmpString);
}

static void DoAsyncArbitraryCal(HANDLE hHandleForTripUnit, HANDLE hKeithley)
{
	int msgboxID;

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("NOTE: RIGOL_DG1000Z is not connected; make sure to manually configure RIGOL voltage and frequency");
		PrintToScreen("NOTE: currently, should be 7RMS for high gain, and {1.32, 0.660, 0.495, 0.330}; for low gain");
	}

	CalibrationResults calResults = {0};
	CalibrationRequest calRequest = {0};
	uint16_t RMSCurrentToCalibrateTo;

	calRequest.Commands = 0;

	// start off by reading the calibration parameters from the INI file that we used last time
	ACPRO2_RG::ReadArbitraryParamsFromINI(iniFile, arbitraryRCCalibrationParams);

	DialogBoxA(NULL, MAKEINTRESOURCE(ID_RC_ARBITRARY_CAL), hwndMain, RC_ArbitraryCal_DlgProc);

	if (!arbitraryRCCalibrationParams.valid)
	{
		PrintToScreen("aborted");
		return;
	}

	// save the parameters that they just typed in
	ACPRO2_RG::WriteArbitraryCalibrationParamsToINI(iniFile, arbitraryRCCalibrationParams);

	switch (arbitraryRCCalibrationParams.High_Gain_Point)
	{
	case 0:
		calRequest.Commands = _CALIBRATION_REQUEST_HI_GAIN_0_5;
		break;
	case 1:
		calRequest.Commands = _CALIBRATION_REQUEST_HI_GAIN_1_0;
		break;
	case 2:
		calRequest.Commands = _CALIBRATION_REQUEST_HI_GAIN_1_5;
		break;
	case 3:
		calRequest.Commands = _CALIBRATION_REQUEST_HI_GAIN_2_0;
		break;
	default:
		PrintToScreen("invalid high gain point; aborting");
		return;
	}

	if (arbitraryRCCalibrationParams.send_DG1000Z_Commands && arbitraryRCCalibrationParams.send_BK9801_Commands)
	{
		PrintToScreen("Error: can't send commands to both RigolDG1000z and BK9801; aborting");
		return;
	}

	if (arbitraryRCCalibrationParams.send_DG1000Z_Commands && !RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("Error: can't send commands to RigolDG1000z; it's not connected. Aborting");
		return;
	}

	if (arbitraryRCCalibrationParams.send_BK9801_Commands && !BK_PRECISION_9801_Connected)
	{
		PrintToScreen("Error: can't send commands to BK9801_Commands; it's not connected. Aborting");
		return;
	}

	// anything past here, the voltage is applied
	// so make sure to turn it off if doing a return

	if (arbitraryRCCalibrationParams.send_DG1000Z_Commands)
	{
		PrintToScreen("Commanding Rigol DG1000z to output sine wave");
		if (!CommandRigolDG100zVoltage(arbitraryRCCalibrationParams.Use50HZ, arbitraryRCCalibrationParams.voltageToCommandRMS))
		{
			PrintToScreen("Error sending command to DG1000z; aborting");
			return;
		}
	}
	else if (arbitraryRCCalibrationParams.send_BK9801_Commands)
	{
		PrintToScreen("Commanding BK9801 to output sine wave");
		if (!CommandBK9801Voltage(arbitraryRCCalibrationParams.Use50HZ, arbitraryRCCalibrationParams.voltageToCommandRMS))
		{
			PrintToScreen("Error sending command to BK9801; aborting");
			return;
		}
	}

	// just get final confirmation
	msgboxID = MessageBoxA(hwndMain, "Click OK to proceed...", "Error", MB_ICONERROR | MB_OKCANCEL);
	if (msgboxID == IDCANCEL)
	{
		PrintToScreen("aborted");

		if (arbitraryRCCalibrationParams.send_DG1000Z_Commands)
			RIGOL_DG1000Z::DisableOutput();
		else if (arbitraryRCCalibrationParams.send_BK9801_Commands)
			BK_PRECISION_9801::DisableOutput();

		return;
	}

	// wait 10 seconds
	PrintToScreen("Waiting 10 seconds...");
	Sleep(10000);

	PrintToScreen("Looking for stablized voltage on Keithley...");

	if (!KEITHLEY::VoltageOnKeithleyIsStable(hKeithley))
	{

		PrintToScreen("Keithley voltage not stable enough to proceed; aborted");

		if (arbitraryRCCalibrationParams.send_DG1000Z_Commands)
			RIGOL_DG1000Z::DisableOutput();
		else if (arbitraryRCCalibrationParams.send_BK9801_Commands)
			BK_PRECISION_9801::DisableOutput();
		return;
	}

	PrintToScreen("taking Keithley voltage reading....");
	double KeithleyVoltageRMS = KEITHLEY::GetVoltageForAutoRange(hKeithley);

	PrintToScreen("keithley voltage: " + std::to_string(KeithleyVoltageRMS));
	RMSCurrentToCalibrateTo = KeithleyVoltageRMS * 3800.0;
	PrintToScreen("keithley voltage * 3800 = " + std::to_string(RMSCurrentToCalibrateTo));

	// phase A
	if (arbitraryRCCalibrationParams.Do_Channel_A)
	{
		if (arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain)
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IA_LO;
			calRequest.RealRMS[i_calibrationIaLO] = RMSCurrentToCalibrateTo;
		}
		else
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IA_HI;
			calRequest.RealRMS[i_calibrationIaHI] = RMSCurrentToCalibrateTo;
		}
	}

	// phase B
	if (arbitraryRCCalibrationParams.Do_Channel_B)
	{
		if (arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain)
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IB_LO;
			calRequest.RealRMS[i_calibrationIbLO] = RMSCurrentToCalibrateTo;
		}
		else
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IB_HI;
			calRequest.RealRMS[i_calibrationIbHI] = RMSCurrentToCalibrateTo;
		}
	}

	// phase C
	if (arbitraryRCCalibrationParams.Do_Channel_C)
	{
		if (arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain)
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IC_LO;
			calRequest.RealRMS[i_calibrationIcLO] = RMSCurrentToCalibrateTo;
		}
		else
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IC_HI;
			calRequest.RealRMS[i_calibrationIcHI] = RMSCurrentToCalibrateTo;
		}
	}

	// phase N
	if (arbitraryRCCalibrationParams.Do_Channel_N)
	{
		if (arbitraryRCCalibrationParams.Do_LowGain_Not_HighGain)
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IN_LO;
			calRequest.RealRMS[i_calibrationInLO] = RMSCurrentToCalibrateTo;
		}
		else
		{
			calRequest.Commands |= _CALIBRATION_REQUEST_IN_HI;
			calRequest.RealRMS[i_calibrationInHI] = RMSCurrentToCalibrateTo;
		}
	}

	PrintCalCommand(calRequest.Commands);
	DumpCalCommands(arbitraryRCCalibrationParams);

	PrintToScreen("sending MSG_EXE_CALIBRATE_AD message to trip unit....");

	// we check to make sure the Keithley voltage is stable before starting the calibration
	// additionally: we also monitor it during the calibration
	ASYNC_KEITHLEY::StartMonitoringKeithley(hKeithley);

	// time how long MSG_EXE_CALIBRATE_AD takes...
	long long durationMS;
	bool retval = ACPRO2_RG::Time_CalibrateChannel(hHandleForTripUnit, calRequest, calResults, durationMS);

	ASYNC_KEITHLEY::StopMonitoringKeithley();

	if (retval)
	{
		retval &= ASYNC_KEITHLEY::KeithleyIsStable();

		if (!retval)
			PrintToScreen("Unstable Keithley voltage seen during MSG_EXE_CALIBRATE_AD; calibration failed");
	}
	if (retval)
		ACPRO2_RG::DumpCalibrationResults(&calResults);
	else
		PrintToScreen("MSG_EXE_CALIBRATE_AD failed");

	PrintToScreen("MSG_EXE_CALIBRATE_AD execution time (milliseconds): " + std::to_string(durationMS));
	PrintCalMatrix(calResults.CalibratedChannels);

	// TODO: check for valid calibration

	if (arbitraryRCCalibrationParams.send_DG1000Z_Commands)
		RIGOL_DG1000Z::DisableOutput();
	else if (arbitraryRCCalibrationParams.send_BK9801_Commands)
		BK_PRECISION_9801::DisableOutput();
}

static void menu_ID_RC_ARBITRARY_CAL()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;
	int msgboxID;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (tripUnitType != TripUnitType::AC_PRO2_RC)
	{
		msgboxID = MessageBoxA(hwndMain, "Trip Unit is not an AC-PRO-2 RC; click OK to proceed anyway...", "Error", MB_ICONERROR | MB_OKCANCEL);
		if (msgboxID == IDCANCEL)
		{
			PrintToScreen("User cancelled operation");
			return;
		}
		PrintToScreen("Trip Unit is not an AC-PRO-2 RC");
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley is not connected");
		return;
	}

	std::thread thread(DoAsyncArbitraryCal, hHandleForTripUnit, hKeithley.handle);
	thread.detach();
}

static void menu_ID_ACPro2_GET_MSG_GET_CAL()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	GetCalibration(hHandleForTripUnit);
}

static void menu_ID_ACPRO2_DUMP_PERSONALITY()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	bool retval;
	URCMessageUnion rsp = {0};

	SendURCCommand(hHandleForTripUnit, MSG_GET_PERSONALITY_4, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	retval =
		GetURCResponse(hHandleForTripUnit, &rsp) &&
		VerifyMessageIsOK(&rsp, MSG_RSP_PERSONALITY_4, sizeof(MsgRspPersonality4) - sizeof(MsgHdr));

	if (!retval)
	{
		PrintToScreen("Error sending MSG_GET_PERSONALITY_4");
		return;
	}

	PrintToScreen("Personality4:");
	PrintPersonality(rsp.msgRspPersonality4.Pers);
}

static void menu_ID_ACPRO2_DUMP_SETTINGS()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	URCMessageUnion rsp1 = {0};
	URCMessageUnion rsp2 = {0};
	bool retval;

	SendURCCommand(hHandleForTripUnit, MSG_GET_SYS_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	retval = GetURCResponse(hHandleForTripUnit, &rsp1) && VerifyMessageIsOK(&rsp1, MSG_RSP_SYS_SETTINGS_4, sizeof(MsgRspSysSet4) - sizeof(MsgHdr));

	if (!retval)
	{
		PrintToScreen("error sending MSG_GET_SYS_SETTINGS");
		return;
	}

	// grab device settings
	SendURCCommand(hHandleForTripUnit, MSG_GET_DEV_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	retval = GetURCResponse(hHandleForTripUnit, &rsp2) && VerifyMessageIsOK(&rsp2, MSG_RSP_DEV_SETTINGS_4, sizeof(MsgRspDevSet4) - sizeof(MsgHdr));

	if (!retval)
	{
		scr_printf("error receiving MSG_GET_DEV_SETTINGS");
		return;
	}

	PrintToScreen("System Settings:");
	PrintSystemSettings(rsp1.msgRspSysSet4.Settings);
	PrintToScreen("Device Settings:");
	PrintDeviceSettings(rsp2.msgRspDevSet4.Settings);
}

static void menu_ID_ACPRO2_DUMP_STATUS()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	bool retval;
	URCMessageUnion rsp = {0};

	SendURCCommand(hHandleForTripUnit, MSG_GET_STATUS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	retval =
		GetURCResponse(hHandleForTripUnit, &rsp) &&
		VerifyMessageIsOK(&rsp, MSG_RSP_STATUS_2, sizeof(MsgRspStatus2) - sizeof(MsgHdr));

	if (!retval)
	{
		PrintToScreen("Error sending MSG_GET_STATUS");
		return;
	}

	PrintToScreen("Status");
	PrintStatus(rsp.msgRspStatus2.Status);
}

static void menu_ID_ACPRO2_DUMP_TRIP_HISTORY()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	URCMessageUnion rsp = {0};

	if (!GetTripHistory(hHandleForTripUnit, &rsp))
	{
		PrintToScreen("Error sending MSG_GET_TRIP_HIST");
		return;
	}

	PrintToScreen("Trip History");
	PrintTripHist4(&rsp.msgRspTripHist4.Trips);
}

static void menu_ID_ACPRO2_DUMP_DYNAMICS()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	URCMessageUnion rsp = {0};

	// grab readings from the trip unit
	if (!GetDynamics(hHandleForTripUnit, &rsp))
	{
		PrintToScreen("ERROR: could not get reading from trip unit; aborting");
		return;
	}

	const Measurements4 &measurements = rsp.msgRspDynamics4.Dynamics.Measurements;

	PrintToScreen(Dots(20, "Vab") + std::to_string(measurements.Vab));
	PrintToScreen(Dots(20, "Vbc") + std::to_string(measurements.Vbc));
	PrintToScreen(Dots(20, "Vca") + std::to_string(measurements.Vca));
	PrintToScreen(Dots(20, "Van") + std::to_string(measurements.Van));
	PrintToScreen(Dots(20, "Vbn") + std::to_string(measurements.Vbn));
	PrintToScreen(Dots(20, "Vcn") + std::to_string(measurements.Vcn));
	PrintToScreen(Dots(20, "Ia") + std::to_string(measurements.Ia));
	PrintToScreen(Dots(20, "Ib") + std::to_string(measurements.Ib));
	PrintToScreen(Dots(20, "Ic") + std::to_string(measurements.Ic));
	PrintToScreen(Dots(20, "In") + std::to_string(measurements.In));
	PrintToScreen(Dots(20, "Igf") + std::to_string(measurements.Igf));
	PrintToScreen(Dots(20, "NSOV") + std::to_string(measurements.NSOV));
	PrintToScreen(Dots(20, "FrequencyX100") + std::to_string(measurements.FrequencyX100));
	PrintToScreen(Dots(20, "KWa") + std::to_string(measurements.KWa));
	PrintToScreen(Dots(20, "KWb") + std::to_string(measurements.KWb));
	PrintToScreen(Dots(20, "KWc") + std::to_string(measurements.KWc));
	PrintToScreen(Dots(20, "KWtotal") + std::to_string(measurements.KWtotal));
	PrintToScreen(Dots(20, "KVAa") + std::to_string(measurements.KVAa));
	PrintToScreen(Dots(20, "KVAb") + std::to_string(measurements.KVAb));
	PrintToScreen(Dots(20, "KVAc") + std::to_string(measurements.KVAc));
	PrintToScreen(Dots(20, "KVAtotal") + std::to_string(measurements.KVAtotal));
	PrintToScreen(Dots(20, "PFtotal") + std::to_string(measurements.PFtotal));
	PrintToScreen(Dots(20, "UnbalancePercent") + std::to_string(measurements.UnbalancePercent));
	PrintToScreen(Dots(20, "Spare0x00") + std::to_string(measurements.Spare0x00));
	PrintToScreen(Dots(20, "WHr") + std::to_string(measurements.WHr));
	PrintToScreen(Dots(20, "VAHr") + std::to_string(measurements.VAHr));
	PrintToScreen(Dots(20, "VARHr") + std::to_string(measurements.VARHr));
}

static void menu_ID_ACPro2_SAVE_TO_FLASH()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (ACPRO2_RG::DoCalibrationCommand(hHandleForTripUnit, _CALIBRATION_WRITE_TO_FLASH))
		PrintToScreen("calibration written to flash successfully");
	else
		PrintToScreen("Error sending _CALIBRATION_WRITE_TO_FLASH command to trip unit");
}

static void menu_ID_ACPro2_FORMAT_FRAM()
{
	HANDLE hHandleForTripUnit;
	URCMessageUnion rsp = {0};

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	SendURCCommand(hHandleForTripUnit, MSG_FORMAT_FRAM, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	if (GetAckURCResponse(hHandleForTripUnit, &rsp) && (rsp.msgHdr.Type == MSG_ACK))
		PrintToScreen("FRAM formatted");
	else
		PrintToScreen("Error formatting FRAM");
}

INT_PTR TripUnitUI_DlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static bool useRemoteSoftKey = true;

	HANDLE hHandleForTripUnit;
	static URCMessageUnion rsp = {0};

	hHandleForTripUnit = GetHandleForTripUnit();

	switch (message)
	{
	case WM_INITDIALOG:
		SendMessage(GetDlgItem(hwnd, IDC_RADIO_SOFTKEYS), BM_SETCHECK, BST_CHECKED, 0);
		return true; // let the system set the focus
	case WM_CLOSE:
		PostQuitMessage(0);
		EndDialog(hwnd, 0);
		return TRUE;
	case WM_TIMER:
		// PrintToScreen("WM_TIMER");
		if (useRemoteSoftKey)
			UpdateRemoteSoftkeys(hHandleForTripUnit, hwnd, 0);
		else
			BuildCurrentVoltageScreen(hHandleForTripUnit, hwnd);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO_SOFTKEYS:
			useRemoteSoftKey = true;
			return TRUE;
		case IDC_RADIO_DYNAMICS:
			useRemoteSoftKey = false;
			return TRUE;
		case IDC_BUTTON1:
			UpdateRemoteSoftkeys(hTripUnit.handle, hwnd, 2);
			return TRUE;
		case IDC_BUTTON2:
			UpdateRemoteSoftkeys(hTripUnit.handle, hwnd, 4);
			return TRUE;
		case IDC_BUTTON3:
			UpdateRemoteSoftkeys(hTripUnit.handle, hwnd, 8);
			return TRUE;
		case IDC_BUTTON4:
			UpdateRemoteSoftkeys(hTripUnit.handle, hwnd, 16);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static void SetOnScreenLEDStatus(HWND hDlg, bool b1, bool b2, bool b3, bool b4)
{
	SendMessage(GetDlgItem(hDlg, IDC_CHECK100), BM_SETCHECK, b1 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(GetDlgItem(hDlg, IDC_CHECK101), BM_SETCHECK, b2 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(GetDlgItem(hDlg, IDC_CHECK102), BM_SETCHECK, b3 ? BST_CHECKED : BST_UNCHECKED, 0);
	SendMessage(GetDlgItem(hDlg, IDC_CHECK103), BM_SETCHECK, b4 ? BST_CHECKED : BST_UNCHECKED, 0);
}

static void BlankOutTripUnitUI(HWND hDlg)
{
	SetDlgItemTextA(hDlg, IDC_STATIC1, "n/a");
	SetDlgItemTextA(hDlg, IDC_STATIC2, "n/a");
	SetDlgItemTextA(hDlg, IDC_STATIC3, "n/a");
	SetDlgItemTextA(hDlg, IDC_STATIC4, "n/a");
	SetDlgItemTextA(hDlg, IDC_STATIC5, "n/a");
	SetDlgItemTextA(hDlg, IDC_STATIC6, "n/a");

	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON1), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON2), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON3), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON4), SW_HIDE);

	SetOnScreenLEDStatus(hDlg, false, false, false, false);
}

bool isblanks(const char *s)
{
	return (' ' == s[0]) && (' ' == s[1]) && (' ' == s[2]) && (' ' == s[3]);
}

static void UpdateRemoteSoftkeys(HANDLE hTripUnit, HWND hDlg, uint8_t Key)
{

#define _LED_RED_PICKUP 0x08
#define _LED_GREEN_OK 0x10
#define _LED_RED_QT 0x20
#define _LED_GREEN_MODBUS 0x40

	static URCMessageUnion rsp = {0};

	if (INVALID_HANDLE_VALUE == hTripUnit)
	{
		BlankOutTripUnitUI(hDlg);
		// not connected to trip unit, nothing to do
		return;
	}

	// send the key they just pressed
	SendRemoteSoftkeyCommand(hTripUnit, Key);
	bool EverythingOK = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_REMOTE_SCREEN, sizeof(MsgRemoteScreen4) - sizeof(MsgHdr));

	if (!EverythingOK)
	{
		BlankOutTripUnitUI(hDlg);
		// not connected to trip unit, nothing to do
		return;
	}

	// and now grab the new screen
	SendRemoteSoftkeyCommand(hTripUnit, 0);
	EverythingOK = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_REMOTE_SCREEN, sizeof(MsgRemoteScreen4) - sizeof(MsgHdr));

	if (!EverythingOK)
	{
		BlankOutTripUnitUI(hDlg);
		// not connected to trip unit, nothing to do
		return;
	}

	SetDlgItemTextA(hDlg, IDC_STATIC1, rsp.msgRemoteScreen4.RemoteScreen.Line1);
	SetDlgItemTextA(hDlg, IDC_STATIC2, rsp.msgRemoteScreen4.RemoteScreen.Line2);
	SetDlgItemTextA(hDlg, IDC_STATIC3, rsp.msgRemoteScreen4.RemoteScreen.Line3);
	SetDlgItemTextA(hDlg, IDC_STATIC4, rsp.msgRemoteScreen4.RemoteScreen.Line4);
	SetDlgItemTextA(hDlg, IDC_STATIC5, rsp.msgRemoteScreen4.RemoteScreen.Line5);
	SetDlgItemTextA(hDlg, IDC_STATIC6, rsp.msgRemoteScreen4.RemoteScreen.Line6);

	char *button_text;

	button_text = rsp.msgRemoteScreen4.RemoteScreen.SoftKeyPB1;
	if (!isblanks(button_text))
	{
		SetDlgItemTextA(hDlg, IDC_BUTTON1, button_text);
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON1), SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON1), SW_HIDE);
	}

	button_text = rsp.msgRemoteScreen4.RemoteScreen.SoftKeyPB2;
	if (!isblanks(button_text))
	{
		SetDlgItemTextA(hDlg, IDC_BUTTON2, button_text);
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON2), SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON2), SW_HIDE);
	}

	button_text = rsp.msgRemoteScreen4.RemoteScreen.SoftKeyPB3;
	if (!isblanks(button_text))
	{
		SetDlgItemTextA(hDlg, IDC_BUTTON3, button_text);
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON3), SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON3), SW_HIDE);
	}

	button_text = rsp.msgRemoteScreen4.RemoteScreen.SoftKeyPB4;
	if (!isblanks(button_text))
	{
		SetDlgItemTextA(hDlg, IDC_BUTTON4, button_text);
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON4), SW_SHOW);
	}
	else
	{
		ShowWindow(GetDlgItem(hDlg, IDC_BUTTON4), SW_HIDE);
	}

	uint8_t LEDs = rsp.msgRemoteScreen4.RemoteScreen.LEDs;

	SetOnScreenLEDStatus(hDlg,
						 (0 != (_LED_GREEN_MODBUS & LEDs)),
						 (0 != (_LED_GREEN_OK & LEDs)),
						 (0 != (_LED_RED_QT & LEDs)),
						 (0 != (_LED_RED_PICKUP & LEDs)));
}

static void BuildCurrentVoltageScreen(HANDLE hTripUnit, HWND hDlg)
{
	bool retval = true;
	static URCMessageUnion rsp = {0};

	if (INVALID_HANDLE_VALUE == hTripUnit)
	{
		BlankOutTripUnitUI(hDlg);
		// not connected to trip unit, nothing to do
		return;
	}

	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON1), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON2), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON3), SW_HIDE);
	ShowWindow(GetDlgItem(hDlg, IDC_BUTTON4), SW_HIDE);

	if (retval)
	{
		SendURCCommand(hTripUnit, MSG_GET_DYNAMICS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, &rsp) &&
				 VerifyMessageIsOK(&rsp, MSG_RSP_DYNAMICS_4, sizeof(MsgRspDynamics4) - sizeof(MsgHdr));

		if (!retval)
			scr_printf("error sending MSG_GET_DYNAMICS");
	}

	if (retval)
	{
		DisplayValues dv = DisplayCurrentVoltageVDM(rsp.msgRspDynamics4.Dynamics.Measurements);

		SetDlgItemTextA(hDlg, IDC_STATIC1, dv.s1.c_str());
		SetDlgItemTextA(hDlg, IDC_STATIC2, dv.s2.c_str());
		SetDlgItemTextA(hDlg, IDC_STATIC3, dv.s3.c_str());
		SetDlgItemTextA(hDlg, IDC_STATIC4, dv.s4.c_str());
		SetDlgItemTextA(hDlg, IDC_STATIC5, dv.s5.c_str());
		SetDlgItemTextA(hDlg, IDC_STATIC6, "");

		SendURCCommand(hTripUnit, MSG_GET_LEDS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, &rsp) && rsp.msgHdr.Type == MSG_RSP_LEDS;

		// i think there is some packing issue...
		// VerifyMessageIsOK(&rsp, MSG_RSP_LEDS, sizeof(MsgRspLEDs) - sizeof(MsgHdr));
	}

	if (retval)
	{
		SetOnScreenLEDStatus(hDlg,
							 rsp.msgRspLEDs.State[LED_DIAG],
							 rsp.msgRspLEDs.State[LED_OK],
							 rsp.msgRspLEDs.State[LED_QT],
							 rsp.msgRspLEDs.State[LED_PICK_UP]);
	}
}

static void menu_ID_ACPRO2_TRIP_UNIT_UI()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	// create dialog box non modally
	HWND hDlg = CreateDialogA(NULL, MAKEINTRESOURCE(IDD_REMOTESOFTKEY), NULL, TripUnitUI_DlgProc);

	if (hDlg == NULL)
	{
		// Handle error
		PrintToScreen("Error creating dialog");
		return;
	}

	UINT_PTR const TIMER_ID = 1; // Timer identifier

	// Set a timer that sends WM_TIMER messages every 1000 milliseconds (1 second)
	UINT_PTR retval = SetTimer(hDlg, TIMER_ID, 1000, NULL);

	if (TIMER_ID != retval)
	{
		PrintToScreen("Error setting timer");
		return;
	}

	// Show the dialog box
	ShowWindow(hDlg, SW_SHOW);

	// Message loop
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	// Kill the timer when it's no longer needed
	KillTimer(hDlg, TIMER_ID);
}

//////////////////////////////////////////////////////
// RIGOL_DG1000Z menu
/////////////////////////////////////////////////////

static void menu_ID_SIGNALGENERATOR_ENABLEOUTPUT()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::EnableOutput();

	if (operationOK)
	{
		PrintToScreen("RIGOL_DG1000Z output enabled");
	}
	else
	{
		PrintToScreen("Error enabling RIGOL_DG1000Z output");
	}
}

static void menu_ID_SIGNALGENERATOR_DISABLEOUTPUT()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::DisableOutput();

	if (operationOK)
	{
		PrintToScreen("RIGOL_DG1000Z output disabled");
	}
	else
	{
		PrintToScreen("Error enabling RIGOL_DG1000Z output");
	}
}

static void menu_ID_SIGNALGENERATOR_APPLYSINWAVE1()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	bool operationOK = true;
	std::string voltageRMS;

	if (!GetInputValue("Enter RIGOL_DG1000Z RMS voltage; 0 to 7 volts RMS", -1))
	{
		PrintToScreen("Invalid voltage");
		return;
	}

	float tmpFloat = std::stof(ReturnedInputValue_String);
	if (!InRange(tmpFloat, 0, 7))
	{
		PrintToScreen("Invalid voltage");
		return;
	}

	voltageRMS = ReturnedInputValue_String;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::SetupToApplySINWave(false, voltageRMS);

	if (operationOK)
	{
		operationOK = RIGOL_DG1000Z::EnableOutput();
	}

	if (operationOK)
	{
		PrintToScreen("Sine wave is currently applied on the RIGOL_DG1000Z; voltage is present");
	}
	else
	{
		RIGOL_DG1000Z::DisableOutput();
		PrintToScreen("Error applying sine wave to RIGOL_DG1000Z; output disabled");
	}
}

//////////////////////////////////////////////////////////////////
// DUAL CHANNEL
//////////////////////////////////////////////////////////////////

static void menu_ID_SIGNALGENERATOR_ENABLEOUTPUT_2()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!RigolDualChannelMode)
	{
		PrintToScreen("Rigol Dual Channel Mode not enabled");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::EnableOutput_2();

	if (operationOK)
	{
		PrintToScreen("RIGOL_DG1000Z output 2 enabled");
	}
	else
	{
		PrintToScreen("Error enabling RIGOL_DG1000Z output 2");
	}
}

static void menu_ID_SIGNALGENERATOR_DISABLEOUTPUT_2()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!RigolDualChannelMode)
	{
		PrintToScreen("Rigol Dual Channel Mode not enabled");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::DisableOutput_2();

	if (operationOK)
	{
		PrintToScreen("RIGOL_DG1000Z output 2 disabled");
	}
	else
	{
		PrintToScreen("Error disabling RIGOL_DG1000Z output 2");
	}
}

static void menu_ID_SIGNALGENERATOR_APPLYSINWAVE_2()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!RigolDualChannelMode)
	{
		PrintToScreen("Rigol Dual Channel Mode not enabled");
		return;
	}

	bool operationOK = true;
	std::string voltageRMS;

	if (!GetInputValue("Enter RIGOL_DG1000Z RMS voltage for channel 2; 0 to 7 volts RMS", -1))
	{
		PrintToScreen("Invalid voltage");
		return;
	}

	float tmpFloat = std::stof(ReturnedInputValue_String);
	if (!InRange(tmpFloat, 0, 7))
	{
		PrintToScreen("Invalid voltage");
		return;
	}

	voltageRMS = ReturnedInputValue_String;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::SetupToApplySINWave_2(false, voltageRMS);

	if (operationOK)
	{
		operationOK = RIGOL_DG1000Z::EnableOutput_2();
	}

	if (operationOK)
	{
		PrintToScreen("Sine wave is currently applied on the RIGOL_DG1000Z (on channel 2); voltage is present");
	}
	else
	{
		RIGOL_DG1000Z::DisableOutput_2();
		PrintToScreen("Error applying sine wave to RIGOL_DG1000Z (on channel 2); output 2 disabled");
	}
}

static void menu_ID_RIGOL_PHASE2_PHASE_180()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::SendChannel2Phase180();

	if (operationOK)
		PrintToScreen("RIGOL_DG1000Z 'SOUR2:PHAS 180' command successful");
	else
		PrintToScreen("Error sending 'SOUR2:PHAS 180' command");
}

static void menu_ID_SETUP_SYNC_CHANNELS()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	bool operationOK = true;

	if (operationOK)
		operationOK = RIGOL_DG1000Z::SendSyncChannels();

	if (operationOK)
		PrintToScreen("RIGOL_DG1000Z SyncChannels command successful");
	else
		PrintToScreen("Error sending SyncChannels command");
}

//////////////////////////////////////////////////////////////////

static void menu_ID_DG1000Z_SYNC_OUTPUT()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (RIGOL_DG1000Z::SetupSyncOutput())
	{
		PrintToScreen("RIGOL_DG1000Z sync output setup correctly");
	}
	else
	{
		PrintToScreen("Error enabling RIGOL_DG1000Z sync output");
	}
}

static void menu_ID_DG1000Z_RUNSCRIPT()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	auto scriptFile = SelectFileToOpen(hwndMain);

	if (scriptFile.empty())
	{
		PrintToScreen("aborted");
		return;
	}

	RIGOL_DG1000Z::ProcessSCPIScript(scriptFile);
}

static void menu_ID_RIGOL_ENABLE_DUAL_CHANNEL()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	RigolDualChannelMode = true;
	MessageBoxA(NULL, "Rigol Dual Channel Mode Enabled.\nYOU HAVE TO WIRE THE CIRCUIT MANAULLY TO REFLECT THIS!", "Info", MB_OK | MB_ICONINFORMATION);
	PrintConnectionStatus();
}

static void menu_ID_RIGOL_DISABLE_DUAL_CHANNEL()
{
	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	RigolDualChannelMode = false;
	MessageBoxA(NULL, "Dual Channel Mode Disabled\nYOU HAVE TO WIRE THE CIRCUIT MANAULLY TO REFLECT THIS!", "Info", MB_OK | MB_ICONINFORMATION);
	PrintConnectionStatus();
}

//////////////////////////////////////////////////////
// BK_PRECISION_9801
//////////////////////////////////////////////////////

static void menu_ID_BK_PRECISION_9801_Enable()
{
	if (!BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	bool operationOK = BK_PRECISION_9801::EnableOutput();

	if (operationOK)
	{
		PrintToScreen("output enabled on BK_PRECISION_9801");
		PrintToScreen("VOLTAGE MAY STILL BE PRESENT!!!");
	}
	else
		PrintToScreen("error enabling output on BK_PRECISION_9801");
}

static void menu_ID_BK_PRECISION_9801_Disable()
{
	if (!BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	bool operationOK = BK_PRECISION_9801::DisableOutput();

	if (operationOK)
		PrintToScreen("output disabled on BK_PRECISION_9801");
	else
	{
		RIGOL_DG1000Z::DisableOutput();
		PrintToScreen("Error!! WARNING!! error disabling output on BK_PRECISION_9801");
		PrintToScreen("VOLTAGE MAY STILL BE PRESENT!!!");
	}
}

static void menu_ID_BK_PRECISION_9801_APPLYSINWAVE1()
{
	std::string voltageRMS;

	if (!BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	if (!GetInputValue("Enter BK_PRECISION_9801 voltage; 0 to 6 volts RMS", -1))
	{
		PrintToScreen("Invalid voltage");
		return;
	}

	voltageRMS = ReturnedInputValue_String;

	// convert voltsRMSAsString to a double
	double voltsRMS = std::stod(voltageRMS);

	auto BK_9801_MAX_VOLTS_RMS = 300;

	// verify is less than BK_9801_MAX_VOLTS_RMS
	if (voltsRMS > BK_9801_MAX_VOLTS_RMS)
	{
		PrintToScreen("BK_PRECISION_9801 error -- voltage exceeds maximum limit");
		PrintToScreen("this build of AUTOCAL_Lite only supports " + std::to_string(BK_9801_MAX_VOLTS_RMS) + " vRMS or less at this time");
		return;
	}

	bool operationOK = BK_PRECISION_9801::SetupToApplySINWave(false, voltageRMS);

	if (operationOK)
	{
		operationOK = BK_PRECISION_9801::EnableOutput();
	}

	if (operationOK)
		PrintToScreen("Sine wave is currently applied to the BK_PRECISION_9801 (output enabled)");
	else
	{
		BK_PRECISION_9801::DisableOutput();
		PrintToScreen("Error applying sine wave to BK_PRECISION_9801; output disabled");
	}
}

//////////////////////////////////////////////////////
// keithley menu
//////////////////////////////////////////////////////

static void menu_ID_KEITHLEY_PRINTVOLTAGE_1()
{

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	PrintToScreen("Keithley Voltage (VoltsRMS), GetVoltage(): " +
				  std::to_string(KEITHLEY::GetVoltage(hKeithley.handle)));
}

static void menu_ID_KEITHLEY_PRINTVOLTAGE_2()
{

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	PrintToScreen("Keithley Voltage (VoltsRMS), GetAverageVoltage(10): " +
				  std::to_string(KEITHLEY::GetAverageVoltage(hKeithley.handle, 10)));
}

static void menu_ID_KEITHLEY_PRINTVOLTAGE_3()
{

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	PrintToScreen("Keithley Voltage (VoltsRMS), GetVoltageForAutoRange(): " +
				  std::to_string(KEITHLEY::GetVoltageForAutoRange(hKeithley.handle)));
}

static void menu_KEITHLEY_RANGE0()
{
	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	KEITHLEY::RANGE0(hKeithley.handle);
}

static void menu_KEITHLEY_RANGE1()
{
	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	KEITHLEY::RANGE1(hKeithley.handle);
}

static void menu_KEITHLEY_RANGE10()
{
	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	KEITHLEY::RANGE10(hKeithley.handle);
}

static void menu_KEITHLEY_AUTO()
{
	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	KEITHLEY::RANGE_AUTO(hKeithley.handle);
}

static void async_KeithelyVoltage()
{
	if (KEITHLEY::VoltageOnKeithleyIsStable(hKeithley.handle))
		PrintToScreen("Keithley is stable");
	else
		PrintToScreen("Keithley is not stable");
}

static void menu_KEITHLEY_STABLE()
{
	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	PrintToScreen("Checking if Keithley is stable...");
	async_KeithelyVoltage();
}

//////////////////////////////////////////////////////
// Arduino menu
//////////////////////////////////////////////////////

static void menu_ID_ARDUINO_PRINTVERSION()
{

	SoftVer ArduinoFirmwareVer;

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (ARDUINO::GetVersion(hArduino.handle, &ArduinoFirmwareVer))
	{
		PrintToScreen("Arduino Firmware Version: " +
					  std::to_string(ArduinoFirmwareVer.Major) + "." + std::to_string(ArduinoFirmwareVer.Minor) + "." +
					  std::to_string(ArduinoFirmwareVer.Build) + "." + std::to_string(ArduinoFirmwareVer.Revision));
	}
	else
	{
		PrintToScreen("Error getting Arduino firmware version");
	}
}

static void menu_ID_ARDUINO_TURNON_LED()
{
	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (!ARDUINO::SetLEDs(hArduino.handle, true))
	{
		PrintToScreen("Cannot set LEDs on Arduino");
	}
}

static void menu_ID_ARDUINO_TURNOFF_LED()
{
	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (!ARDUINO::SetLEDs(hArduino.handle, false))
	{
		PrintToScreen("Cannot set LEDs on Arduino");
	}
}

void AsyncArduinoTripTest_LT(
	int LTPickupAMPS, float LT_Delay_Seconds, float AmpsRMSToApply)
{
	std::vector<LT_TRIP_TEST_RC::testParams> params;
	std::vector<LT_TRIP_TEST_RC::testResults> results;

	params.push_back({LTPickupAMPS, LT_Delay_Seconds, AmpsRMSToApply});

	if (LT_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		LT_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino long time trip test");
	}
}

void AsyncArduinoTripTest_Inst(
	int InstPickupAms, int InstQTPickup, bool QTEnabled, float AmpsRMSToApply)
{
	constexpr int TRIP_TIME_THRESHOLD_MS = 50;

	std::vector<INST_TRIP_TEST_RC::testParams> params;
	std::vector<INST_TRIP_TEST_RC::testResults> results;

	params.push_back({InstPickupAms, InstQTPickup, QTEnabled, AmpsRMSToApply, TRIP_TIME_THRESHOLD_MS});

	if (INST_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		INST_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino instantanious test");
	}
}

void AsyncArduinoTripTest_ST(
	int STPickupAMPS, float ST_Delay_Seconds, bool I2TEnabled, float AmpsRMSToApply)
{
	std::vector<ST_TRIP_TEST_RC::testParams> params;
	std::vector<ST_TRIP_TEST_RC::testResults> results;

	params.push_back({STPickupAMPS, ST_Delay_Seconds, I2TEnabled, AmpsRMSToApply});

	if (ST_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		ST_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino short time trip test");
	}
}

void AsyncArduinoTripTest_GF(
	int CTValue, int GFType, int GFPickup, float GFDelay, int GFSlope, float AmpsRMSToApply)
{
	std::vector<GF_TRIP_TEST_RC::testParams> params;
	std::vector<GF_TRIP_TEST_RC::testResults> results;

	params.push_back({CTValue, GFType, GFPickup, GFDelay, GFSlope, AmpsRMSToApply});

	if (GF_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		GF_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino short time trip test");
	}
}

void AsyncArduinoTripTest_Multi_LT(
	std::vector<LT_TRIP_TEST_RC::testParams> params)
{
	std::vector<LT_TRIP_TEST_RC::testResults> results;

	if (LT_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		LT_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino trip test; dumping incomplete test results...");
		LT_TRIP_TEST_RC::PrintResults(params, results);
	}
}

void AsyncArduinoTripTest_Multi_INST(
	std::vector<INST_TRIP_TEST_RC::testParams> params)
{
	std::vector<INST_TRIP_TEST_RC::testResults> results;

	if (INST_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		INST_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino trip test; dumping incomplete test results...");
		INST_TRIP_TEST_RC::PrintResults(params, results);
	}
}

void AsyncArduinoTripTest_Multi_ST(
	std::vector<ST_TRIP_TEST_RC::testParams> params)
{
	std::vector<ST_TRIP_TEST_RC::testResults> results;

	if (ST_TRIP_TEST_RC::CheckTripTime(
			hTripUnit.handle,
			hKeithley.handle, hArduino.handle, params, results))
	{

		ST_TRIP_TEST_RC::PrintResults(params, results);
	}
	else
	{
		PrintToScreen("Error running Arduino trip test; dumping incomplete test results...");
		ST_TRIP_TEST_RC::PrintResults(params, results);
	}
}

// this just lets us use the arduino for generic timing
// it simply measures the time between voltage applied and the actuator, and reports it
// it is up to the user to setup the trip unit settings, and apply a voltage source
void AsyncArduinoTripTest_Generic()
{

	ARDUINO::MsgRspTimingTestResults *msgRspTimingTestResults = nullptr;
	URCMessageUnion rsp = {0};
	bool retval;

	_ASSERT(hArduino.handle != INVALID_HANDLE_VALUE);

	// tell the Arduino to abort any timing tests in progress
	SendURCCommand(hArduino.handle,
				   ARDUINO::MSG_ABORT_TIMING_TEST,
				   ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

	if (!(GetURCResponse(hArduino.handle, &rsp) && MessageIsACK(&rsp)))
	{
		PrintToScreen("error sending MSG_ABORT_TIMING_TEST to the arduino");
	}

	// tell the Arduino to start timing
	SendURCCommand(hArduino.handle, ARDUINO::MSG_START_TIMING_TEST, ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);
	retval = GetURCResponse(hArduino.handle, &rsp) && MessageIsACK(&rsp);

	if (!retval)
	{
		PrintToScreen("cannot start timing test on the arduino");
		return;
	}

	PrintToScreen("Waiting indefinitely for trip unit to fire actuator...");
	PrintToScreen("Manually apply current to fire actuator now!");
	PrintToScreen("Waiting for test results to become valid....");
	PrintToScreen("Use menu item 'Abort Timing Test' under Arduino menu to abort");

	ArduinoAbortTimingTest = false;

	int retryCount = 0;

	while (!ArduinoAbortTimingTest)
	{
		Sleep(200);
		SendURCCommand(hArduino.handle, ARDUINO::MSG_GET_TIMING_TEST_RESULTS, ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

		// get MSG_RSP_TIMING_TEST_RESULTS
		retval = GetURCResponse(hArduino.handle, &rsp);

		if (!retval)
		{
			PrintToScreen("cannot get timing results from the Arduino");
			return;
		}

		// arduino will NAK us if the test is still in progress...
		if (MSG_NAK == rsp.msgHdr.Type)
			continue;

		retval = VerifyMessageIsOK(&rsp, ARDUINO::MSG_RSP_TIMING_TEST_RESULTS,
								   sizeof(ARDUINO::MsgRspTimingTestResults) - sizeof(MsgHdr));

		if (retval)
		{
			retryCount = 0;
			// check to see if the timing tests are valid yet...
			msgRspTimingTestResults = reinterpret_cast<ARDUINO::MsgRspTimingTestResults *>(&rsp);
			if (msgRspTimingTestResults->timingTestResults.ResultsAreValid)
				break;
		}
		else
		{
			PrintToScreen("cannot get timing results from the Arduino");
			if (retryCount++ == 3)
			{
				PrintToScreen("aborting....");
				return;
			}
		}
	}

	if (ArduinoAbortTimingTest)
	{
		PrintToScreen("Timing test aborted");
		return;
	}

	PrintToScreen("Time To Trip (ms) " + std::to_string(msgRspTimingTestResults->timingTestResults.elapsedTime));
}

static void menu_ID_ARDUINO_RUNTEST_LT()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!GetInputValue("Enter Long Time Pickup (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	int LTPickupValue = ReturnedInputValue_Float;

	if (!GetInputValue("Enter Long Time Delay (Seconds)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	float LT_Delay_Seconds = ReturnedInputValue_Float;

	if (!GetInputValue("Enter Test Set Amps (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	float DesiredAmps = ReturnedInputValue_Float;

	// this is the version of the arduino test using the RIGOL
	std::thread thread(
		AsyncArduinoTripTest_LT, LTPickupValue, LT_Delay_Seconds, DesiredAmps);
	thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_INST()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!GetInputValue("Enter Inst Pickup (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	int InstPickupAms = ReturnedInputValue_Float;

	if (!GetInputValue("Enter QT Inst Pickup (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	int QTInstPickup = ReturnedInputValue_Float;

	if (!GetInputValue("Enter 1 to enable Quick Trip", 1))
	{
		PrintToScreen("aborted");
		return;
	}

	bool QTEnabled = (ReturnedInputValue_String == "1");

	if (!GetInputValue("Enter Test Set Amps (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	float DesiredAmps = ReturnedInputValue_Float;

	// this is the version of the arduino test using the RIGOL
	// FIX std::thread thread(
	// FIX 	AsyncArduinoTripTest_Inst, InstPickupAms, QTInstPickup, QTEnabled, DesiredAmps);
	// FIX thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_ST()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (!GetInputValue("Enter Short Time Pickup (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	int LTPickupValue = ReturnedInputValue_Float;

	if (!GetInputValue("Enter Short Time Delay (Seconds)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	float LT_Delay_Seconds = ReturnedInputValue_Float;

	if (!GetInputValue("Enter 1 to enable I2T setting", 1))
	{
		PrintToScreen("aborted");
		return;
	}

	bool I2TEnabled = (ReturnedInputValue_String == "1");

	if (!GetInputValue("Enter Test Set Amps (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	float DesiredAmps = ReturnedInputValue_Float;

	// this is the version of the arduino test using the RIGOL
	// FIX std::thread thread(
	// FIX 	AsyncArduinoTripTest_ST, LTPickupValue, LT_Delay_Seconds, I2TEnabled, DesiredAmps);
	// FIX thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_GF()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	int CTValue;
	int GFType;
	int GFPickup;
	float GFDelay;
	int GFSlope;
	float AmpsRMSToApply;

	if (!GetInputValue("Enter CT Value", 800))
	{
		PrintToScreen("aborted");
		return;
	}

	CTValue = ReturnedInputValue_Float;

	if (!GetInputValue("Enter Ground Fault pickup (Amps)", 320))
	{
		PrintToScreen("aborted");
		return;
	}

	GFPickup = ReturnedInputValue_Float;

	if (!GetInputValue("Enter Ground Fault Delay (Seconds)", 0.2))
	{
		PrintToScreen("aborted");
		return;
	}

	GFDelay = ReturnedInputValue_Float;

	if (!GetInputValue("Enter value for GF Slope (0 = OFF, 1 = I2T, 2 = I5T)", 0))
	{
		PrintToScreen("aborted");
		return;
	}

	GFSlope = ReturnedInputValue_Float;

	if (GFSlope < 0 || GFSlope > 2)
	{
		PrintToScreen("Invalid GF Slope value; use: 0 = OFF, 1 = I2T, 2 = I5T");
		return;
	}

	if (!GetInputValue("Enter GF type (0 = none; 1 = Residential, 2 = Return)", 1))
	{
		PrintToScreen("aborted");
		return;
	}

	GFType = std::stoi(ReturnedInputValue_String);

	if (GFSlope < 0 || GFSlope > 2)
	{
		PrintToScreen("Invalid GF Type; use: 0 = none, 1 = residental, 2 = return");
		return;
	}

	if (!GetInputValue("Enter Test Set Amps (Amps RMS)", 1600))
	{
		PrintToScreen("aborted");
		return;
	}

	AmpsRMSToApply = ReturnedInputValue_Float;

	/*
	GF_TRIP_TEST_RC::testParams params = {CTValue, GFType, GFPickup, GFDelay, GFSlope, AmpsRMSToApply};
	GF_TRIP_TEST_RC::SetupTripUnit(hTripUnit.handle, params);

	auto expectedTripTimeMS = GF_TRIP_TEST_RC::TimeTimeToTripMS(
		params.CTRating,
		params.GFPickup,
		params.GFDelay,
		params.GFSlope,
		AmpsRMSToApply);

	return;
	*/

	// this is the version of the arduino test using the RIGOL
	// FIX std::thread thread(
	// FIX 	AsyncArduinoTripTest_GF, CTValue, GFType, GFPickup, GFDelay, GFSlope, AmpsRMSToApply);
	// FIX thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_MULTI_LT()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	auto scriptFile = SelectFileToOpen(hwndMain);
	if (scriptFile.empty())
	{
		PrintToScreen("aborted");
		return;
	}

	std::vector<LT_TRIP_TEST_RC::testParams> params;

	if (!LT_TRIP_TEST_RC::ReadTestFile(scriptFile, params))
	{
		PrintToScreen("Error reading test file");
		return;
	}

	std::thread thread(AsyncArduinoTripTest_Multi_LT, params);
	thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_MULTI_ST()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	auto scriptFile = SelectFileToOpen(hwndMain);
	if (scriptFile.empty())
	{
		PrintToScreen("aborted");
		return;
	}

	std::vector<ST_TRIP_TEST_RC::testParams> params;

	if (!ST_TRIP_TEST_RC::ReadTestFile(scriptFile, params))
	{
		PrintToScreen("Error reading test file");
		return;
	}

	std::thread thread(AsyncArduinoTripTest_Multi_ST, params);
	thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_MULTI_INST()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	auto scriptFile = SelectFileToOpen(hwndMain);
	if (scriptFile.empty())
	{
		PrintToScreen("aborted");
		return;
	}

	std::vector<INST_TRIP_TEST_RC::testParams> params;

	if (!INST_TRIP_TEST_RC::ReadTestFile(scriptFile, params))
	{
		PrintToScreen("Error reading test file");
		return;
	}

	// just update what was just put into the params to
	// update the tripTimeThresholdMS
	for (auto &param : params)
		param.tripTimeThresholdMS = 50; // hard-coded value

	std::thread thread(AsyncArduinoTripTest_Multi_INST, params);
	thread.detach();
}

static void menu_ID_ARDUINO_RUNTEST_MULTI_GF()
{
	HANDLE hHandleForTripUnit = INVALID_HANDLE_VALUE;

	/*

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	*/

	auto scriptFile = SelectFileToOpen(hwndMain);
	if (scriptFile.empty())
	{
		PrintToScreen("aborted");
		return;
	}

	std::vector<GF_TRIP_TEST_RC::testParams> params;

	if (!GF_TRIP_TEST_RC::ReadTestFile(scriptFile, params))
	{
		PrintToScreen("Error reading test file");
		return;
	}

	// just update what was just put into the params to
	// update the tripTimeThresholdMS
	for (auto &param : params)
	{

		auto expectedTripTimeMS = GF_TRIP_TEST_RC::TimeTimeToTripMS(
			param.CTRating,
			param.GFPickup,
			param.GFDelay,
			param.GFSlope,
			param.AmpsRMSToApply);

		PrintToScreen("Expected Trip Time (ms): " + std::to_string(expectedTripTimeMS));
		PrintToScreen("CT Rating: " + std::to_string(param.CTRating));
		PrintToScreen("GF Pickup: " + std::to_string(param.GFPickup));
		PrintToScreen("GF Delay: " + std::to_string(param.GFDelay));
		PrintToScreen("GF Slope: " + std::to_string(param.GFSlope));
		PrintToScreen("Amps RMS To Apply: " + std::to_string(param.AmpsRMSToApply));
	}
}

// the only thing needed here is the arduino
static void menu_ID_ARDUINO_GENERIC()
{
	if (hArduino.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Arduino not connected");
		return;
	}

	std::thread thread(AsyncArduinoTripTest_Generic);
	thread.detach();
}

static void menu_ID_ARDUINO_ABORT_TIMING()
{
	URCMessageUnion rsp = {0};

	PrintToScreen("Aborting Arduino timing test...");

	SendURCCommand(hArduino.handle,
				   ARDUINO::MSG_ABORT_TIMING_TEST,
				   ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

	if (GetURCResponse(hArduino.handle, &rsp) && MessageIsACK(&rsp))
		PrintToScreen("ARDUINO::MSG_ABORT_TIMING_TEST sent");
	else
		PrintToScreen("error sending ARDUINO::MSG_ABORT_TIMING_TEST");

	ArduinoAbortTimingTest = true;
}

//////////////////////////////////////////////////////
// FTDI menu
//////////////////////////////////////////////////////

static void menu_ID_COMMANDS_PROGRAM_FTDI()
{
	// to program the FTDI, we need to be directly connected to the FTDI chip
	if (hTripUnit.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	DialogBox(NULL, MAKEINTRESOURCE(ID_FTDI_POPUP), hwndMain, FTDI_DlgProc);
}

static void menu_ID_COMMANDS_DUMP_FTDI()
{
	// to program the FTDI, we need to be directly connected to the FTDI chip
	if (hTripUnit.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	DialogBox(NULL, MAKEINTRESOURCE(ID_FTDI_POPUP), hwndMain, FTDI_DlgProc);
}

//////////////////////////////////////////////////////
// ACPro2 menu
//////////////////////////////////////////////////////

static void menu_ID_ACPro2_50HZ()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (SetupTripUnitFrequency(hHandleForTripUnit, true))
	{
		PrintToScreen("Trip Unit set to 50Hz");
	}
	else
	{
		PrintToScreen("Error setting Trip Unit to 50Hz");
	}
}

static void menu_ID_ACPro2_60HZ()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (SetupTripUnitFrequency(hHandleForTripUnit, false))
		PrintToScreen("Trip Unit set to 60Hz");
	else
		PrintToScreen("Error setting Trip Unit to 60Hz");
}

static void menu_ID_ACPRO2_CLEAR_TRIP_HISTORY()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (SendClearTripHistory(hHandleForTripUnit))
		PrintToScreen("Trip History Cleared");
	else
		PrintToScreen("Error clearing trip history");
}

static void async_SetQTState(bool Enable)
{

	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (!SendSetQTStatus(hHandleForTripUnit, Enable))
	{
		if (Enable)
			PrintToScreen("Error enabling QT");
		else
			PrintToScreen("Error disabling QT");

		return;
	}

	PrintToScreen("waiting 5 seconds for QT to enable/disable...");
	Sleep(1000 * 5);

	// grab LEDs; make sure trip unit reports QT is in the right state
	URCMessageUnion rsp = {0};
	SendURCCommand(hHandleForTripUnit, MSG_GET_LEDS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	GetURCResponse(hHandleForTripUnit, &rsp);

	// NOTE: sizeof(MsgRspLEDs) somehow is not packing correctly?
	//		 it seems to be 5 bytes + header
	//		 but trip unit says length is 6 bytes + header

	if (!VerifyMessageIsOK(&rsp, MSG_RSP_LEDS, 16 - sizeof(MsgHdr)))
	{
		PrintToScreen("Error getting LED status from trip unit");
		return;
	}

	if (Enable)
	{
		// make sure trip unit reports that QT is enabled
		if (rsp.msgRspLEDs.State[LED_QT])
			PrintToScreen("trip unit reports QT enabled as expected");
		else
			PrintToScreen("trip unit does not report correct QT state!");
	}
	else
	{
		// make sure trip unit reports that QT is disabled
		if (!rsp.msgRspLEDs.State[LED_QT])
			PrintToScreen("trip unit reports QT disabled as expected");
		else
			PrintToScreen("trip unit does not report correct QT state!");
	}
}

static void menu_ID_ACPRO2_ENABLE_QT(bool Enable)
{
	// FIX (async_SetQTState, Enable);
	// FIX thread.detach();
}

static void menu_ID_ACPRO2_PROGRAM_SERIAL_NUMBERS()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	// grab existing serial numbers into global variables
	if (!GetSerialNumber(
			hHandleForTripUnit,
			tu_serial, sizeof(tu_serial)))
	{
		PrintToScreen("Error getting serial numbers from TU/LD");
		return;
	}

	DialogBox(NULL, MAKEINTRESOURCE(ID_SERIALNUM), hwndMain, SerialNum_DlgProc);
}

static void menu_ID_ACPRO2_PRINT_VERSION()
{
	SoftVer LD_Version = {0};
	SoftVer TU_Version = {0};
	URCMessageUnion rsp = {0};

	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	SendURCCommand(hHandleForTripUnit, MSG_GET_SW_VER, ADDR_TRIP_UNIT, ADDR_INFOPRO_AC);
	GetURCResponse(hHandleForTripUnit, &rsp);

	if (rsp.msgHdr.Type != MSG_RSP_SW_VER)
	{
		PrintToScreen("cannot determine LD firmware version");
		return;
	}

	LD_Version.Major = rsp.msgRspSoftVer.ver.Major;
	LD_Version.Minor = rsp.msgRspSoftVer.ver.Minor;
	LD_Version.Build = rsp.msgRspSoftVer.ver.Build;
	LD_Version.Revision = rsp.msgRspSoftVer.ver.Revision;

	std::ostringstream oss1;
	oss1 << "LD firmware version: " << std::to_string(LD_Version.Major) << "."
		 << std::to_string(LD_Version.Minor) << "."
		 << std::to_string(LD_Version.Build) << "."
		 << std::to_string(LD_Version.Revision) << "\n";

	PrintToScreen(oss1.str());

	rsp = {0};

	SendURCCommand(hHandleForTripUnit, MSG_GET_SW_VER, ADDR_DISP_LOCAL, ADDR_INFOPRO_AC);
	GetURCResponse(hHandleForTripUnit, &rsp);

	if (rsp.msgHdr.Type != MSG_RSP_SW_VER)
	{
		PrintToScreen("cannot determine LD firmware version");
		return;
	}

	TU_Version.Major = rsp.msgRspSoftVer.ver.Major;
	TU_Version.Minor = rsp.msgRspSoftVer.ver.Minor;
	TU_Version.Build = rsp.msgRspSoftVer.ver.Build;
	TU_Version.Revision = rsp.msgRspSoftVer.ver.Revision;

	std::ostringstream oss2;
	oss2 << "TU firmware version: " << std::to_string(LD_Version.Major) << "."
		 << std::to_string(LD_Version.Minor) << "."
		 << std::to_string(LD_Version.Build) << "."
		 << std::to_string(LD_Version.Revision) << "\n";

	PrintToScreen(oss2.str());
}

static void menu_ID_ACPRO2_SETTINGS_FROM_TXT()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	// prompt user for txt file
	auto filename = SelectFileToOpen(hwndMain);
	if (filename.empty())
	{
		PrintToScreen("No file selected");
		return;
	}

	auto f = [](SystemSettings4 *sysSettings, DeviceSettings4 *devSettings4)
	{
		// no changes
		return false;
	};

	bool SettingsUpdatedOnTripUnit = false;

	if (!SetSystemAndDeviceSettingsFromFile(
			hHandleForTripUnit, filename, SettingsUpdatedOnTripUnit, f))
	{
		PrintToScreen("Error setting settings from file");
		return;
	}

	if (SettingsUpdatedOnTripUnit)
	{
		// Wait 2 seconds for the trip unit to reboot
		Sleep(2000);

		PrintToScreen("Updated settings:");
		menu_ID_ACPRO2_DUMP_SETTINGS();
	}
	else
	{
		PrintToScreen("No changes");
	}
}

//////////////////////////////////////////////////////
// ACPro2-RC menu
//////////////////////////////////////////////////////

static void menu_ID_RC_FULL_CAL(bool shouldDoLoopingCal)
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	// read from the INI file what params we used last time
	ACPRO2_RG::ReadFullCalibrationParamsFromINI(iniFile, fullRCCalibrationParams);

	// first popup the dialog box to get the calibration parameters
	INT_PTR result = DialogBoxA(NULL, MAKEINTRESOURCE(ID_RC_FULL_CAL), hwndMain, RC_FULLCAL_DlgProc);

	if (result == -1)
	{
		DWORD error = GetLastError();
		PrintToScreen(std::string("Error creating dialog box ID_RC_FULL_CAL; error code: ") + std::to_string(error));
		return;
	}

	if (!fullRCCalibrationParams.valid)
	{
		PrintToScreen("aborted");
		return;
	}

	// save the params to the INI file for next time
	ACPRO2_RG::WriteFullCalibrationParamsToINI(iniFile, fullRCCalibrationParams);

	if (fullRCCalibrationParams.use_rigol_dg1000z && !RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	if (fullRCCalibrationParams.use_bk_precision_9801 && !BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	if (!fullRCCalibrationParams.use_rigol_dg1000z && !fullRCCalibrationParams.use_bk_precision_9801)
	{
		PrintToScreen("RIGOL_DG1000Z or BK_PRECISION_9801 must be selected");
		return;
	}
	Async_FullACPro2_RG(shouldDoLoopingCal);
}

static void menu_ID_RC_STEP_RIGOL_DG1000Z(bool Use50hz)
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z not connected");
		return;
	}

	MessageBoxA(hwndMain, "Make Sure trip unit frequency is set to match the frequency you've selected!", "Step Test", MB_OK);

	std::thread thread(stepRIGOL, hHandleForTripUnit, hKeithley.handle, Use50hz);
	thread.detach();
}

static void menu_ID_RC_STEP_BK9801(bool Use50hz)
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	MessageBoxA(hwndMain, "Make Sure trip unit frequency is set to match the frequency you've selected!", "Step Test", MB_OK);

	stepBK9801(hHandleForTripUnit, hKeithley.handle, Use50hz);
}

static void menu_ID_RC_DUMP_CAL_PARAMS()
{
	PrintToScreen("High Gains");
	PrintToScreen("		cal point  | cmd to rigol ");
	PrintToScreen("		-----------|----------------");
	PrintToScreen("		0.5        | 1.4736000 vRMS");
	PrintToScreen("		1.0        | 0.7186800 vRMS");
	PrintToScreen("		1.5        | 0.4788375 vRMS");
	PrintToScreen("		2.0        | 0.3593400 vRMS");
	PrintToScreen("Low Gains");
	PrintToScreen("		all  	   | 7 vRMS ");
}

static void WarnVoltageIsPresent()
{
	MessageBoxA(hwndMain, "WARNING: VOLTAGE MAY BE PRESENT!!!", "WARNING", MB_OK);
}

static void TurnOffVoltageSource(bool use_bk_precision_9801_not_rigol_dg1000z)
{
	if (use_bk_precision_9801_not_rigol_dg1000z)
	{
		// be redundant in our attempts to turn off the voltage source
		for (int i = 0; i < 2; i++)
		{
			if (!BK_PRECISION_9801::DisableOutput())
			{
				PrintToScreen("BK_PRECISION_9801::EnableOutput() failed");
			}

			Sleep(1000);
		}
	}
	else
	{
		// be redundant in our attempts to turn off the voltage source
		for (int i = 0; i < 2; i++)
		{
			if (!RIGOL_DG1000Z::DisableOutput())
			{
				PrintToScreen("RIGOL_DG1000Z::EnableOutput() failed");
			}

			Sleep(1000);
		}
	}
}

static std::string CalcResult(uint32_t valueFromTripUnit, double keithleyReading)
{
	// Calc Keithley * 3800
	double KeithleyX3800 = (uint32_t)(keithleyReading * 3800);

	// Calc percentage difference between (Keithley * 3800) & value from trip unit
	double percentageDifference = 100 * (KeithleyX3800 - valueFromTripUnit) / KeithleyX3800;

	return std::to_string(valueFromTripUnit) + ", " + FloatToString(percentageDifference, 3);
}

static void stepVoltageSource(HANDLE hTripUnit, HANDLE hKeithley, bool use_bk_precision_9801_not_rigol_dg1000z, bool do50hz)
{
	URCMessageUnion rsp;

	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	PrintToScreen("Commanded vRMS, Keithley vRMS, Keithley * 3800, TripUnit_A aRMS, TripUnit_A Error%, TripUnit_B aRMS, TripUnit_B Error%, TripUnit_C aRMS, TripUnit_C Error%, TripUnit_N aRMS, TripUnit_N Error%");

	// Declare and initialize the vector with the given data
	std::vector<double> test_points_voltsRMS;

	// BK Precision can produce higher voltage...
	if (use_bk_precision_9801_not_rigol_dg1000z)
		test_points_voltsRMS = {
			0.001, 0.0015, 0.003, 0.006, 0.012, 0.025, 0.050, 0.100, 0.200, 0.300,
			0.400, 0.500, 0.600, 0.700, 0.800, 0.900, 1.000, 1.100, 1.200, 1.300,
			1.500, 1.600, 1.700, 1.800, 1.900, 2.000, 2.100, 2.200, 2.300, 2.400,
			2.500, 3.000, 3.500, 4.000, 4.500, 5.000, 5.500, 6.000, 6.500, 7.000,
			8.0, 9.0, 10.0, 11, 12, 13, 14, 15, 16, 17, 18,
			19, 20, 21, 22, 23, 24, 25, 26, 27};
	else
		test_points_voltsRMS = {
			0.001, 0.0015, 0.003, 0.006, 0.012, 0.025, 0.050, 0.100, 0.200, 0.300,
			0.400, 0.500, 0.600, 0.700, 0.800, 0.900, 1.000, 1.100, 1.200, 1.300,
			1.500, 1.600, 1.700, 1.800, 1.900, 2.000, 2.100, 2.200, 2.300, 2.400,
			2.500, 3.000, 3.500, 4.000, 4.500, 5.000, 5.500, 6.000, 6.500, 7.000};

	for (auto voltsRMS : test_points_voltsRMS)
	{

		// command the voltage
		if (use_bk_precision_9801_not_rigol_dg1000z)
		{
			if (!BK_PRECISION_9801::SetupToApplySINWave(do50hz, std::to_string(voltsRMS)))
			{
				PrintToScreen("BK_PRECISION_9801::SetupToApplySINWave() failed; aborting");
				return;
			}
		}
		else
		{
			if (!RIGOL_DG1000Z::SetupToApplySINWave(do50hz, std::to_string(voltsRMS)))
			{
				PrintToScreen("RIGOL_DG1000Z::SetupToApplySINWave() failed; aborting");
				return;
			}
		}

		// only enable the output the first time
		// (after that, each time through the loop we will just change the voltage)
		if (voltsRMS == test_points_voltsRMS[0])
		{
			if (use_bk_precision_9801_not_rigol_dg1000z)
			{
				if (!BK_PRECISION_9801::EnableOutput())
				{
					PrintToScreen("BK_PRECISION_9801::EnableOutput() failed; aborting");
					return;
				}
			}
			else
			{
				if (!RIGOL_DG1000Z::EnableOutput())
				{
					PrintToScreen("RIGOL_DG1000Z::EnableOutput() failed; aborting");
					return;
				}
			}
		}

		// Read Keithley
		double keithleyReading = KEITHLEY::GetVoltageForAutoRange(hKeithley);

		// Grab MSG_GET_DYNAMICS phase a from trip unit
		SendURCCommand(hTripUnit, MSG_GET_DYNAMICS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		bool retval = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_RSP_DYNAMICS_4, sizeof(MsgRspDynamics4) - sizeof(MsgHdr));

		if (!retval)
		{
			PrintToScreen("Test aborted; MSG_GET_DYNAMICS failed");
			TurnOffVoltageSource(use_bk_precision_9801_not_rigol_dg1000z);
			return;
		}

		// Print the results
		std::string result = std::to_string(voltsRMS) + ", " +
							 std::to_string(keithleyReading) + ", " +
							 std::to_string(int32_t(keithleyReading * 3800)) + ", " +
							 CalcResult(rsp.msgRspDynamics4.Dynamics.Measurements.Ia, keithleyReading) + ", " +
							 CalcResult(rsp.msgRspDynamics4.Dynamics.Measurements.Ib, keithleyReading) + ", " +
							 CalcResult(rsp.msgRspDynamics4.Dynamics.Measurements.Ic, keithleyReading) + ", " +
							 CalcResult(rsp.msgRspDynamics4.Dynamics.Measurements.In, keithleyReading);

		PrintToScreen(result);

		// wait 1 second between points
		Sleep(1000);
	}

	// we are done; make sure voltage is off
	TurnOffVoltageSource(use_bk_precision_9801_not_rigol_dg1000z);
}

static void stepRIGOL(HANDLE hTripUnit, HANDLE hKeithley, bool do50hz)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	stepVoltageSource(hTripUnit, hKeithley, false, do50hz);
	// redudantly make sure voltage is off
	TurnOffVoltageSource(false);
}

static void stepBK9801(HANDLE hTripUnit, HANDLE hKeithley, bool do50hz)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	stepVoltageSource(hTripUnit, hKeithley, true, do50hz);
	// redudantly make sure voltage is off
	TurnOffVoltageSource(true);
}

// command one voltage from our voltage source, either the BK Precision 9801 or the
// Rigol DG1000Z, and monitor the keithley and the trip unit
static void CheckForVoltageDrift(
	HANDLE hTripUnit, HANDLE hKeithley,
	bool use_bk_precision_9801_not_rigol_dg1000z,
	double voltsRMS)
{
	URCMessageUnion rsp;
	double keithleyReading;
	int valueFromTripUnit;

	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	PrintToScreen("Commanded vRMS, Keithley vRMS, Trip Unit AmpsRMS");
	PrintToScreen("------------------------------------------------");

	// command the voltage
	if (use_bk_precision_9801_not_rigol_dg1000z)
	{
		if (!BK_PRECISION_9801::SetupToApplySINWave(false, std::to_string(voltsRMS)))
		{
			PrintToScreen("BK_PRECISION_9801::SetupToApplySINWave() failed; aborting");
			return;
		}
		if (!BK_PRECISION_9801::EnableOutput())
		{
			PrintToScreen("BK_PRECISION_9801::EnableOutput() failed; aborting");
			return;
		}
	}
	else
	{
		if (!RIGOL_DG1000Z::SetupToApplySINWave(false, std::to_string(voltsRMS)))
		{
			PrintToScreen("RIGOL_DG1000Z::SetupToApplySINWave() failed; aborting");
			return;
		}
		if (!RIGOL_DG1000Z::EnableOutput())
		{
			PrintToScreen("RIGOL_DG1000Z::EnableOutput() failed; aborting");
			return;
		}
	}

	KEITHLEY::RANGE10(hKeithley);

	constexpr int num_reps = 60 * 30; // 30 minutes

	// before we get started, let the voltage source settle
	Sleep(3000);

	std::ofstream file("c:\\tmp\\tmp.tmp");
	if (!file)
	{
		PrintToScreen("Error opening file");
		return;
	}

	for (int i = 0; i < num_reps; ++i)
	{
		// Read Keithley
		keithleyReading = KEITHLEY::GetVoltage(hKeithley);

		// Grab MSG_GET_DYNAMICS phase a from trip unit
		SendURCCommand(hTripUnit, MSG_GET_DYNAMICS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		bool retval = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_RSP_DYNAMICS_4, sizeof(MsgRspDynamics4) - sizeof(MsgHdr));
		if (!retval)
		{
			PrintToScreen("Test aborted; MSG_GET_DYNAMICS failed");
			return;
		}

		valueFromTripUnit = rsp.msgRspDynamics4.Dynamics.Measurements.Ia;

		std::string result = rightjustify(14, std::to_string(voltsRMS)) + ", " +
							 rightjustify(13, std::to_string(keithleyReading)) + ", " +
							 rightjustify(17, std::to_string(valueFromTripUnit));

		file << result << std::endl;

		PrintToScreen(result);
		Sleep(1000);
	}

	file.close();

	// we are done; make sure voltage is off
	TurnOffVoltageSource(use_bk_precision_9801_not_rigol_dg1000z);
}

static void doDG1000Z_DriftTest(HANDLE hTripUnit, HANDLE hKeithley)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	// this value is hard-coded; 7 vRMS is what we use for our high gain
	// calibrations
	constexpr double voltsRMS = 7.0;

	CheckForVoltageDrift(hTripUnit, hKeithley, false, voltsRMS);

	// redudantly make sure voltage is off
	TurnOffVoltageSource(false);
}

static void doBK9801_DriftTest(HANDLE hTripUnit, HANDLE hKeithley)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);
	_ASSERT(hKeithley != INVALID_HANDLE_VALUE);

	// this value is hard-coded; 7 vRMS is what we use for our high gain
	// calibrations
	constexpr double voltsRMS = 7.0;

	CheckForVoltageDrift(hTripUnit, hKeithley, true, voltsRMS);

	// redudantly make sure voltage is off
	TurnOffVoltageSource(true);
}

static void menu_ID_RC_RIGOL_DG1000Z_DRIFT_TEST()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!RIGOL_DG1000Z_Connected)
	{
		PrintToScreen("RIGOL_DG1000Z_Connected not connected");
		return;
	}

	doDG1000Z_DriftTest(hHandleForTripUnit, hKeithley.handle);
}

static void menu_ID_RC_BK9801_DRIFT_TEST()
{
	HANDLE hHandleForTripUnit;

	if (INVALID_HANDLE_VALUE == (hHandleForTripUnit = GetHandleForTripUnit()))
	{
		PrintToScreen("Trip Unit not connected");
		return;
	}

	if (hKeithley.handle == INVALID_HANDLE_VALUE)
	{
		PrintToScreen("Keithley not connected");
		return;
	}

	if (!BK_PRECISION_9801_Connected)
	{
		PrintToScreen("BK_PRECISION_9801 not connected");
		return;
	}

	doBK9801_DriftTest(hHandleForTripUnit, hKeithley.handle);
}

static void LaunchProductionLoop()
{
	std::thread thread(ACPRO2_RG::DoProductionLoop);
	thread.detach();
}

void ShowModelessDialog(HWND hwndParent, LPCTSTR text)
{
	if (hModelessDlg == NULL)
	{
		hModelessDlg =
			CreateDialogParam(
				GetModuleHandle(NULL),
				MAKEINTRESOURCE(IDD_CUSTOM_DIALOG),
				hwndParent,
				ACPRO2_RG::ProductionDialogProc, (LPARAM)text);
		EnableWindow(hwndParent, FALSE); // Disable the parent window
		ShowWindow(hModelessDlg, SW_SHOW);
	}
}

static void processCommands(HWND hwnd, WPARAM wParam)
{
	MSG msg = {0};
	HWND hDlg;

	switch (LOWORD(wParam))
	{

		//////////////////////////////////////////////////////
		// file menu
		//////////////////////////////////////////////////////

	case ID_FILE_SAVE:
		menu_ID_FILE_SAVE();
		break;

	case ID_FILE_CLEARSCREEN:
		menu_ID_FILE_CLEARSCREEN();
		break;

	case ID_FILE_EXIT:
		menu_ID_FILE_EXIT();
		break;

		//////////////////////////////////////////////////////
		// connection menu
		//////////////////////////////////////////////////////

	case ID_CONNECTION_CONNECT_KEITHLEY:
		menu_ID_CONNECTION_CONNECT_KEITHLEY();
		break;

	case ID_CONNECTION_DISCONNECT_KEITHLEY:
		menu_ID_CONNECTION_DISCONNECT_KEITHLEY();
		break;

	case ID_CONNECTION_FIND_KEITHLEY:
		menu_ID_CONNECTION_FIND_KEITHLEY();
		break;

	case ID_CONNECTION_CONNECT_RIGOL_DG1000Z:
		menu_ID_CONNECTION_CONNECT_RIGOL_DG1000Z();
		break;

	case ID_CONNECTION_DISCONNECT_RIGOL_DG1000Z:
		menu_ID_CONNECTION_DISCONNECT_RIGOL_DG1000Z();
		break;

	case ID_CONNECTION_CONNECT_TRIPUNIT:
		menu_ID_CONNECTION_CONNECT_TRIPUNIT();
		break;

	case ID_CONNECTION_DISCONNECT_TRIPUNIT:
		menu_ID_CONNECTION_DISCONNECT_TRIPUNIT();
		break;

	case ID_CONNECTION_FIND_TRIPUNIT:
		menu_ID_CONNECTION_FIND_TRIPUNIT();
		break;

	case ID_CONNECTION_CONNECT_BK_PRECISION_9801:
		menu_ID_CONNECTION_CONNECT_BK_PRECISION_9801();
		break;

	case ID_CONNECTION_DISCONNECT_BK_PRECISION_9801:
		menu_ID_CONNECTION_DISCONNECT_BK_PRECISION_9801();
		break;

	case ID_CONNECTION_CONNECT_ARDUINO:
		menu_ID_CONNECTION_CONNECT_ARDUINO();
		break;

	case ID_CONNECTION_DISCONNECT_ARDUINO:
		menu_ID_CONNECTION_DISCONNECT_ARDUINO();
		break;

	case ID_CONNECTION_FIND_ARUINO:
		menu_ID_CONNECTION_FIND_ARUINO();

	case ID_CONNECTION_ENUMERATECOMMPORTS:
		menu_ID_CONNECTION_ENUMERATECOMMPORTS();
		break;

	case ID_CONNECTION_PRINTCONNECTIONSTATUS:
		menu_ID_CONNECTION_PRINTCONNECTIONSTATUS();
		break;

	case ID_FIND_DEVICES_2:
		menu_ID_FIND_DEVICES_2();
		break;

		//////////////////////////////////////////////////////
		// Production
		//////////////////////////////////////////////////////

	case ID_PRODUCTION_LOOP:
		// note: this has to be done from this thread i think
		ShowModelessDialog(hwnd, "");
		// launch the production loop as a separate thread
		LaunchProductionLoop();
		break;

		//////////////////////////////////////////////////////
		// Database
		//////////////////////////////////////////////////////

	case ID_DUMP_DB_LT_THRESHOLD:
		menu_ID_DUMP_DB_LT_THRESHOLD();
		break;

	case ID_DUMP_DB_PERSONALITY:
		menu_ID_DUMP_DB_PERSONALITY();
		break;

		//////////////////////////////////////////////////////
		// AC-PRO-2
		//////////////////////////////////////////////////////

	case ID_ACPRO2_NEGOTIATE_BAUD:
		menu_ID_ACPRO2_NEGOTIATE_BAUD();
		break;

	case ID_ACPro2_UNCALIBRATE:
		menu_ID_ACPro2_UNCALIBRATE();
		break;

	case ID_CALIBRATION_SET_TO_DEFAULT:
		menu_ID_CALIBRATION_SET_TO_DEFAULT();

	case ID_ACPro2_INITCAL:
		menu_ID_ACPro2_INITCAL();
		break;

	case ID_ACPro2_GET_MSG_GET_CAL:
		menu_ID_ACPro2_GET_MSG_GET_CAL();
		break;

	case ID_RC_ARBITRARY_CAL:
		// dump the cal points out to the screen, just so we have them at hand...
		menu_ID_RC_DUMP_CAL_PARAMS();
		menu_ID_RC_ARBITRARY_CAL();
		break;

	case ID_RC_FULL_CAL:
		menu_ID_RC_FULL_CAL(false);
		break;

	case ID_RC_FULL_CAL_LOOP:
		menu_ID_RC_FULL_CAL(true);
		break;

		// test routines

	case ID_RC_STEP_RIGOL_DG1000Z_50:
		menu_ID_RC_STEP_RIGOL_DG1000Z(true);
		break;

	case ID_RC_STEP_RIGOL_DG1000Z_60:
		menu_ID_RC_STEP_RIGOL_DG1000Z(false);
		break;

	case ID_RC_STEP_BK9801_50:
		menu_ID_RC_STEP_BK9801(true);
		break;

	case ID_RC_STEP_BK9801_60:
		menu_ID_RC_STEP_BK9801(false);
		break;

	case ID_RC_RIGOL_DG1000Z_DRIFT_TEST:
		menu_ID_RC_RIGOL_DG1000Z_DRIFT_TEST();
		break;

	case ID_RC_BK9801_DRIFT_TEST:
		menu_ID_RC_BK9801_DRIFT_TEST();
		break;

	case ID_RC_DUMP_CAL_PARAMS:
		menu_ID_RC_DUMP_CAL_PARAMS();
		break;

	case ID_ACPRO2_DUMP_PERSONALITY:
		menu_ID_ACPRO2_DUMP_PERSONALITY();
		break;

	case ID_ACPRO2_DUMP_SETTINGS:
		menu_ID_ACPRO2_DUMP_SETTINGS();
		break;

	case ID_ACPRO2_DUMP_STATUS:
		menu_ID_ACPRO2_DUMP_STATUS();
		break;

	case ID_ACPRO2_DUMP_TRIP_HISTORY:
		menu_ID_ACPRO2_DUMP_TRIP_HISTORY();
		break;

	case ID_ACPRO2_DUMP_DYNAMICS:
		menu_ID_ACPRO2_DUMP_DYNAMICS();
		break;

	case ID_ACPro2_SAVE_TO_FLASH:
		menu_ID_ACPro2_SAVE_TO_FLASH();
		break;

	case ID_ACPro2_FORMAT_FRAM:
		menu_ID_ACPro2_FORMAT_FRAM();
		break;

	case ID_ACPRO2_TRIP_UNIT_UI:
		menu_ID_ACPRO2_TRIP_UNIT_UI();
		break;

	case ID_ACPro2_50HZ:
		menu_ID_ACPro2_50HZ();
		break;

	case ID_ACPro2_60HZ:
		menu_ID_ACPro2_60HZ();
		break;

	case ID_ACPRO2_CLEAR_TRIP_HISTORY:
		menu_ID_ACPRO2_CLEAR_TRIP_HISTORY();
		break;

	case ID_ACPRO2_PROGRAM_SERIAL_NUMBERS:
		menu_ID_ACPRO2_PROGRAM_SERIAL_NUMBERS();
		break;

	case ID_ACPRO2_ENABLE_QT:
		menu_ID_ACPRO2_ENABLE_QT(true);
		break;

	case ID_ACPRO2_DISABLE_QT:
		menu_ID_ACPRO2_ENABLE_QT(false);
		break;

	case ID_ACPRO2_PRINT_VERSION:
		menu_ID_ACPRO2_PRINT_VERSION();
		break;

	case ID_ACPRO2_SETTINGS_FROM_TXT:
		menu_ID_ACPRO2_SETTINGS_FROM_TXT();
		break;

		//////////////////////////////////////////////////////
		// RIGOL_DG1000Z menu
		/////////////////////////////////////////////////////

	case ID_SIGNALGENERATOR_ENABLEOUTPUT:
		menu_ID_SIGNALGENERATOR_ENABLEOUTPUT();
		break;

	case ID_SIGNALGENERATOR_DISABLEOUTPUT:
		menu_ID_SIGNALGENERATOR_DISABLEOUTPUT();
		break;

	case ID_SIGNALGENERATOR_APPLYSINWAVE1:
		menu_ID_SIGNALGENERATOR_APPLYSINWAVE1();
		break;

	case ID_DG1000Z_SYNC_OUTPUT:
		menu_ID_DG1000Z_SYNC_OUTPUT();
		break;

	case ID_DG1000Z_RUNSCRIPT:
		menu_ID_DG1000Z_RUNSCRIPT();
		break;

	case ID_RIGOL_ENABLE_DUAL_CHANNEL:
		menu_ID_RIGOL_ENABLE_DUAL_CHANNEL();
		break;

	case ID_RIGOL_DISABLE_DUAL_CHANNEL:
		menu_ID_RIGOL_DISABLE_DUAL_CHANNEL();

	case ID_SIGNALGENERATOR_ENABLEOUTPUT_2:
		menu_ID_SIGNALGENERATOR_ENABLEOUTPUT_2();
		break;

	case ID_SIGNALGENERATOR_DISABLEOUTPUT_2:
		menu_ID_SIGNALGENERATOR_DISABLEOUTPUT_2();
		break;

	case ID_SIGNALGENERATOR_APPLYSINWAVE_2:
		menu_ID_SIGNALGENERATOR_APPLYSINWAVE_2();
		break;

	case ID_RIGOL_PHASE2_PHASE_180:
		menu_ID_RIGOL_PHASE2_PHASE_180();
		break;

	case ID_SETUP_SYNC_CHANNELS:
		menu_ID_SETUP_SYNC_CHANNELS();
		break;

		//////////////////////////////////////////////////////
		// BK PRECISION 9801
		/////////////////////////////////////////////////////

	case ID_BK_PRECISION_9801_Enable:
		menu_ID_BK_PRECISION_9801_Enable();
		break;

	case ID_BK_PRECISION_9801_Disable:
		menu_ID_BK_PRECISION_9801_Disable();
		break;

	case ID_BK_PRECISION_9801_APPLYSINWAVE1:
		menu_ID_BK_PRECISION_9801_APPLYSINWAVE1();
		break;

		//////////////////////////////////////////////////////
		// keithley menu
		//////////////////////////////////////////////////////

	case ID_KEITHLEY_PRINTVOLTAGE_1:
		menu_ID_KEITHLEY_PRINTVOLTAGE_1();
		break;

	case ID_KEITHLEY_PRINTVOLTAGE_2:
		menu_ID_KEITHLEY_PRINTVOLTAGE_2();
		break;

	case ID_KEITHLEY_PRINTVOLTAGE_3:
		menu_ID_KEITHLEY_PRINTVOLTAGE_3();
		break;

	case ID_KEITHLEY_RANGE0:
		menu_KEITHLEY_RANGE0();
		break;

	case ID_KEITHLEY_RANGE1:
		menu_KEITHLEY_RANGE1();
		break;

	case ID_KEITHLEY_RANGE10:
		menu_KEITHLEY_RANGE10();
		break;

	case ID_KEITHLEY_AUTO:
		menu_KEITHLEY_AUTO();
		break;

	case ID_KEITHLEY_STABLE:
		menu_KEITHLEY_STABLE();
		break;

	//////////////////////////////////////////////////////
	// Arduino menu
	//////////////////////////////////////////////////////
	case ID_ARDUINO_PRINTVERSION:
		menu_ID_ARDUINO_PRINTVERSION();
		break;

	case ID_ARDUINO_TURNON_LED:
		menu_ID_ARDUINO_TURNON_LED();
		break;

	case ID_ARDUINO_TURNOFF_LED:
		menu_ID_ARDUINO_TURNOFF_LED();
		break;

	case ID_ARDUINO_GENERIC:
		menu_ID_ARDUINO_GENERIC();
		break;

	case ID_ARDUINO_RUNTEST_LT:
		menu_ID_ARDUINO_RUNTEST_LT();
		break;

	case ID_ARDUINO_RUNTEST_MULTI_LT:
		menu_ID_ARDUINO_RUNTEST_MULTI_LT();
		break;

	case ID_ARDUINO_RUNTEST_ST:
		menu_ID_ARDUINO_RUNTEST_ST();
		break;

	case ID_ARDUINO_RUNTEST_MULTI_ST:
		menu_ID_ARDUINO_RUNTEST_MULTI_ST();
		break;

	case ID_ARDUINO_RUNTEST_INST:
		menu_ID_ARDUINO_RUNTEST_INST();
		break;

	case ID_ARDUINO_RUNTEST_MULTI_INST:
		menu_ID_ARDUINO_RUNTEST_MULTI_INST();
		break;

	case ID_ARDUINO_RUNTEST_GF:
		menu_ID_ARDUINO_RUNTEST_GF();
		break;

	case ID_ARDUINO_RUNTEST_MULTI_GF:
		menu_ID_ARDUINO_RUNTEST_MULTI_GF();
		break;

	case ID_ARDUINO_ABORT_TIMING:
		menu_ID_ARDUINO_ABORT_TIMING();
		break;

		//////////////////////////////////////////////////////
		// FTDI menu
		//////////////////////////////////////////////////////

	case ID_COMMANDS_PROGRAM_FTDI:
		menu_ID_COMMANDS_PROGRAM_FTDI();
		break;

		/*

	case ID_COMMANDS_DUMP_FTDI:
		menu_ID_COMMANDS_DUMP_FTDI();

		*/

		//////////////////////////////////////////////////////
		// help menu
		//////////////////////////////////////////////////////
	}
}

void CreateFontForEditBox()
{
	// Create a LOGFONT structure and set its properties
	LOGFONT lf = {0};

	lf.lfHeight = 18; // height of font
	lf.lfPitchAndFamily = FIXED_PITCH | FF_MODERN;
	strcpy_s(lf.lfFaceName, "Courier New");

	// Create the font and select it into the edit control's device context
	HFONT hfont = CreateFontIndirect(&lf);
	SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hfont, TRUE);
}

// sets up a long status bar, with 4 short sections on the far right
void ResizeStatusBarSections()
{
	// Get the width of the status bar.
	RECT rcStatusBar;
	GetClientRect(hwndMain, &rcStatusBar);
	int statusBarWidth = rcStatusBar.right - rcStatusBar.left;

	int smallPartWidth = 100;
	int longPartWidth = statusBarWidth - 4 * smallPartWidth;

	// Set the locations of the far right edge of all the sections
	int partsWidths[] = {
		longPartWidth,
		longPartWidth + smallPartWidth * 1,
		longPartWidth + smallPartWidth * 2,
		longPartWidth + smallPartWidth * 3,
		longPartWidth + smallPartWidth * 4};

	SendMessage(hwndStatusBar, SB_SETPARTS, sizeof(partsWidths) / sizeof(int), (LPARAM)partsWidths);
}

static bool isTimerSet = false;

static void FindTripUnitWhenConnected()
{
	std::thread thread(FindEquipment, false, true, false);
	thread.detach();
}

void DisconnectFromTripUnit()
{
	CloseCommPort(&hTripUnit.handle);
	PrintToScreen("Disconnected from Trip Unit");
	tripUnitType = TripUnitType::NONE;
	PrintConnectionStatus();
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	std::string *message;
	int statusBarHeight;

	switch (msg)
	{
	case WM_DEVICECHANGE:
		if (wParam == DBT_DEVICEARRIVAL)
		{
			PrintToScreen("WM_DEVICECHANGE");

			if (!isTimerSet)
			{
				// setup timer for 5 seconds from now to find trip unit
				SetTimer(hwnd, ID_TIMER_2, 5000, NULL);
				isTimerSet = true;
			}
		}
		break;
	case WM_CREATE:

		// create an edit control
		hwndEdit = CreateWindowExA(
			0, "EDIT", // predefined class
			NULL,	   // no window title
			WS_BORDER | WS_CHILD | WS_VISIBLE | WS_VSCROLL |
				ES_LEFT | ES_MULTILINE | ES_AUTOHSCROLL,
			0, 0, 0, 0,			 // set size in WM_SIZE message
			hwnd,				 // parent window
			(HMENU)ID_EDITCHILD, // edit control ID
			(HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
			NULL); // pointer not needed

		CreateFontForEditBox();

		hwndStatusBar =
			CreateStatusBar(hwnd, ID_TOOL_BAR, (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), 1);

		StatusBarReady();

		return 0;

		// case WM_CTLCOLOREDIT:
		/*
		if ((HWND)lParam == hwndEdit)
		{
			hdc = (HDC)wParam;
			SetTextColor(hdc, RGB(0, 255, 0)); // light green
			SetBkColor(hdc, RGB(0, 0, 0));	   // black
			return (LRESULT)GetStockObject(BLACK_BRUSH);
		}
		break;
		*/

	case WM_KEYDOWN:
		break;

	case WM_TIMER:
		// update screen to show connection status
		switch (wParam)
		{
		case ID_TIMER_1:
			SetConnectionStatus(ConnectionEnum::DEVICE_TRIP_UNIT, hTripUnit.handle != INVALID_HANDLE_VALUE);
			SetConnectionStatus(ConnectionEnum::DEVICE_KEITHLEY, hKeithley.handle != INVALID_HANDLE_VALUE);
			SetConnectionStatus(ConnectionEnum::DEVICE_ARDUINO, hArduino.handle != INVALID_HANDLE_VALUE);
			SetConnectionStatus(ConnectionEnum::DEVICE_RIGOL, RIGOL_DG1000Z_Connected);
			break;

		case ID_TIMER_2:
			KillTimer(hwnd, ID_TIMER_2);
			FindTripUnitWhenConnected();
			isTimerSet = false;
			break;
		}
		break;

	case WM_SIZE:
		// Make the edit control the size of the window's client area.

		// Get the dimensions of the status.
		RECT rcToolbar;
		GetWindowRect(hwndStatusBar, &rcToolbar);
		statusBarHeight = rcToolbar.bottom - rcToolbar.top;

		// Move the toolbar to the bottom of the window.
		MoveWindow(hwndStatusBar,
				   0,								 // starting x-coordinate
				   HIWORD(lParam) - statusBarHeight, // starting y-coordinate
				   LOWORD(lParam),					 // width of client area
				   statusBarHeight,					 // height of toolbar
				   TRUE);							 // repaint window

		// move the edit window to the top of the window
		MoveWindow(hwndEdit,
				   5, 5,								 // starting x- and y-coordinates
				   LOWORD(lParam) - 5,					 // width of client area
				   HIWORD(lParam) - 5 - statusBarHeight, // height of client area
				   TRUE);								 // repaint window

		ResizeStatusBarSections();

		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		EndPaint(hwnd, &ps);
		break;
	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;
	case WM_CALIBRATION_UPDATE_MSG:
		message = reinterpret_cast<std::string *>(lParam);
		_PrintToScreen(*message);
		free((char *)lParam);
	case WM_COMMAND:
		processCommands(hwnd, wParam);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void SetStatusBarText(const char *msg)
{
	SendMessage(hwndStatusBar, SB_SETTEXT, 0, (LPARAM)msg);
}

void StatusBarReady()
{
	SetStatusBarText("Ready");
}

void SetConnectionStatus(ConnectionEnum connection, bool isConnected)
{
	std::string message_to_send;
	std::string connection_string = isConnected ? "OK" : "N/A";

	switch (connection)
	{
	case ConnectionEnum::DEVICE_TRIP_UNIT:
		message_to_send = "ACPRO2: " + connection_string;
		SendMessage(hwndStatusBar, SB_SETTEXT, 1, (LPARAM)message_to_send.c_str());
		// tell status bar to repaint
		InvalidateRect(hwndStatusBar, NULL, TRUE);
		SendMessage(hwndStatusBar, WM_PAINT, 0, 0);
		break;
	case ConnectionEnum::DEVICE_KEITHLEY:
		message_to_send = "Keithley: " + connection_string;
		SendMessage(hwndStatusBar, SB_SETTEXT, 2, (LPARAM)message_to_send.c_str());
		// tell status bar to repaint
		InvalidateRect(hwndStatusBar, NULL, TRUE);
		SendMessage(hwndStatusBar, WM_PAINT, 0, 0);
		break;
	case ConnectionEnum::DEVICE_RIGOL:
		message_to_send = "Rigol: " + connection_string;
		SendMessage(hwndStatusBar, SB_SETTEXT, 3, (LPARAM)message_to_send.c_str());
		// tell status bar to repaint
		InvalidateRect(hwndStatusBar, NULL, TRUE);
		SendMessage(hwndStatusBar, WM_PAINT, 0, 0);
		break;
	case ConnectionEnum::DEVICE_ARDUINO:
		message_to_send = "Arduino: " + connection_string;
		SendMessage(hwndStatusBar, SB_SETTEXT, 4, (LPARAM)message_to_send.c_str());
		// tell status bar to repaint
		InvalidateRect(hwndStatusBar, NULL, TRUE);
		SendMessage(hwndStatusBar, WM_PAINT, 0, 0);
		break;
	default:
		PrintToScreen("Error: invalid connection type passed to SetConnectionStatus()");
		break;
	}
}

void CheckForDir(std::string dir)
{
	if (!CreateDirectory(dir.c_str(), NULL))
	{
		DWORD error = GetLastError();
		if (error != ERROR_ALREADY_EXISTS)
			ExitWithError("cannot create: " + dir);
	}
}

// program will exit with error if the above dirs do not exists and cannot be
// created
void CheckForRequiredPaths()
{
	CheckForDir("C:\\urc\\");
	CheckForDir("C:\\urc\\apps\\");
	CheckForDir("C:\\urc\\apps\\autocal_rc\\");
}

bool OpenLogFile()
{
	// TODO: base log file name on current date/time
	std::string logFileName = "C:\\urc\\apps\\autocal_rc\\autocal_rc.log";
	log_file.open(logFileName, std::ios::out | std::ios::app);
	if (!log_file.is_open())
	{
		PrintToScreen("Error opening log file: " + logFileName);
		return false;
	}
	return true;
}

void Initialize()
{
	PrintToScreen("AutoCal_RC " + FloatToString(AUTOCAL_RC_VERSION, 1));
	std::string buildDate = "Binary build date: " + std::string(__DATE__) + " @ " + std::string(__TIME__);
	PrintToScreen(buildDate);

	CheckForRequiredPaths();
	ReadInConfigurationFile();

	if (DB::Connect(database_file))
		PrintToScreen("Successfully connected to database: " + database_file);
	else
		PrintToScreen("*** ERROR*** cannot open database: " + database_file);

	if (!OpenLogFile())
	{
		PrintToScreen("Error opening log file");
		return;
	}
}

HWND CreateStatusBar(HWND hwndParent, int idStatus, HINSTANCE hinst, int cParts)
{
	HWND hwndStatus;

	// Ensure that the common control DLL is loaded.
	InitCommonControls();

	// Create the status bar.
	hwndStatus = CreateWindowEx(
		0,						   // no extended styles
		STATUSCLASSNAME,		   // name of status bar class
		(PCTSTR)NULL,			   // no text when first created
		SBARS_SIZEGRIP |		   // includes a sizing grip
			WS_CHILD | WS_VISIBLE, // creates a visible child window
		0, 0, 0, 0,				   // ignores size and position
		hwndParent,				   // handle to parent window
		(HMENU)idStatus,		   // child window identifier
		hinst,					   // handle to application instance
		NULL);					   // no window creation data

	return hwndStatus;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	MSG Msg;

	// Register Window Class
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_HREDRAW | CS_VREDRAW; // repaint while resizing
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
		ExitWithError("Window Registration Failed!");

	int screenWidth = GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = GetSystemMetrics(SM_CYSCREEN);

	int windowWidth = 800;
	int windowHeight = 500;

	int windowPosX = (screenWidth - windowWidth) / 2;
	int windowPosY = (screenHeight - windowHeight) / 2;

	std::string windowTitle = "URC_AutoCAL_RC " + FloatToString(AUTOCAL_RC_VERSION, 1) + " - build: " + std::string(__DATE__) + " @ " + std::string(__TIME__);

	hwndMain = CreateWindowExA(
		WS_EX_CLIENTEDGE,
		g_szClassName,
		windowTitle.c_str(),
		WS_OVERLAPPEDWINDOW,
		windowPosX, windowPosY, windowWidth, windowHeight,
		NULL, NULL, hInstance, NULL);

	if (!hwndMain)
		ExitWithError("Window Registration Failed!");

	std::thread init_thread(Initialize);
	init_thread.detach();

	ShowWindow(hwndMain, nCmdShow);
	UpdateWindow(hwndMain);

	// tell OS to send us a timer message every 5 seconds
	// (currently this is used to update the status bar)
	SetTimer(hwndMain,
			 ID_TIMER_1,
			 5000,
			 NULL);

	SetConnectionStatus(ConnectionEnum::DEVICE_TRIP_UNIT, false);
	SetConnectionStatus(ConnectionEnum::DEVICE_KEITHLEY, false);
	SetConnectionStatus(ConnectionEnum::DEVICE_ARDUINO, false);
	SetConnectionStatus(ConnectionEnum::DEVICE_RIGOL, false);

	while (GetMessage(&Msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

	Shutdown();
	return static_cast<int>(Msg.wParam);
}
