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

#include "autocal_rc.hpp"

bool WriteToCommPort(HANDLE hComm, uint8_t *data_ptr, int data_length);
bool SetLocalBaudRate(HANDLE hComm, DWORD baudrate);
bool InitCommPort(HANDLE *hComm, int PortNumber, int Baud);
bool CloseCommPort(HANDLE *hComm);
bool WriteToCommPort_Str(HANDLE hComm, const char *str);
bool WriteToCommPort_Str2(HANDLE hComm, const char *str);
bool GetRawResponse(HANDLE hComm, uint8_t *msg, int total_time_to_wait_ms, int bytesExpected);
bool GetExpectedString(HANDLE hComm, int total_time_to_wait_ms, const char *expectedResponseString);
