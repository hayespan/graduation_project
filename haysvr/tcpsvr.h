#pragma once

#include <map>
#include <vector>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "haycomm/myqueue.hpp"

#include "haysvrdispatcher.h"
#include "haybuf.h"
#include "metadata.h"

int AddSigHandler(int iSig, sighandler_t pfSigHandler);
int AddToEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock=true);
int DelFromEpoll(int iEpFd, int iFd);

class TcpSvrOption { // option stored in haysvr
public:
    TcpSvrOption();
    virtual ~TcpSvrOption();
public:
    int iPort;
    int iMasterCnt;
    int iWorkerCnt;
    int iMaxConnCntPerPro;
};

struct MasterInfo { // monitor used
    int iPid;
    int lPRPipeFd[2]; // parent read
    int lPWPipeFd[2]; // parent write
    MasterInfo();
};


struct ConnData {

    struct MetaData tMetaData;

    int iCurReadLen;
    int iCurWriteLen;
    string sData; // in & out

    int iFd;
    unsigned int iLastTime;
    struct sockaddr_in tSockAddr;

    ConnData();
};

class HayThreadPool {
public:
    HayThreadPool();
    virtual ~HayThreadPool();
    void SetThreadNum(int iThreadNum);
    void SetMasterEpollFd(int iMasterEpFd);
    void SetQueue(MyQueue<struct ConnData * > * pQueue);
    void SetDispatcher(HaysvrDispatcher * pDispatcher);
    void Start();

    struct HayThreadPoolData {
        int iMasterEpFd;
        MyQueue<struct ConnData * > * pQueue;
        HaysvrDispatcher * pDispatcher;
    };

private:
    HayThreadPool & operator=(const HayThreadPool &);
    HayThreadPool(const HayThreadPool &);

private:
    int m_iThreadNum;
    int m_iMasterEpFd;
    MyQueue<struct ConnData * > * m_pQueue;
    HaysvrDispatcher * m_pDispatcher;
    vector<pthread_t> m_vThreadId;
};

class TcpSvr {
public:
    TcpSvr();
    virtual ~TcpSvr();
    void SetSvrOption(const TcpSvrOption * pTcpSvrOption);
    void SetDispatcher(HaysvrDispatcher * pDispatcher);
    void Run();
    void RunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iRdFd, int iWrFd);

private:
    TcpSvr(const TcpSvr &);
    TcpSvr & operator=(const TcpSvr &);

    void InitSigPipe();
    void InitSignalHandler(); // catch signal
    void DealWithSignal(int iSig);
    int ForkAndRunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iMonitorEpFd);
    int RecycleMaster(int iMonitorEpFd, int iPRFd);
    int CreateListenFd();
    pthread_mutex_t * CreateMmapLock();

private:
    const TcpSvrOption * m_pTcpsvrOption;
    HaysvrDispatcher * m_pDispatcher;

    bool m_bRun;
    int m_iCurConnCnt;
    std::vector<MasterInfo> m_vMasterInfo;
    typedef std::map<int, struct ConnData * > ConnMap;
    ConnMap m_mapConn;
    MyQueue<struct ConnData * > m_oQueue;
};

