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

#include <iostream>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>
#include <vector>

// code for interacting with production database

namespace DB
{
    SQLHENV env = NULL;
    SQLHDBC dbc = NULL;

    bool _connected;

    bool Connect(const std::string &database_file)
    {
        SQLCHAR outstr[1024];
        SQLSMALLINT outstrlen;

        bool retval = false;

        // Allocate an environment handle
        SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &env);

        // Set the ODBC version environment attribute
        SQLSetEnvAttr(env, SQL_ATTR_ODBC_VERSION, (void *)SQL_OV_ODBC3, 0);

        // Allocate a connection handle
        SQLAllocHandle(SQL_HANDLE_DBC, env, &dbc);

        // Connect to the database
        std::string connection_string;

        connection_string =
            "DRIVER={Microsoft Access Driver (*.mdb, *.accdb)}; DBQ=" + database_file;

        retval = (SQLDriverConnect(dbc,
                                   NULL,
                                   (SQLCHAR *)connection_string.c_str(),
                                   SQL_NTS,
                                   outstr,
                                   sizeof(outstr),
                                   &outstrlen,
                                   SQL_DRIVER_NOPROMPT) == SQL_SUCCESS);

        _connected = retval;
        return retval;
    }

    void Disconnect()
    {
        SQLDisconnect(dbc);
        SQLFreeHandle(SQL_HANDLE_DBC, dbc);
        SQLFreeHandle(SQL_HANDLE_ENV, env);
    }

    static std::string bool_to_zero_or_one(bool b)
    {
        return b ? "1" : "0";
    }

    // code for dealing with columns re: to the LT Threshold Test
    namespace TLThreshold
    {
        // read the LT Threshold record from the database
        // fills in record, for the row with the given serial number
        bool Read(LTThresholdRecord &record, const std::string &serial_num)
        {
            SQLHSTMT stmt = NULL;
            SQLRETURN result;

            bool retval = false;

            if (!_connected)
                return false;

            std::string column_list =
                "[LTNoPu_A], "
                "[LTPu_A], "
                "[LTNoPu_B], "
                "[LTPu_B], "
                "[LTNoPu_C], "
                "[LTPu_C]";

            std::string sql_query_string =
                "SELECT ID, " + column_list + " FROM AutoTestResults WHERE [Serial Number] = " + "'" + serial_num + "'";

            // Allocate a statement handle
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            // Execute the query
            SQLExecDirect(stmt, (SQLCHAR *)sql_query_string.c_str(), SQL_NTS);

            // query should contain only one row; extract its contents in record
            if (SQLFetch(stmt) == SQL_SUCCESS)
            {
                // save the serial number this record is for
                record.SerialNum = serial_num;

                bool everythingOK = true;
                everythingOK &=
                    (SQLGetData(stmt, 1, SQL_INTEGER, &record.ID, sizeof(record.ID), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 2, SQL_INTEGER, &record.LTNoPu_A, sizeof(record.LTNoPu_A), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 3, SQL_INTEGER, &record.LTPu_A, sizeof(record.LTPu_A), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 4, SQL_INTEGER, &record.LTNoPu_B, sizeof(record.LTNoPu_B), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 5, SQL_INTEGER, &record.LTPu_B, sizeof(record.LTPu_B), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 6, SQL_INTEGER, &record.LTNoPu_C, sizeof(record.LTNoPu_C), NULL) == SQL_SUCCESS);

                everythingOK &=
                    (SQLGetData(stmt, 7, SQL_INTEGER, &record.LTPu_C, sizeof(record.LTPu_C), NULL) == SQL_SUCCESS);

                // if we couldn't get all the columns for some reason, i think it is better
                // to just return a blank record
                if (!everythingOK)
                    record = {0};

                retval = everythingOK;
            }

            return retval;
        }

        // write the LT Threshold record to the database.
        // record already contains the serial number
        bool Write(const LTThresholdRecord &record)
        {
            SQLHSTMT stmt = NULL;
            bool retval = false;

            if (!_connected)
                return false;

            if (record.ID == 0 || record.SerialNum.empty())
                return false;

            std::string sql_query_string =
                "UPDATE AutoTestResults SET "
                "LTNoPu_A = " +
                std::to_string(record.LTNoPu_A) + ", " +
                "LTPu_A = " + std::to_string(record.LTPu_A) + ", " +
                "LTNoPu_B = " + std::to_string(record.LTNoPu_B) + ", " +
                "LTPu_B = " + std::to_string(record.LTPu_B) + ", " +
                "LTNoPu_C = " + std::to_string(record.LTNoPu_C) + ", " +
                "LTPu_C = " + std::to_string(record.LTPu_C) + " " +
                "WHERE ID = " + std::to_string(record.ID);

            // Allocate a statement handle
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            // Execute the query
            retval = (SQLExecDirect(stmt, (SQLCHAR *)sql_query_string.c_str(), SQL_NTS) == SQL_SUCCESS);

            return retval;
        }
    }

    // code for dealing with columns re: the personality info stored in the database
    namespace Personality
    {
        bool Read(PersonalityRecord &record, const std::string &serial_num)
        {

            SQLHSTMT stmt = NULL;
            bool retval = false;

            if (!_connected)
                return false;

            std::string column_list =
                "[LT PU On/Off Personalitity], "
                "[LT PU to 120% Personality], "
                "[LT Dly to 50 sec Personality], "
                "[Inst Override Personality], "
                "[Inst Close Personality], "
                "[Inst Block Personality], "
                "[GF Only Personality]";

            std::string sql_query_string =
                "SELECT ID, " + column_list + " FROM AutoTestResults WHERE [Serial Number] = " + "'" + serial_num + "'";

            // Allocate a statement handle
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            // Execute the query
            SQLExecDirect(stmt, (SQLCHAR *)sql_query_string.c_str(), SQL_NTS);

            // query should contain only one row; extrat its contents in record
            if (SQLFetch(stmt) == SQL_SUCCESS)
            {
                // save the serial number this record is for
                record.SerialNum = serial_num;

                int tmp_int;
                bool everythingOK = true;

                everythingOK &=
                    (SQLGetData(stmt, 1, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.ID = tmp_int;

                everythingOK &=
                    (SQLGetData(stmt, 2, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.LTPUOnOffPersonalitity = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 3, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.LTPUto120PercentPersonality = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 4, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.LTDlyto50secPersonality = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 5, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.InstOverridePersonality = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 6, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.InstClosePersonality = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 7, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.InstBlockPersonality = (tmp_int != 0);

                everythingOK &=
                    (SQLGetData(stmt, 8, SQL_INTEGER, &tmp_int, sizeof(tmp_int), NULL) == SQL_SUCCESS);
                if (everythingOK)
                    record.GFOnlyPersonality = (tmp_int != 0);

                // if we couldn't get all the columns for some reason, i think it is better
                // to just return a blank record
                if (!everythingOK)
                    record = {0};

                retval = true;
            }

            return retval;
        }

        bool Write(const PersonalityRecord &record)
        {

            SQLHSTMT stmt = NULL;
            bool retval = false;

            if (!_connected)
                return false;

            if (record.ID == 0 || record.SerialNum.empty())
                return false;

            std::string sql_query_string =
                "UPDATE AutoTestResults SET "
                "[LT PU On/Off Personalitity] = " +
                bool_to_zero_or_one(record.LTPUOnOffPersonalitity) + ", " +
                "[LT PU to 120% Personality] = " + bool_to_zero_or_one(record.LTPUto120PercentPersonality) + ", "
                                                                                                             "[LT Dly to 50 sec Personality] = " +
                bool_to_zero_or_one(record.LTDlyto50secPersonality) + ", "
                                                                      "[Inst Override Personality] = " +
                bool_to_zero_or_one(record.InstOverridePersonality) + ", "
                                                                      "[Inst Close Personality] = " +
                bool_to_zero_or_one(record.InstClosePersonality) + ", " +
                "[Inst Block Personality] = " + bool_to_zero_or_one(record.InstBlockPersonality) + ", " +
                "[GF Only Personality] = " + bool_to_zero_or_one(record.GFOnlyPersonality) +
                " WHERE ID = " + std::to_string(record.ID);

            // Allocate a statement handle
            SQLAllocHandle(SQL_HANDLE_STMT, dbc, &stmt);

            // Execute the query
            retval = (SQLExecDirect(stmt, (SQLCHAR *)sql_query_string.c_str(), SQL_NTS) == SQL_SUCCESS);
        }

    }

}
