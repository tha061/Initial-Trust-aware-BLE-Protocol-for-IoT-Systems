/* -*- mode:c++ -*- ********************************************************
 // * file:        BLEMacV2.cc
 // *
 // * Created on:  08.3.2014
 // * Updated on:  02.8.2014
 // * author:      Konstantin Mikhaylov
 // *
 // * copyright:   (C) 2014 CWC, University of Oulu, Finland & Konstantin Mikhaylov
 // *
 // *              This program is free software; you can redistribute it
 // *              and/or modify it under the terms of the GNU General Public
 // *              License as published by the Free Software Foundation; either
 // *              version 2 of the License, or (at your option) any later
 // *              version.
 // ***************************************************************************
 // *NOTES/FEATURES:
  * 1)only ADV_DIRECT_IND_LDC are supported
  * 2)Update parameters packets always have higher priority
  * 3)if connSlaveLatency is nonzero results might be unapropriate!
  * 4)the advertiser is in RX up to switching to next channel (after listening at the last one - switches to sleep)
  * 5)command complete events are not returned to host!
  *
  * *IDEAS:
  * 1)Check effect of random delay generation mechanism for T_advEvent par in BLE spec (see p. 2528)
  * i.e., number of steps
  *
  *
  *  *TODO:
  *  1)Scanning state & other advertisements
  *  2)extend HCI & upper layers interface
  *  ...
  *
  *  "When a slave receives an LL_CONNECTION_UPDATE_REQ PDU where
    (Instant – connEventCount) modulo 65536 is less than 32767 and Instant is not
    equal to connEventCount, the slave shall listen to all the connection events
    until it has confirmation that the master has received its acknowledgement of
    the LL_CONNECTION_UPDATE_REQ PDU or connEventCount equals
    Instant. The slave shall also listen to the connection event where
    connEventCount equals Instant and the connection event before it. " - p. 2550 of spec v 4.1 - not implemented for now
  *
  *
  *
 // **************************************************************************/

/* Added by Tham April 2018
 * Implement the ADV_IND packet to make sure that the advertising packet also contain data.
 * At the moment, the implementation only for the ADV_DIRECT_IND which does not contain the Host's data of the advertiser
 * Implement the scanning state: active scanning mode to use the SCAN_REQ and SCAN_RSP packets for more pair of interactions
 * between two BLE devices
 * Do not implement the security protocol as all the packet exchanging in the pairing and authentication procedure
 * has fixed format without any space for Host's data
 * The procedure of exchange the challenge and response as below:
 *
 * 1) The Controller can use the ADV_IND packet to advertise its challenge c1
 * 2) The BLE device can use the ADV_IND packet to advertise its present and also include the response to the challenge c1
 * 3) The controller recived the ADV_IND with address of the ble device and the response,
 * will send a SCAN_REQ to the ble device include its challenge c2. However, as specification of BLE : SCAN_REQ packet does not
 * contain the Host's data of the initiator. How to add data to this packet
 * 4) the device that has its address in the SCAN_REQ packet will continue to answer by sending SCAN_RSP with a response to c2
 * 5) The controller can send more SCAN_REQ packet to the target packet if it has more challenges
 * 6) the device will response with more SCAN_RSP to the controller to provide its responses
 *
 * The process will stop when computed trust value at the controller meets the thresholds
 * Or the process will stop after a challenge-response process timer is expired
 * The simulation outputs is to see if the controller will send the CONNECT_REQ to the device under the challenge-response or not
 * After sending the CONNECT_REQ, the controller admits the device to the Personal Space and will do the security process
 * with pairing and authentication as normal.
 * Our demonstration is only showing that the challenge-response process is realistic and it can be done before the
 * normal pairing and authentication of BLE devices
 */
//=================================================================================//
#include "BLEMacV2.h"
//#include "BLEstructs.h"
#include "BLE_NwkToMac.h"
#include "BLE_MacToNwk.h"
#include "BLE_LLControlInfo.h"

#include "BLE_BasicNwk.h" // Tham added

#include <cassert>

#include "FWMath.h"
#include "BaseDecider.h"
#include "BaseArp.h"
#include "BasePhyLayer.h"
#include "BaseConnectionManager.h"
#include "FindModule.h"

#include "FWMath.h"
//#include "MacPkt_m.h"

#include "BLE_Adv_MacPkt_m.h"
#include "BLE_Data_MacPkt_m.h"
#include "BLE_HCI_DataPkt_m.h"

#include "MiXiMDefs.h"

//LOGG stuff
#include "Stat_FrequencyHops.h"
#include "Stat_RadioState.h"
#include "Stat_QuerryData.h"
#include "Stat_BLEConnection.h"
//#include "BLETestNetworkStatistics.h"

#include <iostream>
#include <string>
#include <sstream>
#include <fstream> // stream class to both read and write from/to files
#include <time.h>
#include <stdlib.h>
using namespace std;


Define_Module(BLEMacV2);


void BLEMacV2::initialize(int stage) {
    BaseMacLayer::initialize(stage);


    // Tham added to initiate the statistic data
    num_scan_req=0;
    num_scan_rsp=0;
    num_scan_req_test=0;
    num_scan_rsp_test=0;
    num_adv_ind=0;
    num_scan_req_test_in_attachsignal=0;
    num_scan_rsp_test_in_attachsignal=0;
    num_adv_ind_test_in_attachsignal=0;

    num_scan_req_rcved=0;
    num_scan_rsp_rcved=0;
    num_adv_ind_rcved=0;

    num_data_pkt_sent=0;
    num_data_pkt_rcved=0;
    min_data_rounds = 100;

   // for trust value
    index_T = -1;
    temp_c_index = 0;
    //temp_c_index[0]=4;
   // temp_T = 0.0;
    temp_ch = 0;
    temp_res = 0;
    data_req = 0.0; // generate scan_req
    data_rsp = 0; // generate scan_rsp

    // number of C_R rounds at discovery phase and connected before pairing phase

    input_rsp = 4;
    input_data_round = 2;

    // for challenge-response randomly selected
    srand (time(NULL));

    //DEBUG stuff
    debug_Timers=true;
    debug_Messages=true;
    debug_StateTransitions=true;
    debug_Pkt_Basic=true;
    debug_Internal_Messages=true;
    debug_ChannelSwitching=true;
    debug_InitPars=true;
    debug_UpperCommands=true;
    info_events = true;


    if (stage == 0){
      //  ev << "Tham - stage = 0" << endl;
        txPower = par("txPower").doubleValue();
        bitrate = par("bitrate");
        // initialize the timers
        advertisementEventTimer = new cMessage("timer-AdvertisementEvent");

        advertisementNextPDUTimer = new cMessage("timer-AdvertisementNextPDU");

        advertisementEndEvent = new cMessage("timer-AdvertisementEndEvent");

        initiatingScanIntervalTimer = new cMessage("timer-InitiatingScanIntervalTimer");

        initiatingScanWindowTimer = new cMessage("timer-InitiatingScanWindowTimer");

        slaveConnectionWakeupTimer = new cMessage("timer-SlaveConnectionWakeupTimer");
        slaveConnectionNoBeaconTimer = new cMessage("timer-SlaveConnectionNoBeaconTimer");
        slaveIFSTimer = new cMessage("timer-SlaveIFSTimer");//wait IFS before sending reply
        slaveWaitReplyTimer = new cMessage("timer-SlaveWaitReplyTimer");//wait reply from master
        slaveEndConnectionEvent = new cMessage("SlaveEndConnectionEvent");//
        slaveConnectionSupervisionTimer = new cMessage("timer-SlaveConnectionSupervisionTimer");
        slaveConnectionWakeupConnectionTimer = new cMessage("timer-SlaveConnectionWakeupConnectionTimer");
        slaveConnectionTransmitWindowTimer = new cMessage("timer-SlaveConnectionTransmitWindowTimer");

        masterConnectionWakeupTimer = new cMessage("timer-MasterConnectionWakeupTimer");
        masterWaitReplyTimer = new cMessage("timer-MasterWaitReplyTimer");
        masterIFSTimer = new cMessage("timer-MasterIFSTimer");//wait IFS before sending reply
        masterEndConnectionEvent = new cMessage("masterEndConnectionEvent");//
        masterConnectionSupervisionTimer = new cMessage("timer-masterConnectionSupervisionTimer");

        ctrl_switchState = new cMessage("event-SwitchState");
        ctrl_terminateConnection = new cMessage("event-TerminateConnection");

        //TST_switchoffTimer = new cMessage("timer-TST_switchoffTimer");

        llAdvHeaderLengthBits=par("llAdvHeaderLengthBits");
        llDataHeaderLengthBits=par("llDataHeaderLengthBits");
        llIFSDeviation=par("llIFSDeviation");
        llIFS=par("llIFS");
        llmaxAdvPDUduration=par("llmaxAdvPDUduration");
        llmaxDataPDUduration=par("llmaxDataPDUduration");
        llminDataPDUduration=par("llminDataPDUduration");
        llmaxDataPDUPayloadBytes=par("llmaxDataPDUPayloadBytes");

        //switch times
        Time_llSLEEPtoTX=par("Time_llSLEEPtoTX");
        Time_llSLEEPtoRX=par("Time_llSLEEPtoRX");
        Time_llTXtoRX=par("Time_llTXtoRX");
        Time_llRXtoTX=par("Time_llRXtoTX");
        Time_llTXtoSLEEP=par("Time_llTXtoSLEEP");
        Time_llRXtoSLEEP=par("Time_llRXtoSLEEP");

      //  if(debug_InitPars)
        EV << "BLEMacV2::initialize Time_llSLEEPtoTX=" << Time_llSLEEPtoTX << "s Time_llSLEEPtoRX=" << Time_llSLEEPtoRX <<
               "s Time_llTXtoRX=" << Time_llTXtoRX << "s Time_llRXtoTX=" << Time_llRXtoTX << "s Time_llTXtoSLEEP=" << Time_llTXtoSLEEP <<
               "s Time_llRXtoSLEEP=" << Time_llRXtoSLEEP << endl;

        TST_DataQueryLgth=par("Init_DataQueryLgth");
        TST_NoAdvIntRandomComponent=par("TST_NoRandomAdvInt");
        TST_forceHop=par("TST_ForceHop");

        //ConnectionTermination=false;
        myEventData=new cBLEstructs_cEventsData();
        myCmdError=new cBLEstructs_Error();

    }

    else if(stage == 1) {
       // ev << "Tham - stage = 1" << endl;
        BaseConnectionManager* cc = getConnectionManager();
        mySCA = par("nodeSCA");
        //DEBUG stuff
        updateMacState(STANDBY_1);

        //connection
        const char *str_beacon_ReplyPolicy = par("slave_beacon_ReplyPolicy");
        if (!strcmp(str_beacon_ReplyPolicy, "ondata"))llSlave_beacon_ReplyPolicy=IF_HAVE_DATA_2;
        else if(!strcmp(str_beacon_ReplyPolicy, "always"))llSlave_beacon_ReplyPolicy=ALWAYS_1;
        else error("BLEMacV2::initialize stage 1: unknown llSlave_noMD_ReplyPolicy");

        //set testing mode & params
        int initState = par("Initial_State");

        DEBUG_PKT_SEQ_ID=0;

        int myAddr=par("advertisingAddr");
        int myIdx=FindModule<>::findHost(this)->getIndex();
        if(myAddr<0)myAddr=myIdx;
        myAdvrtPars.OwnAdvAddr=(LAddress::L2Type)(myAddr);


        int value;
        value = par("connStartingChannel");
        if(value<0) value = FindModule<>::findHost(this)->getParentModule()->getSubmodule("master",myIdx)->getSubmodule("nic")->getSubmodule("mac")->par("connStartingChannel");
        myConnPars.ConnInfo.unmappedChannel=value;

        myConnPars.ConnInfo.transmitWindowOffset=par("transmitWindowOffset");
        if(myConnPars.ConnInfo.transmitWindowOffset>myConnPars.ConnInfo.connInterval) error("BLEMacV2::initialize - according to the standard, transmitWindowOffset should be less or equal to connInterval. Please change parameters accordingly!");//see p. 2540 for spec v4.1
        myConnPars.ConnInfo.transmitWindowSize=par("transmitWindowSize");
        if((myConnPars.ConnInfo.transmitWindowSize<1)||(myConnPars.ConnInfo.transmitWindowSize>(std::min(8,myConnPars.ConnInfo.connInterval-1))))//see p. 2540 for spec v4.1
            error("BLEMacV2::initialize - according to the standard, transmitWindowSize should be in the range of [1;min(8,ConnInfo.connInterval-1)]. Please change parameters accordingly!");


        DataPkt = new BLE_Data_MacPkt("DATA_PDU"); // data
        HighPriorityDataPkt = new BLE_Data_MacPkt("HIGH_PRIORITY_DATA_PDU"); // high priority data
        AdvMessage = new BLE_Adv_MacPkt("ADV_PDU"); // advertisement

        Buf_TotalSize=par("macBufferSize");
        Buf_FreeSize=Buf_TotalSize;
        PktLeft_TX_Size=0;
        PktLeft_RX_Size=0;
        LastPacketPayload=0;
        DataPkt_FirstFragment=true;
        DataPkt_FirstFragmentSent=false;

       // if(debug_InitPars)
        EV << "BLEMacV2::initialize Buffer Size =" << Buf_TotalSize << "bytes, Free " << Buf_FreeSize << "bytes" << endl;



        //General network statistics & simulation stop
       /*BLEnwkStats=FindModule<BLETestNetworkStatistics*>::findGlobalModule();
        if(BLEnwkStats){
            if(TST_DataQueryLgth) BLEnwkStats->change_NumBytes(TST_DataQueryLgth);
        }
        else{
            error("BLEMacV2::initialize - BLEnwkStats not found!");
        }*/

        //NOTE: not really tested for v2

      ev << "Tham check initStage =" << initState << endl;

        if(initState==1){//act as advertiser
            updateMacState(ADVERTISING_2);
            updateMacSubstate(UNUSED);
            initVariablesAtStateStart();
            //startTimer(TIMER_ADVERTISE_EVENTSTART);
        }
        else if(initState==2){//act as initiator
            updateMacState(INITIATING_4);
            updateMacSubstate(UNUSED);
            initVariablesAtStateStart();
            //startTimer(TIMER_INITIATING_INTERVAL);
        }
        else if(initState==3){ //act as a slave (starting from CONN_REQUEST)
            //startTimer(TIMER_CONNECTION_SLAVE_WAKEUP);
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_SLV_WAITCONNECTION);
            //updateMacSubstate(CONNECTED_SLV_SLEEP);
            initVariablesAtStateStart();
            prepareNewDataPkt(1);//WILL NOT WORK OTHERWISE
        }
        else if(initState==4){//act as a master (starting from CONN_REQUEST)
            //startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_MST_SLEEP);
            //updateMacSubstate(CONNECTED_MST_SLEEP);
            initVariablesAtStateStart();
           // prepareNewDataPkt(1);//WILL NOT WORK OTHERWISE
            prepareNewDataPkt_Master(1); // added by Tham
        }
        else if(initState==5){//act as a slave (already established)
            //startTimer(TIMER_CONNECTION_SLAVE_WAKEUP);
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_SLV_SLEEP);
            myConnPars.ConnInfo.numMissedEvents=0;
            myConnPars.ConnInfo.lastunmappedChannel=myConnPars.ConnInfo.unmappedChannel;
            myConnPars.ConnInfo.transmitSeqNum=false;
            myConnPars.ConnInfo.nextExpectedSeqNum=false;
            myConnPars.ConnInfo.delayedGeneration=true;
            myConnPars.ConnInfo.moreData=false;
            startTimer(TIMER_CONNECTION_SLAVE_WAKEUP);
            startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION);
            myConnPars.ConnInfo.timeLastAnchorReceived=0;
            prepareNewDataPkt(1);//WILL NOT WORK OTHERWISE
            eventConnectionCompleted();
        }
        else if(initState==6){//act as a master (already established)
            //startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_MST_SLEEP);
            myConnPars.ConnInfo.numMissedEvents=0;
            myConnPars.ConnInfo.lastunmappedChannel=myConnPars.ConnInfo.unmappedChannel;
            myConnPars.ConnInfo.transmitSeqNum=false;
            myConnPars.ConnInfo.nextExpectedSeqNum=false;
            myConnPars.ConnInfo.delayedGeneration=true;
            myConnPars.ConnInfo.moreData=false;
            myConnPars.ConnInfo.timeLastSuccesfullEvent=0;
            startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
            startTimer(TIMER_CONNECTION_MASTER_SUPERVISION);
          //  prepareNewDataPkt(1);//WILL NOT WORK OTHERWISE
            prepareNewDataPkt_Master(1); // added by Tham
            eventConnectionCompleted();
        }
        else if(initState==0){//
        }
        else{
            error("BLEMacV2::initialize - unknown initState!");
        }

        Flag_DelayedSwitchOff=false;
        //Flag_DataPktPending=false;
        //logg data
        /*recordScalar("Info_settings_connInterval", myConnPars.ConnInfo.connInterval, "");
        recordScalar("Info_settings_hopIncreasement", myConnPars.ConnInfo.hopIncreasement, "");
        recordScalar("Info_settings_startchannel", myConnPars.ConnInfo.unmappedChannel, "");
        recordScalar("Info_settings_AdvertisingIntervalMax", myAdvrtPars.Advertising_Interval_Max, "");
        recordScalar("Info_settings_AdvertisingIntervalMin", myAdvrtPars.Advertising_Interval_Min, "");
        recordScalar("Info_settings_DataQueryLgth", TST_DataQueryLgth, "");
        recordScalar("Info_settings_TxPower", txPower, "");*/

        //if(info_events)eventRadioStateChanged();
        TST_OverlapMonitoringMechanism=par("TST_CollisionMonitoringMechanism");
        if(TST_OverlapMonitoringMechanism){
            TST_CMM_NumPoints=par("TST_CMM_NumPoints");
            TST_CMM_Par1=par("TST_CMM_Par1");
            TST_CMM_Par2=par("TST_CMM_Par2");
            TST_CMM_Offset_Const=par("TST_CMM_Offset_Const_Relative");
            TST_CMM_Offset_Rand=par("TST_CMM_Offset_Random_Relative");
        }
    } // end if stage == 1


}

//=========================================================================
// Randomly choose challenge and response from the probability distribution

// Utility function to find ceiling of r in arr[l..h]
int findCeil(int arr[], int r, int l, int h)
{
    int mid;
    while (l < h)
    {
        mid = l + ((h - l) >> 1); // Same as mid = (l+h)/2
        (r > arr[mid]) ? (l = mid + 1) : (h = mid);
    }
    return (arr[l] >= r) ? l : -1;
}

// The main function that returns a random number from arr[] according to
// distribution array defined by freq[]. n is size of arrays.
int myRand(int arr[], int freq[], int n)
{
    // Create and fill prefix array
    int prefix[n], i;
    prefix[0] = freq[0];
    for (i = 1; i < n; ++i)
        prefix[i] = prefix[i - 1] + freq[i];

    // prefix[n-1] is sum of all frequencies. Generate a random number
    // with value from 1 to this sum
    int r = (rand() % prefix[n - 1]) + 1;

    // Find index of ceiling of r in prefix arrat
    int indexc = findCeil(prefix, r, 0, n - 1);
    return arr[indexc];
}

//===========================================================================


void BLEMacV2::finish() {

    if(info_events)eventRadioStateChanged();

    //get rid of all the timers & messages
    cancelAndDelete(advertisementEventTimer);
    cancelAndDelete(advertisementNextPDUTimer);
    cancelAndDelete(advertisementEndEvent);

    cancelAndDelete(initiatingScanIntervalTimer);
    cancelAndDelete(initiatingScanWindowTimer);

    cancelAndDelete(slaveConnectionWakeupTimer);
    cancelAndDelete(slaveConnectionNoBeaconTimer);
    cancelAndDelete(slaveIFSTimer);
    cancelAndDelete(slaveWaitReplyTimer);
    cancelAndDelete(slaveEndConnectionEvent);
    cancelAndDelete(slaveConnectionSupervisionTimer);
    cancelAndDelete(slaveConnectionWakeupConnectionTimer);
    cancelAndDelete(slaveConnectionTransmitWindowTimer);

    cancelAndDelete(masterConnectionWakeupTimer);
    cancelAndDelete(masterWaitReplyTimer);
    cancelAndDelete(masterIFSTimer);
    cancelAndDelete(masterEndConnectionEvent);
    cancelAndDelete(masterConnectionSupervisionTimer);

    cancelAndDelete(ctrl_switchState);
    cancelAndDelete(ctrl_terminateConnection);

    delete DataPkt;
    delete HighPriorityDataPkt;
    delete AdvMessage;

    //if(DataPktForNWK)delete DataPktForNWK;
    //delete ConRQSTMessage;

       EV << "Simulation end!" << endl;
       EV << "Number of ADV_IND packet sent = " << num_adv_ind << endl;
       EV << "Number of SCAN_REQ packet sent = " << num_scan_req << endl;
       EV << "Number of SCAN_RSP packet sent = " << num_scan_rsp << endl;

       EV << "Number of SCAN_REQ packet test sent = " << num_scan_req_test << endl;
       EV << "Number of SCAN_RSP packet test sent = " << num_scan_rsp_test << endl;

       EV << "Number of SCAN_REQ packet test in attachsignal sent = " << num_scan_req_test_in_attachsignal << endl;
       EV << "Number of SCAN_RSP packet test in attachsignal sent = " << num_scan_rsp_test_in_attachsignal << endl;
       EV << "Number of ADV_IND packet test in attachsignal sent = " << num_adv_ind_test_in_attachsignal << endl;
       EV << "-------------------------------"<< endl;


       EV << "Number of ADV_IND packet received = " << num_adv_ind_rcved << endl;
       EV << "Number of SCAN_REQ packet received = " << num_scan_req_rcved << endl;
       EV << "Number of SCAN_RSP packet received = " << num_scan_rsp_rcved << endl;
       EV << "===============================" << endl;

       EV << "Number of DATA packet sent = " << num_data_pkt_sent << endl;
       EV << "Number of DATA packet received = " << num_data_pkt_rcved << endl;
       EV << "===============================" << endl;


       int rounds = input_rsp +  input_data_round;
       // instant trust value after each round
       for (int i = 0; i<rounds; i++){
       EV << "Trust value at round [" << i+1 <<"]= " << T_inst[i] << endl;
          }
       // Initial trust updated over rounds
       for (int i = 0; i<rounds; i++){
          EV << "Initial Trust value over " << i+1 << " rounds = "<< T_init[i] << endl;
          }

     // Challenge-response
       EV << "When finish : index_T = " << index_T << endl;


       for (int i = 0; i < rounds; i++){
          EV << " Round [" << i+1 << "] Challenge = C_" << challenge_list[i] << " and ";
          EV << "Response = R_" << response_list[i] << endl;
      }
      // Device trust database

       for (int k = 0; k < 3 ; k++) {
           EV << "Device Address = " << Address_device[k] << " has Trust value = " << Trust_device[k] <<endl;
       }
      // t_cost = t_end - t_begin;
      // t_cost_dis = t_end_dis - t_begin;
      // EV <<"Time cost =" << t_cost << endl;
       t_connected = t_end - t_end_dis;
       EV <<"Time cost in discovery =" << t_end_dis << endl;
       EV <<"Time cost in connected =" << t_connected << endl;
       EV <<"Total Time cost =" << t_end << endl;
      // EV << "prob_r_on_c[2][3] = " << prob_r_on_c[2][3] << endl;
       //EV << "prob_r_on_c[0][1] = " << prob_r_on_c[0][1] << endl;
       //EV << "prob_r_on_c[1][2] = " << prob_r_on_c[1][2] << endl;


   //int Role=par("NWK_DEBUG_ROLE");
   //int Role;
   /*
   if (Node_role==1){ // slave
           EV << "Slave - Simulation end!" << endl;
           EV << "Number of ADV_IND packet sent = " << num_adv_ind << endl;
          // EV << "Number of SCAN_REQ packet sent = " << num_scan_req << endl;
           EV << "Number of SCAN_RSP packet sent = " << num_scan_rsp << endl;
           EV << "-------------------------------"<< endl;
          // EV << "Number of ADV_IND packet received = " << num_adv_ind_rcved << endl;
           EV << "Number of SCAN_REQ packet received = " << num_scan_req_rcved << endl;
          // EV << "Number of SCAN_RSP packet received = " << num_scan_rsp_rcved << endl;
           EV << "===============================" << endl;
           EV << "Number of DATA packet sent = " << num_data_pkt_sent << endl;
           EV << "Number of DATA packet received = " << num_data_pkt_rcved << endl;
           EV << "===============================" << endl;
          / for (int i = 0; i<5; i++){
           EV << "Trust value at round[" << i+1 <<"]= " << T_inst[i] << endl;
              }
           for (int i = 0; i<5; i++){
              EV << "Initial Trust value over " << i+1 << " rounds = "<< T_init[i] << endl;
              }
              /
   }
   else if (Node_role==2){ //master
          EV << "Master - Simulation end!" << endl;
          //EV << "Number of ADV_IND packet sent = " << num_adv_ind << endl;
          EV << "Number of SCAN_REQ packet sent = " << num_scan_req << endl;
         // EV << "Number of SCAN_RSP packet sent = " << num_scan_rsp << endl;
          EV << "-------------------------------"<< endl;
          EV << "Number of ADV_IND packet received = " << num_adv_ind_rcved << endl;
          //EV << "Number of SCAN_REQ packet received = " << num_scan_req_rcved << endl;
          EV << "Number of SCAN_RSP packet received = " << num_scan_rsp_rcved << endl;
          EV << "===============================" << endl;
          EV << "Number of DATA packet sent = " << num_data_pkt_sent << endl;
          EV << "Number of DATA packet received = " << num_data_pkt_rcved << endl;
          EV << "===============================" << endl;
          for (int i = 0; i<5; i++){
          EV << "Trust value at round[" << i+1 <<"]= " << T_inst[i] << endl;
             }
          for (int i = 0; i<5; i++){
             EV << "Initial Trust value over " << i+1 << " rounds = "<< T_init[i] << endl;
             }
   }
   */
}

BLEMacV2::~BLEMacV2() {
    MacQueue::iterator it;
    for(it = macQueue.begin(); it != macQueue.end(); ++it){
        delete (*it);
    }


    macQueue.clear();
}


void BLEMacV2::handleUpperMsg(cMessage *msg) {
    cObject*  cInfo = msg->removeControlInfo();

    if (cInfo == NULL) {
        error("BLEMacV2::handleUpperMsg ControlInfo is missing!");
    }
    else{
        cPacket* pkt = static_cast<cPacket*>(msg);
        long int handle = BLE_LL_ControlInfo::getAddressFromControlInfo(cInfo);
        delete cInfo;

       // if(debug_Internal_Messages){
            EV << "BLEMacV2::handleUpperMsg: packet handle=" << handle << " ,length=" << pkt->getByteLength() << " ,PktLeft_TX_Size=" << PktLeft_TX_Size << " bytes"<< endl;
       // }

        if(Buf_FreeSize > pkt->getByteLength()){//buffer can handle the packet
            macQueue.push_back(pkt);
            //Buf_FreeSize=Buf_FreeSize-pkt->getByteLength();
            if(PktLeft_TX_Size==0)PktLeft_TX_Size=pkt->getByteLength();
            //BLEnwkStats->change_NumBytes(pkt->getByteLength());
            eventQuerryChange(pkt->getByteLength());//update querry
        }
        else{
            //TODO
            error("BLEMacV2::handleUpperMsg Data Buffer Overflow is currently NOT IMPLEMENTED!  Here Tham checked");
        }
    }
}

// Added actions for STANDBY_to_INITIATING
void BLEMacV2::handleSelfMsg(cMessage *msg) {
//advertiser
    ev <<"Tham - BLEMacV2::handleSelfMsg: msg = " << *msg << endl;
    if(msg==advertisementEventTimer){
        ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == advertisementEventTimer" << endl;
        executeMac(EV_ADV_EVENT, msg);
    }
    else if(msg==advertisementNextPDUTimer){
        ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == advertisementNextPDUTimer" << endl;
        executeMac(EV_ADV_NEXTPDU, msg);
    }
    else if(msg==advertisementEndEvent){
      //  ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == advertisementEndEvent" << endl;
        executeMac(EV_ADV_EVENTEND, msg);
    }

//initiator
    else if(msg==initiatingScanIntervalTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == initiatingScanIntervalTimer" << endl;
        executeMac(EV_INIT_INTERVAL, msg);
    }
    else if(msg==initiatingScanWindowTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == initiatingScanWindowTimer" << endl;
        executeMac(EV_INIT_WINDOW, msg);
    }

//slave in connection
    else if(msg==slaveConnectionWakeupConnectionTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveConnectionWakeupConnectionTimer" << endl;
        executeMac(EV_CON_SLV_WAKEUP_CONNECTION, msg);
    }
    else if(msg==slaveConnectionTransmitWindowTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveConnectionTransmitWindowTimer" << endl;
        executeMac(EV_CON_SLV_TRANSMIT_WINDOW, msg);
    }
    else if(msg==slaveConnectionWakeupTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveConnectionWakeupTimer" << endl;
        executeMac(EV_CON_SLV_WAKEUP, msg);
    }
    else if(msg==slaveConnectionNoBeaconTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveConnectionNoBeaconTimer" << endl;
        executeMac(EV_CON_SLV_NOBEACON, msg);
    }
    else if(msg==slaveIFSTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveIFSTimer" << endl;
        executeMac(EV_CON_SLV_WAITIFS, msg);
    }
    else if(msg==slaveWaitReplyTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveWaitReplyTimer" << endl;
        executeMac(EV_CON_SLV_WAITREPLY, msg);
    }
    else if(msg==slaveEndConnectionEvent){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveEndConnectionEvent" << endl;
        executeMac(EV_CON_SLV_ENDEVENT, msg);
    }
    else if(msg==slaveConnectionSupervisionTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == slaveConnectionSupervisionTimer" << endl;
        executeMac(EV_CON_SLV_DROPCONNECTION, msg);
    }

 //master in connection
    else if(msg==masterConnectionWakeupTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == masterConnectionWakeupTimer" << endl;
        executeMac(EV_CON_MST_WAKEUP, msg);
    }
    else if(msg==masterWaitReplyTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == masterWaitReplyTimer" << endl;
        executeMac(EV_CON_MST_WAITREPLY, msg);
    }
    else if(msg==masterIFSTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == masterIFSTimer" << endl;
        executeMac(EV_CON_MST_WAITIFS, msg);
    }
    else if(msg==masterEndConnectionEvent){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == masterEndConnectionEvent" << endl;
        executeMac(EV_CON_MST_ENDEVENT, msg);
    }
    else if(msg==masterConnectionSupervisionTimer){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == masterConnectionSupervisionTimer" << endl;
        executeMac(EV_CON_MST_DROPCONNECTION, msg);
    }

    //state switching stuff
    else if(msg==ctrl_switchState){
        //ev <<"Tham - BLEMacV2::handleSelfMsg: handle  msg == ctrl_switchState" << endl;

        stopAllTimers();

        if(msg->getKind()== INITIALIZING_to_CONNECTION){
            ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching INITIALIZING_to_CONNECTION" << endl;
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_MST_SLEEP);
            simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
            if(FreeTime==0) eventRadioStateChanged();
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
            initVariablesAtStateStart();
            eventConnectionCompleted();
        }
        else if(msg->getKind()== ADVERTIZING_to_CONNECTION){
            ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching ADVERTIZING_to_CONNECTION" << endl;
            updateMacState(CONNECTION_5);
            updateMacSubstate(CONNECTED_SLV_WAITCONNECTION);
            simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
            if(FreeTime==0) eventRadioStateChanged();
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
            initVariablesAtStateStart();
            eventConnectionCompleted();
        }
        else if(msg->getKind()== ANY_to_STANDBY){
           // ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching ANY_to_STANDBY" << endl;
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - ANY_to_STANDBY"  << endl;
            if(phy->getRadioState()==MiximRadio::SWITCHING){
                Flag_DelayedSwitchOff=true;
                if(debug_Internal_Messages)
                    EV << "BLEMacV2::updateStatusConnected - Flag_DelayedSwitchOff=true"  << endl;
            }
            else if(phy->getRadioState()!= MiximRadio::SLEEP){
                simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                if(FreeTime==0){
                    eventRadioStateChanged();
                    //inform host
                    myEventData->Connection_Handle=myConnPars.ConnInfo.connectionHandle;
                    myEventData->ErrorCode=myCmdError->ErrorCode;
                    cMessage *m;
                    m = new cMessage("BLE_MACtoNWK_EVENT");
                    m->setControlInfo(BLE_MacToNwk::generate_DisconnectionCompleteEvent(myEventData));
                    sendControlUp(m);
                    if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - ANY_to_STANDBY - informing upper layer"  << endl;
                    Flag_DelayedSwitchOff=false;
                    updateMacState(STANDBY_1);
                    updateMacSubstate(UNUSED);
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    initVariablesAtStateStart();

                }
                else Flag_DelayedSwitchOff=true;
                if(debug_Internal_Messages)
                    EV << "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                if(debug_Internal_Messages)
                    EV << "BLEMacV2::updateStatusConnected - Flag_DelayedSwitchOff=true" << endl;
            }
             // phy->getRadioState() == MiximRadio::SLEEP
            else{
                //inform host
                myEventData->Connection_Handle=myConnPars.ConnInfo.connectionHandle;
                myEventData->ErrorCode=myCmdError->ErrorCode;

                cMessage *m;
                m = new cMessage("BLE_MACtoNWK_EVENT");
                m->setControlInfo(BLE_MacToNwk::generate_DisconnectionCompleteEvent(myEventData));
                sendControlUp(m);

                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - ANY_to_STANDBY - informing upper layer"  << endl;

                Flag_DelayedSwitchOff=false;
                updateMacState(STANDBY_1);
                updateMacSubstate(UNUSED);

                simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0

                if(FreeTime==0) eventRadioStateChanged();

                if(debug_Internal_Messages)  EV << "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                initVariablesAtStateStart();
            }
        } // end of msg->getKind()== ANY_to_STANDBY
        else if(msg->getKind() == STANDBY_to_ADVERTIZING){
            ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching STANDBY_to_ADVERTIZING" << endl;
            updateMacState(ADVERTISING_2);
            updateMacSubstate(UNUSED);
            initVariablesAtStateStart();
        }
        else if(msg->getKind()==STANDBY_to_INITIATING){
            ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching STANDBY_to_INITIATING" << endl;
            updateMacState(INITIATING_4);
            updateMacSubstate(UNUSED);
            initVariablesAtStateStart();
        }
        else if (msg->getKind() == STANDBY_to_SCANNING){ // Tham added
            ev <<"Tham - BLEMacV2::handleSelfMsg: handle  state swithching STANDBY_to_SCANNING" << endl;
            updateMacState(SCANNING_3);
            updateMacSubstate(UNUSED);
            initVariablesAtStateStart();
        } // Tham added
    }
    else if(msg==ctrl_terminateConnection){//terminate connection
        dropConnection();
    }
    else error("BLEMacV2::handleSelfMsg unknown message");
}

void BLEMacV2::handleLowerMsg(cMessage *msg) {
    if(debug_Internal_Messages) EV << "BLEMacV2: RADIO message received" << endl;
    executeMac(EV_FRAME_RECEIVED, msg);
}

void BLEMacV2::handleLowerControl(cMessage *msg) {
    EV <<"BLEMacV2::handleLowerControl" << endl;
    if (msg->getKind() == MacToPhyInterface::TX_OVER) {
        executeMac(EV_FRAME_TRANSMITTED, msg);
    } else if (msg->getKind() == BaseDecider::PACKET_DROPPED) {
        if(debug_Internal_Messages) EV << "control message: PACKED DROPPED" << endl;
    } else if (msg->getKind() == MacToPhyInterface::RADIO_SWITCHING_OVER) {
        if(info_events)eventRadioStateChanged();
        if(Flag_DelayedSwitchOff==true){
            if(debug_Internal_Messages) EV << "BLEMacV2::handleLowerControl Flag_DelayedSwitchOff="<< Flag_DelayedSwitchOff<< endl;
            if(phy->getRadioState()!=MiximRadio::SLEEP){
                simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                if(FreeTime==0) eventRadioStateChanged();
                if(debug_Internal_Messages) EV << "BLEMacV2::handleLowerControl - FreeTime=" << FreeTime << endl;
            }
            else{
                ctrl_switchState->setKind(ANY_to_STANDBY);
                scheduleAt(simTime(), ctrl_switchState);
            }
        }
    } else {
        EV << "Invalid control message type (type=NOTHING) : name="
        << msg->getName() << " modulesrc="
        << msg->getSenderModule()->getFullPath()
        << "." << endl;
    }
    delete msg;
}

//original
/*
void BLEMacV2::handleUpperControl(cMessage *msg) {
    if(debug_Internal_Messages)
    EV << "BLEMacV2::handleUpperControl message name='" << msg->getName() << endl;
    if (strcmp(msg->getName(), "BLE_NWKtoMAC_CMD")){//command
        error("BLEMacV2: received unsupported command from upper layer!");
    }
    else{
        cObject* cInfo = msg->removeControlInfo();
        BLE_NwkToMac *const cCmdInfo = dynamic_cast<BLE_NwkToMac *const>(cInfo);
        if(debug_Internal_Messages)
        EV << "BLEMacV2:: command type " << cCmdInfo->get_CmdType() << endl;

        switch(cCmdInfo->get_CmdType()){
            //advertisement-related commands
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand:
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingDataCommand:
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertiseEnableCommand:
                handleUpperCommand_Adv(cCmdInfo);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Command:
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command:
                handleUpperCommand_Initiate(cCmdInfo);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Set_Host_Channel_Classification_Command:
                handleUpperCommand_updateDataChannelMap(&cCmdInfo->get_ConnectionParsPtr()->Data_Channel_Map);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Update_Command:
                handleUpperCommand_updateConnectionPars(cCmdInfo->get_ConnectionParsPtr());
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_Disconnect_Command:
                handleUpperCommand_disconnect(cCmdInfo->get_ErrorPtr());
            break;
            //NON-STANDARD COMMANDS
            case cBLEstructs_defs::Simulation_LLBufferPayloadUpdate_Command:
                handleUpperCommand_LLBufferPayloadUpdate(cCmdInfo->get_BufferPayloadUpdate());
            break;
            default:
               error("BLEMacV2: received unsupported command from upper layer!");
            break;
        }
    }
    delete msg;
}
*/

// Tham modified
void BLEMacV2::handleUpperControl(cMessage *msg) {
   // if(debug_Internal_Messages)
    EV << "BLEMacV2::handleUpperControl message name = " << msg->getName() << endl;
    EV <<"Tham - compare the msg name = " << strcmp(msg->getName(), "BLE_NWKtoMAC_CMD") << endl;
    if (strcmp(msg->getName(), "BLE_NWKtoMAC_CMD")){//command: if the same (=0) then go to else command
        EV <<"Tham - The msg name is not the same" << endl;
        error("BLEMacV2: received unsupported command from upper layer!");
    }
    else{
        cObject* cInfo = msg->removeControlInfo();
        BLE_NwkToMac *const cCmdInfo = dynamic_cast<BLE_NwkToMac *const>(cInfo);
        //if(debug_Internal_Messages)
        EV << "BLEMacV2:: command type " << cCmdInfo->get_CmdType() << endl;

        switch(cCmdInfo->get_CmdType()){
            //advertisement-related commands
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand: // cmdType = 21
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingDataCommand: // cmdType = 20
            case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertiseEnableCommand: // cmdType = 19
                ev <<"Tham - Call handleUpperCommand_Adv(cCmdInfo)" << endl;
                handleUpperCommand_Adv(cCmdInfo);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Command: // cmdType = 33
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command:
                ev <<"Tham - Call handleUpperCommand_Initiate(cCmdInfo)" << endl;
                handleUpperCommand_Initiate(cCmdInfo);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Set_Host_Channel_Classification_Command: // cmdType = 37
                ev <<"Tham - Call handleUpperCommand_updateDataChannelMap" << endl;
                handleUpperCommand_updateDataChannelMap(&cCmdInfo->get_ConnectionParsPtr()->Data_Channel_Map);
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Update_Command: // cmdType = 30
                ev <<"Tham - Call handleUpperCommand_updateConnectionPars" << endl;
                handleUpperCommand_updateConnectionPars(cCmdInfo->get_ConnectionParsPtr());
            break;
            case cBLEstructs_defs::BLECMD_HCICtrl_Disconnect_Command:
                ev <<"Tham - Call handleUpperCommand_disconnect" << endl;
                handleUpperCommand_disconnect(cCmdInfo->get_ErrorPtr());
            break;
            //NON-STANDARD COMMANDS
            case cBLEstructs_defs::Simulation_LLBufferPayloadUpdate_Command:
                ev <<"Tham - Call handleUpperCommand_LLBufferPayloadUpdate" << endl;
                handleUpperCommand_LLBufferPayloadUpdate(cCmdInfo->get_BufferPayloadUpdate());
            break;
            default:
               error("BLEMacV2: received unsupported command from upper layer!");
            break;
        }
    }
    delete msg;
}



cPacket *BLEMacV2::decapsAdvMsg(BLE_Adv_MacPkt * macPkt){
    //TODO: implement
    EV << "Tham - *BLEMacV2::decapsAdvMsg " << endl;
    return NULL;
}


 //Arms the required timer
bool BLEMacV2::startTimer(t_mac_timer timer){
    ev << "Tham BLEMacV2::startTimer" << endl;
//ADVERTISING
    if (timer == TIMER_ADVERTISE_EVENTSTART) {
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_ADVERTISE_EVENT_START" << endl;
        EV <<"Tham BLEMacV2::startTimer -- Advertising_Interval_Min ="<< myAdvrtPars.Advertising_Interval_Min <<endl;
        EV <<"Tham BLEMacV2::startTimer -- Advertising_Interval_Max ="<< myAdvrtPars.Advertising_Interval_Max <<endl;
        simtime_t NextAdvEvenTimer;
        NextAdvEvenTimer=calculateAdvEvent(myAdvrtPars.Advertising_Interval_Min, myAdvrtPars.Advertising_Interval_Max, myAdvrtPars.Adv_Type);

        ev <<  "Tham BLEMacV2::startTimer : NextAdvEvenTimer = " << NextAdvEvenTimer << endl;
        scheduleAt(simTime()+NextAdvEvenTimer, advertisementEventTimer);

        if (debug_Timers)
            EV  <<"TIMER_ADVERTISE_EVENT_START armed after:" << NextAdvEvenTimer << " s" << endl;
    }
    else if(timer == TIMER_ADVERTISE_NEXTPDU){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_ADVERTISE_NEXT_PDU" << endl;

        simtime_t NextPduTimer;
        simtime_t TimeCalibr;

        if(phy->getRadioState()==MiximRadio::RX){
            TimeCalibr=0;
        }
        else if(phy->getRadioState()==MiximRadio::SLEEP){
            TimeCalibr=Time_llRXtoTX-Time_llSLEEPtoTX;
        }

        switch(myAdvrtPars.Adv_Type){

        case cBLEstructs_defs::ADV_IND_0:
            EV << "Tham - next PDU timer for ADV_IND_0" << endl;
            NextPduTimer=myAdvrtPars.ADV_IND_PDUperiod-TimeCalibr;
            break;
        case cBLEstructs_defs::ADV_DIRECT_IND_HDC_1:
            NextPduTimer=myAdvrtPars.ADV_DIRECT_IND_HDC_PDUperiod-TimeCalibr;
            break;
        case cBLEstructs_defs::ADV_SCAN_IND_2:
            NextPduTimer=myAdvrtPars.ADV_SCAN_IND_PDUperiod-TimeCalibr;
            break;
        case cBLEstructs_defs::ADV_NONCONN_IND_3:
            NextPduTimer=myAdvrtPars.ADV_NONCONN_IND_PDUperiod-TimeCalibr;
            break;
        case cBLEstructs_defs::ADV_DIRECT_IND_LDC_4:
            NextPduTimer=myAdvrtPars.ADV_DIRECT_IND_LDC_PDUperiod-TimeCalibr;
            break;
        }

        scheduleAt(simTime() + NextPduTimer, advertisementNextPDUTimer);

        if (debug_Timers) EV  <<"TIMER_ADVERTISE_NEXT_PDU armed after:" << NextPduTimer << " s" << endl;
    }
    else if(timer == TIMER_ADVERTISE_EVENTEND){//worst case scenario
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_ADVERTISE_EVENT_END" << endl;
        simtime_t EventEndTimer=advertisementEventTimer->getArrivalTime()-Time_llRXtoSLEEP-Time_llSLEEPtoTX;
        if (debug_Timers)
        EV << "BLEMacV2::startTimer arming advertisementEndEvent at " << EventEndTimer << " (advertisementEventTimer=" <<advertisementEventTimer->getArrivalTime() << " Time_llRXtoSLEEP=" << Time_llRXtoSLEEP << " Time_llSLEEPtoTX=" << Time_llSLEEPtoTX << ")" << endl;
        scheduleAt(EventEndTimer, advertisementEndEvent);
    }
//INITIATING
    else if(timer == TIMER_INITIATING_INTERVAL){//
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_INITIATING_INTERVAL" << endl;
        //simtime_t InitIntervalTimer=simTime()+myAdvrtPars.SCAN_ScanInterval;

        simtime_t InitIntervalTimer = simTime() + myScanPars.LE_Scan_Interval*0.000625;

        EV << "Tham BLEMacV2::startTimer LE_Scan_Interval = " << myScanPars.LE_Scan_Interval <<endl;
        EV << "Tham BLEMacV2::startTimer LE_Scan_Window = " << myScanPars.LE_Scan_Window <<endl;

        if (debug_Timers)
            //EV << "BLEMacV2::startTimer arming initiatingScanIntervalTimer at " << InitIntervalTimer << " (ScanInterval=" << myAdvrtPars.SCAN_ScanInterval << "s)" << endl;
            EV << "BLEMacV2::startTimer arming initiatingScanIntervalTimer at " << InitIntervalTimer << " (ScanInterval=" << myScanPars.LE_Scan_Interval << endl;

        scheduleAt(InitIntervalTimer, initiatingScanIntervalTimer);
    }
    else if(timer == TIMER_INITIATING_WINDOW){//
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_INITIATING_WINDOW" << endl;
        //simtime_t InitWindowTimer=simTime()+myAdvrtPars.SCAN_ScanWindow;

        simtime_t InitWindowTimer = simTime() + myScanPars.LE_Scan_Window*0.000625;
        EV << "Tham BLEMacV2::startTimer LE_Scan_Window = " << myScanPars.LE_Scan_Window <<endl;

        if (debug_Timers)
            //EV << "BLEMacV2::startTimer arming initiatingScanWindowTimer at " << InitWindowTimer << " (LE_Scan_Window=" << myAdvrtPars.SCAN_ScanWindow << "s)" << endl;
            EV << "BLEMacV2::startTimer arming initiatingScanWindowTimer at " << InitWindowTimer << " (LE_Scan_Window=" << myScanPars.LE_Scan_Window << endl;

        scheduleAt(InitWindowTimer, initiatingScanWindowTimer);
    }
//CONNECTION SLAVE
    else if(timer ==  TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION_FIRST){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION_FIRST" << endl;
        simtime_t WakeupTimer;
        WakeupTimer=simTime()+0.00125*(myConnPars.ConnInfo.transmitWindowOffset+1);
        WakeupTimer=WakeupTimer-Time_llSLEEPtoRX;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming slaveConnectionWakeupConnectionTimer at " << WakeupTimer << " (Interval=" <<myConnPars.ConnInfo.connInterval <<")" << endl;
        scheduleAt(WakeupTimer, slaveConnectionWakeupConnectionTimer);
    }
    else if(timer ==  TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION" << endl;
        simtime_t WakeupTimer;
        WakeupTimer=simTime()+myConnPars.ConnInfo.connInterval*0.00125;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming slaveConnectionWakeupConnectionTimer at " << WakeupTimer << " (Interval=" <<myConnPars.ConnInfo.connInterval <<")" << endl;
        scheduleAt(WakeupTimer, slaveConnectionWakeupConnectionTimer);
    }
    else if(timer ==  TIMER_CONNECTION_SLAVE_TRANSMIT_WINDOW){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_TRANSMIT_WINDOW" << endl;
        simtime_t TransmitWindow;
        TransmitWindow=0.00125*myConnPars.ConnInfo.transmitWindowSize+llmaxDataPDUduration;//see p. 2541 of spec v4.1
        //Although this should never happen due to init values of the standard, better to check
        if(simTime()+TransmitWindow+Time_llRXtoSLEEP+Time_llSLEEPtoRX>slaveConnectionWakeupConnectionTimer->getArrivalTime()){
            error("LEMac::startTimer on TIMER_CONNECTION_SLAVE_TRANSMIT_WINDOW: impossible relation between TransmitWindow and conInterval");
        }
        else{
            scheduleAt(simTime()+TransmitWindow, slaveConnectionTransmitWindowTimer);
        }
    }
    else if(timer == TIMER_CONNECTION_SLAVE_WAKEUP){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_WAKEUP" << endl;
        simtime_t WindowWidening;
        simtime_t WakeupTimer;
        //NOTE: according to the current equation in the standard (see p. 2543 for spec v4.1) there should be just the simTime() in the formula, but this does not make any sense!

        WakeupTimer=myConnPars.ConnInfo.timeLastAnchorReceived+(myConnPars.ConnInfo.numMissedEvents+1)*myConnPars.ConnInfo.connInterval*0.00125;
        WakeupTimer=WakeupTimer-Time_llSLEEPtoRX;
        WindowWidening = myConnPars.ConnInfo.getWindowWidening(WakeupTimer);
        //EV << "BLEMacV2::initialize (timeLastAnchorReceived=" <<myConnPars.ConnInfo.timeLastAnchorReceived << " WakeupTimer=" << WakeupTimer << ")" << endl;
        //EV << "BLEMacV2::initialize (Interval=" <<myConnPars.ConnInfo.connInterval << " WindowWidening=" << WindowWidening << "numMissedEvents" << myConnPars.ConnInfo.numMissedEvents << "connInterval" << myConnPars.ConnInfo.connInterval <<")" << endl;
        if(WindowWidening>(myConnPars.ConnInfo.connInterval*0.00125/2.0-0.000150)){//see p.2544 for spec v4.1
            //TODO: not sure which error code should be used
            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
            myCmdError->ErrorCode=cBLEstructs_Error::UNSPECIFIFED_ERROR;
            dropConnection();
        }
        else{
            WakeupTimer=WakeupTimer-WindowWidening;//
            if(WakeupTimer<simTime()){
                return false;
            }
            else{
                //if (debug_Timers)
                    EV << "BLEMacV2::startTimer arming slaveConnectionWakeup at " << WakeupTimer << " (Interval=" <<myConnPars.ConnInfo.connInterval << " WindowWidening=" << WindowWidening << "numMissedEvents" << myConnPars.ConnInfo.numMissedEvents <<")" << endl;
                scheduleAt(WakeupTimer, slaveConnectionWakeupTimer);
            }
        }
    }
    else if(timer == TIMER_CONNECTION_SLAVE_NOBCN){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_NOBCN" << endl;
        simtime_t WindowWidening;
        simtime_t NoBcnTimer;
        WindowWidening = myConnPars.ConnInfo.getWindowWidening(simTime());
        if(WindowWidening>(myConnPars.ConnInfo.connInterval*0.00125-0.000150)){//see p.2544 for spec v4.1
            //TODO: not sure which error code should be used
            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
            myCmdError->ErrorCode=cBLEstructs_Error::UNSPECIFIFED_ERROR;
            dropConnection();
        }
        else{
            NoBcnTimer=simTime()+std::max(WindowWidening,llmaxDataPDUduration);//this is NOT according to the standard, but otherwise we might just fucking miss the packet!
            if (debug_Timers)
                EV << "BLEMacV2::startTimer arming slaveConnectionNoBeaconTimer at " << NoBcnTimer << " (WindowWidening=" << WindowWidening << ")" << endl;
            scheduleAt(NoBcnTimer, slaveConnectionNoBeaconTimer);
        }
    }
    else if(timer == TIMER_CONNECTION_SLAVE_IFS){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_IFS" << endl;
        simtime_t IFSTimer;
        IFSTimer=simTime()+llIFS;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming slaveIFSTimer at " << IFSTimer << endl;
        scheduleAt(IFSTimer, slaveIFSTimer);
    }
    else if(timer == TIMER_CONNECTION_SLAVE_REPLY){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_REPLY" << endl;
        simtime_t WaitTimer;
        simtime_t AvailableTime = calculateNextDataEventTime()-std::max(myConnPars.ConnInfo.getWindowWidening(calculateNextDataEventTime()),llIFS)-llIFS - simTime();
        if(AvailableTime>llminDataPDUduration){
            //try receive
            if(AvailableTime>llmaxDataPDUduration){
                WaitTimer=simTime()+llIFS+llIFSDeviation+llmaxDataPDUduration;
            }
            else{
                WaitTimer=simTime()+AvailableTime+llIFS+llIFSDeviation;
            }
            if (debug_Timers)
                EV << "BLEMacV2::startTimer arming slaveWaitReplyTimer at " << WaitTimer << endl;
            scheduleAt(WaitTimer, slaveWaitReplyTimer);
        }
        else{
            if (debug_Timers)
                EV << "BLEMacV2::startTimer arming slaveWaitReplyTimer. No chance to get a reply in time! " << endl;
            return false;
        }
    }
    else if(timer == TIMER_CONNECTION_SLAVE_SUPERVISION_FIRST){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_SUPERVISION_FIRST" << endl;
        simtime_t SupervisionTimer;
        SupervisionTimer=6*myConnPars.ConnInfo.connInterval*0.00125;//see p. 2539 of spec v4.1
        scheduleAt(simTime()+SupervisionTimer, slaveConnectionSupervisionTimer);
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming slaveConnectionSupervisionTimer at " << simTime()+SupervisionTimer << endl;
    }
    else if(timer == TIMER_CONNECTION_SLAVE_SUPERVISION){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_SLAVE_SUPERVISION" << endl;
        simtime_t SupervisionTimer;
        SupervisionTimer=0.01*myConnPars.ConnInfo.connSupervisionTimeout;//see p. 2539 of spec v4.1
        if(SupervisionTimer<0.1){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_SLAVE_SUPERVISION: The desired connSupervisionTimeout is below the minimum possible according to the standard (i.e., 100 ms). Please modify the connSupervisionTimeout accordingly.");
        }
        if(SupervisionTimer>32){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_SLAVE_SUPERVISION: The desired connSupervisionTimeout is above the maximum possible according to the standard (i.e., 32 s). Please modify the connSupervisionTimeout accordingly.");
        }
        if(SupervisionTimer<((1+myConnPars.ConnInfo.connSlaveLatency)*myConnPars.ConnInfo.connInterval*0.00125)){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_SLAVE_SUPERVISION: The desired connSupervisionTimeout (%d) is below (1+connSlaveLatency (%d) )*connInterval (%d)*0.00125. Please modify the connSupervisionTimeout (or connSlaveLatency & connInterval) accordingly.", myConnPars.ConnInfo.connSupervisionTimeout, myConnPars.ConnInfo.connSlaveLatency, myConnPars.ConnInfo.connInterval);
        }
        scheduleAt(simTime()+SupervisionTimer, slaveConnectionSupervisionTimer);
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming slaveConnectionSupervisionTimer at " << simTime()+SupervisionTimer << endl;
    }

//CONNECTION MASTER
    else if(timer == TIMER_CONNECTION_MASTER_WAKEUP){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_WAKEUP" << endl;
        simtime_t WakeupTimer;
        WakeupTimer=calculateNextDataEventTime();
        //WakeupTimer=WakeupTimer-Time_llSLEEPtoRX;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming MasterConnectionWakeupTimer at " << WakeupTimer << " (Interval=" <<myConnPars.ConnInfo.connInterval << "numMissedEvents" << myConnPars.ConnInfo.numMissedEvents << " )" << endl;
        scheduleAt(WakeupTimer, masterConnectionWakeupTimer);
    }
    else if(timer == TIMER_CONNECTION_MASTER_WAKEUP_FIRST){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_WAKEUP_FIRST" << endl;
        simtime_t WakeupTimer;
        WakeupTimer=calculateFirstConnectionEventTime_Master();
        WakeupTimer=WakeupTimer-Time_llSLEEPtoRX;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming MasterConnectionWakeupTimer at " << WakeupTimer << " (Interval=" <<myConnPars.ConnInfo.connInterval << "numMissedEvents" << myConnPars.ConnInfo.numMissedEvents << " )" << endl;
        scheduleAt(WakeupTimer, masterConnectionWakeupTimer);
    }
    else if(timer == TIMER_CONNECTION_MASTER_WAITREPLY){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_WAITREPLY" << endl;
        simtime_t WaitTimer;
        simtime_t AvailableTime = calculateNextDataEventTime()-llIFS-llIFS - simTime();
        if(AvailableTime>llminDataPDUduration){
            //try receive
            if(AvailableTime>llmaxDataPDUduration){
                WaitTimer=simTime()+llIFS+llIFSDeviation+llmaxDataPDUduration;
            }
            else{
                WaitTimer=simTime()+AvailableTime+llIFS+llIFSDeviation;
            }
            if (debug_Timers)
                EV << "BLEMacV2::startTimer arming masterWaitReplyTimer at " << WaitTimer << endl;
            scheduleAt(WaitTimer, masterWaitReplyTimer);
        }
        else{
            if (debug_Timers)
                EV << "BLEMacV2::startTimer arming masterWaitReplyTimer. No chance to get a reply in time! " << endl;
            return false;
        }
    }
    else if(timer == TIMER_CONNECTION_MASTER_IFS){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_IFS" << endl;
        simtime_t IFSTimer;
        IFSTimer=simTime()+llIFS;
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming masterIFSTimer at " << IFSTimer << endl;
        scheduleAt(IFSTimer, masterIFSTimer);
    }
    else if(timer == TIMER_CONNECTION_MASTER_SUPERVISION_FIRST){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_SUPERVISION_FIRST" << endl;
        simtime_t SupervisionTimer;
        SupervisionTimer=6*myConnPars.ConnInfo.connInterval*0.00125;//see p. 2539 of spec v4.1
        scheduleAt(simTime()+SupervisionTimer, masterConnectionSupervisionTimer);
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming masterConnectionSupervisionTimer at " << simTime()+SupervisionTimer << endl;
    }
    else if(timer == TIMER_CONNECTION_MASTER_SUPERVISION){
        ev <<  "Tham BLEMacV2::startTimer - timer == TIMER_CONNECTION_MASTER_SUPERVISION" << endl;
        simtime_t SupervisionTimer;
        SupervisionTimer=0.01*myConnPars.ConnInfo.connSupervisionTimeout;//see p. 2539 of spec v4.1
        if(SupervisionTimer<0.1){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_MASTER_SUPERVISION: The desired connSupervisionTimeout is below the minimum possible according to the standard (i.e., 100 ms). Please modify the connSupervisionTimeout accordingly.");
        }
        if(SupervisionTimer>32){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_MASTER_SUPERVISION: The desired connSupervisionTimeout is above the maximum possible according to the standard (i.e., 32 s). Please modify the connSupervisionTimeout accordingly.");
        }
        if(SupervisionTimer<((1+myConnPars.ConnInfo.connSlaveLatency)*myConnPars.ConnInfo.connInterval*0.00125)){//see p. 2539 of spec v4.1
            error("BLEMacV2::startTimer - TIMER_CONNECTION_MASTER_SUPERVISION: The desired connSupervisionTimeout is below (1+connSlaveLatency)*connInterval*0.00125. Please modify the connSupervisionTimeout (or connSlaveLatency & connInterval) accordingly.");
        }
        scheduleAt(simTime()+SupervisionTimer, masterConnectionSupervisionTimer);
        if (debug_Timers)
            EV << "BLEMacV2::startTimer arming masterConnectionSupervisionTimer at " << simTime()+SupervisionTimer << endl;
    }
//OTHER
    else {
        error("Unknown timer requested to start");
    }

    //check timer consistency
    if (debug_Timers)
        EV  <<"Checking timer consistency:" <<endl;
    if(advertisementEventTimer->isScheduled())
        if (debug_Timers)
            EV  <<"advertisementEventTimer armed at "<< advertisementEventTimer->getArrivalTime() <<endl;
    if(advertisementNextPDUTimer->isScheduled())
        if (debug_Timers)
            EV  <<"advertisementNextPDUTimer armed at "<< advertisementNextPDUTimer->getArrivalTime() <<endl;
    if(advertisementEndEvent->isScheduled())
        if (debug_Timers)
            EV  <<"advertisementEndEvent armed at "<< advertisementEndEvent->getArrivalTime() <<endl;

    if((advertisementEventTimer->isScheduled())&&(advertisementNextPDUTimer->isScheduled())){
        //adv event happens earlier than next adv PDU
        if(advertisementNextPDUTimer->getArrivalTime()>advertisementEventTimer->getArrivalTime()){
            if (debug_Timers)
                EV  <<"advertisementNextPDUTimer > advertisementEventTimer" <<endl;
            cancelEvent(advertisementNextPDUTimer);
        }
    }
    if((advertisementEventTimer->isScheduled())&&(advertisementEndEvent->isScheduled())){
        //adv event happens earlier than next adv PDU
        if(advertisementEndEvent->getArrivalTime()>advertisementEventTimer->getArrivalTime()){
            if (debug_Timers)
                EV  <<"advertisementEndEvent > advertisementEventTimer" <<endl;
            cancelEvent(advertisementEndEvent);
        }
    }
    return true;
}

void BLEMacV2::updateMacState(t_mac_states newMacState){
    macState = newMacState;
}


void BLEMacV2::updateMacSubstate(t_mac_substates newMacSubstate){
    macSubState = newMacSubstate;
}



void BLEMacV2::updateStatusStandby(t_mac_event event, cMessage *msg){
    switch (event) {
        default:
            error("BLEMacV2::updateStatusStandby unsupported event!");
            break;
    }
}

// For data exchanging
void BLEMacV2::updateStatusConnected(t_mac_event event, cMessage *msg){
    //int IndexCh_data;

   // if ((num_data_pkt_rcved > input_data_round) && (num_data_pkt_sent > input_data_round) ) {

       // t_end = simTime();
   //     endSimulation();

  //  }

    //if (num_data_pkt_rcved > input_data_round ) endSimulation();

    switch (event)
    {
        //COMMON - HANDLE RECEIVED RADIO PACKET
        case EV_FRAME_RECEIVED:
        {
           num_data_pkt_rcved++;

         // if (num_data_pkt_rcved < min_data_rounds)
         //  min_data_rounds = num_data_pkt_rcved;

//           if (num_data_pkt_rcved > input_data_round + 1) endSimulation();

            if(macSubState==CONNECTED_SLV_WAITPKT){
                BLE_Data_MacPkt*  RxdLLPkt = static_cast<BLE_Data_MacPkt *> (msg);

                if(debug_Pkt_Basic){
                    EV << "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_SLV_WAITPKT). SLAVE GOT A DATA PDU. AccessAddress=" <<
                    RxdLLPkt->getAccessAddress() <<" my Address=" << myConnPars.ConnInfo.accessAddr <<endl;
                }

                if(RxdLLPkt->getAccessAddress()==myConnPars.ConnInfo.accessAddr){ //for me
                    bool beacon_packet=false;
                    cancelEvent(slaveConnectionSupervisionTimer);//stop timer
                    startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION);
                    //DEBUG
                    if(debug_Pkt_Basic){
                        EV << "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_SLV_WAITPKT). SLAVE GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" Packet Length=" << RxdLLPkt->getBitLength()
                           << " seqID="<< RxdLLPkt->getSequenceId() << " MD=" << RxdLLPkt->getHdr_MD() << " SN=" << RxdLLPkt->getHdr_SN()
                           << " NESN=" << RxdLLPkt->getHdr_NESN() <<endl;
                    }

                    //EV << "BLEMacV2::updateStatusConnected MASTER HAS GIVEN DOBBY A SOCK ... SORRY - A PACKET!" << endl;
                    myConnPars.ConnInfo.moreData=RxdLLPkt->getHdr_MD();
                    //DEBUG
                    //EV << "BLEMacV2::updateStatusConnected numMissedEvents=" << myConnPars.ConnInfo.numMissedEvents << endl;
                    if(myConnPars.ConnInfo.numMissedEvents){//first packet - update connection status
                        beacon_packet=true;
                        //calculate the time of packet start
                        simtime_t PacketStartTime=simTime()-RxdLLPkt->getBitLength()/bitrate;
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected packet is a BEACON(i.e., the first packet in a connection event)!" << endl;
                        //EV << "BLEMacV2::updateStatusConnected calculating event start time: result=" << PacketStartTime << endl;
                        cancelEvent(slaveConnectionNoBeaconTimer);//stop timer
                        //update connection info
                        myConnPars.ConnInfo.timeLastSuccesfullEvent=PacketStartTime;
                        myConnPars.ConnInfo.timeLastAnchorReceived=myConnPars.ConnInfo.timeLastSuccesfullEvent;//just to make compatible with standard naming
                        myConnPars.ConnInfo.numMissedEvents=0;
                    }
                    else{
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected packet is consequent packet!" << endl;
                        cancelEvent(slaveWaitReplyTimer);//stop timer
                    }

                    simtime_t AvailableTime = calculateNextDataEventTime()-
                             std::max(myConnPars.ConnInfo.getWindowWidening(calculateNextDataEventTime()),llIFS)-
                             llIFS-simTime();
                    //Explanation: take left time till next connection event and deduct
                    //llIFS(wait before TX) & max(llIFS - security interval before connection event start, WindowWidening)
                    //EV << "nextExpectedSeqNum=" << myConnPars.ConnInfo.nextExpectedSeqNum << " SN=" << RxdLLPkt->getHdr_SN() << endl;
                    //EV << "transmitSeqNum=" << myConnPars.ConnInfo.transmitSeqNum << " NESN=" << RxdLLPkt->getHdr_NESN() << endl;

                    if(myConnPars.ConnInfo.nextExpectedSeqNum!=RxdLLPkt->getHdr_SN()){
                        eventDataReTXRXd(RxdLLPkt->getHdr_lgth());
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) SN!=nextExpectedSeqNum - this is a resent PDU" << endl;
                    }
                    else{
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) SN==nextExpectedSeqNum - this is a new PDU" << endl;
                        myConnPars.ConnInfo.nextExpectedSeqNum=!myConnPars.ConnInfo.nextExpectedSeqNum;
                        if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_start_2){//start packet of a sequence or whole packet
                            eventDataNewRXd(RxdLLPkt->getHdr_lgth());

                            BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                            RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error

                            DataPktForNWK = RxdLLPktCopy->decapsulate();

                            double challenge = RxdLLPktCopy->getChallenge_data();
                            EV << "Tham - UpdateStausConnected at EV_FRAME_RECEIVED and CONNECTED_SLV_WAITPKT with LLID_Data_start_2 packet: Slave received challenge = " << challenge << endl;

                            if(DataPktForNWK){
                                /*PktLeft_RX_Size=DataPktForNWK->getByteLength()-RxdLLPkt->getHdr_lgth();//get length
                                if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) LLID_2 received, data to receive in next packets =" << PktLeft_RX_Size << endl;
                                if(PktLeft_RX_Size==0){
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) LLID_2 received: forwarding packet to next level" << endl;
                                    sendUp(DataPktForNWK);
                                }
                                else delete DataPktForNWK;
                                *///KVM
                            sendUp(DataPktForNWK);
                            }
                            delete RxdLLPktCopy;//
                        }
                        else if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_frag_1){//fragment
                            //if(PktLeft_RX_Size>=0){//KVM
                            eventDataNewRXd(RxdLLPkt->getHdr_lgth());
                            EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) LLID_1 received! PktLeft_RX_Size=" << PktLeft_RX_Size << "Received packet length=" <<RxdLLPkt->getHdr_lgth() << "Data left=" << PktLeft_RX_Size-RxdLLPkt->getHdr_lgth() << endl;
                            PktLeft_RX_Size=PktLeft_RX_Size-RxdLLPkt->getHdr_lgth();//get length
                            //if(PktLeft_RX_Size<0)error("BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) LLID_1 received PktLeft_RX_Size<0");
                            //else if(PktLeft_RX_Size==0){
                            if(PktLeft_RX_Size<=0){
                                PktLeft_RX_Size=0;
                                BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                                RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error

                                DataPktForNWK = RxdLLPktCopy->decapsulate();

                                double challenge = RxdLLPktCopy->getChallenge_data();


                                EV << "Tham - UpdateStausConnected at EV_FRAME_RECEIVED and CONNECTED_SLV_WAITPKT with LLID_Data_frag_1 packet: Slave received challenge = " << challenge << endl;
                                IndexCh_data = RxdLLPktCopy->getChallenge_index();
                                EV << "Tham - Slave received packet with IndexCh_data = " << IndexCh_data << endl;

                                if(DataPktForNWK){
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) LLID_1 received: forwarding packet to next level" << endl;
                                    sendUp(DataPktForNWK);
                                }
                                else delete DataPktForNWK;
                                delete RxdLLPktCopy;//
                            }
                        //}

                        }
                    }

                    if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_ctrl_3){//this is a command
                        if(RxdLLPkt->getOpcode()==BLEstructs::LL_CHANNEL_MAP_REQ){//we should chnage the channel map
                            //check the validity - see p. 2552 of spec v4.1
                            if(abs(RxdLLPkt->getInstant()-myConnPars.ConnInfo.connEventCounter)>32767) error("break connection");
                            if(debug_Internal_Messages) EV << "LL_CHANNEL_MAP_REQ received, new map will be applied at instance "<< RxdLLPkt->getInstant()<< endl;
                            myConnPars.ConnInfo.Instant_NewMap=RxdLLPkt->getInstant();
                            myConnPars.ConnInfo.new_Data_Channel_Map.fill(
                                    RxdLLPkt->getMapCh0to7(),
                                    RxdLLPkt->getMapCh8to15(),
                                    RxdLLPkt->getMapCh16to23(),
                                    RxdLLPkt->getMapCh24to31(),
                                    RxdLLPkt->getMapCh32to39()
                            );
                        }
                        else if(RxdLLPkt->getOpcode()==BLEstructs::LL_CONNECTION_UPDATE_REQ){
                            //check the validity - see p. 2550 of spec v4.1
                             if(abs(RxdLLPkt->getInstant()-myConnPars.ConnInfo.connEventCounter)>32767) error("break connection");
                             if(debug_Internal_Messages) EV << "LL_CONNECTION_UPDATE_REQ received, new parameters will be applied at instance "<< RxdLLPkt->getInstant()<< endl;
                             myConnPars.ConnInfo.Instant_NewParameters=RxdLLPkt->getInstant();
                             myConnPars.ConnInfo.new_connInterval=RxdLLPkt->getInterval();
                             myConnPars.ConnInfo.new_connSlaveLatency=RxdLLPkt->getLatency();
                             myConnPars.ConnInfo.new_connSupervisionTimeout=RxdLLPkt->getTimeout();
                             myConnPars.ConnInfo.transmitWindowOffset=RxdLLPkt->getWinOffset();
                             myConnPars.ConnInfo.transmitWindowSize=RxdLLPkt->getWinSize();
                        }
                        else if(RxdLLPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                            if(debug_Internal_Messages) EV << "LL_TERMINATE_IND received"<< endl;

                            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
                            myCmdError->ErrorCode=RxdLLPkt->getErrorCode();
                            Flag_PeerTerminatesConnection=true;

                            if(prepareEmptyACKPkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - ing ACK" << endl;
                            }
                            else{//not enough time - switch to next channel
                                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                scheduleAt(simTime(), slaveEndConnectionEvent);
                            }
                        }
                        else{
                            error("BLEMacV2::updateStatusConnected: received unknown data channel command!");
                        }
                    }

                    if(Flag_PeerTerminatesConnection==false){
                        if(myConnPars.ConnInfo.transmitSeqNum==RxdLLPkt->getHdr_NESN()){
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) NESN==transmitSeqNum - I must resend previous PDU" << endl;
                            if(checkCurrentDataPkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - resending data PDU" << endl;
                            }
                            else{//not enough time - switch to next channel
                                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                scheduleAt(simTime(), slaveEndConnectionEvent);
                            }
                        }
                        else{//new message can be sent
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITPKT) NESN!=transmitSeqNum - I can send a new PDU" << endl;
                            if(DataPkt_FirstFragmentSent==true){
                                DataPkt_FirstFragment=false;
                                DataPkt_FirstFragmentSent=false;
                            }
                            myConnPars.ConnInfo.transmitSeqNum=!myConnPars.ConnInfo.transmitSeqNum;
                            EV<< "BLEMacV2::updateStatusConnected Flag_HighPriorityTraffic=" << Flag_HighPriorityTraffic << " Flag_UpdatePending_TerminateConnection=" << Flag_UpdatePending_TerminateConnection << endl;
                            if(Flag_HighPriorityTraffic==true){ // initialise at False - Tham
                                 if(HighPriorityDataPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                                     //got acknowledgment for my connection termination
                                     //dropConnection();
                                     scheduleAt(simTime(), ctrl_terminateConnection);
                                 }
                                 else{
                                     error("BLEMacV2::updateStatusConnected - unsupported case!");
                                     //TODO: other cases
                                 }
                                 Flag_HighPriorityTraffic=false; //clear flag
                             }
                            else{
                                if(Flag_UpdatePending_TerminateConnection==true){//connection terminate
                                    if(prepareConnTerminatePkt(AvailableTime)==true){//enough time
                                         simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                        if(FreeTime==0) eventRadioStateChanged();
                                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                         startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                         if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending Terminate Connection" << endl;
                                    }
                                      else{
                                          if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                           scheduleAt(simTime(), slaveEndConnectionEvent);
                                    }
                                }
                                else{
                                    if((checkBuffer(LastPacketPayload)==true)
                                        ||(myConnPars.ConnInfo.moreData==true)
                                       ||((beacon_packet==true)&&(llSlave_beacon_ReplyPolicy==ALWAYS_1))){
                                         if(prepareNewDataPkt(AvailableTime)==true){//enough time
                                             simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                            if(FreeTime==0) eventRadioStateChanged();
                                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                             startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                             if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending a data PDU" << endl;
                                        }
                                        else{//not enough time - switch to next channel
                                             prepareNewDataPkt(1);//prepare next packet
                                             if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                             scheduleAt(simTime(), slaveEndConnectionEvent);
                                        }
                                    }
                                     else{
                                        if(debug_Internal_Messages)
                                             EV << "BLEMacV2::updateStatusConnected no need to send reply - close event & postprone packet generation till event's start" << endl;
                                         scheduleAt(simTime(), slaveEndConnectionEvent);
                                         myConnPars.ConnInfo.delayedGeneration=true;
                                    }
                                }
                            }
                        }
                    }
                    //delete msg;
                }

                else{ // packet not for me

                 if(debug_Pkt_Basic) EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_SLV_WAITPKT). SLAVE GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" NOT MATCHES MINE! which is: "<< myConnPars.ConnInfo.accessAddr << "Packet dropped! " <<endl;

                    //delete msg;
                }
                     delete msg; // original

            } //----------------------- end of if (macSubState==CONNECTED_SLV_WAITPKT)-------------------------------

                //i.e., before the first packet in a connection has been received

            else if(macSubState==CONNECTED_SLV_WAITCONNECTION){
                BLE_Data_MacPkt*  RxdLLPkt = static_cast<BLE_Data_MacPkt *> (msg);
                if(RxdLLPkt->getAccessAddress()==myConnPars.ConnInfo.accessAddr){//for me - first packet
                    if(debug_Pkt_Basic){
                        EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_SLV_WAITCONNECTION). SLAVE GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" Packet Length=" << RxdLLPkt->getBitLength()
                           << " LLID=" << RxdLLPkt->getHdr_LLID() << " seqID="<< RxdLLPkt->getSequenceId() << " MD=" << RxdLLPkt->getHdr_MD() << " SN=" << RxdLLPkt->getHdr_SN()
                           << " NESN=" << RxdLLPkt->getHdr_NESN() << "" << endl;
                    }

                    bool beacon_packet=true;

                    updateMacSubstate(CONNECTED_SLV_WAITPKT);

                    cancelEvent(slaveConnectionWakeupConnectionTimer);
                    cancelEvent(slaveConnectionTransmitWindowTimer);
                    cancelEvent(slaveConnectionSupervisionTimer);
                    startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION);


                    myConnPars.ConnInfo.moreData=RxdLLPkt->getHdr_MD();

                    simtime_t PacketStartTime=simTime()-RxdLLPkt->getBitLength()/bitrate;

                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected packet is a BEACON!" << endl;

                    //EV<< "BLEMacV2::updateStatusConnected calculating event start time: result=" << PacketStartTime << endl;
                    cancelEvent(slaveConnectionNoBeaconTimer);//stop timer
                    //update connection info
                    myConnPars.ConnInfo.timeLastSuccesfullEvent=PacketStartTime;
                    myConnPars.ConnInfo.timeLastAnchorReceived=myConnPars.ConnInfo.timeLastSuccesfullEvent;//just to make compatible with standard naming
                    myConnPars.ConnInfo.numMissedEvents=0;

                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected timeLastSuccesfullEvent=" << PacketStartTime << endl;

                    simtime_t AvailableTime = calculateNextDataEventTime()-
                                             std::max(myConnPars.ConnInfo.getWindowWidening(calculateNextDataEventTime()),llIFS)-
                                             llIFS-simTime();//Explanation: take left time till next connection event and deduct


                    if(myConnPars.ConnInfo.nextExpectedSeqNum!=RxdLLPkt->getHdr_SN()){
                        eventDataReTXRXd(RxdLLPkt->getHdr_lgth());
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) SN!=nextExpectedSeqNum - this is a resent PDU" << endl;
                    }
                    else{
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) SN==nextExpectedSeqNum - this is a new PDU" << endl;
                        myConnPars.ConnInfo.nextExpectedSeqNum=!myConnPars.ConnInfo.nextExpectedSeqNum;

                        if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_start_2){//start packet of a sequence or whole packet
                            EV<< "BLEMacV2::updateStatusConnected - LLID_2 received" << endl;

                            eventDataNewRXd(RxdLLPkt->getHdr_lgth());

                            BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                            RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error

                            DataPktForNWK = RxdLLPktCopy->decapsulate();

                            double challenge = RxdLLPktCopy->getChallenge_data();
                            EV << "Tham - UpdateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_SLV_WAITCONNECTION with LLID_Data_start_2 packet : Slave received challenge = "<< challenge << endl;

                            IndexCh_data = RxdLLPktCopy->getChallenge_index(); // tham added
                            EV << "Tham - Slave received packet with IndexCh_data = " << IndexCh_data << endl;

                            if(DataPktForNWK){
                                /*PktLeft_RX_Size=DataPktForNWK->getByteLength()-RxdLLPkt->getHdr_lgth();//get length
                                EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_2 received, data to receive in next packets =" << PktLeft_RX_Size << endl;
                                if(PktLeft_RX_Size==0){
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_2 received: forwarding packet to next level" << endl;
                                    sendUp(DataPktForNWK);
                                }
                                else delete DataPktForNWK;*/
                                sendUp(DataPktForNWK);
                            }
                            delete RxdLLPktCopy;//

                        }
                        else if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_frag_1){//fragment
                            //if(PktLeft_RX_Size>=0){//KVM
                                EV<< "BLEMacV2::updateStatusConnected - LLID_1 received" << endl;
                                eventDataNewRXd(RxdLLPkt->getHdr_lgth());
                                PktLeft_RX_Size=PktLeft_RX_Size-RxdLLPkt->getHdr_lgth();//get length
                                EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_1 received! PktLeft_RX_Size= " << PktLeft_RX_Size << " Received packet length= " <<RxdLLPkt->getHdr_lgth() << " Data left=" << PktLeft_RX_Size-RxdLLPkt->getHdr_lgth() << endl;

                                //if(PktLeft_RX_Size<0)error("BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_1 received PktLeft_RX_Size<0");
                                //else if(PktLeft_RX_Size==0){
                                /*if(PktLeft_RX_Size<=0){
                                    PktLeft_RX_Size=0;
                                    BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                                    RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error
                                    DataPktForNWK = RxdLLPktCopy->decapsulate();
                                    if(DataPktForNWK){
                                        EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_1 received: forwarding packet to next level" << endl;
                                        sendUp(DataPktForNWK);
                                    }
                                    else delete DataPktForNWK;
                                    delete RxdLLPktCopy;//
                                }*/

                                BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                                RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error
                                DataPktForNWK = RxdLLPktCopy->decapsulate();

                                double challenge = RxdLLPktCopy->getChallenge_data();
                                EV << "Tham - UpdateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_SLV_WAITCONNECTION with LLID_Data_frag_1 packet: Slave received challenge = "<< challenge << endl;

                                IndexCh_data = RxdLLPktCopy->getChallenge_index();
                                EV << "Tham - Slave received packet with IndexCh_data = " << IndexCh_data << endl;

                                if(DataPktForNWK){
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) LLID_1 received: forwarding packet to next level" << endl;
                                    sendUp(DataPktForNWK);
                                }
                                else
                                    delete DataPktForNWK;
                                    delete RxdLLPktCopy;
                            //}
                        }

                    }


                    if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_ctrl_3){//this is a command
                        if(RxdLLPkt->getOpcode()==BLEstructs::LL_CHANNEL_MAP_REQ){//we should chnage the channel map
                            //check the validity - see p. 2552 of spec v4.1
                            if(abs(RxdLLPkt->getInstant()-myConnPars.ConnInfo.connEventCounter)>32767) error("break connection");
                            if(debug_Internal_Messages) EV << "LL_CHANNEL_MAP_REQ received, new map will be applied at instance "<< RxdLLPkt->getInstant()<< endl;
                            myConnPars.ConnInfo.Instant_NewMap=RxdLLPkt->getInstant();
                            myConnPars.ConnInfo.new_Data_Channel_Map.fill(
                                    RxdLLPkt->getMapCh0to7(),
                                    RxdLLPkt->getMapCh8to15(),
                                    RxdLLPkt->getMapCh16to23(),
                                    RxdLLPkt->getMapCh24to31(),
                                    RxdLLPkt->getMapCh32to39()
                            );
                        }
                        else if(RxdLLPkt->getOpcode()==BLEstructs::LL_CONNECTION_UPDATE_REQ){
                            //check the validity - see p. 2550 of spec v4.1
                             if(abs(RxdLLPkt->getInstant()-myConnPars.ConnInfo.connEventCounter)>32767) error("break connection");
                             if(debug_Internal_Messages) EV << "LL_CONNECTION_UPDATE_REQ received, new parameters will be applied at instance "<< RxdLLPkt->getInstant()<< endl;
                             myConnPars.ConnInfo.Instant_NewParameters=RxdLLPkt->getInstant();
                             myConnPars.ConnInfo.new_connInterval=RxdLLPkt->getInterval();
                             myConnPars.ConnInfo.new_connSlaveLatency=RxdLLPkt->getLatency();
                             myConnPars.ConnInfo.new_connSupervisionTimeout=RxdLLPkt->getTimeout();
                             myConnPars.ConnInfo.transmitWindowOffset=RxdLLPkt->getWinOffset();
                             myConnPars.ConnInfo.transmitWindowSize=RxdLLPkt->getWinSize();
                        }
                        else if(RxdLLPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
                            myCmdError->ErrorCode=RxdLLPkt->getErrorCode();
                            Flag_PeerTerminatesConnection=true;

                            if(prepareEmptyACKPkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending ACK" << endl;
                            }
                            else{//not enough time - switch to next channel
                                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                scheduleAt(simTime(), slaveEndConnectionEvent);
                            }
                        }
                        else{
                            error("BLEMacV2::updateStatusConnected: received unknown data channel command!");
                        }
                    }

                    if(Flag_PeerTerminatesConnection==false){
                        if(myConnPars.ConnInfo.transmitSeqNum==RxdLLPkt->getHdr_NESN()){
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) NESN==transmitSeqNum - I must resend previous PDU" << endl;
                            if(checkCurrentDataPkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                                startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - resending data PDU" << endl;
                            }
                            else{//not enough time - switch to next channel
                                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                scheduleAt(simTime(), slaveEndConnectionEvent);
                            }
                        }
                        else{
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected (CONNECTED_SLV_WAITCONNECTION) NESN!=transmitSeqNum - I can send a new PDU" << endl;
                            if(DataPkt_FirstFragmentSent==true){
                                DataPkt_FirstFragment=false;
                                DataPkt_FirstFragmentSent=false;
                            }
                            myConnPars.ConnInfo.transmitSeqNum=!myConnPars.ConnInfo.transmitSeqNum;
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected Flag_HighPriorityTraffic=" << Flag_HighPriorityTraffic << " Flag_UpdatePending_TerminateConnection=" << Flag_UpdatePending_TerminateConnection << endl;

                            if(Flag_HighPriorityTraffic==true){
                                if(HighPriorityDataPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                                    //got acknowledgment for my connection termination
                                    dropConnection();
                                }
                                else{
                                    error("BLEMacV2::updateStatusConnected - unsupported case!");
                                    //TODO: other cases
                                }
                                Flag_HighPriorityTraffic=false; //clear flag
                            }
                            else{
                                if(Flag_UpdatePending_TerminateConnection==true){//connection terminate
                                    if(prepareConnTerminatePkt(AvailableTime)==true){//enough time
                                        simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                        if(FreeTime==0) eventRadioStateChanged();
                                        if(debug_Internal_Messages)
                                            EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                        startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending Terminate Connection" << endl;
                                     }
                                     else{
                                         if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                          scheduleAt(simTime(), slaveEndConnectionEvent);
                                    }
                                }
                                else{
                                    if((checkBuffer(LastPacketPayload)==true)
                                            ||(myConnPars.ConnInfo.moreData==true)
                                            ||((beacon_packet==true)&&(llSlave_beacon_ReplyPolicy==ALWAYS_1))
                                            ){
                                        if(prepareNewDataPkt(AvailableTime)==true){//enough time
                                            simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                            if(FreeTime==0) eventRadioStateChanged();
                                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                            startTimer(TIMER_CONNECTION_SLAVE_IFS);
                                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending a data PDU" << endl;
                                        }
                                        else{//not enough time - switch to next channel
                                            prepareNewDataPkt(1);//prepare next packet
                                            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected not enough time for TX - closing event" << endl;
                                            scheduleAt(simTime(), slaveEndConnectionEvent);
                                        }
                                    }
                                    else{
                                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected no need to send reply - close event & postprone packet generation till event's start" << endl;
                                        scheduleAt(simTime(), slaveEndConnectionEvent);
                                        myConnPars.ConnInfo.delayedGeneration=true;
                                    }
                                }
                            }
                        }
                    }
                    //delete msg;
                }
                else{//not for me

                      if(debug_Pkt_Basic) EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_SLV_WAITPKT). SLAVE GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" NOT MATCHES MINE! which is: "<< myConnPars.ConnInfo.accessAddr << "Packet dropped! " <<endl;

                  // delete msg;
                }

                 delete msg; // Tham - why it is here????

            }  //----------------------------- end of if (CONNECTED_SLV_WAITCONNECTION)----------------------------

//for MASTER recieved packet (PACKET RX)

           else if (macSubState==CONNECTED_MST_WAITRSP){

                BLE_Data_MacPkt*  RxdLLPkt = static_cast<BLE_Data_MacPkt *> (msg);


                if(debug_Pkt_Basic){
                    EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_MST_WAITRSP). MASTER GOT A DATA PDU. AccessAddress=" <<
                    RxdLLPkt->getAccessAddress() <<" my Address=" << myConnPars.ConnInfo.accessAddr <<endl;
                }

                // Check if packet is for me
                if(RxdLLPkt->getAccessAddress()==myConnPars.ConnInfo.accessAddr){//for me
                    //DEBUG
                    if(debug_Pkt_Basic){
                        EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_MST_WAITRSP). MASTER GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" Packet Length=" << RxdLLPkt->getBitLength()
                                << " seqID="<< RxdLLPkt->getSequenceId() << " MD=" << RxdLLPkt->getHdr_MD() << " SN=" << RxdLLPkt->getHdr_SN()
                                << " NESN=" << RxdLLPkt->getHdr_NESN() <<endl;
                    }

                    simtime_t AvailableTime = calculateNextDataEventTime()-llIFS-llIFS-simTime();//Explanation: take left time till next


                    if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_ctrl_3){ // control packet - tham
                        cancelEvent(masterConnectionSupervisionTimer);//stop timer
                        cancelEvent(masterWaitReplyTimer);//stop timer
                        startTimer(TIMER_CONNECTION_MASTER_SUPERVISION);

                        if(RxdLLPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
                            myCmdError->ErrorCode=RxdLLPkt->getErrorCode();
                            Flag_PeerTerminatesConnection=true;

                            if(myConnPars.ConnInfo.nextExpectedSeqNum!=RxdLLPkt->getHdr_SN()){
                            }
                            else{
                                if(prepareEmptyACKPkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages)
                                        EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages)
                                        EV<< "BLEMacV2::updateStatusConnected enough time - sending ACK" << endl;
                                }
                                else{//not enough time
                                    if(debug_Internal_Messages)
                                        EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                        scheduleAt(simTime(), masterEndConnectionEvent);
                                        //proposed overlap monitoring mechanisms
                                        if(TST_OverlapMonitoringMechanism==true){
                                             ConnectionMonitoring_ConnectionSuccess();
                                         }
                                }
                                myConnPars.ConnInfo.nextExpectedSeqNum=!myConnPars.ConnInfo.nextExpectedSeqNum;
                            }
                        }
                    }

                    if(RxdLLPkt->getHdr_LLID()!=BLEstructs::LLID_Data_ctrl_3){//not a command
                        myConnPars.ConnInfo.moreData=RxdLLPkt->getHdr_MD();
                        cancelEvent(masterConnectionSupervisionTimer);//stop timer
                        startTimer(TIMER_CONNECTION_MASTER_SUPERVISION);

                      if(myConnPars.ConnInfo.numMissedEvents){//first packet - update connection status
                            myConnPars.ConnInfo.timeLastSuccesfullEvent=myConnPars.ConnInfo.timeLastSuccesfullEvent+(myConnPars.ConnInfo.numMissedEvents)*myConnPars.ConnInfo.connInterval*0.00125;
                            myConnPars.ConnInfo.numMissedEvents=0;
                          if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected New timeLastSuccesfullEvent=" << myConnPars.ConnInfo.timeLastSuccesfullEvent <<" numMissedEvents=" << myConnPars.ConnInfo.numMissedEvents << endl;
                        }

                        //EV<< "nextExpectedSeqNum=" << myConnPars.ConnInfo.nextExpectedSeqNum << " SN=" << RxdLLPkt->getHdr_SN() << endl;
                        //EV<< "transmitSeqNum=" << myConnPars.ConnInfo.transmitSeqNum << " NESN=" << RxdLLPkt->getHdr_NESN() << endl;

                        cancelEvent(masterWaitReplyTimer);//stop timer

              //check packet
                        if(myConnPars.ConnInfo.nextExpectedSeqNum!=RxdLLPkt->getHdr_SN()){
                            eventDataReTXRXd(RxdLLPkt->getHdr_lgth());
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected SN!=nextExpectedSeqNum - this is a resent PDU" << endl;
                        }
                        else{
                            eventDataNewRXd(RxdLLPkt->getHdr_lgth());
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected SN==nextExpectedSeqNum - this is a new PDU" << endl;
                            EV<<"PktLeft_RX_Size="<<PktLeft_RX_Size<<endl;
                            myConnPars.ConnInfo.nextExpectedSeqNum=!myConnPars.ConnInfo.nextExpectedSeqNum;

                            if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_start_2){//start packet of a sequence or whole packet

                                EV<< "LLID_2 received" << endl;
                                eventDataNewRXd(RxdLLPkt->getHdr_lgth());

                                BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy

                                RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error

                                DataPktForNWK = RxdLLPktCopy->decapsulate();

                                if(DataPktForNWK){
                                    /*PktLeft_RX_Size=DataPktForNWK->getByteLength()-RxdLLPkt->getHdr_lgth();//get length
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_2 received, data to receive in next packets =" << PktLeft_RX_Size << endl;
                                    if(PktLeft_RX_Size==0){
                                        EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_2 received: forwarding packet to next level" << endl;
                                        sendUp(DataPktForNWK);
                                    }
                                    else delete DataPktForNWK;*/
                                    sendUp(DataPktForNWK);
                                }

                                double response = RxdLLPktCopy->getResponse_data();

                                EV <<"Tham - BLEMacV2::updateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: LLID_Data_start_2: Master received response = " << response << endl;

                                delete RxdLLPktCopy;//
                            }
                            else if(RxdLLPkt->getHdr_LLID()==BLEstructs::LLID_Data_frag_1){ //fragment
                                /*
                                if(PktLeft_RX_Size>=0){//KVM

                                    eventDataNewRXd(RxdLLPkt->getHdr_lgth());
                                    //EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_1 received!" << endl;
                                    PktLeft_RX_Size=PktLeft_RX_Size-RxdLLPkt->getHdr_lgth();//get length
                                    EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_1 received! PktLeft_RX_Size=" << PktLeft_RX_Size << "Received packet length=" <<RxdLLPkt->getHdr_lgth() << "Data left=" << PktLeft_RX_Size-RxdLLPkt->getHdr_lgth() << endl;
                                    //if(PktLeft_RX_Size<0)error("BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_1 received PktLeft_RX_Size<0");

                                    if(PktLeft_RX_Size<=0){
                                        PktLeft_RX_Size=0;
                                        BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy
                                        RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error
                                        DataPktForNWK = RxdLLPktCopy->decapsulate();
                                        if(DataPktForNWK){
                                            EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_1 received: forwarding packet to next level" << endl;
                                            sendUp(DataPktForNWK);
                                        }
                                        else delete DataPktForNWK;
                                        delete RxdLLPktCopy;//
                                    }
                                }
                                */
                                eventDataNewRXd(RxdLLPkt->getHdr_lgth());

                                BLE_Data_MacPkt*  RxdLLPktCopy = RxdLLPkt->dup();//make a copy

                                RxdLLPktCopy->setByteLength(1000000);//a lot to get rid of the decaps error

                                double response = RxdLLPktCopy->getResponse_data();

                                EV <<"Tham - BLEMacV2::updateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: LLID_Data_frag_1 : Master received response = " << response << endl;

                               // EV << "Tham updateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: index_T= " << index_T <<endl;
                       //trust computation
                                EV << "Tham - Trust computation from DATA pkt" << endl;
                                int index_rc_data = RxdLLPktCopy->getResponseIndex_data();
                                EV << "challenge data = " << challenge_data << endl;

                                // temp_res = RxdAdvPkt->getScanRspData();
                                double p_cr_data = (double)(response*challenge_data)/(double)(prob_r_on_c[0][index_rc_data]*prob_c[0]+prob_r_on_c[1][index_rc_data]*prob_c[1]+prob_r_on_c[2][index_rc_data]*prob_c[2]);

                                T_inst[index_T] = p_cr_data - challenge_data;

                                T_init[index_T] = T_init[index_T - 1]*0.3 + T_inst[index_T]*0.7;

                              /*  EV << "index_T = " << index_T << endl;

                                if (index_T > 0) {
                                T_init[index_T] = T_init[index_T - 1]*0.3 + T_inst[index_T]*0.7;
                                } else {
                                    T_init[index_T] = T_inst[index_T];
                                }
                              */

                                response_list[index_T] = index_rc_data;

                                EV << "response_list["<< index_T <<"]= "<< response_list[index_T]<<endl;

                                //Address_device[0] = RxdAdvPkt->getAdvA();
                                Trust_device[0] = T_init[index_T];

                                EV << "at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: response_list["<< index_T<<"] ="<< response_list[index_T]<<endl;
                                EV << "at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: challenge_list["<< index_T<<"] ="<< challenge_list[index_T]<<endl;
                                EV << "Tham updateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP: initial trust at round [" <<index_T+1<<"]= " << T_init[index_T]<<endl;
                       // end trust computation

                                DataPktForNWK = RxdLLPktCopy->decapsulate();

                                if(DataPktForNWK){
                                  EV<< "BLEMacV2::updateStatusConnected (CONNECTED_MST_WAITRSP) LLID_1 received: forwarding packet to next level" << endl;
                                  sendUp(DataPktForNWK);
                                }
                                else delete DataPktForNWK;

                                delete RxdLLPktCopy;
                            }
                        }

                        //EV<< "nextExpectedSeqNum=" << myConnPars.ConnInfo.nextExpectedSeqNum << " SN=" << RxdLLPkt->getHdr_SN() << endl;
                        //EV<< "transmitSeqNum=" << myConnPars.ConnInfo.transmitSeqNum << " NESN=" << RxdLLPkt->getHdr_NESN() << endl;


                         //connection event and deduct llIFS(wait before TX)
                        //& once more llIFS - security interval before connection event start according to the standard

                        if(myConnPars.ConnInfo.transmitSeqNum==RxdLLPkt->getHdr_NESN()){ // resend previous PDU
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected NESN==transmitSeqNum - I must resend previous PDU" << endl;
                            if(Flag_HighPriorityTraffic==true){
                                if(checkHighPriorityDataPkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending high priority PDU" << endl;
                                }
                                else{//not enough time
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                       scheduleAt(simTime(), masterEndConnectionEvent);
                                       //proposed overlap monitoring mechanisms
                                       if(TST_OverlapMonitoringMechanism==true){
                                           ConnectionMonitoring_ConnectionSuccess();
                                       }
                                }

                            }
                            else{ // not high priority packet
                                if(checkCurrentDataPkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending new data PDU" << endl;
                                }
                                else{//not enough time
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                        scheduleAt(simTime(), masterEndConnectionEvent);
                                        //proposed overlap monitoring mechanisms
                                        if(TST_OverlapMonitoringMechanism==true){
                                            ConnectionMonitoring_ConnectionSuccess();
                                        }
                                }
                            }
                        }
                        else{ // send a new PDU
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected NESN!=transmitSeqNum - I can send a new PDU" << endl;

                            if(DataPkt_FirstFragmentSent==true){
                                DataPkt_FirstFragment=false;
                                DataPkt_FirstFragmentSent=false;
                            }

                            myConnPars.ConnInfo.transmitSeqNum=!myConnPars.ConnInfo.transmitSeqNum;

                            // check high priority data
                            if(Flag_HighPriorityTraffic==true){
                                if(HighPriorityDataPkt->getOpcode()==BLEstructs::LL_TERMINATE_IND){
                                    //got acknowledgment for my connection termination
                                    //dropConnection();
                                    scheduleAt(simTime(), ctrl_terminateConnection);
                                }
                                else if(HighPriorityDataPkt->getOpcode()==BLEstructs::LL_CHANNEL_MAP_REQ){
                                    Flag_UpdatePending_DataChannelMap=false;
                                }
                                else if(HighPriorityDataPkt->getOpcode()==BLEstructs::LL_CONNECTION_UPDATE_REQ){
                                    Flag_UpdatePending_ConnectionPars=false;
                                }
                                else{
                                    error("BLEMacV2::updateStatusConnected - unsupported case!");
                                    //TODO: other cases
                                }

                                Flag_HighPriorityTraffic=false; //clear flag
                            }

                            //else{
                            if(Flag_UpdatePending_TerminateConnection==true){//connection terminate
                                if(prepareConnTerminatePkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending new Data Channel Map" << endl;
                                 }
                                 else{
                                     if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                          scheduleAt(simTime(), masterEndConnectionEvent);
                                           //proposed overlap monitoring mechanisms
                                           if(TST_OverlapMonitoringMechanism==true){
                                               ConnectionMonitoring_ConnectionSuccess();
                                           }
                                }
                            }
                            else if(Flag_UpdatePending_DataChannelMap==true){
                                if(prepareChannelMapUpdatePkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending new Data Channel Map" << endl;
                                }
                                else{
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                        scheduleAt(simTime(), masterEndConnectionEvent);
                                        //proposed overlap monitoring mechanisms
                                        if(TST_OverlapMonitoringMechanism==true){
                                            ConnectionMonitoring_ConnectionSuccess();
                                        }
                                }
                            }
                            else if(Flag_UpdatePending_ConnectionPars==true){
                                //PktLeft_TX_Size=PktLeft_TX_Size+DataPkt->getHdr_lgth();//KVM
                                //Flag_DataPktPending=true;
                                if(prepareConnectionUpdatePkt(AvailableTime)==true){//enough time
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    startTimer(TIMER_CONNECTION_MASTER_IFS);
                                    updateMacSubstate(CONNECTED_MST_WAITRSP);
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending new Connection pars" << endl;
                                }
                                else{
                                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                        scheduleAt(simTime(), masterEndConnectionEvent);
                                        //proposed overlap monitoring mechanisms
                                        if(TST_OverlapMonitoringMechanism==true){
                                            ConnectionMonitoring_ConnectionSuccess();
                                        }
                                }
                            }

                            // Check not high priority data
                            if(Flag_HighPriorityTraffic==false){
                                EV<< "BLEMacV2::updateStatusConnected Flag_HighPriorityTraffic=false, LastPacketPayload=" << LastPacketPayload << endl;
                                if((checkBuffer(LastPacketPayload)==true)||(myConnPars.ConnInfo.moreData==true)){
                                    if(prepareNewDataPkt(AvailableTime)==true){//enough time

                                        simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0

                                        if(FreeTime==0) eventRadioStateChanged();
                                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                                        startTimer(TIMER_CONNECTION_MASTER_IFS);
                                        updateMacSubstate(CONNECTED_MST_WAITRSP);

                                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected enough time - sending new data PDU" << endl;
                                    }
                                    else{
                                        EV<< "GENERATING PACKET FOR NEXT EVENT!" << endl;
                                        //prepareNewDataPkt(1);//prepare next packet
                                        prepareNewDataPkt_Master(1);
                                        EV << "Tham - UpdateStatusConnected at EV_FRAME_RECEIVED and CONNECTED_MST_WAITRSP - Master prepares new data packet " << endl;
                                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                                           scheduleAt(simTime(), masterEndConnectionEvent);
                                           //proposed overlap monitoring mechanisms
                                           if(TST_OverlapMonitoringMechanism==true){
                                               ConnectionMonitoring_ConnectionSuccess();
                                           }
                                     }
                                }
                                else{
                                    if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected no need to send reply - close event & postprone packet generation till event's start" << endl;
                                        scheduleAt(simTime(), masterEndConnectionEvent);
                                        //proposed overlap monitoring mechanisms
                                        if(TST_OverlapMonitoringMechanism==true){
                                            ConnectionMonitoring_ConnectionSuccess();
                                        }
                                    myConnPars.ConnInfo.delayedGeneration=true;
                                }
                           }
                        }
                    }
                    //delete msg;

                    if (num_data_pkt_rcved >= input_data_round) {

                                           t_end = simTime();
                                           endSimulation();

                                       }

                }
                else{//packet is not for me

                    if(debug_Pkt_Basic) EV<< "BLEMacV2:updateStatusConnected on EV_FRAME_RECEIVED(CONNECTED_MST_WAITRSP). MASTER GOT A DATA PDU. AccessAddress=" << RxdLLPkt->getAccessAddress() <<" NOT MATCHES MINE! Packet dropped! " <<endl;
                   // delete msg;
                }

                delete msg; // ????


           } // end of if (macSubState==CONNECTED_MST_WAITRSP)

    }

    break;

//------------------------------------ end case EV_FRAME_RECEIVED --------------------------------------

        case EV_FRAME_TRANSMITTED:
        {
            num_data_pkt_sent++;

          //  if (num_data_pkt_sent > input_data_round) endSimulation();

            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_FRAME_TRANSMITTED" << endl;

            if(macSubState==CONNECTED_MST_WAITRSP){
                if(Flag_PeerTerminatesConnection==true){
                    //I have acknowledged connection termination
                    //dropConnection();
                    scheduleAt(simTime(), ctrl_terminateConnection);
                    Flag_PeerTerminatesConnection=false;
                }
                else if((checkBuffer(0)==true)||(myConnPars.ConnInfo.moreData==true)||(Flag_HighPriorityTraffic==true)){
                    if(startTimer(TIMER_CONNECTION_MASTER_WAITREPLY)==false){//not enough time
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected(CONNECTED_MST_WAITRSP) not enough time for reply - closing event" << endl;
                        scheduleAt(simTime(), masterEndConnectionEvent);
                           //proposed overlap monitoring mechanisms
                        if(TST_OverlapMonitoringMechanism==true){
                              ConnectionMonitoring_ConnectionSuccess();
                           }
                    }
                    else{
                        simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                        if(FreeTime==0) eventRadioStateChanged();
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    }
                }
                else{
                    if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected(CONNECTED_MST_WAITRSP) not enough time for reply - closing event" << endl;
                    scheduleAt(simTime(), masterEndConnectionEvent);
                        //proposed overlap monitoring mechanisms
                    if(TST_OverlapMonitoringMechanism==true){
                        ConnectionMonitoring_ConnectionSuccess();
                        }
                    }

            } // end if connected master wait response packet

            else if(macSubState==CONNECTED_SLV_WAITPKT){
                if(Flag_PeerTerminatesConnection==true){
                    //I have acknowledged connection termination
                    scheduleAt(simTime(), ctrl_terminateConnection);
                    Flag_PeerTerminatesConnection=false;
                }
                else if((checkBuffer(0)==true)||(myConnPars.ConnInfo.moreData==true)||(Flag_HighPriorityTraffic==true)){
                    if(startTimer(TIMER_CONNECTION_SLAVE_REPLY)==false){//not enough time
                        if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected(CONNECTED_SLV_WAITPKT) not enough time for reply - closing event" << endl;
                        scheduleAt(simTime(), slaveEndConnectionEvent);
                    }
                    else{
                        simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                        if(FreeTime==0) eventRadioStateChanged();
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    }
                }
                else{
                    if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected(CONNECTED_SLV_WAITPKT) no need to wait for reply - closing event" << endl;
                    scheduleAt(simTime(), slaveEndConnectionEvent);
                }
            } // end if connected Slave wait packet
        }
            break;

//----------------------------- end case EV_FRAME_TRANSMITED ------------------------------------------------

         // from here is for Slave //
        case EV_CON_SLV_WAKEUP_CONNECTION:
            {

                bool Flag_RunConnectionParametersUpdate=false;
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_WAKEUP_CONNECTION" << endl;

                myConnPars.ConnInfo.increaseConnEventCounter();
                myConnPars.ConnInfo.numMissedEvents++;

                if(debug_Internal_Messages) EV << "New connection event with number: " <<  myConnPars.ConnInfo.connEventCounter << endl;

                if(myConnPars.ConnInfo.Instant_NewMap>-1){
                    if(myConnPars.ConnInfo.Instant_NewMap==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        instantUpdateChannelMap();
                    }
                }
                if(myConnPars.ConnInfo.Instant_NewParameters>-1){
                    if(myConnPars.ConnInfo.Instant_NewParameters==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        Flag_RunConnectionParametersUpdate=true;
                    }
                }

                if(Flag_RunConnectionParametersUpdate){
                    instantUpdateConnectionPars();
                }
                else{
                    //find required channel and init RX
                    int ChIdx=myConnPars.ConnInfo.getDataChannel();

                    if(debug_ChannelSwitching) EV << "BLEMacV2::updateStatusConnected New ChIdx=" << ChIdx << " starting receive" << endl;
                    phy->setCurrentRadioChannel(ChIdx);
                    if(info_events)eventRadioChannelChanged();
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    startTimer(TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION);
                    startTimer(TIMER_CONNECTION_SLAVE_TRANSMIT_WINDOW);
                }

        }
            break;
// ----------------------------- end case EV_CON_SLV_WAKEUP_CONNECTION ------------------------

        case EV_CON_SLV_TRANSMIT_WINDOW:
        {
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_TRANSMIT_WINDOW" << endl;
                myConnPars.ConnInfo.lastunmappedChannel = myConnPars.ConnInfo.unmappedChannel;
                simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                if(FreeTime==0) eventRadioStateChanged();
                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
        }
            break;
//---------------------------- end case EV_CON_SLV_TRANSMIT_WINDOW ---------------------

        case EV_CON_SLV_WAKEUP:
            {
               // simtime_t AvailableTime = calculateNextDataEventTime()-std::max(myConnPars.ConnInfo.getWindowWidening(calculateNextDataEventTime()),llIFS)-simTime();
               // simtime_t AvailableTime = std::min(calculateNextDataEventTime()-llIFS-simTime(),masterConnectionSupervisionTimer->getArrivalTime()-llIFS-simTime());

              //  BLE_Data_MacPkt * DataPktCopy; //tham

                bool Flag_RunConnectionParametersUpdate=false;
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_WAKEUP" << endl;
                myConnPars.ConnInfo.increaseConnEventCounter();
                myConnPars.ConnInfo.numMissedEvents++;

                if(debug_Internal_Messages) EV << "New connection event with number: " <<  myConnPars.ConnInfo.connEventCounter << endl;
                if(myConnPars.ConnInfo.Instant_NewMap>-1){
                    if(myConnPars.ConnInfo.Instant_NewMap==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        instantUpdateChannelMap();
                    }
                }
                if(myConnPars.ConnInfo.Instant_NewParameters>-1){
                    if(myConnPars.ConnInfo.Instant_NewParameters==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        Flag_RunConnectionParametersUpdate=true;
                    }
                }

                if(Flag_RunConnectionParametersUpdate){
                    instantUpdateConnectionPars();
                }
                else{
                    int ChIdx=myConnPars.ConnInfo.getDataChannel();

                    if(debug_ChannelSwitching) EV << "BLEMacV2::updateStatusConnected New ChIdx=" << ChIdx << ". Starting receive" << endl;

                    phy->setCurrentRadioChannel(ChIdx);

                    if(info_events)eventRadioChannelChanged();

                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();

                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                    updateMacSubstate(CONNECTED_SLV_WAITPKT);
                    startTimer(TIMER_CONNECTION_SLAVE_NOBCN);

                    if(Flag_HighPriorityTraffic==false){ // not high priority data

                  ////////////////////////////////////////////////////////////
                        if(myConnPars.ConnInfo.delayedGeneration==true){

                            prepareNewDataPkt(1);//prepare next packet

                            EV << "Tham - Update Status Connected at EV_CON_SLV_WAKEUP Slave prepare new packet" << endl;

                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected trying to generate a new packet" << endl;
                        }
                 //////////////////////////////////////////////////////////////

                       /* if(checkCurrentDataPkt(AvailableTime)==true){//enough time

                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending data PDU" << endl;

                            DataPktCopy=DataPkt->dup();
                            DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                            DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1


                            //DataPktCopy->setResponse_data(0.1); // Tham

                          // int ResponseIndex_data = rand() % 4;



                               int arr[] = {1, 2, 3, 4};
                               int freq_rc1[] = {51, 34, 2, 13};
                               int freq_rc2[] = {50, 36, 1, 13};
                               int freq_rc3[] = {50, 31, 6, 13};

                               srand(time(NULL));

                               int n = sizeof(arr) / sizeof(arr[0]);

                               if (IndexCh_data == 0){
                                   ResponseIndex_data = myRand(arr, freq_rc1, n) - 1;

                              } else if (IndexCh_data == 1){
                                  ResponseIndex_data = myRand(arr, freq_rc2, n) - 1;

                                     } else {
                                         ResponseIndex_data = myRand(arr, freq_rc3, n) - 1;
                                }




                           ///////////////////////////////////////////

                           EV << "challenge index pass here = " << IndexCh_data << endl;
                           EV << "Selected response is at [" << IndexCh_data <<"][" << ResponseIndex_data <<"]" << endl;
                           EV << "Response in DATA packet = "<< prob_r_on_c[IndexCh_data][ResponseIndex_data] << endl;

                           double Response_in_dataPKt = prob_r_on_c[IndexCh_data][ResponseIndex_data]; //prob_r_on_c[RandIndex];

                           DataPktCopy->setResponse_data(Response_in_dataPKt);

                           EV <<"Tham - Update status connected at EV_CON_SLV_WAKEUP: send a Response = " << DataPktCopy->getResponse_data() << endl;

                           DataPktCopy->setResponseIndex_data(ResponseIndex_data);

                           //attachSignal_Data(DataPktCopy, simTime());
                            //sendDelayed(DataPktCopy, 0, lowerLayerOut);

                           attachSignal_Data(DataPktCopy, simTime());

                            sendDelayed(DataPktCopy, 0, lowerLayerOut);
                       // }

                        else{//not enough time
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time to send - closing event" << endl;
                                scheduleAt(simTime(), slaveEndConnectionEvent);
                        } */

                 /////////////////////////////////////////////////////////////
                    }
                }
        }
            break;

//------------------------ end case EV_CON_SLV_WAKEUP ---------------------------

        case EV_CON_SLV_NOBEACON:
        {
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_NOBEACON - closing event" << endl;
            eventNoRSPTimer();
            scheduleAt(simTime(), slaveEndConnectionEvent);
        }
            break;

//-------------------------end case EV_CON_SLV_NO_BEACON ------------------------

        case EV_CON_SLV_WAITIFS:
        {
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_WAITIFS" << endl;


                simtime_t AvailableTime = calculateNextDataEventTime()-std::max(myConnPars.ConnInfo.getWindowWidening(calculateNextDataEventTime()),llIFS)-simTime();

                BLE_Data_MacPkt * DataPktCopy;

                if(Flag_HighPriorityTraffic==true){ // high priority data
                    if(checkHighPriorityDataPkt(AvailableTime)==true){//enough time
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending high priority data PDU" << endl;
                        DataPktCopy=HighPriorityDataPkt->dup();
                        DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                        DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1
                        attachSignal_Data(DataPktCopy, simTime());
                        sendDelayed(DataPktCopy, 0, lowerLayerOut);
                    }
                    else{//not enough time
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                            scheduleAt(simTime(), slaveEndConnectionEvent);
                    }
                }
                else{

                    if(myConnPars.ConnInfo.delayedGeneration==true){

                        prepareNewDataPkt(1);//prepare next packet IndexCh_data
                       // prepareNewDataPkt_Slave(1, IndexCh_data);
                        EV << "Tham - Update Status Connected at EV_CON_SLV_WAKEUP Slave prepare new packet" << endl;

                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected trying to generate a new packet" << endl;
                    }


                       // not high priority data
                    if(checkCurrentDataPkt(AvailableTime)==true){//enough time
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending data PDU" << endl;

                        DataPktCopy=DataPkt->dup();
                        DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                        DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1


                        ////////////////////////////////////////////////////////////

                        int arr[] = {1, 2, 3, 4};
                        int freq_rc1[] = {51, 34, 2, 13};
                        int freq_rc2[] = {50, 36, 1, 13};
                        int freq_rc3[] = {50, 31, 6, 13};

                      //  srand(time(NULL));

                        int n = sizeof(arr) / sizeof(arr[0]);

                        if (IndexCh_data == 0){
                            ResponseIndex_data = myRand(arr, freq_rc1, n) - 1;

                        } else if (IndexCh_data == 1){
                            ResponseIndex_data = myRand(arr, freq_rc2, n) - 1;

                        } else {
                            ResponseIndex_data = myRand(arr, freq_rc3, n) - 1;
                        }


                       EV << "challenge index pass here = " << IndexCh_data << endl;
                       EV << "Selected response is at [" << IndexCh_data <<"][" << ResponseIndex_data <<"]" << endl;
                       EV << "Response in DATA packet = "<< prob_r_on_c[IndexCh_data][ResponseIndex_data] << endl;

                       double Response_in_dataPKt = prob_r_on_c[IndexCh_data][ResponseIndex_data]; //prob_r_on_c[RandIndex];

                       DataPktCopy->setResponse_data(Response_in_dataPKt);

                       EV <<"Tham - Update status connected at EV_CON_SLV_WAITIFS: send a Response = " << DataPktCopy->getResponse_data() << endl;

                       DataPktCopy->setResponseIndex_data(ResponseIndex_data);


                       //////////////////////////////////////////////////////////////

                       attachSignal_Data(DataPktCopy, simTime());

                        sendDelayed(DataPktCopy, 0, lowerLayerOut);
                    }
                    else{//not enough time
                        if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time to send - closing event" << endl;
                        scheduleAt(simTime(), slaveEndConnectionEvent);
                    }
                }
        }
            break;

//---------------------------- end case EV_CON_SLV_WAIT_IFS ------------------------

        case EV_CON_SLV_WAITREPLY:
        {
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_NOBEACON" << endl;
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected - no beacon received in time. Ending event." << endl;
                eventNoRSPTimer();
                scheduleAt(simTime(), slaveEndConnectionEvent);
        }
            break;

//--------------------------- end case EV_CON_SLV_WAIT_REPLY -----------------------

        case EV_CON_SLV_ENDEVENT:
        {
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_SLV_ENDEVENT" << endl;
                myConnPars.ConnInfo.lastunmappedChannel=myConnPars.ConnInfo.unmappedChannel;
                if(startTimer(TIMER_CONNECTION_SLAVE_WAKEUP)==false){
                    scheduleAt(simTime(), slaveConnectionWakeupTimer);
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - switch to next channel now" << endl;
                }
                else{
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - starting sleep timer" << endl;
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP); //to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    updateMacSubstate(CONNECTED_SLV_SLEEP);
                }
        }
            break;

//------------------------- end case EV_CON_SLV_END_EVENT ------------------------------

        case EV_CON_SLV_DROPCONNECTION: //on timeout
        {
            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
            myCmdError->ErrorCode=cBLEstructs_Error::CONNECTION_TIMEOUT;
            scheduleAt(simTime(), ctrl_terminateConnection);
        }
        break;

//-------------------------- end case EV_CON_SLV_DROP_CONNECTION -----------------------
        // from here is for Master //

        case EV_CON_MST_WAKEUP:
        {
                bool Flag_RunConnectionParametersUpdate=false;
                if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_WAKEUP" << endl;

                myConnPars.ConnInfo.moreData=true;//just to omit the connection being closed immediately after beacon
                myConnPars.ConnInfo.increaseConnEventCounter();
                EventStartTimestamp = simTime();

                if(debug_Internal_Messages) EV << "New connection event with number: " <<  myConnPars.ConnInfo.connEventCounter << endl;
                myConnPars.ConnInfo.numMissedEvents++;

                //find required channel and init TX
                if(myConnPars.ConnInfo.Instant_NewMap>-1){
                    if(myConnPars.ConnInfo.Instant_NewMap==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        instantUpdateChannelMap();
                    }
                }
                if(myConnPars.ConnInfo.Instant_NewParameters>-1){
                    if(myConnPars.ConnInfo.Instant_NewParameters==myConnPars.ConnInfo.connEventCounter){//time to change the channel map
                        Flag_RunConnectionParametersUpdate=true;
                    }
                }

                if(Flag_RunConnectionParametersUpdate){
                    instantUpdateConnectionPars();
                }
                else{
                    int ChIdx=myConnPars.ConnInfo.getDataChannel();
                    if(debug_ChannelSwitching) EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_WAKEUP. New ChIdx=" << ChIdx << ". Starting transmit" << endl;
                    phy->setCurrentRadioChannel(ChIdx);
                    if(info_events)eventRadioChannelChanged();


                    //send a packet
                    simtime_t AvailableTime = std::min(calculateNextDataEventTime()-llIFS-simTime(),masterConnectionSupervisionTimer->getArrivalTime()-llIFS-simTime());
                    BLE_Data_MacPkt * DataPktCopy;

                    if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_WAKEUP Flag_HighPriorityTraffic=" << Flag_HighPriorityTraffic <<
                            " Flag_UpdatePending_TerminateConnection="<< Flag_UpdatePending_TerminateConnection <<
                            " Flag_UpdatePending_DataChannelMap=" << Flag_UpdatePending_DataChannelMap <<
                            " Flag_UpdatePending_ConnectionPars=" << Flag_UpdatePending_ConnectionPars <<
                             endl;

                    //check if we need to generate a packet

                    if(Flag_HighPriorityTraffic==true){ // high priority data
                        if(checkHighPriorityDataPkt(AvailableTime)==true){//enough time
                            simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                            if(FreeTime==0) eventRadioStateChanged();
                            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                            DataPktCopy=HighPriorityDataPkt->dup();
                            DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                            DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

                            attachSignal_Data(DataPktCopy, simTime()+Time_llSLEEPtoRX);
                            sendDelayed(DataPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                            updateMacSubstate(CONNECTED_MST_WAITRSP);
                         }
                         else{
                             startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
                             if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time" << endl;
                         }
                    }
                    else{
                        //handle high-priority commands
                        if(0){

                        }
                        /*if(Flag_UpdatePending_TerminateConnection==true){//terminate connection
                            if(prepareConnTerminatePkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                DataPktCopy=HighPriorityDataPkt->dup();
                                DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                                DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1
                                attachSignal_Data(DataPktCopy, simTime()+Time_llSLEEPtoRX);
                                sendDelayed(DataPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                                updateMacSubstate(CONNECTED_MST_WAITRSP);
                             }
                             else{
                                 startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
                                 if(debug_Internal_Messages)
                                     EV<< "BLEMacV2::updateStatusConnected not enough time" << endl;
                            }
                        }
                        else if(Flag_UpdatePending_DataChannelMap==true){//channel map update
                            if(prepareChannelMapUpdatePkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                DataPktCopy=HighPriorityDataPkt->dup();
                                DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                                DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1
                                attachSignal_Data(DataPktCopy, simTime()+Time_llSLEEPtoRX);
                                sendDelayed(DataPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                                updateMacSubstate(CONNECTED_MST_WAITRSP);
                             }
                             else{
                                 startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
                                 if(debug_Internal_Messages)
                                     EV<< "BLEMacV2::updateStatusConnected not enough time" << endl;
                             }
                        }
                        else if(Flag_UpdatePending_ConnectionPars==true){
                            //PktLeft_TX_Size=PktLeft_TX_Size+DataPkt->getHdr_lgth();//KVM
                            Flag_DataPktPending=true;
                            if(prepareConnectionUpdatePkt(AvailableTime)==true){//enough time
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                DataPktCopy=HighPriorityDataPkt->dup();
                                DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                                DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1
                                attachSignal_Data(DataPktCopy, simTime()+Time_llSLEEPtoRX);
                                sendDelayed(DataPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                                updateMacSubstate(CONNECTED_MST_WAITRSP);
                            }
                            else{
                                startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
                                if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected not enough time" << endl;
                            }
                        }*/
                        //high priority stuff cannot be sent after wake-up
                        else{
                               //no high-priority stuff - handle data
                            if(myConnPars.ConnInfo.delayedGeneration==true){
                                //prepareNewDataPkt(1);//prepare next packet
                                prepareNewDataPkt_Master(1); // Tham

                              //  index_T++;

                                EV <<"at EV_CON_MST_WAKEUP: Master send Data packet with challenge : index_T =" << index_T <<endl;
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected trying to generate a new packet" << endl;
                                EV << "Tham Update Status Connected at EV_CON_MST_WAKEUP - Master prepares New Data Packet " << endl;
                               // EV <<"Tham - Update Status Connected at EV_CON_MST_WAKEUP - Master send a data packet with challenge = " <<DataPktCopy->getChallenge_data() <<endl;
                            }

                            if(checkCurrentDataPkt(AvailableTime)==true){//enough time
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending data PDU" << endl;
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                                DataPktCopy=DataPkt->dup();
                                DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                                DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

                                challenge_data = DataPktCopy->getChallenge_data();

                                EV <<"Tham - Update Status Connected at EV_CON_MST_WAKEUP - Master send a data packet with challenge = " <<DataPktCopy->getChallenge_data() <<endl;

                               challenge_list[index_T]= ChallengeIndex_data;
                               EV << "at EV_CON_MST_WAKEUP: Master send Data packet with challenge - challenge_list[" <<index_T <<"]=" <<ChallengeIndex_data << endl;

                               attachSignal_Data(DataPktCopy, simTime()+Time_llSLEEPtoRX);
                                sendDelayed(DataPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                                updateMacSubstate(CONNECTED_MST_WAITRSP);
                            }

                            else{//not enough time
                                startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
                                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time" << endl;
                            }
                        }
                    }
                }
        }
            break;

// ------------------------- end case EV_CON_MST_WAKEUP --------------------------

        case EV_CON_MST_WAITREPLY:
        {
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_WAITREPLY!" << endl;
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected no reply in time - closing event!" << endl;
            eventNoRSPTimer();
            scheduleAt(simTime(), masterEndConnectionEvent);
            //proposed overlap monitoring mechanisms
            if(TST_OverlapMonitoringMechanism==true){
                ConnectionMonitoring_ConnectionClosed();
            }
        }
            break;
// ------------------------------ end case EV_CON_MST_WAIT_REPLY --------------------------

        case EV_CON_MST_WAITIFS:
        {
            if(debug_Internal_Messages) EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_WAITIFS!" << endl;
            simtime_t AvailableTime = calculateNextDataEventTime()-llIFS-simTime();
            BLE_Data_MacPkt * DataPktCopy;

            if(Flag_HighPriorityTraffic==true){
                if(checkHighPriorityDataPkt(AvailableTime)==true)
                { //enough time
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending high priority data PDU" << endl;

                    DataPktCopy=HighPriorityDataPkt->dup();
                    DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                    DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

                    attachSignal_Data(DataPktCopy, simTime());
                    sendDelayed(DataPktCopy, 0, lowerLayerOut);
                }
                else
                { //not enough time
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                    scheduleAt(simTime(), masterEndConnectionEvent);
                       //proposed overlap monitoring mechanisms
                    if(TST_OverlapMonitoringMechanism==true)
                    {
                        ConnectionMonitoring_ConnectionSuccess();
                    }
                }
            }
            else{ // not high priority data
                if(checkCurrentDataPkt(AvailableTime)==true){ //enough time
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected sending data PDU" << endl;

                    DataPktCopy=DataPkt->dup();
                    DataPktCopy->setHdr_NESN(myConnPars.ConnInfo.nextExpectedSeqNum);//see p. 2545 of spec v4.1
                    DataPktCopy->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

                    // generate challenge
                     //  ChallengeIndex_data = rand() % 3;
                     //  EV << "Master in EV_CON_MST_WAIT_IFS : ChallengeIndex_data  = " << ChallengeIndex_data << endl;
                     //  //EV << " BLEMacV2::prepareNewDataPkt_Master: challenge = " << prob_c[ChallengeIndex_data] << endl;
                    //   double Challenge_data = prob_c[ChallengeIndex_data];


                   // DataPktCopy->setChallenge_data(0.5);
                   // DataPktCopy->setChallenge_data(Challenge_data);
                    attachSignal_Data(DataPktCopy, simTime());
                   // index_T++;
                 //   challenge_list[index_T]=ChallengeIndex_data;
                //    EV << "at EV_CON_MST_WAITIFS: Master send Data packet with challenge - challenge_list[" <<index_T <<"]=" <<ChallengeIndex_data << endl;
                //    EV <<"at EV_CON_MST_WAITIFS: Master send Data packet with challenge : index_T =" << index_T <<endl;
               //     EV << "Tham - Update Status Connected at EV_CON_MST_WAITIFS - Master send a data packet with challenge = " <<DataPktCopy->getChallenge_data() <<endl;
                    sendDelayed(DataPktCopy, 0, lowerLayerOut);
                }
                else{//not enough time
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected not enough time - closing event" << endl;
                        scheduleAt(simTime(), masterEndConnectionEvent);
                        //proposed overlap monitoring mechanisms
                        if(TST_OverlapMonitoringMechanism==true)
                        {
                           ConnectionMonitoring_ConnectionSuccess();
                        }
                }
            }
        }
            break;

//------------------------------ end case EV_CON_MST_WAIT_IFS ---------------------
        case EV_CON_MST_ENDEVENT:
        {
            if(debug_Internal_Messages)  EV << "BLEMacV2::updateStatusConnected on EV_CON_MST_ENDEVENT" << endl;
            myConnPars.ConnInfo.lastunmappedChannel=myConnPars.ConnInfo.unmappedChannel;
            phy->setRadioState(MiximRadio::SLEEP);
            startTimer(TIMER_CONNECTION_MASTER_WAKEUP);
            updateMacSubstate(CONNECTED_MST_SLEEP);
        }
            break;

// --------------------------------- end case EV_CON_MST_ENDEVENT -----------------
        case EV_CON_MST_DROPCONNECTION:  //on timeout
        {
            myCmdError->Handle=myConnPars.ConnInfo.connectionHandle;
            myCmdError->ErrorCode=cBLEstructs_Error::CONNECTION_TIMEOUT;
            //dropConnection();
            scheduleAt(simTime(), ctrl_terminateConnection);
        }
            break;

// -------------------------------- end case EV_CON_MST_DROP_CONNECTION -----------------
        default:
            if(debug_Internal_Messages)  EV<<"Event="<< event << " Substate=" << macSubState << endl;
            error("BLEMacV2::updateStatusConnected unsupported event!");
            break;
    }
}


//original
/*
void BLEMacV2::updateStatusAdvertising(t_mac_event event, cMessage *msg){
    BLE_Adv_MacPkt * AdvPktCopy;
    int CurrentAdvChannel;
    int NextAdvChannel;
    switch (event) {
        case EV_ADV_EVENT:
            if(debug_Internal_Messages)
                EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_EVENT. "<< endl;
            if(AdvMessage==NULL) error("BLEMacV2::updateStatusAdvertising: trying advertise with missing advertising message");
            CurrentAdvChannel=myAdvrtPars.GetNextAdvChannel(0);
            if(CurrentAdvChannel==-1){//no free advertisement channels
                error("BLEMacV2::updateStatusAdvertising on EV_ADV_EVENT- no free advertisement channels");
            }
            else{//first channel found
                phy->setCurrentRadioChannel(CurrentAdvChannel);
                if(info_events)eventRadioChannelChanged();
                NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);
                startTimer(TIMER_ADVERTISE_NEXTPDU);
                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                if(FreeTime==0) eventRadioStateChanged();
                if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                AdvPktCopy= AdvMessage->dup();
                attachSignal_Adv(AdvPktCopy, simTime()+Time_llSLEEPtoRX);
                sendDelayed(AdvPktCopy, Time_llSLEEPtoRX, lowerLayerOut);
                if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_EVENT. Sending advertisement on first Adv channel, i.e. " << CurrentAdvChannel << "Next channel is " << NextAdvChannel << endl;
            }
            startTimer(TIMER_ADVERTISE_EVENTSTART);
            startTimer(TIMER_ADVERTISE_EVENTEND);
            break;
        case EV_ADV_NEXTPDU:
            CurrentAdvChannel=phy->getCurrentRadioChannel();
            if(debug_ChannelSwitching) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU. CurrentAdvChannel=" << CurrentAdvChannel << endl;
            CurrentAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);
            if(debug_ChannelSwitching) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU. NextAdvChannel=" << CurrentAdvChannel << endl;

            if(CurrentAdvChannel==-1){//no last advertisement channel
                cancelEvent(advertisementEndEvent);
                scheduleAt(simTime(), advertisementEndEvent);
                if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU. Forcing End Event." << endl;
            }
            else{
                simtime_t SwitchingDelay;
                int CurrentState=phy->getRadioState();

                if(CurrentState==MiximRadio::SLEEP)SwitchingDelay=Time_llSLEEPtoTX;
                else if(CurrentState==MiximRadio::RX)SwitchingDelay=Time_llRXtoTX;
                else error("BLEMacV2::updateStatusAdvertising on EV_ADV_NEXTPDU - unexpected radio state!");
                simtime_t AvailableTime=advertisementEndEvent->getArrivalTime()-simTime()-SwitchingDelay;
                if(checkCurrentAdvPkt(AvailableTime)==true){
                    if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU enough time to send data!" << endl;

                    phy->setCurrentRadioChannel(CurrentAdvChannel);
                    if(info_events)eventRadioChannelChanged();
                    NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);
                    startTimer(TIMER_ADVERTISE_NEXTPDU);

                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                    AdvPktCopy= AdvMessage->dup();
                    attachSignal_Adv(AdvPktCopy, simTime()+SwitchingDelay);
                    sendDelayed(AdvPktCopy, SwitchingDelay, lowerLayerOut);

                    if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU. Sending advertisement on channel " << CurrentAdvChannel << "Next channel is " << NextAdvChannel << endl;

                }
                else{
                    if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU NOT enough time to send data!" << endl;
                    cancelEvent(advertisementEndEvent);
                    scheduleAt(simTime(), advertisementEndEvent);
                    if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU. Forcing End Event." << endl;
                }
                }
            break;
        case EV_ADV_EVENTEND:
            cancelEvent(advertisementNextPDUTimer);
            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusAdvertising on EV_ADV_EVENTEND. Switching to SLEEP " << endl;
            phy->setRadioState(MiximRadio::SLEEP);
            break;
        case EV_FRAME_TRANSMITTED:
            switch(myAdvrtPars.Adv_Type){
            case BLEstructs::ADV_IND_0:
            case BLEstructs::ADV_DIRECT_IND_HDC_1:
            case BLEstructs::ADV_SCAN_IND_2:
            case BLEstructs::ADV_DIRECT_IND_LDC_4:
                {
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;//we might receive something
                }
                break;
            case BLEstructs::ADV_NONCONN_IND_3:
                {
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;//we do not care about receiving
                }
                break;
            }
            break;
        case EV_FRAME_RECEIVED:
            {
                BLE_Adv_MacPkt*  RxdAdvPkt = static_cast<BLE_Adv_MacPkt *> (msg);
                if(debug_Pkt_Basic){
                    EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED. ADV PDU type=" << RxdAdvPkt->getAdv_PDU_type() << " Length=" << RxdAdvPkt->getBitLength() << " bits, Access Address="
                    << RxdAdvPkt->getAccessAddress() << " InitA=" << RxdAdvPkt->getInitA()
                    <<endl;
                }
                if(RxdAdvPkt->getAccessAddress()==0x8E89BED6){ //correct Adv channel access address
                    if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_CONNECT_REQ_5){
                        //should we process connection requests?
                        if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_IND_0){
                            error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - do not know how to process ADV_IND_0");
                        }
                        else if((myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_HDC_1)||(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_LDC_4)){
                            if((RxdAdvPkt->getTxAdd()==myAdvrtPars.Direct_Address_Type)&&(RxdAdvPkt->getRxAdd()==myAdvrtPars.Own_Address_Type)){
                                if((RxdAdvPkt->getAdvA()==myAdvrtPars.OwnAdvAddr)&&(RxdAdvPkt->getInitA()==myAdvrtPars.Direct_Address)){//address ok
                                    if(debug_Internal_Messages)
                                        EV<< "BLEMacV2:updateStatusAdvertising: packet is a CONNECT_REQ for me!" << endl;
                                    //get all connection parameters from the packet
                                    myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getInitA();
                                    if(RxdAdvPkt->getTxAdd()){
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                    }
                                    else{
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                    }
                                    myConnPars.ConnInfo.accessAddr=RxdAdvPkt->getAA();
                                    myConnPars.ConnInfo.transmitWindowOffset=RxdAdvPkt->getWinOffset();
                                    myConnPars.ConnInfo.transmitWindowSize=RxdAdvPkt->getWinSize();
                                    myConnPars.ConnInfo.connInterval=RxdAdvPkt->getInterval();
                                    myConnPars.ConnPars.Conn_Latency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnInfo.connSlaveLatency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnPars.Supervision_Timeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.connSupervisionTimeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.hopIncreasement=RxdAdvPkt->getHop();
                                    myConnPars.ConnInfo.masterSCA=RxdAdvPkt->getSCA();
                                    myConnPars.ConnInfo.slaveSCA=mySCA;
                                    myConnPars.ConnInfo.Data_Channel_Map.fill(
                                            RxdAdvPkt->getMapCh0to7(),
                                            RxdAdvPkt->getMapCh8to15(),
                                            RxdAdvPkt->getMapCh16to23(),
                                            RxdAdvPkt->getMapCh24to31(),
                                            RxdAdvPkt->getMapCh32to39()
                                    );

                                    //print all out
                                    if(debug_Pkt_Basic){
                                        EV << "BLEMacV2::received ConnectionRequest" << endl;
                                        EV << "Adv_PDU_Type=" << RxdAdvPkt->getAdv_PDU_type() << endl;// Tham added PDU
                                        EV << "AdvA=" << RxdAdvPkt->getAdvA() << endl;
                                        EV << "InitA=" << RxdAdvPkt->getInitA() << endl;
                                        EV << "TxAdd=" << RxdAdvPkt->getTxAdd() << endl;
                                        EV << "RxAdd=" << RxdAdvPkt->getRxAdd() << endl;
                                        EV << "Length(bytes)=" << RxdAdvPkt->getLength() << endl;
                                        EV << "CONNECTION PARAMETERS:" << endl;
                                        EV << "AA=" << RxdAdvPkt->getAA() << endl;
                                        EV << "WinOffset=" << RxdAdvPkt->getWinOffset() << endl;
                                        EV << "WinSize=" << RxdAdvPkt->getWinSize() << endl;
                                        EV << "Interval=" << RxdAdvPkt->getInterval() << endl;
                                        EV << "Latency=" << RxdAdvPkt->getLatency() << endl;
                                        EV << "Timeout=" << RxdAdvPkt->getTimeout() << endl;
                                        EV << "MapCh0to7=" << RxdAdvPkt->getMapCh0to7() << endl;
                                        EV << "MapCh8to15=" << RxdAdvPkt->getMapCh8to15() << endl;
                                        EV << "MapCh16to23=" << RxdAdvPkt->getMapCh16to23() << endl;
                                        EV << "MapCh24to31=" << RxdAdvPkt->getMapCh24to31() << endl;
                                        EV << "MapCh32to39=" << RxdAdvPkt->getMapCh32to39() << endl;
                                        EV << "Hop=" << RxdAdvPkt->getHop() << endl;
                                        EV << "SCA=" << RxdAdvPkt->getSCA() << endl;
                                    }


                                    if(debug_StateTransitions)
                                        EV<< "BLEMacV2:updateStatusAdvertising: Switching to CONNECTION state!" << endl;
                                    ctrl_switchState->setKind(ADVERTIZING_to_CONNECTION);
                                    scheduleAt(simTime(), ctrl_switchState);
                                }
                            }
                        }
                    }



                    //error("BONG1");

                    /*if((RxdAdvPkt->getInitA()==myConnPars.ConnInfo.accessAddr)&&(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_CONNECT_REQ_5)){//this is connection request for me
                        EV<< "BLEMacV2:updateStatusAdvertising: packet is a CONNECT_REQ for me!" << endl;
                        if(debug_StateTransitions)
                            EV<< "BLEMacV2:updateStatusAdvertising: Switching to CONNECTION state!" << endl;
                        ctrl_switchState->setKind(ADVERTIZING_to_CONNECTION);
                        scheduleAt(simTime(), ctrl_switchState);
                    }

                    /



                }
            }
            delete msg;
            break;
        default:
            error("BLEMacV2::updateStatusAdvertising unsupported event!");
            break;
    }
}

*/

// tham modified - This function is for the advertiser
void BLEMacV2::updateStatusAdvertising(t_mac_event event, cMessage *msg){
    BLE_Adv_MacPkt * AdvPktCopy;
    int CurrentAdvChannel;
    int NextAdvChannel;

    EV << "Tham BLEMacV2::updateStatusAdvertising - msg = " << *msg << endl;
   // EV << "THam BLEMacV2::updateStatusAdvertising - AdvMessage = " << *AdvMessage << endl;

    switch (event) {
        case EV_ADV_EVENT: // for sending ADV_IND packet

            if(debug_Internal_Messages)   EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_EVENT. "<< endl;
            if(AdvMessage==NULL) error("BLEMacV2::updateStatusAdvertising: trying advertise with missing advertising message");

            CurrentAdvChannel=myAdvrtPars.GetNextAdvChannel(0);

            if(CurrentAdvChannel==-1){//no free advertisement channels
                error("BLEMacV2::updateStatusAdvertising on EV_ADV_EVENT- no free advertisement channels");
            }
            else
            {     //first channel found
                phy->setCurrentRadioChannel(CurrentAdvChannel);

                if(info_events) eventRadioChannelChanged();

                NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);

                startTimer(TIMER_ADVERTISE_NEXTPDU);

                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0

                if(FreeTime==0) eventRadioStateChanged();

                if(debug_Internal_Messages)  EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                AdvPktCopy = AdvMessage->dup();

                EV << "AdvPktCopy->Adv_PDU_type = " <<AdvPktCopy->getAdv_PDU_type()<< endl;
                EV << "AdvPktCopy->Adv_type = " <<AdvPktCopy->getAdv_type() << endl;

                attachSignal_Adv(AdvPktCopy, simTime()+Time_llSLEEPtoRX);
                num_adv_ind++;

                sendDelayed(AdvPktCopy, Time_llSLEEPtoRX, lowerLayerOut);

                if(debug_Internal_Messages)
                EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_EVENT. Sending advertisement on first Adv channel, i.e. " << CurrentAdvChannel << " Next channel is " << NextAdvChannel << endl;
            }

            startTimer(TIMER_ADVERTISE_EVENTSTART);
            startTimer(TIMER_ADVERTISE_EVENTEND);
            break;

        case EV_ADV_NEXTPDU:

            CurrentAdvChannel=phy->getCurrentRadioChannel();

            if(debug_ChannelSwitching)  EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXT_PDU. CurrentAdvChannel=" << CurrentAdvChannel << endl;

            CurrentAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);

            if(debug_ChannelSwitching)  EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXT_PDU. NextAdvChannel=" << CurrentAdvChannel << endl;

            if(CurrentAdvChannel==-1){//no last advertisement channel
                cancelEvent(advertisementEndEvent);
                scheduleAt(simTime(), advertisementEndEvent);
                if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXT_PDU. Forcing End Event." << endl;
            }
            else{
                simtime_t SwitchingDelay;
                int CurrentState=phy->getRadioState();

                if(CurrentState==MiximRadio::SLEEP) SwitchingDelay=Time_llSLEEPtoTX;
                else if(CurrentState==MiximRadio::RX) SwitchingDelay=Time_llRXtoTX;
                else error("BLEMacV2::updateStatusAdvertising on EV_ADV_NEXT_PDU - unexpected radio state!");

                simtime_t AvailableTime=advertisementEndEvent->getArrivalTime() - simTime() - SwitchingDelay;

                if(checkCurrentAdvPkt(AvailableTime)==true){

                    if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXT_PDU enough time to send data!" << endl;

                    phy->setCurrentRadioChannel(CurrentAdvChannel);

                    if(info_events)eventRadioChannelChanged();

                    NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);

                    startTimer(TIMER_ADVERTISE_NEXTPDU);

                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0

                    if(FreeTime==0) eventRadioStateChanged();

                    if(debug_Internal_Messages)  EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                    AdvPktCopy= AdvMessage->dup();

                    EV << "AdvPktCopy->Adv_PDU_type = " <<AdvPktCopy->getAdv_PDU_type() << endl;
                    EV << "AdvPktCopy->Adv_type = " <<AdvPktCopy->getAdv_type() << endl;


                    attachSignal_Adv(AdvPktCopy, simTime()+SwitchingDelay);
                    num_adv_ind++;

                    sendDelayed(AdvPktCopy, SwitchingDelay, lowerLayerOut);

                    if(debug_Internal_Messages)  EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEX_TPDU. Sending advertisement on channel " << CurrentAdvChannel << " Next channel is " << NextAdvChannel << endl;

                }
                else{

                    if(debug_Internal_Messages)  EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXTPDU NOT enough time to send data!" << endl;

                    cancelEvent(advertisementEndEvent);

                    scheduleAt(simTime(), advertisementEndEvent);

                    if(debug_Internal_Messages)  EV<< "BLEMacV2:updateStatusAdvertising on EV_ADV_NEXT_PDU. Forcing End Event." << endl;
                   }
                }
            break;

        case EV_ADV_EVENTEND:

            cancelEvent(advertisementNextPDUTimer);

            if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusAdvertising on EV_ADV_EVENT_END. Switching to SLEEP " << endl;

            phy->setRadioState(MiximRadio::SLEEP);

            break;

//---------for transmitted packet---------
        case EV_FRAME_TRANSMITTED:

            EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED" << endl;
           // EV << "myAdvrtPars.Adv_Type = " << myAdvrtPars.Adv_Type << endl;

            AdvPktCopy= AdvMessage->dup();

            EV << "AdvPktCopy->Adv_PDU_type =" << AdvPktCopy->getAdv_PDU_type() << endl;
            EV << "AdvPktCopy->Adv_type =" << AdvPktCopy->getAdv_type() << endl;
            EV << "Adv Packet Name = " << AdvPktCopy->getAdvA() << endl;
            EV << "Adv Packet Length = " << AdvPktCopy->getLength() << endl;
            EV << "Adv Packet Adv Data = " << AdvPktCopy->getAdvData() << endl;
            EV << "Adv Packet Adv Name= " << AdvPktCopy->getName() << endl;


      switch(myAdvrtPars.Adv_Type){
            case BLEstructs::ADV_IND_0:
                 { // Tham added

                    // num_adv_ind++;

                    // EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with ADV_IND_0" << endl;

                   simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0

                   if(FreeTime==0) eventRadioStateChanged();

              if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;//we might receive something

                 }
            break;

            case BLEstructs::ADV_DIRECT_IND_HDC_1:
               // EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with ADV_DIRECT_IND_HDC_1" << endl;
            case BLEstructs::ADV_SCAN_IND_2:
               // EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with ADV_SCAN_IND_2" << endl;
            case BLEstructs::ADV_DIRECT_IND_LDC_4:
                {
                  //  EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with ADV_DIRECT_IND_LDC_4" << endl;
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;//we might receive something
                }
            break;

            case BLEstructs::SCAN_RSP: // Tham added for when slave transnits Scan response packet
                {
                    //EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with SCAN_RSP" << endl;

                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0

                    if(FreeTime==0) eventRadioStateChanged();

                    if(debug_Internal_Messages) EV<< "Tham - After transmit SCAN_RSP: BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;

                    /*if(macSubState == INITIATING_TXD) {
                            EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_TRANSMITTED after sending SCAN_RSP" << endl;
                            ctrl_switchState->setKind(STANDBY_to_ADVERTIZING);
                            scheduleAt(simTime(), ctrl_switchState);
                          }*/
                }
            break;

            case BLEstructs::ADV_NONCONN_IND_3:
                {
                   // EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_TRANSMITTED with ADV_NONCONN_IND_3" << endl;
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::SLEEP);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages) EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;//we do not care about receiving
                }
            break;

            case BLEstructs::SCAN_REQ:
                EV <<"Tham - Slave does not transmit SCAN_REQ packet" << endl;
            break;

            }

        break;


//------- for Received packet -------------

        case EV_FRAME_RECEIVED:
        {
                BLE_Adv_MacPkt*  RxdAdvPkt = static_cast<BLE_Adv_MacPkt *> (msg);

                if(debug_Pkt_Basic){
                    EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_RECEIVED. ADV PDU type=" << RxdAdvPkt->getAdv_PDU_type() << " Length=" << RxdAdvPkt->getBitLength() << " bits, Access Address="
                    << RxdAdvPkt->getAccessAddress() << " InitA=" << RxdAdvPkt->getInitA()
                    << endl;
                }

                //if correct Adv channel access address
                if(RxdAdvPkt->getAccessAddress() == 0x8E89BED6){
                // if(RxdAdvPkt->getAccessAddress() == 2391391958){ // tham check 8/7/2018
                    if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_CONNECT_REQ_3){

                        //should we process connection requests?
                        //received CONNECT_REQ after sending ADV_IND packet - Tham added
                        if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_IND_0){
                       // error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - do not know how to process ADV_IND_0");
                            if((RxdAdvPkt->getTxAdd()==myAdvrtPars.Direct_Address_Type)&&(RxdAdvPkt->getRxAdd()==myAdvrtPars.Own_Address_Type)){
                            if((RxdAdvPkt->getAdvA()==myAdvrtPars.OwnAdvAddr)&&(RxdAdvPkt->getInitA()==myAdvrtPars.Direct_Address)){//address ok
                            if(debug_Internal_Messages)
                                EV<< "BLEMacV2:updateStatusAdvertising: packet is a CONNECT_REQ for me once I sent ADV_IND!" << endl;
                                    //get all connection parameters from the packet
                                    myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getInitA();
                                    if(RxdAdvPkt->getTxAdd()){
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                    }
                                    else{
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                    }
                                    myConnPars.ConnInfo.accessAddr=RxdAdvPkt->getAA();
                                    myConnPars.ConnInfo.transmitWindowOffset=RxdAdvPkt->getWinOffset();
                                    myConnPars.ConnInfo.transmitWindowSize=RxdAdvPkt->getWinSize();
                                    myConnPars.ConnInfo.connInterval=RxdAdvPkt->getInterval();
                                    myConnPars.ConnPars.Conn_Latency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnInfo.connSlaveLatency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnPars.Supervision_Timeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.connSupervisionTimeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.hopIncreasement=RxdAdvPkt->getHop();
                                    myConnPars.ConnInfo.masterSCA=RxdAdvPkt->getSCA();
                                    myConnPars.ConnInfo.slaveSCA=mySCA;
                                    myConnPars.ConnInfo.Data_Channel_Map.fill(
                                            RxdAdvPkt->getMapCh0to7(),
                                            RxdAdvPkt->getMapCh8to15(),
                                            RxdAdvPkt->getMapCh16to23(),
                                            RxdAdvPkt->getMapCh24to31(),
                                            RxdAdvPkt->getMapCh32to39()
                                    );

                                    //print all out
                                    if(debug_Pkt_Basic){
                                        EV << "BLEMacV2::received ConnectionRequest" << endl;
                                        EV << "Adv_PDU_Type=" << RxdAdvPkt->getAdv_PDU_type() << endl;// Tham added PDU
                                        EV << "Adv_PDU_Name= " << RxdAdvPkt->getName() << endl; // Tham added
                                        EV << "AdvA=" << RxdAdvPkt->getAdvA() << endl;
                                        EV << "InitA=" << RxdAdvPkt->getInitA() << endl;
                                        EV << "TxAdd=" << RxdAdvPkt->getTxAdd() << endl;
                                        EV << "RxAdd=" << RxdAdvPkt->getRxAdd() << endl;
                                        EV << "Length(bytes)=" << RxdAdvPkt->getLength() << endl;
                                        EV << "CONNECTION PARAMETERS:" << endl;
                                        EV << "AA=" << RxdAdvPkt->getAA() << endl;
                                        EV << "WinOffset=" << RxdAdvPkt->getWinOffset() << endl;
                                        EV << "WinSize=" << RxdAdvPkt->getWinSize() << endl;
                                        EV << "Interval=" << RxdAdvPkt->getInterval() << endl;
                                        EV << "Latency=" << RxdAdvPkt->getLatency() << endl;
                                        EV << "Timeout=" << RxdAdvPkt->getTimeout() << endl;
                                        EV << "MapCh0to7=" << RxdAdvPkt->getMapCh0to7() << endl;
                                        EV << "MapCh8to15=" << RxdAdvPkt->getMapCh8to15() << endl;
                                        EV << "MapCh16to23=" << RxdAdvPkt->getMapCh16to23() << endl;
                                        EV << "MapCh24to31=" << RxdAdvPkt->getMapCh24to31() << endl;
                                        EV << "MapCh32to39=" << RxdAdvPkt->getMapCh32to39() << endl;
                                        EV << "Hop=" << RxdAdvPkt->getHop() << endl;
                                        EV << "SCA=" << RxdAdvPkt->getSCA() << endl;
                                    }


                                    if(debug_StateTransitions) EV<< "BLEMacV2:updateStatusAdvertising: Switching to CONNECTION state!" << endl;

                                    ctrl_switchState->setKind(ADVERTIZING_to_CONNECTION);

                                    scheduleAt(simTime(), ctrl_switchState);
                                }
                            }

                        } // recieved CONNECT_REQ after sending ADV_DIRECT_IND packet
                        else if((myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_HDC_1)||(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_LDC_4)){
                            if((RxdAdvPkt->getTxAdd()==myAdvrtPars.Direct_Address_Type)&&(RxdAdvPkt->getRxAdd()==myAdvrtPars.Own_Address_Type)){
                                if((RxdAdvPkt->getAdvA()==myAdvrtPars.OwnAdvAddr)&&(RxdAdvPkt->getInitA()==myAdvrtPars.Direct_Address)){//address ok
                                    if(debug_Internal_Messages)
                                        EV<< "BLEMacV2:updateStatusAdvertising: packet is a CONNECT_REQ for me once I sent ADV_DIRECT_IND!" << endl;
                                    //get all connection parameters from the packet
                                    myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getInitA();
                                    if(RxdAdvPkt->getTxAdd()){
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                    }
                                    else{
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                    }
                                    myConnPars.ConnInfo.accessAddr=RxdAdvPkt->getAA();
                                    myConnPars.ConnInfo.transmitWindowOffset=RxdAdvPkt->getWinOffset();
                                    myConnPars.ConnInfo.transmitWindowSize=RxdAdvPkt->getWinSize();
                                    myConnPars.ConnInfo.connInterval=RxdAdvPkt->getInterval();
                                    myConnPars.ConnPars.Conn_Latency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnInfo.connSlaveLatency=RxdAdvPkt->getLatency();
                                    myConnPars.ConnPars.Supervision_Timeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.connSupervisionTimeout=RxdAdvPkt->getTimeout();
                                    myConnPars.ConnInfo.hopIncreasement=RxdAdvPkt->getHop();
                                    myConnPars.ConnInfo.masterSCA=RxdAdvPkt->getSCA();
                                    myConnPars.ConnInfo.slaveSCA=mySCA;
                                    myConnPars.ConnInfo.Data_Channel_Map.fill(
                                            RxdAdvPkt->getMapCh0to7(),
                                            RxdAdvPkt->getMapCh8to15(),
                                            RxdAdvPkt->getMapCh16to23(),
                                            RxdAdvPkt->getMapCh24to31(),
                                            RxdAdvPkt->getMapCh32to39()
                                    );

                                    //print all out
                                    if(debug_Pkt_Basic){
                                        EV << "BLEMacV2::received ConnectionRequest" << endl;
                                        EV << "Adv_PDU_Type=" << RxdAdvPkt->getAdv_PDU_type() << endl;// Tham added PDU
                                        EV << "AdvA=" << RxdAdvPkt->getAdvA() << endl;
                                        EV << "InitA=" << RxdAdvPkt->getInitA() << endl;
                                        EV << "TxAdd=" << RxdAdvPkt->getTxAdd() << endl;
                                        EV << "RxAdd=" << RxdAdvPkt->getRxAdd() << endl;
                                        EV << "Length(bytes)=" << RxdAdvPkt->getLength() << endl;
                                        EV << "CONNECTION PARAMETERS:" << endl;
                                        EV << "AA=" << RxdAdvPkt->getAA() << endl;
                                        EV << "WinOffset=" << RxdAdvPkt->getWinOffset() << endl;
                                        EV << "WinSize=" << RxdAdvPkt->getWinSize() << endl;
                                        EV << "Interval=" << RxdAdvPkt->getInterval() << endl;
                                        EV << "Latency=" << RxdAdvPkt->getLatency() << endl;
                                        EV << "Timeout=" << RxdAdvPkt->getTimeout() << endl;
                                        EV << "MapCh0to7=" << RxdAdvPkt->getMapCh0to7() << endl;
                                        EV << "MapCh8to15=" << RxdAdvPkt->getMapCh8to15() << endl;
                                        EV << "MapCh16to23=" << RxdAdvPkt->getMapCh16to23() << endl;
                                        EV << "MapCh24to31=" << RxdAdvPkt->getMapCh24to31() << endl;
                                        EV << "MapCh32to39=" << RxdAdvPkt->getMapCh32to39() << endl;
                                        EV << "Hop=" << RxdAdvPkt->getHop() << endl;
                                        EV << "SCA=" << RxdAdvPkt->getSCA() << endl;
                                    }


                                    if(debug_StateTransitions) EV<< "BLEMacV2:updateStatusAdvertising: Switching to CONNECTION state!" << endl;

                                    ctrl_switchState->setKind(ADVERTIZING_to_CONNECTION);

                                    scheduleAt(simTime(), ctrl_switchState);
                                }
                            }
                        }

                    } // else of RX CONN_REQ packet

                     // Recieved SCAN_REQ, need to send SCAN_RSP packet
                    else if (RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_SCAN_REQ_5){

                        num_scan_req_rcved++;

                   if((RxdAdvPkt->getTxAdd()==myAdvrtPars.Direct_Address_Type)&&(RxdAdvPkt->getRxAdd()==myAdvrtPars.Own_Address_Type)){

                      if((RxdAdvPkt->getAdvA()==myAdvrtPars.OwnAdvAddr)&&(RxdAdvPkt->getInitA()==myAdvrtPars.Direct_Address)){//address ok

                            if(debug_Internal_Messages)  EV<< "BLEMacV2:updateStatusAdvertising: packet is a SCAN_REQ for me" << endl;
                            //get all connection parameters from the packet
                            myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getInitA();

                            if(RxdAdvPkt->getTxAdd()){
                                myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                            }
                            else{
                                myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                            }



                            //print all out
                            if(debug_Pkt_Basic){
                                EV << "BLEMacV2::received Scan Request" << endl;
                                EV << "Adv_PDU_Type=" << RxdAdvPkt->getAdv_PDU_type() << endl;// Tham added PDU
                                EV << "Adv_PDU_Name= " << RxdAdvPkt->getName() << endl; // Tham added
                                EV << "AdvA=" << RxdAdvPkt->getAdvA() << endl;
                                EV << "InitA=" << RxdAdvPkt->getInitA() << endl;
                                EV << "TxAdd=" << RxdAdvPkt->getTxAdd() << endl;
                                EV << "RxAdd=" << RxdAdvPkt->getRxAdd() << endl;
                                EV << "Length(bytes)=" << RxdAdvPkt->getLength() << endl;

                            }


                      //  if(debug_StateTransitions) EV<< "BLEMacV2:updateStatusAdvertising: Switching to CONNECTION state!" << endl;
                      //  ctrl_switchState->setKind(ADVERTIZING_to_CONNECTION);
                      //  scheduleAt(simTime(), ctrl_switchState);

                         int temp_c = RxdAdvPkt->getTemp_c(); // trick to to get index to generate response probability distribution

                         EV<< "BLEMacV2:updateStatusAdvertising on EV_FRAME_RECEIVED. ADV PDU type=" << RxdAdvPkt->getAdv_PDU_type() << ". Then, generate SCAN_RSP" << endl;

                        generateScan_Rsp(temp_c);

                        // num_scan_rsp++;


                  simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0

                  if(FreeTime==0) eventRadioStateChanged();

                     // scan response packet to send:
                     AdvPktCopy = AdvMessage->dup();
                     EV << "AdvPktCopy->Adv_PDU_type =" << AdvPktCopy->getAdv_PDU_type() << endl;
                     EV << "AdvPktCopy->Adv_type = " <<AdvPktCopy->getAdv_type() << endl;

                     attachSignal_Adv(AdvPktCopy, simTime() + Time_llRXtoTX);
                     num_scan_rsp_test++;
                     sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                                }
                            }
                  } // else of received SCAN_REQ packet

                } // else of checking the Packet with correct access channel address
        }
            delete msg;

        break;

        default:
            error("BLEMacV2::updateStatusAdvertising unsupported event!");
        break;
    }
}


//original
/*
void BLEMacV2::updateStatusInitiating(t_mac_event event, cMessage *msg){
    BLE_Adv_MacPkt * AdvPktCopy;
    int CurrentAdvChannel;
    int NextAdvChannel;
    switch (event) {
        case EV_INIT_INTERVAL:
            {
                startTimer(TIMER_INITIATING_INTERVAL);
                simtime_t ScanWindow=myScanPars.LE_Scan_Window*0.000625;
                simtime_t ScanInterval=myScanPars.LE_Scan_Interval*0.000625;
                if(ScanWindow < ScanInterval-Time_llRXtoSLEEP-Time_llSLEEPtoRX){//any sense to start window timer?
                    startTimer(TIMER_INITIATING_WINDOW);
                }
                //set next channel to use
                CurrentAdvChannel=phy->getCurrentRadioChannel();
                NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);
                if(NextAdvChannel==-1){
                    NextAdvChannel=myAdvrtPars.GetNextAdvChannel(0);
                }
                if(NextAdvChannel==-1){
                   error("BLEMacV2::updateStatusInitiating on EV_INIT_INTERVAL - cannot find next channel!");
                }
                phy->setCurrentRadioChannel(NextAdvChannel);
                if(info_events)eventRadioChannelChanged();
                if(phy->getRadioState()!=MiximRadio::RX){
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages)
                        EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                }
                if(debug_ChannelSwitching)
                    EV<< "BLEMacV2:updateStatusInitiating on EV_INIT_INTERVAL. starting receive in channel " << NextAdvChannel << endl;
            }
            break;
        case EV_INIT_WINDOW:
            if(debug_Internal_Messages)
                EV<< "BLEMacV2:updateStatusInitiating on EV_INIT_WINDOW. switching to sleep " << endl;
            phy->setRadioState(MiximRadio::SLEEP);//switch to sleep
            break;
        case EV_FRAME_TRANSMITTED:
            //switch to connection as a master
            if(debug_StateTransitions)
                EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_TRANSMITTED. Switching to CONNECTION state!" << endl;
            ctrl_switchState->setKind(INITIALIZING_to_CONNECTION);
            scheduleAt(simTime(), ctrl_switchState);
            break;
        case EV_FRAME_RECEIVED:
            {
                BLE_Adv_MacPkt*  RxdAdvPkt = static_cast<BLE_Adv_MacPkt *> (msg);
                if(debug_Pkt_Basic){
                    EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED. ADV PDU type=" << RxdAdvPkt->getAdv_PDU_type() << " Length=" << RxdAdvPkt->getBitLength() << " bits, Access Address="
                    << RxdAdvPkt->getAccessAddress() << " AdvA=" << RxdAdvPkt->getAdvA()
                    <<endl;
                }
                if(RxdAdvPkt->getAccessAddress()==0x8E89BED6){//see p 2503 of standard v4.1
                    if((RxdAdvPkt->getRxAdd())||(RxdAdvPkt->getTxAdd())){
                        error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - random addresses are NOT SUPPORTED!");
                    }
                    else{
                        if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_DIRECT_IND_1){
                            if(RxdAdvPkt->getAdvA()==myScanPars.Peer_Address){//my target - start connection
                                long int AA=generateAccessAddr();
                                if(debug_Internal_Messages)
                                    EV<<"AA=" << AA << endl;
                                myConnPars.ConnInfo.accessAddr=AA;
                                myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getAdvA();
                                if(RxdAdvPkt->getTxAdd()){
                                    myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                }
                                else{
                                    myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                }

                                myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.getInterval(myConnPars.ConnPars.Conn_Interval_Min,myConnPars.ConnPars.Conn_Interval_Max);
                                generateConnectionRequest();

                                //stop timers
                                cancelEvent(initiatingScanIntervalTimer);
                                cancelEvent(initiatingScanWindowTimer);
                                //send packet
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                /*   if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                                    /
                                AdvPktCopy = AdvMessage->dup();
                                attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                                sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                                updateMacSubstate(INITIATING_TXD);
                                //error("MEGA-BONG");
                            }
                            else{
                                //DO NOTHING
                            }
                        }
                        else if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_NONCONN_IND_2){
                            //DO NOTHING
                            /*   cPacket *m = RxdAdvPkt->decapsulate();
                            if(m){
                                if(debug_Internal_Messages)
                                    EV<< "Name of the encapsualted packet is =" << m->getName() << endl;
                            }
                            else{
                                if(debug_Internal_Messages)
                                    EV<< "No packet incapsulated!" << endl;
                            }
                            error("STOP HERE!");
                            /
                        }
                        else if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_CONNECT_REQ_5){
                            //DO NOTHING
                        }
                        else{
                            error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - do not know how to process this PDU type!");
                        }
                    }
                }
            }
            delete msg;
            break;
        default:
            error("BLEMacV2::updateStatusInitiating unsupported event!");
            break;
    }
}
*/

// Tham modified This function is for master
void BLEMacV2::updateStatusInitiating(t_mac_event event, cMessage *msg){
    BLE_Adv_MacPkt * AdvPktCopy;
    int CurrentAdvChannel;
    int NextAdvChannel;

    EV << "Tham BLEMacV2::updateStatusInitiating : msg =" << *msg << endl;

    switch (event) {
        case EV_INIT_INTERVAL:
            {
                ev << "Tham BLEMacV2::updateStatusInitiating TIMER_INITIATING_INTERVAL = " << TIMER_INITIATING_INTERVAL <<  endl;
                startTimer(TIMER_INITIATING_INTERVAL);
                simtime_t ScanWindow=myScanPars.LE_Scan_Window*0.000625;
                simtime_t ScanInterval=myScanPars.LE_Scan_Interval*0.000625;

                if(ScanWindow < ScanInterval-Time_llRXtoSLEEP-Time_llSLEEPtoRX){//any sense to start window timer?
                    startTimer(TIMER_INITIATING_WINDOW);
                }
                //set next channel to use
                CurrentAdvChannel=phy->getCurrentRadioChannel();
                NextAdvChannel=myAdvrtPars.GetNextAdvChannel(CurrentAdvChannel+1);

                if(NextAdvChannel==-1){
                    NextAdvChannel=myAdvrtPars.GetNextAdvChannel(0);
                }

                /*
                if(NextAdvChannel==-1){
                   error("BLEMacV2::updateStatusInitiating on EV_INIT_INTERVAL - cannot find next channel!");
                } */ // Tham deleted

                phy->setCurrentRadioChannel(NextAdvChannel);
                if(info_events)eventRadioChannelChanged();
                if(phy->getRadioState()!=MiximRadio::RX){
                    simtime_t FreeTime=phy->setRadioState(MiximRadio::RX);//to handle the stupid new system, when no message is returned if FreeTime=0
                    if(FreeTime==0) eventRadioStateChanged();
                    if(debug_Internal_Messages)
                        EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;
                }
                if(debug_ChannelSwitching)
                    EV<< "BLEMacV2:updateStatusInitiating on EV_INIT_INTERVAL. starting receive in channel " << NextAdvChannel << endl;
            }
        break;

        case EV_INIT_WINDOW:
            {
            if(debug_Internal_Messages) EV<< "BLEMacV2:updateStatusInitiating on EV_INIT_WINDOW. switching to sleep " << endl;
            phy->setRadioState(MiximRadio::SLEEP);//switch to sleep
            }
        break;

     /*   case EV_FRAME_TRANSMITTED:
                    //switch to connection as a master
                    if(debug_StateTransitions)
                        EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_TRANSMITTED. Switching to CONNECTION state!" << endl;
                    ctrl_switchState->setKind(INITIALIZING_to_CONNECTION);
                    scheduleAt(simTime(), ctrl_switchState);
                    break; */

        case EV_FRAME_TRANSMITTED:
            {
            //switch to connection as a master sent connection request
                //After tx connection reques
            if(macSubState == INITIATING_TXD) {
                if(debug_StateTransitions) EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_TRANSMITTED. Switching to CONNECTION state!" << endl;
                ctrl_switchState->setKind(INITIALIZING_to_CONNECTION);
                scheduleAt(simTime(), ctrl_switchState);
            }
            // how abt when it sends the Scan-req packet??

        }
        break;

        case EV_FRAME_RECEIVED:
            {
                BLE_Adv_MacPkt*  RxdAdvPkt = static_cast<BLE_Adv_MacPkt *> (msg);

                if(debug_Pkt_Basic){
                    EV<< "BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED. ADV PDU type=" << RxdAdvPkt->getAdv_PDU_type() << " Length=" << RxdAdvPkt->getBitLength() << " bits, Access Address="
                    << RxdAdvPkt->getAccessAddress() << " AdvA=" << RxdAdvPkt->getAdvA()
                    <<endl;
                }

                if(RxdAdvPkt->getAccessAddress()==0x8E89BED6){ //see p 2503 of standard v4.1
                //if(RxdAdvPkt->getAccessAddress()== 2391391958){ //see p 2503 of standard v4.1
                    if((RxdAdvPkt->getRxAdd())||(RxdAdvPkt->getTxAdd())){
                        error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - random addresses are NOT SUPPORTED!");
                    }
                    else{
                        //=======Process ADV_DIRECT packet==========
                        if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_DIRECT_IND_1){
                            EV << "Recieved ADV_DIRECT_IND_1 packet" << endl; // Tham
                            EV <<"Received Packet length = " << RxdAdvPkt->getLength() << endl; // Tham
                            if(RxdAdvPkt->getAdvA()==myScanPars.Peer_Address){//my target - start connection
                                unsigned int AA=generateAccessAddr();
                                if(debug_Internal_Messages)
                                    EV<<"AA=" << AA << endl;
                                myConnPars.ConnInfo.accessAddr=AA;
                                myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getAdvA();
                                if(RxdAdvPkt->getTxAdd()){
                                    myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                }
                                else{
                                    myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                }

                                myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.getInterval(myConnPars.ConnPars.Conn_Interval_Min,myConnPars.ConnPars.Conn_Interval_Max);

                                generateConnectionRequest();

                                //stop timers
                                cancelEvent(initiatingScanIntervalTimer);
                                cancelEvent(initiatingScanWindowTimer);
                                //send packet
                                simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                if(FreeTime==0) eventRadioStateChanged();
                                /*if(debug_Internal_Messages)
                                    EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;*/
                                AdvPktCopy = AdvMessage->dup();
                                attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                                sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                                updateMacSubstate(INITIATING_TXD);

                                //error("MEGA-BONG");

                            }
                            else{
                                //DO NOTHING
                            }
                        }

               //========== Process AV_NONCONN_IND packet=======
                        else if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_NONCONN_IND_2){
                            //DO NOTHING
                            /*cPacket *m = RxdAdvPkt->decapsulate();
                            if(m){
                                if(debug_Internal_Messages)
                                    EV<< "Name of the encapsualted packet is =" << m->getName() << endl;
                            }
                            else{
                                if(debug_Internal_Messages)
                                    EV<< "No packet incapsulated!" << endl;
                            }
                            error("STOP HERE!");
                            */
                        }
              // ==============  CONN_REQ ====================
                        else if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_CONNECT_REQ_3){
                            //DO NOTHING Master will not receive the connection requestion packet
                        }

                //================= Process ADV_IND packet ===============
                        else if(RxdAdvPkt->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_IND_0){ // Tham added

                            num_adv_ind_rcved++;

                            EV << "Recieved ADV_IND packet" << endl;
                            EV <<"Received Packet length = " << RxdAdvPkt->getLength() << endl;
                            EV << "Recieved Packet Adv Data = " << RxdAdvPkt->getAdvData() << endl;

                           // t_begin = simTime();
                          //  EV <<"Received ADV packet at time: " << t_begin << endl;

                           // temp = RxdAdvPkt->getAdvData();

                            if (input_rsp > 0) { // send scan request

                            if(RxdAdvPkt->getAdvA()==myScanPars.Peer_Address){//my target - start connection
                               unsigned int AA=generateAccessAddr();
                               if(debug_Internal_Messages) EV<<"AA=" << AA << endl;
                               myConnPars.ConnInfo.accessAddr=AA;
                               myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getAdvA();

                               if(RxdAdvPkt->getTxAdd()){
                                   myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                               }
                               else{
                                   myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                               }

                               myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.getInterval(myConnPars.ConnPars.Conn_Interval_Min,myConnPars.ConnPars.Conn_Interval_Max);

                              // generateConnectionRequest();

                              generateScanRequest();

                               //send packet
                               simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                               if(FreeTime==0) eventRadioStateChanged();

                               /*if(debug_Internal_Messages)
                                   EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;*/

                               AdvPktCopy = AdvMessage->dup();

                               attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                               num_scan_req_test++;
                               sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                                 // updateMacSubstate(INITIATING_TXD); // original
                                   // updateMacSubstate(INITIATING_SCAN); // Tham changed for SCAN_REQ
                               //error("MEGA-BONG");

                                 //  ctrl_switchState->setKind(STANDBY_to_SCANNING);
                                  // scheduleAt(simTime(), ctrl_switchState);


                           }
                               else {
                                   //DO NOTHING
                               }

                           }
                            else { // not sending scan request

                                if(RxdAdvPkt->getAdvA()==myScanPars.Peer_Address){//my target - start connection

                                    unsigned int AA=generateAccessAddr();
                                    if(debug_Internal_Messages) EV<<"AA=" << AA << endl;
                                    myConnPars.ConnInfo.accessAddr=AA;
                                    myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getAdvA();

                                    if(RxdAdvPkt->getTxAdd()){
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                                    }
                                    else{
                                        myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                                    }

                                    myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.getInterval(myConnPars.ConnPars.Conn_Interval_Min,myConnPars.ConnPars.Conn_Interval_Max);

                                    generateConnectionRequest();

                                    //stop timers
                                    cancelEvent(initiatingScanIntervalTimer);
                                    cancelEvent(initiatingScanWindowTimer);
                                    //send packet
                                    simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                                    if(FreeTime==0) eventRadioStateChanged();
                                    /*if(debug_Internal_Messages)
                                        EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;*/
                                    AdvPktCopy = AdvMessage->dup();
                                    attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                                    sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                                    updateMacSubstate(INITIATING_TXD);

                                    //error("MEGA-BONG");

                                }
                                else{
                                    //DO NOTHING
                                }

                               t_end_dis = simTime();
                               EV <<"Connection request sent at time: " << simTime() << endl;
                           }

                        } // Once the master received a scan response packet




               //========= Process the SCAN RSP ====================
                     else if(RxdAdvPkt->getAdv_PDU_type()== cBLEstructs_defs::PDU_SCAN_RSP_6){

                               num_scan_rsp_rcved++;

                            if(RxdAdvPkt->getAdvA()==myScanPars.Peer_Address){ //my target - start connection
                                EV << "Recieved SCAN_RSP packet" << endl;
                                EV <<"Received Packet length = " << RxdAdvPkt->getLength() << endl;
                                EV << "Recieved Packet Adv Data = " << RxdAdvPkt->getScanRspData() << endl;

                                unsigned int AA=generateAccessAddr();

                            if(debug_Internal_Messages) EV<<"AA=" << AA << endl;

                            myConnPars.ConnInfo.accessAddr=AA;

                            myConnPars.ConnInfo.Peer_Address=RxdAdvPkt->getAdvA();

                            if(RxdAdvPkt->getTxAdd()){
                                myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_RANDOM_1;
                            }
                            else{
                                myConnPars.ConnInfo.Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;
                            }

                            myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.getInterval(myConnPars.ConnPars.Conn_Interval_Min,myConnPars.ConnPars.Conn_Interval_Max);

                    //====Trust computation====

                           // EV << "BLEMacV2::updateStatusInitiating - Trust compuation: temp before Rx ScanRspData = "<< temp_ch << endl;

                           // if (RxdAdvPkt->getScanRspData() == temp*11) T = 0.5;
                           // else T = 0;

                            int index_rc = RxdAdvPkt->getIndex_r();
                            temp_res = RxdAdvPkt->getScanRspData();
                            double p_cr = (double)(temp_res*temp_ch)/(double)(prob_r_on_c[0][index_rc]*prob_c[0]+prob_r_on_c[1][index_rc]*prob_c[1]+prob_r_on_c[2][index_rc]*prob_c[2]);
                            T_inst[index_T] = p_cr - temp_ch;

                            EV << "index_T = " << index_T << endl;

                            if (index_T > 0) {
                            T_init[index_T] = T_init[index_T - 1]*0.3 + T_inst[index_T]*0.7;
                            } else {
                                T_init[index_T] = T_inst[index_T];
                            }

                            //T_init[index_T] = T_init[index_T - 1]*0.3 + T_inst[index_T]*0.7;

                            response_list[index_T] = index_rc;
                            EV << "Tham response_list[" <<index_T<< "] =" << response_list[index_T] << endl;
                            Address_device[0] = RxdAdvPkt->getAdvA();
                           // Trust_device[0] = T_init[index_T];
                            /*
                            for (int i=0; i<3; i=i+1){
                                cout << "p_c[" << i<< "]= " << prob_c[i] <<endl;
                           }

                            for (int i=0; i<3; i=i+1){
                                for (int j = 0; j <4; j=j+1){
                                  cout << "p_rc["<<i<<"][" <<j<<"]= " << prob_r_on_c[i][j] <<endl;
                                }
                            }
                            */

                           // temp_T = temp_ch*temp_res;

                           // EV << "BLEMacV2::updateStatusInitiating - trust value = " << T_inst[index_T] << endl;


                            if ((input_rsp > 0) && (num_scan_rsp_rcved < input_rsp))
                            {
                              // Did not send enought challenges
                                generateScanRequest();

                              //send packet
                              simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                              if(FreeTime==0) eventRadioStateChanged();

                              /*if(debug_Internal_Messages)
                                  EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;*/

                              AdvPktCopy = AdvMessage->dup();

                              attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                              num_scan_req_test++;
                              sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);
                            }
                            else {
                             // Sent enough challenges
                             generateConnectionRequest();


                           // EV << "Tham - Recieved SCAN_RSP, find Trust = " << T << endl;

                            //stop timers
                            cancelEvent(initiatingScanIntervalTimer);
                            cancelEvent(initiatingScanWindowTimer);
                            //send packet
                            simtime_t FreeTime=phy->setRadioState(MiximRadio::TX);//to handle the stupid new system, when no message is returned if FreeTime=0
                            if(FreeTime==0) eventRadioStateChanged();
                            /*if(debug_Internal_Messages)
                                EV<< "BLEMacV2::updateStatusConnected - FreeTime=" << FreeTime << endl;*/
                            AdvPktCopy = AdvMessage->dup();
                            attachSignal_Adv(AdvPktCopy, simTime()+Time_llRXtoTX);
                            sendDelayed(AdvPktCopy, Time_llRXtoTX, lowerLayerOut);

                            updateMacSubstate(INITIATING_TXD);
                            t_end_dis = simTime();
                            EV <<"Connection request sent at time: " <<simTime() << endl;

                               }
                        }
                  }
   // ------------------------ others ---------------------------
                else {
                       error("BLEMacV2:updateStatusInitiating on EV_FRAME_RECEIVED - do not know how to process this PDU type!");
                               }
                      }
               }
        }

        delete msg;

    break;

    default:
        error("BLEMacV2::updateStatusInitiating unsupported event!");
    break;
  }
}

// Updates state machine.
void BLEMacV2::executeMac(t_mac_event event, cMessage *msg){
    //EV<< "In executeMac macState=" << macState << "event=" << event << endl;
    //ev << "Tham BLEMacV2::executeMac: msg = " << *msg << endl;
    ev << "BLEMacV2::executeMac" << endl;
    switch(macState) {
    case STANDBY_1:
        EV <<"BLEMacV2::executeMac - STANDBY_1" << endl;
        updateStatusStandby(event, msg);
        break;
    case ADVERTISING_2:
        EV <<"BLEMacV2::executeMac - ADVERTISING_2" << endl;
        updateStatusAdvertising(event, msg);
        break;
    case INITIATING_4:
        EV <<"BLEMacV2::executeMac - INITIATING_4" << endl;
        updateStatusInitiating(event, msg);
        break;
    case CONNECTION_5:
        EV <<"BLEMacV2::executeMac - CONNECTION_5" << endl;
        updateStatusConnected(event, msg);
        break;
    default:
        error("BLEMacV2::executeMac unsupported event!");
        break;
    }
}


//Calculates the time (and checks the validity of values) up to the next advertisement event given the advertisement intervals and type

simtime_t BLEMacV2::calculateAdvEvent(long int Advertising_Interval_Min, long int Advertising_Interval_Max, int AdvType){
    int advInt_slots;
    simtime_t TadvEvent;//T_advEvent par in BLE spec (see p. 2528)
    simtime_t advDelay;//advDelay in BLE spec (see p. 2528)
    //Check validity of the data
    if(Advertising_Interval_Min>Advertising_Interval_Max)error("BLEMacV2::scheduleAdv: According to BLE std v4.1 Advertising_Interval_Min CANNOT be higher than Advertising_Interval_Max");
    if(Advertising_Interval_Max<0x20)error("BLEMacV2::scheduleAdv: According to BLE std v4.1 Advertising_Interval_Max CANNOT be lower than 0x20");

    switch(AdvType){
    case BLEstructs::ADV_IND_0:
        if((Advertising_Interval_Min<0x020)||(Advertising_Interval_Max<0x020)){//see p. 2528 of Bluetooth v4.1 Spec
            error("BLEMacV2::scheduleAdv for ADV_IND_0: According to BLE std v4.1 Advertising_Interval_Min&Advertising_Interval_Max should be >= 0x020");
        }
        break;
    case BLEstructs::ADV_DIRECT_IND_HDC_1:
        //TODO:
        error("BLEMacV2::scheduleAdv for ADV_DIRECT_IND_HDC_1: NOT IMPLEMENTED YET");
        break;
    case BLEstructs::ADV_SCAN_IND_2:
        if((Advertising_Interval_Min<0x0A0)||(Advertising_Interval_Max<0x0A0)){//see p. 2528 of Bluetooth v4.1 Spec
            error("BLEMacV2::scheduleAdv for ADV_SCAN_IND_2: According to BLE std v4.1 Advertising_Interval_Min&Advertising_Interval_Max should be >= 0x00A0");
        }
        else{
            //TODO:
            error("BLEMacV2::scheduleAdv for ADV_SCAN_IND_2: NOT IMPLEMENTED YET");
        }
        break;
    case BLEstructs::ADV_NONCONN_IND_3:
        if((Advertising_Interval_Min<0x0A0)||(Advertising_Interval_Max<0x0A0)){//see p. 2528 of Bluetooth v4.1 Spec
            error("BLEMacV2::scheduleAdv for ADV_NONCONN_IND_3: According to BLE std v4.1 Advertising_Interval_Min&Advertising_Interval_Max should be >= 0x00A0");
        }
        /*else{
            //TODO:
            error("BLEMacV2::scheduleAdv for ADV_NONCONN_IND_3: NOT IMPLEMENTED YET");
        }*/
        break;
    case BLEstructs::ADV_DIRECT_IND_LDC_4:
        if((Advertising_Interval_Min<0x020)||(Advertising_Interval_Max<0x020)){//see p. 2528 of Bluetooth v4.1 Spec
            error("BLEMacV2::scheduleAdv for ADV_DIRECT_IND_LDC_4: According to BLE std v4.1 Advertising_Interval_Min&Advertising_Interval_Max should be >= 0x020");
        }
        break;
    }


    //mechanism for generating the advInterval from Advertising_Interval_Min and Advertising_Interval_Max
    //NOTE: TTBMK (To the best of my knowledge), it is not described in Spec.

    advInt_slots = Advertising_Interval_Min + intuniform(0, Advertising_Interval_Max - Advertising_Interval_Min, 0);//use uniform int generator

    //NOTE: TTBMK the mechanism of generating the random delay of 0 to 10 ms is not described in spec (e.g., TI guys for CC2540 made it also 0.625ms based)
    if(TST_NoAdvIntRandomComponent==false){
        advDelay = uniform(0, 0.010, 0);//in seconds
    }
    else advDelay=0;
    //advDelay=0;//FOR DEBUG!
    TadvEvent = advInt_slots*0.000625 + advDelay;//in seconds

    if (debug_Timers){
        EV <<"BLEMacV2::scheduleAdv for AdvType=" << AdvType << " Advertising_Interval_Min=" << Advertising_Interval_Min << " Advertising_Interval_Max=" << Advertising_Interval_Max << endl;
        EV <<"advInt_slots=" << advInt_slots << " advDelay=" << advDelay << " TadvEvent=advInt_slots*0.000625+advDelay=" << TadvEvent << endl;
    }
    return TadvEvent;
}

//Calculates the time of the first connection event (used during initialization)
simtime_t BLEMacV2::calculateFirstConnectionEventTime_Master(void){
    simtime_t TconnEvent, T_TXWindowSTART, T_TXWindowEND;//
    T_TXWindowSTART=simTime()+0.00125*(1+myConnPars.ConnInfo.transmitWindowOffset);
    T_TXWindowEND=simTime()+0.00125*(1+myConnPars.ConnInfo.transmitWindowOffset+myConnPars.ConnInfo.transmitWindowSize);
    TconnEvent=T_TXWindowSTART;//just as fast as possible
    return TconnEvent;
}


simtime_t BLEMacV2::calculateTransmitWindowStart_Slave(void){
    simtime_t T_TXWindowSTART;//
    T_TXWindowSTART=simTime()+0.00125*(1+myConnPars.ConnInfo.transmitWindowOffset);
    return T_TXWindowSTART;
}



void BLEMacV2::attachSignal_Data(BLE_Data_MacPkt* macPkt, simtime_t_cref startTime){
    simtime_t duration = (macPkt->getBitLength())/bitrate;
    setDownControlInfo(macPkt, createSignal(startTime, duration, txPower, bitrate));

    if(debug_Pkt_Basic){
        EV<< "BLEMacV2:attachSignal_Data. Sending DATA PDU. Access Address=" << macPkt->getAccessAddress() <<" Packet Length=" << macPkt->getBitLength()
                << " seqID="<< macPkt->getSequenceId() << " MD=" << macPkt->getHdr_MD() << " SN=" << macPkt->getHdr_SN()
                << " NESN=" << macPkt->getHdr_NESN() <<endl;
    }
    //    EV <<"BLEMacV2::attachSignal_Data:" << " bitrate=" << bitrate << " length(bits)=" << macPkt->getBitLength() <<" startTime=" << startTime << " duration=" << duration << " TXpower=" << txPower << "mW"<< endl;
}


/*
int BLEMacV2::GetNextAdvChannel(int StartChannel){
    int iChannelIndex=StartChannel;
    while(iChannelIndex<40){
        //EV <<"BLEMacV2::GetNextAdvChannel: iChannelIndex=" << iChannelIndex << " TotalChNumber=" <<myAdvrtPars.Advertising_Channel_Map.TotalChNumber << " channel state=" << myAdvrtPars.Advertising_Channel_Map.ChannelTbl[iChannelIndex] << endl;
        if(myAdvrtPars.Advertising_Channel_Map.ChannelTbl[iChannelIndex]==true){
            //EV <<"BLEMacV2::GetNextAdvChannel next channel is=" << iChannelIndex<< endl;
            return iChannelIndex;
        }
        iChannelIndex++;
    }
    return -1;
}
*/


//check packet of which size we might send in remaining time (if we can) and prepares the packet

// normal data packet

bool BLEMacV2::prepareNewDataPkt(simtime_t RemainingTime){
    long int PktBitLgth;
    int MaxPossiblePktPayloadBytes;

    delete DataPkt;
    DataPkt = new BLE_Data_MacPkt("DATA_PDU");

    if(RemainingTime<((phyHeaderLength+llDataHeaderLengthBits)/bitrate)){//not enough time to send anything
        if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - FAIL - not enough time to send anything"<< endl;
        return false;
    }
    else if(RemainingTime>((phyHeaderLength+llDataHeaderLengthBits+llmaxDataPDUPayloadBytes)/bitrate)){//we can send a maximum-size packet
        MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }
    else{ //estimate how many data bytes we might send
        double AvailableTime;
        AvailableTime=RemainingTime.dbl()-(phyHeaderLength+llDataHeaderLengthBits)/bitrate;
        //EV<<"BLEMacV2::prepareNewDataPkt - AvailableTime="<< AvailableTime <<endl;
        MaxPossiblePktPayloadBytes=(int)(FWMath::floorToZero(AvailableTime*bitrate/8));
        if(MaxPossiblePktPayloadBytes==0){
            if(debug_Internal_Messages)  EV<<"BLEMacV2::prepareNewDataPkt - FAIL - we cannot send a single full byte"<< endl;
            return false;
        }
        else if(llmaxDataPDUPayloadBytes<MaxPossiblePktPayloadBytes)MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }

    if(debug_Internal_Messages) EV << "BLEMacV2::prepareNewDataPkt PktLeft_TX_Size=" << PktLeft_TX_Size << " ,MaxPossiblePktPayloadBytes=" << MaxPossiblePktPayloadBytes << endl;

    if(PktLeft_TX_Size>MaxPossiblePktPayloadBytes){//we cannot fit all our data in single packet

        if(debug_Internal_Messages)  ev << "PktLeft_TX_Size > MaxPossiblePktPayloadBytes" << endl;

        DataPkt->setHdr_lgth(MaxPossiblePktPayloadBytes);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        DataPkt->setHdr_MD(true);
        myConnPars.ConnInfo.moreData=true;
    }
    else if(PktLeft_TX_Size > 0){//this is the last fragment
        if(debug_Internal_Messages) ev << "PktLeft_TX_Size > 0" << endl;
        DataPkt->setHdr_lgth(PktLeft_TX_Size);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            //DataPkt_FirstFragment=false;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        if(macQueue.size()>1){
            DataPkt->setHdr_MD(true);
            myConnPars.ConnInfo.moreData=true;
        }
        else{
            DataPkt->setHdr_MD(false);
        }
        //encapsulate the packet
        cPacket * pkt=macQueue.front()->dup();
        DataPkt->encapsulate(pkt);

        //delete macQueue.front();
    }
    // original
   /* else{//we do not have any data to TX - just send empty packet
        EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx" << endl;
        DataPkt->setHdr_lgth(0);
        DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
       // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
        if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PUD start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
        DataPkt->setHdr_MD(false);
    } */
    //end original

    else{//we do not have any data to TX - just send empty packet
            EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx - just send empty packet" << endl;
            DataPkt->setHdr_lgth(0);
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
           // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1: LL Data PDU: Continuation fragment of an L2CAP msg, or an empty PDU" << endl;
            DataPkt->setHdr_MD(false);
        }

    DataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);
    LastPacketPayload=DataPkt->getHdr_lgth();
    if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - LastPacketPayload="<< LastPacketPayload << endl;
    DataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);

    //calculate the length of the packet
    PktBitLgth=DataPkt->getHdr_lgth()*8 + phyHeaderLength + llDataHeaderLengthBits;
    DataPkt->setBitLength(PktBitLgth);


    eventQuerryChange(-(DataPkt->getHdr_lgth()));

    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepareNewDataPkt - packetID= " << DataPkt->getSequenceId() <<" Actual packet length = "<< PktBitLgth <<
                "bits of which: payload= " << DataPkt->getHdr_lgth()*8 << "bits, LL header= " << llDataHeaderLengthBits  <<
                "bits, PHY header=" <<  phyHeaderLength << "bits, ST_DataQueryLgth= "<< TST_DataQueryLgth << " bytes" <<
                endl;
   }

    myConnPars.ConnInfo.delayedGeneration=false;
    return true;
}

// prepare new data with challenge for Slave

bool BLEMacV2::prepareNewDataPkt_Slave(simtime_t RemainingTime, int IndexCh_data){
    long int PktBitLgth;
    int MaxPossiblePktPayloadBytes;

    delete DataPkt;
    DataPkt = new BLE_Data_MacPkt("DATA_PDU");

    if(RemainingTime<((phyHeaderLength+llDataHeaderLengthBits)/bitrate)){//not enough time to send anything
        if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - FAIL - not enough time to send anything"<< endl;
        return false;
    }
    else if(RemainingTime>((phyHeaderLength+llDataHeaderLengthBits+llmaxDataPDUPayloadBytes)/bitrate)){//we can send a maximum-size packet
        MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }
    else{ //estimate how many data bytes we might send
        double AvailableTime;
        AvailableTime=RemainingTime.dbl()-(phyHeaderLength+llDataHeaderLengthBits)/bitrate;
        //EV<<"BLEMacV2::prepareNewDataPkt - AvailableTime="<< AvailableTime <<endl;
        MaxPossiblePktPayloadBytes=(int)(FWMath::floorToZero(AvailableTime*bitrate/8));
        if(MaxPossiblePktPayloadBytes==0){
            if(debug_Internal_Messages)  EV<<"BLEMacV2::prepareNewDataPkt - FAIL - we cannot send a single full byte"<< endl;
            return false;
        }
        else if(llmaxDataPDUPayloadBytes<MaxPossiblePktPayloadBytes)MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }

    if(debug_Internal_Messages) EV << "BLEMacV2::prepareNewDataPkt PktLeft_TX_Size=" << PktLeft_TX_Size << " ,MaxPossiblePktPayloadBytes=" << MaxPossiblePktPayloadBytes << endl;

    if(PktLeft_TX_Size>MaxPossiblePktPayloadBytes){//we cannot fit all our data in single packet

        if(debug_Internal_Messages)  ev << "PktLeft_TX_Size > MaxPossiblePktPayloadBytes" << endl;

        DataPkt->setHdr_lgth(MaxPossiblePktPayloadBytes);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        DataPkt->setHdr_MD(true);
        myConnPars.ConnInfo.moreData=true;
    }
    else if(PktLeft_TX_Size > 0){//this is the last fragment
        if(debug_Internal_Messages) ev << "PktLeft_TX_Size > 0" << endl;
        DataPkt->setHdr_lgth(PktLeft_TX_Size);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            //DataPkt_FirstFragment=false;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        if(macQueue.size()>1){
            DataPkt->setHdr_MD(true);
            myConnPars.ConnInfo.moreData=true;
        }
        else{
            DataPkt->setHdr_MD(false);
        }
        //encapsulate the packet
        cPacket * pkt=macQueue.front()->dup();
        DataPkt->encapsulate(pkt);

        //delete macQueue.front();
    }
    // original
   /* else{//we do not have any data to TX - just send empty packet
        EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx" << endl;
        DataPkt->setHdr_lgth(0);
        DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
       // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
        if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PUD start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
        DataPkt->setHdr_MD(false);
    } */
    //end original

    else{//we do not have any data to TX - just send empty packet
            EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx - just send empty packet" << endl;
            DataPkt->setHdr_lgth(0);
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
           // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1: LL Data PDU: Continuation fragment of an L2CAP msg, or an empty PDU" << endl;
            DataPkt->setHdr_MD(false);
        }

    DataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);
    LastPacketPayload=DataPkt->getHdr_lgth();
    if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - LastPacketPayload="<< LastPacketPayload << endl;
    DataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);

    //calculate the length of the packet
    PktBitLgth=DataPkt->getHdr_lgth()*8 + phyHeaderLength + llDataHeaderLengthBits;
    DataPkt->setBitLength(PktBitLgth);

    ////////////////////////////////////////////////////
        int arr[] = {1, 2, 3, 4};
        int freq_rc1[] = {51, 34, 2, 13};
        int freq_rc2[] = {50, 36, 1, 13};
        int freq_rc3[] = {50, 31, 6, 13};

       // srand(time(NULL));

        int n = sizeof(arr) / sizeof(arr[0]);

        if (IndexCh_data == 0){
            ResponseIndex_data = myRand(arr, freq_rc1, n) - 1;

        } else if (IndexCh_data == 1){
            ResponseIndex_data = myRand(arr, freq_rc2, n) - 1;

        } else {
            ResponseIndex_data = myRand(arr, freq_rc3, n) - 1;
        }


       EV << "challenge index pass here = " << IndexCh_data << endl;
       EV << "Selected response is at [" << IndexCh_data <<"][" << ResponseIndex_data <<"]" << endl;
       EV << "Response in DATA packet = "<< prob_r_on_c[IndexCh_data][ResponseIndex_data] << endl;

       double Response_in_dataPKt = prob_r_on_c[IndexCh_data][ResponseIndex_data]; //prob_r_on_c[RandIndex];

       DataPkt->setResponse_data(Response_in_dataPKt);

       EV <<"Tham - Update status connected at EV_CON_SLV_WAITIFS: send a Response = " << DataPkt->getResponse_data() << endl;

       DataPkt->setResponseIndex_data(ResponseIndex_data);

       /////////////////////////////////////////


    eventQuerryChange(-(DataPkt->getHdr_lgth()));

    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepareNewDataPkt - packetID= " << DataPkt->getSequenceId() <<" Actual packet length = "<< PktBitLgth <<
                "bits of which: payload= " << DataPkt->getHdr_lgth()*8 << "bits, LL header= " << llDataHeaderLengthBits  <<
                "bits, PHY header=" <<  phyHeaderLength << "bits, ST_DataQueryLgth= "<< TST_DataQueryLgth << " bytes" <<
                endl;
   }

    myConnPars.ConnInfo.delayedGeneration=false;
    return true;
}
//*/

// Tham added - prepare data packet with response for Master
bool BLEMacV2::prepareNewDataPkt_Master(simtime_t RemainingTime){
    long int PktBitLgth;
    int MaxPossiblePktPayloadBytes;

    delete DataPkt;
    DataPkt = new BLE_Data_MacPkt("DATA_PDU");

    if(RemainingTime<((phyHeaderLength+llDataHeaderLengthBits)/bitrate)){//not enough time to send anything
        if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - FAIL - not enough time to send anything"<< endl;
        return false;
    }
    else if(RemainingTime>((phyHeaderLength+llDataHeaderLengthBits+llmaxDataPDUPayloadBytes)/bitrate)){//we can send a maximum-size packet
        MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }
    else{ //estimate how many data bytes we might send
        double AvailableTime;
        AvailableTime=RemainingTime.dbl()-(phyHeaderLength+llDataHeaderLengthBits)/bitrate;
        //EV<<"BLEMacV2::prepareNewDataPkt - AvailableTime="<< AvailableTime <<endl;
        MaxPossiblePktPayloadBytes=(int)(FWMath::floorToZero(AvailableTime*bitrate/8));
        if(MaxPossiblePktPayloadBytes==0){
            if(debug_Internal_Messages)  EV<<"BLEMacV2::prepareNewDataPkt - FAIL - we cannot send a single full byte"<< endl;
            return false;
        }
        else if(llmaxDataPDUPayloadBytes<MaxPossiblePktPayloadBytes)MaxPossiblePktPayloadBytes=llmaxDataPDUPayloadBytes;
    }

    if(debug_Internal_Messages) EV << "BLEMacV2::prepareNewDataPkt PktLeft_TX_Size=" << PktLeft_TX_Size << " ,MaxPossiblePktPayloadBytes=" << MaxPossiblePktPayloadBytes << endl;

    if(PktLeft_TX_Size>MaxPossiblePktPayloadBytes){//we cannot fit all our data in single packet

        if(debug_Internal_Messages)  ev << "PktLeft_TX_Size > MaxPossiblePktPayloadBytes" << endl;

        DataPkt->setHdr_lgth(MaxPossiblePktPayloadBytes);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        DataPkt->setHdr_MD(true);
        myConnPars.ConnInfo.moreData=true;
    }
    else if(PktLeft_TX_Size > 0){//this is the last fragment
        if(debug_Internal_Messages) ev << "PktLeft_TX_Size > 0" << endl;
        DataPkt->setHdr_lgth(PktLeft_TX_Size);

        if(DataPkt_FirstFragment){
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PDU start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
            //DataPkt_FirstFragment=false;
            DataPkt_FirstFragmentSent=true;
        }
        else{
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1 = LL Data PDU continuation fragment of an L2CAP or an empty PDU" << endl;
        }
        if(macQueue.size()>1){
            DataPkt->setHdr_MD(true);
            myConnPars.ConnInfo.moreData=true;
        }
        else{
            DataPkt->setHdr_MD(false);
        }
        //encapsulate the packet
        cPacket * pkt=macQueue.front()->dup();
        DataPkt->encapsulate(pkt);

        //delete macQueue.front();
    }
    // original
   /* else{//we do not have any data to TX - just send empty packet
        EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx" << endl;
        DataPkt->setHdr_lgth(0);
        DataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
       // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
        if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 2 = LL Data PUD start of an L2CAP msg or a complete L2CAP msg with no fragmentation" << endl;
        DataPkt->setHdr_MD(false);
    } */
    //end original

    else{ //we do not have any data to TX - just send empty packet
            EV << "BLEMacV2::prepareNewDataPkt: Tham check : no data to Tx - just send empty packet" << endl;
            DataPkt->setHdr_lgth(0);
            DataPkt->setHdr_LLID(BLEstructs::LLID_Data_frag_1);
           // if(debug_Pkt_Basic) ev << "BLEMacV2::prepareNewDataPkt LLID 2" << endl; // original
            if(debug_Pkt_Basic)  ev << "BLEMacV2::prepareNewDataPkt LLID 1: LL Data PDU: Continuation fragment of an L2CAP msg, or an empty PDU" << endl;
            DataPkt->setHdr_MD(false);
        }

    DataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);

    LastPacketPayload=DataPkt->getHdr_lgth();

    if(debug_Internal_Messages) EV<<"BLEMacV2::prepareNewDataPkt - LastPacketPayload="<< LastPacketPayload << endl;

    DataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);

    //calculate the length of the packet
    PktBitLgth=DataPkt->getHdr_lgth()*8 + phyHeaderLength + llDataHeaderLengthBits;

    DataPkt->setBitLength(PktBitLgth);

    // generate challenge
    //ChallengeIndex_data = rand() % 3;
        int arr[] = {1, 2, 3};
        int freq[] = {60,30,10};
        int n = sizeof(arr) / sizeof(arr[0]);

        // Use a different seed value for every run.
        srand(time(NULL));

        ChallengeIndex_data = myRand(arr, freq, n) - 1;



    EV << "BLEMacV2::prepareNewDataPkt_Master : ChallengeIndex_data  = " << ChallengeIndex_data << endl;
    EV << " BLEMacV2::prepareNewDataPkt_Master: challenge = " << prob_c[ChallengeIndex_data] << endl;

    double Challenge_data = prob_c[ChallengeIndex_data];

    index_T ++;

       // temp_ch = prob_c[RandIndex_c];
        //challenge_list[index_T]=RandIndex_c;


    DataPkt->setChallenge_data(Challenge_data); // Tham added response
    DataPkt->setChallenge_index(ChallengeIndex_data);//Tham added

    eventQuerryChange(-(DataPkt->getHdr_lgth()));

    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepareNewDataPkt - packetID= " << DataPkt->getSequenceId() <<" Actual packet length = "<< PktBitLgth <<
                "bits of which: payload= " << DataPkt->getHdr_lgth()*8 << "bits, LL header= " << llDataHeaderLengthBits  <<
                "bits, PHY header=" <<  phyHeaderLength << "bits, ST_DataQueryLgth= "<< TST_DataQueryLgth << " bytes" <<
                endl;
      }

    myConnPars.ConnInfo.delayedGeneration=false;
    return true;
}


//prepares a connection terminate packet and estimates whether we might send it straight ahead
bool BLEMacV2::prepareConnTerminatePkt(simtime_t RemainingTime){
    long int PktBitLgth;
    int PktPayloadBytes=2;//Opcode + 1 byte Error code (see p. 2515 of Spec v 4.1)
    HighPriorityDataPkt->setHdr_lgth(PktPayloadBytes);
    HighPriorityDataPkt->setHdr_LLID(BLEstructs::LLID_Data_ctrl_3);
    HighPriorityDataPkt->setOpcode(BLEstructs::LL_TERMINATE_IND);
    HighPriorityDataPkt->setHdr_MD(true);//NOTE: - should be true - to get the ACK
    HighPriorityDataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);
    HighPriorityDataPkt->setErrorCode(myCmdError->ErrorCode);

    //DEBUG
    HighPriorityDataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);
    //DEBUG

    //calculate the length of the packet
    PktBitLgth=HighPriorityDataPkt->getHdr_lgth()*8+phyHeaderLength+llDataHeaderLengthBits;
    HighPriorityDataPkt->setBitLength(PktBitLgth);
    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepare ConnTerminatePkt - packetID=" << HighPriorityDataPkt->getSequenceId() <<" Actual packet length (bits)="<< PktBitLgth <<
                " of which: payload=" << HighPriorityDataPkt->getHdr_lgth()*8 << "bits, LL header=" << llDataHeaderLengthBits  <<
                " bits, PHY header=" <<  phyHeaderLength << " bits, ST_DataQueryLgth="<< TST_DataQueryLgth << " bytes" <<
                endl;
    }

    Flag_HighPriorityTraffic=true;

    if(RemainingTime>(PktBitLgth/bitrate)) return true;
    else return false;
}

//prepares a channel map update packet and estimates whether we might send it straight ahead
bool BLEMacV2::prepareChannelMapUpdatePkt(simtime_t RemainingTime){
    long int PktBitLgth;
    int MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39;
    int PktPayloadBytes=7;//Opcode + 2 byte Intant + 5 byte Channel Map (see p.2514 of spec v 4.1)

    myConnPars.ConnInfo.new_Data_Channel_Map.getMap(&MapCh0to7,&MapCh8to15,&MapCh16to23,&MapCh24to31,&MapCh32to39);

    HighPriorityDataPkt->setHdr_lgth(PktPayloadBytes);
    HighPriorityDataPkt->setHdr_LLID(BLEstructs::LLID_Data_ctrl_3);
    HighPriorityDataPkt->setOpcode(BLEstructs::LL_CHANNEL_MAP_REQ);
    HighPriorityDataPkt->setHdr_MD(true);
    HighPriorityDataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);

    HighPriorityDataPkt->setMapCh0to7(MapCh0to7);
    HighPriorityDataPkt->setMapCh8to15(MapCh8to15);
    HighPriorityDataPkt->setMapCh16to23(MapCh16to23);
    HighPriorityDataPkt->setMapCh24to31(MapCh24to31);
    HighPriorityDataPkt->setMapCh32to39(MapCh32to39);

    myConnPars.ConnInfo.Instant_NewMap=myConnPars.ConnInfo.getInstance();
    HighPriorityDataPkt->setInstant(myConnPars.ConnInfo.Instant_NewMap);

    //DEBUG
    HighPriorityDataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);
    //DEBUG

    //fill in the NESN & SN (see p. 2545-2547 of spec v4.1)
    //HighPriorityDataPkt->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

    //calculate the length of the packet
    PktBitLgth=HighPriorityDataPkt->getHdr_lgth()*8+phyHeaderLength+llDataHeaderLengthBits;
    HighPriorityDataPkt->setBitLength(PktBitLgth);
    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepare LL_CHANNEL_MAP_REQ - packetID=" << HighPriorityDataPkt->getSequenceId() <<" Actual packet length (bits)="<< PktBitLgth <<
                " of which: payload=" << HighPriorityDataPkt->getHdr_lgth()*8 << "bits, LL header=" << llDataHeaderLengthBits  <<
                " bits, PHY header=" <<  phyHeaderLength << " bits" <<
                endl;
    }

    Flag_HighPriorityTraffic=true;

    if(RemainingTime>(PktBitLgth/bitrate)) return true;
    else return false;
}


//prepares a connection  update packet and estimates whether we might send it straight ahead
bool BLEMacV2::prepareConnectionUpdatePkt(simtime_t RemainingTime){
    long int PktBitLgth;
    int PktPayloadBytes=12;//Opcode + 11 bytes of CtrData (see p.2514 of spec v 4.1)
    HighPriorityDataPkt->setHdr_lgth(PktPayloadBytes);

    HighPriorityDataPkt->setHdr_LLID(BLEstructs::LLID_Data_ctrl_3);
    HighPriorityDataPkt->setOpcode(BLEstructs::LL_CONNECTION_UPDATE_REQ);
    HighPriorityDataPkt->setHdr_MD(true);
    HighPriorityDataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);

    int new_connInterval=myConnPars.ConnInfo.getInterval(myConnPars.New_ConnPars.Conn_Interval_Min,myConnPars.New_ConnPars.Conn_Interval_Max);
    int new_connSlaveLatency=myConnPars.New_ConnPars.Conn_Latency;
    int new_connSupervisionTimeout=myConnPars.New_ConnPars.Supervision_Timeout;

    myConnPars.ConnInfo.new_connInterval=new_connInterval;
    myConnPars.ConnInfo.new_connSlaveLatency=new_connSlaveLatency;
    myConnPars.ConnInfo.new_connSupervisionTimeout=new_connSupervisionTimeout;

    HighPriorityDataPkt->setWinOffset(myConnPars.ConnInfo.transmitWindowOffset);
    HighPriorityDataPkt->setWinSize(myConnPars.ConnInfo.transmitWindowSize);
    HighPriorityDataPkt->setInterval(myConnPars.ConnInfo.new_connInterval);
    HighPriorityDataPkt->setLatency(myConnPars.ConnInfo.new_connSlaveLatency);
    HighPriorityDataPkt->setTimeout(myConnPars.ConnInfo.new_connSupervisionTimeout);

    myConnPars.ConnInfo.Instant_NewParameters=myConnPars.ConnInfo.getInstance();
    HighPriorityDataPkt->setInstant(myConnPars.ConnInfo.Instant_NewParameters);

    //DEBUG
    HighPriorityDataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);
    //DEBUG

    //fill in the NESN & SN (see p. 2545-2547 of spec v4.1)
    //HighPriorityDataPkt->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

    //calculate the length of the packet
    PktBitLgth=HighPriorityDataPkt->getHdr_lgth()*8+phyHeaderLength+llDataHeaderLengthBits;
    HighPriorityDataPkt->setBitLength(PktBitLgth);
    if(debug_Pkt_Basic){
        EV<<"BLEMacV2::prepare LL_CONNECTION_UPDATE_REQ - packetID=" << HighPriorityDataPkt->getSequenceId() <<" Actual packet length (bits)="<< PktBitLgth <<
                " of which: payload=" << HighPriorityDataPkt->getHdr_lgth()*8 << "bits, LL header=" << llDataHeaderLengthBits  <<
                " bits, PHY header=" <<  phyHeaderLength << " bits" <<
                endl;
    }

    Flag_HighPriorityTraffic=true;

    if(RemainingTime>(PktBitLgth/bitrate)) return true;
    else return false;
}

bool BLEMacV2::prepareEmptyACKPkt(simtime_t RemainingTime){
    long int PktBitLgth;
     int PktPayloadBytes=0;//
     HighPriorityDataPkt->setHdr_lgth(PktPayloadBytes);
     HighPriorityDataPkt->setHdr_LLID(BLEstructs::LLID_Data_start_2);
     HighPriorityDataPkt->setHdr_MD(false);
     HighPriorityDataPkt->setAccessAddress(myConnPars.ConnInfo.accessAddr);

     //DEBUG
     HighPriorityDataPkt->setSequenceId(DEBUG_PKT_SEQ_ID++);
     //DEBUG

     //fill in the NESN & SN (see p. 2545-2547 of spec v4.1)
     //DataPkt->setHdr_SN(myConnPars.ConnInfo.transmitSeqNum);//see p. 2545 of spec v4.1

     //calculate the length of the packet
     PktBitLgth=HighPriorityDataPkt->getHdr_lgth()*8+phyHeaderLength+llDataHeaderLengthBits;
     HighPriorityDataPkt->setBitLength(PktBitLgth);
     if(debug_Pkt_Basic){
         EV<<"BLEMacV2::prepare Empty ACK packet - packetID=" << HighPriorityDataPkt->getSequenceId() <<" Actual packet length (bits)="<< PktBitLgth <<
                 " of which: payload=" << HighPriorityDataPkt->getHdr_lgth()*8 << "bits, LL header=" << llDataHeaderLengthBits  <<
                 " bits, PHY header=" <<  phyHeaderLength << " bits, ST_DataQueryLgth="<< TST_DataQueryLgth << " bytes" <<
                 endl;
     }
     Flag_HighPriorityTraffic=true;

     if(RemainingTime>(PktBitLgth/bitrate)) return true;
     else return false;
}

//check if we have enough time to send the packet
bool BLEMacV2::checkCurrentDataPkt(simtime_t RemainingTime){
    long int PktBitLgth=DataPkt->getHdr_lgth()*8;
    if(RemainingTime<((phyHeaderLength+llDataHeaderLengthBits+PktBitLgth)/bitrate)){//we cannot fit the packet in the remaining time
        return false;
    }
    else return true;
}

//check if we have enough time to send the packet
bool BLEMacV2::checkHighPriorityDataPkt(simtime_t RemainingTime){
    long int PktBitLgth=HighPriorityDataPkt->getHdr_lgth()*8;
    if(RemainingTime<((phyHeaderLength+llDataHeaderLengthBits+PktBitLgth)/bitrate)){//we cannot fit the packet in the remaining time
        return false;
    }
    else return true;
}

//check if we have enough time to send the current packet
bool BLEMacV2::checkCurrentAdvPkt(simtime_t RemainingTime){
    long int PktBitLgth=AdvMessage->getBitLength();
    simtime_t RequiredTime=((phyHeaderLength+llAdvHeaderLengthBits+PktBitLgth)/bitrate);
    if(debug_Internal_Messages)
        EV<< "BLEMacV2::checkCurrentAdvPkt remaining time="  << RemainingTime << " required time=" << RequiredTime << endl;
    if(RemainingTime<RequiredTime){//we cannot fit the packet in the remaining time
        return false;
    }
    else return true;
}

//check if we have data to send
bool BLEMacV2::checkBuffer(int AcknowledgedReception){
    //if((PktLeft_TX_Size>0)&&(Flag_DataPktPending==false)){
    if(PktLeft_TX_Size>0){
        PktLeft_TX_Size=PktLeft_TX_Size-AcknowledgedReception;
    }
    ev<<"BLEMacV2::updateBuffer PktLeft_TX_Size=" << PktLeft_TX_Size << " AcknowledgedReception=" << AcknowledgedReception << endl;
    if(PktLeft_TX_Size<0){
        error("BLEMacV2::updateBuffer somehow PktLeft_TX_Size is < 0!");
    }
    else if(PktLeft_TX_Size>0){
        return true;
    }
    else if(PktLeft_TX_Size==0){
        while(macQueue.size()){
            delete macQueue.front();//
            macQueue.pop_front();
            if(macQueue.size()){
                cPacket * pkt=macQueue.front();
                //error("BINGO!");
                int temp_PktSize=pkt->getByteLength();
                if(temp_PktSize){
                    PktLeft_TX_Size=temp_PktSize;
                    DataPkt_FirstFragment=true;
                    DataPkt_FirstFragmentSent=false;
                    ev<<"BLEMacV2::updateBuffer we have more packets in buffer. Next PktLeft_TX_Size=" << PktLeft_TX_Size << endl;
                    return true;
                }
                else{
                    ev<<"BLEMacV2::updateBuffer No more data in the buffer!" << PktLeft_TX_Size << endl;
                }
            }
            else{
                //error("BONGO!");
                ev<<"BLEMacV2::updateBuffer buffer is empty!" << endl;
                return false;
            }

        }
    }
}


simtime_t BLEMacV2::calculateNextDataEventTime(void){
    simtime_t EventTime;
    EventTime=myConnPars.ConnInfo.timeLastSuccesfullEvent+(myConnPars.ConnInfo.numMissedEvents+1)*myConnPars.ConnInfo.connInterval*0.00125;
    return EventTime;
}

//drop connection
void BLEMacV2::dropConnection(void){
    bool StopCondition = par("TST_stopSimulation_ConnectionBreak");
    if(StopCondition==true)endSimulation();
    else{
        ctrl_switchState->setKind(ANY_to_STANDBY);
        //EV<< "BLEMacV2:dropConnection. Switching to IDLE state!" << endl;
        scheduleAt(simTime(), ctrl_switchState);

        if(debug_Internal_Messages)
            EV<< "BLEMacV2:dropConnection. Current TST_DataQueryLgth=" << TST_DataQueryLgth << endl;
        //"return" untransmitted data from the armed packet back to buffer
        if(myConnPars.ConnInfo.delayedGeneration!=true){
            TST_DataQueryLgth=TST_DataQueryLgth+DataPkt->getHdr_lgth();
            eventQuerryChange(DataPkt->getHdr_lgth());//update querry
        }
        if(debug_Internal_Messages)
            EV<< "BLEMacV2:dropConnection. Fixed TST_DataQueryLgth=" << TST_DataQueryLgth << endl;



    }
    eventConnectionDroped();
}

//drop connection
void BLEMacV2::stopSimulation(void){
    endSimulation();
}

void BLEMacV2::stopAllTimers(void){
    cancelEvent(advertisementEventTimer);
    cancelEvent(advertisementNextPDUTimer);
    cancelEvent(advertisementEndEvent);

    cancelEvent(initiatingScanIntervalTimer);
    cancelEvent(initiatingScanWindowTimer);

    cancelEvent(slaveConnectionWakeupTimer);
    cancelEvent(slaveConnectionNoBeaconTimer);
    cancelEvent(slaveIFSTimer);
    cancelEvent(slaveWaitReplyTimer);
    cancelEvent(slaveEndConnectionEvent);
    cancelEvent(slaveConnectionSupervisionTimer);
    cancelEvent(slaveConnectionWakeupConnectionTimer);
    cancelEvent(slaveConnectionTransmitWindowTimer);

    cancelEvent(masterConnectionWakeupTimer);
    cancelEvent(masterWaitReplyTimer);
    cancelEvent(masterIFSTimer);
    cancelEvent(masterEndConnectionEvent);
    cancelEvent(masterConnectionSupervisionTimer);

    //cancelEvent(TST_switchoffTimer);
}

void BLEMacV2::initVariablesAtStateStart(void){
    switch(macState){
    case STANDBY_1:
        break;
    case ADVERTISING_2:
        startTimer(TIMER_ADVERTISE_EVENTSTART);
        break;
    case INITIATING_4:
        scheduleAt(simTime(), initiatingScanIntervalTimer);
        break;
    case SCANNING_3: // Tham added
        scheduleAt(simTime(), initiatingScanIntervalTimer);
        break;
    case CONNECTION_5:{
            int myIdx=FindModule<>::findHost(this)->getIndex();
            int value = par("connStartingChannel");
            if(value<0) value = FindModule<>::findHost(this)->getParentModule()->getSubmodule("master",myIdx)->getSubmodule("nic")->getSubmodule("mac")->par("connStartingChannel");
            myConnPars.ConnInfo.unmappedChannel=value;
            myConnPars.ConnInfo.lastunmappedChannel=value;
            myConnPars.ConnInfo.connEventCounter=0;
            //ConnectionTermination=NONE_0;
            Flag_PeerTerminatesConnection=false;
            Flag_HighPriorityTraffic=false;
            Flag_UpdatePending_DataChannelMap=false;
            Flag_UpdatePending_ConnectionPars=false;
            Flag_UpdatePending_TerminateConnection=false;
            Flag_DelayedSwitchOff=false;
            myConnPars.ConnInfo.Instant_NewMap=-1;
            myConnPars.ConnInfo.transmitWindowOffset=par("transmitWindowOffset");
    }

        if(macSubState==CONNECTED_MST_SLEEP){
            startTimer(TIMER_CONNECTION_MASTER_WAKEUP_FIRST);
            startTimer(TIMER_CONNECTION_MASTER_SUPERVISION_FIRST);
            //startTimer(TIMER_CONNECTION_MASTER_SUPERVISION);
            myConnPars.ConnInfo.timeLastSuccesfullEvent=masterConnectionWakeupTimer->getArrivalTime()-myConnPars.ConnInfo.connInterval*0.00125;
             //masterConnectionWakeupTimer->getArrivalTime();
        }
        else if(macSubState==CONNECTED_SLV_WAITCONNECTION){
            startTimer(TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION_FIRST);
            //startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION);
            startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION_FIRST);
            //myConnPars.ConnInfo.timeLastAnchorReceived=
            myConnPars.ConnInfo.timeLastAnchorReceived=masterConnectionWakeupTimer->getArrivalTime();//although, not really necessary
            prepareNewDataPkt(1);//WILL NOT WORK OTHERWISE - master in first event will request packet retransmission - thus we should have something ready
        }
        else{
            error("BLEMacV2::initVariablesAtStateStart unsupported substate!");
        }
        myConnPars.ConnInfo.numMissedEvents=0;
        myConnPars.ConnInfo.transmitSeqNum=false;
        myConnPars.ConnInfo.nextExpectedSeqNum=false;
        myConnPars.ConnInfo.delayedGeneration=true;
        myConnPars.ConnInfo.moreData=false;
        break;


    default:
        error("BLEMacV2::initVariablesAtStateStart - Unknown state");
        break;
    }
}

//STATISTICS LOGGING STUFF

void BLEMacV2::eventRadioChannelChanged(void){
    //Fast and dirty solution
    Stat_FrequencyHops *FreqStats  = FindModule<Stat_FrequencyHops*>::findSubModule(this->getParentModule()->getParentModule());
    FreqStats->Log(phy->getCurrentRadioChannel());
    //
}

void BLEMacV2::eventRadioStateChanged(void){
    //Fast and dirty solution
    Stat_RadioState *StateStats  = FindModule<Stat_RadioState*>::findSubModule(this->getParentModule()->getParentModule());
    StateStats->Log(phy->getRadioState());
}

void BLEMacV2::eventQuerryChange(int Change){
    //Fast and dirty solution
    //EV << "BLEMacV2::eventQuerryChange new value=" << << endl;
    if(Change!=0){
        Stat_QuerryData *QuerryStats  = FindModule<Stat_QuerryData*>::findSubModule(this->getParentModule()->getParentModule());
        //BLEnwkStats->change_NumBytes(Change);
        if(Buf_FreeSize+Change<0)error("BLEMacV2::eventQuerryChange buffer overfill");
        Buf_FreeSize=Buf_FreeSize-Change;
        //ev<<"BLEMacV2::eventQuerryChange Buf_TotalSize=" << Buf_TotalSize << " Buf_FreeSize=" << Buf_FreeSize << endl;
        QuerryStats->LogQuerry(Buf_TotalSize-Buf_FreeSize);
    }
}

void BLEMacV2::eventDataNewRXd(int DataLgth){
    //Fast and dirty solution
    Stat_QuerryData *QuerryStats  = FindModule<Stat_QuerryData*>::findSubModule(this->getParentModule()->getParentModule());
    QuerryStats->LogReceivedNew(DataLgth);
}

void BLEMacV2::eventDataReTXRXd(int DataLgth){
    //Fast and dirty solution
    Stat_QuerryData *QuerryStats  = FindModule<Stat_QuerryData*>::findSubModule(this->getParentModule()->getParentModule());
    QuerryStats->LogReceivedReTX(DataLgth);
}

void BLEMacV2::eventConnectionCompleted(void){

    //inform NWK layer
    /*cMessage *m;
    m = new cMessage("BLE_MACtoNWK_EVENT");
    m->setControlInfo(BLE_MacToNwk::generate_LEConnectionCompleteEvent());
    sendControlUp(m);*/
    myConnPars.ConnInfo.connectionHandle=777;//TODO: this is fine while we support only one connection per node
    sendEventNotificationToNwk(cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Complete_Event);

    //Fast and dirty solution for logging
    Stat_BLEConnection *BLEConnection  = FindModule<Stat_BLEConnection*>::findSubModule(this->getParentModule()->getParentModule());
    BLEConnection->ConnEstablished();
}

void BLEMacV2::eventConnectionDroped(void){
    //Fast and dirty solution
    Stat_BLEConnection *BLEConnection  = FindModule<Stat_BLEConnection*>::findSubModule(this->getParentModule()->getParentModule());
    BLEConnection->ConnDroped();
}

void BLEMacV2::eventNoRSPTimer(void){
    //Fast and dirty solution
    Stat_BLEConnection *BLEConnection  = FindModule<Stat_BLEConnection*>::findSubModule(this->getParentModule()->getParentModule());
    BLEConnection->NoRSPTimer();
}

//original --
/*
void BLEMacV2::handleUpperCommand_Adv(BLE_NwkToMac *const cCmdInfo){
    cBLEstructs_cAdvertisePars* AdvParsPointer=cCmdInfo->get_AdvertisingParsPtr();
    if(!AdvParsPointer) error("BLEMacV2::handleUpperCommand_Adv: pointer to AdvertisePars is empty!");
    //EV << "BLEMacV2::handleUpperCommand_Adv: printing old settings:" <<endl;
    //myAdvrtPars.print();
    switch(cCmdInfo->get_CmdType()){
        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand:
            if(debug_UpperCommands){
                EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertisingParametersCommand"<< endl;
            }
            myAdvrtPars.Adv_Type=AdvParsPointer->Adv_Type;
            myAdvrtPars.Advertising_Interval_Max=AdvParsPointer->Advertising_Interval_Max;
            if(debug_UpperCommands){
                EV << "BLEMacV2::handleUpperCommand_Adv: New Adv_Type="<< myAdvrtPars.Adv_Type << " Advertising_Interval_Max=" << myAdvrtPars.Advertising_Interval_Max << endl;
            }

            myAdvrtPars.Advertising_Interval_Min=AdvParsPointer->Advertising_Interval_Min;
            myAdvrtPars.Advertising_Channel_Map=AdvParsPointer->Advertising_Channel_Map;
            myAdvrtPars.Direct_Address_Type=AdvParsPointer->Direct_Address_Type;
            myAdvrtPars.Direct_Address=AdvParsPointer->Direct_Address;

            //TODO: Note, the pars below are currently not accounted for!
            myAdvrtPars.Own_Address_Type=AdvParsPointer->Own_Address_Type;
            myAdvrtPars.Advertising_Filter_Policy=AdvParsPointer->Advertising_Filter_Policy;

            updateAdvertismentPkt();
        break;

        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingDataCommand:
            if(debug_UpperCommands){
                EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertisingDataCommand"<< endl;
            }
            myAdvrtPars.AdvPacketLgth=AdvParsPointer->AdvPacketLgth;
            if(myAdvrtPars.AdvPacketLgth==0) myAdvrtPars.AdvPacket=NULL;
            else myAdvrtPars.AdvPacket=AdvParsPointer->AdvPacket;
            updateAdvertismentPkt();
        break;

        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertiseEnableCommand:
            if(debug_UpperCommands)
                EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertiseEnableCommand"<< endl;
            switch(macState){
            case STANDBY_1:
                if(AdvParsPointer->AdvEnabled==true){//we need to start, else - ignore command
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(STANDBY_to_ADVERTIZING);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                break;
            case ADVERTISING_2:
                if((myAdvrtPars.AdvEnabled==true)&&(AdvParsPointer->AdvEnabled==true)){
                    //we are already advertising - ignore
                }
                else if((AdvParsPointer->AdvEnabled==true)&&(myAdvrtPars.AdvEnabled==false)){
                    //we must start advertising
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(STANDBY_to_ADVERTIZING);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                else if((AdvParsPointer->AdvEnabled==false)&&(myAdvrtPars.AdvEnabled==true)){
                    //we must start advertising
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(ANY_to_STANDBY);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                else if((AdvParsPointer->AdvEnabled==false)&&(myAdvrtPars.AdvEnabled==false)){
                    //we are already in standby - ignore
                }
                break;
            case SCANNING_3:
                EV <<"Tham added this state - To do switch from SCANNING to ADVERTISING" << endl;
            break;
            case INITIATING_4:
                error("BLEMacV2::handleUpperCommand_Adv: we can not switch from CONNECTION to ADVERTISING!");
                break;
            case CONNECTION_5:
                error("BLEMacV2::handleUpperCommand_Adv: we can not switch from CONNECTION to ADVERTISING!");
                break;
            default:
                error("BLEMacV2::handleUpperCommand_Adv unsupported event!");
                break;
            }
        break;

        default:
           error("BLEMacV2: received unsupported command from upper layer!");
        break;
    }
    //EV << "BLEMacV2::handleUpperCommand_Adv: printing new settings:" <<endl;
    //myAdvrtPars.print();
}
 */

// Tham modified
void BLEMacV2::handleUpperCommand_Adv(BLE_NwkToMac *const cCmdInfo){

    cBLEstructs_cAdvertisePars* AdvParsPointer=cCmdInfo->get_AdvertisingParsPtr();

    if(!AdvParsPointer) error("BLEMacV2::handleUpperCommand_Adv: pointer to AdvertisePars is empty!");

   // EV << "BLEMacV2::handleUpperCommand_Adv: printing old settings:" <<endl;
    //myAdvrtPars.print();

    switch(cCmdInfo->get_CmdType()){
        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand:
            if(debug_UpperCommands){
                EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertisingParametersCommand"<< endl;
             }
            myAdvrtPars.Adv_Type=AdvParsPointer->Adv_Type;
            myAdvrtPars.Advertising_Interval_Max=AdvParsPointer->Advertising_Interval_Max;

            if(debug_UpperCommands){
            EV << "BLEMacV2::handleUpperCommand_Adv: New Adv_Type="<< myAdvrtPars.Adv_Type << " Advertising_Interval_Max=" << myAdvrtPars.Advertising_Interval_Max << endl;
            }

            myAdvrtPars.Advertising_Interval_Min=AdvParsPointer->Advertising_Interval_Min;
            myAdvrtPars.Advertising_Channel_Map=AdvParsPointer->Advertising_Channel_Map;
            myAdvrtPars.Direct_Address_Type=AdvParsPointer->Direct_Address_Type;
            myAdvrtPars.Direct_Address=AdvParsPointer->Direct_Address;

            //TODO: Note, the pars below are currently not accounted for!

            myAdvrtPars.Own_Address_Type=AdvParsPointer->Own_Address_Type;
            myAdvrtPars.Advertising_Filter_Policy = AdvParsPointer->Advertising_Filter_Policy;

            updateAdvertismentPkt();

        break;

        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingDataCommand:
            if(debug_UpperCommands){
            EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertisingDataCommand"<< endl;
            }
            myAdvrtPars.AdvPacketLgth=AdvParsPointer->AdvPacketLgth;
            ev << "Tham - BLEMacV2::handleUpperCommand_Adv : myAdvrtPars.AdvPacketLgth = " << myAdvrtPars.AdvPacketLgth << endl;
           // ev <<" Tham - BLEMacV2::handleUpperCommand_Adv AdvPacket pointer = " << myAdvrtPars.AdvPacket << endl;

            ev <<" Tham - BLEMacV2::handleUpperCommand_Adv AdvPacket pointer before if else= " << myAdvrtPars.AdvPacket << endl;

               if(myAdvrtPars.AdvPacketLgth==0) myAdvrtPars.AdvPacket=NULL;
               else {
                       myAdvrtPars.AdvPacket= AdvParsPointer->AdvPacket;
                       ev <<" Tham - BLEMacV2::handleUpperCommand_Adv AdvPacket pointer after if else = " << myAdvrtPars.AdvPacket << endl;
               }

           // if(myAdvrtPars.AdvPacketLgth==0) myAdvrtPars.AdvPacket=NULL;
            //else myAdvrtPars.AdvPacket= AdvParsPointer->AdvPacket;


            updateAdvertismentPkt();

        break;

        case cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertiseEnableCommand:
            if(debug_UpperCommands)
                EV << "BLEMacV2::handleUpperCommand_Adv: LESetAdvertiseEnableCommand"<< endl;
            switch(macState){
            case STANDBY_1:
                if(AdvParsPointer->AdvEnabled==true){//we need to start, else - ignore command
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(STANDBY_to_ADVERTIZING);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                break;
            case ADVERTISING_2:
                if((myAdvrtPars.AdvEnabled==true)&&(AdvParsPointer->AdvEnabled==true)){
                    //we are already advertising - ignore
                }
                else if((AdvParsPointer->AdvEnabled==true)&&(myAdvrtPars.AdvEnabled==false)){
                    //we must start advertising
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(STANDBY_to_ADVERTIZING);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                else if((AdvParsPointer->AdvEnabled==false)&&(myAdvrtPars.AdvEnabled==true)){
                    //we must start advertising
                    myAdvrtPars.AdvEnabled=AdvParsPointer->AdvEnabled;
                    ctrl_switchState->setKind(ANY_to_STANDBY);
                    scheduleAt(simTime(), ctrl_switchState);
                }
                else if((AdvParsPointer->AdvEnabled==false)&&(myAdvrtPars.AdvEnabled==false)){
                    //we are already in standby - ignore
                }
                break;
            case SCANNING_3:
                EV <<"Tham added this state - To do switch from SCANNING to ADVERTISING" << endl;
            break;
            case INITIATING_4:
                error("BLEMacV2::handleUpperCommand_Adv: we can not switch from CONNECTION to ADVERTISING!");
                break;
            case CONNECTION_5:
                error("BLEMacV2::handleUpperCommand_Adv: we can not switch from CONNECTION to ADVERTISING!");
                break;
            default:
                error("BLEMacV2::handleUpperCommand_Adv unsupported event!");
                break;
            }
        break;

        default:
           error("BLEMacV2: received unsupported command from upper layer!");
        break;
    }
    //EV << "BLEMacV2::handleUpperCommand_Adv: printing new settings:" <<endl;
    //myAdvrtPars.print();
}




void BLEMacV2::handleUpperCommand_Initiate(BLE_NwkToMac *const cCmdInfo){
    cBLEstructs_cConnectionPars* ConParsPointer=cCmdInfo->get_ConnectionParsPtr();
    cBLEstructs_cScanningPars* ScanParsPointer=cCmdInfo->get_ScanningParsPtr();
    //if(debug_UpperCommands) EV << "BLEMacV2::handleUpperCommand_Initiate: printing old settings:" <<endl;
    switch(cCmdInfo->get_CmdType()){
        case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Command:{
           // EV <<"Tham : LE_Create_Connection_Command" << endl;
            if(debug_UpperCommands)
            EV << "BLEMacV2::handleUpperCommand_Initiate: LE_Create_Connection_Command"<< endl;
            if((!ConParsPointer)||(!ScanParsPointer)) error("BLEMacV2::handleUpperCommand_Initiate: pointer to ConParsPointer or ScanParsPointer is empty!");
            switch(macState){
                case STANDBY_1:
                    //modify parameters
                    {
                        EV << "Tham: at Standby state" << endl;
                        // set scanning connection parameters
                        myScanPars.LE_Scan_Interval=ScanParsPointer->LE_Scan_Interval;
                        myScanPars.LE_Scan_Window=ScanParsPointer->LE_Scan_Window;
                        myScanPars.Own_Address_Type=ScanParsPointer->Own_Address_Type;
                        myScanPars.Peer_Address=ScanParsPointer->Peer_Address;
                        myScanPars.Peer_Address_Type=ScanParsPointer->Peer_Address_Type;

                        // set desired connection parameters
                        myConnPars.ConnPars.Conn_Interval_Max=ConParsPointer->Conn_Interval_Max;
                        myConnPars.ConnPars.Conn_Interval_Min=ConParsPointer->Conn_Interval_Min;
                        myConnPars.ConnPars.Conn_Latency=ConParsPointer->Conn_Latency;
                        myConnPars.ConnPars.Supervision_Timeout=ConParsPointer->Supervision_Timeout;
                        myConnPars.ConnInfo.connSlaveLatency=myConnPars.ConnPars.Conn_Latency;
                        myConnPars.ConnInfo.connSupervisionTimeout=myConnPars.ConnPars.Supervision_Timeout;

                    }
                    ctrl_switchState->setKind(STANDBY_to_INITIATING);
                    scheduleAt(simTime(), ctrl_switchState);
                    break;

                case ADVERTISING_2:
                    error("BLEMacV2::handleUpperCommand_Initiate: we can not switch from ADVERTISING to INITIATING!");
                    break;

                case INITIATING_4:
                    //modify parameters
                    {
                        EV << "Tham: at initiating state" << endl;
                        // set scanning connection parameters
                        myScanPars.LE_Scan_Interval=ScanParsPointer->LE_Scan_Interval;
                        myScanPars.LE_Scan_Window=ScanParsPointer->LE_Scan_Window;
                        myScanPars.Own_Address_Type=ScanParsPointer->Own_Address_Type;
                        myScanPars.Peer_Address=ScanParsPointer->Peer_Address;
                        myScanPars.Peer_Address_Type=ScanParsPointer->Peer_Address_Type;

                        // set desired connection parameters
                        myConnPars.ConnPars.Conn_Interval_Max=ConParsPointer->Conn_Interval_Max;
                        myConnPars.ConnPars.Conn_Interval_Min=ConParsPointer->Conn_Interval_Min;
                        myConnPars.ConnPars.Conn_Latency=ConParsPointer->Conn_Latency;
                        myConnPars.ConnPars.Supervision_Timeout=ConParsPointer->Supervision_Timeout;
                    }

                    break;
                case CONNECTION_5:
                    error("BLEMacV2::handleUpperCommand_Initiate: we can not switch from CONNECTION to INITIATING!");
                    break;
                default:
                    //EV <<"Tham - default" << endl;
                    error("BLEMacV2::handleUpperCommand_Initiate unsupported event!");
                    break;
            }
        }
        break;

        case cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command:{

           // EV << "Tham: LE_Create_Connection_Cancel_Command" << endl;
            if(macState==INITIATING_4){
                ctrl_switchState->setKind(ANY_to_STANDBY);
                scheduleAt(simTime(), ctrl_switchState);
            }
            else{
                //ignore command
            }
        }
        break;

        default:
           // EV <<"Tham check - default" << endl;
           error("BLEMacV2: received unsupported command from upper layer!");
        break;

    }
}


void BLEMacV2::sendEventNotificationToNwk(cBLEstructs_defs::BLECmdTypes_HCItoCtrl EventType){
    switch(EventType){
    case cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Complete_Event:
        {
            myEventData->Conn_Interval=myConnPars.ConnInfo.connInterval;
            myEventData->Conn_Latency=myConnPars.ConnInfo.connSlaveLatency;
            myEventData->Connection_Handle=myConnPars.ConnInfo.connectionHandle;
            myEventData->Peer_Address=myConnPars.ConnInfo.Peer_Address;
            myEventData->Peer_Address_Type=myConnPars.ConnInfo.Peer_Address_Type;
            myEventData->Master_Clock_Accuracy=myConnPars.ConnInfo.masterSCA;
            if((macSubState==CONNECTED_MST_SLEEP)||(macSubState==CONNECTED_MST_WAITRSP)||(macSubState==CONNECTED_MST_CONNECTIONEVENT)){
                myEventData->Role=cBLEstructs_defs::MASTER_0;
            }
            else if((macSubState==CONNECTED_SLV_WAITCONNECTION)||(macSubState==CONNECTED_SLV_SLEEP)||(macSubState==CONNECTED_SLV_WAITPKT)||(macSubState==CONNECTED_SLV_CONNECTIONEVENT)){
                myEventData->Role=cBLEstructs_defs::SLAVE_1;
            }
            myEventData->Status=0x00;//Successful
            myEventData->Supervision_Timeout=myConnPars.ConnInfo.connSupervisionTimeout;

            cMessage *m;
            m = new cMessage("BLE_MACtoNWK_EVENT");
            m->setControlInfo(BLE_MacToNwk::generate_LEConnectionCompleteEvent(myEventData));
            sendControlUp(m);
        }
        break;
    default:
        error("BLEMacV2: sendEventNotificationToNwk - unsupported event!");
        break;
    }

}

//original
/*
void BLEMacV2::updateAdvertismentPkt(void){
    int Length;
    delete AdvMessage;
    AdvMessage = new BLE_Adv_MacPkt("ADV_PDU");
    AdvMessage->setAccessAddress(0x8E89BED6);//see p 2503 of standard v4.1
    if(debug_Pkt_Basic) EV << " myAdvrtPars.Adv_Type = " << myAdvrtPars.Adv_Type<< endl;
    // ADV_IND packet
    if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_IND_0){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_IND_0);
        AdvMessage->setName("ADV_IND");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)=8 ; AdvPacketLgth set by Tham
        EV << "Packet ADV_IND length = " << Length << endl;
        EV <<"myAdvrtPars.AdvPacketLgth = "<< myAdvrtPars.AdvPacketLgth << endl;
        if(myAdvrtPars.AdvPacketLgth){
            EV <<"myAdvrtPars.AdvPacket = " << myAdvrtPars.AdvPacket << endl; // Tham added to debug
                    if(myAdvrtPars.AdvPacket){
                        AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
                    }
                    else{
                        error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
                    }
                }
    }
    // ADV_DIRECT_IND packet
    else if((myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_HDC_1)||(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_LDC_4)){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_DIRECT_IND_1);
        AdvMessage->setName("ADV_DIRECT_IND");
        Length=14;//2(header)+6(InitA)+6(AdvA)=14
    }
    // ADV_SCAN_IND packet
    else if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_SCAN_IND_2){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_SCAN_IND_6);
        AdvMessage->setName("ADV_SCAN_IND");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)= 8 + AdvData
        if(myAdvrtPars.AdvPacketLgth){
            if(myAdvrtPars.AdvPacket){
                AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
            }
        }
    }
    // ADV_NONCONN_IND packet
    else if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_NONCONN_IND_3){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_NONCONN_IND_2);
        AdvMessage->setName("ADV_NONCONN_IND");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)=8 + AdvData if is set > 0

        if(myAdvrtPars.AdvPacketLgth){
            if(myAdvrtPars.AdvPacket){
                AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
            }
        }
    }
    // check packet PDU type: PDU_ADV_DIRECT_IND
    if(AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_DIRECT_IND_1){
        AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
        if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
            AdvMessage->setTxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setInitA(myAdvrtPars.Direct_Address);
        if(myAdvrtPars.Direct_Address_Type==cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0){
            AdvMessage->setRxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setLength(Length);
        AdvMessage->setBitLength(Length*8);

        //print all out
        if(debug_Pkt_Basic){
            EV << "BLEMacV2::updateAdvertismentPkt" << endl;
            EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl; // Tham added PDU for match with the definded PDU type
            EV << "AdvA=" << AdvMessage->getAdvA() << endl;
            EV << "InitA=" << AdvMessage->getInitA() << endl;
            EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
            EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
            EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
        }
    }
    // PDU_ADV_NONCONN_IND_2
    else if(AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_NONCONN_IND_2){
        AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
        if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
            AdvMessage->setTxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setLength(Length);
        AdvMessage->setBitLength(Length*8);



        //print all out
        if(debug_Pkt_Basic){
            EV << "BLEMacV2::updateAdvertismentPkt" << endl;
            EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl;//Tham added 'PDU'
            EV << "AdvA=" << AdvMessage->getAdvA() << endl;
            EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
            EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
            EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
        }

    }

    else{
        error("BLEMacV2::updateAdvertismentPkt - PDU TYPE IS NOT supported YET!");
    }
}
*/

//Tham modified
void BLEMacV2::updateAdvertismentPkt(void){

    int Length;
    delete AdvMessage;
    AdvMessage = new BLE_Adv_MacPkt("ADV_PDU");
    AdvMessage->setAccessAddress(0x8E89BED6);//see p 2503 of standard v4.1 0x8E89BED6 to decimal = 2391391958
    // AdvMessage->setAccessAddress(2391391958); // tham 8/7/2018
    if(debug_Pkt_Basic) EV << "myAdvrtPars.Adv_Type = " << myAdvrtPars.Adv_Type << endl;

// check packet type

    // ADV_IND packet
    if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_IND_0){
        EV <<"Tham - ADV_IND packet" << endl;
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_IND_0);
        AdvMessage->setName("ADV_IND");
       //AdvMessage->setAdvData(0); // Tham
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)=8 + AdvPacketLgth for data set by Tham

        // EV << "Packet ADV_IND length = " << Length << endl;
       // EV << "For Advdata: myAdvrtPars.AdvPacketLgth = "<< myAdvrtPars.AdvPacketLgth << endl;

       // ev << "AdvMessage Adv data =" << AdvMessage->getAdvData() <<endl;
       // EV << "myAdvrtPars.AdvData = " << myAdvrtPars.AdvData << endl;
       // EV <<"myAdvrtPars.AdvPacket = " << myAdvrtPars.AdvPacket << endl; // Tham added to debug

       if(myAdvrtPars.AdvPacketLgth){
            EV <<"myAdvrtPars.AdvPacket = " << myAdvrtPars.AdvPacket << endl; // Tham added to debug
                    if(myAdvrtPars.AdvPacket != NULL){
                       AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());

                    }
                    else{
                        ev <<"pointer AdvPacket = NULL" << endl;
                        error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvPacketLgth!=0 but the message pointer is empty!");
                    }
                }

        // myAdvrtPars.print();//tham
    }

    // ADV_DIRECT_IND packet
    else if((myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_HDC_1)||(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_DIRECT_IND_LDC_4)){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_DIRECT_IND_1);
        AdvMessage->setName("ADV_DIRECT_IND");
        Length=14;//2(header)+6(InitA)+6(AdvA)=14
    }

    // ADV_SCAN_IND packet
    else if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_SCAN_IND_2){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_SCAN_IND_4);
        AdvMessage->setName("ADV_SCAN_IND");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)= 8 + AdvData
        if(myAdvrtPars.AdvPacketLgth){
            if(myAdvrtPars.AdvPacket){
                AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
            }
        }
    }

    // ADV_NONCONN_IND packet
    else if(myAdvrtPars.Adv_Type==cBLEstructs_defs::ADV_NONCONN_IND_3){
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_ADV_NONCONN_IND_2);
        AdvMessage->setName("ADV_NONCONN_IND");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)=8 + AdvData if is set > 0

        if(myAdvrtPars.AdvPacketLgth){
            if(myAdvrtPars.AdvPacket){
                AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
            }
        }
    }

     // Tham added SCAN REQ
    else if (myAdvrtPars.Adv_Type==cBLEstructs_defs::SCAN_REQ)
    {
        EV << "Tham SCAN_REQ packet" <<endl;
        AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_SCAN_REQ_5);
        AdvMessage->setName("SCAN_REQ");
        Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(InitA)=8

                        if(myAdvrtPars.AdvPacketLgth){
                            if(myAdvrtPars.AdvPacket){
                                AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
                            }
                            else{
                                error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
                            }
                        }

             //   myAdvrtPars.print();//tham

    }
    // Tham : added SCAN RSP
    else if (myAdvrtPars.Adv_Type==cBLEstructs_defs::SCAN_RSP)
        {
            EV << "Tham SCAN_RSP packet" <<endl;
            AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_SCAN_RSP_6);
            AdvMessage->setName("SCAN_RSP");
            Length=8+myAdvrtPars.AdvPacketLgth;//2(header)+6(AdvA)=8

                if(myAdvrtPars.AdvPacketLgth){
                    if(myAdvrtPars.AdvPacket){
                        AdvMessage->encapsulate(myAdvrtPars.AdvPacket->dup());
                    }
                    else{
                        error("BLEMacV2::updateAdvertismentPkt: myAdvrtPars.AdvMessageLgth!=0 but the message pointer is empty!");
                    }
                }

        }

// check packet PDU type: PDU_ADV_DIRECT_IND
    if(AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_DIRECT_IND_1){
        AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
        if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
            AdvMessage->setTxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setInitA(myAdvrtPars.Direct_Address);
        if(myAdvrtPars.Direct_Address_Type==cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0){
            AdvMessage->setRxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setLength(Length);
        AdvMessage->setBitLength(Length*8);

        //print all out
      //  if(debug_Pkt_Basic){
            EV << "BLEMacV2::updateAdvertismentPkt" << endl;
            EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl; // Tham added PDU for match with the definded PDU type
            EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
            EV << "AdvA=" << AdvMessage->getAdvA() << endl;
            EV << "InitA=" << AdvMessage->getInitA() << endl;
            EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
            EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
            EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
       // }
    }
    // PDU_ADV_NONCONN_IND_2
    else if(AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_NONCONN_IND_2){
        AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
        if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
            AdvMessage->setTxAdd(false);
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setLength(Length);
        AdvMessage->setBitLength(Length*8);



        //print all out
        if(debug_Pkt_Basic){
            EV << "BLEMacV2::updateAdvertismentPkt" << endl;
            EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl;//Tham added 'PDU'
            EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
            EV << "AdvA=" << AdvMessage->getAdvA() << endl;
            EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
            EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
            EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
        }

    } // Tham added to check PDU_ADV_IND_0
    else if (AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_ADV_IND_0){
        AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
        if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
            AdvMessage->setTxAdd(false); // TxAdd = 0 for public , = 1 for random - Tham follow spec
        }
        else{
            error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
        }

        AdvMessage->setLength(Length);
        AdvMessage->setBitLength(Length*8);

        //print all out
        if(debug_Pkt_Basic){
            EV << "BLEMacV2::updateAdvertismentPkt" << endl;
            EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl;//Tham added 'PDU'
            EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
            EV << "AdvA=" << AdvMessage->getAdvA() << endl;
            EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
            EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
            EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
           // EV << "Tham - The ADV_IND packet with AdvData length =" << AdvMessage->getLength() - 8 << endl;
        }

    } // // Tham added to check PDU_SCAN_REQ_3
    else if (AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_SCAN_REQ_5){
            AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
            if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
                AdvMessage->setTxAdd(false); // TxAdd = 0 for public , = 1 for random - Tham follow spec
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
            }

            AdvMessage->setLength(Length);
            AdvMessage->setBitLength(Length*8);

            //print all out
            if(debug_Pkt_Basic){
                EV << "BLEMacV2::updateAdvertismentPkt" << endl;
                EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl;//Tham added 'PDU'
                EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
                EV << "AdvA=" << AdvMessage->getAdvA() << endl;
                EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
                EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
                EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
                //EV << "Tham - The ADV_IND packet with AdvData length =" << AdvMessage->getLength() - 14 << endl;
            }

        }
    // Tham added to check PDU_SCAN_RSP_4
    else if (AdvMessage->getAdv_PDU_type()==cBLEstructs_defs::PDU_SCAN_RSP_6){
            AdvMessage->setAdvA(myAdvrtPars.OwnAdvAddr);
            if(myAdvrtPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
                AdvMessage->setTxAdd(false); // TxAdd = 0 for public , = 1 for random - Tham follow spec
            }
            else{
                error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
            }

            AdvMessage->setLength(Length);
            AdvMessage->setBitLength(Length*8);

            //print all out
            if(debug_Pkt_Basic){
                EV << "BLEMacV2::updateAdvertismentPkt" << endl;
                EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl;//Tham added 'PDU'
                EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
                EV << "AdvA=" << AdvMessage->getAdvA() << endl;
                EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
                EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
                EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
                //EV << "Tham - The ADV_IND packet with AdvData length =" << AdvMessage->getLength() - 8 << endl;
            }

        }


    else{
        error("BLEMacV2::updateAdvertismentPkt - PDU TYPE IS NOT supported YET!");
    }
}



void BLEMacV2::generateConnectionRequest(void){
    int MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39;

    delete AdvMessage;
    AdvMessage = new BLE_Adv_MacPkt("CONNECT_REQ");
    AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_CONNECT_REQ_3);
    AdvMessage->setAccessAddress(0x8E89BED6);//see p 2503 of standard v4.1 0x8E89BED6 to decimal = 2391391958
   // AdvMessage->setAccessAddress(2391391958); // tham 8/7/2018
    AdvMessage->setInitA(myAdvrtPars.OwnAdvAddr);
    if(myScanPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
        AdvMessage->setTxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    AdvMessage->setAdvA(myConnPars.ConnInfo.Peer_Address);
    if(myConnPars.ConnInfo.Peer_Address_Type==cBLEstructs_defs::PEER_ADDR_PUBLIC_0){
        AdvMessage->setRxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    if(debug_Pkt_Basic) EV << "BLEMacV2::myConnPars.ConnInfo.accessAddr=" << myConnPars.ConnInfo.accessAddr << endl;

    AdvMessage->setAA(myConnPars.ConnInfo.accessAddr);
    AdvMessage->setWinOffset(myConnPars.ConnInfo.transmitWindowOffset);
    AdvMessage->setWinSize(myConnPars.ConnInfo.transmitWindowSize);
    AdvMessage->setInterval(myConnPars.ConnInfo.connInterval);
    AdvMessage->setLatency(myConnPars.ConnPars.Conn_Latency);
    AdvMessage->setTimeout(myConnPars.ConnPars.Supervision_Timeout);



    myConnPars.ConnInfo.Data_Channel_Map.getMap(&MapCh0to7,&MapCh8to15,&MapCh16to23,&MapCh24to31,&MapCh32to39);

    AdvMessage->setMapCh0to7(MapCh0to7);
    AdvMessage->setMapCh8to15(MapCh8to15);
    AdvMessage->setMapCh16to23(MapCh16to23);
    AdvMessage->setMapCh24to31(MapCh24to31);
    AdvMessage->setMapCh32to39(MapCh32to39);

    int Hop;
    if(TST_forceHop<0)Hop=intuniform(5, 16, 0);
    else Hop=TST_forceHop;
    myConnPars.ConnInfo.hopIncreasement=Hop;

    AdvMessage->setHop(Hop);//see p. 2510 of spec v4.1

    AdvMessage->setSCA(mySCA);

    int Length=36;//2(header)+6(InitA)+6(AdvA) + 22 (LL Data )=34 - see p. 2509 of spec v4.1

    AdvMessage->setLength(Length);
    AdvMessage->setBitLength(Length*8);
    AdvMessage->setName("CONNECT_REQ");//Tham added


    //print all out
    if(debug_Pkt_Basic){
        EV << "BLEMacV2::generateConnectionRequest" << endl;
        EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl; // Tham added 'PDU' for matching with PDU type defined
        EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
        EV << "AdvA=" << AdvMessage->getAdvA() << endl;
        EV << "InitA=" << AdvMessage->getInitA() << endl;
        EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
        EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
        EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
        EV << "CONNECTION PARAMETERS:" << endl;
        EV << "AA=" << AdvMessage->getAA() << endl;
        EV << "WinOffset=" << AdvMessage->getWinOffset() << endl;
        EV << "WinSize=" << AdvMessage->getWinSize() << endl;
        EV << "Interval=" << AdvMessage->getInterval() << endl;
        EV << "Latency=" << AdvMessage->getLatency() << endl;
        EV << "Timeout=" << AdvMessage->getTimeout() << endl;
        EV << "MapCh0to7=" << AdvMessage->getMapCh0to7() << endl;
        EV << "MapCh8to15=" << AdvMessage->getMapCh8to15() << endl;
        EV << "MapCh16to23=" << AdvMessage->getMapCh16to23() << endl;
        EV << "MapCh24to31=" << AdvMessage->getMapCh24to31() << endl;
        EV << "MapCh32to39=" << AdvMessage->getMapCh32to39() << endl;
        EV << "Hop=" << AdvMessage->getHop() << endl;
        EV << "SCA=" << AdvMessage->getSCA() << endl;
    }
}

// Tham added generate SCAN_REQ

void BLEMacV2::generateScanRequest(){

    num_scan_req++;

    index_T ++;
    EV << "Tham - BLEMacV2::generateScanRequest : index_T = " << index_T <<endl;

   // double prob_c[3]={0.15, 0.05, 0.80};

    // insert scan request data = challenge selection with probability

   // srand (time(NULL)); // initilize the seed

    //==============================
        //generate challenge based on probability distribution
            int arr[] = {1, 2, 3};
            int freq[] = {60,30,10};
            int n = sizeof(arr) / sizeof(arr[0]);

            // Use a different seed value for every run.
            srand(time(NULL));


           //printf("%d\n", myRand(arr, freq, n));



   // RandIndex_c = rand() % 3;
    RandIndex_c = myRand(arr, freq, n) - 1;

    //==================================

    EV << "Challenge index is " << RandIndex_c << endl;
    EV << "BLEMacV2::generateScanRequest: temp_c_index = " << RandIndex_c << endl;
    EV << "BLEMacV2::generateScanRequest: Data selected for SCAN _REQ is: " << prob_c[RandIndex_c] << endl;
    data_req = prob_c[RandIndex_c];
    temp_ch = prob_c[RandIndex_c];
    challenge_list[index_T]=RandIndex_c;

    EV << "Tham - challenge_list[" << index_T <<"]=" <<challenge_list[index_T]<< endl;



    /*
    // Scan Req Data insert = response selection with probability
       cout<<"BLEMacV2::generateScanRequest: enter data of SCAN_REQ: ";
       cin >> data_req;
       cout << "BLEMacV2::generateScanRequestp: Data entered for SCAN _REP is: " << data_req << ".\n";
      */

   // EV << "BLEMacV2::generateScanRequest for " << num_scan_req << " times" << endl;
    delete AdvMessage;

    AdvMessage = new BLE_Adv_MacPkt("SCAN_REQ");
    AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_SCAN_REQ_5);

    AdvMessage->setAdv_type(cBLEstructs_defs::SCAN_REQ);

    AdvMessage->setAccessAddress(0x8E89BED6);//see p 2503 of standard v4.1 0x8E89BED6 to decimal = 2391391958
    // AdvMessage->setAccessAddress(2391391958); // tham 8/7/2018

   // AdvMessage->setInitA(myAdvrtPars.OwnAdvAddr); // no InitAdress in SCAN Req packet

    if(myScanPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
        AdvMessage->setTxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    AdvMessage->setAdvA(myConnPars.ConnInfo.Peer_Address); // set Adv adress: advertiser address

    if(myConnPars.ConnInfo.Peer_Address_Type==cBLEstructs_defs::PEER_ADDR_PUBLIC_0){
        AdvMessage->setRxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    if(debug_Pkt_Basic) EV << "BLEMacV2::myConnPars.ConnInfo.accessAddr=" << myConnPars.ConnInfo.accessAddr << endl;

    int Length = 22;//2(header)+6(InitA)+6(AdvA) + 8(data_req)  if data for scan request, modify the packet

    AdvMessage->setLength(Length);
    AdvMessage->setBitLength(Length*8);
    AdvMessage->setName("SCAN_REQ");//Tham added

   // AdvMessage->setAdvData(data_req); // scan request = challenge information
    //AdvMessage->setAdvData(data_req);
    AdvMessage->setScanReqData(data_req);
    AdvMessage->setTemp_c(RandIndex_c); // trick to select response data

       EV << "Adv_PDU_Type= " << AdvMessage->getAdv_PDU_type() << endl; // Tham added 'PDU' for matching with PDU type defined
       EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
       EV << "AdvA= " << AdvMessage->getAdvA() << endl;
       EV << "InitA= " << AdvMessage->getInitA() << endl;
       EV << "TxAdd= " << AdvMessage->getTxAdd() << endl;
       EV << "RxAdd= " << AdvMessage->getRxAdd() << endl;
       EV << "Length(bytes)= " << AdvMessage->getLength() << endl;
       EV << "ScanReq Data = " << AdvMessage->getScanReqData() << endl;

}


// Tham Added : Generate Scan Rsp packet
void BLEMacV2::generateScan_Rsp(int temp_c){

       num_scan_rsp++;

       //double prob_r_on_c[4]={0.33, 0.34, 0.23, 0.1};

        // insert scan request data = challenge selection with probability

       // srand (time(NULL)); // initilize the seed


    /*
    // Scan Rsp Data insert = response selection with probability
       cout<<"BLEMacV2::generateScan_Rsp: enter data of SCAN_RSP: ";
       cin >> data_rsp;
       cout << "BLEMacV2::generateScan_Rsp: Data entered for SCAN _RSP is: " << data_rsp << ".\n";
       */

    delete AdvMessage;
    AdvMessage = new BLE_Adv_MacPkt("SCAN_RSP");
    AdvMessage->setAdv_PDU_type(cBLEstructs_defs::PDU_SCAN_RSP_6);
    AdvMessage->setAdv_type(cBLEstructs_defs::SCAN_RSP);

    AdvMessage->setAccessAddress(0x8E89BED6);//see p 2503 of standard v4.1 0x8E89BED6 to decimal = 2391391958
    // AdvMessage->setAccessAddress(2391391958); // tham 8/7/2018
    //AdvMessage->setInitA(myAdvrtPars.OwnAdvAddr); // There is no InitA in SCAN_RSP

    if(myScanPars.Own_Address_Type==cBLEstructs_defs::OWN_ADDR_PUBLIC_0){
        AdvMessage->setTxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    AdvMessage->setAdvA(myConnPars.ConnInfo.Peer_Address);
   // AdvMessage->setAdvA(00-00-00-00-00-01);

    if(myConnPars.ConnInfo.Peer_Address_Type==cBLEstructs_defs::PEER_ADDR_PUBLIC_0){
        AdvMessage->setRxAdd(false);
    }
    else{
        error("BLEMacV2::updateAdvertismentPkt - random addresses are NOT supported YET!");
    }

    if(debug_Pkt_Basic) EV << "BLEMacV2::myConnPars.ConnInfo.accessAddr=" << myConnPars.ConnInfo.accessAddr << endl;

    int Length = 16;//2(header)+ 6(AdvA) + 8(Data)  :data for scan response is double type

    AdvMessage->setLength(Length);
    AdvMessage->setBitLength(Length*8);
    AdvMessage->setName("SCAN_RSP");//Tham added

    //==============================
       //generate challenge based on probability distribution
                int arr[] = {1, 2, 3, 4};
                int freq_rc1[] = {51, 34, 2, 13};
                int freq_rc2[] = {50, 36, 1, 13};
                int freq_rc3[] = {50, 31, 6, 13};

               // srand(time(NULL));

                int n = sizeof(arr) / sizeof(arr[0]);

                if (temp_c == 0){
                    RandIndex_r = myRand(arr, freq_rc1, n) - 1;

               } else if (temp_c == 1){
                   RandIndex_r = myRand(arr, freq_rc2, n) - 1;

                      } else {
                          RandIndex_r = myRand(arr, freq_rc3, n) - 1;
                 }



    //==================================

   //EV << "temp_c for challenge = " << temp_c << endl;
  // RandIndex_r = rand() % 4;

   EV << "temp_c = " << temp_c << endl;
   EV << "Rand Index for response = " << RandIndex_r << endl;
   EV << "BLEMacV2::generateScanResponse: Selected response is at [" << temp_c <<"][" << RandIndex_r <<"]" << endl;
   EV << "BLEMacV2::generateScanResponse: Data selected for SCAN _RSP is: " << prob_r_on_c[temp_c][RandIndex_r] << endl;
   data_rsp = prob_r_on_c[temp_c][RandIndex_r]; //prob_r_on_c[RandIndex];

   AdvMessage->setScanRspData(data_rsp); // scan data for response information
   AdvMessage->setIndex_r(RandIndex_r);

   EV << "Adv_PDU_Type=" << AdvMessage->getAdv_PDU_type() << endl; // Tham added 'PDU' for matching with PDU type defined
   EV << "Adv_paket_name = " << AdvMessage->getName() << endl; // tham added
   EV << "AdvA=" << AdvMessage->getAdvA() << endl;
   EV << "InitA=" << AdvMessage->getInitA() << endl;
   EV << "TxAdd=" << AdvMessage->getTxAdd() << endl;
   EV << "RxAdd=" << AdvMessage->getRxAdd() << endl;
   EV << "Length(bytes)=" << AdvMessage->getLength() << endl;
   EV << "ScanResponse Data =" << AdvMessage->getScanRspData() << endl;

}


void BLEMacV2::attachSignal_Adv(BLE_Adv_MacPkt* macPkt, simtime_t_cref startTime){
    simtime_t duration = (macPkt->getBitLength() + phyHeaderLength)/bitrate;
    setDownControlInfo(macPkt, createSignal(startTime, duration, txPower, bitrate));

    if (macPkt->getAdv_PDU_type()==0) num_adv_ind_test_in_attachsignal++;
    if (macPkt->getAdv_PDU_type()==5) num_scan_req_test_in_attachsignal++;
    if (macPkt->getAdv_PDU_type()==6) num_scan_rsp_test_in_attachsignal++;

    if(debug_Pkt_Basic){
    EV<< "BLEMacV2:attachSignal_Adv. SENDING ADV PDU packet. ADV PDU type= " << macPkt->getAdv_PDU_type()<< " Length=" << macPkt->getBitLength() << " bits, Access Address="
        << macPkt->getAccessAddress() << " InitA=" << macPkt->getInitA()
        <<endl;
    }
    // EV <<"BLEMacV2::attachSignal_Adv: phyHeaderLength=" << phyHeaderLength<< " bitrate=" << bitrate << " startTime=" << startTime << " duration=" << duration << " TXpower=" << txPower << "mW"<< endl;
}

/*
long int BLEMacV2::generateAccessAddr(void){
    long int AA=intuniform(0, 65535, 0)+intuniform(0, 65535, 0)*65535;
    EV<< "BLEMacV2::generateAccessAddr - generating random access address. NOTE: we simply use the random address, without checking whether it follows the BLE requirements (see p. 2504 of spec 4.1): AA=" << AA << endl;
    return AA;
}*/

unsigned int BLEMacV2::generateAccessAddr(void){
    unsigned int AA = intuniform(0, 65535, 0) + intuniform(0, 65535, 0)*65535;
   // unsigned int AA = intuniform(0, 65535, 1) + intuniform(0, 65535, 1)*65535; //tham tried 8/7/2018 failed
    EV<< "BLEMacV2::generateAccessAddr - generating random access address. NOTE: we simply use the random address, without checking whether it follows the BLE requirements (see p. 2504 of spec 4.1): AA=" << AA << endl;
    return AA;
}

void BLEMacV2::handleUpperCommand_updateDataChannelMap(cBLEstructs_ChannelMap *NewMap){
    if((macState!=CONNECTION_5)&&((macState!=INITIATING_4)||((macState==INITIATING_4)&&(macSubState==INITIATING_SCAN)))){//we are not using the data channels and thus can freely change the channel table
        if(debug_UpperCommands)
            EV<< "BLEMacV2::updateDataChannelMap" << endl;
        //copy data in a perverted way
        int MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39;
        NewMap->getMap(&MapCh0to7,&MapCh8to15,&MapCh16to23,&MapCh24to31,&MapCh32to39);
        myConnPars.ConnInfo.Data_Channel_Map.fill(MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39);
        myConnPars.ConnInfo.Data_Channel_Map.print();
    }
    else{
        //we need to update the parameters through the LL_CONNECTION_UPDATE_REQ PDU
        //NOTE: this can be done ONLY by the master node!
        if((macState!=INITIATING_4)&&(macSubState!=CONNECTED_MST_SLEEP)&&(macSubState!=CONNECTED_MST_WAITRSP)&&(macSubState==CONNECTED_MST_CONNECTIONEVENT)){
            error("BLEMacV2::updateDataChannelMap - slave CAN NOT USE LL_CONNECTION_UPDATE_REQ PDU!");
        }
        else{
            int MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39;
            NewMap->getMap(&MapCh0to7,&MapCh8to15,&MapCh16to23,&MapCh24to31,&MapCh32to39);
            myConnPars.ConnInfo.new_Data_Channel_Map.fill(MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39);
            if(Flag_UpdatePending_DataChannelMap==true) error("BLEMacV2::updateDataChannelMap - previous update is still pending!");
            Flag_UpdatePending_DataChannelMap=true;
        }
    }
}

void BLEMacV2::handleUpperCommand_updateConnectionPars(cBLEstructs_cConnectionPars *ParsCon){
    if((macState==CONNECTION_5)&&(ParsCon->ConnHandle==myConnPars.ConnInfo.connectionHandle)){//state and handle are correct
        if((macState!=INITIATING_4)&&(macSubState!=CONNECTED_MST_SLEEP)&&(macSubState!=CONNECTED_MST_WAITRSP)&&(macSubState==CONNECTED_MST_CONNECTIONEVENT)){
            error("BLEMacV2::updateDataChannelMap - slave CAN NOT USE LL_CONNECTION_UPDATE_REQ PDU!");
        }
        else{//prepare update
            if(debug_UpperCommands)
                EV<< "BLEMacV2::handleUpperCommand_updateConnectionPars" << endl;
            myConnPars.New_ConnPars.ConnHandle=ParsCon->ConnHandle;
            myConnPars.New_ConnPars.Conn_Interval_Max=ParsCon->Conn_Interval_Max;
            myConnPars.New_ConnPars.Conn_Interval_Min=ParsCon->Conn_Interval_Min;
            myConnPars.New_ConnPars.Conn_Latency=ParsCon->Conn_Latency;
            myConnPars.New_ConnPars.Supervision_Timeout=ParsCon->Supervision_Timeout;
            myConnPars.New_ConnPars.Minimum_CE_Length=ParsCon->Minimum_CE_Length;
            myConnPars.New_ConnPars.Maximum_CE_Length=ParsCon->Maximum_CE_Length;
            if(Flag_UpdatePending_ConnectionPars==true) error("BLEMacV2::handleUpperCommand_updateConnectionPars - previous update is still pending!");
            Flag_UpdatePending_ConnectionPars=true;
        }
    }
}

void BLEMacV2::handleUpperCommand_disconnect(cBLEstructs_Error *ParsError){
    if(macState==CONNECTION_5){
        if(debug_UpperCommands)
            EV<< "BLEMacV2::handleUpperCommand_disconnect" << endl;
        myCmdError->ErrorCode=ParsError->ErrorCode;
        myCmdError->Handle=ParsError->Handle;
        Flag_UpdatePending_TerminateConnection=true;
    }
}


void BLEMacV2::handleUpperCommand_LLBufferPayloadUpdate(long int LLBufferPayloadUpdate){
    if(debug_UpperCommands)
        EV<< "BLEMacV2::handleUpperCommand_LLBufferPayloadUpdate Old Buffer Payload " << TST_DataQueryLgth << "Added:" << LLBufferPayloadUpdate << endl;
    TST_DataQueryLgth=TST_DataQueryLgth+LLBufferPayloadUpdate;
    if(TST_DataQueryLgth<0)TST_DataQueryLgth=0;
    if(debug_UpperCommands)
        EV<< "BLEMacV2::handleUpperCommand_LLBufferPayloadUpdate New Buffer Payload " << TST_DataQueryLgth << endl;
    eventQuerryChange(LLBufferPayloadUpdate);//update querry
    cMessage *m;
    m = new cMessage("BLE_MACtoNWK_EVENT");
    m->setControlInfo(BLE_MacToNwk::generate_BufferPayloadEvent(TST_DataQueryLgth));
    sendControlUp(m);
}


void BLEMacV2::instantUpdateChannelMap(void){
    if(debug_UpperCommands)
        EV<< "BLEMacV2::instantUpdateChannelMap - activating new map of channels!!!! BEEEP!!!" << endl;
    int MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39;
    myConnPars.ConnInfo.new_Data_Channel_Map.getMap(&MapCh0to7,&MapCh8to15,&MapCh16to23,&MapCh24to31,&MapCh32to39);
    myConnPars.ConnInfo.Data_Channel_Map.fill(MapCh0to7,MapCh8to15,MapCh16to23,MapCh24to31,MapCh32to39);
    myConnPars.ConnInfo.Data_Channel_Map.print();
    myConnPars.ConnInfo.Instant_NewMap=-1;
    //according to the spec, we do not need to inform the host
}


void BLEMacV2::instantUpdateConnectionPars(void){
    //activate new parameters
    myConnPars.ConnInfo.connInterval=myConnPars.ConnInfo.new_connInterval;
    myConnPars.ConnInfo.connSlaveLatency=myConnPars.ConnInfo.new_connSlaveLatency;
    myConnPars.ConnInfo.connSupervisionTimeout=myConnPars.ConnInfo.new_connSupervisionTimeout;

    myConnPars.ConnInfo.numMissedEvents=0;
    myConnPars.ConnInfo.connEventCounter=myConnPars.ConnInfo.connEventCounter-1;//compensates for next increase

    if(myEventData->Role==cBLEstructs_defs::MASTER_0){
        //copy data
        myConnPars.ConnPars.ConnHandle=myConnPars.New_ConnPars.ConnHandle;
        myConnPars.ConnPars.Conn_Interval_Max=myConnPars.New_ConnPars.Conn_Interval_Max;
        myConnPars.ConnPars.Conn_Interval_Min=myConnPars.New_ConnPars.Conn_Interval_Min;
        myConnPars.ConnPars.Conn_Latency=myConnPars.New_ConnPars.Conn_Latency;
        myConnPars.ConnPars.Supervision_Timeout=myConnPars.New_ConnPars.Supervision_Timeout;
        myConnPars.ConnPars.Minimum_CE_Length=myConnPars.New_ConnPars.Minimum_CE_Length;
        myConnPars.ConnPars.Maximum_CE_Length=myConnPars.New_ConnPars.Maximum_CE_Length;
        //start required timer
        stopAllTimers();
        updateMacSubstate(CONNECTED_MST_SLEEP);
        startTimer(TIMER_CONNECTION_MASTER_SUPERVISION);
        startTimer(TIMER_CONNECTION_MASTER_WAKEUP_FIRST);
        myConnPars.ConnInfo.timeLastSuccesfullEvent=masterConnectionWakeupTimer->getArrivalTime()-myConnPars.ConnInfo.connInterval*0.00125;
    }
    else if(myEventData->Role==cBLEstructs_defs::SLAVE_1){
        //start required timer
        stopAllTimers();
        updateMacSubstate(CONNECTED_SLV_WAITCONNECTION);
        startTimer(TIMER_CONNECTION_SLAVE_SUPERVISION);
        startTimer(TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION_FIRST);
    }
    myConnPars.ConnInfo.Instant_NewParameters=-1;

    //inform HOST
    myEventData->Status=0x00;//Successful
    myEventData->Connection_Handle=myConnPars.ConnInfo.connectionHandle;
    myEventData->Conn_Interval=myConnPars.ConnInfo.connInterval;
    myEventData->Conn_Latency=myConnPars.ConnInfo.connSlaveLatency;
    myEventData->Supervision_Timeout=myConnPars.ConnInfo.connSupervisionTimeout;
    cMessage *m;
    m = new cMessage("BLE_MACtoNWK_EVENT");
    m->setControlInfo(BLE_MacToNwk::generate_LEConnectionUpdateCompleteEvent(myEventData));
    sendControlUp(m);

    //proposed overlap monitoring mechanisms
    if(TST_OverlapMonitoringMechanism==true){
        if(debug_Internal_Messages)
            ev << "ConnectionMonitoring_EVENT parameters changed";
        ConnectionMonitoring_Init();
    }
}


void BLEMacV2::updateFlowControlOfCurrentDataPkt(void){

}


void BLEMacV2::ConnectionMonitoring_Init(void){
    EventDuration.clear();
}

void BLEMacV2::ConnectionMonitoring_ConnectionClosed(void){
    if(debug_Internal_Messages)
        ev << "ConnectionMonitoring_ConnectionClosed current time" << simTime() << " EventStartTime " << EventStartTimestamp << endl;
    simtime_t Stamp= simTime()-EventStartTimestamp;
    EventDuration.push_back(Stamp);//insert new data
    if(EventDuration.size()>TST_CMM_NumPoints){//drop old data
        EventDuration.pop_front();
    }
    if(EventDuration.size()==TST_CMM_NumPoints){//check the condition for rebuilding the link
        //FAST & SIMPLE SOLUTION
        TimeQueue::iterator it;
        bool result=true;
        const char *str_TST_CMM_DetectionPolicy = par("TST_CMM_DetectionPolicy");
        if (!strcmp(str_TST_CMM_DetectionPolicy, "DropTimeInRangeRelative")){
            //TST_CMM_Par1 - threeshold for starting
            //TST_CMM_Par2 - max deviation between the points
            it = EventDuration.begin();
            simtime_t FirstConnectionBreakTime=*it;
            if(FirstConnectionBreakTime/(myConnPars.ConnInfo.connInterval*0.00125)>TST_CMM_Par1){
                result=false;
            }
            else{
                for(; it != EventDuration.end(); ++it){
                    simtime_t NextConnectionBreakTime=*it;
                    if((NextConnectionBreakTime<((1-TST_CMM_Par2)*FirstConnectionBreakTime))||(NextConnectionBreakTime>((1+TST_CMM_Par2)*FirstConnectionBreakTime))){
                        result=false;
                        break;
                    }
                }
            }
        }
        if(result==true){//change link
            if((Flag_UpdatePending_ConnectionPars!=true)&&(myConnPars.ConnInfo.Instant_NewParameters<0)){
                myConnPars.ConnInfo.transmitWindowOffset=round(myConnPars.ConnInfo.connInterval*TST_CMM_Offset_Const + uniform(0,myConnPars.ConnInfo.connInterval*TST_CMM_Offset_Rand));
                myConnPars.New_ConnPars.ConnHandle= myConnPars.ConnPars.ConnHandle;
                myConnPars.New_ConnPars.Conn_Interval_Max=myConnPars.ConnPars.Conn_Interval_Max;
                myConnPars.New_ConnPars.Conn_Interval_Min=myConnPars.ConnPars.Conn_Interval_Min;
                myConnPars.New_ConnPars.Conn_Latency=myConnPars.ConnPars.Conn_Latency;
                myConnPars.New_ConnPars.Supervision_Timeout=myConnPars.ConnPars.Supervision_Timeout;
                myConnPars.New_ConnPars.Minimum_CE_Length=myConnPars.ConnPars.Minimum_CE_Length;
                myConnPars.New_ConnPars.Maximum_CE_Length=myConnPars.ConnPars.Maximum_CE_Length;

                Flag_UpdatePending_ConnectionPars=true;
                if(debug_Internal_Messages)
                    ev << "ConnectionMonitoring_EVENT parameters change initiated!";
            }
        }
    }
    if(debug_Internal_Messages)
        ev << "ConnectionMonitoring_ConnectionClosed PRINTING EVENT DURATION QUERRY: size " << EventDuration.size() << endl;
    TimeQueue::iterator it;
    int num=0;
    if(debug_Internal_Messages){
        for(it = EventDuration.begin(); it != EventDuration.end(); ++it){
            num++;
            ev << "Enty# " << num << " value" << *it << endl;
        }
    }
}

void BLEMacV2::ConnectionMonitoring_ConnectionSuccess(void){
    if(debug_Internal_Messages)
        ev << "ConnectionMonitoring_ConnectionSuccess" << endl;
    EventDuration.clear();
}
