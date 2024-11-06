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

#include <fstream>
#include <iostream>
#include <cstdint>

#include "autocal_rc.hpp"

bool SendSetQTStatus(HANDLE hTripUnit, bool beOn)
{
	MsgSetSwitchQT4 msg = {0};

	msg.Hdr.Type = MSG_SET_QT_SWITCH_4;
	msg.Hdr.Version = PROTOCOL_VERSION;
	msg.Hdr.Length = 2;
	msg.Hdr.Seq = SequenceNumber();
	msg.Hdr.Dst = ADDR_TRIP_UNIT;
	msg.Hdr.Src = ADDR_CAL_APP;

	if (beOn)
		msg.State = _QT_SWITCH_STATE_ON;
	else
		msg.State = _QT_SWITCH_STATE_OFF;

	msg.Hdr.ChkSum = 0;
	msg.Hdr.ChkSum = CalcChecksum((uint8_t *)&msg, sizeof(msg));

	return WriteToCommPort(hTripUnit, (uint8_t *)&msg, sizeof(msg));
}

bool SendConnectMessage(HANDLE hTripUnit)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	URCMessageUnion msg = {0};
	SendURCCommand(hTripUnit, MSG_CONNECT, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	return GetAckURCResponse(hTripUnit, &msg) && (msg.msgHdr.Type == MSG_ACK);
}

bool ConnectTripUnit(HANDLE *hTripUnit, int port, TripUnitType *tripUnitType)
{
	URCMessageUnion rsp = {0};
	bool retval;

	retval = InitCommPort(hTripUnit, port, 19200);

	if (retval)
	{
		SendURCCommand(*hTripUnit, MSG_CONNECT, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = (GetAckURCResponse(*hTripUnit, &rsp) && (rsp.msgHdr.Type == MSG_ACK));

		// determine the type of trip unit
		if (retval)
		{
			switch (rsp.msgHdr.Src)
			{
			case ADDR_TRIP_UNIT:
				*tripUnitType = TripUnitType::AC_PRO2_V4;
				break;
			case ADDR_TRIP_UNIT_GFO:
				*tripUnitType = TripUnitType::AC_PRO2_GFO;
				break;
			case ADDR_AC_PRO_NW:
				*tripUnitType = TripUnitType::AC_PRO2_NW;
				break;
			case ADDR_AC_PRO_RC:
				*tripUnitType = TripUnitType::AC_PRO2_RC;
				break;
			default:
				PrintToScreen("unknown trip unit type: " + std::to_string(rsp.msgHdr.Src));
				*tripUnitType = TripUnitType::NONE;
				retval = false;
				break;
			}
		}
	}

	// right now we can't differentiate between an ACPRO2 NW and ACPro_RC.
	// so one way to do that is to send a MSG_GET_CALIBRATION, because
	// the ACPro2_RC has a different response to that command
	if (retval)
	{
		if (ACPRO2_RG::TripUnitisACPro2_RC(*hTripUnit))
			*tripUnitType = TripUnitType::AC_PRO2_RC;
	}
	else
	{
		*tripUnitType = TripUnitType::NONE;
		CloseCommPort(hTripUnit);
	}

	return retval;
}

void DumpBufferToFile(const uint8_t *buffer, size_t size, const std::string &filename)
{
	std::ofstream outFile(filename, std::ios::binary);
	if (!outFile)
	{
		std::cerr << "Failed to open file for writing: " << filename << std::endl;
		return;
	}

	outFile.write(reinterpret_cast<const char *>(buffer), size);
	if (!outFile)
	{
		std::cerr << "Failed to write data to file: " << filename << std::endl;
	}

	outFile.close();
}

bool SendSetUserSet4(HANDLE hTripUnit, SystemSettings4 *SysSettings, DeviceSettings4 *DevSettings)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	MsgSetUserSet4 cmd = {0};
	bool retval;

	cmd.Hdr.Type = MSG_SET_USR_SETTINGS_4;
	cmd.Hdr.Version = PROTOCOL_VERSION;
	cmd.Hdr.Length = sizeof(MsgSetUserSet4) - sizeof(MsgHdr);
	cmd.Hdr.Seq = SequenceNumber();
	cmd.Hdr.Dst = ADDR_TRIP_UNIT;
	cmd.Hdr.Src = ADDR_CAL_APP;

	cmd.Filler16 = 0;
	cmd.SysSettings = *SysSettings;
	cmd.DevSettings = *DevSettings;

	cmd.Hdr.ChkSum = CalcChecksum((uint8_t *)&cmd, sizeof(cmd));

	// DumpBufferToFile((uint8_t *)&cmd, sizeof(cmd), "c:\\tmp\\MSG_SET_USR_SETTINGS_4.txt");

	retval = WriteToCommPort(hTripUnit, (uint8_t *)&cmd, sizeof(cmd));
	return retval;
}

bool SetupDefaultUserSettings()
{

	// setup the default settings
	SystemSettings4 sysSettings = {0};
	DeviceSettings4 devSettings = {0};

	// TODO: fill this in

	return false;
}

bool SetupTripUnitFrequency(HANDLE hTripUnit, bool Use50Hz)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	SetSettingsFuncPtr funcptr;

	// use lambda function syntax
	if (Use50Hz)
		funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
		{
			Settings->Frequency = 50;
			return true;
		};

	else
		funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
		{
			Settings->Frequency = 60;
			return true;
		};

	return SetSystemAndDeviceSettings(hTripUnit, funcptr);
}

// the way this works is to setup a function pointer
// then when we call SetSystemAndDeviceSettings, it reads the settings, then calls our function
// so that we can modify them however we want..
// and then it writes them back to the trip unit
bool SetupTripUnitForCalibration(HANDLE hTripUnit, bool Use50Hz)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	SetSettingsFuncPtr funcptr;

	// use lambda function syntax
	if (Use50Hz)
		funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
		{
			Settings->CTNeutralRating = 1000;
			Settings->CTRating = 1000;
			Settings->Frequency = 50;
			DevSettings4->SBThreshold = 0;
			DevSettings4->LTPickup = 9000;
			DevSettings4->InstantPickup = 10000;
			DevSettings4->QTInstPickup = 10000;

			return true;
		};

	else
		funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
		{
			Settings->CTNeutralRating = 1000;
			Settings->CTRating = 1000;
			Settings->Frequency = 60;
			DevSettings4->SBThreshold = 0;
			DevSettings4->LTPickup = 9000;
			DevSettings4->InstantPickup = 10000;
			DevSettings4->QTInstPickup = 10000;

			return true;
		};

	return SetSystemAndDeviceSettings(hTripUnit, funcptr);
}

bool SetSystemAndDeviceSettings(HANDLE hTripUnit, SetSettingsFuncPtr funcPtr)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	URCMessageUnion rsp1 = {0};
	URCMessageUnion rsp2 = {0};
	URCMessageUnion rsp3 = {0};

	bool retval = true;

	// grab system settings
	if (retval)
	{
		SendURCCommand(hTripUnit, MSG_GET_SYS_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, &rsp1) && VerifyMessageIsOK(&rsp1, MSG_RSP_SYS_SETTINGS_4, sizeof(MsgRspSysSet4) - sizeof(MsgHdr));

		if (!retval)
		{
			PrintToScreen("error receiving MSG_RSP_SYS_SETTINGS_4");
		}
	}

	// grab device settings
	if (retval)
	{
		SendURCCommand(hTripUnit, MSG_GET_DEV_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, &rsp2) && VerifyMessageIsOK(&rsp2, MSG_RSP_DEV_SETTINGS_4, sizeof(MsgRspDevSet4) - sizeof(MsgHdr));

		if (!retval)
		{
			PrintToScreen("error sending MSG_GET_DEV_SETTINGS");
		}
	}

	if (retval)
	{
		// call function to make any modifications to the settings that we want
		funcPtr(&rsp1.msgRspSysSet4.Settings, &rsp2.msgRspDevSet4.Settings);

		// disable the instant trip??
		// not sure why right this second
		// rsp2.msgRspDevSet4.Settings.InstantPickup = 10000;

		// and now send the settings back to the trip unit
		SendSetUserSet4(hTripUnit, &rsp1.msgRspSysSet4.Settings, &rsp2.msgRspDevSet4.Settings);

		retval = GetURCResponse(hTripUnit, &rsp3);
		if (retval)
		{
			// trip unit will NAK if nothing changed
			retval =
				(rsp3.msgHdr.Type == MSG_ACK) ||
				(rsp3.msgHdr.Type == MSG_NAK && rsp3.msgNAK.Error == NAK_NO_CHANGES);
		}

		if (!retval)
		{
			PrintToScreen("did not receive ACK from MSG_SET_USR_SETTINGS_4");
			if (MSG_NAK == rsp3.msgHdr.Type)
			{
				PrintToScreen("NAK Code: " + std::to_string(rsp3.msgNAK.Error));
				PrintToScreen("NAK Meaning: " + NAKCodeToString(rsp3.msgNAK.Error));
			}
			scr_printf("did not receive ACK from MSG_SET_USR_SETTINGS_4");
		}
	}

	return retval;
}

bool GetDynamics(HANDLE hTripUnit, URCMessageUnion *rsp)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	bool retval = true;

	if (retval)
	{
		SendURCCommand(hTripUnit, MSG_GET_DYNAMICS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, rsp) && VerifyMessageIsOK(rsp, MSG_RSP_DYNAMICS_4, sizeof(MsgRspDynamics4) - sizeof(MsgHdr));

		if (!retval)
		{
			scr_printf("error sending MSG_GET_DYNAMICS");
		}
	}

	return retval;
}

void DumpCalResults(MsgRspCalibr *msg)
{
	CalibrationDataAtFrequency *cal_ptr;

	scr_printf("calibrated: %d", msg->CalibrationDataInfoC.Calibrated);

	for (int hz_index = 0; hz_index < 2; hz_index++)
	{

		if (0 == hz_index)
		{
			scr_printf("50hz");
			cal_ptr = &msg->CalibrationDataInfoC.Hz50;
		}
		else
		{
			scr_printf("60hz");
			cal_ptr = &msg->CalibrationDataInfoC.Hz60;
		}

		scr_printf("calibrated channeles: %d", cal_ptr->CalibratedChannels);

		for (int i = 0; i < _NUM_A2D_INPUTS_RAW_CAL; i++)
		{
			scr_printf("%d offset... %d", i, cal_ptr->Offset[i]);
			scr_printf("%d gain..... %d", i, cal_ptr->SwGain[i]);
		}
	}
}

void DumpCalResults_RC(MsgRspCalibrRC *msg)
{
	CalibrationDataAtFrequencyRC *cal_ptr;

	scr_printf("calibrated: %d", msg->CalibrationDataInfoD.Calibrated);

	for (int hi_gain_channel = 0; hi_gain_channel < _NUM_HI_GAIN; hi_gain_channel++)
	{

		switch (hi_gain_channel)
		{
		case 0:
			scr_printf("HI_GAIN_0_5");
			break;
		case 1:
			scr_printf("HI_GAIN_1_0");
			break;
		case 2:
			scr_printf("HI_GAIN_1_5");
			break;
		case 3:
			scr_printf("HI_GAIN_2_0");
			break;
		}

		for (int hz_index = 0; hz_index < 2; hz_index++)
		{

			if (0 == hz_index)
			{
				scr_printf("50hz");
				cal_ptr = &msg->CalibrationDataInfoD.GainHI[hi_gain_channel].Hz50;
			}
			else
			{
				scr_printf("60hz");
				cal_ptr = &msg->CalibrationDataInfoD.GainHI[hi_gain_channel].Hz60;
			}

			scr_printf("calibrated channels: %d", cal_ptr->CalibratedChannels);

			for (int i = 0; i < _NUM_TO_CALIBRATE_RC; i++)
			{
				scr_printf("%d offset... %d", i, cal_ptr->Offset[i]);
				scr_printf("%d gain..... %d", i, cal_ptr->SwGain[i]);
			}
		}
	}
}

bool GetCalibration(HANDLE hTripUnit)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	MsgGetCalibr cmd = {0};
	URCMessageUnion rsp = {0};
	bool retval;
	bool isACPro_RC;

	retval = SendURCCommand(hTripUnit, MSG_GET_CALIBRATION, ADDR_TRIP_UNIT, ADDR_CAL_APP);

	if (retval)
	{
		retval = GetURCResponse(hTripUnit, &rsp);
	}

	if (retval)
	{
		switch (rsp.msgHdr.Type)
		{
		case MSG_RSP_CALIBRATION:
			isACPro_RC = false;
			break;
		case MSG_RSP_CALIBRATION_RC:
			isACPro_RC = true;
			break;
		default:
			scr_printf("unexpected response type to MSG_GET_CALIBRATION: %d", rsp.msgHdr.Type);
			retval = false;
			break;
		}
	}

	if (retval)
	{
		if (isACPro_RC)
			retval = VerifyMessageIsOK(&rsp, MSG_RSP_CALIBRATION_RC, sizeof(MsgRspCalibrRC) - sizeof(MsgHdr));
		else
			retval = VerifyMessageIsOK(&rsp, MSG_RSP_CALIBRATION, sizeof(MsgRspCalibr) - sizeof(MsgHdr));
	}

	if (retval)
	{
		if (isACPro_RC)
		{
			scr_printf("Dumping ACPro_RC calibration results");
			DumpCalResults_RC(&rsp.msgRspCalibrRC);
		}
		else
		{
			scr_printf("Dumping ACPro2 calibration results");
			DumpCalResults(&rsp.msgRspCalibr);
		}
	}
	else
	{
		scr_printf("MSG_GET_CALIBRATION failed");
	}

	return retval;
}

bool SendClearTripHistory(HANDLE hTripUnit)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	URCMessageUnion msg = {0};
	SendURCCommand(hTripUnit, MSG_CLR_TRIP_HIST, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	return GetAckURCResponse(hTripUnit, &msg) && (msg.msgHdr.Type == MSG_ACK);
}

bool GetTripHistory(HANDLE hTripUnit, URCMessageUnion *msg)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	SendURCCommand(hTripUnit, MSG_GET_TRIP_HIST, ADDR_TRIP_UNIT, ADDR_CAL_APP);
	return GetAckURCResponse(hTripUnit, msg) &&
		   VerifyMessageIsOK(msg, MSG_RSP_TRIP_HIST_4, sizeof(MsgRspTripHist4) - sizeof(MsgHdr));
}

// return value corresponds to command succeeding
// if retval == true, then tripTypeIsAsExpected is set to true if the trip type was as expected
bool CheckForExactlyOneTrip(HANDLE hTripUnit, int ExpectedTripType, bool &tripTypeIsAsExpected)
{
	URCMessageUnion rsp = {0};
	bool retval;
	tripTypeIsAsExpected = false;

	if (!GetTripHistory(hTripUnit, &rsp))
	{
		PrintToScreen("Error getting trip history");
		return false;
	}

	// sum up all the trips
	int total_trip = 0;
	{
		for (int i = 0; i < _TRIP_HISTORY_MAX_TRIPS; i++)
			total_trip += rsp.msgRspTripHist4.Trips.Counter.trip[i];
	}

	if (total_trip != 1)
	{
		PrintToScreen("expected 1 total trip, but found " + std::to_string(total_trip));
		return false;
	}

	if (rsp.msgRspTripHist4.Trips.Counter.trip[ExpectedTripType] != 1)
	{
		PrintToScreen("Did not see exactly one trip of expected type; instead saw " +
					  std::to_string(rsp.msgRspTripHist4.Trips.Counter.trip[ExpectedTripType]));

		return false;
	}

	tripTypeIsAsExpected = true;
	return true;
}

// make sure that we got 1 or more of the correct trip types
// we can't check for "exactly one" because the trip unit may have tripped multiple times
bool CheckForCorrectTrip(HANDLE hTripUnit, int ExpectedTripType, bool &tripTypeIsAsExpected)
{
	URCMessageUnion rsp = {0};
	bool retval;
	tripTypeIsAsExpected = false;

	if (!GetTripHistory(hTripUnit, &rsp))
	{
		PrintToScreen("Error getting trip history");
		return false;
	}

	// make sure no trips of wrong type
	int wrong_trips = 0;
	{
		for (int i = 0; i < _TRIP_HISTORY_MAX_TRIPS; i++)
		{
			if (i != ExpectedTripType)
				wrong_trips += rsp.msgRspTripHist4.Trips.Counter.trip[i];
		}
	}

	if (wrong_trips > 0)
	{
		PrintToScreen("1 or more unexpected trips seen");
		return false;
	}

	// make sure at least one trip of the expected type
	if (rsp.msgRspTripHist4.Trips.Counter.trip[ExpectedTripType] == 0)
	{
		PrintToScreen("Did not see at least one trip of expected type");
		return false;
	}

	tripTypeIsAsExpected = true;
	return true;
}

bool GetSerialNumbers(
	HANDLE hTripUnit,
	char *ld_serial_num, size_t size_buf_1,
	char *tu_serial_num, size_t size_buf_2)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	URCMessageUnion rsp1 = {0};
	URCMessageUnion rsp2 = {0};

	if (size_buf_1 != 12 || size_buf_2 != 12)
		return false;

	// ask LD for its serial num
	if (!SendURCCommand(hTripUnit, MSG_GET_SER_NUM, ADDR_DISP_LOCAL, ADDR_CAL_APP))
		return false;
	if (!GetAckURCResponse(hTripUnit, &rsp1))
		return false;
	if (!VerifyMessageIsOK(&rsp1, MSG_RSP_SER_NUM, sizeof(MsgRspSerNum) - sizeof(MsgHdr)))
		return false;

	// ask TU for its serial num
	if (!SendURCCommand(hTripUnit, MSG_GET_SER_NUM, ADDR_TRIP_UNIT, ADDR_CAL_APP))
		return false;
	if (!GetAckURCResponse(hTripUnit, &rsp2))
		return false;
	if (!VerifyMessageIsOK(&rsp1, MSG_RSP_SER_NUM, sizeof(MsgRspSerNum) - sizeof(MsgHdr)))
		return false;

	strcpy_s(ld_serial_num, size_buf_1, rsp1.msgRspSerNum.Number);
	strcpy_s(tu_serial_num, size_buf_2, rsp2.msgRspSerNum.Number);

	return true;
}

bool SendSetSerial(HANDLE hTripUnit, int dest, char *serial_num, HardVer hw_version)
{
	_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

	URCMessageUnion rsp = {0};
	MsgSetSerNum4 cmd = {0};

	cmd.Hdr.Type = MSG_SET_SER_NUM_4;
	cmd.Hdr.Version = PROTOCOL_VERSION;
	cmd.Hdr.Length = sizeof(MsgSetSerNum4) - sizeof(MsgHdr);
	cmd.Hdr.Seq = SequenceNumber();
	cmd.Hdr.Dst = dest;
	cmd.Hdr.Src = ADDR_CAL_APP;

	memcpy(cmd.Number, serial_num, 12);
	cmd.HWversion = hw_version;

	cmd.Hdr.ChkSum = CalcChecksum((uint8_t *)&cmd, sizeof(cmd));

	WriteToCommPort(hTripUnit, (uint8_t *)&cmd, sizeof(cmd));

	return GetAckURCResponse(hTripUnit, &rsp) && (rsp.msgHdr.Type == MSG_ACK);
}

bool SetSerialNumbers(
	HANDLE hTripUnit,
	char *ld_serial_num,
	char *tu_serial_num)
{

	// start out by requesting the hardware revision, from both trip unit and LD

	URCMessageUnion rsp1 = {0};
	URCMessageUnion rsp2 = {0};

	// ask TU for its hardware rev (since we need to send it back with MSG_SET_SER_NUM_4)
	if (!SendURCCommand(hTripUnit, MSG_GET_HW_REV, ADDR_DISP_LOCAL, ADDR_CAL_APP))
		return false;
	if (!GetAckURCResponse(hTripUnit, &rsp1))
		return false;
	if (!VerifyMessageIsOK(&rsp1, MSG_RSP_HW_REV, sizeof(MsgRspHwRev) - sizeof(MsgHdr)))
		return false;

	// ask TU for its hardware rev (since we need to send it back with MSG_SET_SER_NUM_4)
	if (!SendURCCommand(hTripUnit, MSG_GET_HW_REV, ADDR_TRIP_UNIT, ADDR_CAL_APP))
		return false;
	if (!GetAckURCResponse(hTripUnit, &rsp2))
		return false;
	if (!VerifyMessageIsOK(&rsp2, MSG_RSP_HW_REV, sizeof(MsgRspHwRev) - sizeof(MsgHdr)))
		return false;

	if (!SendSetSerial(hTripUnit,
					   ADDR_DISP_LOCAL, ld_serial_num, rsp1.msgRspHwRev.HW))
		return false;

	if (!SendSetSerial(hTripUnit,
					   ADDR_TRIP_UNIT, tu_serial_num, rsp2.msgRspHwRev.HW))
		return false;

	return true;
}
