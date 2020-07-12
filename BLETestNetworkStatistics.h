/* -*- mode:c++ -*- ********************************************************
 * file:        BLETestNetworkStatistics.h
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
#ifndef BLETESTNWK_STATISTICS_H
#define BLETESTNWK_STATISTICS_H

#include <omnetpp.h>

#include "MiXiMDefs.h"

class BLETestNetworkStatistics : public cSimpleModule {
private:

public:
    BLETestNetworkStatistics() : cSimpleModule()
    {}
    //Variables
    bool pauseWriting;


    long int NumMasters;
    long int NumSlaves;


    long int NumBytes;


    cModule* network;

    virtual void initialize(void);
    virtual void finish();

    virtual void cleanAll(void){
        NumBytes=0;
    };

    virtual void logAll(void);

    virtual void set_pauseWriting(bool s){
        pauseWriting = s;
    };


   virtual void change_NumBytes(long int s){
       NumBytes = NumBytes + s;
       EV << " BLETestNetworkStatistics on change_NumBytes. New NumBytes=" <<  NumBytes << " Change: " << s << endl;
       if(NumBytes==0){
           bool StopCondition=par("StopSimulationOnNoData");
           if(StopCondition){
               recordScalar("End Time", simTime(), "s");
               endSimulation();
           }
       }
   };



};


#endif

