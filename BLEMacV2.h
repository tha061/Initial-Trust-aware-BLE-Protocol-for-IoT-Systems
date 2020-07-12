/* -*- mode:c++ -*- ********************************************************
 // * file:        BLEMacV2.h
 // *
 // * Created on:  08.3.2014
 // * Updated on:  08.5.2014
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
 // * OPEN ISSUES:
 // * 1) Transmit power setting
 // **************************************************************************/


#ifndef BLEMacV2_H
#define BLEMacV2_H


#include <string>
#include <sstream>
#include <vector>
#include <list>

#include "MiXiMDefs.h"
#include "BaseMacLayer.h"
#include "DroppedPacket.h"

#include "BLE_Adv_MacPkt_m.h"
#include "BLE_Data_MacPkt_m.h"

#include "BLEstructs.h"
#include "BLEdefs.h"
#include "BLECmds_HCItoCtrl.h"
#include "BLE_NwkToMac.h"


//FOR LOGS
#include "BLETestNetworkStatistics.h"

// For randomly choose challenge and response
#include <stdio.h>
#include <stdlib.h>

class BLE_Adv_MacPkt;
class BLE_Data_MacPkt;
class BLEstructs;
class cBLEstructs_cAdvertisePars;
class cBLEstructs_cConnectionPars;


class MIXIM_API BLEMacV2 : public BaseMacLayer
{
  public:
    BLEMacV2() : BaseMacLayer()
    //Timers
        , advertisementEventTimer(NULL)
        , advertisementNextPDUTimer(NULL)
        , advertisementEndEvent(NULL)
        , initiatingScanIntervalTimer(NULL)
        , initiatingScanWindowTimer(NULL)
        , slaveEndConnectionEvent(NULL)
        , slaveConnectionSupervisionTimer(NULL)
        , slaveConnectionWakeupTimer(NULL)
        , slaveConnectionNoBeaconTimer(NULL)
        , slaveIFSTimer(NULL)
        , slaveWaitReplyTimer(NULL)
        , masterConnectionWakeupTimer(NULL)
        , masterIFSTimer(NULL)
        , masterWaitReplyTimer(NULL)
        , masterEndConnectionEvent(NULL)
        , masterConnectionSupervisionTimer(NULL)
        , ctrl_switchState(NULL)
        , ctrl_terminateConnection(NULL)
        //, TST_switchoffTimer(NULL)
    //Variables
        , myAdvrtPars()
        , myScanPars()
        , myConnPars()
        , myEventData()
        , myCmdError()
        , llDataHeaderLengthBits(16)
        , llmaxDataPDUPayloadBytes()
        , llminDataPDUduration()
        , EventDuration()

    {} // constructor

    //TYPEDEFS
    enum t_mac_DEBUG_PRINTS {
        myAdvPars_1
    };

    // Tham added for statistic
  public:
    int num_adv_ind;
    int num_scan_req;
    int num_scan_rsp;
    int num_scan_req_test;
    int num_scan_rsp_test;
    int num_scan_req_test_in_attachsignal;
    int num_scan_rsp_test_in_attachsignal;
    int num_adv_ind_test_in_attachsignal;


    int num_scan_req_rcved;
    int num_scan_rsp_rcved;
    int num_adv_ind_rcved;

    int num_data_pkt_sent;
    int num_data_pkt_rcved;

    int end_sim_packet;

    double T_init[10]={0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    double T_inst[10]={0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    //double temp_T;
    int index_T;
    int temp_c_index;
    int RandIndex_c;
    int RandIndex_r;

    double data_req;
    double data_rsp;

    // Device Database
    //LAddress::L2Type Address_device[3]={00-00-00-00-00-00, 00-00-00-00-00-01, 00-00-00-00-00-02};
   LAddress::L2Type Address_device[3];
    double Trust_device[3] ={0, 0, 0};


    int input_rsp;
    int input_data_round;
    int min_data_rounds;

    int challenge_list[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    int response_list[20]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    // Challenge-Response in DATA packet
    int ChallengeIndex_data;
    int ResponseIndex_data;
    int IndexCh_data;
    double challenge_data;

    // for challenge-respone
    double prob_c[3]= {0.6, 0.3, 0.1};
    double prob_r_on_c[3][4]= { {0.51, 0.34, 0.02, 0.13}, {0.50, 0.36, 0.01, 0.13}, {0.50, 0.31, 0.06, 0.13} };

    //double prob_c[3]= {0.05, 0.15, 0.80};
   // double prob_r_on_c[3][4]= { {0.66, 0.10, 0.10, 0.14}, {0.12, 0.62, 0.12, 0.14}, {0.10, 0.18, 0.58, 0.04} };

    int challenge[3] = {1, 2, 3};
    int response[4] = {1, 2, 3, 4};

    double temp_ch;
    double temp_res;
    simtime_t t_begin;
    simtime_t t_end;
    simtime_t t_end_dis;
   // simtime_t t_cost_dis;
   // simtime_t t_cost;
    simtime_t t_connected;

    //FUNCTIONS
	virtual ~BLEMacV2(); //destructor

    // @brief Initialization of the module and some variables
    virtual void initialize(int);


    // @brief Delete all dynamically allocated objects of the module
    virtual void finish();

    // @brief Handle messages from lower layer
    virtual void handleLowerMsg(cMessage*);

    // @brief Handle messages from upper layer
    virtual void handleUpperMsg(cMessage*);

    // @brief Handle self messages such as timers
    virtual void handleSelfMsg(cMessage*);

    // @brief Handle control messages from lower layer
    virtual void handleLowerControl(cMessage *msg);

    // @brief Handle control messages from upper layer
    virtual void handleUpperControl(cMessage *msg);

    bool debug_Internal_Messages;

  protected:
    typedef std::list<cPacket*> MacQueue;
    typedef std::list<simtime_t> TimeQueue;
    /** @brief The power (in mW) to transmit with.*/
    double txPower;



     /** @brief the bit rate at which we transmit */
    double bitrate;
    int CurrentAdvChannel;
    int llDataHeaderLengthBits;
    int llAdvHeaderLengthBits;

    simtime_t llIFSDeviation;
    simtime_t llIFS;
    simtime_t llmaxAdvPDUduration;
    simtime_t llmaxDataPDUduration;
    simtime_t llminDataPDUduration;
    int llmaxDataPDUPayloadBytes;
    int llSlave_beacon_ReplyPolicy;

    simtime_t Time_llSLEEPtoTX;
    simtime_t Time_llSLEEPtoRX;
    simtime_t Time_llTXtoRX;
    simtime_t Time_llRXtoTX;
    simtime_t Time_llTXtoSLEEP;
    simtime_t Time_llRXtoSLEEP;

    //"data buffer"
    long int Buf_TotalSize;
    long int Buf_FreeSize;
    long int PktLeft_TX_Size;
    long int PktLeft_RX_Size;

    bool DataPkt_FirstFragment;
    bool DataPkt_FirstFragmentSent;
    bool Flag_DataPktPending;
    int LastPacketPayload;
    MacQueue macQueue;

    long int DEBUG_PKT_SEQ_ID;
    //int ConnectionTermination;

protected:

//BASIC STUFF
    //TYPEDEFS
    enum t_mac_states{//see p.2498 of Spec. v4.1
        STANDBY_1=1
        ,ADVERTISING_2
        ,SCANNING_3
        ,INITIATING_4
        ,CONNECTION_5
    };

    enum t_mac_substates{
        UNUSED=0x40

        ,INITIATING_SCAN=0x30
        ,INITIATING_TXD
        //connected as slave
        ,CONNECTED_SLV_MASK=0x50
        ,CONNECTED_SLV_SLEEP=0x51
        ,CONNECTED_SLV_WAITCONNECTION
        ,CONNECTED_SLV_WAITPKT
        //,CONNECTED_SLV_WAITBCN
        ,CONNECTED_SLV_CONNECTIONEVENT
        //connected as master
        ,CONNECTED_MST_MASK=0x60
        ,CONNECTED_MST_SLEEP=0x61
        ,CONNECTED_MST_WAITRSP
        ,CONNECTED_MST_CONNECTIONEVENT
        ,MASK=0x40
    };

    enum t_mac_event {
        EV_ADV_EVENT=0
        ,EV_ADV_NEXTPDU
        ,EV_ADV_EVENTEND

        ,EV_INIT_INTERVAL
        ,EV_INIT_WINDOW

        ,EV_CON_SLV_WAKEUP
        ,EV_CON_SLV_WAKEUP_CONNECTION
        ,EV_CON_SLV_TRANSMIT_WINDOW
        ,EV_CON_SLV_NOBEACON
        ,EV_CON_SLV_WAITIFS
        ,EV_CON_SLV_WAITREPLY
        ,EV_CON_SLV_ENDEVENT
        ,EV_CON_SLV_DROPCONNECTION

        ,EV_CON_MST_WAKEUP
        ,EV_CON_MST_WAITREPLY
        ,EV_CON_MST_WAITIFS
        ,EV_CON_MST_ENDEVENT
        ,EV_CON_MST_DROPCONNECTION

        ,EV_FRAME_RECEIVED
        ,EV_FRAME_TRANSMITTED

        ,EV_SWITCH_STATE

        ,EV_TST_SWITCHOFF
    };
    //Kinds for timer messages
    enum t_mac_timer {
      TIMER_ADVERTISE_EVENTSTART=0,
      TIMER_ADVERTISE_NEXTPDU,
      TIMER_ADVERTISE_EVENTEND,

      TIMER_INITIATING_INTERVAL,
      TIMER_INITIATING_WINDOW,

      TIMER_CONNECTION_SLAVE_WAKEUP,
      TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION,
      TIMER_CONNECTION_SLAVE_WAKEUP_CONNECTION_FIRST,
      TIMER_CONNECTION_SLAVE_TRANSMIT_WINDOW,
      TIMER_CONNECTION_SLAVE_NOBCN,
      TIMER_CONNECTION_SLAVE_IFS,
      TIMER_CONNECTION_SLAVE_REPLY,
      TIMER_CONNECTION_SLAVE_SUPERVISION,
      TIMER_CONNECTION_SLAVE_SUPERVISION_FIRST,

      TIMER_CONNECTION_MASTER_WAKEUP,
      TIMER_CONNECTION_MASTER_WAKEUP_FIRST,
      TIMER_CONNECTION_MASTER_WAITREPLY,
      TIMER_CONNECTION_MASTER_IFS,
      TIMER_CONNECTION_MASTER_SUPERVISION,
      TIMER_CONNECTION_MASTER_SUPERVISION_FIRST
      //TIMER_CONNECTION_MASTER_REPLY
    };



    enum t_mac_slave_beacon_ReplyPolicy{//what a slave should do if it has received a beacon from master with
                                  //no MD set and it does not have any data of its own?
        ALWAYS_1=1
        ,IF_HAVE_DATA_2
    };


    enum t_mac_switch_states{
        INITIALIZING_to_CONNECTION=0
        ,ADVERTIZING_to_CONNECTION
        //,CONNECTION_to_INITIALIZING
        //,CONNECTION_to_ADVERTIZING

        ,STANDBY_to_ADVERTIZING
        ,STANDBY_to_SCANNING
        ,STANDBY_to_INITIATING

        //,ADVERTIZING_to_STANDBY
        //,INITIATING_to_STANDBY
        //,CONNECTION_to_STANDBY
        ,ANY_to_STANDBY
    };

    enum t_termination{//see p.2498 of Spec. v4.1
        NONE_0=1
        ,INIT_BY_ME_1
        ,INIT_BY_LINK_2
        ,INIT_BY_PEER_2
        ,WAIT_ACK_3
        ,ACK_ISSUED_4
    };


    //VARS
    // @name Pointer for timer messages.
    cMessage * advertisementEventTimer;//for advertisement Events
    cMessage * advertisementNextPDUTimer;//for different PDUs (i.e., switching the channel)
    cMessage * advertisementEndEvent;//end event

    cMessage * initiatingScanIntervalTimer;//for start of scan intervals
    cMessage * initiatingScanWindowTimer;//for start of scan intervals



    cMessage * slaveConnectionWakeupTimer;//for connection events (by slave)- see p. 2543 of spec. v4.1
    cMessage * slaveConnectionWakeupConnectionTimer;//for start of receive during connection setup (by slave)- see p. 2541 of spec. v4.1
    cMessage * slaveConnectionTransmitWindowTimer;//end of transmit window (by slave)- see p. 2541 of spec. v4.1


    cMessage * slaveConnectionNoBeaconTimer;//for connection end event (no beacon)
    cMessage * slaveIFSTimer;//wait IFS before sending reply
    cMessage * slaveWaitReplyTimer;//wait reply from master
    cMessage * slaveEndConnectionEvent;//end this connection event
    cMessage * slaveConnectionSupervisionTimer;//connection should be dropped

    cMessage * masterConnectionWakeupTimer;//for connection events (by master)
    //cMessage * masterWaitBCNReplyTimer;//for master waiting a reply (152 us - see p 2524 of spec. v4.1)
    cMessage * masterIFSTimer;//wait IFS before sending reply
    cMessage * masterWaitReplyTimer;//wait reply from slave
    cMessage * masterEndConnectionEvent;//end this connection event
    cMessage * masterConnectionSupervisionTimer;//connection should be dropped

    cMessage * ctrl_switchState;//switch to a new state
    //cMessage * ctrl_forceSleep;//connection should be dropped

    cMessage * ctrl_terminateConnection;//connection should be dropped

    //cMessage * TST_switchoffTimer;//connection should be dropped

    t_mac_states macState;
    t_mac_substates macSubState;

    BLE_Adv_MacPkt* AdvMessage;
    //BLE_Adv_MacPkt* ConRQSTMessage;
    BLE_Data_MacPkt* DataPkt;
    BLE_Data_MacPkt* HighPriorityDataPkt;//
    cPacket * DataPktForNWK;
    //FUNCTIONS
    virtual void attachSignal_Data(BLE_Data_MacPkt* macPkt, simtime_t_cref startTime);
    virtual void attachSignal_Adv(BLE_Adv_MacPkt* macPkt, simtime_t_cref startTime);
    virtual cPacket *decapsAdvMsg(BLE_Adv_MacPkt * macPkt);

//BLE STUFF
    //TYPEDEFS


    typedef struct tActiveConnectionPars{//different pars of the protocol, which can not be changed by the upper layers

        cBLEstructs_defs::tConnectionHandle ConnID;
        cBLEstructs_cConnectionPars ConnPars;
        cBLEstructs_cConnectionPars New_ConnPars;
        cBLEstructs_cConnectionInfo ConnInfo;
        tActiveConnectionPars():ConnID(0),ConnPars(),New_ConnPars(),ConnInfo(){};
       } tAdvertiseConstPars;



    cBLEstructs_cAdvertisePars myAdvrtPars;
    cBLEstructs_cScanningPars myScanPars;
    cBLEstructs_cEventsData *myEventData;
    cBLEstructs_Error *myCmdError;

    tActiveConnectionPars myConnPars; // struct variable
    //cBLEstructs_cConnectionPars myCnctPars;

    int mySCA;


protected:
    simtime_t  llTST_switchoffTime;
    long int TST_DataQueryLgth;
    bool TST_stopSimulation_SlaveNoData;
    bool TST_stopSimulation_MasterNoData;
    bool TST_forceSleepOnSendComplete;

    //FUNCTIONS
    virtual bool startTimer(t_mac_timer timer);

    virtual simtime_t calculateAdvEvent(long int Advertising_Interval_Min, long int Advertising_Interval_Max, int AdvType);//calculates advertisement interval using the data about Advertising_Interval_Min and Advertising_Interval_Max (mechanism NOT DEFINED by the standard (v4.1))
    virtual simtime_t calculateFirstConnectionEventTime_Master(void);
    virtual simtime_t calculateTransmitWindowStart_Slave(void);
    virtual simtime_t calculateNextDataEventTime(void);

    virtual void executeMac(t_mac_event event, cMessage *msg) ;
    virtual void updateMacState(t_mac_states newMacState);
    virtual void updateMacSubstate(t_mac_substates newMacSubstate);
    virtual void updateStatusStandby(t_mac_event event, cMessage *msg);
    virtual void updateStatusAdvertising(t_mac_event event, cMessage *msg);
    virtual void updateStatusInitiating(t_mac_event event, cMessage *msg);
    virtual void updateStatusConnected(t_mac_event event, cMessage *msg);

    virtual void dropConnection(void);


    virtual bool prepareNewDataPkt(simtime_t RemainingTime);
    virtual bool prepareNewDataPkt_Slave(simtime_t RemainingTime, int challenge_index);
    virtual bool prepareNewDataPkt_Master(simtime_t RemainingTime);
    //Data channel commands
    virtual bool prepareConnTerminatePkt(simtime_t RemainingTime);
    virtual bool prepareChannelMapUpdatePkt(simtime_t RemainingTime);
    virtual bool prepareConnectionUpdatePkt(simtime_t RemainingTime);
    virtual bool prepareEmptyACKPkt(simtime_t RemainingTime);

    virtual void updateAdvertismentPkt(void);
    virtual void generateConnectionRequest(void);
    virtual void updateFlowControlOfCurrentDataPkt(void);//should be launched after finishing the transmission of high-priority packets - used to update the NESN of the packet


    virtual bool checkCurrentDataPkt(simtime_t RemainingTime);
    virtual bool checkHighPriorityDataPkt(simtime_t RemainingTime);
    virtual bool checkCurrentAdvPkt(simtime_t RemainingTime);




    //int GetNextAdvChannel(int StartChannel);
    //virtual void initChannelMap(cBLEstructs_ChannelMap *Map, int StateCh0_7, int StateCh8_15, int StateCh16_23, int StateCh24_31, int StateCh32_39);
    virtual void initVariablesAtStateStart(void);
    virtual void stopSimulation(void);
    virtual void stopAllTimers(void);
    virtual bool checkBuffer(int AcknowledgedReception);

   // virtual long int generateAccessAddr(void);
    virtual unsigned int generateAccessAddr(void);


    virtual void handleUpperCommand_Adv(BLE_NwkToMac *const cCmdInfo);
    virtual void handleUpperCommand_Initiate(BLE_NwkToMac *const cCmdInfo);
    virtual void handleUpperCommand_updateDataChannelMap(cBLEstructs_ChannelMap *NewMap);
    virtual void handleUpperCommand_updateConnectionPars(cBLEstructs_cConnectionPars *ParsCon);
    virtual void handleUpperCommand_disconnect(cBLEstructs_Error *ParsError);
    virtual void handleUpperCommand_LLBufferPayloadUpdate(long int LLBufferPayloadUpdate);

    virtual void sendEventNotificationToNwk(cBLEstructs_defs::BLECmdTypes_HCItoCtrl EventType);

    virtual void instantUpdateChannelMap(void);
    virtual void instantUpdateConnectionPars(void);

    virtual void eventRadioChannelChanged(void);
    virtual void eventRadioStateChanged(void);
    virtual void eventQuerryChange(int Change);
    virtual void eventDataNewRXd(int DataLgth);
    virtual void eventDataReTXRXd(int DataLgth);
    virtual void eventConnectionCompleted(void);
    virtual void eventConnectionDroped(void);
    virtual void eventNoRSPTimer(void);



    virtual void ConnectionMonitoring_Init(void);
    virtual void ConnectionMonitoring_ConnectionClosed(void);
    virtual void ConnectionMonitoring_ConnectionSuccess(void);

private:
  	/** @brief Copy constructor is not allowed.
  	 */
	BLEMacV2(const BLEMacV2&); //copy constructor
  	/** @brief Assignment operator is not allowed.
  	 */
	BLEMacV2 & operator=(const BLEMacV2&);

	//DEBUG STUFF
	bool debug_Timers;
	bool debug_Messages;
	bool debug_StateTransitions;
	bool debug_Pkt_Basic;
	//bool debug_Internal_Messages;
	bool debug_ChannelSwitching;
	bool debug_InitPars;
	bool debug_UpperCommands;

	bool info_events;

	bool Flag_UpdatePending_DataChannelMap;
	bool Flag_UpdatePending_ConnectionPars;
	bool Flag_UpdatePending_TerminateConnection;
	bool Flag_PeerTerminatesConnection;
	bool Flag_HighPriorityTraffic;

	bool Flag_DelayedSwitchOff;

	//BLETestNetworkStatistics* BLEnwkStats;

	bool TST_forceSleep_MasterNoData;
	bool TST_forceSleep_SlaveNoData;
	bool TST_NoAdvIntRandomComponent;
	int  TST_forceHop;

    //Overlap Monitoring Mechanism
    bool TST_OverlapMonitoringMechanism;
    unsigned int TST_CMM_NumPoints;
    double TST_CMM_Par1;
    double TST_CMM_Par2;
    double TST_CMM_Offset_Const;
    double TST_CMM_Offset_Rand;

    TimeQueue EventDuration;
    simtime_t EventStartTimestamp;

public:
    //virtual void generateScanRequest(void); // Tham added for SCAN_REQ packet
    //virtual void generateScanRequest(int *index_my);// added by Stephen
    virtual void generateScanRequest();
    virtual void generateScan_Rsp(int temp_c); // Tham added for SCAN_RSP packet

//private:
   //simsignal_t arrivalSignal_TH; // Tham added 26/02/2018

};

#endif

