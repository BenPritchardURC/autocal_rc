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

#include <fstream>
#include <sstream>
#include <unordered_map>

// returns an int value from a string
template <typename T>
static T stringToNum(const std::string &str)
{
    long num = std::stol(str);
    return static_cast<T>(num);
}

// sets valueT to the value from the map for the key
template <typename T>
static bool SetValueWithPointer(
    const std::unordered_map<std::string, std::string> &nameValuePairs,
    const std::string &key, T *valueT)
{
    bool valueGotChanged = false;

    // is the key we are looking for in the nameValuePairs map?
    if (nameValuePairs.find(key) != nameValuePairs.end())
    {

        // set the value in the struct (using its pointer) to the value in the map
        *valueT = stringToNum<T>(nameValuePairs.at(key));

        // remember that we changed something
        valueGotChanged = true;
    }

    return valueGotChanged;
}

// sets boolValue to be true/false based on the value from the map for the key
static bool SetBool8ValueWithPointer(
    const std::unordered_map<std::string, std::string> &nameValuePairs,
    const std::string &key, bool8 *boolValue)
{
    bool valueGotChanged = false;

    // values we interpret to be "true"
    std::string true_values[] = {"1", "true", "True", "TRUE", "yes", "Yes", "YES"};

    // is the key we are looking for in the nameValuePairs map?
    if (nameValuePairs.find(key) != nameValuePairs.end())
    {
        // grab value of the key
        auto value_as_string = nameValuePairs.at(key);

        // does the value represent a "true" value?
        *boolValue = std::find(std::begin(true_values), std::end(true_values), value_as_string) != std::end(true_values);

        // remember that we changed something
        valueGotChanged = true;
    }

    return valueGotChanged;
}

// returns true if anything was changed
static bool PopulateSysSettings(
    const std::unordered_map<std::string, std::string> &nameValuePairs,
    SystemSettings4 &sysSettings)
{
    bool anythingChanged = false;

    //*********************************************
    // handle all the uint8_t values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, uint8_t *> uint8_Values =
        {
            {"BreakerL", &sysSettings.BreakerL},
            {"Frequency", &sysSettings.Frequency},
            {"BkrPosCntType", &sysSettings.BkrPosCntType},
            {"PowerReversed", &sysSettings.PowerReversed},
            {"AutoPolarityCT", &sysSettings.AutoPolarityCT},
            {"TripUnitMode", &sysSettings.TripUnitMode},
            {"TypePT", &sysSettings.TypePT}};

    for (auto &pair8 : uint8_Values)
    {
        try
        {
            anythingChanged |= SetValueWithPointer<uint8_t>(nameValuePairs, pair8.first, pair8.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pair8.first + " must be of type uint8_t");
            return false;
        }
    }

    //*********************************************
    // handle all the uint16_t values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, uint16_t *> uint16_Values =
        {
            {"CTNeutralRating", &sysSettings.CTNeutralRating},
            {"CTRating", &sysSettings.CTRating},
            {"CTSecondary", &sysSettings.CTSecondary},
            {"CTNeutralSecondary", &sysSettings.CTNeutralSecondary},
            {"RatioPT", &sysSettings.RatioPT},
            {"CoilSensitivity", &sysSettings.CoilSensitivity},
            {"SensorType", &sysSettings.SensorType}};

    for (auto &pair16 : uint16_Values)
    {
        try
        {
            anythingChanged |= SetValueWithPointer<uint16_t>(nameValuePairs, pair16.first, pair16.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pair16.first + " must be of type uint16_t");
            return false;
        }
    }

    //*********************************************
    // handle all the bool8 values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, bool8 *> bool8_Values =
        {{"HPCdevice", &sysSettings.HPCdevice}};

    for (auto &pairBool : bool8_Values)
    {
        try
        {
            anythingChanged |= SetBool8ValueWithPointer(nameValuePairs, pairBool.first, pairBool.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pairBool.first + " must be of type bool8");
            return false;
        }
    }

    return anythingChanged;
}

// returns true if anything was changed
static bool PopulateDeviceSettings(
    const std::unordered_map<std::string, std::string> nameValuePairs,
    DeviceSettings4 &deviceSettings)
{
    bool anythingChanged = false;

    //*********************************************
    // handle all the uint8_t values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, uint8_t *> uint8_Values =
        {
            {"LTThermMem", &deviceSettings.LTThermMem},
            {"STI2T", &deviceSettings.STI2T},
            {"GFI2T", &deviceSettings.GFI2T},
            {"GFType", &deviceSettings.GFType},
            {"GFI2TAmps", &deviceSettings.GFI2TAmps},
            {"GFQTType", &deviceSettings.GFQTType},
            {"UBEnabled", &deviceSettings.UBEnabled},
            {"OVTripDelayLL", &deviceSettings.OVTripDelayLL},
            {"UVTripDelayLL", &deviceSettings.UVTripDelayLL},
            {"OFEnabled", &deviceSettings.OFEnabled},
            {"UFEnabled", &deviceSettings.UFEnabled},
            {"PVLossEnabled", &deviceSettings.PVLossEnabled},
            {"PVLossDelay", &deviceSettings.PVLossDelay},
            {"SystemRotation", &deviceSettings.SystemRotation},
            {"NSOVPickupPercent", &deviceSettings.NSOVPickupPercent},
            {"RPDelay", &deviceSettings.RPDelay},
            {"RPEnabled", &deviceSettings.RPEnabled},
            {"SBThreshold", &deviceSettings.SBThreshold},
            {"DateFormat", &deviceSettings.DateFormat},
            {"LanguageID", &deviceSettings.LanguageID},
            {"NeutralProtectionPercent", &deviceSettings.NeutralProtectionPercent},
            {"ZoneBlockBits_0", &deviceSettings.ZoneBlockBits[0]},
            {"ZoneBlockBits_1", &deviceSettings.ZoneBlockBits[1]},
            {"ProgrammableRelay_0", &deviceSettings.ProgrammableRelay[0]},
            {"ProgrammableRelay_1", &deviceSettings.ProgrammableRelay[1]},
            {"ThresholdPhaseCT", &deviceSettings.ThresholdPhaseCT},
            {"ThresholdNeutralCT", &deviceSettings.ThresholdNeutralCT},
            {"DINF", &deviceSettings.DINF}};

    for (auto &pair8 : uint8_Values)
    {
        try
        {
            anythingChanged |= SetValueWithPointer<uint8_t>(nameValuePairs, pair8.first, pair8.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pair8.first + " must be of type uint8_t");
            return false;
        }
    }

    //*********************************************
    // handle all the uint16_t values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, uint16_t *> uint16_Values =
        {
            {"LTDelayx10", &deviceSettings.LTDelay},
            {"STDelayx100", &deviceSettings.STDelay},
            {"GFDelayx100", &deviceSettings.GFDelay},
            {"UBPickup", &deviceSettings.UBPickup},
            {"UBDelay", &deviceSettings.UBDelay},
            {"OVTripPickupLL", &deviceSettings.OVTripPickupLL},
            {"OVAlarmPickupLL", &deviceSettings.OVAlarmPickupLL},
            {"UVTripPickupLL", &deviceSettings.UVTripPickupLL},
            {"UVAlarmPickupLL", &deviceSettings.UVAlarmPickupLL},
            {"OFValueTrip", &deviceSettings.OFValueTrip},
            {"OFValueAlarm", &deviceSettings.OFValueAlarm},
            {"UFValueTrip", &deviceSettings.UFValueTrip},
            {"UFValueAlarm", &deviceSettings.UFValueAlarm},
            {"RPPickup", &deviceSettings.RPPickup},
            {"AlarmRelayTU", &deviceSettings.AlarmRelayTU},
            {"AlarmRelayHI", &deviceSettings.AlarmRelayHI},
            {"AlarmRelayRIU1", &deviceSettings.AlarmRelayRIU1},
            {"AlarmRelayRIU2", &deviceSettings.AlarmRelayRIU2}};

    for (auto &pair16 : uint16_Values)
    {
        try
        {
            anythingChanged |= SetValueWithPointer<uint16_t>(nameValuePairs, pair16.first, pair16.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pair16.first + " must be of type uint16_t");
            return false;
        }
    }

    //*********************************************
    // handle all the uint32_t values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, uint32_t *> uint32_Values =
        {
            {"LTPickupx10", &deviceSettings.LTPickup},
            {"STPickup", &deviceSettings.STPickup},
            {"GFPickup", &deviceSettings.GFPickup},
            {"QTInstPickup", &deviceSettings.QTInstPickup},
            {"InstantPickup", &deviceSettings.InstantPickup},
            {"GFQTPickup", &deviceSettings.GFQTPickup}};

    for (auto &pair32 : uint32_Values)
    {
        try
        {
            anythingChanged |= SetValueWithPointer<uint32_t>(nameValuePairs, pair32.first, pair32.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pair32.first + " must be of type uint32_t");
            return false;
        }
    }

    //*********************************************
    // handle all the bool8 values
    //*********************************************

    // mapping of field names to pointers to the fields in the struct
    std::unordered_map<std::string, bool8 *> bool8_Values =
        {
            {"LTEnabled", &deviceSettings.LTEnabled},
            {"STEnabled", &deviceSettings.STEnabled},
            {"InstantEnabled", &deviceSettings.InstantEnabled},
            {"OVTripEnabledLL", &deviceSettings.OVTripEnabledLL},
            {"UVTripEnabledLL", &deviceSettings.UVTripEnabledLL},
            {"USBTripEnabled", &deviceSettings.USBTripEnabled},
            {"ModbusForcedTripEnabled", &deviceSettings.ModbusForcedTripEnabled},
            {"ModbusChangeUserSettingsEnabled", &deviceSettings.ModbusChangeUserSettingsEnabled},
            {"ModbusSoftQuickTripSwitchEnabled", &deviceSettings.ModbusSoftQuickTripSwitchEnabled}};

    for (auto &pairBool : bool8_Values)
    {
        try
        {
            anythingChanged |= SetBool8ValueWithPointer(nameValuePairs, pairBool.first, pairBool.second);
        }
        catch (std::exception &e)
        {
            PrintToScreen("Error: " + pairBool.first + " must be of type bool8");
            return false;
        }
    }

    return anythingChanged;
}

// return map of name:value pairs from the indicated file
std::unordered_map<std::string, std::string> ParseTripUnitSettingsFile(
    const std::string &filename)
{

    std::string line;
    std::unordered_map<std::string, std::string> nameValuePairs;

    std::ifstream file(filename);

    if (!file)
    {
        // Handle the error, e.g., by throwing an exception
        throw std::runtime_error("Failed to open file: " + filename);
    }

    // create a map of key:value pairs from the file
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        std::string key;
        std::string value;

        if (std::getline(iss, key, ':') && iss >> value)
        {
            nameValuePairs[key] = value;
        }
        // otherwise just ignore the line
    }

    return nameValuePairs;
}

// grabs existing system and device settings from the trip unit
// call a function that can update SystemSettings4 or DeviceSettings4 after they are read from the file but before they are sent to the trip unit
// if the file contains new values, or the function updated the values, then send all the values back down to the trip unit
bool SetSystemAndDeviceSettings(
    HANDLE hTripUnit,
    const std::unordered_map<std::string, std::string> &newValuesFromFile,
    bool &SettingsUpdatedOnTripUnit,
    const SetSettingsFuncPtr &funcptr)
{
    _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

    URCMessageUnion rsp1 = {0}; // response to MSG_GET_SYS_SETTINGS
    URCMessageUnion rsp2 = {0}; // response to MSG_GET_DEV_SETTINGS
    URCMessageUnion rsp3 = {0}; // response to MSG_SET_USR_SETTINGS_4

    SystemSettings4 sysSettings = {0};
    DeviceSettings4 deviceSettings = {0};

    bool retval = true;
    SettingsUpdatedOnTripUnit = false;

    // grab existing system settings from trip unit
    if (retval)
    {
        SendURCCommand(hTripUnit, MSG_GET_SYS_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
        retval = GetURCResponse(hTripUnit, &rsp1) &&
                 VerifyMessageIsOK(&rsp1, MSG_RSP_SYS_SETTINGS_4, sizeof(MsgRspSysSet4) - sizeof(MsgHdr));

        if (!retval)
        {
            PrintToScreen("error receiving MSG_RSP_SYS_SETTINGS_4");
        }
    }

    if (retval)
    {
        // save a copy of existing system settings
        sysSettings = rsp1.msgRspSysSet4.Settings;
    }

    // grab existing device settings from trip unit
    if (retval)
    {
        SendURCCommand(hTripUnit, MSG_GET_DEV_SETTINGS, ADDR_TRIP_UNIT, ADDR_CAL_APP);
        retval = GetURCResponse(hTripUnit, &rsp2) &&
                 VerifyMessageIsOK(&rsp2, MSG_RSP_DEV_SETTINGS_4, sizeof(MsgRspDevSet4) - sizeof(MsgHdr));

        if (!retval)
        {
            PrintToScreen("error receiving MSG_RSP_DEV_SETTINGS_4");
        }
    }

    if (retval)
    {
        // save a copy of existing device settings
        deviceSettings = rsp2.msgRspDevSet4.Settings;
    }

    if (retval)
    {
        // first find all the system settings in the file
        bool sysSettingsChanged = PopulateSysSettings(newValuesFromFile, sysSettings);

        // then find all the device settings in the file
        bool deviceSettingsChanged = PopulateDeviceSettings(newValuesFromFile, deviceSettings);

        // call the function that can update SystemSettings4 or DeviceSettings4, to see if it changes any values
        bool functionChangedValues = funcptr(&sysSettings, &deviceSettings);

        bool ShouldUpdateTripUnit = false;

        // did the file or the routine set any values?
        if (sysSettingsChanged || deviceSettingsChanged || functionChangedValues)
        {
            // see if anything actually changed from what we read from the trip unit
            ShouldUpdateTripUnit =
                (0 != std::memcmp(&sysSettings, &rsp1.msgRspSysSet4.Settings, sizeof(sysSettings))) ||
                (0 != std::memcmp(&deviceSettings, &rsp2.msgRspDevSet4.Settings, sizeof(deviceSettings)));
        }

        if (ShouldUpdateTripUnit)
        {
            // if so, send all the values back down to the trip unit
            SendSetUserSet4(hTripUnit, &sysSettings, &deviceSettings);
            retval = GetURCResponse(hTripUnit, &rsp3);
            if (retval)
            {
                // because of the checks above, we should never get back a NAK
                // saying nothing changed
                retval =
                    (MSG_ACK == rsp3.msgHdr.Type);
            }

            if (retval)
                SettingsUpdatedOnTripUnit = true;
            else
            {
                PrintToScreen("did not receive ACK from MSG_SET_USR_SETTINGS_4");
                if (MSG_NAK == rsp3.msgHdr.Type)
                {
                    PrintToScreen("NAK Code: " + std::to_string(rsp3.msgNAK.Error));
                    PrintToScreen("NAK Meaning: " + NAKCodeToString(rsp3.msgNAK.Error));
                }
            }
        }
    }

    return retval;
}

// convenience function that just calls SetSystemAndDeviceSettings() with a lambda that does nothing
bool SetSystemAndDeviceSettings(
    const HANDLE hTripUnit,
    const std::unordered_map<std::string, std::string> &newValuesFromFile,
    bool &SettingsUpdatedOnTripUnit)
{
    // just call SetSystemAndDeviceSettings with a lambda that does nothing
    return SetSystemAndDeviceSettings(
        hTripUnit, newValuesFromFile, SettingsUpdatedOnTripUnit,
        [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4) -> bool
        { return false; });
}

bool ReadSystemSettingsFromFile(
    const std::string &filename,
    std::unordered_map<std::string, std::string> &newValuesFromFile)
{
    bool retval = true;

    // read in new values from the file
    if (retval)
    {
        try
        {
            newValuesFromFile = ParseTripUnitSettingsFile(filename);
        }
        catch (std::exception &e)
        {
            return false;
        }
    }

    return true;
}

// reads in the values from a file
// takes a function that SetSystemAndDeviceSettings() will call that can update SystemSettings4 or DeviceSettings4
// after they are read from the file but before they are sent to the trip unit
bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit, const std::string &filename,
    bool &SettingsUpdatedOnTripUnit, const SetSettingsFuncPtr &funcptr)
{
    _ASSERT(hTripUnit != INVALID_HANDLE_VALUE);

    std::unordered_map<std::string, std::string> newValuesFromFile;
    bool retval = true;

    // read in new values from the file
    if (retval)
    {
        retval = ReadSystemSettingsFromFile(filename, newValuesFromFile);
    }

    if (retval)
    {
        retval = SetSystemAndDeviceSettings(
            hTripUnit, newValuesFromFile, SettingsUpdatedOnTripUnit, funcptr);
    }

    return retval;
}

// convenience function that just calls SetSystemAndDeviceSettingsFromFile() with a lambda that does nothing
bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit,
    const std::string &filename, bool &SettingsUpdatedOnTripUnit)
{
    // just call SetSystemAndDeviceSettingsFromFile() with a lambda that does nothing
    return SetSystemAndDeviceSettingsFromFile(
        hTripUnit, filename, SettingsUpdatedOnTripUnit,
        [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4) -> bool
        { return false; });
}

// convenience function that just calls SetSystemAndDeviceSettingsFromFile()
// with a lambda that does nothing, and doesn't fill in the SettingsUpdatedOnTripUnit variable
bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit,
    const std::string &filename)

{
    bool ignoreMe;

    // just call SetSystemAndDeviceSettingsFromFile() with a lambda that does nothing
    return SetSystemAndDeviceSettingsFromFile(
        hTripUnit, filename, ignoreMe,
        [](SystemSettings4 *Settings, DeviceSettings4 *DevSettings4) -> bool
        { return false; });
}
