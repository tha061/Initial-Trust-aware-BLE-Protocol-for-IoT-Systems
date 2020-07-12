//
// Generated file, do not edit! Created by opp_msgc 4.4 from modules/messages/BLE_Data_MacPkt.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#include <iostream>
#include <sstream>
#include "BLE_Data_MacPkt_m.h"

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




Register_Class(BLE_Data_MacPkt);

BLE_Data_MacPkt::BLE_Data_MacPkt(const char *name, int kind) : ::MacPkt(name,kind)
{
    this->AccessAddress_var = 0;
    this->hdr_LLID_var = 0;
    this->hdr_NESN_var = 0;
    this->hdr_SN_var = 0;
    this->hdr_MD_var = 0;
    this->hdr_lgth_var = 0;
    this->response_data_var = 0;
    this->challenge_data_var = 0;
    this->challenge_index_var = 0;
    this->responseIndex_data_var = 0;
    this->Opcode_var = 0;
    this->Instant_var = 0;
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
    this->ErrorCode_var = 0;
}

BLE_Data_MacPkt::BLE_Data_MacPkt(const BLE_Data_MacPkt& other) : ::MacPkt(other)
{
    copy(other);
}

BLE_Data_MacPkt::~BLE_Data_MacPkt()
{
}

BLE_Data_MacPkt& BLE_Data_MacPkt::operator=(const BLE_Data_MacPkt& other)
{
    if (this==&other) return *this;
    ::MacPkt::operator=(other);
    copy(other);
    return *this;
}

void BLE_Data_MacPkt::copy(const BLE_Data_MacPkt& other)
{
    this->AccessAddress_var = other.AccessAddress_var;
    this->hdr_LLID_var = other.hdr_LLID_var;
    this->hdr_NESN_var = other.hdr_NESN_var;
    this->hdr_SN_var = other.hdr_SN_var;
    this->hdr_MD_var = other.hdr_MD_var;
    this->hdr_lgth_var = other.hdr_lgth_var;
    this->response_data_var = other.response_data_var;
    this->challenge_data_var = other.challenge_data_var;
    this->challenge_index_var = other.challenge_index_var;
    this->responseIndex_data_var = other.responseIndex_data_var;
    this->Opcode_var = other.Opcode_var;
    this->Instant_var = other.Instant_var;
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
    this->ErrorCode_var = other.ErrorCode_var;
}

void BLE_Data_MacPkt::parsimPack(cCommBuffer *b)
{
    ::MacPkt::parsimPack(b);
    doPacking(b,this->AccessAddress_var);
    doPacking(b,this->hdr_LLID_var);
    doPacking(b,this->hdr_NESN_var);
    doPacking(b,this->hdr_SN_var);
    doPacking(b,this->hdr_MD_var);
    doPacking(b,this->hdr_lgth_var);
    doPacking(b,this->response_data_var);
    doPacking(b,this->challenge_data_var);
    doPacking(b,this->challenge_index_var);
    doPacking(b,this->responseIndex_data_var);
    doPacking(b,this->Opcode_var);
    doPacking(b,this->Instant_var);
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
    doPacking(b,this->ErrorCode_var);
}

void BLE_Data_MacPkt::parsimUnpack(cCommBuffer *b)
{
    ::MacPkt::parsimUnpack(b);
    doUnpacking(b,this->AccessAddress_var);
    doUnpacking(b,this->hdr_LLID_var);
    doUnpacking(b,this->hdr_NESN_var);
    doUnpacking(b,this->hdr_SN_var);
    doUnpacking(b,this->hdr_MD_var);
    doUnpacking(b,this->hdr_lgth_var);
    doUnpacking(b,this->response_data_var);
    doUnpacking(b,this->challenge_data_var);
    doUnpacking(b,this->challenge_index_var);
    doUnpacking(b,this->responseIndex_data_var);
    doUnpacking(b,this->Opcode_var);
    doUnpacking(b,this->Instant_var);
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
    doUnpacking(b,this->ErrorCode_var);
}

unsigned int BLE_Data_MacPkt::getAccessAddress() const
{
    return AccessAddress_var;
}

void BLE_Data_MacPkt::setAccessAddress(unsigned int AccessAddress)
{
    this->AccessAddress_var = AccessAddress;
}

int BLE_Data_MacPkt::getHdr_LLID() const
{
    return hdr_LLID_var;
}

void BLE_Data_MacPkt::setHdr_LLID(int hdr_LLID)
{
    this->hdr_LLID_var = hdr_LLID;
}

bool BLE_Data_MacPkt::getHdr_NESN() const
{
    return hdr_NESN_var;
}

void BLE_Data_MacPkt::setHdr_NESN(bool hdr_NESN)
{
    this->hdr_NESN_var = hdr_NESN;
}

bool BLE_Data_MacPkt::getHdr_SN() const
{
    return hdr_SN_var;
}

void BLE_Data_MacPkt::setHdr_SN(bool hdr_SN)
{
    this->hdr_SN_var = hdr_SN;
}

bool BLE_Data_MacPkt::getHdr_MD() const
{
    return hdr_MD_var;
}

void BLE_Data_MacPkt::setHdr_MD(bool hdr_MD)
{
    this->hdr_MD_var = hdr_MD;
}

int BLE_Data_MacPkt::getHdr_lgth() const
{
    return hdr_lgth_var;
}

void BLE_Data_MacPkt::setHdr_lgth(int hdr_lgth)
{
    this->hdr_lgth_var = hdr_lgth;
}

double BLE_Data_MacPkt::getResponse_data() const
{
    return response_data_var;
}

void BLE_Data_MacPkt::setResponse_data(double response_data)
{
    this->response_data_var = response_data;
}

double BLE_Data_MacPkt::getChallenge_data() const
{
    return challenge_data_var;
}

void BLE_Data_MacPkt::setChallenge_data(double challenge_data)
{
    this->challenge_data_var = challenge_data;
}

int BLE_Data_MacPkt::getChallenge_index() const
{
    return challenge_index_var;
}

void BLE_Data_MacPkt::setChallenge_index(int challenge_index)
{
    this->challenge_index_var = challenge_index;
}

int BLE_Data_MacPkt::getResponseIndex_data() const
{
    return responseIndex_data_var;
}

void BLE_Data_MacPkt::setResponseIndex_data(int responseIndex_data)
{
    this->responseIndex_data_var = responseIndex_data;
}

int BLE_Data_MacPkt::getOpcode() const
{
    return Opcode_var;
}

void BLE_Data_MacPkt::setOpcode(int Opcode)
{
    this->Opcode_var = Opcode;
}

int BLE_Data_MacPkt::getInstant() const
{
    return Instant_var;
}

void BLE_Data_MacPkt::setInstant(int Instant)
{
    this->Instant_var = Instant;
}

int BLE_Data_MacPkt::getWinSize() const
{
    return WinSize_var;
}

void BLE_Data_MacPkt::setWinSize(int WinSize)
{
    this->WinSize_var = WinSize;
}

int BLE_Data_MacPkt::getWinOffset() const
{
    return WinOffset_var;
}

void BLE_Data_MacPkt::setWinOffset(int WinOffset)
{
    this->WinOffset_var = WinOffset;
}

int BLE_Data_MacPkt::getInterval() const
{
    return Interval_var;
}

void BLE_Data_MacPkt::setInterval(int Interval)
{
    this->Interval_var = Interval;
}

int BLE_Data_MacPkt::getLatency() const
{
    return Latency_var;
}

void BLE_Data_MacPkt::setLatency(int Latency)
{
    this->Latency_var = Latency;
}

int BLE_Data_MacPkt::getTimeout() const
{
    return Timeout_var;
}

void BLE_Data_MacPkt::setTimeout(int Timeout)
{
    this->Timeout_var = Timeout;
}

int BLE_Data_MacPkt::getMapCh0to7() const
{
    return MapCh0to7_var;
}

void BLE_Data_MacPkt::setMapCh0to7(int MapCh0to7)
{
    this->MapCh0to7_var = MapCh0to7;
}

int BLE_Data_MacPkt::getMapCh8to15() const
{
    return MapCh8to15_var;
}

void BLE_Data_MacPkt::setMapCh8to15(int MapCh8to15)
{
    this->MapCh8to15_var = MapCh8to15;
}

int BLE_Data_MacPkt::getMapCh16to23() const
{
    return MapCh16to23_var;
}

void BLE_Data_MacPkt::setMapCh16to23(int MapCh16to23)
{
    this->MapCh16to23_var = MapCh16to23;
}

int BLE_Data_MacPkt::getMapCh24to31() const
{
    return MapCh24to31_var;
}

void BLE_Data_MacPkt::setMapCh24to31(int MapCh24to31)
{
    this->MapCh24to31_var = MapCh24to31;
}

int BLE_Data_MacPkt::getMapCh32to39() const
{
    return MapCh32to39_var;
}

void BLE_Data_MacPkt::setMapCh32to39(int MapCh32to39)
{
    this->MapCh32to39_var = MapCh32to39;
}

int BLE_Data_MacPkt::getErrorCode() const
{
    return ErrorCode_var;
}

void BLE_Data_MacPkt::setErrorCode(int ErrorCode)
{
    this->ErrorCode_var = ErrorCode;
}

class BLE_Data_MacPktDescriptor : public cClassDescriptor
{
  public:
    BLE_Data_MacPktDescriptor();
    virtual ~BLE_Data_MacPktDescriptor();

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

Register_ClassDescriptor(BLE_Data_MacPktDescriptor);

BLE_Data_MacPktDescriptor::BLE_Data_MacPktDescriptor() : cClassDescriptor("BLE_Data_MacPkt", "MacPkt")
{
}

BLE_Data_MacPktDescriptor::~BLE_Data_MacPktDescriptor()
{
}

bool BLE_Data_MacPktDescriptor::doesSupport(cObject *obj) const
{
    return dynamic_cast<BLE_Data_MacPkt *>(obj)!=NULL;
}

const char *BLE_Data_MacPktDescriptor::getProperty(const char *propertyname) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? basedesc->getProperty(propertyname) : NULL;
}

int BLE_Data_MacPktDescriptor::getFieldCount(void *object) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    return basedesc ? 23+basedesc->getFieldCount(object) : 23;
}

unsigned int BLE_Data_MacPktDescriptor::getFieldTypeFlags(void *object, int field) const
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
    return (field>=0 && field<23) ? fieldTypeFlags[field] : 0;
}

const char *BLE_Data_MacPktDescriptor::getFieldName(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldName(object, field);
        field -= basedesc->getFieldCount(object);
    }
    static const char *fieldNames[] = {
        "AccessAddress",
        "hdr_LLID",
        "hdr_NESN",
        "hdr_SN",
        "hdr_MD",
        "hdr_lgth",
        "response_data",
        "challenge_data",
        "challenge_index",
        "responseIndex_data",
        "Opcode",
        "Instant",
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
        "ErrorCode",
    };
    return (field>=0 && field<23) ? fieldNames[field] : NULL;
}

int BLE_Data_MacPktDescriptor::findField(void *object, const char *fieldName) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    int base = basedesc ? basedesc->getFieldCount(object) : 0;
    if (fieldName[0]=='A' && strcmp(fieldName, "AccessAddress")==0) return base+0;
    if (fieldName[0]=='h' && strcmp(fieldName, "hdr_LLID")==0) return base+1;
    if (fieldName[0]=='h' && strcmp(fieldName, "hdr_NESN")==0) return base+2;
    if (fieldName[0]=='h' && strcmp(fieldName, "hdr_SN")==0) return base+3;
    if (fieldName[0]=='h' && strcmp(fieldName, "hdr_MD")==0) return base+4;
    if (fieldName[0]=='h' && strcmp(fieldName, "hdr_lgth")==0) return base+5;
    if (fieldName[0]=='r' && strcmp(fieldName, "response_data")==0) return base+6;
    if (fieldName[0]=='c' && strcmp(fieldName, "challenge_data")==0) return base+7;
    if (fieldName[0]=='c' && strcmp(fieldName, "challenge_index")==0) return base+8;
    if (fieldName[0]=='r' && strcmp(fieldName, "responseIndex_data")==0) return base+9;
    if (fieldName[0]=='O' && strcmp(fieldName, "Opcode")==0) return base+10;
    if (fieldName[0]=='I' && strcmp(fieldName, "Instant")==0) return base+11;
    if (fieldName[0]=='W' && strcmp(fieldName, "WinSize")==0) return base+12;
    if (fieldName[0]=='W' && strcmp(fieldName, "WinOffset")==0) return base+13;
    if (fieldName[0]=='I' && strcmp(fieldName, "Interval")==0) return base+14;
    if (fieldName[0]=='L' && strcmp(fieldName, "Latency")==0) return base+15;
    if (fieldName[0]=='T' && strcmp(fieldName, "Timeout")==0) return base+16;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh0to7")==0) return base+17;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh8to15")==0) return base+18;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh16to23")==0) return base+19;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh24to31")==0) return base+20;
    if (fieldName[0]=='M' && strcmp(fieldName, "MapCh32to39")==0) return base+21;
    if (fieldName[0]=='E' && strcmp(fieldName, "ErrorCode")==0) return base+22;
    return basedesc ? basedesc->findField(object, fieldName) : -1;
}

const char *BLE_Data_MacPktDescriptor::getFieldTypeString(void *object, int field) const
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
        "bool",
        "int",
        "double",
        "double",
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
        "int",
        "int",
        "int",
    };
    return (field>=0 && field<23) ? fieldTypeStrings[field] : NULL;
}

const char *BLE_Data_MacPktDescriptor::getFieldProperty(void *object, int field, const char *propertyname) const
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

int BLE_Data_MacPktDescriptor::getArraySize(void *object, int field) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getArraySize(object, field);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Data_MacPkt *pp = (BLE_Data_MacPkt *)object; (void)pp;
    switch (field) {
        default: return 0;
    }
}

std::string BLE_Data_MacPktDescriptor::getFieldAsString(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldAsString(object,field,i);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Data_MacPkt *pp = (BLE_Data_MacPkt *)object; (void)pp;
    switch (field) {
        case 0: return ulong2string(pp->getAccessAddress());
        case 1: return long2string(pp->getHdr_LLID());
        case 2: return bool2string(pp->getHdr_NESN());
        case 3: return bool2string(pp->getHdr_SN());
        case 4: return bool2string(pp->getHdr_MD());
        case 5: return long2string(pp->getHdr_lgth());
        case 6: return double2string(pp->getResponse_data());
        case 7: return double2string(pp->getChallenge_data());
        case 8: return long2string(pp->getChallenge_index());
        case 9: return long2string(pp->getResponseIndex_data());
        case 10: return long2string(pp->getOpcode());
        case 11: return long2string(pp->getInstant());
        case 12: return long2string(pp->getWinSize());
        case 13: return long2string(pp->getWinOffset());
        case 14: return long2string(pp->getInterval());
        case 15: return long2string(pp->getLatency());
        case 16: return long2string(pp->getTimeout());
        case 17: return long2string(pp->getMapCh0to7());
        case 18: return long2string(pp->getMapCh8to15());
        case 19: return long2string(pp->getMapCh16to23());
        case 20: return long2string(pp->getMapCh24to31());
        case 21: return long2string(pp->getMapCh32to39());
        case 22: return long2string(pp->getErrorCode());
        default: return "";
    }
}

bool BLE_Data_MacPktDescriptor::setFieldAsString(void *object, int field, int i, const char *value) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->setFieldAsString(object,field,i,value);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Data_MacPkt *pp = (BLE_Data_MacPkt *)object; (void)pp;
    switch (field) {
        case 0: pp->setAccessAddress(string2ulong(value)); return true;
        case 1: pp->setHdr_LLID(string2long(value)); return true;
        case 2: pp->setHdr_NESN(string2bool(value)); return true;
        case 3: pp->setHdr_SN(string2bool(value)); return true;
        case 4: pp->setHdr_MD(string2bool(value)); return true;
        case 5: pp->setHdr_lgth(string2long(value)); return true;
        case 6: pp->setResponse_data(string2double(value)); return true;
        case 7: pp->setChallenge_data(string2double(value)); return true;
        case 8: pp->setChallenge_index(string2long(value)); return true;
        case 9: pp->setResponseIndex_data(string2long(value)); return true;
        case 10: pp->setOpcode(string2long(value)); return true;
        case 11: pp->setInstant(string2long(value)); return true;
        case 12: pp->setWinSize(string2long(value)); return true;
        case 13: pp->setWinOffset(string2long(value)); return true;
        case 14: pp->setInterval(string2long(value)); return true;
        case 15: pp->setLatency(string2long(value)); return true;
        case 16: pp->setTimeout(string2long(value)); return true;
        case 17: pp->setMapCh0to7(string2long(value)); return true;
        case 18: pp->setMapCh8to15(string2long(value)); return true;
        case 19: pp->setMapCh16to23(string2long(value)); return true;
        case 20: pp->setMapCh24to31(string2long(value)); return true;
        case 21: pp->setMapCh32to39(string2long(value)); return true;
        case 22: pp->setErrorCode(string2long(value)); return true;
        default: return false;
    }
}

const char *BLE_Data_MacPktDescriptor::getFieldStructName(void *object, int field) const
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
    return (field>=0 && field<23) ? fieldStructNames[field] : NULL;
}

void *BLE_Data_MacPktDescriptor::getFieldStructPointer(void *object, int field, int i) const
{
    cClassDescriptor *basedesc = getBaseClassDescriptor();
    if (basedesc) {
        if (field < basedesc->getFieldCount(object))
            return basedesc->getFieldStructPointer(object, field, i);
        field -= basedesc->getFieldCount(object);
    }
    BLE_Data_MacPkt *pp = (BLE_Data_MacPkt *)object; (void)pp;
    switch (field) {
        default: return NULL;
    }
}


