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

#include "..\util\urc_protocol.hpp"

#pragma pack(1)

namespace ARDUINO

{
    constexpr int ADDR_AUTOCAL_ARDUINO = 40;

    enum ArduinoCommands : uint8_t
    {
        MSG_GET_LED = 6,
        MSG_RSP_LED = 7,
        MSG_SET_LED = 8,           // ACK returned
        MSG_START_TIMING_TEST = 9, // ACK returned
        MSG_GET_TIMING_TEST_RESULTS = 10,
        MSG_RSP_TIMING_TEST_RESULTS = 11,
        MSG_ABORT_TIMING_TEST = 12
    };

    enum ArduinoNAKs : uint8_t
    {
        NAK_ERROR_BUSY = 100
    };

    typedef struct _MsgGetLEDs
    {
        MsgHdr Hdr;
    } MsgGetLEDs;

    typedef struct _MsgRspLEDs
    {
        MsgHdr Hdr;
        uint8_t LED1;
    } MsgRspLEDs;

    typedef struct _MsgSetLEDs
    {
        MsgHdr Hdr;
        uint8_t LED1;
    } MsgSetLEDs;

    typedef struct _MsgStartTimingTest
    {
        MsgHdr Hdr;
    } MsgStartTimingTest;

    typedef struct _MsgGetTimingTestResults
    {
        MsgHdr Hdr;
    } MsgGetTimingTestResults;

    typedef struct TimingTestResults
    {
        uint32_t elapsedTime;
        uint32_t ResultsAreValid; // bool can be defined differently on different platforms
    } TimingTestResults;

    typedef struct _MsgRspTimingTestResults
    {
        MsgHdr Hdr;
        TimingTestResults timingTestResults;
    } MsgRspTimingTestResults;

    // prototypes

    bool Connect(HANDLE *hArduino, int port);

    bool SendConnectMessage(HANDLE hArduino);
    bool SendSetLEDs(HANDLE hArduino, bool beOn);
    bool SetLEDs(HANDLE hArduino, bool beOn);
    bool GetVersion(HANDLE hArduino, SoftVer *ArduinoFirmwareVer);
}