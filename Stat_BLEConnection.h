/* -*- mode:c++ -*- ********************************************************
 * file:        Stat_BLEConnection.h
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

