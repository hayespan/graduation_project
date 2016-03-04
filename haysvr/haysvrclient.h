#pragma once

#include "haysvrcliproto.h"

#include "haycomm/haylog.h"
#include "haycomm/config.h"

class HaysvrClient {

public:
    HaysvrClient(const string & sCliConfigPath, HaysvrCliProto * pCliProto);
    virtual ~HaysvrClient();

    // config
    int LoadConfig(const string & sOthConfigPath="");
    HayComm::Config & GetConfig();
    void SetConnTimeout(int iSec);
    void SetReadTimeout(int iSec);
    void SetSvrIp(const string & sSvrIp);
    void SetSvrPort(int iSvrPort);

    // proto
    HaysvrCliProto & GetCliProto();
    int GetResponseCode();

private:
    HaysvrClient & operator=(const HaysvrClient &);
    HaysvrClient(const HaysvrClient &);
    void InitClient(const string & sCliConfigPath, HaysvrCliProto * pCliProto);

private:
    // config 
    string m_sCliConfigPath;
    HayComm::Config m_oConfig;;

    CliProtoOption m_oCliProtoOption;
    HaysvrCliProto * m_pCliProto;    

};

