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

namespace FTDI
{

    int FindTripUnitFTDIIndex(int TripUnitCommPort)
    {
        FT_STATUS ftStatus;
        DWORD numDevs;
        FT_HANDLE fthandle;
        LONG COMPORT;

        // figure out how many FTDI devices there are
        ftStatus = FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
        if (ftStatus == FT_OK)
        {
            // then go through each one and open it to see what COM port it is
            for (int i = 0; i < numDevs; i++)
            {
                if (FT_Open(i, &fthandle) == FT_OK)
                {
                    if (FT_GetComPortNumber(fthandle, &COMPORT) == FT_OK)
                    {
                        FT_Close(fthandle);
                        if (COMPORT == TripUnitCommPort)
                        {
                            return i;
                        }
                    }
                }
            }
        }
        return NO_FTDI_INDEX;
    }

    void DumpEEProm(FT_HANDLE handle)
    {
        FT_STATUS ftStatus;

        FT_PROGRAM_DATA ftData;
        char ManufacturerBuf[32];
        char ManufacturerIdBuf[16];
        char DescriptionBuf[64];
        char SerialNumberBuf[16];

        ftData.Signature1 = 0x00000000;
        ftData.Signature2 = 0xffffffff;
        ftData.Version = 0x00000005; // EEPROM structure with FT232H extensions
        ftData.Manufacturer = ManufacturerBuf;
        ftData.ManufacturerId = ManufacturerIdBuf;
        ftData.Description = DescriptionBuf;
        ftData.SerialNumber = SerialNumberBuf;

        ftStatus = FT_EE_Read(handle, &ftData);

        if (ftStatus == FT_OK)
        {
            scr_printf("Signature1 = %X", ftData.Signature1);
            scr_printf("Signature2 = %X", ftData.Signature2);
            scr_printf("Version = %X", ftData.Version);
            scr_printf("VendorId = %X", ftData.VendorId);
            scr_printf("ProductId = %X", ftData.ProductId);
            scr_printf("Manufacturer = %s", ftData.Manufacturer);
            scr_printf("ManufacturerId = %s", ftData.ManufacturerId);
            scr_printf("Description = %s", ftData.Description);
            scr_printf("SerialNumber = %s", ftData.SerialNumber);
        }
        else
        {
            scr_printf("FT_EE_Read Failed");
        }
    }

    bool VerifyEEPROM(const char *expected_mfg, const char *expected_description, int FTDI_Index)
    {
        FT_STATUS ftStatus;
        FT_HANDLE ftHandle;
        bool retval;

        FT_PROGRAM_DATA ftData;
        char ManufacturerBuf[32];
        char ManufacturerIdBuf[16];
        char DescriptionBuf[64];
        char SerialNumberBuf[16];

        ftData.Signature1 = 0x00000000;
        ftData.Signature2 = 0xffffffff;
        ftData.Version = 0x00000005; // EEPROM structure with FT232H extensions
        ftData.Manufacturer = ManufacturerBuf;
        ftData.ManufacturerId = ManufacturerIdBuf;
        ftData.Description = DescriptionBuf;
        ftData.SerialNumber = SerialNumberBuf;

        retval = false;

        // if everything goes right...
        if (FT_Open(FTDI_Index, &ftHandle) == FT_OK)
        {

            if (FT_EE_Read(ftHandle, &ftData) == FT_OK)
                if (strcmp(ManufacturerBuf, expected_mfg) == 0)
                    if (strcmp(DescriptionBuf, expected_description) == 0)
                        retval = true; // we are golden!

            FT_Close(ftHandle);
        }

        // otherwise: no dice
        return retval;
    }

    // right now, this just opens the first FTDI device it finds
    void EnumerateFTDI()
    {
        DWORD numDevs;
        FT_HANDLE ftHandle;
        FT_STATUS ftStatus;

        // figure out how many FTDI devices there are
        if (FT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY) == FT_OK)
        {
            scr_printf("Number of FTDI devices: %d", numDevs);
            for (int i = 0; i < numDevs; i++)
            {
                if (ftStatus = FT_Open(i, &ftHandle) == FT_OK)
                {
                    DumpEEProm(ftHandle);
                    scr_printf("");
                    FT_Close(ftHandle);
                }
            }
        }
    }

    // read existing EEPROM info
    // update the manufacturer and description
    bool ProgramEEPROM(const char *mfg, const char *description, int FTDI_Index)
    {
        FT_HANDLE ftHandle;
        FT_STATUS ftStatus;
        FT_PROGRAM_DATA ftData;

        char ManufacturerBuf[32] = {0};
        char ManufacturerIdBuf[16] = {0};
        char DescriptionBuf[64] = {0};
        char SerialNumberBuf[16] = {0};

        ftData.Signature1 = 0x00000000;
        ftData.Signature2 = 0xffffffff;
        ftData.Version = 0x00000005; // EEPROM structure with FT232H extensions
        ftData.Manufacturer = ManufacturerBuf;
        ftData.ManufacturerId = ManufacturerIdBuf;
        ftData.Description = DescriptionBuf;
        ftData.SerialNumber = SerialNumberBuf;

        if (FT_Open(FTDI_Index, &ftHandle) != FT_OK)
            return false;

        if (FT_EE_Read(ftHandle, &ftData) != FT_OK)
        {
            FT_Close(ftHandle);
            return false;
        }

        memcpy(ManufacturerBuf, mfg, sizeof(ManufacturerBuf));
        memcpy(DescriptionBuf, description, sizeof(DescriptionBuf));

        if (FT_EE_Program(ftHandle, &ftData) != FT_OK)
        {
            FT_Close(ftHandle);
            return false;
        }

        FT_Close(ftHandle);
        return true;
    }
}