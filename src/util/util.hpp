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

#include "..\autocal_rc.hpp"
#include <string>
#include <chrono>

bool InRange(double value, double target, double tolerance);
std::string Tab(int num);
std::string ZeroOrOne(bool b);
std::string Dots(int numDots, std::string prefix);
float PercentDifference(float a, float b);
std::string BoolToPassFail(bool pass);
int NumSecondsElapsed(std::chrono::system_clock::time_point StartTime);
std::string NAKCodeToString(int code);
std::string FloatToString(float value, int num_digits);
std::string rightjustify(int width, const std::string &str);
float convertToFloat(const char *str);
std::string BoolToString(bool b);
std::string BoolToYesNo(bool pass);
std::string SelectFileToOpen(HWND hwnd);
std::string SelectFileToSave(HWND hwnd);
