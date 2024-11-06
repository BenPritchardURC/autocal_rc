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

#pragma once

#include <stdbool.h>
#include <windows.h>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <functional>

#include "..\trip4.hpp"

std::unordered_map<std::string, std::string> ParseTripUnitSettingsFile(const std::string &filename);

bool SetSystemAndDeviceSettings(
    HANDLE hTripUnit,
    const std::unordered_map<std::string, std::string> &newValuesFromFile,
    bool &SettingsUpdatedOnTripUnit,
    const SetSettingsFuncPtr &funcptr);

// convenience function that just calls SetSystemAndDeviceSettings() with a lambda that does nothing
bool SetSystemAndDeviceSettings(
    const HANDLE hTripUnit,
    const std::unordered_map<std::string, std::string> &newValuesFromFile,
    bool &SettingsUpdatedOnTripUnit);

bool ReadSystemSettingsFromFile(
    const std::string &filename,
    std::unordered_map<std::string, std::string> &newValuesFromFile);

// convenience function that just calls SetSystemAndDeviceSettingsFromFile() with a lambda that does nothing
bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit,
    const std::string &filename, bool &SettingsUpdatedOnTripUnit);

bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit, const std::string &filename,
    bool &SettingsUpdatedOnTripUnit, const SetSettingsFuncPtr &funcptr);

// convenience function that just calls SetSystemAndDeviceSettingsFromFile()
// with a lambda that does nothing, and doesn't fill in the SettingsUpdatedOnTripUnit variable
bool SetSystemAndDeviceSettingsFromFile(
    HANDLE hTripUnit,
    const std::string &filename);
