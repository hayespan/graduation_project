#include "haycomm/string_util.h"

#include "haysvrerrno.h"
#include "haysvrclient.h"

using namespace HayComm;

HaysvrClient::HaysvrClient(const string & sCliConfigPath, HaysvrCliProto * pCliProto) {
    // init client
    InitClient(sCliConfigPath, pCliProto);
}

HaysvrClient::~HaysvrClient() {
    delete m_pCliProto;
}

void HaysvrClient::InitClient(const string & sCliConfigPath, HaysvrCliProto * pCliProto) {

    if (sCliConfigPath.empty() ||
            !pCliProto) {
        HayLog(LOG_FATAL, "%s %s parameters error.", __FILE__, __PRETTY_FUNCTION__);
    }
    m_sCliConfigPath = sCliConfigPath;
    m_pCliProto = pCliProto;

    int iRet = 0;
    if ((iRet=LoadConfig()) < 0) {
        HayLog(LOG_FATAL, "%s %s init client config fail. ret[%d]", 
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return;
    }
}

int HaysvrClient::LoadConfig(const string & sOthConfigPath) {
    int iRet = 0;
    string sConfigPath;
    if (sOthConfigPath.empty()) {
        sConfigPath = m_sCliConfigPath;
    } else {
        sConfigPath = sOthConfigPath;
        m_sCliConfigPath = sOthConfigPath;
    }
    // init config
    if ((iRet=m_oConfig.InitConfigFile(m_sCliConfigPath)) < 0) {
        HayLog(LOG_FATAL, "%s %s InitConfigFile fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        // if LoadConfig fails, set cliprotooption ptr to be NULL
        // then m_pCliProto won't work
        m_pCliProto->SetCliProtoOption(NULL);
        return HaysvrErrno::InitConfigFail;
    }
    // read config and do initialization
    string sBuf;
    if (m_oConfig.GetValue("client", "connect_timeout", sBuf) == 0) {
        m_oCliProtoOption.iConnTimeout = HayComm::StrToUInt(sBuf);
    }
    if (m_oConfig.GetValue("client", "read_timeout", sBuf) == 0) {
        m_oCliProtoOption.iReadTimeout = HayComm::StrToUInt(sBuf);
    }
    if (m_oConfig.GetValue("client", "svr_ip", sBuf) == 0) {
        m_oCliProtoOption.sSvrIp = sBuf;
    }
    if (m_oConfig.GetValue("client", "svr_port", sBuf) == 0) {
        m_oCliProtoOption.iSvrPort = HayComm::StrToInt(sBuf);
    }
    if (m_oConfig.GetValue("log", "debug_mode", sBuf) == 0) {
        if (sBuf == "1") {
            HayComm::g_Logger.SetLogLevel(LOG_DBG);
        } else {
            HayComm::g_Logger.SetLogLevel(LOG_WARN);
        }
    }
    // if LoadConfig succ, set cliprotooption
    m_pCliProto->SetCliProtoOption(&m_oCliProtoOption);
    return HaysvrErrno::Ok;
}

HayComm::Config & HaysvrClient::GetConfig() {
    return m_oConfig;
}

HaysvrCliProto & HaysvrClient::GetCliProto() {
    return *m_pCliProto;
} 

int HaysvrClient::GetResponseCode() {
    return m_pCliProto->GetResponseCode();
}

void HaysvrClient::SetConnTimeout(int iSec) {
    m_oCliProtoOption.iConnTimeout = iSec;
}

void HaysvrClient::SetReadTimeout(int iSec) {
    m_oCliProtoOption.iReadTimeout = iSec;
}

void HaysvrClient::SetSvrIp(const string & sSvrIp) {
    m_oCliProtoOption.sSvrIp = sSvrIp;
}

void HaysvrClient::SetSvrPort(int iSvrPort) {
    m_oCliProtoOption.iSvrPort = iSvrPort;
}

