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

#include <windows.h>
#include <shellapi.h>

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>
#include <limits>

#include "..\calibrate_rc.hpp"
#include "urc_protocol.hpp"

struct DisplayValues
{
    std::string s1;
    std::string s2;
    std::string s3;
    std::string s4;
    std::string s5;
};

DisplayValues DisplayCurrentVoltageVDM(const Measurements4 &readings);