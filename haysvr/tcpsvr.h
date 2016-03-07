#pragma once

#include <vector>
#include <signal.h>
#include <pthread.h>

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

struct MasterInfo {
    int iPid;
    int lPRPipeFd[2]; // parent read
    int lPWPipeFd[2]; // parent write
    MasterInfo();
};

class TcpSvr {
public:
    TcpSvr();
    virtual ~TcpSvr();
    void SetSvrOption(const TcpSvrOption * pTcpSvrOption);
    void SetDispatcher(const HaysvrDispatcher * pDispatcher);
    void Run();
    void RunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iRdFd, int iWrFd);

private:
    TcpSvr(const TcpSvr &);
    TcpSvr & operator=(const TcpSvr &);

    void InitSigPipe();
    void InitSignalHandler(); // catch signal
    static int AddSigHandler(int iSig, sighandler_t pfSigHandler);
    static int AddToEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock=true);
    static int DelFromEpoll(int iEpFd, int iFd);
    void DealWithSignal(int iSig);
    int ForkAndRunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iMonitorEpFd);
    int RecycleMaster(int iMonitorEpFd, int iPRFd);
    int CreateListenFd();
    pthread_mutex_t * CreateMmapLock();

private:
    const TcpSvrOption * m_pTcpsvrOption;
    const HaysvrDispatcher * m_pDispatcher;

    bool m_bRun;
    std::vector<MasterInfo> m_vMasterInfo;
};

