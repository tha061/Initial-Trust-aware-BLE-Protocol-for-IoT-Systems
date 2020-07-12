//
// Generated file, do not edit! Created by opp_msgc 4.4 from modules/messages/BLE_Adv_MacPkt.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "BLE_Adv_MacPkt_m.h"

USING_NAMESPACE

// Template rule which fires if a struct or class doesn't have operator<<
template<typename T>
std::ostream& operator<<(std::ostream& out,const T&) {return out;}

// Another default rule (prevents compiler from choosing base class' doPacking())
template<typename T>
void doPacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doPacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}

template<typename T>
void doUnpacking(cCommBuffer *, T& t) {
    throw cRuntimeError("Parsim error: no doUnpacking() function for type %s or its base class (check .msg and _m.cc/h files!)",opp_typename(typeid(t)));
}




Register_Class(BLE_Adv_MacPkt);

BLE_Adv_MacPkt::BLE_Adv_MacPkt(const char *name, int kind) : ::MacPkt(name,kind)
{
    this->AccessAddress_var = 0;
    this->Adv_PDU_type_var = 0;
    this->TxAdd_var = 0;
    this->RxAdd_var = 0;
    this->Length_var = 0;
    this->Adv_type_var = 0;
    this->AdvData_var = 0;
    this->ScanReqData_var = 0;
    this->ScanRspData_var = 0;
    this->temp_c_var = 0;
    this->index_r_var = 0;
    this->AA_var = 0;
    this->WinSize_var = 0;
    this->WinOffset_var = 0;
    this->Interval_var = 0;
    this->Latency_var = 0;
    this->Timeout_var = 0;
    this->MapCh0to7_var = 0;
    this->MapCh8to15_var = 0;
    this->MapCh16to23_var = 0;
    this->MapCh24to31_var = 0;
    this->MapCh32to39_var = 0;
    this->Hop_var = 0;
    this->SCA_var = 0;
}

BLE_Adv_MacPkt::BLE_Adv_MacPkt(const BLE_Adv_MacPkt& other) : ::MacPkt(other)
{
    copy(other);
}

BLE_Adv_MacPkt::~BLE_Adv_MacPkt()
{
}

BLE_Adv_MacPkt& BLE_Adv_MacPkt::operator=(const BLE_Adv_MacPkt& other)
{
    if (this==&other) return *this;
    ::MacPkt::operator=(other);
    copy(other);
    return *this;
}

void BLE_Adv_MacPkt::copy(const BLE_Adv_MacPkt& other)
{
    this->AccessAddress_var = other.AccessAddress_var;
    this->Adv_PDU_type_var = other.Adv_PDU_type_var;
    this->TxAdd_var = other.TxAdd_var;
    this->RxAdd_var = other.RxAdd_var;
    this->Length_var = other.Length_var;
    this->Adv_type_var = other.Adv_type_var;
    this->AdvA_var = other.AdvA_var;
    this->InitA_var = other.InitA_var;
    this->ScanA_var = other.ScanA_var;
    this->AdvData_var = other.AdvData_var;
    this->ScanReqData_var = other.ScanReqData_var;
    this->ScanRspData_var = other.ScanRspData_var;
    this->temp_c_var = other.temp_c_var;
    this->index_r_var = other.index_r_var;
    this->AA_var = other.AA_var;
    this->WinSize_var = other.WinSize_var;
    this->WinOffset_var = other.WinOffset_var;
    this->Interval_var = other.Interval_var;
    this->Latency_var = other.Latency_var;
    this->Timeout_var = other.Timeout_var;
    this->MapCh0to7_var = other.MapCh0to7_var;
    this->MapCh8to15_var = other.MapCh8to15_var;
    this->MapCh16to23_var = other.MapCh16to23_var;
    this->MapCh24to31_var = other.MapCh24to31_var;
    this->MapCh32to39_var = other.MapCh32to39_var;
    this->Hop_var = other.Hop_var;
    this->SCA_var = other.SCA_var;
}

void BLE_Adv_MacPkt::parsimPack(cCommBuffer *b)
{
    ::MacPkt::parsimPack(b);
    doPacking(b,this->AccessAddress_var);
    doPacking(b,this->Adv_PDU_type_var);
    doPacking(b,this->TxAdd_var);
    doPacking(b,this->RxAdd_var);
    doPacking(b,this->Length_var);
    doPacking(b,this->Adv_type_var);
    doPacking(b,this->AdvA_var);
    doPacking(b,this->InitA_var);
    doPacking(b,this->ScanA_var);
    doPacking(b,this->AdvData_var);
    doPacking(b,this->ScanReqData_var);
    doPacking(b,this->ScanRspData_var);
    doPacking(b,this->temp_c_var);
    doPacking(b,this->index_r_var);
    doPacking(b,this->AA_var);
    doPacking(b,this->WinSize_var);
    doPacking(b,this->WinOffset_var);
    doPacking(b,this->Interval_var);
    doPacking(b,this->Latency_var);
    doPacking(b,this->Timeout_var);
    doPacking(b,this->MapCh0to7_var);
    doPacking(b,this->MapCh8to15_var);
    doPacking(b,this->MapCh16to23_var);
    doPacking(b,this->MapCh24to31_var);
    doPacking(b,this->MapCh32to39_var);
    doPacking(b,this->Hop_var);
    doPacking(b,this->SCA_var);
}

void BLE_Adv_MacPkt::parsimUnpack(cCommBuffer *b)
{
    ::MacPkt::parsimUnpack(b);
    doUnpacking(b,this->AccessAddress_var);
    doUnpacking(b,this->Adv_PDU_type_var);
    doUnpacking(b,this->TxAdd_var);
    doUnpacking(b,this->RxAdd_var);
    doUnpacking(b,this->Length_var);
    doUnpacking(b,this->Adv_type_var);
    doUnpacking(b,this->AdvA_var);
    doUnpacking(b,this->InitA_var);
    doUnpacking(b,this->ScanA_var);
    doUnpacking(b,this->AdvData_var);
    doUnpacking(b,this->ScanReqData_var);
    doUnpacking(b,this->ScanRspData_var);
    doUnpacking(b,this->temp_c_var);
    doUnpacking(b,this->index_r_var);
    doUnpacking(b,this->AA_var);
    doUnpacking(b,this->WinSize_var);
    doUnpacking(b,this->WinOffset_var);
    doUnpacking(b,this->Interval_var);
    doUnpacking(b,this->Latency_var);
    doUnpacking(b,this->Timeout_var);
    doUnpacking(b,this->MapCh0to7_var);
    doUnpacking(b,this->MapCh8to15_var);
    doUnpacking(b,this->MapCh16to23_var);
    doUnpacking(b,this->MapCh24to31_var);
    doUnpacking(b,this->MapCh32to39_var);
    doUnpacking(b,this->Hop_var);
    doUnpacking(b,this->SCA_var);
}

unsigned int BLE_Adv_MacPkt::getAccessAddress() const
{
    return AccessAddress_var;
}

void BLE_Adv_MacPkt::setAccessAddress(unsigned int AccessAddress)
{
    this->AccessAddress_var = AccessAddress;
}

int BLE_Adv_MacPkt::getAdv_PDU_type() const
{
    return Adv_PDU_type_var;
}

void BLE_Adv_MacPkt::setAdv_PDU_type(int Adv_PDU_type)
{
    this->Adv_PDU_type_var = Adv_PDU_type;
}

bool BLE_Adv_MacPkt::getTxAdd() const
{
    return TxAdd_var;
}

void BLE_Adv_MacPkt::setTxAdd(bool TxAdd)
{
    this->TxAdd_var = TxAdd;
}

bool BLE_Adv_MacPkt::getRxAdd() const
{
    return RxAdd_var;
}

void BLE_Adv_MacPkt::setRxAdd(bool RxAdd)
{
    this->RxAdd_var = RxAdd;
}

int BLE_Adv_MacPkt::getLength() const
{
    return Length_var;
}

void BLE_Adv_MacPkt::setLength(int Length)
{
    this->Length_var = Length;
}

int BLE_Adv_MacPkt::getAdv_type() const
{
    return Adv_type_var;
}

void BLE_Adv_MacPkt::setAdv_type(int Adv_type)
{
    this->Adv_type_var = Adv_type;
}

LAddress::L2Type& BLE_Adv_MacPkt::getAdvA()
{
    return AdvA_var;
}

void BLE_Adv_MacPkt::setAdvA(const LAddress::L2Type& AdvA)
{
    this->AdvA_var = AdvA;
}

LAddress::L2Type& BLE_Adv_MacPkt::getInitA()
{
    return InitA_var;
}

void BLE_Adv_MacPkt::setInitA(const LAddress::L2Type& InitA)
{
    this->InitA_var = InitA;
}

LAddress::L2Type& BLE_Adv_MacPkt::getScanA()
{
    return ScanA_var;
}

void BLE_Adv_MacPkt::setScanA(const LAddress::L2Type& ScanA)
{
    this->ScanA_var = ScanA;
}

double BLE_Adv_MacPkt::getAdvData() const
{
    return AdvData_var;
}

void BLE_Adv_MacPkt::setAdvData(double AdvData)
{
    this->AdvData_var = AdvData;
}

double BLE_Adv_MacPkt::getScanReqData() const
{
    return ScanReqData_var;
}

void BLE_Adv_MacPkt::setScanReqData(double ScanReqData)
{
    this->ScanReqData_var = ScanReqData;
}

double BLE_Adv_MacPkt::getScanRspData() const
{
    return ScanRspData_var;
}

void BLE_Adv_MacPkt::setScanRspData(double ScanRspData)
{
    this->ScanRspData_var = ScanRspData;
}

int BLE_Adv_MacPkt::getTemp_c() const
{
    return temp_c_var;
}

void BLE_Adv_MacPkt::setTemp_c(int temp_c)
{
    this->temp_c_var = temp_c;
}

int BLE_Adv_MacPkt::getIndex_r() const
{
    return index_r_var;
}

void BLE_Adv_MacPkt::setIndex_r(int index_r)
{
    this->index_r_var = index_r;
}

unsigned int BLE_Adv_MacPkt::getAA() const
{
    return AA_var;
}

void BLE_Adv_MacPkt::setAA(unsigned int AA)
{
    this->AA_var = AA;
}

int BLE_Adv_MacPkt::getWinSize() const
{
    return WinSize_var;
}

void BLE_Adv_MacPkt::setWinSize(int WinSize)
{
    this->WinSize_var = WinSize;
}

int BLE_Adv_MacPkt::getWinOffset() const
{
    return WinOffset_var;
}

void BLE_Adv_MacPkt::setWinOffset(int WinOffset)
{
    this->WinOffset_var = WinOffset;
}

int BLE_Adv_MacPkt::getInterval() const
{
    return Interval_var;
}

void BLE_Adv_MacPkt::setInterval(int Interval)
{
    this->Interval_var = Interval;
}

int BLE_Adv_MacPkt::getLatency() const
{
    return Latency_var;
}

void BLE_Adv_MacPkt::setLatency(int Latency)
{
    this->Latency_var = Latency;
}

int BLE_Adv_MacPkt::getTimeout() const
{
    return Timeout_var;
}

void BLE_Adv_MacPkt::setTimeout(int Timeout)
{
    this->Timeout_var = Timeout;
}

int BLE_Adv_MacPkt::getMapCh0to7() const
{
    return MapCh0to7_var;
}

void BLE_Adv_MacPkt::setMapCh0to7(int MapCh0to7)
{
    this->MapCh0to7_var = MapCh0to7;
}

int BLE_Adv_MacPkt::getMapCh8to15() const
{
    return MapCh8to15_var;
}

void BLE_Adv_MacPkt::setMapCh8to15(int MapCh8to15)
{
    this->MapCh8to15_var = MapCh8to15;
}

int BLE_Adv_MacPkt::getMapCh16to23() const
{
    return MapCh16to23_var;
}

void BLE_Adv_MacPkt::setMapCh16to23(int MapCh16to23)
{
    this->MapCh16to23_var = MapCh16to23;
}

int BLE_Adv_MacPkt::getMapCh24to31() const
{
    return MapCh24to31_var;
}

void BLE_Adv_MacPkt::setMapCh24to31(int MapCh24to31)
{
    this->MapCh24to31_var = MapCh24to31;
}

int BLE_Adv_MacPkt::getMapCh32to39() const
{
    return MapCh32to39_var;
}

void BLE_Adv_MacPkt::setMapCh32to39(int MapCh32to39)
{
    this->MapCh32to39_var = MapCh32to39;
}

int BLE_Adv_MacPkt::getHop() const
{
    return Hop_var;
}

void BLE_Adv_MacPkt::setHop(int Hop)
{
    this->Hop_var = Hop;
}

int BLE_Adv_MacPkt::getSCA() const
{
    return SCA_var;
}

void BLE_Adv_MacPkt::setSCA(int SCA)
{
    this->SCA_var = SCA;
}

class BLE_Adv_MacPktDescriptor : public cClassDescriptor
{
  public:
    BLE_Adv_MacPktDescriptor();
    virtual ~BLE_Adv_MacPktDescriptor();

    virtual bool doesSupport(cObject *obj) const;
    virtual const char *getProperty(const char *propertyname) const;
    virtual int getFieldCount(void *object) const;
    virtual const char *getFieldName(void *object, int field) const;
    virtual int findField(void *object, const char *fieldName) const;
    virtual unsigned int getFieldTypeFlags(void *object, int field) const;
    virtual const char *getFieldTypeString(void *object, int field) const;
    virtual const char *getFieldProperty(void *object, int field, const char *propertyname) const;
    virtual int getArraySize(void *object, int field) const;

    virtual std::string getFieldAsString(void *object, int field, int i) const;
    virtual bool setFieldAsString(void *object, int field, int i, const char *value) const;

    virtual const char *getFieldStructName(void *object, int field) const;
    virtual void *getFieldStructPointer(void *object, int field, int i) const;
};

Register_ClassDescriptor(BLE_Adv_MacPktDescriptor);

BLE_Adv_MacPktDescriptor::BLE_Adv_MacPktDescriptor() : cClassDescriptor("BLE_Adv_MacPkt", "MacPkt")
{
}

BLE_Adv_MacPktDescriptor::~BLE_Adv_MacPktDescriptor()
{
}

bool BLE_Adv_MacPktDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<BLE_Adv_MacPkt *>(obj)!=NULL;
}

const char *BLE_Adv_MacPktDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int BLE_Adv_MacPktDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 27+basedesc->getFieldCount(object) : 27;
}

unsigned int BLE_Adv_MacPktDescriptor::getFieldTypeFlags(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeFlags(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISCOMPOUND,
        FD_ISCOMPOUND,
        FD_ISCOMPOUND,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
        FD_ISEDITABLE,
    };
    return (field>=0 && field<27) ? fieldTypeFlags[field] : 0;
}

const char *BLE_Adv_MacPktDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "AccessAddress",
        "Adv_PDU_type",
        "TxAdd",
        "RxAdd",
        "Length",
        "Adv_type",
        "AdvA",
        "InitA",
        "ScanA",
        "AdvData",
        "ScanReqData",
        "ScanRspData",
        "temp_c",
        "index_r",
        "AA",
        "WinSize",
        "WinOffset",
        "Interval",
        "Latency",
        "Timeout",
        "MapCh0to7",
        "MapCh8to15",
        "MapCh16to23",
        "MapCh24to31",
        "MapCh32to39",
        "Hop",
        "SCA",
    };
    return (field>=0 && field<27) ? fieldNames[field] : NULL;
}

int BLE_Adv_MacPktDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='A' && strcmp(fieldName, "AccessAddress")==0) return base+0;
    if (fieldName[0]=='A' && strcmp(fieldName, "Adv_PDU_type")==0) return base+1;
    if (fieldName[0]=='T' && strcmp(fieldName, "TxAdd")==0) return base+2;
    if (fieldName[0]=='R' && strcmp(fieldName, "RxAdd")==0) return base+3;
    if (fieldName[0]=='L' && strcmp(fieldName, "Length")==0) return base+4;
    if (fieldName[0]=='A' && strcmp(fieldName, "Adv_type")==0) return base+5;
    if (fieldName[0]=='A' && strcmp(fieldName, "AdvA")==0) return base+6;
    if (fieldName[0]=='I' && strcmp(fieldName, "InitA")==0) return base+7;
    if (fieldName[0]=='S' && strcmp(fieldName, "ScanA")==0) return base+8;
    if (fieldName[0]=='A' && strcmp(fieldName, "AdvData")==0) return base+9;
    if (fieldName[0]=='S' && strcmp(fieldName, "ScanReqData")==0) return base+10;
    if (fieldName[0]=='S' && strcmp(fieldName, "ScanRspData")==0) return base+11;
    if (fieldName[0]=='t' && strcmp(fieldName, "temp_c")==0) return base+12;
    if (fieldName[0]=='i' && strcmp(fieldName, "index_r")==0) return base+13;
    if (fieldName[0]=='A' && strcmp(fieldName, "AA")==0) return base+14;
    if (fieldName[0]=='W' && strcmp(fieldName, "WinSize")==0) return base+15;
    if (fieldName[0]=='W' && strcmp(fieldName, "WinOffset")==0) return base+16;
    if (fieldName[0]=='I' && strcmp(fieldName, "Interval")==0) return base+17;
    if (fieldName[0]=='L' && strcmp(fieldName, "Latency")==0) return base+18;
    if (fieldName[0]=='T' && strcmp(fieldName, "Timeout")==0) return base+19;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh0to7")==0) return base+20;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh8to15")==0) return base+21;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh16to23")==0) return base+22;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh24to31")==0) return base+23;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh32to39")==0) return base+24;
    if (fieldName[0]=='H' && strcmp(fieldName, "Hop")==0) return base+25;
    if (fieldName[0]=='S' && strcmp(fieldName, "SCA")==0) return base+26;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *BLE_Adv_MacPktDescriptor::getFieldTypeString(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldTypeString(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldTypeStrings[] = {
        "unsigned int",
        "int",
        "bool",
        "bool",
        "int",
        "int",
        "LAddress::L2Type",
        "LAddress::L2Type",
        "LAddress::L2Type",
        "double",
        "double",
        "double",
        "int",
        "int",
        "unsigned int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
        "int",
    };
    return (field>=0 && field<27) ? fieldTypeStrings[field] : NULL;
}

const char *BLE_Adv_MacPktDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldProperty(object, field, propertyname);
        field -= basedesc->getFieldCount(object);
    }
    switch (field) {
        default: return NULL;
    }
}

int BLE_Adv_MacPktDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Adv_MacPkt *pp = (BLE_Adv_MacPkt *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string BLE_Adv_MacPktDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Adv_MacPkt *pp = (BLE_Adv_MacPkt *)object; (void)pp;
    switch (field) {
        case 0: return ulong2string(pp->getAccessAddress());
        case 1: return long2string(pp->getAdv_PDU_type());
        case 2: return bool2string(pp->getTxAdd());
        case 3: return bool2string(pp->getRxAdd());
        case 4: return long2string(pp->getLength());
        case 5: return long2string(pp->getAdv_type());
        case 6: {std::stringstream out; out << pp->getAdvA(); return out.str();}
        case 7: {std::stringstream out; out << pp->getInitA(); return out.str();}
        case 8: {std::stringstream out; out << pp->getScanA(); return out.str();}
        case 9: return double2string(pp->getAdvData());
        case 10: return double2string(pp->getScanReqData());
        case 11: return double2string(pp->getScanRspData());
        case 12: return long2string(pp->getTemp_c());
        case 13: return long2string(pp->getIndex_r());
        case 14: return ulong2string(pp->getAA());
        case 15: return long2string(pp->getWinSize());
        case 16: return long2string(pp->getWinOffset());
        case 17: return long2string(pp->getInterval());
        case 18: return long2string(pp->getLatency());
        case 19: return long2string(pp->getTimeout());
        case 20: return long2string(pp->getMapCh0to7());
        case 21: return long2string(pp->getMapCh8to15());
        case 22: return long2string(pp->getMapCh16to23());
        case 23: return long2string(pp->getMapCh24to31());
        case 24: return long2string(pp->getMapCh32to39());
        case 25: return long2string(pp->getHop());
        case 26: return long2string(pp->getSCA());
        default: return "";
    }
}

bool BLE_Adv_MacPktDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Adv_MacPkt *pp = (BLE_Adv_MacPkt *)object; (void)pp;
    switch (field) {
        case 0: pp->setAccessAddress(string2ulong(value)); return true;
        case 1: pp->setAdv_PDU_type(string2long(value)); return true;
        case 2: pp->setTxAdd(string2bool(value)); return true;
        case 3: pp->setRxAdd(string2bool(value)); return true;
        case 4: pp->setLength(string2long(value)); return true;
        case 5: pp->setAdv_type(string2long(value)); return true;
        case 9: pp->setAdvData(string2double(value)); return true;
        case 10: pp->setScanReqData(string2double(value)); return true;
        case 11: pp->setScanRspData(string2double(value)); return true;
        case 12: pp->setTemp_c(string2long(value)); return true;
        case 13: pp->setIndex_r(string2long(value)); return true;
        case 14: pp->setAA(string2ulong(value)); return true;
        case 15: pp->setWinSize(string2long(value)); return true;
        case 16: pp->setWinOffset(string2long(value)); return true;
        case 17: pp->setInterval(string2long(value)); return true;
        case 18: pp->setLatency(string2long(value)); return true;
        case 19: pp->setTimeout(string2long(value)); return true;
        case 20: pp->setMapCh0to7(string2long(value)); return true;
        case 21: pp->setMapCh8to15(string2long(value)); return true;
        case 22: pp->setMapCh16to23(string2long(value)); return true;
        case 23: pp->setMapCh24to31(string2long(value)); return true;
        case 24: pp->setMapCh32to39(string2long(value)); return true;
        case 25: pp->setHop(string2long(value)); return true;
        case 26: pp->setSCA(string2long(value)); return true;
        default: return false;
    }
}

const char *BLE_Adv_MacPktDescriptor::getFieldStructName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldStructNames[] = {
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        "LAddress::L2Type",
        "LAddress::L2Type",
        "LAddress::L2Type",
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
    };
    return (field>=0 && field<27) ? fieldStructNames[field] : NULL;
}

void *BLE_Adv_MacPktDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Adv_MacPkt *pp = (BLE_Adv_MacPkt *)object; (void)pp;
    switch (field) {
        case 6: return (void *)(&pp->getAdvA()); break;
        case 7: return (void *)(&pp->getInitA()); break;
        case 8: return (void *)(&pp->getScanA()); break;
        default: return NULL;
    }
}


