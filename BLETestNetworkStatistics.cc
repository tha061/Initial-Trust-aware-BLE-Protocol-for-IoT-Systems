/* -*- mode:c++ -*- ********************************************************
 * file:        BLETestNetworkStatistics.cc
 *
 * Created on:  
 * Updated on:  
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



