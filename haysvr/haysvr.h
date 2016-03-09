#pragma once

#include "haycomm/config.h"

#include "haysvrdispatcher.h"
#include "tcpsvr.h"

#include <string>

using std::string;

class Haysvr {

public:
    Haysvr(const string & sSvrConfigPath, TcpSvr * pTcpSvr, HaysvrDispatcher * pDispatcher);
    virtual ~Haysvr();

    int LoadConfig(const string & sSvrConfigPath);
    void SetDispatcher(HaysvrDispatcher * pDispatcher);

    void Daemonize();
    void Singleton();
    void Run();

private:
    Haysvr(const Haysvr &);
    Haysvr & operator=(const Haysvr &);
    void InitSvr(const string & sSvrConfigPath, TcpSvr * pTcpSvr, HaysvrDispatcher * pDispatcher);
    void ReadOptionFromConfig(TcpSvrOption & oTcpSvrOption, HayComm::Config & oConfig);

private:

    // config
    string m_sSvrConfigPath;
    HayComm::Config m_oConfig;
    
    // inner svr & option
    TcpSvr * m_pTcpSvr;
    TcpSvrOption m_oTcpSvrOption;

};

