package org.mixim.modules.node;

 //* file:         Host_BLE.ned
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
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
