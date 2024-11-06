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

#include <unordered_map>
#include <mutex>

#include "..\autocal_rc.hpp"

// Global unordered_map to store HANDLE to mutex mapping
// we do it like this because we want a mutex in GetRawResponse() that is specific to
// each HANDLE
// reason: we want multiple threads to be able to read from DIFFERENT comm ports at same time...
// while being prevented from reading from the _same_ comm port at the same time!
std::unordered_map<HANDLE, std::mutex> hCommMutexMap;

// Function to get the mutex for a given hComm HANDLE
std::mutex &getMutexForHandle(HANDLE hComm)
{
	static std::mutex mapMutex; // Mutex to protect the map itself
	std::lock_guard<std::mutex> lock(mapMutex);
	return hCommMutexMap[hComm];
}

bool GetExpectedString(HANDLE hComm, int total_time_to_wait_ms, const char *expectedResponseString)
{
	URCMessageUnion rsp = {0};
	bool retval;

	retval = GetRawResponse(hComm, rsp.buf, total_time_to_wait_ms, strlen(expectedResponseString));

	if (retval)
	{
		retval = (strncmp((const char *)rsp.buf, expectedResponseString, strlen(expectedResponseString)) == 0);
	}

	return retval;
}

// read from serial port, puts message into msg
// this is not for working with URC messages
// read anything that is available, until we get as many bytes as we expect
// or until total_time_to_wait_ms has elapsed
bool GetRawResponse(HANDLE hComm, uint8_t *msg, int total_time_to_wait_ms, int bytesExpected)
{
	int TotalBytesRead = 0;
	bool Status = false;
	bool TimedOut = false;
	ULONGLONG StartTime = 0;
	uint8_t b;
	DWORD tmpByteCount;

	// this will block if another thread is currently writing to the port identified by hComm
	std::mutex &mtx = getMutexForHandle(hComm);
	std::lock_guard<std::mutex> lock(mtx);

	StartTime = GetTickCount64();

	do
	{
		// read one byte
		Status = ReadFile(hComm, &b, 1, &tmpByteCount, NULL);

		// got some data?
		if (Status)
		{
			// put one byte into our buffer
			if (tmpByteCount == 1)
			{
				msg[TotalBytesRead++] = b;
			}
		}

		// did we timeout
		if (GetTickCount64() - StartTime > total_time_to_wait_ms)
			break;

		// if we know how many bytes we are expecting, then we can stop when we get that many
		if (bytesExpected != -1)
		{
			if (TotalBytesRead >= bytesExpected)
				break;
		}

	} while (true);

	return (TotalBytesRead > 0);
}

bool WriteToCommPort_Str(HANDLE hComm, const char *str)
{
	bool retval;
	retval = WriteToCommPort(hComm, (uint8_t *)str, (int)strlen(str));
	if (retval)
	{
		WriteToCommPort(hComm, (uint8_t *)"\r", 1);
	}
	return retval;
}

bool WriteToCommPort_Str2(HANDLE hComm, const char *str)
{
	return WriteToCommPort(hComm, (uint8_t *)str, (int)strlen(str));
}

bool WriteToCommPort(HANDLE hComm, uint8_t *data_ptr, int data_length)
{
	DWORD actual_length;

	// this will block if another thread is currently writing to the port identified by hComm
	std::mutex &mtx = getMutexForHandle(hComm);
	std::lock_guard<std::mutex> lock(mtx);

	// because we are dealing with a command/response type protocol, whenever we go
	// to send a command, we need to make sure that the buffer is cleared out to begin
	// with
	PurgeComm(hComm, PURGE_RXCLEAR | PURGE_TXCLEAR);

	bool Status = WriteFile(
		hComm,	  // Handle to the Serialport
		data_ptr, // Data to be written to the port
		data_length,
		&actual_length, // No of bytes written to the port
		NULL);

	if (!Status)
		scr_printf("\nWriteFile() failed with error %d.", GetLastError());

	if (data_length != actual_length)
		scr_printf("\nWriteFile() failed to write all bytes.");

	return Status && (data_length == actual_length);
}

bool SetLocalBaudRate(HANDLE hComm, DWORD baudrate)
{
	bool Status;
	DCB dcb = {0};

	// Setting the Parameters for the SerialPort
	// but first grab the current settings
	dcb.DCBlength = sizeof(dcb);

	Status = GetCommState(hComm, &dcb);
	if (!Status)
		return false;

	dcb.BaudRate = baudrate;
	dcb.ByteSize = 8;		   // ByteSize = 8
	dcb.StopBits = ONESTOPBIT; // StopBits = 1
	dcb.Parity = NOPARITY;	   // Parity = None

	Status = SetCommState(hComm, &dcb);

	return Status;
}

bool InitCommPort(HANDLE *hComm, int PortNumber, int Baud)
{
	uint8_t SerialBuffer[2048] = {0};
	COMMTIMEOUTS timeouts = {0};
	bool Status;
	char commName[50] = {0};

	// special syntax needed for comm ports > 10, but syntax also works for comm ports < 10
	sprintf_s(commName, sizeof(commName), "\\\\.\\COM%d", PortNumber);

	// Open the serial com port
	*hComm = CreateFileA(commName,
						 GENERIC_READ | GENERIC_WRITE, // Read/Write Access
						 0,							   // No Sharing, ports cant be shared
						 NULL,						   // No Security
						 OPEN_EXISTING,				   // Open existing port only
						 0,
						 NULL); // Null for Comm Devices

	if (*hComm == INVALID_HANDLE_VALUE)
		return false;

	Status = SetLocalBaudRate(*hComm, Baud);
	if (!Status)
		return false;

	// setup the timeouts for the SerialPort
	// https://docs.microsoft.com/en-us/windows/win32/api/winbase/ns-winbase-commtimeouts

	timeouts.ReadIntervalTimeout = MAXDWORD;
	timeouts.ReadTotalTimeoutMultiplier = 0;
	timeouts.ReadTotalTimeoutConstant = 0;

	timeouts.WriteTotalTimeoutConstant = 0;
	timeouts.WriteTotalTimeoutMultiplier = 0;

	if (!SetCommTimeouts(*hComm, &timeouts))
		return false;

	Status = PurgeComm(*hComm, PURGE_RXCLEAR | PURGE_TXCLEAR); // clear the buffers before we start
	if (!Status)
		return false;

	return true;
}

bool CloseCommPort(HANDLE *hComm)
{
	if (hComm == INVALID_HANDLE_VALUE)
		return false;
	if (!CloseHandle(*hComm))
		return false;

	*hComm = INVALID_HANDLE_VALUE;
	return true;
}
