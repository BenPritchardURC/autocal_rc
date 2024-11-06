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

#include "ld.hpp"

#include <sstream>
#include <iomanip>

DisplayValues DisplayCurrentVoltageVDM(const Measurements4 &readings)
{
    int LowValue = 10; // 10A is the lowest value to display.
    DisplayValues dv;
    std::stringstream S1, S2, S3, S4, S5;

    S1 << "!A: " << std::setw(5) << (readings.Ia <= LowValue ? "LOW" : std::to_string(readings.Ia)) << " A   " << std::setw(3) << (readings.Vab < 10 || readings.Vab > 1000 ? "LOW" : std::to_string(readings.Vab)) << " Vab";
    S2 << "!B: " << std::setw(5) << (readings.Ib <= LowValue ? "LOW" : std::to_string(readings.Ib)) << " A   " << std::setw(3) << (readings.Vbc < 10 || readings.Vbc > 1000 ? "LOW" : std::to_string(readings.Vbc)) << " Vbc";
    S3 << "!C: " << std::setw(5) << (readings.Ic <= LowValue ? "LOW" : std::to_string(readings.Ic)) << " A   " << std::setw(3) << (readings.Vca < 10 || readings.Vca > 1000 ? "LOW" : std::to_string(readings.Vca)) << " Vca";
    S4 << " N: " << std::setw(5) << (readings.In == 0 || readings.In <= LowValue ? " " : std::to_string(readings.In)) << " A";
    S5 << "GF: " << std::setw(5) << (readings.Igf == 0 ? " " : std::to_string(readings.Igf)) << " A  F:" << std::setw(4) << (readings.FrequencyX100 == 0 ? " " : std::to_string(readings.FrequencyX100 / 100.0)) << "Hz";

    dv.s1 = S1.str();
    dv.s2 = S2.str();
    dv.s3 = S3.str();
    dv.s4 = S4.str();
    dv.s5 = S5.str();

    return dv;
}