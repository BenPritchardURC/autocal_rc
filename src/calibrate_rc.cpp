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
#include <fstream>

extern TripUnitType tripUnitType;

// the code in here is specific to the ACPRO2-RC trip unit
namespace ACPRO2_RG
{
	// static std::ofstream cal_file;

	void WriteArbitraryCalibrationParamsToINI(const std::string INIFileName, const ArbitraryCalibrationParams &params)
	{
		std::string section = "ArbitraryCalibrationParams";

		WritePrivateProfileStringA(section.c_str(), "Do_LowGain_Not_HighGain", std::to_string(params.Do_LowGain_Not_HighGain).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "High_Gain_Point", std::to_string(params.High_Gain_Point).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "Do_Channel_A", std::to_string(params.Do_Channel_A).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "Do_Channel_B", std::to_string(params.Do_Channel_B).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "Do_Channel_C", std::to_string(params.Do_Channel_C).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "Do_Channel_N", std::to_string(params.Do_Channel_N).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "send_DG1000Z_Commands", std::to_string(params.send_DG1000Z_Commands).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "Use50HZ", std::to_string(params.Use50HZ).c_str(), INIFileName.c_str());
		WritePrivateProfileStringA(section.c_str(), "voltageToCommandRMS", std::to_string(params.voltageToCommandRMS).c_str(), INIFileName.c_str());
	}

	void ReadArbitraryParamsFromINI(const std::string INIFileName, ArbitraryCalibrationParams &params)
	{
		char buffer[32];
		std::string section = "ArbitraryCalibrationParams";

		GetPrivateProfileStringA(section.c_str(), "Do_LowGain_Not_HighGain", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Do_LowGain_Not_HighGain = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "High_Gain_Point", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.High_Gain_Point = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "Do_Channel_A", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Do_Channel_A = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "Do_Channel_B", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Do_Channel_B = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "Do_Channel_C", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Do_Channel_C = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "Do_Channel_N", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Do_Channel_N = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "send_DG1000Z_Commands", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.send_DG1000Z_Commands = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "Use50HZ", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.Use50HZ = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "voltageToCommandRMS", "0.5", buffer, sizeof(buffer), INIFileName.c_str());
		params.voltageToCommandRMS = std::stoi(buffer);
	}

	void WriteFullCalibrationParamsToINI(const std::string INIFileName, const FullCalibrationParams &params)
	{
		std::string section = "FullCalibrationParams";

		for (int i = 0; i < 4; i++)
		{
			WritePrivateProfileStringA(
				section.c_str(),
				("lo_gain_voltages_rms_" + std::to_string(i)).c_str(),
				std::to_string(params.lo_gain_voltages_rms[i]).c_str(),
				INIFileName.c_str());
		}

		for (int i = 0; i < 4; i++)
		{
			WritePrivateProfileStringA(
				section.c_str(),
				("hi_gain_voltages_rms_" + std::to_string(i)).c_str(),
				std::to_string(params.hi_gain_voltages_rms[i]).c_str(),
				INIFileName.c_str());
		}

		WritePrivateProfileStringA(section.c_str(), "do50hz",
								   std::to_string(params.do50hz).c_str(),
								   INIFileName.c_str());

		WritePrivateProfileStringA(section.c_str(), "do60hz",
								   std::to_string(params.do60hz).c_str(),
								   INIFileName.c_str());

		WritePrivateProfileStringA(section.c_str(), "use_bk_precision_9801",
								   std::to_string(params.use_bk_precision_9801).c_str(),
								   INIFileName.c_str());

		WritePrivateProfileStringA(section.c_str(), "use_rigol_dg1000z",
								   std::to_string(params.use_rigol_dg1000z).c_str(),
								   INIFileName.c_str());

		WritePrivateProfileStringA(section.c_str(), "doHighGain",
								   std::to_string(params.doHighGain).c_str(),
								   INIFileName.c_str());

		WritePrivateProfileStringA(section.c_str(), "doLowGain",
								   std::to_string(params.doLowGain).c_str(),
								   INIFileName.c_str());
	}

	void ReadFullCalibrationParamsFromINI(const std::string INIFileName, FullCalibrationParams &params)
	{

		char buffer[32];
		std::string section = "FullCalibrationParams";

		// defaults for lo gains are all 7 volts RMS
		for (int i = 0; i < 4; i++)
		{
			GetPrivateProfileStringA(section.c_str(), ("lo_gain_voltages_rms_" + std::to_string(i)).c_str(), "7", buffer, sizeof(buffer), INIFileName.c_str());
			params.lo_gain_voltages_rms[i] = std::stof(buffer);
		}

		GetPrivateProfileStringA(section.c_str(), "hi_gain_voltages_rms_0", "1.4736000",
								 buffer, sizeof(buffer), INIFileName.c_str());
		params.hi_gain_voltages_rms[0] = std::stof(buffer);

		GetPrivateProfileStringA(section.c_str(), "hi_gain_voltages_rms_1", "0.7186800",
								 buffer, sizeof(buffer), INIFileName.c_str());
		params.hi_gain_voltages_rms[1] = std::stof(buffer);

		GetPrivateProfileStringA(section.c_str(), "hi_gain_voltages_rms_2", "0.4788375",
								 buffer, sizeof(buffer), INIFileName.c_str());
		params.hi_gain_voltages_rms[2] = std::stof(buffer);

		GetPrivateProfileStringA(section.c_str(), "hi_gain_voltages_rms_3", "0.3593400",
								 buffer, sizeof(buffer), INIFileName.c_str());
		params.hi_gain_voltages_rms[3] = std::stof(buffer);

		GetPrivateProfileStringA(section.c_str(), "do50hz", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.do50hz = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "do60hz", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.do60hz = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "use_bk_precision_9801", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.use_bk_precision_9801 = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "use_rigol_dg1000z", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.use_rigol_dg1000z = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "doHighGain", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.doHighGain = std::stoi(buffer);

		GetPrivateProfileStringA(section.c_str(), "doLowGain", "0", buffer, sizeof(buffer), INIFileName.c_str());
		params.doLowGain = std::stoi(buffer);
	}

	static bool CalibrateChannel(
		HANDLE hTripUnit, CalibrationRequest &calRequest, CalibrationResults &calResults)
	{
		// MSG_EXE_CALIBRATE_AD on the R.C. trip unit can take a long time to execute
		constexpr int TIME_OUT_MS = 1000 * 60 * 3; // 3 minutes

		MsgExeCalibrateAD cmd = {0};
		URCMessageUnion rsp = {0};
		bool retval;

		cmd.Hdr.Type = MSG_EXE_CALIBRATE_AD;
		cmd.Hdr.Version = PROTOCOL_VERSION;
		cmd.Hdr.Length = sizeof(MsgExeCalibrateAD) - sizeof(MsgHdr);
		cmd.Hdr.Seq = SequenceNumber();
		cmd.Hdr.Dst = ADDR_TRIP_UNIT;
		cmd.Hdr.Src = ADDR_CAL_APP;
		cmd.CalibrateRequest = calRequest;
		cmd.Hdr.ChkSum = CalcChecksum((uint8_t *)&cmd, sizeof(cmd));

		retval = WriteToCommPort(hTripUnit, (uint8_t *)&cmd, sizeof(cmd));

		if (retval)
		{
			// it is possible this command takes a long time to execute, so we need to wait for the response
			retval = GetURCResponse_Long(hTripUnit, &rsp, TIME_OUT_MS) &&
					 VerifyMessageIsOK(&rsp, MSG_RSP_CALIBRATE_AD, sizeof(MsgRspCalibrateAD) - sizeof(MsgHdr));
		}

		if (retval)
			calResults = rsp.msgRspCalibrateAD.CalibrateResults;
		else
			calResults = {0};

		return retval;
	}

	// time how long MSG_EXE_CALIBRATE_AD takes, and return duration in milliseconds in duration
	bool Time_CalibrateChannel(
		HANDLE hTripUnit, CalibrationRequest &calRequest, CalibrationResults &calResults, long long &duration)
	{

		auto start = std::chrono::high_resolution_clock::now();

		bool retval = CalibrateChannel(hTripUnit, calRequest, calResults);

		auto end = std::chrono::high_resolution_clock::now();
		duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		return retval;
	}

	// default values, per jeff
	void setDefaultRCFullCalParams(FullCalibrationParams &params)
	{

		params.hi_gain_voltages_rms[0] = 1.4736000;
		params.hi_gain_voltages_rms[1] = 0.7186800;
		params.hi_gain_voltages_rms[2] = 0.4788375;
		params.hi_gain_voltages_rms[3] = 0.3593400;

		// all low gains are the same
		params.lo_gain_voltages_rms[0] = 7;
		params.lo_gain_voltages_rms[1] = 7;
		params.lo_gain_voltages_rms[2] = 7;
		params.lo_gain_voltages_rms[3] = 7;

		// default to doing both channels
		params.do50hz = true;
		params.do60hz = true;

		// default to using the rigol AWG
		params.use_rigol_dg1000z = true;
		params.use_bk_precision_9801 = false;
	}

	bool TripUnitisACPro2_RC(HANDLE hTripUnit)
	{

		return (tripUnitType == TripUnitType::AC_PRO2_RC);
	}

	// the way this works is to setup a function pointer
	// then when we call SetSystemAndDeviceSettings, it reads the settings, then calls our function
	// so that we can modify them however we want..
	// and then it writes them back to the trip unit
	static bool SetupTripUnitForCalibration(HANDLE hTripUnit, bool Use50Hz)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		SetSettingsFuncPtr funcptr;

		// use lambda function syntax
		if (Use50Hz)
			funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
			{
				// this is a temporary thing... to make sure the trip unit won't trip
				DevSettings4->ModbusForcedTripEnabled = false;
				Settings->Frequency = 50;
				return true;
			};

		else
			funcptr = [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)
			{
				// this is a temporary thing... to make sure the trip unit won't trip
				DevSettings4->ModbusForcedTripEnabled = false;
				Settings->Frequency = 60;
				return true;
			};

		return ACPRO2_RG::SetSystemAndDeviceSettings(hTripUnit, funcptr);
	}

	static bool SetSystemAndDeviceSettings(HANDLE hTripUnit, SetSettingsFuncPtr funcPtr)
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
				PrintToScreen("error receiving MSG_RSP_DEV_SETTINGS_4");
			}
		}

		if (retval)
		{
			// call function to make any modifications to the settings that we want
			funcPtr(&rsp1.msgRspSysSet4.Settings, &rsp2.msgRspDevSet4.Settings);

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
			}
		}

		return retval;
	}

	std::string gain_constant_to_string(int GAIN_CONSTANT)
	{
		switch (GAIN_CONSTANT)
		{
		case _CALIBRATION_REQUEST_HI_GAIN_0_5:
			return "GAIN_0_5";
		case _CALIBRATION_REQUEST_HI_GAIN_1_0:
			return "GAIN_1_0";
		case _CALIBRATION_REQUEST_HI_GAIN_1_5:
			return "GAIN_1_5";
		case _CALIBRATION_REQUEST_HI_GAIN_2_0:
			return "GAIN_2_0";
		default:
			return "unknown";
		}
	}

	static void OutputDataPoint(std::string s)
	{
		// cal_file.write(s.c_str(), s.size());
		// cal_file.write("\r\n", 2);
	}

	static bool CalibrateOneGain(
		HANDLE hTripUnit,
		HANDLE hKeithley,
		int GAIN_CONSTANT, const FullCalibrationParams &params, bool Is60hz)
	{

		CalibrationResults calResults = {0};
		std::string voltsRMS_as_string;
		CalibrationRequest calRequest = {0};
		double KeithleyReadingVoltsRMS;
		int16_t RMSCurrentToCalibrateTo;
		bool EverythingOK = true;

		bool retval;
		int rms_index = 0;
		long long durationMS; // used to time MSG_EXE_CALIBRATE_AD

		switch (GAIN_CONSTANT)
		{
		case _CALIBRATION_REQUEST_HI_GAIN_0_5:
			rms_index = 0;
			break;
		case _CALIBRATION_REQUEST_HI_GAIN_1_0:
			rms_index = 1;
			break;
		case _CALIBRATION_REQUEST_HI_GAIN_1_5:
			rms_index = 2;
			break;
		case _CALIBRATION_REQUEST_HI_GAIN_2_0:
			rms_index = 3;
			break;
		}

		//////////////////////////////////////
		// start with high gain
		//////////////////////////////////////
		if (params.doHighGain)
		{
			voltsRMS_as_string = std::to_string(params.hi_gain_voltages_rms[rms_index]);
			PrintToScreen("gain constant " + gain_constant_to_string(GAIN_CONSTANT) + " high gain @ " + voltsRMS_as_string + " volts RMS");

			if (params.use_rigol_dg1000z)
			{
				RIGOL_DG1000Z::SetupToApplySINWave(!Is60hz, voltsRMS_as_string);
				RIGOL_DG1000Z::EnableOutput();
			}
			else
			{
				EverythingOK = true;
				EverythingOK &= BK_PRECISION_9801::SetupToApplySINWave(!Is60hz, voltsRMS_as_string);
				EverythingOK &= BK_PRECISION_9801::EnableOutput();

				if (!EverythingOK)
				{
					PrintToScreen("error setting up BK_PRECISION_9801 for calibration");
					return false;
				}
			}

			if (!KEITHLEY::VoltageOnKeithleyIsStable(hKeithley))
			{
				PrintToScreen("Keithley voltage not stable enough to proceed; aborted");
				return false;
			}

			KeithleyReadingVoltsRMS = KEITHLEY::GetVoltageForAutoRange(hKeithley);
			if (KeithleyReadingVoltsRMS == std::numeric_limits<double>::quiet_NaN())
			{
				PrintToScreen("Keithley voltage reading failed; aborted");
				return false;
			}

			if (KeithleyReadingVoltsRMS > params.hi_gain_voltages_rms[rms_index] * 1.25)
			{
				PrintToScreen("Keithley voltage reading too high; aborted");
				return false;
			}

			if (KeithleyReadingVoltsRMS < params.hi_gain_voltages_rms[rms_index] * 0.75)
			{
				PrintToScreen("Keithley voltage reading too low; aborted");
				return false;
			}

			// fixme; eventually this needs to use selected value; not just 3800
			RMSCurrentToCalibrateTo = 3800 * KeithleyReadingVoltsRMS;

			PrintToScreen("keithley voltage: " + std::to_string(KeithleyReadingVoltsRMS));
			PrintToScreen("keithley voltage * 3800 = " + std::to_string(RMSCurrentToCalibrateTo));

			// OutputDataPoint(voltsRMS_as_string);
			// OutputDataPoint(std::to_string(KeithleyReadingVoltsRMS));
			// OutputDataPoint(std::to_string(RMSCurrentToCalibrateTo));

			// calibrate all channels
			calRequest = {0};
			calRequest.Commands =
				GAIN_CONSTANT |
				_CALIBRATION_REQUEST_IA_HI |
				_CALIBRATION_REQUEST_IB_HI |
				_CALIBRATION_REQUEST_IC_HI |
				_CALIBRATION_REQUEST_IN_HI;

			PrintToScreen("calRequest.Commands = " + std::to_string(calRequest.Commands));

			calRequest.RealRMS[i_calibrationIaHI] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationIbHI] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationIcHI] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationInHI] = RMSCurrentToCalibrateTo;

			// we check to make sure the Keithley voltage is stable before starting the calibration
			// additionally: we also monitor it during the calibration
			ASYNC_KEITHLEY::StartMonitoringKeithley(hKeithley);

			retval = ACPRO2_RG::Time_CalibrateChannel(hTripUnit, calRequest, calResults, durationMS);
			PrintToScreen("MSG_EXE_CALIBRATE_AD execution time (milliseconds): " + std::to_string(durationMS));

			ASYNC_KEITHLEY::StopMonitoringKeithley();

			if (retval)
			{
				retval &= ASYNC_KEITHLEY::KeithleyIsStable();

				if (!retval)
					PrintToScreen("Unstable Keithley voltage seen during MSG_EXE_CALIBRATE_AD; calibration aborted");
			}

			// calibration failed, and we are going to abort...
			// make sure we disable the output of the signal generator
			if (!retval)
			{
				PrintToScreen("error calibrating A/B/C/N @ " + gain_constant_to_string(GAIN_CONSTANT));

				if (params.use_rigol_dg1000z)
					RIGOL_DG1000Z::DisableOutput();
				else
					BK_PRECISION_9801::DisableOutput();

				return false;
			}

			DumpCalibrationResults(&calResults);

			/*
			OutputDataPoint(std::to_string(calResults.Offset[0]));
			OutputDataPoint(std::to_string(calResults.SwGain[0]));
			OutputDataPoint(std::to_string(calResults.RmsVal[0]));

			OutputDataPoint(std::to_string(calResults.Offset[2]));
			OutputDataPoint(std::to_string(calResults.SwGain[2]));
			OutputDataPoint(std::to_string(calResults.RmsVal[2]));

			OutputDataPoint(std::to_string(calResults.Offset[4]));
			OutputDataPoint(std::to_string(calResults.SwGain[4]));
			OutputDataPoint(std::to_string(calResults.RmsVal[4]));

			OutputDataPoint(std::to_string(calResults.Offset[6]));
			OutputDataPoint(std::to_string(calResults.SwGain[6]));
			OutputDataPoint(std::to_string(calResults.RmsVal[6]));

			OutputDataPoint(std::to_string(durationMS));
			*/

			// we should have high gains calibrated successfully for a,b,c,n
			// (1 + 4 + 16 + 64 = 85)
			retval = calResults.CalibratedChannels == 85;

			if (!retval)
				PrintToScreen("calibration failed for some channels " + calResults.CalibratedChannels);
		}

		//////////////////////////////////////
		// now do low gain
		//////////////////////////////////////
		if (retval && params.doLowGain)
		{
			voltsRMS_as_string = std::to_string(params.lo_gain_voltages_rms[rms_index]);
			PrintToScreen("gain constant " + gain_constant_to_string(GAIN_CONSTANT) + " low gain @ " + voltsRMS_as_string + " volts RMS");

			if (params.use_rigol_dg1000z)
			{
				RIGOL_DG1000Z::SetupToApplySINWave(!Is60hz, voltsRMS_as_string);
				RIGOL_DG1000Z::EnableOutput();
			}
			else
			{
				EverythingOK = true;
				EverythingOK &= BK_PRECISION_9801::SetupToApplySINWave(!Is60hz, voltsRMS_as_string);
				EverythingOK &= BK_PRECISION_9801::EnableOutput();

				if (!EverythingOK)
				{
					PrintToScreen("error setting up BK_PRECISION_9801 for calibration");
					return false;
				}
			}

			if (!KEITHLEY::VoltageOnKeithleyIsStable(hKeithley))
			{
				PrintToScreen("Keithley voltage not stable enough to proceed; aborted");
				return false;
			}

			KeithleyReadingVoltsRMS = KEITHLEY::GetVoltageForAutoRange(hKeithley);
			if (KeithleyReadingVoltsRMS == std::numeric_limits<double>::quiet_NaN())
			{
				PrintToScreen("Keithley voltage reading failed; aborted");
				return false;
			}

			if (KeithleyReadingVoltsRMS > params.lo_gain_voltages_rms[rms_index] * 1.25)
			{
				PrintToScreen("Keithley voltage reading too high; aborted");
				return false;
			}

			if (KeithleyReadingVoltsRMS < params.lo_gain_voltages_rms[rms_index] * 0.75)
			{
				PrintToScreen("Keithley voltage reading too low; aborted");
				return false;
			}

			RMSCurrentToCalibrateTo = 3800 * KeithleyReadingVoltsRMS;

			PrintToScreen("keithley voltage: " + std::to_string(KeithleyReadingVoltsRMS));
			PrintToScreen("keithley voltage * 3800 = " + std::to_string(RMSCurrentToCalibrateTo));

			/*
			OutputDataPoint(voltsRMS_as_string);
			OutputDataPoint(std::to_string(KeithleyReadingVoltsRMS));
			OutputDataPoint(std::to_string(RMSCurrentToCalibrateTo));
			*/

			// calibrate all channels
			calRequest = {0};

			calRequest.Commands =
				GAIN_CONSTANT |
				_CALIBRATION_REQUEST_IA_LO |
				_CALIBRATION_REQUEST_IB_LO |
				_CALIBRATION_REQUEST_IC_LO |
				_CALIBRATION_REQUEST_IN_LO;

			PrintToScreen("calRequest.Commands = " + std::to_string(calRequest.Commands));

			calRequest.RealRMS[i_calibrationIaLO] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationIbLO] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationIcLO] = RMSCurrentToCalibrateTo;
			calRequest.RealRMS[i_calibrationInLO] = RMSCurrentToCalibrateTo;

			// we check to make sure the Keithley voltage is stable before starting the calibration
			// additionally: we also monitor it during the calibration
			ASYNC_KEITHLEY::StartMonitoringKeithley(hKeithley);

			retval = ACPRO2_RG::Time_CalibrateChannel(hTripUnit, calRequest, calResults, durationMS);
			PrintToScreen("MSG_EXE_CALIBRATE_AD execution time (milliseconds): " + std::to_string(durationMS));

			ASYNC_KEITHLEY::StopMonitoringKeithley();

			if (retval)
			{
				retval = retval && ASYNC_KEITHLEY::KeithleyIsStable();

				if (!retval)
					PrintToScreen("Unstable Keithley voltage seen during MSG_EXE_CALIBRATE_AD; calibration aborted");
			}

			// calibration failed, and we are going to abort...
			// make sure we disable the output of the signal generator
			if (!retval)
			{
				PrintToScreen("error calibrating A/B/C/N @ " + gain_constant_to_string(GAIN_CONSTANT));
				if (params.use_rigol_dg1000z)
					RIGOL_DG1000Z::DisableOutput();
				else
					BK_PRECISION_9801::DisableOutput();

				return false;
			}

			DumpCalibrationResults(&calResults);

			/*

			OutputDataPoint(std::to_string(calResults.Offset[1]));
			OutputDataPoint(std::to_string(calResults.SwGain[1]));
			OutputDataPoint(std::to_string(calResults.RmsVal[1]));

			OutputDataPoint(std::to_string(calResults.Offset[3]));
			OutputDataPoint(std::to_string(calResults.SwGain[3]));
			OutputDataPoint(std::to_string(calResults.RmsVal[3]));

			OutputDataPoint(std::to_string(calResults.Offset[5]));
			OutputDataPoint(std::to_string(calResults.SwGain[5]));
			OutputDataPoint(std::to_string(calResults.RmsVal[5]));

			OutputDataPoint(std::to_string(calResults.Offset[7]));
			OutputDataPoint(std::to_string(calResults.SwGain[7]));
			OutputDataPoint(std::to_string(calResults.RmsVal[7]));

			OutputDataPoint(std::to_string(durationMS));
			*/

			retval = calResults.CalibratedChannels == 255;

			if (!retval)
				PrintToScreen("calibration failed for some channels " + calResults.CalibratedChannels);
		}

		// calibration is done; make sure to disable the output of the signal generator
		if (params.use_rigol_dg1000z)
			RIGOL_DG1000Z::DisableOutput();
		else
			BK_PRECISION_9801::DisableOutput();

		return retval;
	}

	static bool CalibrateAllChannels_AtFreq(
		HANDLE hTripUnit, HANDLE hKeithley, bool Do60hz, const FullCalibrationParams &params)
	{
		bool retval = true;

		if (Do60hz)
			retval = SetupTripUnitForCalibration(hTripUnit, false);
		else
			retval = SetupTripUnitForCalibration(hTripUnit, true);

		if (!retval)
		{
			PrintToScreen("cannot setup trip unit parameters (frequency) for calibration");
			return false;
		}

		if (retval)
		{
			PrintToScreen("waiting 2 seconds for trip unit to reboot...");
			Sleep(2000);

			retval = InitCalibration(hTripUnit);
			if (!retval)
			{
				scr_printf("InitCalibration failed");
			}
		}

		constexpr int TOTAL_CHANNEL_CAL_ATTEMPTS = 1; // 1 = no retries
		int retryCount;

		//////////////////////////////////////////////////////////////////////////////////////////////
		// hardware gain at 0.5
		//////////////////////////////////////////////////////////////////////////////////////////////

		retryCount = 0;

		while (retryCount < TOTAL_CHANNEL_CAL_ATTEMPTS)
		{
			retval = CalibrateOneGain(hTripUnit, hKeithley, _CALIBRATION_REQUEST_HI_GAIN_0_5, params, Do60hz);
			if (retval)
				break;
			else
			{
				PrintToScreen("error calibrating A/B/C/N @ gain 0.5; retrying...");
				return false;
			}

			retryCount++;
		}

		if (!retval)
		{
			PrintToScreen("error calibrating A/B/C/N @ gain 0.5; aborting");
			return false;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		// hardware gain at 1.0
		//////////////////////////////////////////////////////////////////////////////////////////////

		retryCount = 0;
		while (retryCount < TOTAL_CHANNEL_CAL_ATTEMPTS)
		{
			retval = CalibrateOneGain(hTripUnit, hKeithley, _CALIBRATION_REQUEST_HI_GAIN_1_0, params, Do60hz);

			if (retval)
				break;
			else
			{
				PrintToScreen("error calibrating A/B/C/N @ gain 1.0; retrying...");
				return false;
			}

			retryCount++;
		}

		if (!retval)
		{
			PrintToScreen("error calibrating A/B/C/N @ gain 1.0; aborting");
			return false;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		// hardware gain at 1.5
		//////////////////////////////////////////////////////////////////////////////////////////////

		retryCount = 0;
		while (retryCount < TOTAL_CHANNEL_CAL_ATTEMPTS)
		{
			retval = CalibrateOneGain(hTripUnit, hKeithley, _CALIBRATION_REQUEST_HI_GAIN_1_5, params, Do60hz);

			if (retval)
				break;
			else
			{
				PrintToScreen("error calibrating A/B/C/N @ gain 1.5; retrying...");
				return false;
			}

			retryCount++;
		}

		if (!retval)
		{
			PrintToScreen("error calibrating A/B/C/N @ gain 1.5; aborting");
			return false;
		}

		//////////////////////////////////////////////////////////////////////////////////////////////
		// hardware gain at 2.0
		//////////////////////////////////////////////////////////////////////////////////////////////

		retryCount = 0;
		while (retryCount < TOTAL_CHANNEL_CAL_ATTEMPTS)
		{
			retval = CalibrateOneGain(hTripUnit, hKeithley, _CALIBRATION_REQUEST_HI_GAIN_2_0, params, Do60hz);

			if (retval)
				break;
			else
			{
				PrintToScreen("error calibrating A/B/C/N @ gain 2.0; retrying...");
				return false;
			}

			retryCount++;
		}

		if (!retval)
		{
			PrintToScreen("error calibrating A/B/C/N @ gain 2.0; aborting");
			return false;
		}

		if (!WriteCalibrationToFlash(hTripUnit))
		{
			PrintToScreen("error writing calibration to flash");
			return false;
		}

		return true;
	}

	static bool CheckCalParams(const FullCalibrationParams &params)
	{
		bool retval = true;

		if (retval)
		{
			if (params.do50hz == false && params.do60hz == false)
			{
				PrintToScreen("error: must select either 50hz or 60hz calibration");
				retval = false;
			}
		}

		if (retval)
		{
			if (params.use_bk_precision_9801 == false && params.use_rigol_dg1000z == false)
			{
				PrintToScreen("error: must select either BK Precision 9801 or Rigol DG1000Z");
				retval = false;
			}
		}

		if (retval)
		{
			if (params.use_bk_precision_9801 && params.use_rigol_dg1000z)
			{
				PrintToScreen("error: cannot select both BK Precision 9801 and Rigol DG1000Z");
				retval = false;
			}
		}

		if (retval)
		{
			if (!params.doHighGain && !params.doLowGain)
			{
				PrintToScreen("error: must select either high gain or low gain calibration");
				retval = false;
			}
		}

		return retval;
	}

	// perform a full calibration procedure on a ACPRO2-RC trip unit
	bool DoFullTripUnitCAL(
		HANDLE hTripUnit, HANDLE hKeithley, const FullCalibrationParams &params)
	{
		bool retval = true;

		auto start = std::chrono::high_resolution_clock::now();

		// cal_file.open("c:\\tmp\\cal.tmp", std::ios::out | std::ios::binary);

		if (retval)
		{
			retval = CheckCalParams(params);
			if (!retval)
			{
				PrintToScreen("invalid calibration parameters; aborting");
				return false;
			}
		}

		// check trip unit version
		if (retval)
		{
			retval = TripUnitisACPro2_RC(hTripUnit);
			if (!retval)
				PrintToScreen("cannot calibrate; only AC-PRO-2-RC trip units are supported");
		}

		if (retval)
		{
			PrintToScreen("uncalibrating trip unit...");
			retval = Uncalibrate(hTripUnit);
			if (!retval)
				PrintToScreen("trip unit calibration failed");
		}

		/* not needed; we call _CALIBRATION_INITIALIZE after we setup the trip
		   unit params for the frequency
		if (retval)
		{
			PrintToScreen("SetCalToDefault...");
			retval = SetCalToDefault(hTripUnit);
			if (!retval)
				PrintToScreen("error decommissioning trip unit");
		}
		*/

		// if we are going to be calibrating at 50hz, we need to enable the 50hz personality
		if (retval)
		{
			if (params.do50hz)
			{
				retval = Enable50hzPersonality(hTripUnit);
				if (!retval)
					PrintToScreen("cannot calibrate; failed to enable 50hz personality");
			}
		}

		if (retval)
		{
			if (params.do50hz)
			{
				PrintToScreen("calibrating trip unit at 50hz");
				retval = CalibrateAllChannels_AtFreq(hTripUnit, hKeithley, false, params);

				if (!retval)
					PrintToScreen("50hz calibration failed failed");
			}
		}

		if (retval)
		{
			if (params.do60hz)
			{
				PrintToScreen("calibrating trip unit at 60hz");
				retval = CalibrateAllChannels_AtFreq(hTripUnit, hKeithley, true, params);

				if (!retval)
					PrintToScreen("60hz calibration failed failed");
			}
		}

		// check to make sure trip unit reports being calibrated
		if (retval)
		{
			PrintToScreen("waiting 2 seconds...");
			Sleep(2000);
			retval = CheckTripUnitCalibration(hTripUnit);

			if (!retval)
				PrintToScreen("trip unit does not report being calibrated!");
		}

		if (retval)
		{
			PrintToScreen("ACPRO2-RC full calibration procedure completed successfully!");
			GetCalibration(hTripUnit);
		}

		auto end = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

		PrintToScreen("Total Calibration Time (milliseconds): " + std::to_string(duration));

		// cal_file.close();

		return retval;
	}

	void DumpCalibrationResults(CalibrationResults *results)
	{
		int i;

		scr_printf("CalibratedChannels: %u", results->CalibratedChannels);
		scr_printf("Offset: ||");
		for (i = 0; i < _NUM_A2D_INPUTS_RAW_CAL; i++)
		{
			scr_printf("%5u ||", results->Offset[i]);
		}

		scr_printf("");

		scr_printf("SwGain: ||");
		for (i = 0; i < _NUM_A2D_INPUTS_RAW_CAL; i++)
		{
			scr_printf("%5u ||", results->SwGain[i]);
		}

		scr_printf("");

		scr_printf("RmsVal: ||");
		for (i = 0; i < _NUM_A2D_INPUTS_RAW_CAL; i++)
		{
			scr_printf("%5u ||", results->RmsVal[i]);
		}

		scr_printf("");
	}

	// does not work with command _CALIBRATION_REQUEST_IA_HI or any command to calibrate that
	// requires RealRMS[xxx] to be filled in
	bool DoCalibrationCommand(HANDLE hTripUnit, int CalCommand)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		MsgExeCalibrateAD cmd = {0};
		URCMessageUnion rsp = {0};
		bool retval;

		cmd.Hdr.Type = MSG_EXE_CALIBRATE_AD;
		cmd.Hdr.Version = PROTOCOL_VERSION;
		cmd.Hdr.Length = sizeof(MsgExeCalibrateAD) - sizeof(MsgHdr);
		cmd.Hdr.Seq = SequenceNumber();
		cmd.Hdr.Dst = ADDR_TRIP_UNIT;
		cmd.Hdr.Src = ADDR_CAL_APP;

		cmd.CalibrateRequest.Commands = CalCommand;
		// cmd.CalibrateRequest.RealRMS[_NUM_A2D_INPUTS_RAW_CAL];

		cmd.Hdr.ChkSum = CalcChecksum((uint8_t *)&cmd, sizeof(cmd));

		retval = WriteToCommPort(hTripUnit, (uint8_t *)&cmd, sizeof(cmd));

		if (retval)
		{
			// it is possible this command takes a long time to execute, so we need to wait for the response
			retval = GetURCResponse_Long(hTripUnit, &rsp, 300) && VerifyMessageIsOK(&rsp, MSG_RSP_CALIBRATE_AD, sizeof(MsgRspCalibrateAD) - sizeof(MsgHdr));
		}

		if (retval)
		{
			DumpCalibrationResults(&rsp.msgRspCalibrateAD.CalibrateResults);
		}

		return retval;
	}

	bool InitCalibration(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;
		retval = DoCalibrationCommand(hTripUnit, _CALIBRATION_INITIALIZE);

		if (retval)
			scr_printf("_CALIBRATION_INITIALIZE ok");
		else
			scr_printf("_CALIBRATION_INITIALIZE failed");

		return retval;
	}

	bool Uncalibrate(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;

		retval = DoCalibrationCommand(hTripUnit, _CALIBRATION_UNCALIBRATE);

		return retval;
	}

	bool SetCalToDefault(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;

		retval = DoCalibrationCommand(hTripUnit, _CALIBRATION_SET_TO_DEFAULT);

		if (retval)
			scr_printf("_CALIBRATION_SET_TO_DEFAULT ok");
		else
			scr_printf("_CALIBRATION_SET_TO_DEFAULT failed");

		return retval;
	}

	bool DeCommissionTripUnit(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;
		URCMessageUnion rsp = {0};

		retval = SendURCCommand(hTripUnit, MSG_DECOMMISSION, ADDR_TRIP_UNIT, ADDR_CAL_APP);

		if (retval)
			retval = GetURCResponse(hTripUnit, &rsp) && MessageIsACK(&rsp);

		return retval;
	}

	bool CalibrateChannel(
		HANDLE hTripUnit, int16_t Command, int16_t RealRMSIndex, uint16_t RMSValue, CalibrationResults *calResults)
	{

		MsgExeCalibrateAD cmd = {0};
		URCMessageUnion rsp = {0};
		bool retval;

		cmd.Hdr.Type = MSG_EXE_CALIBRATE_AD;
		cmd.Hdr.Version = PROTOCOL_VERSION;
		cmd.Hdr.Length = sizeof(MsgExeCalibrateAD) - sizeof(MsgHdr);
		cmd.Hdr.Seq = SequenceNumber();
		cmd.Hdr.Dst = ADDR_TRIP_UNIT;
		cmd.Hdr.Src = ADDR_CAL_APP;

		cmd.CalibrateRequest.Commands = Command;
		cmd.CalibrateRequest.RealRMS[RealRMSIndex] = RMSValue;

		cmd.Hdr.ChkSum = CalcChecksum((uint8_t *)&cmd, sizeof(cmd));

		retval = WriteToCommPort(hTripUnit, (uint8_t *)&cmd, sizeof(cmd));

		if (retval)
		{
			// it is possible this command takes a long time to execute, so we need to wait for the response
			retval = GetURCResponse_Long(hTripUnit, &rsp, 3000) &&
					 VerifyMessageIsOK(&rsp, MSG_RSP_CALIBRATE_AD, sizeof(MsgRspCalibrateAD) - sizeof(MsgHdr));
		}

		if (retval)
		{
			DumpCalibrationResults(&rsp.msgRspCalibrateAD.CalibrateResults);
			memcpy(calResults, &rsp.msgRspCalibrateAD.CalibrateResults, sizeof(CalibrationResults));
		}
		else
		{
			memset(calResults, 0, sizeof(CalibrationResults));
		}

		return retval;
	}

	bool WriteCalibrationToFlash(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;

		retval = DoCalibrationCommand(hTripUnit, _CALIBRATION_WRITE_TO_FLASH);

		if (retval)
			scr_printf("_CALIBRATION_WRITE_TO_FLASH ok");
		else
			scr_printf("_CALIBRATION_WRITE_TO_FLASH failed");

		return retval;
	}

	// first switch to 50hz
	// then send _CALIBRATION_SET_TO_DEFAULT + _CALIBRATION_UNCALIBRATE + _CALIBRATION_INITIALIZE + _CALIBRATION_WRITE_TO_FLASH
	// then switch to 60hz
	// then send the same command
	bool VirginizeTripUnit(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval = true;
		bool Do50Hz = true;

		// loop through 50hz / 60hz operation
		for (int i = 0; i <= 1; i++)
		{

			if (retval)
			{
				retval = SetupTripUnitForCalibration(hTripUnit, Do50Hz);
				if (!retval)
				{
					scr_printf("cannot setup trip unit frequency");
				}
			}

			if (retval)
			{
				scr_printf("waiting 2 seconds for trip unit to reboot...");
				Sleep(2000);
				retval = DoCalibrationCommand(hTripUnit, _CALIBRATION_SET_TO_DEFAULT + _CALIBRATION_UNCALIBRATE + _CALIBRATION_INITIALIZE + _CALIBRATION_WRITE_TO_FLASH);
			}

			// do next phase
			Do50Hz = false;
		}

		return retval;
	}

	bool CheckTripUnitCalibration(HANDLE hTripUnit)
	{

		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		URCMessageUnion rsp = {0};
		bool retval;

		retval = SendURCCommand(hTripUnit, MSG_GET_STATUS, ADDR_TRIP_UNIT, ADDR_CAL_APP);

		if (retval)
		{
			retval = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_RSP_STATUS_2, sizeof(MsgRspStatus2) - sizeof(MsgHdr));
		}

		if (retval)
		{
			retval = ((rsp.msgRspStatus2.Status & STS_CALIBRATED) == STS_CALIBRATED);
		}

		return retval;
	}

	bool TripUnitIsV4(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		bool retval;
		URCMessageUnion rsp = {0};

		// make sure to send this request as "infopro" because v4 trip unit
		// lies about its version number to make QTIM work if send from address 5
		retval = SendURCCommand(hTripUnit, MSG_GET_SW_VER, ADDR_TRIP_UNIT, ADDR_INFOPRO_AC);

		if (retval)
		{
			retval = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_RSP_SW_VER, sizeof(MsgRspSoftVer) - sizeof(MsgHdr));
			if (!retval)
			{
				scr_printf("error receiving MSG_RSP_SW_VER");
			}
		}

		if (retval)
		{
			retval = (rsp.msgRspSoftVer.ver.Major >= 4);
		}

		return retval;
	}

	bool SendSetPersonality(HANDLE hTripUnit, Personality4 *pers)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

		MsgSetPersonality4 msg = {0};

		msg.Hdr.Type = MSG_SET_PERSONALITY_4;
		msg.Hdr.Version = PROTOCOL_VERSION;
		msg.Hdr.Length = sizeof(MsgSetPersonality4) - sizeof(MsgHdr);
		msg.Hdr.Seq = SequenceNumber();
		msg.Hdr.Dst = ADDR_TRIP_UNIT;
		msg.Hdr.Src = ADDR_CAL_APP;
		msg.Pers = *pers;

		msg.Hdr.ChkSum = CalcChecksum((uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);

		return WriteToCommPort(hTripUnit, (uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);
	}

	bool Enable50hzPersonality(HANDLE hTripUnit)
	{
		_ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

#define _BIT_Frequency50Hz 0x00000100ul // 50 Hz is allowed.

		bool retval;
		URCMessageUnion rsp = {0};

		// grab existing personality
		SendURCCommand(hTripUnit, MSG_GET_PERSONALITY_4, ADDR_TRIP_UNIT, ADDR_CAL_APP);
		retval = GetURCResponse(hTripUnit, &rsp) && VerifyMessageIsOK(&rsp, MSG_RSP_PERSONALITY_4, sizeof(MsgRspPersonality4) - sizeof(MsgHdr));

		if (retval)
		{
			// set 50hz personality
			rsp.msgRspPersonality4.Pers.options32 |= _BIT_Frequency50Hz;
			if (SendSetPersonality(hTripUnit, &rsp.msgRspPersonality4.Pers))
			{
				retval = GetURCResponse(hTripUnit, &rsp) && MessageIsACK(&rsp);
			}
		}

		return retval;
	}

}