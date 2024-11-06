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

#include "..\autocal_rc.hpp"
#include "..\misc_defines.hpp"

enum TripUnitType
{
	NONE = 0,
	AC_PRO2_V4,
	AC_PRO2_GFO,
	AC_PRO2_NW,
	AC_PRO2_RC,
};

typedef int8_t bool8;

#define PROTOCOL_VERSION 0

#define _QT_SWITCH_STATE_ON 0x8014
#define _QT_SWITCH_STATE_OFF 0x1290

// different addresses used
#define ADDR_TRIP_UNIT 1
#define ADDR_DISP_LOCAL 2
#define ADDR_TRIP_UNIT_GFO 41
#define ADDR_DISP_LOCAL_GFO 42
#define ADDR_AC_PRO_NW 15
#define ADDR_AC_PRO_RC 13
#define ADDR_URC_PI 100

#define ADDR_NONE 0
#define ADDR_INFOPRO_AC 4
#define ADDR_CAL_APP 5
#define ADDR_QTD2 22

// commands we send / receive; only commands we specifically use in autocal_lite are listed
#define MSG_NONE 0
#define MSG_ACK 1
#define MSG_NAK 2
#define MSG_CONNECT 3
#define MSG_GET_SW_VER 4
#define MSG_RSP_SW_VER 5
#define MSG_GET_CALIBRATION 41
#define MSG_RSP_CALIBRATION 42
#define MSG_RSP_CALIBRATION_RC 142
#define MSG_EXE_CALIBRATE_AD 51
#define MSG_RSP_CALIBRATE_AD 52
#define MSG_GET_STATUS 53
#define MSG_RSP_STATUS_2 101
#define MSG_GET_DEV_SETTINGS 16
#define MSG_RSP_DEV_SETTINGS_4 117
#define MSG_GET_PERSONALITY_4 128
#define MSG_RSP_PERSONALITY_4 129
#define MSG_SET_PERSONALITY_4 130
#define MSG_GET_SYS_SETTINGS 44
#define MSG_RSP_SYS_SETTINGS_4 120
#define MSG_SET_USR_SETTINGS_4 119
#define MSG_RSP_SYS_SETTINGS_4 120
#define MSG_DECOMMISSION 100
#define MSG_GET_DYNAMICS 89
#define MSG_RSP_DYNAMICS_4 91
#define MSG_QTD2_SETRELAY1 132
#define MSG_QTD2_ENABLE_5V 133
#define MSG_QTD2_GET_STATUS 190
#define MSG_QTD2_RSP_STATUS 191
#define MSG_GET_TRIP_HIST 36
#define MSG_RSP_TRIP_HIST_4 116
#define MSG_CLR_TRIP_HIST 38
#define MSG_GET_SER_NUM 6
#define MSG_RSP_SER_NUM 7
#define MSG_SET_SER_NUM_4 131
#define MSG_GET_HW_REV 14
#define MSG_RSP_HW_REV 15
#define MSG_FORMAT_FRAM 40
#define MSG_REMOTE_SCREEN 93
#define MSG_REMOTE_SOFTKEYS 94
#define MSG_GET_LEDS 26
#define MSG_RSP_LEDS 27
#define MSG_GET_BAUD_RATES 86
#define MSG_RSP_BAUD_RATES 87
#define MSG_SET_BAUD_RATE 88
#define MSG_SET_QT_SWITCH_4 132

#define _TRIP_TYPE_UNKNOWN 0		 // Unidentified
#define _TRIP_TYPE_LT 1				 // Long Time
#define _TRIP_TYPE_TOC 1			 // TOC
#define _TRIP_TYPE_ST 2				 // Short Time
#define _TRIP_TYPE_GF 3				 // Ground Fault
#define _TRIP_TYPE_GF_QT 4			 // Ground Fault Quick Trip
#define _TRIP_TYPE_INST_ANALOG 5	 // trip type I
#define _TRIP_TYPE_INST_DIGITAL 6	 // trip type I
#define _TRIP_TYPE_QT_I_ANALOG 7	 // trip type I-QT
#define _TRIP_TYPE_QT_I_DIGITAL 8	 // trip type I-QT
#define _TRIP_TYPE_I_OVRD 9			 // Inst Override
#define _TRIP_TYPE_I_CLOSE 10		 // Inst on Close
#define _TRIP_TYPE_REMOTE 11		 // Remote
#define _TRIP_TYPE_ACTUATOR_TEST 12	 // Actuator Test
#define _TRIP_TYPE_NOT_USED_13 13	 // NOL
#define _TRIP_TYPE_UI 14			 // Under Current
#define _TRIP_TYPE_UV 15			 // Under Voltage
#define _TRIP_TYPE_OV 16			 // Over  Voltage
#define _TRIP_TYPE_V_PH_LOSS_REV 17	 // voltage phase-Loss/Reverse
#define _TRIP_TYPE_UNBALANCED 18	 // Unbalanced
#define _TRIP_TYPE_UF 19			 // Under Frequency
#define _TRIP_TYPE_OF 20			 // Over  Frequency
#define _TRIP_TYPE_REVERSED_POWER 21 // Reversed Power
#define _TRIP_TYPE_FORCED 22		 // Forced
#define _TRIP_TYPE_SAFETY 23		 // Safe-T-Trip
#define _TRIP_TYPE_NOT_USED_24 24	 // Available
#define _TRIP_TYPE_NOT_USED_25 25	 // Available
#define _TRIP_TYPE_GF_TEST 26		 // Ground Fault Test
#define _TRIP_TYPE_NP 27			 // Neutral Protection
#define _TRIP_TYPE_NOT_USED_28 28	 // Available
#define _TRIP_TYPE_NOT_USED_29 29	 // Available
#define _TRIP_TYPE_NOT_USED_30 30	 // Available
#define _TRIP_TYPE_NOT_USED_31 31	 // Available
#define _TRIP_TYPE_MAX 32			 // Maximum number of trip types.

// for use with rogowski coil
// Select HI-Gain value to be calibrated.
#define _CALIBRATION_REQUEST_HI_GAIN_MASK 0x0700
#define _CALIBRATION_REQUEST_HI_GAIN_2_0 0x0000
#define _CALIBRATION_REQUEST_HI_GAIN_0_5 0x0500
#define _CALIBRATION_REQUEST_HI_GAIN_1_0 0x0600
#define _CALIBRATION_REQUEST_HI_GAIN_1_5 0x0700

// Number of samples taken per electrical cycle.
#define _SAMPLES_PER_CYCLE 63

#define MSG_GET_RESPONSE_4 97		  // V4.01+
#define MSG_SET_TESTSET_AMPS_4 146	  // V4.01+
#define MSG_SET_TESTSET_VOLTS_4 147	  // V4.01+
#define MSG_EXE_TESTSET_SETUP_4 148	  // V4.01+
#define MSG_RSP_TESTSET_RESULTS_4 149 // V4.01+

// data structures; only those specifically used by autocal_lite are listed

#pragma pack(1)

typedef struct _MsgHdr
{
	uint8_t Type;	 // Message code                                //  1
	uint8_t Version; // Protocol version                            //  1
	uint16_t Length; // Message Length                              //  2
	uint16_t Seq;	 // Message sequence number                     //  2
	uint8_t Dst;	 // Destination Address                         //  1
	uint8_t Src;	 // Source Address                              //  1
	uint16_t ChkSum; // Message Checksum                            //  2
} MsgHdr;

typedef struct MsgACK
{
	MsgHdr Hdr;
	uint16_t AckSeq; // sequence number that is being ack'd
} MsgACK;

typedef struct _MsgNAK
{
	MsgHdr Hdr;
	uint16_t NAKSeq; // Sequence # of the msg to NAK
	uint16_t Error;	 // Error code
} MsgNAK;

// Time/Date stamp.
typedef struct _TimeDate4
{
	uint8_t Year;	   // binary 0-99                                     //  1
	uint8_t Month;	   // binary 1-12                                     //  1
	uint8_t Day;	   // binary 1-31                                     //  1
	uint8_t Hour;	   // binary 0-23                                     //  1
	uint8_t Minute;	   // binary 0-59                                     //  1
	uint8_t Second;	   // binary 0-59                                     //  1
	uint16_t MilliSec; // Binary 0-999                                    //  2
} TimeDate4;

typedef struct _MsgSetTimeDate
{
	MsgHdr Hdr;
	TimeDate4 Time;
} MsgSetTimeDate;

typedef struct _MsgRspTimeDate
{
	MsgHdr Hdr;
	TimeDate4 Time;
} MsgRspTimeDate;

typedef struct _SoftVer
{
	uint8_t Major;
	uint8_t Minor;
	uint8_t Build;
	uint8_t Revision;
} SoftVer;

typedef struct _SystemSettings4
{
	uint32_t Spare00000000;		 // Future                                //  4
	uint8_t SpareFF;			 // Future                                //  1
	uint8_t BreakerL;			 // 'L' = Breaker nameplate includes 'L'. //  1
	uint16_t CTNeutralRating;	 // CT Rating of Neutral.                 //  2
	uint16_t CTRating;			 // CT Rating                             //  2
	uint16_t CTSecondary;		 // Secondary CT Rating * 100             //  2
	uint16_t CTNeutralSecondary; // Neutral Secondary CT Rating * 100     //  2
	uint16_t RatioPT;			 // 100V-600V in 1V steps                 //  2
	uint16_t CoilSensitivity;	 // Amp / Volt                            //  2
	uint16_t SensorType;		 // 0 to ....                             //  2
	uint8_t Frequency;			 // 50/60 Hz                              //  1
	uint8_t BkrPosCntType;		 // Breaker Position Contact Type         //  1
	uint8_t PowerReversed;		 // 0= Normal 1= Reversed                 //  1
	uint8_t AutoPolarityCT;		 // 0= Alarm only  1= Correction  2= OFF  //  1
	uint8_t TripUnitMode;		 // 0= All Features Accessible + TBD      //  1
	uint8_t TypePT;				 // 0= L-L    1= L-N                      //  1
	bool8 HPCdevice;			 // GE Offering for Quick-Trip.           //  1
	uint8_t ChangeSource;		 // Message Source (4.1) of Who Changed.  //  1
	TimeDate4 LastChanged;		 // Settings Last Changed Time Stamp      //  8
} SystemSettings4;				 //---
// 36

// Device Settings for Trip Unit version 4.00.000+
// Aligned on 32-bit boundaries for MSP432 structures.
typedef struct _DeviceSettings4
{
	// Long Time:                                                           //   8
	uint32_t LTPickup;	// Long Time Pickup * 10
	uint16_t LTDelay;	// Long Time Delay  * 10
	uint8_t LTThermMem; // Long Time Thermal Memory
	bool8 LTEnabled;	// Long Time Pickup Enabled

	// Short Time:                                                        //   8
	uint32_t STPickup; // Short Time Pickup
	uint16_t STDelay;  // Short Time Delay * 100
	uint8_t STI2T;	   // Short Time I2T Ramp
	bool8 STEnabled;   // Short Time Pickup Enabled

	// Ground Fault:                                                      //  12
	uint32_t GFPickup;	 // Ground Fault Pickup
	uint16_t GFDelay;	 // Ground Fault Delay * 100
	uint8_t GFI2T;		 // Ground Fault 1=I2T or 2=I5T Ramp
	uint8_t GFType;		 // Ground Fault Pickup Type
	uint8_t GFI2TAmps;	 // GF I2T transition to "definite time".
	uint8_t GFSpare8[3]; // Not Used

	// QT Instantaneous:                                                  //   4
	uint32_t QTInstPickup; // Quick Trip Instant Pickup

	// Instantaneous:                                                     //   8
	uint32_t InstantPickup;	  // Instantaneous Pickup
	bool8 InstantEnabled;	  // true= Instantaneous Pickup Enabled:
	uint8_t InstantSpare8[3]; // Not Used

	// GF Quick Trip:                                                     //   6
	uint32_t GFQTPickup; // Quick Trip Grnd Fault Pickup
	uint8_t GFQTType;	 // Quick Trip GF Pickup Enabled
	uint8_t GFQTSpare8;	 // Future

	// Phase Unbalanced:                                                  //   6
	uint16_t UBPickup; // 20-50%
	uint16_t UBDelay;  //  1-60 Seconds
	uint8_t UBEnabled; // 0=OFF, 1=ALARM, 2=TRIP
	uint8_t UBSpare8;  // Not Used

	// L-L Over Voltage:                                                  //   8
	uint16_t OVTripPickupLL;  // L-L Over Voltage Pickup for trip
	uint16_t OVAlarmPickupLL; // L-L Over Voltage Pickup for alarm
	uint8_t OVTripDelayLL;	  // L-L Over Voltage Delay for trip (seconds)
	uint8_t OVAlarmDelayLL;	  // L-L Over Voltage Delay for alarm (seconds)
	bool8 OVTripEnabledLL;	  // true= L-L Over Voltage Trip Enabled:
	uint8_t OVSpare8;		  // Not Used

	// L-L Under Voltage:                                                 //   8
	uint16_t UVTripPickupLL;  // L-L Under voltage pickup for trip
	uint16_t UVAlarmPickupLL; // L-L Under voltage pickup for alarm
	uint8_t UVTripDelayLL;	  // L-L Under Voltage Delay for trip (seconds)
	uint8_t UVAlarmDelayLL;	  // L-L Under Voltage Delay for alarm(seconds)
	bool8 UVTripEnabledLL;	  // true= L-L Under Voltage Trip Enabled
	uint8_t UVSpare8;		  // Not Used

	// Over Frequency:                                                    //   6
	uint16_t OFValueTrip;  // Over Frequency Value to Cause Trip.   x10
	uint16_t OFValueAlarm; // Over Frequency Value to Cause Alarm.  x10
	uint8_t OFEnabled;	   // 0=OFF, 1=ALARM, 2=TRIP
	uint8_t OFSpare8;	   // Not Used

	// Under Frequency:                                                   //   6
	uint16_t UFValueTrip;  // Under Frequency Value to Cause Trip.  x10
	uint16_t UFValueAlarm; // Under Frequency Value to Cause Alarm. x10
	uint8_t UFEnabled;	   // 0=OFF, 1=ALARM, 2=TRIP
	uint8_t UFSpare8;	   // Not Used

	// Phase Loss/Reverse:                                                //   4
	uint8_t PVLossEnabled;	   // Phase Voltage Loss Trip Enabled
	uint8_t PVLossDelay;	   // Phase Voltage Loss Delay
	uint8_t SystemRotation;	   // 1=ABC 0=CBA
	uint8_t NSOVPickupPercent; // 10-30 percent (default is 20%).

	// Reversed Power.                                                    //   4
	uint16_t RPPickup; // KW
	uint8_t RPDelay;   // Seconds
	uint8_t RPEnabled; // 0=OFF, 1=ALARM, 2=TRIP

	// Alarms:                                                            //   8
	uint16_t AlarmRelayTU;	 // Alarm Relay #1 Trip Unit
	uint16_t AlarmRelayHI;	 // Future upper 32-bits
	uint16_t AlarmRelayRIU1; // Alarm Relay #1 RIU
	uint16_t AlarmRelayRIU2; // Alarm Relay #2 RIU

	// Sluggish Breaker Threshold (20-100 ms):                            //   1
	uint8_t SBThreshold;

	// International Format of Date (0= MDY, 1= DMY, 2= YMD):             //   1
	uint8_t DateFormat;

	// Language (0= English, 1= French, 2= Spanish):                      //   1
	uint8_t LanguageID;

	// Neutral Protection (0= OFF, 50% - 200% of LT Pickup):              //   1
	uint8_t NeutralProtectionPercent;

	// Zone Block Bit Pattern:                                            //   2
	uint8_t ZoneBlockBits[2];

	// Programmable Relay:                                                //   2
	uint8_t ProgrammableRelay[2];

	// true= USB command causes Remotr Trip.                              //   1
	bool8 USBTripEnabled;

	// Phase CT Threshold for Peak-to-RMS ( MaxRMS x CT Rating )          //   1
	uint8_t ThresholdPhaseCT;

	// Neutral CT Threshold for Peak-to-RMS ( MaxRMS x CT Rating )        //   1
	uint8_t ThresholdNeutralCT;

	// Select _NONE or _NT or _NW.                                        //   1
	uint8_t DINF;

	// true= Forced Trip from MODBUS Enabled:                             //   1
	bool8 ModbusForcedTripEnabled;

	// true= Change User Settings from MODBUS Enabled.                    //   1
	bool8 ModbusChangeUserSettingsEnabled;

	// true= Change soft QT switch from MODBUS Enabled.                   //   1
	bool8 ModbusSoftQuickTripSwitchEnabled;

	// Spare bytes                                                        //  12
	uint8_t Spare8[12]; // Not Used

	// Changed Device                                                     //   9
	uint8_t ChangeSource;  // Message Source (4.1) of Who Changed.
	TimeDate4 LastChanged; // Settings Last Changed Time Stamp.
} DeviceSettings4;

// MSG_RSP_DEV_SETTINGS_4
typedef struct _MsgRspDevSet4
{
	MsgHdr Hdr;				  //  10
	uint16_t Filler16;		  //   2
	DeviceSettings4 Settings; // 132
} MsgRspDevSet4;

typedef struct _MsgRspSoftVer
{
	MsgHdr Hdr;
	SoftVer ver;
} MsgRspSoftVer;

typedef struct _StatusTU2
{
	uint32_t Status;
	uint32_t Mask;
} StatusTU2;

typedef struct _MsgRspStatus2
{
	MsgHdr Hdr;
	uint32_t Status;
	uint32_t Mask;
} MsgRspStatus2;

// This is calibration data used by isr_dma to calibrate A/D inputs.
// There are one of these tables for each frequency.
typedef struct _CalibrationDataAtFrequency
{
	uint16_t CalibratedChannels;			  // Remembers calibrated channels;
	uint16_t Offset[_NUM_A2D_INPUTS_RAW_CAL]; // Raw A/D offset with A2D_OFFSET.
	uint16_t SwGain[_NUM_A2D_INPUTS_RAW_CAL]; // Software gain.
} CalibrationDataAtFrequency;

// All of the A/D offsets and software gains for this AC-PRO-II.
// This data is calibrated by the factory and stored in FLASH INFO-C.
typedef struct _CalibrationData
{
	uint32_t Calibrated;
	CalibrationDataAtFrequency Hz60; // 60 Hz calibration data
	CalibrationDataAtFrequency Hz50; // 50 Hz calibration data.
} CalibrationData;

// The active calibration data based on selected frequency.
typedef struct _CalibrationDataRAM
{
	uint32_t Calibrated;
	uint16_t Offset[_NUM_A2D_INPUTS_CALIBRATED]; // Raw A/D offset with A2D_OFFSET
	uint16_t SwGain[_NUM_A2D_INPUTS_CALIBRATED]; // Software gain.
	uint16_t CRC;								 // Must be last.
} CalibrationDataRAM;

typedef struct _CalibrationRequest
{
	// Calibration request (bit pattern)
	uint16_t Commands;
	// Selects RMS Amps calibrated value should match.
	uint16_t RealRMS[_NUM_A2D_INPUTS_RAW_CAL];
	// Placeholder to keep the structure the same size.
	uint16_t Spare;
} CalibrationRequest;

typedef struct _CalibrationResults
{
	uint16_t CalibratedChannels;			  // Remembers calibrated channels.
	uint16_t Offset[_NUM_A2D_INPUTS_RAW_CAL]; // Raw A/D offset with A2D_OFFSET.
	uint16_t SwGain[_NUM_A2D_INPUTS_RAW_CAL]; // Software gain.
	uint16_t RmsVal[_NUM_A2D_INPUTS_RAW_CAL]; // Calculated RMS amps.
} CalibrationResults;

typedef struct _MsgExeCalibrateAD
{
	MsgHdr Hdr;
	CalibrationRequest CalibrateRequest;
} MsgExeCalibrateAD;

typedef struct _MsgRspCalibrateAD
{
	MsgHdr Hdr;
	CalibrationResults CalibrateResults;
} MsgRspCalibrateAD;

typedef struct _MsgGetCalibr
{
	MsgHdr Hdr;
} MsgGetCalibr;

typedef struct _MsgRspCalibr
{
	MsgHdr Hdr;
	CalibrationData CalibrationDataInfoC;
} MsgRspCalibr;

// MSG_RSP_SYS_SETTINGS_4
typedef struct _MsgRspSysSet4
{
	MsgHdr Hdr;				  // 10
	uint16_t Filler16;		  //  2
	SystemSettings4 Settings; // 36
} MsgRspSysSet4;

// MSG_SET_USR_SETTINGS_4
typedef struct _MsgSetUserSet4
{
	MsgHdr Hdr;					 //  10
	uint16_t Filler16;			 //   2
	SystemSettings4 SysSettings; //  36
	DeviceSettings4 DevSettings; // 132
} MsgSetUserSet4;

typedef struct _Personality4
{
	// New personality for version 4.02
	uint32_t structID; // Make sure this is a valid Personality4.   //  4
	// Each bit is a boolean option (see above).
	uint32_t options32;	 //  4
	uint16_t options16;	 //  2
	uint8_t optionsTrip; //  1
	// N x CT Rating (x10 so 120= 12.0)(0= 12).
	uint8_t maxRMS; //  1
	// CT limits.
	uint16_t maxCTRatingX;	 // * Not Used *                              //  2
	uint16_t minCTRating;	 //  2
	uint16_t maxCTSecondary; //  2
	uint16_t minCTSecondary; //  2
	uint16_t maxCTNeutral;	 //  2
	uint16_t minCTNeutral;	 //  2
	// Long Time:
	uint32_t absLTPickup;  //  4
	uint16_t maxLTPickup;  //  2
	uint16_t minLTPickupX; // * Not Used *                              //  2
	uint16_t maxLTDelay;   //  2
	uint16_t minLTDelay;   //  2
	// Short Time:
	uint32_t absSTPickup; //  4
	uint16_t maxSTPickup; //  2
	uint16_t minSTPickup; //  2
	uint16_t maxSTDelay;  //  2
	uint16_t minSTDelay;  //  2
	// Instantaneous:
	uint32_t absInstPickup; //  4
	uint16_t maxInstPickup; //  2
	uint16_t minInstPickup; //  2
	// Ground Fault:
	uint32_t absGFPickup; //  4
	uint16_t maxGFPickup; //  2
	uint16_t minGFPickup; // Not Used                                   //  2
	uint16_t maxGFDelay;  //  2
	uint16_t minGFDelay;  //  2
	// Reduce A/D CT Saturation rail percent x 10 (i.e. 15 = 1.5%).
	uint8_t reduceRailPercentPhase;	  //  1
	uint8_t reduceRailPercentNeutral; //  1
	// Spares
	uint16_t spare0000[5]; // 10
	uint16_t spareFFFF[6]; // 12
	// Conditional Compile Options.
	uint32_t CC_Options; //  4
} Personality4;			 //---
// 96

// MSG_RSP_PERSONALITY_4
typedef struct _MsgRspPersonality4
{
	MsgHdr Hdr;		   //  10
	uint16_t Filler16; //   2
	Personality4 Pers; //  96
} MsgRspPersonality4;  //----
// 108

// MSG_SET_PERSONALITY_4
typedef struct _MsgSetPersonality4
{
	MsgHdr Hdr;		   //  10
	uint16_t Filler16; //   2
	Personality4 Pers; //  96
} MsgSetPersonality4;  //----
// 108

typedef struct _Measurements4
{
	// RMS
	uint16_t Vab; //   2
	uint16_t Vbc; //   2
	uint16_t Vca; //   2
	uint16_t Van; //   2
	uint16_t Vbn; //   2
	uint16_t Vcn; //   2
	uint32_t Ia;  //   4
	uint32_t Ib;  //   4
	uint32_t Ic;  //   4
	uint32_t In;  //   4
	uint32_t Igf; //   4
	// 0-100% if phase loss/rev trip/alarm otherwise set to 65535.
	uint16_t NSOV; //   2
	// Frequency * 100 (as measured).
	uint16_t FrequencyX100; //   2
							// Power
	int32_t KWa;			//   4
	int32_t KWb;			//   4
	int32_t KWc;			//   4
	int32_t KWtotal;		//   4
	int32_t KVAa;			//   4
	int32_t KVAb;			//   4
	int32_t KVAc;			//   4
	int32_t KVAtotal;		//   4
	int16_t PFtotal;		//   2
	// Unbalance %.
	uint8_t UnbalancePercent; //   1
	uint8_t Spare0x00;		  //   1
							  // Running Sum
	int64_t WHr;			  //   8
	int64_t VAHr;			  //   8
	int64_t VARHr;			  //   8
} Measurements4;			  //----
							  //  96

typedef struct _Dynamics4
{
	// Readings.
	Measurements4 Measurements; //  96
	// Trip Unit Status.
	uint32_t Status;	   //   4
	uint32_t StatusHI;	   //   4
	uint32_t StatusMask;   //   4
	uint32_t StatusMaskHI; //   4
	// Local Display Status.
	uint32_t StatusLD;	   //   4
	uint32_t StatusMaskLD; //   4
	// Alarms.
	uint16_t Alarms;		// Raw active alarm bits.                  //   2
	uint16_t AlarmsHI;		//   2
	uint16_t AlarmsMask;	// Alarm bits that can close the relay.    //   2
	uint16_t AlarmsMaskHI;	//   2
	uint16_t AlarmsRelay;	// Alarms & AlarmsMask                     //   2
	uint16_t AlarmsRelayHI; //   2
	// # times breaker has cycled.
	uint16_t BreakerCycleCount; //   2
	// Total number of trips.
	uint16_t TripCount; //   2
	// Power Demand.
	uint8_t PowerDemandPeriod;	//   1
	uint8_t PowerDemandMinutes; //   1
	uint16_t KWD_Last;			//   2
	uint16_t KWD_Peak;			//   2
	uint16_t KVAD_Last;			//   2
	uint16_t KVAD_Peak;			//   2
	// MSP430 temperature in degrees C x10.
	uint16_t TemperatureDegreesCx10; //    2
} Dynamics4;						 //----
									 // 148

typedef struct _MsgRspDynamics4
{
	MsgHdr Hdr;			//  10
	uint16_t Filler16;	//   2
	Dynamics4 Dynamics; // 148
} MsgRspDynamics4;

// Number of Rogowski Coil raw inputs that get calibrated.
#define _NUM_TO_CALIBRATE_RC 8

// Number of HI-Gains supported.
#define _NUM_HI_GAIN 4

// This is calibration data used by A2D_DMA_ISR to calibrate A/D inputs.
// There are one of these tables for each frequency.
typedef struct _CalibrationDataAtFrequencyRC
{
	// Remembers calibrated channels;
	uint16_t CalibratedChannels; //  2
	// Raw A/D offset with A2D_OFFSET.
	uint16_t Offset[_NUM_TO_CALIBRATE_RC]; // 16
	// Software gain.
	uint16_t SwGain[_NUM_TO_CALIBRATE_RC]; // 16
} CalibrationDataAtFrequencyRC;			   //----
// 34

// All of the A/D offsets and software gains for this AC-PRO-II.
// This data is calibrated by the factory and stored in FLASH INFO-D+C+B.
typedef struct _CalibrationDataRC
{
	// 60 Hz calibration data
	CalibrationDataAtFrequencyRC Hz60; // 34
	// 50 Hz calibration data.
	CalibrationDataAtFrequencyRC Hz50; // 34
} CalibrationDataRC;				   //----
// 68

// All of the A/D offsets and software gains for this AC-PRO-RC.
// This data is calibrated by the factory and stored in FLASH INFO-D+C+B.
typedef struct _CalibrationDataFLASH
{
	// Keyword to indicate that all A/D channels are calibrated.
	uint32_t Calibrated; //     4
	// Rogowski Coil calibration information per HI-Gain selected.
	CalibrationDataRC GainHI[_NUM_HI_GAIN]; // 4 x 68 = 272
} CalibrationDataFLASH;						//------
// 276

typedef struct _MsgRspCalibrRC
{
	MsgHdr Hdr;								   //   10
	uint16_t Filler16;						   //     2
	CalibrationDataFLASH CalibrationDataInfoD; // 276
} MsgRspCalibrRC;							   //------
// 288

#define _NUM_PHASES_ABC 3
#define _NUM_PHASES_ABCN 4
#define _TRIP_HISTORY_MAX_TRIPS 8
#define _TRIP_TYPE_MAX 32 // Maximum number of trip types.

// Trip Count Information.
typedef struct _CounterTH4
{
	// # trips per type.                                                     // 64
	uint16_t trip[_TRIP_TYPE_MAX];
	// # of Sluggish Breaker Events                                        //  2
	uint16_t sluggishBreaker;
	// Next offset to store trip history in FRAM.                          //  2
	uint16_t nextAvailableOffset;
} CounterTH4;

// Breaker Clearing Times in 0.1 ms.
typedef struct _BkrClrTimeTH
{
	uint16_t Phase[_NUM_PHASES_ABC]; //  6
} BkrClrTimeTH;

// Information collected when a trip occurs.
typedef struct _TripData4
{
	TimeDate4 TimeStamp;		   // Time/Date that each trip occured.//  8
	BkrClrTimeTH BCT;			   // BCT in ms per phase.             //  6
	uint16_t Spare0xFFFF;		   // Future - set to 0xFFFF           //  2
	uint32_t TripStatus;		   // Trip Status bit pattern          //  4
	uint32_t I[_NUM_PHASES_ABCN];  // Phase and Neutral Current.       // 16
	uint32_t I_GF;				   // Ground Fault Current.            //  4
	uint16_t VLL[_NUM_PHASES_ABC]; // Voltage Line-Line                //  6
	uint16_t VLN[_NUM_PHASES_ABC]; // Voltage Line-Neutral             //  6
	uint16_t CTRating;			   // CT Rating                        //  2
	uint16_t CTSecondary;		   // CT Rating Secondary Phase        //  2
	uint16_t CTNeutralSec;		   // CT Rating Secondary Neutral      //  2
	uint8_t UnbalancePercent;	   // Unbalance % that caused a trip.  //  1
	uint8_t Spare0x00;			   // Future - set to 0x00.            //  1
	uint8_t Frequency;			   // 60 or 50 Hz                      //  1
	uint8_t NSOVpercent;		   // 2-32% - TRIP_TYPE_V_PH_LOSS_REV  //  1
	uint8_t TripType;			   // Trip Type (see above).           //  1
	uint8_t ThresholdSB;		   // Sluggish Breaker Threshold       //  1
	uint32_t PeakRMS;			   // Maximum peak current of trip.    //  4
} TripData4;

typedef struct _TripHist4
{
	// Number of trips per trip type.                                     //  68
	CounterTH4 Counter;
	// Spare.                                                             //   2
	uint16_t Spare16;
	// # of trip data buffers (0 - TRIP_HISTORY_MAX_TRIPS)                //   2
	uint16_t NumBufs;
	// Data captured per trip.                   // 8 x sizeof(TripData4) =  544
	TripData4 Data[_TRIP_HISTORY_MAX_TRIPS];
} TripHist4;

// MSG_RSP_TRIP_HIST_4
typedef struct _MsgRspTripHist4
{
	MsgHdr Hdr;		   //  10
	uint16_t Filler16; //   2
	TripHist4 Trips;   // 616
} MsgRspTripHist4;

#define MSG_STR_LEN 12
typedef struct _MsgRspSerNum
{
	MsgHdr Hdr;
	char Number[MSG_STR_LEN];
} MsgRspSerNum;

typedef struct _HardVer
{
	uint8_t boardID;  // 0= Trip Unit  1= Local Display  11= ACPRO-Update
	uint8_t boardRev; // 0 Increment if board is ever modified.
	uint8_t batchID;  // 0 Increment when new batch of boards is used.
	uint8_t msgAddr;  // USB message address of board.
} HardVer;

typedef struct _MsgRspHwRev
{
	MsgHdr Hdr;
	HardVer HW;
} MsgRspHwRev;

#define _SER_NUM_LENGTH 12
typedef struct _MsgSetSerNum4
{
	MsgHdr Hdr;					  // 10
	uint16_t Filler;			  //  2
	char Number[_SER_NUM_LENGTH]; // 12
	HardVer HWversion;			  //  4
} MsgSetSerNum4;

// Only 21 characters displayed per line, Line[21] should be '\0'.
// If line not used put '\0' into Line[0].
// Maximum 5-character softKey.
// If sofyKey is not used put '\0' into SoftKeyPBx[0].
typedef struct _RemoteScreen4
{
	// true= Remote Screen is active - override all other screens.
	bool Enabled; //   1
	// LED 1=ON/0=OFF bit pattern.
	uint8_t LEDs; //   1
	// Blink Information (see SCREEN.Blink in screen.h)
	uint16_t BlinkInfo; //   2
	// SoftKey: Max 5-char + '\0'.
	char SoftKeyPB1[6]; //   6
	char SoftKeyPB2[6]; //   6
	char SoftKeyPB3[6]; //   6
	char SoftKeyPB4[6]; //   6
	// Text Line: Max 21-char + '\0'.
	char Line1[22]; //  22
	char Line2[22]; //  22
	char Line3[22]; //  22
	char Line4[22]; //  22
	char Line5[22]; //  22
	char Line6[22]; //  22
} RemoteScreen4;	//----
					// 160
// Sent back to whoever sent MSG_REMOTE_SOFTKEYS.
// Use MSG_REMOTE_SCREEN 93
typedef struct _MsgRemoteScreen4
{
	MsgHdr Hdr;					//  10
	uint16_t Filler16;			//   2
	RemoteScreen4 RemoteScreen; // 160
} MsgRemoteScreen4;				//----
								// 172
// Sent back to whoever sent MSG_REMOTE_SCREEN.
// Use MSG_REMOTE_SOFTKEYS 94
typedef struct _MsgRemoteSoftKeys4
{
	MsgHdr Hdr; //  10
	// true= Send Remote Screens.
	bool SendRemoteScreen; //   1
	// SoftKey bit pattern.
	uint8_t RecvRemoteSoftKeys; //   1
} MsgRemoteSoftKeys4;			//----
								//  12

// Simulation timing per A/D channel.
typedef struct _TimingA2d4
{
	// Cycle to start simulation.
	uint16_t cycleON; //  2
	// Cycle to stop  simulation.
	uint16_t cycleOFF; //  2
} TimingA2D4;

// Software Testset Control.
typedef struct _SoftTestSetSimulation4
{
	// Number of cycles until wrap-around (reset cycleNumber to zero).
	uint16_t cycleNumberMax; //  2
	// CycleNumber to end the simulation.
	uint16_t cycleNumberTimeOut; //  2
	// Simulated Amps/Volts bit pattern.
	uint16_t inputsA2D; //  2

	//   C U R R E N T

	// # samples after firing the actuator to stop the simulation.
	uint16_t ampsOFF[_NUM_PHASES_ABCN]; //  8
	// Simulate Background Amps.
	TimingA2D4 backgroundAmpsTiming[_NUM_PHASES_ABCN]; // 16
	// Simulate Foreground Amps.
	TimingA2D4 foregroundAmpsTiming[_NUM_PHASES_ABCN]; // 16

	//   V O L T A G E

	// # samples after firing the actuator to stop the simulation.
	uint16_t voltsOFF[_NUM_PHASES_ABC]; //  6
	// Simulate Background Volts.
	TimingA2D4 backgroundVoltsTiming[_NUM_PHASES_ABC]; // 12
	// Simulate Foreground Volts.
	TimingA2D4 foregroundVoltsTiming[_NUM_PHASES_ABC]; // 12

	//   F U T U R E

	// Not Used.
	uint16_t spare16[10]; // 20
} SoftTestSetSimulation4; //---
						  // 96

// MSG_EXE_TESTSET_SETUP_4
typedef struct _MsgExeTestSet4
{
	MsgHdr Hdr;					  //  10
	uint16_t Filler16;			  //   2
	SoftTestSetSimulation4 Setup; //  96
} MsgExeTestSet4;

// MSG_SET_TESTSET_AMPS_4
typedef struct _MsgSetTestSetAmps4
{
	MsgHdr Hdr;		   //  10
	uint16_t Filler16; //   2
	// Waveform type (0==background or 1==foreground).
	uint16_t Type; //   2
	// Phase or Neutral (0==A, 1==B, 2==C, 3==Neutral).
	uint16_t Phase;					 //   2
									 // 18-bit signed A/D readings for a phase or neutral.
	int32_t A2D[_SAMPLES_PER_CYCLE]; // 252
} MsgSetTestSetAmps4;

// One voltage sample of 3 phases.
typedef struct _WaveVoltsA2D
{
	// 12-bit signed A/D readings of 3 phases of voltage.
	int16_t phase[_NUM_PHASES_ABC]; //   6
} WaveVoltsA2D;

// MSG_SET_TESTSET_VOLTS_4
typedef struct _MsgSetTestSetVolts4
{
	MsgHdr Hdr;		   //  10
	uint16_t Filler16; //   2
	// Waveform type (0==background or 1==foreground).
	uint16_t Type; //   2
	// Signed 12-bit A/D readings for a voltage.
	WaveVoltsA2D voltsFor[_SAMPLES_PER_CYCLE]; // 378
} MsgSetTestSetVolts4;

// Software Testset Results per Phase.
typedef struct _PerPhase4
{
	// RMS value of Amps calculated from waveform.
	uint32_t CalcI; //  4
	// RMS value of Amps to use for protection and display.
	uint32_t ValueI; //  4
	// RMS value of Amps calculated from waveform peak.
	uint32_t PeakI; //  4
	// RMS value of Amps used to trip.
	uint32_t TripI; //  4
	// RMS value of voltage calculated from waveform (line-neutral).
	uint16_t VLN; //  2
	// RMS value of voltage calculated from waveform (line-line).
	uint16_t VLL; //  2
	// Breaker Clearing Time in ms.
	uint16_t BCTIMEms; //  2
	// Not Used.
	uint16_t spare16;	 //  2
	uint32_t spare32[2]; //  8
} PerPhase4;			 //---

// Software Testset Results.
typedef struct _SoftTestSetResults4
{
	// Time/Date
	TimeDate4 timeDateStamp; //  8
	// Results per phase.
	PerPhase4 phase[3]; // 96
	// RMS value of Amps calculated from waveform.
	uint32_t calcIn; //  4
	// RMS value of Amps to use for protection and display.
	uint32_t valueIn; //  4
	// RMS value of Amps calculated from waveform peak.
	uint32_t peakIn; //  4
	// RMS value of Amps used to trip.
	uint32_t tripIn; //  4
	// Calculated Ground Fault current.
	uint32_t calcIgf; //  4
	// Time from start of pickup until trip in ms.
	uint16_t pickupUntilTripTIMEms; //  2
	// GF Thermal Memory cycle count until trip register is zero.
	uint16_t thermalMemoryCyclesUntilZeroGF; //  2
	// LT Thermal Memory cycle count until trip register is zero.
	uint16_t thermalMemoryCyclesUntilZeroLT[3]; //  6
	// Time from trip until actuator is fired in us.
	uint16_t tripUntilFireActuatorTIMEus; //  2
	// LEDs bit pattern.
	// If we had as many flashing LEDs as USS Enterprise these would be them.
	// But only a Vulcan could read and understand them.
	uint16_t LEDs; //  2
	// Not Used.
	uint16_t spare16[5]; // 10
	// MSP430 temperature x10.
	uint16_t temperatureDegreesCx10; //  2
	// Actuator voltage x10.
	uint16_t voltaActuatorX10; //  2
	// Battery voltage x10.
	uint8_t voltsBatteryX10; //  1
	// +5 volts.
	uint8_t voltsFiveX10; //  1
	// true= Time-Out stopped the simulation.
	bool timedOut; //  1
	// true= Trip stopped simulation.
	bool tripped; //  1
	// Trip Status Bit Pattern.
	uint32_t tripStatus; //  4
	// Trip Type.
	uint8_t tripType; //  1
	// U/B Percent.
	uint8_t ubPercent; //  1
	// 2-32% - TRIP_TYPE_V_PH_LOSS_REV
	uint8_t NSOVperctnt; //  1
	// Not Used.
	uint8_t spare8;	   //  1
} SoftTestSetResults4; //---

// MSG_RSP_TESTSET_RESULTS_4
typedef struct _MsgRspTestSetResults4
{
	MsgHdr Hdr;					 //  10
	uint16_t Filler16;			 //   2
	SoftTestSetResults4 Results; // 164
} MsgRspTestSetResults4;		 //----

// MSG_GET_RESPONSE_4
typedef struct _MsgGetRsp4
{
	MsgHdr Hdr;
	uint8_t Response;
	uint8_t Spare8;
} MsgGetRsp4;

typedef struct _MsgSetBaudRate
{
	MsgHdr Hdr;
	uint32_t BaudRate;
} MsgSetBaudRate;

typedef struct _MsgRspBaudRates
{
	MsgHdr Hdr;
	uint32_t BaudRates[8];
} MsgRspBaudRates;

typedef struct _MsgRspSwVer
{
	MsgHdr Hdr;
	SoftVer Version;
} MsgRspSwVer;

/////////////////////////////////////////////////////////////
// QTD2 messages
/////////////////////////////////////////////////////////////

typedef struct _Msg_QTD2_Set5VEnable
{
	MsgHdr Hdr;
	uint8_t shouldEnable;

} Msg_QTD2_Set5VEnable;

typedef struct _Msg_QTD2_SetRelayStatus
{
	MsgHdr Hdr;
	uint8_t enableRelay1;
	// other relay is always driven to opposite state

} Msg_QTD2_SetRelayStatus;

typedef struct _QTD2Status
{
	uint8_t Connection; // NONE (0), ADDR_TRIP_UNIT (1), ADDR_AC_PRO_NW (15), ...
	// Add more QTD2 info?
	uint32_t statusBits;
	uint32_t Reserved2;
} QTD2Status;

typedef struct _MsgRspQTD2Status

{
	MsgHdr Hdr;
	QTD2Status Status;
} MsgRspQTD2Status;

#define MAX_NUM_LEDS 4
#define LED_PICK_UP 0
#define LED_QT 1
#define LED_OK 2
#define LED_DIAG 3

#pragma pack()

typedef struct _MsgRspLEDs
{
	MsgHdr Hdr;
	uint8_t Num;
	uint8_t State[MAX_NUM_LEDS];
} MsgRspLEDs;

typedef struct _MsgSetSwitchQT4
{
	MsgHdr Hdr;		// 10
	uint16_t State; //  2
} MsgSetSwitchQT4;

#pragma pack(1)

typedef union _URCMessageUnion
{
	uint8_t buf[1024]; // largest size of a single URC message....

	MsgHdr msgHdr;
	MsgACK msgACK;
	MsgNAK msgNAK;
	TimeDate4 timeDate4;
	MsgSetTimeDate msgSetTimeDate;
	MsgRspTimeDate msgRspTimeDate;
	SoftVer softVer;
	SystemSettings4 systemSettings4;
	DeviceSettings4 deviceSettings4;
	MsgRspDevSet4 msgRspDevSet4;
	MsgRspSoftVer msgRspSoftVer;
	StatusTU2 statusTU2;
	MsgRspStatus2 msgRspStatus2;
	CalibrationDataAtFrequency calibrationDataAtFrequency;
	CalibrationData calibrationData;
	CalibrationDataRAM calibrationDataRAM;
	CalibrationRequest calibrationRequest;
	CalibrationResults calibrationResults;
	MsgExeCalibrateAD msgExeCalibrateAD;
	MsgRspCalibrateAD msgRspCalibrateAD;
	MsgGetCalibr msgGetCalibr;
	MsgRspCalibr msgRspCalibr;
	MsgRspSysSet4 msgRspSysSet4;
	MsgSetUserSet4 msgSetUserSet4;
	Personality4 personality4;
	MsgRspPersonality4 msgRspPersonality4;
	MsgSetPersonality4 msgSetPersonality4;
	MsgRspDynamics4 msgRspDynamics4;
	MsgRspCalibrRC msgRspCalibrRC;
	MsgRspTripHist4 msgRspTripHist4;
	MsgRspSerNum msgRspSerNum;
	MsgRspHwRev msgRspHwRev;
	MsgSetSerNum4 msgSetSerNum4;
	MsgRemoteSoftKeys4 msgRemoteSoftKeys4;
	MsgRemoteScreen4 msgRemoteScreen4;
	MsgRspLEDs msgRspLEDs;
	MsgExeTestSet4 msgExeTestSet4;
	MsgRspTestSetResults4 msgRspTestSetResults4;
	MsgGetRsp4 msgGetRsp4;
	MsgSetBaudRate msgSetBaudRate;
	MsgRspBaudRates msgRspBaudRates;
	MsgRspSwVer msgRspSwVer;
	MsgSetSwitchQT4 msgSetSwitchQT4;

	// QTD2 messages
	Msg_QTD2_Set5VEnable msg_QTD2_Set5VEnable;
	Msg_QTD2_SetRelayStatus msg_QTD2_SetRelayStatus;
	MsgRspQTD2Status msgRspQTD2Status;

} URCMessageUnion;

#pragma pack()

// function prototypes

uint16_t SequenceNumber();
uint16_t CalcChecksum(const uint8_t *dataBuf, uint16_t length);

bool SendURCCommand(HANDLE hComm, uint8_t msgType, uint8_t dst, uint8_t src);
bool GetAckURCResponse(HANDLE hComm, URCMessageUnion *msg);
bool GetURCResponse(HANDLE hComm, URCMessageUnion *msg);
bool SendAck(HANDLE hComm, MsgHdr hdr);
bool GetURCResponse_Long(HANDLE hComm, URCMessageUnion *msg, int longTimeOutMS);
void PrintAddress(int addr);
void DumpHeader(MsgHdr *hdr);
void PrintAddress(int addr);
void DumpRawMsgData(uint8_t *data_ptr, int data_length, bool AmSending);
void HexDumpBuffer(uint8_t *buf, int len);
bool VerifyMessageIsOK(URCMessageUnion *msg, int ExpectedMessageType, int ExpectedMessageLength);
bool MessageIsACK(URCMessageUnion *msg);
UINT SendRemoteSoftkeyCommand(HANDLE hComm, uint8_t key);
bool VerifyMessageIsOK(URCMessageUnion *msg, int ExpectedMessageType, int ExpectedMessageLength, int ExpectedDst, int ExpectedSrc);