#pragma once

#include "haysvrdispatcher.h"

class TcpSvrOption {
public:
    TcpSvrOption();
    virtual ~TcpSvrOption();
public:
    int iPort;
    int iMasterCnt;
    int iWorkerCnt;
};

class TcpSvr {
public:
    TcpSvr();
    virtual ~TcpSvr();
    void SetSvrOption(const TcpSvrOption * pTcpSvrOption);
    void SetDispatcher(const HaysvrDispatcher * pDispatcher);
    void Run();

private:
    TcpSvr(const TcpSvr &);
    TcpSvr & operator=(const TcpSvr &);

private:
    const TcpSvrOption * m_pTcpsvrOption;
    const HaysvrDispatcher * m_pDispatcher;

    bool m_bRun;
};

