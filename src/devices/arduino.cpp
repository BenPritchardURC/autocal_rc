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

#include "..\autocal_rc.hpp"
#include "arduino.hpp"

namespace ARDUINO
{

    bool Connect(HANDLE *hArduino, int port)
    {
        constexpr int MAX_TRIES = 3;
        bool retval = true;

        retval = InitCommPort(hArduino, port, 19200);

        if (retval)
            retval = SendConnectMessage(*hArduino);

        if (!retval)
            CloseCommPort(hArduino);

        return retval;
    }

    bool SendConnectMessage(HANDLE hArduino)
    {
        URCMessageUnion rsp = {0};
        bool retval;

        SendURCCommand(hArduino, MSG_CONNECT, ARDUINO::ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

        retval = GetURCResponse(hArduino, &rsp) &&
                 VerifyMessageIsOK(&rsp,
                                   MSG_ACK,
                                   sizeof(MsgACK) - sizeof(MsgHdr),
                                   ADDR_CAL_APP,
                                   ARDUINO::ADDR_AUTOCAL_ARDUINO);

        return retval;
    }

    bool SendSetLEDs(HANDLE hArduino, bool beOn)
    {
        MsgSetLEDs msg = {0};

        msg.Hdr.Type = ArduinoCommands::MSG_SET_LED;
        msg.Hdr.Version = PROTOCOL_VERSION;
        msg.Hdr.Length = sizeof(msg) - sizeof(msg.Hdr);
        msg.Hdr.Seq = SequenceNumber();
        msg.Hdr.Dst = ADDR_AUTOCAL_ARDUINO;
        msg.Hdr.Src = ADDR_CAL_APP;
        msg.Hdr.ChkSum = 0;

        msg.LED1 = beOn ? 1 : 0;
        msg.Hdr.ChkSum = CalcChecksum((uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);

        return WriteToCommPort(hArduino, (uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);
    }

    bool SetLEDs(HANDLE hArduino, bool beOn)
    {
        URCMessageUnion rsp = {0};
        bool retval;

        SendSetLEDs(hArduino, beOn);

        retval = GetURCResponse(hArduino, &rsp) &&
                 VerifyMessageIsOK(&rsp, MSG_ACK, sizeof(MsgACK) - sizeof(MsgHdr));

        return retval;
    }

    bool GetVersion(HANDLE hArduino, SoftVer *ArduinoFirmwareVer)
    {
        URCMessageUnion rsp = {0};
        bool retval;

        SendURCCommand(hArduino, MSG_GET_SW_VER, ADDR_AUTOCAL_ARDUINO, ADDR_CAL_APP);

        retval = GetURCResponse(hArduino, &rsp) &&
                 VerifyMessageIsOK(&rsp, MSG_RSP_SW_VER, sizeof(MsgRspSwVer) - sizeof(MsgHdr));

        if (retval)
            *ArduinoFirmwareVer = rsp.msgRspSwVer.Version;
        else
            *ArduinoFirmwareVer = {0};

        return retval;
    }

    bool GetStatus(HANDLE hArduino, ArduinoStatus *status)
    {
        AurduinoURCMessageUnion rsp = {0};
        bool retval;

        SendURCCommand(hArduino, MSG_GET_SW_VER, MSG_ARDUINO_GET_STATUS, ADDR_CAL_APP);

        retval = GetURCResponse(hArduino, (URCMessageUnion *)&rsp) &&
                 VerifyMessageIsOK((URCMessageUnion *)&rsp, MSG_ARDUINO_RSP_STATUS, sizeof(MsgArduinoGetStatus) - sizeof(MsgHdr));

        if (retval)
            *status = rsp.msgArduinoGetStatus.status;
        else
            *status = {0};

        return retval;
    }

}