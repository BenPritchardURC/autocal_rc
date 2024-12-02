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
#include <vector>

// note: this includes the file for the VISA Library 5.1 specification, not something
//       not something from our project. Normally it should be located at:
//		 C:\Program Files\IVI Foundation\VISA\Win64\Include\visa.h
#include <C:\Program Files\IVI Foundation\VISA\Win64\Include\visa.h>

// VISA =  (Virtual Instrument Software Architecture) library
// NI-VISA = National Instruments VISA
// USBTMC = USB Test and Measurement Class
// SCPI = Standard Commands for Programmable Instruments

/* notes from BKprecision website:

B&K Precision Corporation

USBTMC Driver Readme

USBTMC (USB Test & Measurement Class) is one type of USB protocol that is commonly used for remote communication
with test and measurement instrumentations.  It's behavior allows for GPIB-like communication over USB interface.
Additionally, it supports service request, triggers, and other specific GPIB operations.

Unlike most types of USB devices that have specific drivers, USBTMC devices only require VISA installed on the computer.
VISA will have the driver installed automatically for USBTMC communication.

There are different VISA drivers available.  B&K Precision recommends using NI-VISA.

This can be downloaded here:
https://www.ni.com/en-us/support/downloads/drivers/download.ni-visa.html#346210

Select the link for "Downloads".  You may download any NI-VISA version 3.0 or above, but the latest version is recommended.  Once NI-VISA is installed,
connect the USBTMC compliant instrument to the computer and the drivers will install automatically.  The same automatic
driver installation will occur for all other USBTMC compliant devices.

For remote communication using USBTMC, we recommend using VISA drivers.

Before connecting, please make sure the USBTMC communication setting is enabled on your instrument.

*/

namespace BK_PRECISION_9801
{
    constexpr int BK_9801_MAX_VOLTS_RMS = 300;

    // default time of 5 seconds to wait for a response from the instrument
    ViUInt32 timeout_ms = 5000;

    ViSession session;
    ViSession resource_manager;
    ViStatus status;

    void PrintError(std::string msg)
    {
        PrintToScreen("BK_PRECISION_9801 Error: " + msg);
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
            PrintError("no BK_PRECISION_9801 VISA instruments found");
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

                    if (retval)
                    {
                        if (response.find("BK PRECISION, 9801") != std::string::npos)
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
            PrintError("no BK_PRECISION_9801 VISA instruments found");
            return false;
        }
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

        std::string command_with_terminator = command + "\r\n";

        ViBuf scpi_command = (ViBuf)command_with_terminator.c_str();
        ViUInt32 bytes_to_write = (ViUInt32)command_with_terminator.length();
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

    // we are writing the command to the signal generator as a string,
    // so i think it makes sense to pass the voltage as one here
    bool SetupToApplySINWave(bool use50Hz, const std::string &voltsRMSAsString)
    {
        bool retval = true;
        std::string cmd_to_apply;
        std::string expected_string_back;
        std::string actual_string_back;

        // convert voltsRMSAsString to a double
        double voltsRMS = std::stod(voltsRMSAsString);

        retval = true;

        // check to make sure RMS is less than or equal to 30vRMS
        if (retval)
        {
            // this code is setup to be on the other side of an attenuation network
            // that scales the voltage down
            voltsRMS *= 7.930951;

            // verify is less than BK_9801_MAX_VOLTS_RMS
            if (voltsRMS > BK_9801_MAX_VOLTS_RMS)
            {
                PrintError("voltage exceeds maximum limit");
                PrintError("this build of AUTOCAL_Lite only supports " + std::to_string(BK_9801_MAX_VOLTS_RMS) + " vRMS or less at this time");
                return false;
            }
        }
        // set frequency
        if (retval)
        {
            if (use50Hz)
                cmd_to_apply = ":SOUR:FREQ 50";
            else
                cmd_to_apply = ":SOUR:FREQ 60";

            retval = WriteCommand(cmd_to_apply);
        }

        if (retval)
        {
            // maybe we need a certain amount of time between commands??
            Sleep(1000);
        }

        // set voltage
        if (retval)
        {
            // convert back to string after scaling up
            std::string scaledVoltsRMSAsString = std::to_string(voltsRMS);
            cmd_to_apply = ":SOUR:VOLT " + scaledVoltsRMSAsString;
            retval = WriteCommand(cmd_to_apply);
        }

        // output must be intentionally enabled using EnableOutput()

        return retval;
    }

    // reset signal generator to default settings
    bool Initialize()
    {
        bool retval = true;

        if (retval)
            retval = WriteCommand("*RST");
        if (retval)
            retval = WriteCommand("SYSTem:REMote");

        return retval;
    }

    bool EnableOutput()
    {
        return WriteCommand("OUTP ON");
    }

    bool DisableOutput()
    {
        return WriteCommand("OUTP OFF");
    }
}
