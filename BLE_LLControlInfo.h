/* -*- mode:c++ -*- *******************************************************
 * file:        BLE_LLControlInfo.h
 *
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 **************************************************************************
 * part of:     framework implementation developed by tkn
 * description: - control info to pass the netw addresses between the
 *                network and application layer
 **************************************************************************/

#ifndef BLELLCONTROLINFO_H
#define BLELLCONTROLINFO_H

#include <omnetpp.h>

#include "MiXiMDefs.h"


class MIXIM_API BLE_LL_ControlInfo : public cObject
{
  protected:

    long int handle;

  public:
    /** @brief Default constructor*/
    BLE_LL_ControlInfo(const long int PktHandle) : handle(PktHandle) {}
    /** @brief Destructor*/
    virtual ~BLE_LL_ControlInfo() {}

    /** @brief Getter method*/
    virtual const long int getHandle() const {
        return handle;
    }

    /** @brief Setter method*/
    virtual void setHandle(const long int PktHandle){
        handle = PktHandle;
    }


    static cObject* setControlInfo(cMessage *const pMsg, const long int PktHandle) {
        BLE_LL_ControlInfo *const cCtrlInfo = new BLE_LL_ControlInfo(PktHandle);
    	pMsg->setControlInfo(cCtrlInfo);
    	return cCtrlInfo;
    }

    static const long int getAddressFromControlInfo(cObject *const pCtrlInfo) {
        BLE_LL_ControlInfo *const cCtrlInfo = dynamic_cast<BLE_LL_ControlInfo *const>(pCtrlInfo);

    	if (cCtrlInfo)
    		return cCtrlInfo->getHandle();
    	return 0;
    }
};


#endif
