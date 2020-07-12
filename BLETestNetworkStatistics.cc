/* -*- mode:c++ -*- ********************************************************
 * file:        BLETestNetworkStatistics.cc
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.


 ***************************************************************************
 * BLE data and parameters structures
 **************************************************************************/
#include "BLETestNetworkStatistics.h"
#include "FindModule.h"

Define_Module(BLETestNetworkStatistics);

void BLETestNetworkStatistics::initialize(void){
    network = FindModule<>::findNetwork(this);
    cleanAll();
}


void BLETestNetworkStatistics::logAll(void){
    if(pauseWriting==false){
    }
}

void BLETestNetworkStatistics::finish() {

}



