/* -*- mode:c++ -*- ********************************************************
 * file:        BLECmds_HCItoCtrl.h
 *
 * Created on:  15.02.2014
 * Updated on:  15.02.2014
 * author:      Konstantin Mikhaylov
 *
 * copyright:   (C) Konstantin Mikhaylov
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 * BLE HCI to Controller commands
 **************************************************************************/
#ifndef BLECMMDS_HCITOCTRL_H_
#define BLECMMDS_HCITOCTRL_H_

#include "MiXiMDefs.h"
#include "SimpleAddress.h"
#include "BLEstructs.h"

class MIXIM_API BLECmds_HCItoCtrl : public cObject
{
protected:
    int CmdType;
    BLEstructs::tAdvertisePars *ptr_CmdAdvertisePars;

public:
    /** @brief Default constructor*/
    BLECmds_HCItoCtrl():
        CmdType(0)
        ,ptr_CmdAdvertisePars()

    {};

    /** @brief Destructor*/
    virtual ~BLECmds_HCItoCtrl(){};

    /** @brief Get method*/
    virtual int get_CmdType(void){
        return CmdType;
    };

    /** @brief Set method*/
    virtual void set_CmdType(int Type){
        CmdType=Type;
    };

    /** @brief Set method*/
    virtual void set_LESetAdvertisingParametersCommand_Pars(BLEstructs::tAdvertisePars *Pars){
        ptr_CmdAdvertisePars=Pars;
    };

    static cObject* generate_LESetAdvertisingParametersCommand(BLEstructs::tAdvertisePars *Pars){
        BLECmds_HCItoCtrl *const Cmd = new BLECmds_HCItoCtrl();
        Cmd->set_CmdType(BLEstructs::BLECMD_HCICtrl_LESetAdvertisingParametersCommand);
        Cmd->set_LESetAdvertisingParametersCommand_Pars(Pars);
        return Cmd;
    }

public:





};


#endif /* IPACNETWCONTROLINFO_H_ */
