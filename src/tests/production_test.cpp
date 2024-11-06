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

enum productionStates
{
    PRODUCTION_STATE_WAIT_FOR_TRIP_UNIT,
    PRODUCTION_STATE_DONE
};

void Production_State_Machine(HANDLE hATB, HANDLE hTripUnit, HANDLE hKeithley)
{

    productionStates productionState = PRODUCTION_STATE_WAIT_FOR_TRIP_UNIT;

    switch (productionState)
    {
    case PRODUCTION_STATE_WAIT_FOR_TRIP_UNIT:
        break;

    case PRODUCTION_STATE_DONE:
        // do nothing
        break;
    }

    // must somehow proceed through all the steps of the production test
}
