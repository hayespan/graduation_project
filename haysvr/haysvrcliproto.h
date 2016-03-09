#pragma once

#include <string>

#include "haybuf.h"

using std::string;

class CliProtoOption {
public:
    CliProtoOption();
    virtual ~CliProtoOption();

public:
    int iConnTimeout;
    int iReadTimeout;
    string sSvrIp;
    int iSvrPort;
};

class HaysvrCliProto {

public:
    HaysvrCliProto();
    int DoProtoCall(int iCmd, const HayBuf & inbuf, HayBuf & outbuf);
    int GetResponseCode();
    void SetCliProtoOption(CliProtoOption * pCliProtoOption);
    virtual ~HaysvrCliProto();

private:
    HaysvrCliProto(const HaysvrCliProto &);
    HaysvrCliProto & operator=(const HaysvrCliProto &);

private:
    int m_iResponseCode;
    CliProtoOption * m_pCliProtoOption;
} ;
