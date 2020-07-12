package org.mixim.modules.mac;
 //* file:         BLEMacV2.ned
 // *
 // * Created on:  08.3.2014
 // * Updated on:  08.3.2014
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

import org.mixim.base.modules.BaseMacLayer;


simple BLEMacV2 extends BaseMacLayer
{
    parameters:
		@class(BLEMacV2);
        // debug switch
        bool debug = default(false);
        bool stats = default(true);
        bool trace = default(false);

        double bitrate @unit(bps) = default(1000000 bps);        
        // length of MAC header - not really used by BLE (BLE uses llDataHeaderLengthBits instead), but this one is required not to get the errors from underlying modules
        headerLength @unit(bit) = default(72 bit);          
          
        //desired tx power [mW]
        double txPower @unit(mW) = default(1mW); //i.e., 0 dbm
		int Initial_State = default(0) ;
        
//BLE standard stuff        
        int llDataHeaderLengthBits @unit(bit)= default(16 bit);//according to the standard - see p. 2511 of spec v4.1
        int llAdvHeaderLengthBits @unit(bit)= default(16 bit);//according to the standard - 
        double llIFSDeviation @unit(s) = default(0.000002 s);//according to the standard - see p. 2524 of spec v4.1
        double llIFS @unit(s) = default(0.000150 s);//duration of IFS - according to the standard 
        double llmaxAdvPDUduration @unit(s) = default(0.000376 s);//according to the standard (i.e., 1byte preamble + 4 byte access address + 2 byte data header + 37 bytes payload + 3 bytes CRC)
        double llmaxDataPDUduration@unit(s) = default(0.000328 s);//according to the standard (i.e., 1byte preamble + 4 byte access address + 2 byte data header + 27 bytes payload + 4 bytes NESN + 3 bytes CRC)
        double llminDataPDUduration@unit(s) = default(0.000088 s);//according to the standard (i.e., 1byte preamble + 4 byte access address + 2 byte data header + 1 bytes payload + 3 bytes CRC        
        int llmaxDataPDUPayloadBytes @unit(byte)= default(27 byte);//according to the standard 
        
//HW stuff required for correct timing        
		//Switching times
        double Time_llSLEEPtoTX @unit(s) = default(0.0 s);//
        double Time_llSLEEPtoRX @unit(s) = default(0.0 s);//
        double Time_llTXtoRX@unit(s) = default(0.000150 s);//should be less or equal to IFS (otherwise MIGHT not work)
        double Time_llRXtoTX@unit(s) = default(0.000150 s);//should be less or equal to IFS (otherwise MIGHT not work)
        double Time_llTXtoSLEEP@unit(s) = default(0.0 s);//
        double Time_llRXtoSLEEP@unit(s) = default(0.0 s);//
        		         
//CONNECTION    
	//for initialization
		int transmitWindowOffset = default(0); //in 1.25ms units, range [0, connInterval], see p. 2540 of spec v4.1
		int transmitWindowSize = default(1);//in 1.25ms units, range [1.25 ms, min(10 ms, (connInterval-1.25ms))], see p. 2540 of spec v4.1
		
	//for connection    
		int advertisingAddr = default(-1);
		int connAccessAddress = default(777);
		int connStartingChannel = default(0);// 
		int nodeSCA = default(0);

		string slave_beacon_ReplyPolicy = default("ondata");//defines the policy of the slave's reply in case if there is no 
															//MD set in a beacon and if slave has no data to TX 
															//Can be one of: "ondata","always"		
        
//TESTING and debug parameters
        int Init_DataQueryLgth @unit(byte)= default(0 byte);//initial nuber of bytes in the LL data buffer
        bool TST_stopSimulation_ConnectionBreak = default(false);   
        bool TST_NoRandomAdvInt	= default(false);//true = do not use random component of AdvInterval  
        int	 TST_ForceHop =	default(-1);//force specific hop value  
        
        
        bool TST_CollisionMonitoringMechanism = default(false);//true - use connection monitoring mechanism
        string TST_CMM_DetectionPolicy	= default("DropTimeInRangeRelative");
        int TST_CMM_NumPoints = default(3);
        double TST_CMM_Par1 = default(0.5);
        double TST_CMM_Par2 = default(0.1);
        double TST_CMM_Offset_Const_Relative = default(1);
        double TST_CMM_Offset_Random_Relative = default(0);
        
//HW params
		int macBufferSize @unit(byte) = default(1000 byte);       
        
}

