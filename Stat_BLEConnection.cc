/* -*- mode:c++ -*- ********************************************************
 * file:        Stat_BLEConnection.c
 *
// Created on:  21.2.2014
 * Updated on:  21.2.2014
 * author:      Konstantin Mikhaylov
 *
 * copyright:   (C) 2014 Konstantin Mikhaylov
 *
 *              This program is free software; you can redistribute it
 *              and/or modify it under the terms of the GNU General Public
 *              License as published by the Free Software Foundation; either
 *              version 2 of the License, or (at your option) any later
 *              version.
 *              For further information see file COPYING
 *              in the top level directory
 ***************************************************************************
 **************************************************************************/

#include "Stat_BLEConnection.h"
#include "FindModule.h"
#include "ccomponent.h"
//#include "BatteryAccess.h"

Define_Module(Stat_BLEConnection);


void Stat_BLEConnection::initialize(int stage){
    if(stage == 0){
        LogActive=par("Log");
        EV << "Stat_BLEConnection: LogActive=" << LogActive << endl;
        NumEstablished=0;
        NumDroped=0;
        NumNoRSPTimer=0;
        NodeNoRSPTimer.setName("NoRSP_timer_fired");
    }
    else if(stage == 1){

    }
}


void Stat_BLEConnection::ConnEstablished(void){
    NumEstablished=NumEstablished+1;
}

void Stat_BLEConnection::ConnDroped(void){
    NumDroped=NumDroped+1;
}

void Stat_BLEConnection::NoRSPTimer(void){
    NumNoRSPTimer=NumNoRSPTimer+1;
    NodeNoRSPTimer.record(NumNoRSPTimer);

}

void Stat_BLEConnection::finish() {
    recordScalar("Connections established", NumEstablished, "");
    recordScalar("Connections droped", NumDroped, "");
}


