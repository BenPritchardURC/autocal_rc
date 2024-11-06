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

#define DEFAULT_TIMEOUT_MS 1000

int timoutTimeMS = DEFAULT_TIMEOUT_MS;

uint16_t _SequenceNumber = 0; // local sequence number

extern bool logData;

void DumpHeader(MsgHdr *hdr)
{
	scr_printf("Type............%d", hdr->Type);
	scr_printf("Version.........%u", hdr->Version);
	scr_printf("Length..........%u", hdr->Length);
	scr_printf("Seq.............%u", hdr->Seq);
	scr_printf("Dst.............");
	PrintAddress(hdr->Dst);
	scr_printf("Src.............");
	PrintAddress(hdr->Src);
	scr_printf("ChkSum..........%u", hdr->ChkSum);
}

void PrintAddress(int addr)
{
	switch (addr)
	{
	case ADDR_TRIP_UNIT:
		scr_printf("ADDR_TRIP_UNIT (%d)", addr);
		break;
	case ADDR_DISP_LOCAL:
		scr_printf("ADDR_DISP_LOCAL (%d)", addr);
		break;
	case ADDR_INFOPRO_AC:
		scr_printf("ADDR_INFOPRO_AC (%d)", addr);
		break;
	case ADDR_CAL_APP:
		scr_printf("ADDR_CAL_APP (%d)", addr);
		break;

	default:
		scr_printf("Unknown Address (%d)", addr);
		break;
	}
}

void HexDumpBuffer(uint8_t *buf, int len)
{
	int i;

	// Process every byte in the data.
	for (i = 0; i < len; i++)
	{
		// Multiple of 16 means new line (with line offset).

		if ((i % 16) == 0)
		{
			if (i > 0)
				scr_printf("");
			// Output the offset.
			scr_printf("%04x ", i);
		}

		// Now the hex code for the specific character.
		scr_printf(" %02x", buf[i]);
	}

	scr_printf("");
}

void DumpRawMsgData(uint8_t *data_ptr, int data_length, bool AmSending)
{

	if (AmSending)
		scr_printf("\nRaw Bytes Sent (including header):");
	else
		scr_printf("Raw Bytes Received (including header):");

	HexDumpBuffer(data_ptr, data_length);
	scr_printf("");

	if (AmSending)
		scr_printf("Decoded Header Sent:");
	else
		scr_printf("\nDecoded Header Received:");

	DumpHeader((MsgHdr *)data_ptr);
}

uint16_t SequenceNumber()
{
	return _SequenceNumber++;
}

uint16_t CalcChecksum(const uint8_t *dataBuf, uint16_t length)
{
	uint16_t chkSum = 0;
	uint16_t i;
	for (i = 0; i < length; i++)
	{
		chkSum += ((i & 1) == 1 ? (uint8_t)~dataBuf[i] : dataBuf[i]);
	}

	return chkSum;
}

// sends a command with data length of 0
bool SendURCCommand(HANDLE hComm, uint8_t msgType, uint8_t dst, uint8_t src)
{
	URCMessageUnion msg = {0};

	msg.msgHdr.Type = msgType;
	msg.msgHdr.Version = PROTOCOL_VERSION;
	msg.msgHdr.Length = 0;
	msg.msgHdr.Seq = SequenceNumber();
	msg.msgHdr.Dst = dst;
	msg.msgHdr.Src = src;

	msg.msgHdr.ChkSum = CalcChecksum((uint8_t *)&msg.msgHdr, sizeof(msg.msgHdr));

	return WriteToCommPort(hComm, (uint8_t *)&msg.buf, sizeof(msg.msgHdr));
}

bool GetAckURCResponse(HANDLE hComm, URCMessageUnion *msg)
{
	return GetURCResponse(hComm, msg) && (msg->msgHdr.Type != MSG_NAK && msg->msgHdr.Type != MSG_NONE);
}

bool SendAck(HANDLE hComm, MsgHdr hdr)
{
	MsgACK a;

	a.Hdr.Type = MSG_ACK;
	a.Hdr.Version = 0;
	a.Hdr.Length = 2;
	a.Hdr.Seq = SequenceNumber();
	a.Hdr.Dst = hdr.Src;
	a.Hdr.Src = hdr.Dst;
	a.AckSeq = hdr.Seq;
	a.Hdr.ChkSum = CalcChecksum((const unsigned char *)&a, sizeof(a));

	return WriteToCommPort(hComm, (uint8_t *)&a, sizeof(a));
}

// read from serial port, puts message into msg
// 	-	data in msg is not valid if return value is false
//	-	TimeToReceiveCmd is not valid if return value is false
// this could be made generic...
// but right now, it uses Win32 COMM port functions like:
//		ReadFile()
//
bool GetURCResponse(HANDLE hComm, URCMessageUnion *msg)
{
	int TotalBytesRead = 0;
	bool Status = false;
	bool TimedOut = false;
	ULONGLONG StartTime = 0;
	uint8_t b;
	DWORD tmpByteCount;

	StartTime = GetTickCount64();

	// grab an entire header, or timeout
	do
	{
		// read one byte
		Status = ReadFile(hComm, &b, 1, &tmpByteCount, NULL);

		// can't read from port at all??
		if (!Status)
			return false;

		// put one byte into our buffer
		if (tmpByteCount == 1)
		{
			msg->buf[TotalBytesRead++] = b;
		}

		// did we time out yet??
		if (GetTickCount64() - StartTime > timoutTimeMS)
		{
			TimedOut = true;
			break;
		}

	} while (TotalBytesRead < sizeof(MsgHdr));

	if (TimedOut)
	{
		scr_printf("\n**** timed out waiting for msg header ****");
		return false;
	}

	// read as many more bytes as the header's length indicates there is
	do
	{
		// read one byte
		Status = ReadFile(hComm, &b, 1, &tmpByteCount, NULL);

		// can't read from port at all??
		if (!Status)
			return false;

		// put one byte into our buffer
		if (tmpByteCount == 1)
		{
			msg->buf[TotalBytesRead++] = b;
		}

		// did we time out yet??
		if (GetTickCount64() - StartTime > timoutTimeMS)
		{
			TimedOut = true;
			break;
		}

	} while (TotalBytesRead < msg->msgHdr.Length + sizeof(MsgHdr));

	if (TimedOut)
	{
		scr_printf("\n**** timed out waiting for rest of message ****");
		return false;
	}

	// verify checksum
	// grab their original checksum
	uint16_t origCheckSum = msg->msgHdr.ChkSum;

	// and blank it out, so that we can calculate it ourselves
	msg->msgHdr.ChkSum = 0;

	uint16_t tmpCheckSum = CalcChecksum((uint8_t *)msg, sizeof(MsgHdr) + msg->msgHdr.Length);

	// put the bytes back in there, just in case somebody looks later
	msg->msgHdr.ChkSum = origCheckSum;

	if (tmpCheckSum != origCheckSum)
	{

		// this line is very important!! if for some reason we timed out, or had an error
		// we have to make sure to get rid of the whole rest of the buffer
		PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR);

		scr_printf("**** checksum received: received checksum %d; calc'd checksum:%d ****", origCheckSum, tmpCheckSum);

		for (int index = 0; index < TotalBytesRead; ++index)
			scr_printf("%d ", msg->buf[index]);

		scr_printf("");
		return false;
	}

	// we are still going to return true if the response was a NAK...
	// however, we will make a note of it here just to help with debugging
	if (true)
	{
		if (false && MSG_NAK == msg->msgHdr.Type)
		{
			scr_printf("\n**** NAK seen to msg %d; error code = %d", msg->msgNAK.NAKSeq, msg->msgNAK.Error);
			scr_printf(NAKCodeToString(msg->msgNAK.Error).c_str());
		}
	}

	if (logData)
	{
		DumpRawMsgData((uint8_t *)msg, sizeof(MsgHdr) + msg->msgHdr.Length, false);
	}

	return true;
}

bool GetURCResponse_Long(HANDLE hComm, URCMessageUnion *msg, int longTimeOutMS)
{
	bool retval;

	timoutTimeMS = longTimeOutMS;
	retval = GetURCResponse(hComm, msg);
	timoutTimeMS = DEFAULT_TIMEOUT_MS;

	return retval;
}

bool VerifyMessageIsOK(URCMessageUnion *msg, int ExpectedMessageType, int ExpectedMessageLength)
{
	if (msg->msgHdr.Type != ExpectedMessageType)
	{
		PrintToScreen("Message type is not expected type; expected: " + std::to_string(ExpectedMessageType) +
					  " but got: " + std::to_string(msg->msgHdr.Type));

		return false;
	}

	if (msg->msgHdr.Length != ExpectedMessageLength)
	{
		PrintToScreen("Message length is not expected length; expected: " + std::to_string(ExpectedMessageLength) +
					  " but got: " + std::to_string(msg->msgHdr.Length));

		return false;
	}

	return true;
}

bool VerifyMessageIsOK(URCMessageUnion *msg, int ExpectedMessageType, int ExpectedMessageLength, int ExpectedDst, int ExpectedSrc)
{
	return msg->msgHdr.Type == ExpectedMessageType &&
		   msg->msgHdr.Length == ExpectedMessageLength &&
		   msg->msgHdr.Dst == ExpectedDst &&
		   msg->msgHdr.Src == ExpectedSrc;
}

bool MessageIsACK(URCMessageUnion *msg)
{
	return VerifyMessageIsOK(msg, MSG_ACK, 2);
}

UINT SendRemoteSoftkeyCommand(HANDLE hComm, uint8_t key)
{

	MsgRemoteSoftKeys4 msg = {0};

	msg.Hdr.Type = MSG_REMOTE_SOFTKEYS;
	msg.Hdr.Version = PROTOCOL_VERSION;
	msg.Hdr.Length = 2;
	msg.Hdr.Seq = SequenceNumber();
	msg.Hdr.Dst = ADDR_TRIP_UNIT;
	msg.Hdr.Src = ADDR_INFOPRO_AC;

	msg.SendRemoteScreen = false;
	msg.RecvRemoteSoftKeys = key;

	msg.Hdr.ChkSum = CalcChecksum((uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);

	return WriteToCommPort(hComm, (uint8_t *)&msg, sizeof(msg.Hdr) + msg.Hdr.Length);
}
