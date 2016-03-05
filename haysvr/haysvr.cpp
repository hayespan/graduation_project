#include <cstdlib>
#include <sys/types.h>
#include <unistd.h>


#include "haycomm/string_util.h"
#include "haycomm/daemonize.h"

#include "haylog.h"
#include "haysvr.h"
#include "haysvrerrno.h"


Haysvr::Haysvr(const string & sSvrConfigPath, TcpSvr * pTcpSvr, HaysvrDispatcher * pDispatcher) {
    InitSvr(sSvrConfigPath, pTcpSvr, pDispatcher);
}

Haysvr::~Haysvr() {
}

void Haysvr::InitSvr(const string & sSvrConfigPath, TcpSvr * pTcpSvr, HaysvrDispatcher * pDispatcher) {
    if (sSvrConfigPath.empty() ||
            !pDispatcher ||
            !pTcpSvr) {
        HayLog(LOG_FATAL, "%s %s svr parameters error.",
                __FILE__, __PRETTY_FUNCTION__);
        exit(EXIT_FAILURE);
    }
    m_sSvrConfigPath = sSvrConfigPath;
    m_pTcpSvr = pTcpSvr;

    int iRet = 0;
    if ((iRet=LoadConfig(sSvrConfigPath)) < 0) {
        HayLog(LOG_FATAL, "%s %s init server config fail. ret[%d]", 
                __FILE__, __PRETTY_FUNCTION__, iRet);
        // exit if init fail
        exit(EXIT_FAILURE);
    }
    // set dispatcher for inner svr
    SetDispatcher(pDispatcher);
}

int Haysvr::LoadConfig(const string & sOthConfigPath) {
    int iRet = 0;
    string sConfigPath;
    if (sOthConfigPath.empty()) {
        sConfigPath = m_sSvrConfigPath;
    } else {
        sConfigPath = sOthConfigPath;
        m_sSvrConfigPath = sOthConfigPath;
    }

    // init config
    if ((iRet=m_oConfig.InitConfigFile(m_sSvrConfigPath)) < 0) {
        HayLog(LOG_FATAL, "%s %s InitConfigFile fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        if (m_pTcpSvr) {
            m_pTcpSvr->SetSvrOption(NULL);
        }
        return HaysvrErrno::InitConfigFail;
    }

    // read option from config file
    ReadOptionFromConfig(m_oTcpSvrOption, m_oConfig);

    // set tcpoption for inner tcpsvr
    if (m_pTcpSvr) {
        m_pTcpSvr->SetSvrOption(&m_oTcpSvrOption);
    }
    return HaysvrErrno::Ok; 
}


void Haysvr::ReadOptionFromConfig(TcpSvrOption & oTcpSvrOption, HayComm::Config & oConfig) {
    string sBuf;
    if (oConfig.GetValue("server", "port", sBuf) == 0) {
        oTcpSvrOption.iPort = HayComm::StrToInt(sBuf);
    }
    if (oConfig.GetValue("server", "mastercnt", sBuf) == 0) {
        oTcpSvrOption.iMasterCnt = HayComm::StrToInt(sBuf);
    }    if (oConfig.GetValue("server", "workercnt", sBuf) == 0) {        oTcpSvrOption.iWorkerCnt = HayComm::StrToInt(sBuf);    }
}

void Haysvr::SetDispatcher(const HaysvrDispatcher * pDispatcher) {
    if (m_pTcpSvr) {
        m_pTcpSvr->SetDispatcher(pDispatcher);
    }    
}

void Haysvr::Daemonize() {
    if (HayComm::Daemonize() < 0) {
        exit(EXIT_FAILURE);
    }    
}

void Haysvr::Singleton() {
    const string sPidPath = "/tmp";
    extern char *program_invocation_short_name;
    string sFullPath = sPidPath + "/" + program_invocation_short_name + ".pid";
    int iRet = 0;
    if ((iRet=HayComm::Singleton(sFullPath, getpid())) < 0) {
        HayLog(LOG_FATAL, "haysvr Singleton fail. ret[%d]",
                iRet);
        exit(EXIT_FAILURE);
    }
}

void Haysvr::Run() {
    Daemonize();
    HayLog(LOG_INFO, "haysvr daemonize succ.");
    Singleton();
    HayLog(LOG_INFO, "haysvr Singleton succ.");
    HayLog(LOG_INFO, "haysvr tcp svr running...", __FILE__, __PRETTY_FUNCTION__);
    m_pTcpSvr->Run();
}

