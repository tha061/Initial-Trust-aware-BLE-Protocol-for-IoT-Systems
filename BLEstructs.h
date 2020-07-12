/* -*- mode:c++ -*- ********************************************************
 * file:        BLEstructs.h
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 ***************************************************************************
 * BLE data and parameters structures
 **************************************************************************/
#ifndef BLESTRUCTS_H_
#define BLESTRUCTS_H_

#include "MiXiMDefs.h"
#include "SimpleAddress.h"

#define DEF_NUM_BLE_CHANNELS 40

class MIXIM_API BLEstructs : public cObject
{
protected:
    //int CmdType;

public:
    /** @brief Default constructor*/
    BLEstructs()//:
    ////CmdType(BLECMD_HCICtrl_UNDEFINED)
    {};

    /** @brief Destructor*/
    virtual ~BLEstructs(){};

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
        ,BLECMD_HCICtrl_LESetAdvertiseEnableCommand //+
        ,BLECMD_HCICtrl_LESetAdvertisingDataCommand //+
        ,BLECMD_HCICtrl_LESetAdvertisingParametersCommand //+
        ,BLECMD_HCICtrl_LESetRandomAddressCommand
        //C2
        ,BLECMD_HCICtrl_LEAdvertisingReportEvent
        ,BLECMD_HCICtrl_LEReceiverTestCommand
        ,BLECMD_HCICtrl_LESetScanEnableCommand
        ,BLECMD_HCICtrl_LESetScanParametersCommand
        //TODO: extend
    };

    enum t_mac_AdvType {//spec v4.1 p. 1247
        ADV_IND_0=0
        ,ADV_DIRECT_IND_HDC_1
        ,ADV_SCAN_IND_2
        ,ADV_NONCONN_IND_3
        ,ADV_DIRECT_IND_LDC_4
        ,SCAN_REQ  //Tham added 2018
        ,SCAN_RSP  //Tham added 2018
    };

    enum t_mac_OwnAddrType {//spec v4.1 p. 1247
        OWN_ADDR_PUBLIC_0=0
        ,OWN_ADDR_RANDOM_1
    };

    enum t_mac_DirectAddrType {//spec v4.1 p. 1248
        DIRECT_ADDR_PUBLIC_0=0
        ,DIRECT_ADDR_RANDOM_1
    };

    enum t_mac_AdvFilterPolicy {//spec v4.1 p. 1248
        ANY_0=0
        ,CNCTfrmANY_SCANfrmWL
        ,CNCTfrmWL_SCANfrmANY
        ,WHITE_LIST
    };

    enum t_DataPDU_LLID {//spec v4.1 p. 2512
        LLID_Reserved_0=0
        ,LLID_Data_frag_1
        ,LLID_Data_start_2
        ,LLID_Data_ctrl_3
    };

    enum t_DataPDU_Cmd_Opcode {//spec v4.1 p. 2513
        LL_CONNECTION_UPDATE_REQ=0
        ,LL_CHANNEL_MAP_REQ
        ,LL_TERMINATE_IND
        //TO DO: a hell lot of other commands
    };

    typedef struct tChannelsMap{
        int TotalChNumber;
        bool ChannelTbl[DEF_NUM_BLE_CHANNELS];
        //set default values
        //tChannelsMap(): TotalChNumber(DEF_NUM_BLE_CHANNELS), ChannelTbl({1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1}){}
        tChannelsMap(): TotalChNumber(DEF_NUM_BLE_CHANNELS), ChannelTbl(){}
    }tChannelsMap;


    //Data structures
    typedef struct tAdvertisePars{//spec v4.1 p. 1246
        int                     Advertising_Interval_Min; //in slots, to get actual time multiply by 0.625 ms
        int                     Advertising_Interval_Max; //in slots, to get actual time multiply by 0.625 ms
        t_mac_AdvType           Adv_Type;
        t_mac_OwnAddrType       Own_Address_Type;
        t_mac_DirectAddrType    Direct_Address_Type;
        LAddress::L2Type        Direct_Address;
        tChannelsMap            Advertising_Channel_Map;
        t_mac_AdvFilterPolicy   Advertising_Filter_Policy;
        cMessage        *       AdvMessage;
        int                     AdvMessageLgth;
        bool                    AdvEnabled;
        //set default values
        tAdvertisePars(): Advertising_Interval_Min(0x800),Advertising_Interval_Max(0x800),Adv_Type(ADV_IND_0),Own_Address_Type(OWN_ADDR_PUBLIC_0),Direct_Address_Type(DIRECT_ADDR_PUBLIC_0),Advertising_Channel_Map(),Advertising_Filter_Policy(ANY_0){}
    }tAdvertisePars;

    typedef struct tScanPars{
    }tScanPars;

    typedef struct tAdvertiseConstPars{//different pars of the protocol, which can not be changed by the upper layers
        simtime_t ADV_IND_PDU_period;
        simtime_t ADV_DIRECT_IND_LDC_period;
        simtime_t ADV_DIRECT_IND_HDC_period;
        simtime_t ADV_SCAN_IND_period;
        simtime_t ADV_NONCONN_IND_period;
        simtime_t ADV_ListenTime;
    }tAdvertiseConstPars;//TODO: remove

    typedef struct tConnPars{// see p. 1269 of BLE spec v4.1

        long int Conn_Interval_Min;//in 1.25 ms units, value range 0x0006 to 0x0C80 - see p. 1270 of BLE spec v4.1
        long int Conn_Interval_Max;//in 1.25 ms units, value range 0x0006 to 0x0C80 - see p. 1270 of BLE spec v4.1
        long int Conn_Latency;//range: 0x0000 to 0x01F3 - see p. 1270 of BLE spec v4.1
        long int Supervision_Timeout;//in 10 ms units, range:0x000A to 0x0C80 - see p. 1270 of BLE spec v4.1
        long int Minimum_CE_Length;//Range: 0x0000 – 0xFFFF - see p. 1271 of BLE spec v4.1
        long int Maximum_CE_Length;//Range: 0x0000 – 0xFFFF - see p. 1271 of BLE spec v4.1

        tConnPars(): Conn_Interval_Min(800), Conn_Interval_Max(800), Conn_Latency(0), Supervision_Timeout(500), Minimum_CE_Length(0), Maximum_CE_Length(0){}
    }tConnPars;//connection parameters

    virtual bool Check_ConnPars(tConnPars* pars) const {
        if((pars->Conn_Interval_Min<6)||(pars->Conn_Interval_Min>0xC80))return false;
        if((pars->Conn_Interval_Max<6)||(pars->Conn_Interval_Max>0xC80))return false;
        if((pars->Conn_Latency<0)||(pars->Conn_Latency>0x1F3))return false;
        if((pars->Supervision_Timeout<0xA)||(pars->Conn_Interval_Max>0xC80))return false;
        if((pars->Minimum_CE_Length<0)||(pars->Conn_Interval_Max>0xFFFF))return false;
        if((pars->Maximum_CE_Length<0)||(pars->Maximum_CE_Length>0xFFFF))return false;
        return true;
    }

    virtual void Print_ConnPars(tConnPars* pars) const {
        EV  <<"Printing connection parameters:"<< endl;
        EV  <<"Conn_Interval_Min=" << pars->Conn_Interval_Min << endl;
        EV  <<"Conn_Interval_Max=" << pars->Conn_Interval_Max << endl;
        EV  <<"Conn_Latency=" << pars->Conn_Latency << endl;
        EV  <<"Supervision_Timeout=" << pars->Conn_Interval_Max << endl;
        EV  <<"Minimum_CE_Length=" << pars->Minimum_CE_Length << endl;
        EV  <<"Maximum_CE_Length=" << pars->Maximum_CE_Length << endl;
    }

};


#endif /* IPACNETWCONTROLINFO_H_ */
