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

#include <sstream>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <string>
#include <functional>

// note: this includes the file for the VISA Library 5.1 specification, not something
//       not something from our project. Normally it should be located at:
//		 C:\Program Files\IVI Foundation\VISA\Win64\Include\visa.h
#include <C:\Program Files\IVI Foundation\VISA\Win64\Include\visa.h>
#include <vector>

// VISA =  (Virtual Instrument Software Architecture) library
// NI-VISA = National Instruments VISA
// USBTMC = USB Test and Measurement Class
// SCPI = Standard Commands for Programmable Instruments
// AWG = Arbitrary Waveform Generator

namespace RIGOL_DG1000Z
{

	// NOTE:
	//		these functions in here are very specific to the way we are using
	// 		the signal generator to calibrate a rogowski coil trip unit.
	// 		I did not implement these functions to be able to deal with the
	//		function generator in a "general" kind of way

	// default time of 5 seconds to wait for a response from the instrument
	ViUInt32 timeout_ms = 5000;

	ViSession session;
	ViSession resource_manager;
	ViStatus status;

	void PrintError(std::string msg)
	{
		PrintToScreen("RIGOL_DG1000Z Error: " + msg);
	}

	bool OpenVISAInterface()
	{
		status = viOpenDefaultRM(&resource_manager);
		if (status < VI_SUCCESS)
		{
			PrintError("cannot open VISA resource manager");
			return false;
		}

		// find all instruments connected to the computer
		ViPFindList findList = new unsigned long;
		ViPUInt32 retcnt = new unsigned long;
		ViChar instrDesc[1000];
		ViChar search_expression[] = "?*";
		status = viFindRsrc(resource_manager, search_expression, findList, retcnt, instrDesc);
		if (status < VI_SUCCESS)
		{
			PrintError("device not found");
			return false;
		}

		// first just enumerate all the VISA devices

		std::vector<std::string> deviceNames;
		while (status != VI_ERROR_RSRC_NFOUND)
		{
			deviceNames.push_back(std::string(instrDesc));
			status = viFindNext(*findList, instrDesc);
		}

		if (deviceNames.empty())
		{
			PrintError("no RIGOL_DG1000Z VISA instruments found");
			return false;
		}

		// now we are going to go through each one in turn, open it if we can, and ask it
		// to identify itself...

		bool instrumentOpenedOK = false;

		for (const std::string &deviceName : deviceNames)
		{
			// only open USB devices
			if (deviceName.find("USB", 0) != std::string::npos)
			{
				status = viOpen(resource_manager, (ViRsrc)deviceName.c_str(), VI_NULL, VI_NULL, &session);
				if (status == VI_SUCCESS)
				{
					// Set timeout on instrument io
					viSetAttribute(session, VI_ATTR_TMO_VALUE, timeout_ms);

					// ask the device to identify itself
					std::string response;
					bool retval = WriteCommandReadResponse("*IDN?", response);

					PrintToScreen("full *IDN? response: " + response);

					if (retval)
					{
						if (response.find("Rigol Technologies,DG10") != std::string::npos)
						{
							instrumentOpenedOK = true;
							break;
						}

						viClose(session); // close this device, and move onto next one
					}
					else
						viClose(session); // close this device, and move onto next one
				}
			}
		}

		if (!instrumentOpenedOK)
		{
			PrintError("no RIGOL_DG1000Z VISA instruments found");
			return false;
		}

		return true;
	}

	// Close the connection to the instrument
	void CloseVISAInterface()
	{
		viClose(session);
		viClose(resource_manager);
	}

	bool WriteCommand(const std::string &command)
	{

		// write command to instrument

		ViBuf scpi_command = (ViBuf)command.c_str();
		ViUInt32 bytes_to_write = (ViUInt32)command.length();
		ViUInt32 write_count;

		// we rely on viWrite() to fail if the instrument is not opened
		status = viWrite(session, scpi_command, bytes_to_write, &write_count);
		if (status < VI_SUCCESS || write_count != bytes_to_write)
		{
			PrintError("cannot write to VISA instrument");
			return false;
		}
		return true;
	}

	bool WriteCommandReadResponse(const std::string &command, std::string &response)
	{

		if (!WriteCommand(command))
			return false;

		// read response back

		const ViUInt32 read_buffer_size = 1000;
		ViChar read_buffer[read_buffer_size];
		ViUInt32 bytes_read;

		status = viRead(session, (ViBuf)read_buffer, read_buffer_size, &bytes_read);
		if (status < VI_SUCCESS || bytes_read == 0)
		{
			PrintError("cannot read from VISA instrument");
			return false;
		}

		// null terminate the string we read back
		if (bytes_read < read_buffer_size)
			read_buffer[bytes_read] = '\0';
		else
			read_buffer[read_buffer_size - 1] = '\0';

		// store the response in the output parameter
		response = read_buffer;
		return true;
	}

	// reset signal generator to default settings
	bool Initialize()
	{
		bool retval = true;
		std::string response;

		retval = WriteCommand("*RST");

		if (retval)
		{
			retval = WriteCommandReadResponse(":SOUR1:APPL?", response);
		}

		// because we are doing calibrations, be very paranoid about
		// making sure we know exactly how the signal generator is setup
		if (retval)
		{
			// check for default, out of reset, known values
			bool StartupStateOK =
				(response.find("SIN,1.000000E+03,5.000000E+00,0.000000E+00,0.000000E+00") != std::string::npos);

			if (!StartupStateOK)
			{
				PrintError("RIGOL AWG not in default state " + response);
				retval = false;
			}
		}

		return retval;
	}

	// we are writing the command to the signal generator as a string,
	// so i think it makes sense to pass the voltage as one here
	bool SetupToApplySINWave_ByChannel(bool use50Hz, const std::string &voltsRMSAsString, bool useChannel2NotONe)
	{
		bool retval = true;
		std::string cmd_to_apply;
		std::string expected_string_back;
		std::string actual_string_back;

		// we are trying to make a string like this to send:
		// :SOUR1:APPL:SIN 50,2.5vrms,0,0

		cmd_to_apply = ":SOUR";

		if (useChannel2NotONe)
			cmd_to_apply += "2";
		else
			cmd_to_apply += "1";

		cmd_to_apply += ":APPL:SIN ";

		if (use50Hz)
			cmd_to_apply += "50,";
		else
			cmd_to_apply += "60,";

		cmd_to_apply += voltsRMSAsString + "vrms,0,0";
		retval = WriteCommand(cmd_to_apply);

		return retval;
	}

	// channel 1
	bool SetupToApplySINWave(bool use50Hz, const std::string &voltsRMSAsString)
	{
		return SetupToApplySINWave_ByChannel(use50Hz, voltsRMSAsString, false);
	}

	// channel 2
	bool SetupToApplySINWave_2(bool use50Hz, const std::string &voltsRMSAsString)
	{
		return SetupToApplySINWave_ByChannel(use50Hz, voltsRMSAsString, true);
	}

	// channel 1
	bool EnableOutput()
	{
		return WriteCommand(":OUTP1 ON");
	}

	// channel 2
	bool EnableOutput_2()
	{
		return WriteCommand(":OUTP2 ON");
	}

	// channel 1
	bool DisableOutput()
	{
		return WriteCommand(":OUTP1 OFF");
	}

	// channel 2
	bool DisableOutput_2()
	{
		return WriteCommand(":OUTP2 OFF");
	}

	// channel 2
	bool SendChannel2Phase180()
	{
		return WriteCommand(":SOUR2:PHAS 180");
	}

	bool SendSyncChannels()
	{
		bool retval = true;
		retval &= WriteCommand(":SOUR1:PHAS:INIT");
		Sleep(1000);
		retval &= WriteCommand(":SOUR2:PHAS:SYNC");
		return retval;
	}

	// issue the commands needed to turn on the sync output
	// :OUTPut 1:SYNC:ON
	// :OUTPUT 1:SYNC:DELAY 0
	// :OUTPUT 1:SYNC:POLARITY POSITIVE
	// after every command, we issue a query for that parameter, and make sure
	// it is as expected
	bool SetupSyncOutput()
	{
		bool InitOK = true;
		std::string response;

		/////////////////////////////////////////////
		// step 1: set the output to sync mode
		/////////////////////////////////////////////

		if (InitOK)
		{
			InitOK = WriteCommand(":OUTPut 1:SYNC:ON");
		}

		if (InitOK)
		{
			InitOK = WriteCommandReadResponse(":OUTP1:SYNC?", response);
		}

		if (InitOK)
		{
			// check for default, out of reset, known values
			InitOK = (response.find("ON") != std::string::npos);
		}

		/////////////////////////////////////////////
		// step 2: set the delay to 0
		/////////////////////////////////////////////

		if (InitOK)
		{
			InitOK = WriteCommand(":OUTPUT 1:SYNC:DELAY 0");
		}

		if (InitOK)
		{
			InitOK = WriteCommandReadResponse(":OUTP1:SYNC:DEL?", response);
		}

		if (InitOK)
		{
			// check for default, out of reset, known values
			InitOK = (response.find("0.0") != std::string::npos);
		}

		/////////////////////////////////////////////
		// step 3
		/////////////////////////////////////////////

		if (InitOK)
		{
			InitOK = WriteCommand(":OUTPUT 1:SYNC:POLARITY POSITIVE");
		}

		if (InitOK)
		{
			InitOK = WriteCommandReadResponse(":OUTP1:SYNC:POL?", response);
		}

		if (InitOK)
		{
			// check for default, out of reset, known values
			InitOK = (response.find("POS") != std::string::npos);
		}

		return InitOK;
	}

	static void SendSCPICommand(const std::string &command)
	{
		std::string response;
		bool isQuery = (command.find("?") != std::string::npos);

		if (isQuery)
		{
			// send query
			// and get the response back and print it

			PrintToScreen(command);

			if (WriteCommandReadResponse(command, response))
			{
				PrintToScreen(" " + response);
			}
			else
			{
				PrintToScreen("Failed to send command: " + command);
			}
		}
		else
		{
			// just send the command, don't expect a response
			PrintToScreen(command);
			WriteCommand(command);
		}
	}

	// read in all the lines from a text file, and send each in turn to the signal generator
	// for query commands, print the response to the screen
	static void ProcessLines(
		const std::string &filename,
		const std::function<void(const std::string &)> &processLine)
	{
		std::ifstream file(filename);
		if (!file.is_open())
		{
			PrintToScreen("Failed to open file: " + filename);
			return;
		}

		std::string line;
		while (std::getline(file, line))
		{
			processLine(line); // call the function that was passed in
		}

		file.close();
	}

	// Example usage with a lambda that prints each line
	void ProcessSCPIScript(const std::string &filename)
	{
		PrintToScreen("====================================");
		PrintToScreen("Processing SCPI script: " + filename);
		PrintToScreen("====================================");
		PrintToScreen("");
		ProcessLines(filename, [](const std::string &line)
					 { 
						SendSCPICommand(line);
						PrintToScreen("waiting two seconds between SCPI commands..."); 
						Sleep(2000); });
	}
}
