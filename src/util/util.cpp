
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

#include "util.hpp"
#include <sstream>
#include <iomanip>
#include <map>
#include <string>
#include <cstdlib> // For std::strtof
#include <cstring> // For std::strlen

bool InRange(double value, double min, double max)
{
    return (value >= min) && (value <= max);
}

std::string Tab(int num)
{
    switch (num)
    {
    case 0:
        return "";
    case 1:
        return "    ";
    case 2:
        return "        ";
    case 3:
        return "            ";
    case 4:
        return "                ";
    case 5:
        return "                    ";
    default:
        return "";
    }
}

std::string ZeroOrOne(bool b)
{
    return b ? "1" : "0";
}

std::string BoolToString(bool b)
{
    return b ? "True" : "False";
}

std::string Dots(int numDots, std::string prefix)
{
    int dotsNeeded = numDots - prefix.length();
    if (dotsNeeded <= 0)
        return prefix;
    return prefix + std::string(dotsNeeded, '.');
}

float PercentDifference(float a, float b)
{
    if (0 == a || 0 == b)
        return 100;
    return 100 * std::abs(a - b) / ((a + b) / 2);
}

std::string BoolToPassFail(bool pass)
{
    return pass ? "Pass" : "Fail";
}

std::string BoolToYesNo(bool pass)
{
    return pass ? "Yes" : "No";
}

int NumSecondsElapsed(std::chrono::system_clock::time_point StartTime)
{
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end - StartTime;
    return (int)elapsed_seconds.count();
}

std::string NAKCodeToString(int code)
{
    std::map<int, std::string> errorCodes = {
        {NAK_ERROR_UNDEF, "NAK_ERROR_UNDEF"},
        {NAK_ERROR_CHECKSUM, "NAK_ERROR_CHECKSUM"},
        {NAK_ERROR_UNREC, "NAK_ERROR_UNREC"},
        {NAK_ERROR_NOT_READY, "NAK_ERROR_NOT_READY"},
        {NAK_ERROR_PROTOCOL, "NAK_ERROR_PROTOCOL"},
        {NAK_ERROR_NO_DATA, "NAK_ERROR_NO_DATA"},
        {NAK_ERROR_INVALID_PARAMETERS, "NAK_ERROR_INVALID_PARAMETERS"},
        {NAK_ERROR_INVALID_LENGTH, "NAK_ERROR_INVALID_LENGTH"},
        {NAK_ERROR_SOURCE, "NAK_ERROR_SOURCE"},
        {NAK_ERROR_TARGET, "NAK_ERROR_TARGET"},
        {NAK_ERROR_SAMPLE_TYPE, "NAK_ERROR_SAMPLE_TYPE"},
        {NAK_ERROR_INVALID_PARAMETER, "NAK_ERROR_INVALID_PARAMETER"},
        {NAK_ERROR_RS485_ADDR, "NAK_ERROR_RS485_ADDR"},
        {NAK_ERROR_NO_CODE, "NAK_ERROR_NO_CODE"},
        {NAK_ERROR_FRAM_LOW_VOLT, "NAK_ERROR_FRAM_LOW_VOLT"},
        {NAK_ERROR_FRAM_TRIP_NOW, "NAK_ERROR_FRAM_TRIP_NOW"},
        {NAK_ERROR_NOT_REQUESTED, "NAK_ERROR_NOT_REQUESTED"},
        {NAK_ERROR_NOT_SUPPORTED, "NAK_ERROR_NOT_SUPPORTED"},
        {NAK_ERROR_CALIBRATING, "NAK_ERROR_CALIBRATING"},
        {NAK_ERROR_FLASHWRITE, "NAK_ERROR_FLASHWRITE"},
        {NAK_ERROR_UNABLE_TO_DO, "NAK_ERROR_UNABLE_TO_DO"},
        {NAK_ERROR_FRAM_FAULT, "NAK_ERROR_FRAM_FAULT"},
        {NAK_ERROR_FACTORY_ONLY, "NAK_ERROR_FACTORY_ONLY"},
        {NAK_ERROR_NO_RESPONSE, "NAK_ERROR_NO_RESPONSE"},
        {NAK_ERROR_QT_DISPLAY_II_CRC, "NAK_ERROR_QT_DISPLAY_II_CRC"},
        {NAK_ERROR_TIMEOUT_TOO_SLOW, "NAK_ERROR_TIMEOUT_TOO_SLOW"},
        {NAK_ERROR_PERSONALITY4_STRUCT_ID, "NAK_ERROR_PERSONALITY4_STRUCT_ID"},
        {NAK_ERROR_NO_RELAY_ASSIGNED, "NAK_ERROR_NO_RELAY_ASSIGNED"},
        {NAK_ERROR_INVALID_A2D_READING, "NAK_ERROR_INVALID_A2D_READING"},
        {NAK_ERROR_BT_NOT_AVAILABLE, "NAK_ERROR_BT_NOT_AVAILABLE"},
        {NAK_ERROR_PERSONALITY4_CC_OPTIONS, "NAK_ERROR_PERSONALITY4_CC_OPTIONS"},
        {NAK_ERROR_BAD_SOFTKEY_CHAR, "NAK_ERROR_BAD_SOFTKEY_CHAR"},
        {NAK_ERROR_BAD_LINE_CHAR, "NAK_ERROR_BAD_LINE_CHAR"},
        {NAK_ERROR_NO_END_OF_TEXT, "NAK_ERROR_NO_END_OF_TEXT"},
        {NAK_ERROR_BLANK_SOFTKEY, "NAK_ERROR_BLANK_SOFTKEY"},
        {NAK_ERROR_ALREADY_CALIBRATING, "NAK_ERROR_ALREADY_CALIBRATING"},
        {NAK_ERROR_CANNOT_DO_BOTH_HI_AND_LO, "NAK_ERROR_CANNOT_DO_BOTH_HI_AND_LO"},
        {NAK_ERROR_MUST_DO_HI_GAIN_FIRST, "NAK_ERROR_MUST_DO_HI_GAIN_FIRST"},
        {NAK_ERROR_ILLEGAL_GAIN_SETTING, "NAK_ERROR_ILLEGAL_GAIN_SETTING"},
        {NAK_ERROR_RAILOUT_HI_GAIN, "NAK_ERROR_RAILOUT_HI_GAIN"},
        {NAK_ERROR_RAILOUT_LO_GAIN, "NAK_ERROR_RAILOUT_LO_GAIN"},
        {NAK_ERROR_CANNOT_HIT_TARGET, "NAK_ERROR_CANNOT_HIT_TARGET"},
        {NAK_ERROR_TARGET_TOO_BIG, "NAK_ERROR_TARGET_TOO_BIG"},
        {NAK_BSL_FIRST, "NAK_BSL_FIRST"},
        {BSL_TRIP_UNIT_STILL_ACTIVE, "BSL_TRIP_UNIT_STILL_ACTIVE"},
        {BSL_ERROR_ERASING_FLASH, "BSL_ERROR_ERASING_FLASH"},
        {BSL_ERROR_BURNING_FLASH, "BSL_ERROR_BURNING_FLASH"},
        {BSL_ILLEGAL_FLASH_ADDRESS, "BSL_ILLEGAL_FLASH_ADDRESS"},
        {BSL_ILLEGAL_ASCII_HEX_DIGIT, "BSL_ILLEGAL_ASCII_HEX_DIGIT"},
        {BSL_UNEXPECTED_CHAR_IN_FILE, "BSL_UNEXPECTED_CHAR_IN_FILE"},
        {BSL_ONLY_ONE_DIGIT_OF_DATA, "BSL_ONLY_ONE_DIGIT_OF_DATA"},
        {BSL_BAD_CHECKSUM, "BSL_BAD_CHECKSUM"},
        {BSL_NO_APP_PROGRAM, "BSL_NO_APP_PROGRAM"},
        {BSL_OUT_OF_SEQUENCE, "BSL_OUT_OF_SEQUENCE"},
        {BSL_FATAL_ERROR, "BSL_FATAL_ERROR"},
        {BSL_CMD_NOT_SUPPORTED, "BSL_CMD_NOT_SUPPORTED"},
        {NAK_BSL_LAST, "NAK_BSL_LAST"},
        {NAK_SS_CT_RATING, "NAK_SS_CT_RATING"},
        {NAK_SS_CT_SECONDARY, "NAK_SS_CT_SECONDARY"},
        {NAK_SS_CT_NEUTRAL_SEC, "NAK_SS_CT_NEUTRAL_SEC"},
        {NAK_SS_FREQUENCY, "NAK_SS_FREQUENCY"},
        {NAK_SS_BKR_POS_CNT_TYPE, "NAK_SS_BKR_POS_CNT_TYPE"},
        {NAK_SS_POWER_DIRECTION, "NAK_SS_POWER_DIRECTION"},
        {NAK_SS_AUTO_POLARITY_CT, "NAK_SS_AUTO_POLARITY_CT"},
        {NAK_SS_ROGOWSKI_COIL_SENSITIVITY, "NAK_SS_ROGOWSKI_COIL_SENSITIVITY"},
        {NAK_SS_ROGOWSKI_COIL_SENSOR_TYPE, "NAK_SS_ROGOWSKI_COIL_SENSOR_TYPE"},
        {NAK_US_FV_SEG_ALARMS, "NAK_US_FV_SEG_ALARMS"},
        {NAK_DS_LT_ENABLED, "NAK_DS_LT_ENABLED"},
        {NAK_DS_LT_PICKUP, "NAK_DS_LT_PICKUP"},
        {NAK_DS_LT_DELAY, "NAK_DS_LT_DELAY"},
        {NAK_DS_LT_THERM_MEM, "NAK_DS_LT_THERM_MEM"},
        {NAK_DS_ST_ENABLED, "NAK_DS_ST_ENABLED"},
        {NAK_DS_ST_PICKUP, "NAK_DS_ST_PICKUP"},
        {NAK_DS_ST_DELAY, "NAK_DS_ST_DELAY"},
        {NAK_DS_ST_I2T, "NAK_DS_ST_I2T"},
        {NAK_DS_GF_TYPE, "NAK_DS_GF_TYPE"},
        {NAK_DS_GF_PICKUP, "NAK_DS_GF_PICKUP"},
        {NAK_DS_GF_DELAY, "NAK_DS_GF_DELAY"},
        {NAK_DS_GF_I2T, "NAK_DS_GF_I2T"},
        {NAK_DS_GF_QT_TYPE, "NAK_DS_GF_QT_TYPE"},
        {NAK_DS_GF_QT_PICKUP, "NAK_DS_GF_QT_PICKUP"},
        {NAK_DS_QT_INST_PICKUP, "NAK_DS_QT_INST_PICKUP"},
        {NAK_DS_INSTANT_ENABLED, "NAK_DS_INSTANT_ENABLED"},
        {NAK_DS_INSTANT_PICKUP, "NAK_DS_INSTANT_PICKUP"},
        {NAK_DS_NOL_ENABLED, "NAK_DS_NOL_ENABLED"},
        {NAK_DS_NOL_PICKUP, "NAK_DS_NOL_PICKUP"},
        {NAK_DS_NOL_DELAY, "NAK_DS_NOL_DELAY"},
        {NAK_DS_NOL_THERM_MEM, "NAK_DS_NOL_THERM_MEM"},
        {NAK_DS_UV_TRIP_ENABLED, "NAK_DS_UV_TRIP_ENABLED"},
        {NAK_DS_UV_PICKUP, "NAK_DS_UV_PICKUP"},
        {NAK_DS_UV_DELAY, "NAK_DS_UV_DELAY"},
        {NAK_DS_OV_TRIP_ENABLED, "NAK_DS_OV_TRIP_ENABLED"},
        {NAK_DS_OV_PICKUP, "NAK_DS_OV_PICKUP"},
        {NAK_DS_OV_DELAY, "NAK_DS_OV_DELAY"},
        {NAK_DS_PV_LOSS_ENABLED, "NAK_DS_PV_LOSS_ENABLED"},
        {NAK_DS_PV_LOSS_DELAY, "NAK_DS_PV_LOSS_DELAY"},
        {NAK_DS_NSOV_PICKUP_PERCENT, "NAK_DS_NSOV_PICKUP_PERCENT"},
        {NAK_DS_AVAILABLE_2040, "NAK_DS_AVAILABLE_2040"},
        {NAK_DS_AVAILABLE_2041, "NAK_DS_AVAILABLE_2041"},
        {NAK_DS_AVAILABLE_2042, "NAK_DS_AVAILABLE_2042"},
        {NAK_DS_SB_THRESHOLD, "NAK_DS_SB_THRESHOLD"},
        {NAK_DS_FORCED_TRIP_ENABLED, "NAK_DS_FORCED_TRIP_ENABLED"},
        {NAK_DS_REMOTE_ENABLED, "NAK_DS_REMOTE_ENABLED"},
        {NAK_DS_POWER_REVERSED, "NAK_DS_POWER_REVERSED"},
        {NAK_DS_ALARM_RELAY_TU, "NAK_DS_ALARM_RELAY_TU"},
        {NAK_DS_ALARM_RELAY_RIU1, "NAK_DS_ALARM_RELAY_RIU1"},
        {NAK_DS_ALARM_RELAY_RIU2, "NAK_DS_ALARM_RELAY_RIU2"},
        {NAK_US_LOAD_MONITOR_PICKUP_IC1, "NAK_US_LOAD_MONITOR_PICKUP_IC1"},
        {NAK_US_LOAD_MONITOR_PICKUP_IC2, "NAK_US_LOAD_MONITOR_PICKUP_IC2"},
        {NAK_DS_QT_SWITCH_ENABLED, "NAK_DS_QT_SWITCH_ENABLED"},
        {NAK_DS_UB_ENABLED, "NAK_DS_UB_ENABLED"},
        {NAK_DS_UB_PICKUP, "NAK_DS_UB_PICKUP"},
        {NAK_DS_UB_DELAY, "NAK_DS_UB_DELAY"},
        {NAK_DS_DATE_FORMAT, "NAK_DS_DATE_FORMAT"},
        {NAK_DS_SAFE_T_CLOSE, "NAK_DS_SAFE_T_CLOSE"},
        {NAK_NO_CHANGES, "NAK_NO_CHANGES"},
        {NAK_MODBUS_CONFIG_REPLY_DELAY, "NAK_MODBUS_CONFIG_REPLY_DELAY"},
        {NAK_MODBUS_CONFIG_PARITY, "NAK_MODBUS_CONFIG_PARITY"},
        {NAK_MODBUS_CONFIG_BAUDRATE, "NAK_MODBUS_CONFIG_BAUDRATE"},
        {NAK_MODBUS_CONFIG_ADDR, "NAK_MODBUS_CONFIG_ADDR"},
    };

    auto it = errorCodes.find(code);
    if (it != errorCodes.end())
    {
        return it->second;
    }
    else
    {
        return "<unknown NAK value>";
    }
}

std::string FloatToString(float value, int num_digits)
{
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(num_digits) << value;
    return oss.str();
}

std::string rightjustify(int width, const std::string &str)
{
    std::ostringstream result;
    result << std::right << std::setw(width) << str;

    return result.str();
}

// returns -1 if the string is not a valid float
float convertToFloat(const char *str)
{
    if (str == nullptr || std::strlen(str) == 0)
    {
        return std::numeric_limits<float>::quiet_NaN(); // Return NaN if input is null or empty
    }

    char *pEnd;
    float floatValue = std::strtof(str, &pEnd);

    // Check if conversion was successful
    if (pEnd == str || *pEnd != '\0')
    {
        // If pEnd == str, no characters were converted
        // If *pEnd != '\0', the string contains extra characters that are not part of the float
        return std::numeric_limits<float>::quiet_NaN(); // Return NaN if input is null or empty
    }

    return floatValue; // Return the converted float value
}

std::string SelectFileToSave(HWND hwnd)
{
    OPENFILENAME ofn; // common dialog box structure
    char szFile[260]; // buffer for file name

    // Initialize OPENFILENAME
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    // Display the Save As dialog box
    if (GetSaveFileName(&ofn) == TRUE)
    {
        return std::string(ofn.lpstrFile);
    }

    return std::string();
}

std::string SelectFileToOpen(HWND hwnd)
{
    // Initialize the OPENFILENAME structure
    OPENFILENAME ofn = {0};
    char szFile[260];

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0Text\0*.TXT\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = nullptr;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = nullptr;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box
    if (GetOpenFileName(&ofn) == TRUE)
    {
        return std::string(ofn.lpstrFile);
    }

    return std::string();
}
