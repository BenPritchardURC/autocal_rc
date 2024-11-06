
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

#define STS_CALIBRATED 0x00000001L              // Calibrated
#define STS_COMMISSIONED 0x00000002L            // Commissioned
#define STS_VDM_ATTACHED 0x00000004L            // VDM attached
#define STS_VDM_NOT_POWERED 0x00000008L         // only if VDM attached
#define STS_ACTUATOR_OPEN 0x00000010L           // Actuator Opened
#define STS_SLUGGISH_BKR 0x00000020L            // Sluggish Breaker
#define STS_INTERNAL_ERR 0x00000040L            // Internal TU error
#define STS_POWER_FRON_CT_OR_24V 0x00000080L    // CT or 24V is providing power
#define STS_FATAL_ERROR 0x00000100L             // No Protection
#define STS_GF_BLOCKED 0x00000200L              // Residual GF-BLOCK
#define STS_00000400 0x00000400L                // * Not Used *
#define STS_00000800 0x00000800L                // * Not Used *
#define STS_TEST_MODE_ACTIVE 0x00001000L        // Test Mode is active (60 min)
#define STS_TEST_MODE_WARNING 0x00002000L       // Test Mode time-out in 5 min
#define STS_TEST_MODE_TIMEOUT 0x00004000L       // Test Mode timed-out.
#define STS_QT_ON 0x00008000L                   // Quick Trip Switch
#define STS_BKR_POS_CONTACT_ERR 0x00010000L     // Open but has current
#define STS_00020000 0x00020000L                // * Not Used *
#define STS_VOLTAGE_NA 0x00040000L              // Display Voltage n/a.
#define STS_GF_I5T_SUPPORT 0x00080000L          // Supports GF I5T
#define STS_EEPOT_ERROR_WIPER_INST 0x00100000L  // EEPOT Wiper Inst failed.
#define STS_EEPOT_ERROR_WIPER_QT_I 0x00200000L  // EEPOT Wiper QT-I failed.
#define STS_TRIP_UNIT_POWER_UP 0x00400000L      // Set for 1 sec after power-up.
#define STS_REV_PHASE_CT_POLARITY 0x00800000L   // Phase CT reversed polarity
#define STS_NO_COMMUNICATIONS 0x01000000L       // * Local Display Only *
#define STS_BOOT_STRAP_LOADER 0x02000000L       // * Local Display Only *
#define STS_SET_USER_SETTINGS 0x04000000L       // * Local Display Only *
#define STS_LOW_BATTERY 0x08000000L             // * Local Display Only *
#define STS_GF_DEFEATED_BY_INPUT 0x10000000L    // Input defeats GF
#define STS_BREAKER_OPENED 0x20000000L          // Breaker is Open
#define STS_REV_NEUTRAL_CT_POLARITY 0x40000000L // Neutral CT reversed polarity
#define STS_GF_TRIP_TEST_POWER 0x80000000L      // Power to do GF Trip Test
#define STS_OPEN_CIRCUIT_CT 0x00040000ul        // One or more CT Open Circuit

#define NAK_ERROR_UNDEF 1              // Some other error
#define NAK_ERROR_CHECKSUM 2           // Invalid checksum
#define NAK_ERROR_UNREC 3              // Unrecognized message
#define NAK_ERROR_NOT_READY 4          // Not ready to process
#define NAK_ERROR_PROTOCOL 5           // Unsupported protocol version
#define NAK_ERROR_NO_DATA 6            // No Data Available
#define NAK_ERROR_INVALID_PARAMETERS 7 // Invalid Parameters
#define NAK_ERROR_INVALID_LENGTH 8     // Invalid Length field
#define NAK_ERROR_SOURCE 9             // Invalid Source field
#define NAK_ERROR_TARGET 10            // Invalid Destination field
#define NAK_ERROR_SAMPLE_TYPE 11       // Invalid Sample Type field
#define NAK_ERROR_INVALID_PARAMETER 12 // Invalid parameter/data
#define NAK_ERROR_RS485_ADDR 13        // Invalid RS485 address == 0
#define NAK_ERROR_NO_CODE 14           // Not implemented yet
#define NAK_ERROR_FRAM_LOW_VOLT 15     // FRAM Low Voltage
#define NAK_ERROR_FRAM_TRIP_NOW 16     // FRAM Trip in Process
#define NAK_ERROR_NOT_REQUESTED 17     // Recieved response not requested.
#define NAK_ERROR_NOT_SUPPORTED 18     // Received request  not supported.
#define NAK_ERROR_CALIBRATING 19       // Calibration in progress.
#define NAK_ERROR_FLASHWRITE 20        // unable to write to flash
#define NAK_ERROR_UNABLE_TO_DO 21      // cannot do, eg, clear trip history
#define NAK_ERROR_FRAM_FAULT 22        // FRAM update failed.
#define NAK_ERROR_FACTORY_ONLY 23      // Only URC factory GUI can set data.
#define NAK_ERROR_NO_RESPONSE 24       // Time-out - no response from dest.

#define NAK_ERROR_QT_DISPLAY_II_CRC 25       // RJ45 CRC was not correct.
#define NAK_ERROR_TIMEOUT_TOO_SLOW 26        // messages > 500 ms apart.
#define NAK_ERROR_PERSONALITY4_STRUCT_ID 27  // Bad Personality4.structID
#define NAK_ERROR_NO_RELAY_ASSIGNED 28       // No relay was assigned to breaker.
#define NAK_ERROR_INVALID_A2D_READING 29     // A/D sample out-of-range.
#define NAK_ERROR_BT_NOT_AVAILABLE 30        // Cannot talk to Bluetooth.
#define NAK_ERROR_PERSONALITY4_CC_OPTIONS 31 // Bad Personality4.structID

// Remote Screen specific errors.
#define NAK_ERROR_BAD_SOFTKEY_CHAR 40 // Non-printable softKey text char.
#define NAK_ERROR_BAD_LINE_CHAR 41    // Non-printable screen line char.
#define NAK_ERROR_NO_END_OF_TEXT 42   // Did not find '\0' in screen line.
#define NAK_ERROR_BLANK_SOFTKEY 43    // SoftKey is blank.

// Calibration
#define NAK_ERROR_ALREADY_CALIBRATING 100
#define NAK_ERROR_CANNOT_DO_BOTH_HI_AND_LO 101
#define NAK_ERROR_MUST_DO_HI_GAIN_FIRST 102
#define NAK_ERROR_ILLEGAL_GAIN_SETTING 103
#define NAK_ERROR_RAILOUT_HI_GAIN 104
#define NAK_ERROR_RAILOUT_LO_GAIN 105
#define NAK_ERROR_CANNOT_HIT_TARGET 106
#define NAK_ERROR_TARGET_TOO_BIG 107

// BootStrap Loader specific errors.
#define NAK_BSL_FIRST 1000
#define BSL_TRIP_UNIT_STILL_ACTIVE 1000
#define BSL_ERROR_ERASING_FLASH 1001
#define BSL_ERROR_BURNING_FLASH 1002
#define BSL_ILLEGAL_FLASH_ADDRESS 1003
#define BSL_ILLEGAL_ASCII_HEX_DIGIT 1004
#define BSL_UNEXPECTED_CHAR_IN_FILE 1005
#define BSL_ONLY_ONE_DIGIT_OF_DATA 1006
#define BSL_BAD_CHECKSUM 1007
#define BSL_NO_APP_PROGRAM 1008
#define BSL_OUT_OF_SEQUENCE 1009
#define BSL_FATAL_ERROR 1010
#define BSL_CMD_NOT_SUPPORTED 1011
#define NAK_BSL_LAST 1011
// Change System Settings specific errors.
#define NAK_SS_CT_RATING 2000
#define NAK_SS_CT_SECONDARY 2001
#define NAK_SS_CT_NEUTRAL_SEC 2002
#define NAK_SS_FREQUENCY 2003
#define NAK_SS_BKR_POS_CNT_TYPE 2004
#define NAK_SS_POWER_DIRECTION 2005
#define NAK_SS_AUTO_POLARITY_CT 2006
#define NAK_SS_ROGOWSKI_COIL_SENSITIVITY 2007
#define NAK_SS_ROGOWSKI_COIL_SENSOR_TYPE 2008
#define NAK_US_FV_SEG_ALARMS 2009
// Change Device Settings specific errors.
#define NAK_DS_LT_ENABLED 2010
#define NAK_DS_LT_PICKUP 2011
#define NAK_DS_LT_DELAY 2012
#define NAK_DS_LT_THERM_MEM 2013
#define NAK_DS_ST_ENABLED 2014
#define NAK_DS_ST_PICKUP 2015
#define NAK_DS_ST_DELAY 2016
#define NAK_DS_ST_I2T 2017
#define NAK_DS_GF_TYPE 2018
#define NAK_DS_GF_PICKUP 2019
#define NAK_DS_GF_DELAY 2020
#define NAK_DS_GF_I2T 2021
#define NAK_DS_GF_QT_TYPE 2022
#define NAK_DS_GF_QT_PICKUP 2023
#define NAK_DS_QT_INST_PICKUP 2024
#define NAK_DS_INSTANT_ENABLED 2025
#define NAK_DS_INSTANT_PICKUP 2026
#define NAK_DS_NOL_ENABLED 2027
#define NAK_DS_NOL_PICKUP 2028
#define NAK_DS_NOL_DELAY 2029
#define NAK_DS_NOL_THERM_MEM 2030
#define NAK_DS_UV_TRIP_ENABLED 2031
#define NAK_DS_UV_PICKUP 2032
#define NAK_DS_UV_DELAY 2033
#define NAK_DS_OV_TRIP_ENABLED 2034
#define NAK_DS_OV_PICKUP 2035
#define NAK_DS_OV_DELAY 2036
#define NAK_DS_PV_LOSS_ENABLED 2037
#define NAK_DS_PV_LOSS_DELAY 2038
#define NAK_DS_NSOV_PICKUP_PERCENT 2039
#define NAK_DS_AVAILABLE_2040 2040
#define NAK_DS_AVAILABLE_2041 2041
#define NAK_DS_AVAILABLE_2042 2042
#define NAK_DS_SB_THRESHOLD 2043
#define NAK_DS_FORCED_TRIP_ENABLED 2044
#define NAK_DS_REMOTE_ENABLED 2045
#define NAK_DS_POWER_REVERSED 2046
#define NAK_DS_ALARM_RELAY_TU 2047
#define NAK_DS_ALARM_RELAY_RIU1 2048
#define NAK_DS_ALARM_RELAY_RIU2 2049
#define NAK_US_LOAD_MONITOR_PICKUP_IC1 2050
#define NAK_US_LOAD_MONITOR_PICKUP_IC2 2051
#define NAK_DS_QT_SWITCH_ENABLED 2052
#define NAK_DS_UB_ENABLED 2053
#define NAK_DS_UB_PICKUP 2054
#define NAK_DS_UB_DELAY 2055
#define NAK_DS_DATE_FORMAT 2056
#define NAK_DS_SAFE_T_CLOSE 2057
#define NAK_NO_CHANGES 2069
#define NAK_MODBUS_CONFIG_REPLY_DELAY 2096
#define NAK_MODBUS_CONFIG_PARITY 2097
#define NAK_MODBUS_CONFIG_BAUDRATE 2098
#define NAK_MODBUS_CONFIG_ADDR 2099

#define ALARM_ACTUATOR_OPEN 0x0001    // Actuator Open
#define ALARM_INTERNAL_ERROR 0x0002   // TU error (OK LED OFF)
#define ALARM_UNDER_VOLTAGE 0x0004    // Under Voltage condition
#define ALARM_OVER_VOLTAGE 0x0008     // Over  Voltage Condition
#define ALARM_SLUGGISH_BREAKER 0x0010 // Sluggish or Stuck Breaker
#define ALARM_LT_PICKUP 0x0020        // LT Pickup
#define ALARM_TRIPPED 0x0040          // Trip Event
#define ALARM_GROUND_FAULT 0x0080     // Ground Fault
#define ALARM_PHASE_LOSS_REV 0x0100   // Phase Loss / Reverse
#define ALARM_AVAILABLE_0200 0x0200   // * Not Used *
#define ALARM_HIGH_CURRENT 0x0400     // Trip Event - cleared by QT-CONTROL
#define ALARM_AVAILABLE_0800 0x0800   // * Not Used *
#define ALARM_AVAILABLE_1000 0x1000   // * Not Used *
#define ALARM_UNDER_FREQUENCY 0x2000  // Under Frequency condition
#define ALARM_OVER_FREQUENCY 0x4000   // Over  Frequency Condition
#define ALARM_RELAY_ON 0x8000         // Alarm Relay Status

#define _NUM_A2D_INPUTS_CALIBRATED 8
#define _NUM_A2D_INPUTS_RAW_CAL 12

// Index into CalibrationDataRAM.Offset[NUM_A2D_INPUTS_CALIBRATED]
// Index into CalibrationDataRAM.swGain[NUM_A2D_INPUTS_CALIBRATED]
#define i_calibrationIaHI 0
#define i_calibrationIaLO 1
#define i_calibrationIbHI 2
#define i_calibrationIbLO 3
#define i_calibrationIcHI 4
#define i_calibrationIcLO 5
#define i_calibrationInHI 6
#define i_calibrationInLO 7

// Bits for CalibrationRequest.commands
#define _CALIBRATION_REQUEST_IA_HI 0x0001
#define _CALIBRATION_REQUEST_IA_LO 0x0002
#define _CALIBRATION_REQUEST_IB_HI 0x0004
#define _CALIBRATION_REQUEST_IB_LO 0x0008
#define _CALIBRATION_REQUEST_IC_HI 0x0010
#define _CALIBRATION_REQUEST_IC_LO 0x0020
#define _CALIBRATION_REQUEST_IN_HI 0x0040
#define _CALIBRATION_REQUEST_IN_LO 0x0080
#define _CALIBRATION_REQUEST_ALL 0x00FF
#define _CALIBRATION_REQUEST_AMPS 0x00FF

// Reset calibration edit buffer to default values.
#define _CALIBRATION_SET_TO_DEFAULT 0x1000
// Clear all calibrated bits in FLASH (leave offset/gain alone).
#define _CALIBRATION_UNCALIBRATE 0x2000
// Get new RAM copy of calibration data to edit from FLASH.
#define _CALIBRATION_INITIALIZE 0x4000
// Burn RAM edited copy of calibration data to FLASH INFO-C
#define _CALIBRATION_WRITE_TO_FLASH 0x8000
