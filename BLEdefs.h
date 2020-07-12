/* -*- mode:c++ -*- ********************************************************
 * file:        BLEdefs.h
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 ***************************************************************************
 * BLE data and parameters structures
 **************************************************************************/
#ifndef BLEDEFS_H_
#define BLEDEFS_H_

#include "MiXiMDefs.h"
#include "SimpleAddress.h"

#include "ConstsBLE.h"


class MIXIM_API cBLEstructs_Error : public cObject{
public:
    cBLEstructs_Error(): Handle(0), ErrorCode(0) {}; // this is a constructor with initial value of some members = Tham

    virtual ~cBLEstructs_Error(){}; // this is a destrustor

    public:
    long int Handle;
    int ErrorCode;

    enum ErrorCodes { //see pp. 663 of BLE spec. v.4.1
        AUTHENTICATION_FAILURE=0x05
        ,CONNECTION_TIMEOUT=0x08
        ,REMOTE_USER_TERMINATED_CONNECTION=0x13
        ,REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_LOW_RESOURCES
        ,REMOTE_DEVICE_TERMINATED_CONNECTION_DUE_TO_POWER_OFF
        ,UNSUPPORTED_REMOTE_FEATURE=0x1A
        ,UNSPECIFIFED_ERROR=0x1F
        ,PAIRING_WITH_UNIT_KEY_NOT_SUPPORTED=0x29
        ,UNACCEPTABLE_CONNECTION_PARAMETERS=0x3B
    };


};


//map of the channel with bool value for each one (intended for masking the used channels)
class MIXIM_API cBLEstructs_ChannelMap : public cObject{
    public:
        cBLEstructs_ChannelMap(): TotalChNumber(40), ChannelTbl() {}; // this is a constructor to set the initial values
        virtual ~cBLEstructs_ChannelMap(){};

    public:
        int TotalChNumber;
        bool ChannelTbl[40];

    virtual void fill(int StateCh0_7, int StateCh8_15, int StateCh16_23, int StateCh24_31, int StateCh32_39){
        //set channel map - not elegant, but should work
         int i_temp,i_mask,i_ctr, i_numCh;
         i_numCh=0;

         i_temp = StateCh0_7;
         i_mask = 1;
         i_ctr=0;
         do{
             if(i_mask&i_temp){
                 ChannelTbl[i_ctr]=true;
                 i_numCh++;
             }
             else ChannelTbl[i_ctr]=false;
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);

         i_temp = StateCh8_15;
         i_mask = 1;
         i_ctr=8;
         do{
             if(i_mask&i_temp){
                 ChannelTbl[i_ctr]=true;
                 i_numCh++;
             }
             else ChannelTbl[i_ctr]=false;
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);

         i_temp = StateCh16_23;
         i_mask = 1;
         i_ctr=16;
         do{
             if(i_mask&i_temp){
                 ChannelTbl[i_ctr]=true;
                 i_numCh++;
             }
             else ChannelTbl[i_ctr]=false;
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);

         i_temp = StateCh24_31;
         i_mask = 1;
         i_ctr=24;
         do{
             if(i_mask&i_temp){
                 ChannelTbl[i_ctr]=true;
                 i_numCh++;
             }
             else ChannelTbl[i_ctr]=false;
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);

         i_temp = StateCh32_39;
         i_mask = 1;
         i_ctr=32;
         do{
             if(i_mask&i_temp){
                 ChannelTbl[i_ctr]=true;
                 i_numCh++;
             }
             else ChannelTbl[i_ctr]=false;
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         TotalChNumber=i_numCh;
    }

    virtual void getMap(int *StateCh0_7, int *StateCh8_15, int *StateCh16_23, int *StateCh24_31, int *StateCh32_39){
        //set channel map - not elegant, but should work
         int i_temp,i_mask,i_ctr, i_numCh;
         i_numCh=0;

         i_temp = 0;
         i_mask = 1;
         i_ctr=0;

         do{
             if(ChannelTbl[i_ctr]){
                 i_temp=i_temp+i_mask;
             }
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         *StateCh0_7=i_temp;


         i_temp = 0;
         i_mask = 1;
         i_ctr=8;
         do{
             if(ChannelTbl[i_ctr]){
                 i_temp=i_temp+i_mask;
             }
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         *StateCh8_15=i_temp;

         i_temp = 0;
         i_mask = 1;
         i_ctr=16;
         do{
             if(ChannelTbl[i_ctr]){
                 i_temp=i_temp+i_mask;
             }
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         *StateCh16_23=i_temp;

         i_temp = 0;
         i_mask = 1;
         i_ctr=24;
         do{
             if(ChannelTbl[i_ctr]){
                 i_temp=i_temp+i_mask;
             }
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         *StateCh24_31=i_temp;

         i_temp = 0;
         i_mask = 1;
         i_ctr=32;
         do{
             if(ChannelTbl[i_ctr]){
                 i_temp=i_temp+i_mask;
             }
             i_ctr++;
             i_mask=i_mask*2;
         }while(i_mask<256);
         *StateCh32_39=i_temp;
    }





    virtual void print(){
        EV  <<"Printing ChannelMap:"<< endl;
        EV  <<"TotalChNumber="<< TotalChNumber<< endl;
        for(int i=0; i<40;i++){
            EV  <<"Channel N=" << i << " State=" << ChannelTbl[i] << endl;
        }
    }
};

//just defines most basic things

class MIXIM_API cBLEstructs_defs : public cObject{
    public:
        cBLEstructs_defs(){};
        virtual ~cBLEstructs_defs(){};

    public:
        enum BLECmdTypes_HCItoCtrl { //see pp. 747-749 of BLE spec. v.4.1
            //Note: not all the commands are actually implemented
            BLECMD_HCICtrl_UNDEFINED=-1
            //Mandatory
            ,BLECMD_HCICtrl_CommandCompleteEvent=0
            ,BLECMD_HCICtrl_CommandStatusEvent
            ,BLECMD_HCICtrl_LEAddDeviceToWhiteListCommand
            ,BLECMD_HCICtrl_LEClearWhiteListCommand
            ,BLECMD_HCICtrl_LEReadBufferSizeCommand
            ,BLECMD_HCICtrl_LEReadLocalSupportedFeaturesCommand
            ,BLECMD_HCICtrl_LEReadSupportedStatesCommand
            ,BLECMD_HCICtrl_LEReadWhiteListSizeCommand
            ,BLECMD_HCICtrl_LERemoveDeviceFromWhiteListCommand
            ,BLECMD_HCICtrl_LESetEventMaskCommand
            ,BLECMD_HCICtrl_LETestEndCommand
            ,BLECMD_HCICtrl_ReadBD_ADDRCommand
            ,BLECMD_HCICtrl_ReadLocalSupportedFeaturesCommand
            ,BLECMD_HCICtrl_ReadLocalVersionInformationCommand
            ,BLECMD_HCICtrl_ResetCommand
            ,BLECMD_HCICtrl_ReadLocalSupportedCommandsCommand
            ,BLECMD_HCICtrl_SetEventMaskCommand
            //C1
            ,BLECMD_HCICtrl_LEReadAdvertisingChannelTXPowerCommand
            ,BLECMD_HCICtrl_LETransmitterTestCommand
            ,BLECMD_HCICtrl_LESetAdvertiseEnableCommand // 19 +
            ,BLECMD_HCICtrl_LESetAdvertisingDataCommand // 20 +
            ,BLECMD_HCICtrl_LESetAdvertisingParametersCommand // 21 +
            ,BLECMD_HCICtrl_LESetRandomAddressCommand
            //C2
            ,BLECMD_HCICtrl_LEAdvertisingReportEvent
            ,BLECMD_HCICtrl_LEReceiverTestCommand
            ,BLECMD_HCICtrl_LESetScanEnableCommand
            ,BLECMD_HCICtrl_LESetScanParametersCommand
            //C3
            ,BLECMD_HCICtrl_Disconnect_Command //+ pp. 805 spec v4.1
            ,BLECMD_HCICtrl_Disconnection_Complete_Event //+ pp. 1138 spec v4.1
            ,BLECMD_HCICtrl_LE_Connection_Complete_Event //+
            ,BLECMD_HCICtrl_LE_Connection_Update_Command // 30  + pp. 1269 spec v4.1
            ,BLECMD_HCICtrl_LE_Connection_Update_Complete_Event // pp. 1222 spec v4.1
            ,BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command //+
            ,BLECMD_HCICtrl_LE_Create_Connection_Command // 33 +
            ,BLECMD_HCICtrl_LE_Read_Channel_Map_Command
            ,BLECMD_HCICtrl_LE_Read_Remote_Used_Features_Command
            ,BLECMD_HCICtrl_LE_Read_Remote_Used_Features_Complete_Event
            ,BLECMD_HCICtrl_LE_Set_Host_Channel_Classification_Command // 37  + - sets data map - pp. 1272 spec v4.1
            ,BLECMD_HCICtrl_LE_Set_Scan_Response_Data_Command // 38
            ,BLECMD_HCICtrl_Number_Of_Completed_Packets_Event
            ,BLECMD_HCICtrl_Read_Transmit_Power_Level_Command
            ,BLECMD_HCICtrl_Read_Remote_Version_Information_Command
            ,BLECMD_HCICtrl_Read_Remote_Version_Information_Complete_Event

            //TODO: extend


            //NON-STANDARD SIMULATION SPECIFIC STUFF
            ,Simulation_LLBufferPayloadUpdate_Command //used to modify the number of bytes in LL buffer
            ,Simulation_LLBufferPayloadUpdate_Event //returns number of bytes in LL buffer


        };


        //const static char* BLECmdTypes_HCItoCtrl_TEXT[2];
        static const char* BLECmdTypes_HCItoCtrl_TEXT[2];
/*              //Note: not all the commands are actually implemented
              "BLECMD_HCICtrl_UNDEFINED"
              //Mandatory
              ,"BLECMD_HCICtrl_CommandCompleteEvent"
              ,"BLECMD_HCICtrl_CommandStatusEvent"
              ,"BLECMD_HCICtrl_LEAddDeviceToWhiteListCommand"
              ,"BLECMD_HCICtrl_LEClearWhiteListCommand"
              ,"BLECMD_HCICtrl_LEReadBufferSizeCommand"
              ,"BLECMD_HCICtrl_LEReadLocalSupportedFeaturesCommand"
              ,"BLECMD_HCICtrl_LEReadSupportedStatesCommand"
              ,"BLECMD_HCICtrl_LEReadWhiteListSizeCommand"
              ,"BLECMD_HCICtrl_LERemoveDeviceFromWhiteListCommand"
              ,"BLECMD_HCICtrl_LESetEventMaskCommand"
              ,"BLECMD_HCICtrl_LETestEndCommand"
              ,"BLECMD_HCICtrl_ReadBD_ADDRCommand"
              ,"BLECMD_HCICtrl_ReadLocalSupportedFeaturesCommand"
              ,"BLECMD_HCICtrl_ReadLocalVersionInformationCommand"
              ,"BLECMD_HCICtrl_ResetCommand"
              ,"BLECMD_HCICtrl_ReadLocalSupportedCommandsCommand"
              ,"BLECMD_HCICtrl_SetEventMaskCommand"
              //C1
              ,"BLECMD_HCICtrl_LEReadAdvertisingChannelTXPowerCommand"
              ,"BLECMD_HCICtrl_LETransmitterTestCommand"
              ,"BLECMD_HCICtrl_LESetAdvertiseEnableCommand" // 19 +
              ,"BLECMD_HCICtrl_LESetAdvertisingDataCommand" // 20 +
              ,"BLECMD_HCICtrl_LESetAdvertisingParametersCommand" // 21 +
              ,"BLECMD_HCICtrl_LESetRandomAddressCommand"
              //C2
              ,"BLECMD_HCICtrl_LEAdvertisingReportEvent"
              ,"BLECMD_HCICtrl_LEReceiverTestCommand"
              ,"BLECMD_HCICtrl_LESetScanEnableCommand"
              ,"BLECMD_HCICtrl_LESetScanParametersCommand"
              //C3
              ,"BLECMD_HCICtrl_Disconnect_Command" //+ pp. 805 spec v4.1
              ,"BLECMD_HCICtrl_Disconnection_Complete_Event" //+ pp. 1138 spec v4.1
              ,"BLECMD_HCICtrl_LE_Connection_Complete_Event" //+
              ,"BLECMD_HCICtrl_LE_Connection_Update_Command" //+ pp. 1269 spec v4.1
              ,"BLECMD_HCICtrl_LE_Connection_Update_Complete_Event" // pp. 1222 spec v4.1
              ,"BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command" //+
              ,"BLECMD_HCICtrl_LE_Create_Connection_Command" // 33 +
              ,"BLECMD_HCICtrl_LE_Read_Channel_Map_Command"
              ,"BLECMD_HCICtrl_LE_Read_Remote_Used_Features_Command"
              ,"BLECMD_HCICtrl_LE_Read_Remote_Used_Features_Complete_Event"
              ,"BLECMD_HCICtrl_LE_Set_Host_Channel_Classification_Command" //+ - sets data map - pp. 1272 spec v4.1
              ,"BLECMD_HCICtrl_LE_Set_Scan_Response_Data_Command"
              ,"BLECMD_HCICtrl_Number_Of_Completed_Packets_Event"
              ,"BLECMD_HCICtrl_Read_Transmit_Power_Level_Command"
              ,"BLECMD_HCICtrl_Read_Remote_Version_Information_Command"
              ,"BLECMD_HCICtrl_Read_Remote_Version_Information_Complete_Event"

              //TODO: extend


              //NON-STANDARD SIMULATION SPECIFIC STUFF
              ,"Simulation_LLBufferPayloadUpdate_Command" //used to modify the number of bytes in LL buffer
              ,"Simulation_LLBufferPayloadUpdate_Event" //returns number of bytes in LL buffer

*/



        enum t_mac_AdvType {//spec v4.1 p. 1247
            ADV_IND_0=0
            ,ADV_DIRECT_IND_HDC_1
            ,ADV_SCAN_IND_2
            ,ADV_NONCONN_IND_3
            ,ADV_DIRECT_IND_LDC_4
            ,SCAN_REQ  //Tham added 2018
            ,SCAN_RSP  //Tham added 2018
        };


        enum t_mac_AdvPDUtype {//spec v4.1 p. 2505
            PDU_ADV_IND_0=0
            ,PDU_ADV_DIRECT_IND_1
            ,PDU_ADV_NONCONN_IND_2
            ,PDU_CONNECT_REQ_3
            ,PDU_ADV_SCAN_IND_4
            ,PDU_SCAN_REQ_5
            ,PDU_SCAN_RSP_6
        };

       // enum t_mac_ScanType { //Tham added 2018
          //  SCAN_REQ=0
          //  ,SCAN_RSP_1
      //  };

        enum t_mac_OwnAddrType {//spec v4.1 p. 1247
            OWN_ADDR_PUBLIC_0=0
            ,OWN_ADDR_RANDOM_1
        };

        enum t_mac_DirectAddrType {//spec v4.1 p. 1248
            DIRECT_ADDR_PUBLIC_0=0
            ,DIRECT_ADDR_RANDOM_1
        };

        enum t_mac_PeerAddrType {//spec v4.1 p. 1261
            PEER_ADDR_PUBLIC_0=0
            ,PEER_ADDR_RANDOM_1
        };

        enum t_mac_AdvFilterPolicy {//spec v4.1 p. 1248
            ANY_0=0
            ,CNCTfrmANY_SCANfrmWL
            ,CNCTfrmWL_SCANfrmANY
            ,WHITE_LIST
        };

        enum t_NodeRole {
            MASTER_0=0
            ,SLAVE_1
        };


        typedef int tConnectionHandle;//2-byte connection handle ID (Range 0x0000-0x0EFF - see spec v4.1 p. 1269)


};

/*
//advertisement parameters - original
class MIXIM_API cBLEstructs_cAdvertisePars : public cObject{
    public:
    cBLEstructs_cAdvertisePars():
        Advertising_Interval_Min(0x800),Advertising_Interval_Max(0x800),Adv_Type(cBLEstructs_defs::ADV_DIRECT_IND_LDC_4),
        Own_Address_Type(cBLEstructs_defs::OWN_ADDR_PUBLIC_0),Direct_Address_Type(cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0),
        Direct_Address(),Advertising_Channel_Map(),Advertising_Filter_Policy(cBLEstructs_defs::ANY_0),
        ADV_IND_PDUperiod(0.01),ADV_DIRECT_IND_LDC_PDUperiod(0.01),ADV_DIRECT_IND_HDC_PDUperiod(0.00375),ADV_SCAN_IND_PDUperiod(0.01),ADV_NONCONN_IND_PDUperiod(0.01),
        AdvPacketLgth(0),

        SCAN_ScanWindow(0.1),SCAN_ScanInterval(0.1)

        //,OwnAdvAddr()
     {};
    // @brief Destructor
    virtual ~cBLEstructs_cAdvertisePars(){
        //~Advertising_Channel_Map();
    };

    public:
    int                     Advertising_Interval_Min; //in slots, to get actual time multiply by 0.625 ms
    int                     Advertising_Interval_Max; //in slots, to get actual time multiply by 0.625 ms
    cBLEstructs_defs::t_mac_AdvType           Adv_Type;
    cBLEstructs_defs::t_mac_OwnAddrType       Own_Address_Type;
    cBLEstructs_defs::t_mac_DirectAddrType    Direct_Address_Type;
    LAddress::L2Type        Direct_Address;
    cBLEstructs_ChannelMap  Advertising_Channel_Map;
    cBLEstructs_defs::t_mac_AdvFilterPolicy   Advertising_Filter_Policy;

    cPacket        *        AdvPacket;
    int                     AdvPacketLgth;
    bool                    AdvEnabled;

    LAddress::L2Type        OwnAdvAddr;

    simtime_t ADV_IND_PDUperiod;
    simtime_t ADV_DIRECT_IND_LDC_PDUperiod;
    simtime_t ADV_DIRECT_IND_HDC_PDUperiod;
    simtime_t ADV_SCAN_IND_PDUperiod;
    simtime_t ADV_NONCONN_IND_PDUperiod;
    simtime_t SCAN_ScanWindow;//see p.2535 of Spec v4.1
    simtime_t SCAN_ScanInterval;//see p.2535 of Spec v4.1

    int GetNextAdvChannel(int StartChannel){
        int iChannelIndex=StartChannel;
        if (iChannelIndex>=40) return -1;
        while(iChannelIndex<40){
            if(Advertising_Channel_Map.ChannelTbl[iChannelIndex]==true){
                return iChannelIndex;
            }
            iChannelIndex++;
        }
        return -1;
    }


    //public
    virtual void print(){
        EV  <<"Printing advertising parameters:"<< endl;
        EV  <<"Advertising_Interval_Min=" << Advertising_Interval_Min << endl;
        EV  <<"Advertising_Interval_Max=" << Advertising_Interval_Max << endl;
        EV  <<"Adv_Type=" << Adv_Type << endl;
        EV  <<"Own_Address_Type=" << Own_Address_Type << endl;
        EV  <<"Direct_Address_Type=" << Direct_Address_Type << endl;
        EV  <<"Direct_Address=" << Direct_Address << endl;
        for(int i=0; i<Advertising_Channel_Map.TotalChNumber;i++){
            EV  <<"Channel N=" << i << " State=" << Advertising_Channel_Map.ChannelTbl[i] << endl;
        }
        EV  <<"Advertising_Filter_Policy=" << Advertising_Filter_Policy <<endl;

        if(AdvPacket)
        EV  <<"AdvPacket=" << AdvPacket->getName() <<endl;

        EV  <<"AdvPacketLgth=" << AdvPacketLgth <<endl;
        EV  <<"AdvEnabled=" << AdvEnabled <<endl;
    }

    virtual bool check(){
        //TODO
        return true;
    }

};

*/

//advertisement parameters - Tham modified for ADV_IND

class MIXIM_API cBLEstructs_cAdvertisePars : public cObject{
    public:
     cBLEstructs_cAdvertisePars(): //constructor
        Advertising_Interval_Min(0x800), //orginal (0x800) = default as Spec 4.2 0x0800 (1.28s) - can be set in omnetppini
        Advertising_Interval_Max(0x800), // original 0x800 = default as Spec 4.2 0x0800 (1.28s) - can be set in omnetppini
        Adv_Type(cBLEstructs_defs::ADV_IND_0),
        Own_Address_Type(cBLEstructs_defs::OWN_ADDR_PUBLIC_0),
        Direct_Address_Type(cBLEstructs_defs::DIRECT_ADDR_PUBLIC_0),
        Direct_Address(),
        Advertising_Channel_Map(),
        Advertising_Filter_Policy(cBLEstructs_defs::ANY_0),
        ADV_IND_PDUperiod(0.01),
        ADV_DIRECT_IND_LDC_PDUperiod(0.01),
        ADV_DIRECT_IND_HDC_PDUperiod(0.00375),
        ADV_SCAN_IND_PDUperiod(0.01),
        ADV_NONCONN_IND_PDUperiod(0.01),
        AdvPacketLgth(0), //at first there is no data, the packet will fill data once it is created.
        SCAN_ScanWindow(0.1), // for ADV_SCAN not for LE scanning
        SCAN_ScanInterval(0.1) // for ADV_SCAN not for LE scanning
        //,OwnAdvAddr()
     {}; // initial values from the construstor

    //@brief Destructor
    virtual ~cBLEstructs_cAdvertisePars(){
        //~Advertising_Channel_Map();
    };

    public:
    int                     Advertising_Interval_Min; //in slots, to get actual time multiply by 0.625 ms
    int                     Advertising_Interval_Max; //in slots, to get actual time multiply by 0.625 ms
    cBLEstructs_defs::t_mac_AdvType           Adv_Type;
    cBLEstructs_defs::t_mac_OwnAddrType       Own_Address_Type;
    cBLEstructs_defs::t_mac_DirectAddrType    Direct_Address_Type;
    LAddress::L2Type        Direct_Address;
    cBLEstructs_ChannelMap  Advertising_Channel_Map;
    cBLEstructs_defs::t_mac_AdvFilterPolicy   Advertising_Filter_Policy;
    cPacket                 * AdvPacket;
    int                     AdvPacketLgth;
    bool                    AdvEnabled;
    LAddress::L2Type        OwnAdvAddr;
    simtime_t ADV_IND_PDUperiod;
    simtime_t ADV_DIRECT_IND_LDC_PDUperiod;
    simtime_t ADV_DIRECT_IND_HDC_PDUperiod;
    simtime_t ADV_SCAN_IND_PDUperiod;
    simtime_t ADV_NONCONN_IND_PDUperiod;
    simtime_t SCAN_ScanWindow;//see p.2535 of Spec v4.1
    simtime_t SCAN_ScanInterval;//see p.2535 of Spec v4.1

    int GetNextAdvChannel(int StartChannel){
        int iChannelIndex=StartChannel;
        if (iChannelIndex>=40) return -1;
        while(iChannelIndex<40){
            if(Advertising_Channel_Map.ChannelTbl[iChannelIndex]==true){
                return iChannelIndex;
            }
            iChannelIndex++;
        }
        return -1;
    }

    //public
    virtual void print(){
        EV  <<"Printing advertising parameters:"<< endl;
        EV  <<"Advertising_Interval_Min=" << Advertising_Interval_Min << endl;
        EV  <<"Advertising_Interval_Max=" << Advertising_Interval_Max << endl;
        EV  <<"Adv_Type=" << Adv_Type << endl;
        EV  <<"Own_Address_Type=" << Own_Address_Type << endl;
        EV  <<"Direct_Address_Type=" << Direct_Address_Type << endl;
        EV  <<"Direct_Address=" << Direct_Address << endl;
        for(int i=0; i<Advertising_Channel_Map.TotalChNumber;i++){
            EV  <<"Channel N=" << i << " State=" << Advertising_Channel_Map.ChannelTbl[i] << endl;
        }

        EV  <<"Advertising_Filter_Policy=" << Advertising_Filter_Policy <<endl;

      //  if(AdvPacket)
        EV  <<"AdvPacket=" << AdvPacket->getName() <<endl;
        EV  <<"AdvPacketLgth=" << AdvPacketLgth <<endl;
        EV  <<"AdvEnabled=" << AdvEnabled <<endl;
        EV << "Tham added for ADV_IND instead of ADV_DIRECT_IND" << endl;
    }


    virtual bool check(){
        //TODO
        return true;
    }

};


/*
//scanning & initialization parameters - original
class MIXIM_API cBLEstructs_cScanningPars : public cObject{

    public:
    cBLEstructs_cScanningPars(): LE_Scan_Interval(160), LE_Scan_Window(160), Peer_Address_Type(){};
    // @brief Destructor
    virtual ~cBLEstructs_cScanningPars(){
    };

    public:
    //scanning establishment
    long int LE_Scan_Interval;//in 0.626 ms units, Range: 0x0004 – 0x4000, see p.1261 of Spec v4.1
    long int LE_Scan_Window;//in 0.626 ms units, Range: 0x0004 – 0x4000, see p.1261 of Spec v4.1
    cBLEstructs_defs::t_mac_PeerAddrType      Peer_Address_Type;
    LAddress::L2Type        Peer_Address;  //address of the device to be connected
    cBLEstructs_defs::t_mac_OwnAddrType       Own_Address_Type;

    //public
    virtual void print(){
        EV  <<"LE_Scan_Interval=" << LE_Scan_Interval << "       Note: range: 0x0004 – 0x4000" << endl;
        EV  <<"LE_Scan_Window=" << LE_Scan_Window << "       Note: range: 0x0004 – 0x4000" << endl;
        EV  <<"Peer_Address_Type=" << Peer_Address_Type << endl;
        EV  <<"Peer_Address=" << Peer_Address << endl;
        EV  <<"Own_Address_Type=" << Own_Address_Type << endl;
    }

    virtual bool check(){
        if((LE_Scan_Window<4)||(LE_Scan_Window>0x4000))return false;
        if((LE_Scan_Interval<4)||(LE_Scan_Interval>0x4000))return false;
        return true;
    }

};
*/

//scanning & initialization parameters - Tham modified
class MIXIM_API cBLEstructs_cScanningPars : public cObject{
    public:
    cBLEstructs_cScanningPars(): LE_Scan_Type(0x01),
    LE_Scan_Interval(160), // can be set in omnetppini
    LE_Scan_Window(160), // can be set in omnetppini
    Peer_Address_Type(){}; // original 160
   // Adv_Type(cBLEstructs_defs::SCAN_REQ),

    // @brief Destructor*/
    virtual ~cBLEstructs_cScanningPars(){
    };

    public:
    //scanning establishment
    long int LE_Scan_Type; //0x00 is passive; 0x01 is active scanning
    long int LE_Scan_Interval;//in 0.625 ms units, Range: 0x0004 to 0x4000, see p.1261 of Spec v4.1
    long int LE_Scan_Window;//in 0.625 ms units, Range: 0x0004 to 0x4000, see p.1261 of Spec v4.1
    cBLEstructs_defs::t_mac_PeerAddrType      Peer_Address_Type;
    LAddress::L2Type        Peer_Address;  //address of the device to be connected
    cBLEstructs_defs::t_mac_OwnAddrType       Own_Address_Type;

    //public
    virtual void print(){
        EV  <<"LE_Scan_Type=" << LE_Scan_Type << endl;
        EV  <<"LE_Scan_Interval=" << LE_Scan_Interval << "Note: range: 0x0004 to 0x4000 (2.5ms to 10.24s)" << endl;
        EV  <<"LE_Scan_Window=" << LE_Scan_Window << "Note: range: 0x0004 to 0x4000 (2.5ms to 10.24s)" << endl;
        EV  <<"Peer_Address_Type=" << Peer_Address_Type << endl;
        EV  <<"Peer_Address=" << Peer_Address << endl;
        EV  <<"Own_Address_Type=" << Own_Address_Type << endl;
        EV  <<"For scanning state" << endl; // Tham added
    }

    virtual bool check(){
        if((LE_Scan_Window<4)||(LE_Scan_Window>0x4000))return false;
        if((LE_Scan_Interval<4)||(LE_Scan_Interval>0x4000))return false;
        return true;
    }

};



//events data
class MIXIM_API cBLEstructs_cEventsData : public cObject{ // Tham - it is A derived class of cObject class

    public:
    cBLEstructs_cEventsData(): Connection_Handle(0) {};
    /** @brief Destructor*/
    virtual ~cBLEstructs_cEventsData(){
    };

    public:
    int Status;
    long int Connection_Handle;
    cBLEstructs_defs::t_NodeRole Role;
    cBLEstructs_defs::t_mac_PeerAddrType      Peer_Address_Type;
    LAddress::L2Type        Peer_Address;  //address of the device to be connected
    int Conn_Interval;//in 1.25 ms units, value range 0x0006 to 0x0C80 - see p. 1270 of BLE spec v4.1
    long int Conn_Latency;//range: 0x0000 to 0x01F3 - see p. 1270 of BLE spec v4.1
    long int Supervision_Timeout;//in 10 ms units, range:0x000A to 0x0C80 - see p. 1270 of BLE spec v4.1
    int Master_Clock_Accuracy;//see p. 1219
    int ErrorCode;

    //public
    virtual void print(int event){
        if(event==cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Complete_Event){
            EV  <<"Connection_Handle=" << Connection_Handle << endl;
            EV  <<"Role=" << Role << endl;
            EV  <<"Peer_Address_Type=" << Peer_Address_Type << endl;
            EV  <<"Peer_Address=" << Peer_Address << endl;
            EV  <<"Conn_Interval=" << Conn_Interval << endl;
            EV  <<"Conn_Latency=" << Conn_Latency << endl;
            EV  <<"Supervision_Timeout=" << Supervision_Timeout << endl;
            EV  <<"Master_Clock_Accuracy=" << Master_Clock_Accuracy << endl;
        }
    }

    virtual bool check(){
        return true;
    }

};



//connection parameters
class MIXIM_API cBLEstructs_cConnectionPars : public cObject{ // Tham - it is a derived class of cObject
    public:
    cBLEstructs_cConnectionPars(): Conn_Interval_Min(800)
                                   ,Conn_Interval_Max(800)
                                   ,Conn_Latency(0)
                                   ,Supervision_Timeout(500)
                                   ,Minimum_CE_Length(0)
                                   ,Maximum_CE_Length(0)
                                   ,Data_Channel_Map()
                                   ,ConnHandle(0){};
    /** @brief Destructor*/
    virtual ~cBLEstructs_cConnectionPars(){
        //~Advertising_Channel_Map();
    };

    public:
    //actual connection
    long int ConnHandle;
    long int Conn_Interval_Min;//in 1.25 ms units, value range 0x0006 to 0x0C80 - see p. 1270 of BLE spec v4.1
    long int Conn_Interval_Max;//in 1.25 ms units, value range 0x0006 to 0x0C80 - see p. 1270 of BLE spec v4.1
    long int Conn_Latency;//range: 0x0000 to 0x01F3 - see p. 1270 of BLE spec v4.1
    long int Supervision_Timeout;//in 10 ms units, range:0x000A to 0x0C80 - see p. 1270 of BLE spec v4.1
    long int Minimum_CE_Length;//Range: 0x0000 – 0xFFFF - see p. 1271 of BLE spec v4.1
    long int Maximum_CE_Length;//Range: 0x0000 – 0xFFFF - see p. 1271 of BLE spec v4.1
    cBLEstructs_ChannelMap  Data_Channel_Map;//see p 2544 of spec. v 4.1


    //public
    virtual void print(){
        EV  <<"Printing connection parameters:"<< endl;
        EV  <<"Conn_Interval_Min=" << Conn_Interval_Min << "       Note: in 1.25 ms units, value range 0x0006 to 0x0C80" << endl;
        EV  <<"Conn_Interval_Max=" << Conn_Interval_Max << "       Note: in 1.25 ms units, value range 0x0006 to 0x0C80" << endl;
        EV  <<"Conn_Latency=" << Conn_Latency << "       Note: range: 0x0000 to 0x01F3" << endl;
        EV  <<"Supervision_Timeout=" << Conn_Interval_Max << "       Note: in 10 ms units, range:0x000A to 0x0C80"<< endl;
        EV  <<"Minimum_CE_Length=" << Minimum_CE_Length << "       Note: range: 0x0000 – 0xFFFF" << endl;
        EV  <<"Maximum_CE_Length=" << Maximum_CE_Length << "       Note: range: 0x0000 – 0xFFFF" << endl;
        Data_Channel_Map.print();
    }

    virtual bool check(){
        if((Conn_Interval_Min<6)||(Conn_Interval_Min>0xC80))return false;
        if((Conn_Interval_Max<6)||(Conn_Interval_Max>0xC80))return false;
        if(Conn_Interval_Max<Conn_Interval_Min)return false;
        if((Conn_Latency<0)||(Conn_Latency>0x1F3))return false;
        if((Supervision_Timeout<0xA)||(Conn_Interval_Max>0xC80))return false;
        if((Minimum_CE_Length<0)||(Conn_Interval_Max>0xFFFF))return false;
        if((Maximum_CE_Length<0)||(Maximum_CE_Length>0xFFFF))return false;
        return true;
    }



};

//connection parameters (LL)
class MIXIM_API cBLEstructs_cConnectionInfo : public cObject{
    public:
        cBLEstructs_cConnectionInfo():
            transmitWindowOffset(0),transmitWindowSize(1),
            connectionHandle(0),accessAddr(),hopIncreasement(1),masterSCA(1), slaveSCA(1),
            Peer_Address_Type(cBLEstructs_defs::PEER_ADDR_PUBLIC_0),
            Peer_Address(),
            connInterval(80),connSlaveLatency(0), connSupervisionTimeout(80),
            Data_Channel_Map(),
            new_connInterval(80),new_connSlaveLatency(0),new_connSupervisionTimeout(80),
            new_Data_Channel_Map(),
            Instant_NewMap(-1),Instant_NewParameters(-1),
            lastunmappedChannel(0),unmappedChannel(0),
            timeLastAnchorReceived(0),timeLastSuccesfullEvent(0),numMissedEvents(0),
            moreData(true),connEventCounter(0),
            transmitSeqNum(0),nextExpectedSeqNum(0),
            delayedGeneration(true)//, connectionInititalized(false)
        {};

    /** @brief Destructor*/
    virtual ~cBLEstructs_cConnectionInfo(){

    };

    public:
    //parameters for initialization phase
    int transmitWindowOffset; //see p. 2540 of spec. v4.1
    int transmitWindowSize;//see p. 2540 of spec. v4.1

    //basic parameters
    //general
    long int connectionHandle;
    //long int accessAddr;//see p.2503 of spec v4.1
    unsigned int accessAddr; //tham
    //unsigned int accessAddr;//see p.2503 of spec v4.1
    int hopIncreasement;//see p.2510 of spec v4.1
    int masterSCA;//see p.2510 & p.2543 of spec v4.1
    int slaveSCA;//see p.2510 & p.2543 of spec v4.1
    cBLEstructs_defs::t_mac_PeerAddrType      Peer_Address_Type;
    LAddress::L2Type        Peer_Address;  //address of the device to be connected


    //currently used parameters
    int connInterval;// see p.2510 of spec v4.1 - in 1.25 ms units,
    int connSlaveLatency;// see p.2510 of spec v4.1
    int connSupervisionTimeout;// see p.2510 of spec v4.1
    cBLEstructs_ChannelMap  Data_Channel_Map;//see p 2544 of spec. v 4.1


    //new parameters (used for updating)
    int new_connInterval;// see p.2510 of spec v4.1 - in 1.25 ms units,
    int new_connSlaveLatency;// see p.2510 of spec v4.1
    int new_connSupervisionTimeout;// see p.2510 of spec v4.1
    cBLEstructs_ChannelMap  new_Data_Channel_Map;//see p 2544 of spec. v 4.1

    //
    long int Instant_NewMap;//see p 2552 of spec. v 4.1
    long int Instant_NewParameters;//see p 2550 of spec. v 4.1


    //variables
    int lastunmappedChannel;//see p 2544 of spec. v 4.1
    int unmappedChannel;//see p 2544 of spec. v 4.1
    simtime_t timeLastAnchorReceived;//see p. 2543 of spec. v 4.1
    simtime_t timeLastSuccesfullEvent;
    int numMissedEvents;
    //more data on the other end - rewritten with every received packet
    bool moreData;
    int connEventCounter;


    //packet contents
    bool transmitSeqNum;//see p 2545 of spec. v 4.1
    bool nextExpectedSeqNum;//see p 2545 of spec. v 4.1

    //service
    //delayedGeneration - means that we might have made a new packet, but we have not had data at a time
    bool delayedGeneration;


    //that's how I understand the equation on p. 2543 of spec. v 4.1
    simtime_t getWindowWidening(simtime_t TimeReference){
        simtime_t Value;
        /*if((masterSCA==0)&&(slaveSCA==0)){
            opp_error("Although the standard enables to have both masterSCA and slaveSCA equal to 0, but the listening time for the receiver which is calculated based on those (see page 253 of spec v4.1) appears to be also 0. This does not make any sense! Thus you see this error. ");
        }*/
        Value= ((masterSCA+slaveSCA)/1000000.0)*(TimeReference-timeLastAnchorReceived);
        return Value;
    }

    //
    int getDataChannel(){//see p. 2544 of spec. v 4.1
        int iunmappedChannel;
        int imappedChannel;
        int remappingIndex;
        //unmapped channel
        iunmappedChannel=lastunmappedChannel+hopIncreasement;
        //Data_Channel_Map.print();
        while(iunmappedChannel>36){
            iunmappedChannel=iunmappedChannel-37;
        }
        EV<< "mapDataChannel: lastunmappedChannel="<< lastunmappedChannel<< " hopIncreasement=" << hopIncreasement << " resultingChannel=" << iunmappedChannel <<endl;
        if(Data_Channel_Map.ChannelTbl[iunmappedChannel]==true){
            //EV<< "mapDataChannel: channel is ok. returning it. "<<endl;
            unmappedChannel=iunmappedChannel;
            return iunmappedChannel;
        }
        EV<< "mapDataChannel: channel is taken out of use. remapping channel. "<<endl;
        remappingIndex=iunmappedChannel;
        while(remappingIndex>Data_Channel_Map.TotalChNumber){
            remappingIndex=remappingIndex-Data_Channel_Map.TotalChNumber;
        }
        EV<< "mapDataChannel: remappingIndex="<< remappingIndex << " TotalChNumber=" << Data_Channel_Map.TotalChNumber << endl;
        imappedChannel=0;
        if(remappingIndex==Data_Channel_Map.TotalChNumber){
            while(Data_Channel_Map.ChannelTbl[imappedChannel]==false){
                imappedChannel++;
            }
        }
        else{
            while(remappingIndex!=0){
                if(Data_Channel_Map.ChannelTbl[imappedChannel]==true)remappingIndex--;
                imappedChannel++;
            }
        }
        EV<< "mapDataChannel: resulting channel index="<< imappedChannel <<endl;
        unmappedChannel=iunmappedChannel;
        return imappedChannel;
    }

    //returns instance for changing the parameters (see p. 2550 of Spec v4.1)
    virtual long int getInstance(void){
        //according to the spec, "The master should allow a minimum of 6 connection events that the slave will be listening for before the instant occurs." (see p. 2549 of Spec v4.1)
        long int Delay=6*(connSlaveLatency+1); //that's how I understand this
        long int Instance=connEventCounter+Delay;
        if (Instance>0xFFFF) Instance=Instance-65535;
        return Instance;
    }

    //returns interval based on min and max values
    virtual long int getInterval(int Conn_Interval_Min, int Conn_Interval_Max){
        long int Interval;
        Interval=round((Conn_Interval_Min+Conn_Interval_Max)/2);
        return Interval;
    }

    virtual void increaseConnEventCounter(void){
        connEventCounter++;
        if(connEventCounter>0xFFFF)connEventCounter=0;
    }



    //public
    virtual void print(){
        EV  <<"Printing connection parameters:"<< endl;
        EV  <<"connInterval=" << connInterval << endl;
        EV  <<"connSlaveLatency=" << connSlaveLatency << endl;
        EV  <<"connSupervisionTimeout=" << connSupervisionTimeout << endl;
        EV  <<"hopIncreasement=" << hopIncreasement << endl;
        EV  <<"masterSCA=" << masterSCA << endl;
        EV  <<"slaveSCA=" << slaveSCA << endl;
        EV  <<"timeLastAnchorReceived=" << timeLastAnchorReceived << endl;
    }

    virtual bool check(){
        return true;
    }
};



#endif /**/
