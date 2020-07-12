/* -*- mode:c++ -*- ********************************************************
 * file:        BLE_NwkToMac.h
 *
* copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 ***************************************************************************
 * BLE HCI to Controller commands
 **************************************************************************/
#ifndef BLENWK_TO_MAC_H_
#define BLENWK_TO_MAC_H_

#include "MiXiMDefs.h"
#include "SimpleAddress.h"
#include "BLEdefs.h"


class MIXIM_API BLE_NwkToMac : public cObject
{
protected:
    int CmdType;
    cBLEstructs_cAdvertisePars *ptr_CmdAdvertisePars;
    cBLEstructs_cConnectionPars *ptr_CmdConnectionPars; //
    cBLEstructs_cScanningPars *ptr_CmdScanningPars; //
    cBLEstructs_Error *ptr_CmdError;

    long int LLBufferPayloadUpdate; //a simple workaround for changing the number of databytes to be TXd in LL buffer

public:
    /** @brief Default constructor*/
    BLE_NwkToMac():
        CmdType(0)
        ,ptr_CmdAdvertisePars()
        ,ptr_CmdConnectionPars()
        ,ptr_CmdScanningPars()
        ,LLBufferPayloadUpdate() //a simple workaround for changing the number of databytes to be TXd in LL buffer

    {};

    /** @brief Destructor*/
    virtual ~BLE_NwkToMac(){};

    /** @brief Get method*/
    virtual int get_CmdType(void){
        return CmdType;
    };

    /** @brief Set method*/
    virtual void set_CmdType(int Type){
        CmdType=Type;
    };

//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual cBLEstructs_cAdvertisePars* get_AdvertisingParsPtr(void){
        return ptr_CmdAdvertisePars;
    };

    /** @brief Set method*/
    virtual void set_AdvertisingParsPtr(cBLEstructs_cAdvertisePars *Pars){
        ptr_CmdAdvertisePars=Pars;
    };

//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual cBLEstructs_cConnectionPars* get_ConnectionParsPtr(void){
        return ptr_CmdConnectionPars;
    };

    /** @brief Set method*/
    virtual void set_ConnectionParsPtr(cBLEstructs_cConnectionPars *Pars){
        ptr_CmdConnectionPars=Pars;
    };

//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual cBLEstructs_cScanningPars* get_ScanningParsPtr(void){
        return ptr_CmdScanningPars;
    };

    /** @brief Set method*/
    virtual void set_ScanningParsPtr(cBLEstructs_cScanningPars *Pars){
        ptr_CmdScanningPars=Pars;
    };

//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual cBLEstructs_Error* get_ErrorPtr(void){
        return ptr_CmdError;
    };

    /** @brief Set method*/
    virtual void set_ErrorPtr(cBLEstructs_Error *Pars){
        ptr_CmdError=Pars;
    };


//////////////////////////////////////////////////////////////////////////////////
    /** @brief Get method*/
    virtual long int get_BufferPayloadUpdate(void){
        return LLBufferPayloadUpdate;
    };

    /** @brief Set method*/
    virtual void set_BufferPayloadUpdate(long int new_BufferPayloadUpdate){
        LLBufferPayloadUpdate=new_BufferPayloadUpdate;
    };

//////////////////////////////////////////////////////////////////////////////////
// ADVERTISING AND AROUND IT
//////////////////////////////////////////////////////////////////////////////////
    static cObject* generate_LESetAdvertisingParametersCommand(cBLEstructs_cAdvertisePars *Pars){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand);
        Cmd->set_AdvertisingParsPtr(Pars);
        return Cmd;
    }

    static cObject* generate_LESetAdvertisingDataCommand(cBLEstructs_cAdvertisePars *Pars){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertisingDataCommand);
        Cmd->set_AdvertisingParsPtr(Pars);
        return Cmd;
    }

    static cObject* generate_LESetAdvertiseEnableCommand(cBLEstructs_cAdvertisePars *Pars){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LESetAdvertiseEnableCommand);
        Cmd->set_AdvertisingParsPtr(Pars);
        return Cmd;
    }

//////////////////////////////////////////////////////////////////////////////////
// CONNECTION AND INITIATION
//////////////////////////////////////////////////////////////////////////////////
    static cObject* generate_LESetHostChannelClassificationCommand(cBLEstructs_cConnectionPars *ParsCon){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Set_Host_Channel_Classification_Command);
        Cmd->set_ConnectionParsPtr(ParsCon);
        return Cmd;
    }

    static cObject* generate_LECreateConnectionCommand(cBLEstructs_cScanningPars *ParsScan, cBLEstructs_cConnectionPars *ParsCon){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Command);
        Cmd->set_ScanningParsPtr(ParsScan);
        Cmd->set_ConnectionParsPtr(ParsCon);
        return Cmd;
    }

    static cObject* generate_LECreateConnectionCancelCommand(void){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Create_Connection_Cancel_Command);
        return Cmd;
    }

    static cObject* generate_LEConnectionUpdateCommand(cBLEstructs_cConnectionPars *ParsCon){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Update_Command);
        Cmd->set_ConnectionParsPtr(ParsCon);
        return Cmd;
    }

    static cObject* generate_DisconnectCommand(cBLEstructs_Error *ParsErr){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::BLECMD_HCICtrl_Disconnect_Command);
        Cmd->set_ErrorPtr(ParsErr);
        return Cmd;
    }


//////////////////////////////////////////////////////////////////////////////////
// OTHER - I.E., NON STANDARD!
//////////////////////////////////////////////////////////////////////////////////
    static cObject* generate_BufferPayloadUpdate(long int PayloadChange){
        BLE_NwkToMac *const Cmd = new BLE_NwkToMac();
        Cmd->set_CmdType(cBLEstructs_defs::Simulation_LLBufferPayloadUpdate_Command);
        Cmd->set_BufferPayloadUpdate(PayloadChange);
        return Cmd;
    }




public:





};


#endif /* BLENWK_TO_MAC_H_ */
