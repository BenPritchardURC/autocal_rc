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

typedef std::function<bool(SystemSettings4 *Settings, DeviceSettings4 *DevSettings4)> SetSettingsFuncPtr;

bool ConnectTripUnit(HANDLE *hTripUnit, int port, TripUnitType *tripUnitType);
bool SendSetUserSet4(HANDLE hTripUnit, SystemSettings4 *SysSettings, DeviceSettings4 *DevSettings);
bool SetupTripUnitForCalibration(HANDLE hTripUnit, bool Use50Hz);
bool SetSystemAndDeviceSettings(HANDLE hTripUnit, SetSettingsFuncPtr funcPtr);
bool GetDynamics(HANDLE hTripUnit, URCMessageUnion *rsp);
void DumpCalResults(MsgRspCalibr *msg);
void DumpCalResults_RC(MsgRspCalibrRC *msg);
bool SetupTripUnitFrequency(HANDLE hTripUnit, bool Use50Hz);
bool SendClearTripHistory(HANDLE hTripUnit);
bool GetTripHistory(HANDLE hTripUnit, URCMessageUnion *msg);
bool GetSerialNumbers(HANDLE hTripUnit, char *ld_serial_num, size_t size_buf_1, char *tu_serial_num, size_t size_buf_2);
bool SetSerialNumbers(HANDLE hTripUnit, char *ld_serial_num, char *tu_serial_num);
bool SendSetQTStatus(HANDLE hTripUnit, bool beOn);
bool CheckForExactlyOneTrip(HANDLE hTripUnit, int ExpectedTripType, bool &tripTypeIsAsExpected);
bool CheckForCorrectTrip(HANDLE hTripUnit, int ExpectedTripType, bool &tripTypeIsAsExpected);
bool GetCalibration(HANDLE hTripUnit);