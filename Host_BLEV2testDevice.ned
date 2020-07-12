package org.mixim.modules.node;

 //* file:         Host_BLE.ned
 // *
 // * Created on:  13.2.2014
 // * Updated on:  13.2.2014
 // * author:      Konstantin Mikhaylov
 // *
 // * copyright:   (C) 2014 CWC, University of Oulu, Finland
 // *
 // *              This program is free software; you can redistribute it
 // *              and/or modify it under the terms of the GNU General Public
 // *              License as published by the Free Software Foundation; either
 // *              version 2 of the License, or (at your option) any later
 // *              version.
 // ***************************************************************************
 // *
 // **************************************************************************/

import org.mixim.modules.utility.Stat_ConsumptionChart;
import org.mixim.modules.utility.Stat_FrequencyHops;
import org.mixim.modules.utility.Stat_RadioState;
import org.mixim.modules.utility.Stat_QuerryData;
import org.mixim.modules.utility.Stat_BLEConnection;

module Host_BLEV2testDevice extends Host_BLEV2
{
    parameters:
        bool LogCurrent;
        bool LogFrequency;
        bool LogRadioState;
        bool LogQuerryLgth;
        bool LogBLEConnection;

        @display("bgb=287,520");
        //@display("bgb=100,520");
       
    submodules:
        nodeStat_Consumption: Stat_ConsumptionChart {
            Log = LogCurrent;
        }
        nodeStat_Frequency: Stat_FrequencyHops {
            Log = LogFrequency;
        }
        nodeStat_RadioState: Stat_RadioState {
            Log = LogRadioState;
        }
        nodeStat_QuerryLength: Stat_QuerryData {
            Log = LogQuerryLgth;
        }
        nodeStat_BLEConnection: Stat_BLEConnection {
            Log = LogBLEConnection;
        }
}
