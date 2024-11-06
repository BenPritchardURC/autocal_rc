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

#include <windows.h>
#include <shellapi.h>

#include <iostream>
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <thread>
#include <limits>
#include <functional>

#include "calibrate_rc.hpp"
#include "devices\keithley.hpp"
#include "trip4.hpp"
#include "util\urc_protocol.hpp"
#include "util\win32_api_comm.hpp"
#include "util\util.hpp"
#include "devices\ftdi.hpp"
#include "devices\rigol_DG1000z.hpp"
#include "devices\bk_precision_9801.hpp"
#include "util\db.hpp"
#include "tests\production_test.hpp"

// function prototypes
void ExitWithError(const std::string &message);
void PrintToScreen(const std::string &message);
void scr_printf(const char *format, ...);
static void menu_ID_ACPRO2_PRINT_VERSION();
static void menu_ID_ARDUINO_PRINTVERSION();