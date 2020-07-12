/* -*- mode:c++ -*- ********************************************************
 * file:        BLETestNetworkStatistics.h
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
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

