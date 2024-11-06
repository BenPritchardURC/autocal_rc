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

namespace KEITHLEY
{
    bool Connect(HANDLE *hKeithley, int port);
    void Initialize(HANDLE hKeithley);
    double GetVoltage(HANDLE hKeithley);
    double GetAverageVoltage(HANDLE hKeithley, int samples);

    void RANGE0(HANDLE hKeithley);
    void RANGE1(HANDLE hKeithley);
    void RANGE10(HANDLE hKeithley);

    void RANGE_AUTO(HANDLE hKeithley);

    double GetVoltageForAutoRange(HANDLE hKeithley);

    bool VoltageOnKeithleyIsStable(HANDLE hKeithley);
    bool VoltageOnKeithleyIsStable(HANDLE hKeithley, bool quietMode);

}

namespace ASYNC_KEITHLEY
{
    void StartMonitoringKeithley(HANDLE hKeithley);
    void StopMonitoringKeithley();
    bool KeithleyIsStable();
}
