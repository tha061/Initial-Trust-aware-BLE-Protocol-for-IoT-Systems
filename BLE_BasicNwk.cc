/* -*- mode:c++ -*- ********************************************************
 * file:        BLE_BasicNwk.c
 *
 * Created on:  15.02.2014
 * Updated on:  02.08.2014
 * author:      Konstantin Mikhaylov
 *
 * copyright:   (C) 2014 CWC, University of Oulu, Finland & Konstantin Mikhaylov
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *
 *              This program is distributed in the hope that it will be useful,
 *              but WITHOUT ANY WARRANTY; without even the implied warranty of
 *              MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *              GNU General Public License for more details.
 *
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * Nwk level for basic BLE host
 **************************************************************************/

#include "BLE_BasicNwk.h"
#include "BLEstructs.h"
#include "BLE_NwkToMac.h"
#include "BLE_MacToNwk.h"

#include <limits>
#include <algorithm>
#include <cassert>

#include "NetwControlInfo.h"
//#include "ModularNetwCmds.h"
//#include "ModularAppCmds.h"
#include "BaseMacLayer.h"
#include "MacToNetwControlInfo.h"
#include "ArpInterface.h"
#include "FindModule.h"

//#include "Modular_NwkPkt_m.h"


#include "SimTracer.h"
#include "ChannelAccess.h"
#include "MacToPhyInterface.h"
#include "FWMath.h"
#include "BaseNwkLayer_TXPwrCtrl.h"
//#include "NwkMsgFIFO.h"

#include "BLE_HCI_DataPkt_m.h"
#include "BLE_LLControlInfo.h"
//#include "ModuleDefs.h"
#include "BLETestNetworkStatistics.h"

class BLEstructs;

using std::make_pair;
Define_Module(BLE_BasicNwk);

//original
/*
void BLE_BasicNwk::initialize(int stage){
    ev << "Tham - BLE_BasicNwk::initialize " << endl;
    cModule*    host;
    BaseNwkLayer_TXPwrCtrl::initialize(stage);
    EV << "Node: " << this->getParentModule()->getFullName() <<" Initializing  Nwk Layer: BLE_BasicNwk"<< endl;
    if(stage == 1) {

        BLEnwkStats=FindModule<BLETestNetworkStatistics*>::findGlobalModule();
        if(!BLEnwkStats){
            error("BLEMacV2::initialize - BLEnwkStats not found!");
        }

        TESTTimer = new cMessage("timer-TEST");
        StartNode = new cMessage("timer-StartNode");
        GenerateNewData = new cMessage("timer-GenerateNewData");

        stats = par("stats");
        trace = par("trace");
        debug = par("debug");
        useSimTracer = par("useSimTracer");

        //create parameter tables
        currentCmdAdvertisePars=new cBLEstructs_cAdvertisePars();
        currentCmdConnectionPars=new cBLEstructs_cConnectionPars();
        currentCmdScanningPars=new cBLEstructs_cScanningPars();
        currentCmdError=new cBLEstructs_Error();

        //Init parameters
        currentCmdAdvertisePars->Advertising_Channel_Map.fill((int)(par("connAdvertiseChannelMap_0to7")), (int)(par("connAdvertiseChannelMap_8to15")), (int)(par("connAdvertiseChannelMap_16to23")), (int)(par("connAdvertiseChannelMap_24to31")), (int)(par("connAdvertiseChannelMap_32to39")));
        if(currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber<1)error("BLE_BasicNwk::initialize - no advertising channels awailable - please update the channel map!");

        //fill in the data channel map
        currentCmdConnectionPars->Data_Channel_Map.fill((int)(par("connDataChannelMap_0to7")), (int)(par("connDataChannelMap_8to15")), (int)(par("connDataChannelMap_16to23")), (int)(par("connDataChannelMap_24to31")), (int)(par("connDataChannelMap_32to39")));
        if(currentCmdConnectionPars->Data_Channel_Map.TotalChNumber<2)error("BLE_BasicNwk::initialize - according to the standard, we should have at least 2 different data channels - please update the channel map!");

        int Role=par("NWK_DEBUG_ROLE");
        initData=par("initialData");

        if(Role==1){//slave
            ev << "Tham - BLE_BasicNwk::initialize - This is a slave" << endl;
            scheduleAt(simTime()+par("nodeStartupDelay"), StartNode);//start all
            scheduleAt(simTime(), GenerateNewData);//generate more data
        }
        else if(Role==2){//master
            ev << "Tham BLE_BasicNwk::initialize - This is a master" << endl;
            scheduleAt(simTime()+par("nodeStartupDelay"), StartNode);//start all
            scheduleAt(simTime(), GenerateNewData);//generate more data
            / if(initData!=0){
                scheduleAt(simTime(), GenerateNewData);//generate more data
              }
            /
        }
      }
}
*/

// Tham modified
void BLE_BasicNwk::initialize(int stage){
    ev << "Tham - BLE_BasicNwk::initialize " << endl;
    cModule*    host;
    BaseNwkLayer_TXPwrCtrl::initialize(stage);
    EV << "Node: " << this->getParentModule()->getFullName() <<" Initializing  Nwk Layer: BLE_BasicNwk"<< endl;
    if(stage == 1) {

        BLEnwkStats=FindModule<BLETestNetworkStatistics*>::findGlobalModule();
        if(!BLEnwkStats){
            error("BLEMacV2::initialize - BLEnwkStats not found!");
        }

        TESTTimer = new cMessage("timer-TEST");
        StartNode = new cMessage("timer-StartNode");
        GenerateNewData = new cMessage("timer-GenerateNewData");

        stats = par("stats");
        trace = par("trace");
        debug = par("debug");
        useSimTracer = par("useSimTracer");

        //create parameter tables
        currentCmdAdvertisePars=new cBLEstructs_cAdvertisePars();
        currentCmdConnectionPars=new cBLEstructs_cConnectionPars();
        currentCmdScanningPars=new cBLEstructs_cScanningPars();
        currentCmdError=new cBLEstructs_Error();

        //Init parameters
        currentCmdAdvertisePars->Advertising_Channel_Map.fill((int)(par("connAdvertiseChannelMap_0to7")), (int)(par("connAdvertiseChannelMap_8to15")), (int)(par("connAdvertiseChannelMap_16to23")), (int)(par("connAdvertiseChannelMap_24to31")), (int)(par("connAdvertiseChannelMap_32to39")));
        if(currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber<1)error("BLE_BasicNwk::initialize - no advertising channels awailable - please update the channel map!");

        //fill in the data channel map
        currentCmdConnectionPars->Data_Channel_Map.fill((int)(par("connDataChannelMap_0to7")), (int)(par("connDataChannelMap_8to15")), (int)(par("connDataChannelMap_16to23")), (int)(par("connDataChannelMap_24to31")), (int)(par("connDataChannelMap_32to39")));
        if(currentCmdConnectionPars->Data_Channel_Map.TotalChNumber<2)error("BLE_BasicNwk::initialize - according to the standard, we should have at least 2 different data channels - please update the channel map!");

        int Role=par("NWK_DEBUG_ROLE");
        initData=par("initialData");

        if(Role==1){//slave
            ev << "Tham - BLE_BasicNwk::initialize - This is a slave" << endl;
            scheduleAt(simTime()+par("nodeStartupDelay"), StartNode);//start all
          //  scheduleAt(simTime(), GenerateNewData);//generate more data - Tham
        }
        else if(Role==2){//master
            ev << "Tham BLE_BasicNwk::initialize - This is a master" << endl;
            scheduleAt(simTime()+par("nodeStartupDelay"), StartNode);//start all
           // scheduleAt(simTime(), GenerateNewData);//generate more data - Tham


            /*if(initData!=0){
                scheduleAt(simTime(), GenerateNewData);//generate more data
            }*/
        }
      }
}


BLE_BasicNwk::~BLE_BasicNwk(){

}

void BLE_BasicNwk::finish(){
    cancelAndDelete(TESTTimer);
    cancelAndDelete(StartNode);
    cancelAndDelete(GenerateNewData);
}

//original
/*
void BLE_BasicNwk::handleSelfMsg(cMessage* msg){

    EV << "Tham test msg = " << msg << endl;
    if(msg==TESTTimer){
         /* if(currentCmdAdvertisePars->AdvEnabled==true){
                currentCmdAdvertisePars->AdvEnabled=false;
             }
             else{
                currentCmdAdvertisePars->AdvEnabled=true;

                currentCmdAdvertisePars->Advertising_Interval_Max=currentCmdAdvertisePars->Advertising_Interval_Max+160;
                currentCmdAdvertisePars->Advertising_Interval_Min=currentCmdAdvertisePars->Advertising_Interval_Min+160;
                cMessage *m1;
                m1 = new cMessage("BLE_NWKtoMAC_CMD");
                m1->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
                sendControlDown(m1);

            }
            cMessage *m;
            m = new cMessage("BLE_NWKtoMAC_CMD");
            m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertiseEnableCommand(currentCmdAdvertisePars));
            sendControlDown(m);
            scheduleAt(simTime()+1, TESTTimer);
        */

        /*      cMessage *m;
                m = new cMessage("BLE_NWKtoMAC_CMD");
                m->setControlInfo(BLE_NwkToMac::generate_LECreateConnectionCancelCommand());
                sendControlDown(m);
       /

        //disconnect
        currentCmdError->Handle=currentCmdConnectionPars->ConnHandle;
        currentCmdError->ErrorCode=cBLEstructs_Error::REMOTE_USER_TERMINATED_CONNECTION;
        cMessage *m;
        m = new cMessage("BLE_NWKtoMAC_CMD");
        m->setControlInfo(BLE_NwkToMac::generate_DisconnectCommand(currentCmdError));
        sendControlDown(m);
    }


    if(msg==StartNode){//start all
        startNode();
    }

    if(msg==GenerateNewData){//"add" some data to LL buffer
        //ev<< "BLE_BasicNwk::handleSelfMsg on GenerateNewData" << endl;
        //generate a data frame and handle it to MAC
        if(initData){
            cPacket *TestDataMsg;
            TestDataMsg = new cPacket("This is a test data message");
            TestDataMsg->setByteLength(initData);
            BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
            sendDown(TestDataMsg);
            BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());
        }
/*
        TestDataMsg = new cPacket("This is a test data message 2");
        TestDataMsg->setByteLength(2000);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg);
        BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());
        */
        /*TestDataMsg = new cPacket("This is a test data message 2");
        TestDataMsg->setByteLength(555);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg);
        BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());*/
        //generate a data frame and handle it to MAC
        /*cPacket *TestDataMsg2;
        TestDataMsg2 = new cPacket("This is a test data message 2");
        TestDataMsg2->setByteLength(5);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg2, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg2);
    */

/*        cMessage *m;
        m = new cMessage("BLE_NWKtoMAC_CMD");
        long int initData=par("initialData");
        m->setControlInfo(BLE_NwkToMac::generate_BufferPayloadUpdate(initData));
        sendControlDown(m);
*/

        //error("STOP!");
        // Send route flood packet and restart the timer
        /*WiseRoutePkt* pkt = new WiseRoutePkt("route-flood", ROUTE_FLOOD);
        pkt->setByteLength(headerLength);
        pkt->setInitialSrcAddr(myNetwAddr);
        pkt->setFinalDestAddr(LAddress::L3BROADCAST);
        pkt->setSrcAddr(myNetwAddr);
        pkt->setDestAddr(LAddress::L3BROADCAST);
        pkt->setNbHops(0);
        floodTable.insert(make_pair(myNetwAddr, floodSeqNumber));
        pkt->setSeqNum(floodSeqNumber);
        floodSeqNumber++;
        pkt->setIsFlood(1);
        setDownControlInfo(pkt, LAddress::L2BROADCAST);
        sendDown(pkt);
*/
        /*  BLE_HCI_DataPkt* pkt = new BLE_HCI_DataPkt("HCI_ACL_Data", 0);
            pkt->setHandle(1111);
            //pkt->setPBFlag(true);
            //pkt->setBCFlag(true);
            pkt->setByteLength(400);
            //pkt->setDataTotalLgth(11);
            sendDown(pkt);

        /


        //scheduleAt(simTime()+10, GenerateNewData);//generate more data
    }

}
*/

//Tham modified
void BLE_BasicNwk::handleSelfMsg(cMessage* msg){

    EV << "Tham BLE_BasicNwk::handleSelfMsg : msg = " << msg << endl;

    if(msg==TESTTimer){
         /* if(currentCmdAdvertisePars->AdvEnabled==true){
                currentCmdAdvertisePars->AdvEnabled=false;
             }
             else{
                currentCmdAdvertisePars->AdvEnabled=true;

                currentCmdAdvertisePars->Advertising_Interval_Max=currentCmdAdvertisePars->Advertising_Interval_Max+160;
                currentCmdAdvertisePars->Advertising_Interval_Min=currentCmdAdvertisePars->Advertising_Interval_Min+160;
                cMessage *m1;
                m1 = new cMessage("BLE_NWKtoMAC_CMD");
                m1->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
                sendControlDown(m1);

            }
            cMessage *m;
            m = new cMessage("BLE_NWKtoMAC_CMD");
            m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertiseEnableCommand(currentCmdAdvertisePars));
            sendControlDown(m);
            scheduleAt(simTime()+1, TESTTimer);
        */

        /*cMessage *m;
        m = new cMessage("BLE_NWKtoMAC_CMD");
        m->setControlInfo(BLE_NwkToMac::generate_LECreateConnectionCancelCommand());
        sendControlDown(m);
        */

        //disconnect
        currentCmdError->Handle=currentCmdConnectionPars->ConnHandle;
        currentCmdError->ErrorCode=cBLEstructs_Error::REMOTE_USER_TERMINATED_CONNECTION;
        cMessage *m;
        m = new cMessage("BLE_NWKtoMAC_CMD");
        m->setControlInfo(BLE_NwkToMac::generate_DisconnectCommand(currentCmdError));
        sendControlDown(m);
    }


    if(msg==StartNode){//start all
        ev <<"Tham - BLE_BasicNwk::handleSelfMsg - start node" << endl;
        startNode();
    }



    if(msg==GenerateNewData){//"add" some data to LL buffer
        //ev<< "BLE_BasicNwk::handleSelfMsg on GenerateNewData" << endl;
        //generate a data frame and handle it to MAC
        if(initData){
            ev <<"Tham - BLE_BasicNwk::handleSelfMsg - generate new data with initData = " << initData <<endl;
            cPacket *TestDataMsg;
            TestDataMsg = new cPacket("This is a test data message");
            //ev <<"Tham as initData > 0 "<<initData <<endl;
            TestDataMsg->setByteLength(initData);
            BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
            sendDown(TestDataMsg);
            BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());
        }
        else  ev <<"Tham - BLE_BasicNwk::handleSelfMsg - No Data to generate" << endl;
/*
        TestDataMsg = new cPacket("This is a test data message 2");
        TestDataMsg->setByteLength(2000);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg);
        BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());
        */
        /*TestDataMsg = new cPacket("This is a test data message 2");
        TestDataMsg->setByteLength(555);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg);
        BLEnwkStats->change_NumBytes(TestDataMsg->getByteLength());*/
        //generate a data frame and handle it to MAC
        /*cPacket *TestDataMsg2;
        TestDataMsg2 = new cPacket("This is a test data message 2");
        TestDataMsg2->setByteLength(5);
        BLE_LL_ControlInfo::setControlInfo(TestDataMsg2, currentCmdConnectionPars->ConnHandle);
        sendDown(TestDataMsg2);
    */

/*        cMessage *m;
        m = new cMessage("BLE_NWKtoMAC_CMD");
        long int initData=par("initialData");
        m->setControlInfo(BLE_NwkToMac::generate_BufferPayloadUpdate(initData));
        sendControlDown(m);
*/

        //error("STOP!");
        // Send route flood packet and restart the timer
        /*WiseRoutePkt* pkt = new WiseRoutePkt("route-flood", ROUTE_FLOOD);
        pkt->setByteLength(headerLength);
        pkt->setInitialSrcAddr(myNetwAddr);
        pkt->setFinalDestAddr(LAddress::L3BROADCAST);
        pkt->setSrcAddr(myNetwAddr);
        pkt->setDestAddr(LAddress::L3BROADCAST);
        pkt->setNbHops(0);
        floodTable.insert(make_pair(myNetwAddr, floodSeqNumber));
        pkt->setSeqNum(floodSeqNumber);
        floodSeqNumber++;
        pkt->setIsFlood(1);
        setDownControlInfo(pkt, LAddress::L2BROADCAST);
        sendDown(pkt);
*/
        /*BLE_HCI_DataPkt* pkt = new BLE_HCI_DataPkt("HCI_ACL_Data", 0);
        pkt->setHandle(1111);
        //pkt->setPBFlag(true);
        //pkt->setBCFlag(true);
        pkt->setByteLength(400);
        //pkt->setDataTotalLgth(11);
        sendDown(pkt);*/


        //scheduleAt(simTime()+10, GenerateNewData);//generate more data
    }


}

void BLE_BasicNwk::handleLowerMsg(cMessage* msg){
    ev << "Tham - BLE_BasicNwk::handleLowerMsg " << endl;
    int32_t PacketLength;
    cPacket* pkt = check_and_cast<cPacket*>(msg);
    EV << "BLE_BasicNwk::handleLowerMsg with name " << pkt->getName() << endl;
    PacketLength=pkt->getByteLength();
    delete msg;
    BLEnwkStats->change_NumBytes(-PacketLength);
}


void BLE_BasicNwk::handleUpperControl(cMessage *msg){

}

void BLE_BasicNwk::handleLowerControl(cMessage *msg){

    ev << "Tham - BLE_BasicNwk::handleLowerControl " << endl;

    cObject* cInfo = msg->removeControlInfo();
    BLE_MacToNwk *const cEventInfo = dynamic_cast<BLE_MacToNwk *const>(cInfo);
    EV << "BLE_BasicNwk: Lower control received: event type" << cEventInfo->get_EventType() << endl;

    switch(cEventInfo->get_EventType()){

    case cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Complete_Event:
        EV << "BLE_BasicNwk: BLECMD_HCICtrl_LE_Connection_Complete_Event Connection Handle:" << cEventInfo->get_EventsParsPtr()->Connection_Handle << endl;
        currentCmdConnectionPars->ConnHandle=cEventInfo->get_EventsParsPtr()->Connection_Handle;

        if(cEventInfo->get_EventsParsPtr()->Role==cBLEstructs_defs::MASTER_0){
            //scheduleAt(simTime(), GenerateNewData);//generate more data
           bool useSuggestedFastIntervalSturtupMechanism=par("useSuggestedFastIntervalSturtupMechanism");

           if(useSuggestedFastIntervalSturtupMechanism==true){//modify the parameters
               currentCmdConnectionPars->Conn_Interval_Max=par("connInterval");
               currentCmdConnectionPars->Conn_Interval_Min=par("connInterval");
               currentCmdConnectionPars->Supervision_Timeout=par("supervision_Timeout");
               cMessage *m1;
               m1 = new cMessage("BLE_NWKtoMAC_CMD");
               m1->setControlInfo(BLE_NwkToMac::generate_LEConnectionUpdateCommand(currentCmdConnectionPars));
               sendControlDown(m1);
           }

            //modify connection parameters
/*            bool useSuggestedFastIntervalSturtupMechanism=par("useSuggestedFastIntervalSturtupMechanism");
            if(useSuggestedFastIntervalSturtupMechanism==true){//modify the parameters
                currentCmdConnectionPars->Conn_Interval_Max=par("connInterval");
                currentCmdConnectionPars->Conn_Interval_Min=par("connInterval");
                cMessage *m1;
                m1 = new cMessage("BLE_NWKtoMAC_CMD");
                m1->setControlInfo(BLE_NwkToMac::generate_LEConnectionUpdateCommand(currentCmdConnectionPars));
                sendControlDown(m1);
            }
*/
            //if(initData!=0){
            //scheduleAt(simTime(), GenerateNewData);//generate more data
            //}

            /*
            //update connection parameters
            currentCmdConnectionPars->Conn_Interval_Max=80;
            currentCmdConnectionPars->Conn_Interval_Min=80;
            cMessage *m1;
            m1 = new cMessage("BLE_NWKtoMAC_CMD");
            m1->setControlInfo(BLE_NwkToMac::generate_LEConnectionUpdateCommand(currentCmdConnectionPars));
            sendControlDown(m1);

            //update map
            currentCmdConnectionPars->Data_Channel_Map.fill(0x00,0x00,0x00,0xFF,0x00);
            cMessage *m0;
            m0 = new cMessage("BLE_NWKtoMAC_CMD");
            m0->setControlInfo(BLE_NwkToMac::generate_LESetHostChannelClassificationCommand(currentCmdConnectionPars));
            sendControlDown(m0);

            //disconnect
            currentCmdError->Handle=currentCmdConnectionPars->ConnHandle;
            currentCmdError->ErrorCode=cBLEstructs_Error::REMOTE_USER_TERMINATED_CONNECTION;
            cMessage *m2;
            m2 = new cMessage("BLE_NWKtoMAC_CMD");
            m2->setControlInfo(BLE_NwkToMac::generate_DisconnectCommand(currentCmdError));
            sendControlDown(m2);*/

            //scheduleAt(simTime()+0.02, TESTTimer);
        }
        else if(cEventInfo->get_EventsParsPtr()->Role==cBLEstructs_defs::SLAVE_1){

            /*currentCmdError->Handle=currentCmdConnectionPars->ConnHandle;
            currentCmdError->ErrorCode=cBLEstructs_Error::REMOTE_USER_TERMINATED_CONNECTION;
            cMessage *m;
            m = new cMessage("BLE_NWKtoMAC_CMD");
            m->setControlInfo(BLE_NwkToMac::generate_DisconnectCommand(currentCmdError));
            sendControlDown(m);*/

            //scheduleAt(simTime()+2, TESTTimer);
            //scheduleAt(simTime(), GenerateNewData);//generate more data
        }
        break;

    case cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Update_Complete_Event:
        EV << "BLE_BasicNwk: BLECMD_HCICtrl_LE_Connection_Update_Complete_Event Connection Handle:" << cEventInfo->get_EventsParsPtr()->Connection_Handle << endl;
        break;

    case cBLEstructs_defs::BLECMD_HCICtrl_Disconnection_Complete_Event:
        EV << "BLE_BasicNwk: BLECMD_HCICtrl_Disconnection_Complete_Event Connection Handle:" << cEventInfo->get_EventsParsPtr()->Connection_Handle <<" ErrorCode:" << cEventInfo->get_EventsParsPtr()->ErrorCode << endl;
        scheduleAt(simTime(), StartNode);//restart all
        break;

    //OTHER (NON-STANDARD) EVENT
    case cBLEstructs_defs::Simulation_LLBufferPayloadUpdate_Event:
        EV << "BLE_BasicNwk: Simulation_LLBufferPayloadUpdate_Event In LL TX buffer currently are:" << cEventInfo->get_BufferPayload() <<" bytes (armed packets are not accounted for)" << endl;
        break;
    }

    delete msg;
}


void BLE_BasicNwk::handleUpperMsg(cMessage* msg){
    ev << "Tham - BLE_BasicNwk::handleUpperMsg " << endl;
    EV <<"BLE_BasicNwk: APP data received, name: " << msg->getName() << endl;

}

cMessage* BLE_BasicNwk::decapsMsg(Modular_NwkPkt *msg){
    /*cMessage *m = msg->decapsulate();
    setUpControlInfo(m, msg->getSrcAddr());
    delete msg;
    return m;*/
}



//original
/*
void BLE_BasicNwk::startNode(void){
    int Role=par("NWK_DEBUG_ROLE");
     if(Role==1){//slave
         // set advertisement parameters
         int myIdx=FindModule<>::findHost(this)->getIndex();
/
         currentCmdAdvertisePars->Adv_Type=cBLEstructs_defs::ADV_NONCONN_IND_3;
         ev<< "BLE_BasicNwk::startNode SLAVE: currentCmdAdvertisePars->Adv_Type=" << currentCmdAdvertisePars->Adv_Type <<endl;
         cPacket *testmsg;
         testmsg = new cPacket("IS ANYONE ALIVE????");
         currentCmdAdvertisePars->AdvPacket=testmsg;
         currentCmdAdvertisePars->AdvPacketLgth=20;
         currentCmdAdvertisePars->Direct_Address=(LAddress::L2Type)(myIdx);
         currentCmdAdvertisePars->Direct_Address_Type=cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Advertising_Interval_Max=par("advInterval");
         currentCmdAdvertisePars->Advertising_Interval_Min=par("advInterval");
         currentCmdAdvertisePars->Advertising_Filter_Policy=cBLEstructs_defs::ANY_0;
/

         currentCmdAdvertisePars->Adv_Type=cBLEstructs_defs::ADV_DIRECT_IND_LDC_4;
         ev<< "currentCmdAdvertisePars->Adv_Type" << currentCmdAdvertisePars->Adv_Type <<endl;

         currentCmdAdvertisePars->Direct_Address=(LAddress::L2Type)(myIdx);
         currentCmdAdvertisePars->Direct_Address_Type=cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Advertising_Interval_Max=par("advInterval");
         currentCmdAdvertisePars->Advertising_Interval_Min=par("advInterval");
         currentCmdAdvertisePars->Advertising_Filter_Policy=cBLEstructs_defs::ANY_0;

         //currentCmdAdvertisePars->Advertising_Channel_Map.fill(0, 0, 0, 0, 0x80);
         //currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber=40;

         cMessage *m;
         m = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
         sendControlDown(m);

         // set advertisement data

         / cMessage* AdvMessage = new cMessage("ADVERTISMENT TEST DATA");
         currentCmdAdvertisePars->AdvMessage=AdvMessage;
         currentCmdAdvertisePars->AdvMessageLgth=23; /

         cMessage *m1;
         m1 = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         m1->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingDataCommand(currentCmdAdvertisePars));
         sendControlDown(m1);

         // start advertisements
         currentCmdAdvertisePars->AdvEnabled=true;

         cMessage *m2;
         m2 = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         m2->setControlInfo(BLE_NwkToMac::generate_LESetAdvertiseEnableCommand(currentCmdAdvertisePars));
         sendControlDown(m2);
     }

     else if(Role==2){ // Tham ? is this for master - Passive scanning
         //set required channels
         //currentCmdAdvertisePars->Advertising_Channel_Map.fill(0, 0, 0, 0, 0x80);
         //currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber=40;
         currentCmdAdvertisePars->AdvPacketLgth=0;

         cMessage *m;
         m = new cMessage("BLE_NWKtoMAC_CMD");
         m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
         sendControlDown(m);

         int myIdx=FindModule<>::findHost(this)->getIndex();

         // set scanning connection parameters
         currentCmdScanningPars->LE_Scan_Interval=par("scanInterval");
         currentCmdScanningPars->LE_Scan_Window=par("scanWindow");
         currentCmdScanningPars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdScanningPars->Peer_Address=(LAddress::L2Type)(myIdx);
         currentCmdScanningPars->Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;

         // set desired connection parameters
         currentCmdConnectionPars->Conn_Latency=par("connLatency");

         if(currentCmdConnectionPars->Supervision_Timeout<10)currentCmdConnectionPars->Supervision_Timeout=10;//see p. 2539 of spec v4.1

         bool useSuggestedFastIntervalSturtupMechanism=par("useSuggestedFastIntervalSturtupMechanism");
         if(useSuggestedFastIntervalSturtupMechanism==true){//set the minimum interval
             currentCmdConnectionPars->Conn_Interval_Max=8;
             currentCmdConnectionPars->Conn_Interval_Min=8;
             currentCmdConnectionPars->Supervision_Timeout=10;
         }
         else{//set desired interval straight away
             currentCmdConnectionPars->Conn_Interval_Max=par("connInterval");
             currentCmdConnectionPars->Conn_Interval_Min=par("connInterval");
             currentCmdConnectionPars->Supervision_Timeout=par("supervision_Timeout");
         }

         //currentCmdConnectionPars->Data_Channel_Map.fill(0x00,0xFF,0x00,0x00,0x00);

         //set used data channels
         cMessage *m0;
         m0 = new cMessage("BLE_NWKtoMAC_CMD");
         m0->setControlInfo(BLE_NwkToMac::generate_LESetHostChannelClassificationCommand(currentCmdConnectionPars));
         sendControlDown(m0);

         //switch to initialize state
         cMessage *m1;
         m1 = new cMessage("BLE_NWKtoMAC_CMD");
         m1->setControlInfo(BLE_NwkToMac::generate_LECreateConnectionCommand(currentCmdScanningPars,currentCmdConnectionPars));
         sendControlDown(m1);
     }
}

*/

//Tham modified
void BLE_BasicNwk::startNode(void){
    ev << "Tham - BLE_BasicNwk::startNode" << endl;

    int Role=par("NWK_DEBUG_ROLE");

     if(Role==1) {  //slave

         // (1) set advertisement parameters

         int myIdx = FindModule<>::findHost(this)->getIndex();

         ev <<"Tham - BLE_BasicNwk::startNode  This is a slave" <<endl;
/*
         currentCmdAdvertisePars->Adv_Type=cBLEstructs_defs::ADV_NONCONN_IND_3;
         ev<< "BLE_BasicNwk::startNode SLAVE: currentCmdAdvertisePars->Adv_Type=" << currentCmdAdvertisePars->Adv_Type <<endl;
         cPacket *testmsg;
         testmsg = new cPacket("IS ANYONE ALIVE????");
         currentCmdAdvertisePars->AdvPacket=testmsg;
         currentCmdAdvertisePars->AdvPacketLgth=20;
         currentCmdAdvertisePars->Direct_Address=(LAddress::L2Type)(myIdx);
         currentCmdAdvertisePars->Direct_Address_Type=cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Advertising_Interval_Max=par("advInterval");
         currentCmdAdvertisePars->Advertising_Interval_Min=par("advInterval");
         currentCmdAdvertisePars->Advertising_Filter_Policy=cBLEstructs_defs::ANY_0;
*/

         currentCmdAdvertisePars->Adv_Type=cBLEstructs_defs::ADV_IND_0;

        // ev<< "Tham : currentCmdAdvertisePars->Adv_Type " << currentCmdAdvertisePars->Adv_Type <<endl;

         currentCmdAdvertisePars->Direct_Address=(LAddress::L2Type)(myIdx);
         currentCmdAdvertisePars->Direct_Address_Type=cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdAdvertisePars->Advertising_Interval_Max=par("advInterval");
         currentCmdAdvertisePars->Advertising_Interval_Min=par("advInterval");
         currentCmdAdvertisePars->Advertising_Filter_Policy=cBLEstructs_defs::ANY_0;


         //currentCmdAdvertisePars->Advertising_Channel_Map.fill(0, 0, 0, 0, 0x80);
         //currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber=40;

         cMessage *m;
         m = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
         sendControlDown(m);

         // (2) set advertisement data

         cPacket* AdvMsg = new cPacket("BLE_NWKtoMAC_CMD");
         currentCmdAdvertisePars->AdvPacket=AdvMsg;
         currentCmdAdvertisePars->AdvPacketLgth=16; // tham set
         AdvMsg->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingDataCommand(currentCmdAdvertisePars));
         sendControlDown(AdvMsg);

        /* cMessage *m1;
         m1 = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         currentCmdAdvertisePars->AdvData = 10;
        // currentCmdAdvertisePars->AdvPacketLgth=23;
         m1->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingDataCommand(currentCmdAdvertisePars));
         sendControlDown(m1); */

         /*
          cPacket *m1;
          m1 = new cPacket("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose //packet_with_Adv_Data
          currentCmdAdvertisePars->AdvPacket = m1;
         // currentCmdAdvertisePars->AdvEnabled = true;
          currentCmdAdvertisePars->AdvPacketLgth = 10;
         // currentCmdAdvertisePars->AdvData = '12345';
          m1->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingDataCommand(currentCmdAdvertisePars));
          sendControlDown(m1);
        */

         // (3) start advertisements

         currentCmdAdvertisePars->AdvEnabled=true;

         cMessage *m2;
         m2 = new cMessage("BLE_NWKtoMAC_CMD"); // Tham ? this msg for what purpose
         m2->setControlInfo(BLE_NwkToMac::generate_LESetAdvertiseEnableCommand(currentCmdAdvertisePars));
         sendControlDown(m2);
         Node_role = 1;
     }

     else if(Role==2){ // Master passive scanning

         ev <<"Tham - BLE_BasicNwk::startNode  This is a master" <<endl;


         //set required channels
         //currentCmdAdvertisePars->Advertising_Channel_Map.fill(0, 0, 0, 0, 0x80);
         //currentCmdAdvertisePars->Advertising_Channel_Map.TotalChNumber=40;

        // currentCmdAdvertisePars->Adv_Type=cBLEstructs_defs::SCAN_REQ; // tham
        // currentCmdAdvertisePars->AdvPacketLgth=0; // Tham - no data -> SCAN_REQ

         cMessage *m;
         m = new cMessage("BLE_NWKtoMAC_CMD");
         m->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingParametersCommand(currentCmdAdvertisePars));
         sendControlDown(m);

         int myIdx=FindModule<>::findHost(this)->getIndex();

         // (1) set scanning connection parameters
         currentCmdScanningPars->LE_Scan_Interval=par("scanInterval");
         currentCmdScanningPars->LE_Scan_Window=par("scanWindow");
         //currentCmdScanningPars->LE_Scan_Type=par("scanType");
         currentCmdScanningPars->Own_Address_Type=cBLEstructs_defs::OWN_ADDR_PUBLIC_0;
         currentCmdScanningPars->Peer_Address=(LAddress::L2Type)(myIdx);
         currentCmdScanningPars->Peer_Address_Type=cBLEstructs_defs::PEER_ADDR_PUBLIC_0;

         // (2) set desired connection parameters
         currentCmdConnectionPars->Conn_Latency=par("connLatency");

         if(currentCmdConnectionPars->Supervision_Timeout<10)currentCmdConnectionPars->Supervision_Timeout=10;//see p. 2539 of spec v4.1

         bool useSuggestedFastIntervalSturtupMechanism=par("useSuggestedFastIntervalSturtupMechanism");
         if(useSuggestedFastIntervalSturtupMechanism==true){//set the minimum interval
             currentCmdConnectionPars->Conn_Interval_Max=8;
             currentCmdConnectionPars->Conn_Interval_Min=8;
             currentCmdConnectionPars->Supervision_Timeout=10;
         }
         else{//set desired interval straight away
             currentCmdConnectionPars->Conn_Interval_Max=par("connInterval");
             currentCmdConnectionPars->Conn_Interval_Min=par("connInterval");
             currentCmdConnectionPars->Supervision_Timeout=par("supervision_Timeout");
         }

         //currentCmdConnectionPars->Data_Channel_Map.fill(0x00,0xFF,0x00,0x00,0x00);

         //(3) set used data channels
         cMessage *m0;
         m0 = new cMessage("BLE_NWKtoMAC_CMD");
         m0->setControlInfo(BLE_NwkToMac::generate_LESetHostChannelClassificationCommand(currentCmdConnectionPars));
         sendControlDown(m0);

         //switch to initialize state
         cMessage *m1;
         m1 = new cMessage("BLE_NWKtoMAC_CMD");
         m1->setControlInfo(BLE_NwkToMac::generate_LECreateConnectionCommand(currentCmdScanningPars,currentCmdConnectionPars));
         sendControlDown(m1);
         Node_role = 2;

        /* // THAM - (4) set advertisement data

              cPacket* ScanMsg = new cPacket("BLE_NWKtoMAC_CMD");
              currentCmdAdvertisePars->AdvPacket=ScanMsg;
              currentCmdAdvertisePars->AdvPacketLgth=10;
              ScanMsg->setControlInfo(BLE_NwkToMac::generate_LESetAdvertisingDataCommand(currentCmdAdvertisePars));
              sendControlDown(ScanMsg);
                */

     }
}

