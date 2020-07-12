/* -*- mode:c++ -*- ********************************************************
 * file:        Stat_BLEConnection.c
 *
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
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


