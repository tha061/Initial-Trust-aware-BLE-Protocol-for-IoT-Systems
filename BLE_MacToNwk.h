/* -*- mode:c++ -*- ********************************************************
 * file:        BLE_MacToNwk.h
 *
 * Created on:  08.03.2014
 * Updated on:  08.03.2014
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
#ifndef BLEMAC_TO_NWK_H_
#define BLEMAC_TO_NWK_H_

#include "MiXiMDefs.h"
#include "SimpleAddress.h"
#include "BLEdefs.h"


class MIXIM_API BLE_MacToNwk : public cObject
{
protected:
    int EventType;

    cBLEstructs_cEventsData *ptr_EventData;
    long int LLBufferPayload;

public:
    /** @brief Default constructor*/
    BLE_MacToNwk():
        EventType(0)
        ,LLBufferPayload(0)//non-standard, a simple workaround for getting the number of databytes in LL TX buffer
    {};

    /** @brief Destructor*/
    virtual ~BLE_MacToNwk(){};


//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual int get_EventType(void){
        return EventType;
    };

    /** @brief Set method*/
    virtual void set_EventType(int Type){
        EventType=Type;
    };

//////////////////////////////////////////////////////////////////////////////////

    /** @brief Get method*/
    virtual cBLEstructs_cEventsData* get_EventsParsPtr(void){
        return ptr_EventData;
    };

    /** @brief Set method*/
    virtual void set_EventsParsPtr(cBLEstructs_cEventsData *Pars){
        ptr_EventData=Pars;
    };

//////////////////////////////////////////////////////////////////////////////////
    /** @brief Get method*/
    virtual long int get_BufferPayload(void){
        return LLBufferPayload;
    };

    /** @brief Set method*/
    virtual void set_BufferPayload(long int current_BufferPayload){
        LLBufferPayload=current_BufferPayload;
    };

//////////////////////////////////////////////////////////////////////////////////

    static cObject* generate_LEConnectionCompleteEvent(cBLEstructs_cEventsData *EventData){
        BLE_MacToNwk *const Event = new BLE_MacToNwk();
        Event->set_EventType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Complete_Event);
        Event->set_EventsParsPtr(EventData);
        return Event;
    };

    static cObject* generate_LEConnectionUpdateCompleteEvent(cBLEstructs_cEventsData *EventData){
        BLE_MacToNwk *const Event = new BLE_MacToNwk();
        Event->set_EventType(cBLEstructs_defs::BLECMD_HCICtrl_LE_Connection_Update_Complete_Event);
        Event->set_EventsParsPtr(EventData);
        return Event;
    };

    static cObject* generate_DisconnectionCompleteEvent(cBLEstructs_cEventsData *EventData){
        BLE_MacToNwk *const Event = new BLE_MacToNwk();
        Event->set_EventType(cBLEstructs_defs::BLECMD_HCICtrl_Disconnection_Complete_Event);
        Event->set_EventsParsPtr(EventData);
        return Event;
    };






//////////////////////////////////////////////////////////////////////////////////
// OTHER - I.E., NON STANDARD!
//////////////////////////////////////////////////////////////////////////////////
    static cObject* generate_BufferPayloadEvent(long int BufferPayload){
        BLE_MacToNwk *const Event = new BLE_MacToNwk();
        Event->set_EventType(cBLEstructs_defs::Simulation_LLBufferPayloadUpdate_Command);
        Event->set_BufferPayload(BufferPayload);
        return Event;
    };

};


#endif /* BLEMAC_TO_NWK_H_ */
