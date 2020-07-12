/* -*- mode:c++ -*- ********************************************************
 * file:        BLE_BasicNwk.h
 * copyright:   (C) 2014 CWC, University of Oulu, Finland
 * Adding initial trust-aware implementation by Tham Nguyen, 2018.
 ***************************************************************************
 *
 **************************************************************************/

#ifndef BLE_BasicNwk_h
#define BLE_BasicNwk_h

#include <map>
#include <omnetpp.h>

#include "BLEdefs.h"
#include "BLEstructs.h"
#include "BLECmds_HCItoCtrl.h"

#include "MiXiMDefs.h"
#include "BaseNwkLayer_TXPwrCtrl.h"
#include "SimpleAddress.h"

//#include "NetworkStatisticsFerry.h"
//EXTENDED STATS!
//#include "NetworkStatisticsExtended.h"
//#include "ExtNwkStatsTable.h"
//EXTENDED STATS!
//#include "NwkMsgFIFO.h"
//#include "Modular_NwkPkt_m.h"

class SimTracer;
class Modular_NwkPkt;
#include "BLETestNetworkStatistics.h"
/**

 **/
class MIXIM_API BLE_BasicNwk : public BaseNwkLayer_TXPwrCtrl
{
private:
    /** @brief Copy constructor is not allowed.
     */
    BLE_BasicNwk(const BLE_BasicNwk&);
    /** @brief Assignment operator is not allowed.
     */
    BLE_BasicNwk& operator=(const BLE_BasicNwk&);

public:
    BLE_BasicNwk()
        : BaseNwkLayer_TXPwrCtrl()

        , TESTTimer(NULL)

        , headerLength(0)
        , useSimTracer(false)
        , tracer(NULL)
        , trace(false), stats(false), debug(false)
        , Sensititvity(-90)
        , PowerControlBudget(10)
        , MaxTxPower(0)
    {}


    typedef enum NodeStatus {
        NodeStatus_Unknown=0
    } NodeStatus;

    typedef enum NodeMode {
        //Unknown
        NodeMode_Unknown=0,
    } NodeMode;

    /** @brief Initialization of the module and some variables*/
    virtual void initialize(int);
    virtual void finish();
    virtual ~BLE_BasicNwk();
    int Node_role;

protected:
    int headerLength;
    bool useSimTracer;
    double energyLogPeriod;
    double nwkBatteryMinVoltage;
    double nwkBatteryMaxVoltage;
    long int initData;

    SimTracer *tracer;

    //Power Control
    double Sensititvity;
    double PowerControlBudget;
    double MaxTxPower;
    double par_GENERAL_LogEnergyPeriod;


    cOutVector energy;
    cOutVector voltage;

    bool trace, stats, debug;
    NodeStatus nodeStatus;
    NodeMode nodeMode;

    cBLEstructs_cAdvertisePars *currentCmdAdvertisePars;
    cBLEstructs_cConnectionPars *currentCmdConnectionPars;
    cBLEstructs_cScanningPars *currentCmdScanningPars;
    cBLEstructs_Error *currentCmdError;

    cMessage * TESTTimer;//for testing
    cMessage * StartNode;//for testing
    cMessage * GenerateNewData;//

    BLETestNetworkStatistics* BLEnwkStats;

    /** @brief Handle messages from upper layer */
    virtual void handleUpperMsg(cMessage* msg);

    /** @brief Handle messages from lower layer */
    virtual void handleLowerMsg(cMessage* msg);

    /** @brief Handle self messages */
    virtual void handleSelfMsg(cMessage* msg);

    /** @brief Handle control messages from lower layer */
    virtual void handleLowerControl(cMessage *msg);

    /** @brief Handle control messages from upper layer */
    virtual void handleUpperControl(cMessage *msg);


    virtual void startNode(void);

    /** @brief Decapsulate a message */
    cMessage* decapsMsg(Modular_NwkPkt *msg);

};

#endif
