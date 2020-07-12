/* -*- mode:c++ -*- ********************************************************
 * file:        Stat_BLEConnection.h
 *
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 ***************************************************************************
 **************************************************************************/
#ifndef STATISTICS_BLECONNECTION_H
#define STATISTICS_BLECONNECTION_H

#include <omnetpp.h>
#include "MiXiMDefs.h"
#include "BaseModule.h"

class MIXIM_API Stat_BLEConnection : public BaseModule
{
private:
    /** @brief Copy constructor is not allowed.
     */
    Stat_BLEConnection(const Stat_BLEConnection&);
    /** @brief Assignment operator is not allowed.
     */
    Stat_BLEConnection& operator=(const Stat_BLEConnection&);

    bool LogActive;
    int NumEstablished;
    int NumDroped;
    int NumNoRSPTimer;
public:
    Stat_BLEConnection() : BaseModule()
    {}
    //Variables

    cOutVector NodeNoRSPTimer;//

    virtual void initialize(int stage);
    virtual void finish();
    virtual void ConnEstablished(void);
    virtual void ConnDroped(void);
    virtual void NoRSPTimer(void);
};


#endif

