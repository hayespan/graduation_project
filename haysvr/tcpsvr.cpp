#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cstring>
#include <errno.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/mman.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctime>
#include <list>

#include "haycomm/file.h"
#include "haycomm/time_util.h"

#include "haysvrutil.h"
#include "haylog.h"
#include "tcpsvr.h"
#include "metadata.h"

// different exit way
#define MONITOR_SUCC_EXIT exit(EXIT_SUCCESS);
#define MONITOR_ERR_EXIT exit(EXIT_FAILURE);
#define MASTER_SUCC_EXIT _exit(EXIT_SUCCESS);
#define MASTER_ERR_EXIT _exit(EXIT_FAILURE);
#define BUFFER_SIZE 512

using std::vector;
using std::list;

// TcpSvrOption
TcpSvrOption::TcpSvrOption() {
    iPort = -1;
    // from http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c
    int iCpuCnt = sysconf(_SC_NPROCESSORS_ONLN); 
    iMasterCnt = iCpuCnt;
    iWorkerCnt = iCpuCnt * 2;
    iMaxConnCntPerPro = 1000;
}

TcpSvrOption::~TcpSvrOption() {

}

// MasterInfo 
MasterInfo::MasterInfo() {
    iPid = -1;
    lPRPipeFd[0] = lPRPipeFd[1] = -1;
    lPWPipeFd[0] = lPWPipeFd[1] = -1;
}

// ConnData
ConnData::ConnData() {
    iCurReadLen = 0;
    iCurWriteLen = 0;
    
    // io fd
    iFd = -1; 
    iLastTime = HayComm::GetNowTimestamp(); // last read/write time
    bzero(&tSockAddr, sizeof(tSockAddr));
}

// TcpSvr
TcpSvr::TcpSvr() {
    m_bRun = false;
    m_iCurConnCnt = 0;
    m_pTcpsvrOption = NULL;
    m_pDispatcher = NULL;
}

TcpSvr::~TcpSvr() {
    
}

void TcpSvr::SetSvrOption(const TcpSvrOption * pTcpSvrOption) {
    m_pTcpsvrOption = pTcpSvrOption;    
}

void TcpSvr::SetDispatcher(HaysvrDispatcher * pDispatcher) {
    m_pDispatcher = pDispatcher;
}


// signal pipe
static int g_lPipeFd[2];  // pipe for signal

// signal handler
static void SigHandler(int iSig) {
    int iErrno = errno;
    send(g_lPipeFd[1], (char *)&iSig, 1, 0);
    errno = iErrno;
}

void TcpSvr::InitSignalHandler() {
    signal(SIGPIPE, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    signal(SIGTERM, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);
    AddSigHandler(SIGHUP, SigHandler);
}

void TcpSvr::InitSigPipe() {
    if (socketpair(PF_UNIX, SOCK_STREAM, 0, g_lPipeFd) == -1) {
        HayLog(LOG_FATAL, "haysvr init pipe fail. err[%s]",
                strerror(errno));
        MONITOR_ERR_EXIT;
    }
}

int TcpSvr::ForkAndRunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iMonitorEpFd) {
    MasterInfo oMasterInfo;
    // create pipe for monitor & master
    if (pipe(oMasterInfo.lPRPipeFd) == -1 ||
            pipe(oMasterInfo.lPWPipeFd) == -1) {
        close(oMasterInfo.lPRPipeFd[0]);
        close(oMasterInfo.lPRPipeFd[1]);
        close(oMasterInfo.lPWPipeFd[0]);
        close(oMasterInfo.lPWPipeFd[1]);
        return -1;
    }
    int iMasterPid = -1;
    switch ((iMasterPid=fork())) {

        case -1:
            return -2;

        case 0: // master
            // close monitor's pipe
            close(g_lPipeFd[0]);
            close(g_lPipeFd[1]);
            // close monitor's epoll
            close(iMonitorEpFd);

            close(oMasterInfo.lPRPipeFd[0]);
            close(oMasterInfo.lPWPipeFd[1]);

            // run master
            try {
                RunMaster(
                        iListenFd,
                        pMmapLock,
                        oMasterInfo.lPWPipeFd[0], // read cmd
                        oMasterInfo.lPRPipeFd[1] 
                        );
            } catch (...) {
                HayLog(LOG_FATAL, "haysvr catch master exception!");
            }
            // master must exit itself,
            // otherwise consider it to be fail
            HayLog(LOG_ERR, "haysvr master didn't exit correctly itself. master_pid[%d]", getpid());
            MASTER_ERR_EXIT;

        default: // monitor
            break;
    }
    // monitor
    oMasterInfo.iPid = iMasterPid;
    close(oMasterInfo.lPRPipeFd[1]);
    close(oMasterInfo.lPWPipeFd[0]);
    m_vMasterInfo.push_back(oMasterInfo);
    AddToEpoll(iMonitorEpFd, oMasterInfo.lPRPipeFd[0], 
            EPOLLET|EPOLLIN|EPOLLRDHUP);

    HayLog(LOG_INFO, "haysvr fork master succ. pid[%d]",
            iMasterPid);
    return iMasterPid;
}


int TcpSvr::RecycleMaster(int iMonitorEpFd, int iPRFd) {
    vector<MasterInfo>::iterator iter = m_vMasterInfo.begin();
    // find die master and recycle resource
    while (iter != m_vMasterInfo.end()) {
        if (iter->lPRPipeFd[0] == iPRFd) {
            HayLog(LOG_INFO, "haysvr master die. pid[%d]",
                    iter->iPid);
            close(iter->lPRPipeFd[0]);
            close(iter->lPWPipeFd[1]);
            DelFromEpoll(iMonitorEpFd, iter->lPRPipeFd[0]);
            m_vMasterInfo.erase(iter);
            break;
        }
        ++iter;
    }
    return 0;
}


void TcpSvr::Run() {

    // init pipe for signal
    InitSigPipe();
    // init signal handler
    InitSignalHandler();

    int iFdCnt = m_pTcpsvrOption->iMasterCnt+16; //redundance
    int iMonitorEpFd = epoll_create(iFdCnt);
    if (iMonitorEpFd == -1) {
        HayLog(LOG_FATAL, "haysvr create epoll fail."); 
        MONITOR_ERR_EXIT;
    }
    
    // add g_lpipe[0] to epoll
    AddToEpoll(iMonitorEpFd, g_lPipeFd[0], EPOLLIN|EPOLLET);

    // create listen fd
    int iListenFd = CreateListenFd();

    // create mmap lock for listen_fd
    pthread_mutex_t * pMmapLock = CreateMmapLock();

    // begin fork
    int iMasterIndex = 0;
    while (iMasterIndex < m_pTcpsvrOption->iMasterCnt) {

        if (ForkAndRunMaster(iListenFd, pMmapLock, iMonitorEpFd) < 0) {
            HayLog(LOG_ERR, "haysvr fork fail. index[%d]", 
                    iMasterIndex);
        }

        ++iMasterIndex;
    }

    // monitor signal and masters
    struct epoll_event lEvs[iFdCnt]; 
    while (1) {
        int iRet = 0;
        do {
            iRet = epoll_wait(iMonitorEpFd, lEvs, iFdCnt, 5000);
        } while (iRet == -1 && errno == EINTR); // should ignore EINTR
        if (iRet < 0) { // other epoll error
            HayLog(LOG_FATAL, "haysvr epoll_wait fail. err[%s]",
                    strerror(errno));
            MONITOR_ERR_EXIT;
        } else if (iRet == 0) { // timeout
            HayLog(LOG_INFO, "haysvr monitor heartbeat.");
        }

        for (int i=0; i<iRet; ++i) {
            if (lEvs[i].data.fd == g_lPipeFd[0]) { // sig pipe 
                if (lEvs[i].events&EPOLLIN) {
                    // received signal notify
                    while (1) { // EPOLLET should read until EAGAIN

                        char lsBuf[512] = {0};
                        int iRcnt = recv(g_lPipeFd[0], lsBuf, sizeof(lsBuf), 0);
                        if (iRcnt==-1 && errno==EAGAIN) {
                            break;
                        }
                        for (int iSigIndex = 0; iSigIndex < iRcnt; ++iSigIndex) {
                            DealWithSignal((int)lsBuf[iSigIndex]);
                        }
                    }
                } else if (lEvs[i].events&EPOLLERR||
                        lEvs[i].events&EPOLLHUP) {
                    // monitor sig pipe error
                    HayLog(LOG_FATAL, "haysvr sig pipe error.");
                    MONITOR_ERR_EXIT;
                }
            } else { // masters' pipe
                if (lEvs[i].events&EPOLLIN) { // rcev msg  from masters
                    while (1) {
                        char lsBuf[512] = {0};
                        int iRcnt = read(lEvs[i].data.fd, lsBuf, 512);
                        if (iRcnt == -1 && errno == EAGAIN) {
                            break;
                        }
                    }
                    // pass XXX
                    HayLog(LOG_INFO, "haysvr monitor received msg from master.");
                } else if (lEvs[i].events&EPOLLRDHUP ||
                        lEvs[i].events&EPOLLERR ||
                        lEvs[i].events&EPOLLHUP) { // master die
                    // recycle master
                    RecycleMaster(iMonitorEpFd, lEvs[i].data.fd);

                    // fork more if less than iMasterCnt
                    int iNeedForkCnt = m_pTcpsvrOption->iMasterCnt - m_vMasterInfo.size();
                    int iForkRetryCnt = 3;
                    while (iNeedForkCnt > 0 && iForkRetryCnt > 0) {
                        if (ForkAndRunMaster(iListenFd, pMmapLock, iMonitorEpFd) < 0) {
                            --iForkRetryCnt;
                            continue;
                        }
                        iForkRetryCnt = 3;
                        --iNeedForkCnt;
                    }
                }
            }
        }
    }

}

void TcpSvr::DealWithSignal(int iSig) {
    // refresh config etc
}

void TcpSvr::RunMaster(int iListenFd, pthread_mutex_t * pMmapLock, int iRdFd, int iWrFd) {

    (void)iRdFd;
    (void)iWrFd;
    HayLog(LOG_INFO, "haysvr master begin...");

    int iEpSize = 1024; 
    // Since Linux 2.6.8, the size argument is unused
    int iMasterEpFd = epoll_create(iEpSize);
    if (iMasterEpFd == -1) {
        HayLog(LOG_FATAL, "tcpsvr master create epoll fail. err[%s]", 
                strerror(errno));
        MASTER_ERR_EXIT;
    }

    int ilEvLen = 128;
    struct epoll_event lEvs[ilEvLen];

    // start thread pool
    HayThreadPool oThreadPool;
    oThreadPool.SetThreadNum(m_pTcpsvrOption->iWorkerCnt);
    oThreadPool.SetMasterEpollFd(iMasterEpFd);
    oThreadPool.SetQueue(&m_oQueue);
    oThreadPool.SetDispatcher(m_pDispatcher);
    oThreadPool.Start();

    bool bHasLock = false;

    while (1) {

        if (!bHasLock) {
            if (m_iCurConnCnt > (int)(m_pTcpsvrOption->iMaxConnCntPerPro*7/8.)) { // too busy
                // do not trylock
            } else { // not busy
                int iRet = 0;
                if ((iRet=pthread_mutex_trylock(pMmapLock)) == 0) {
                    HayLog(LOG_INFO, "pid[%d] get lock succ.", getpid());
                    AddToEpoll(iMasterEpFd, iListenFd, EPOLLIN|EPOLLET, false);
                    bHasLock = true;
                } else {
                    if (iRet == EOWNERDEAD) {
                        pthread_mutex_consistent_np(pMmapLock);
                        HayLog(LOG_INFO, "pid[%d] try consist dead lock.", getpid());
                        pthread_mutex_unlock(pMmapLock);
                    } else {
                        bHasLock = false;
                        DelFromEpoll(iMasterEpFd, iListenFd);
                        // HayLog(LOG_INFO, "pid[%d] get lock fail.", getpid());
                    }
                }
            }
        }

        HayLog(LOG_DBG, "xxx1");
        int iRet = 0;
        do {
            iRet = epoll_wait(iMasterEpFd, lEvs, ilEvLen, 500);
        } while (iRet == -1 && errno == EINTR);
        if (iRet < 0) {
            HayLog(LOG_FATAL, "haysvr master epoll_wait fail. ret[%d] err[%s]", iRet, strerror(errno));
            MASTER_ERR_EXIT;
        } else if (iRet == 0) {
            continue;
        }

        HayLog(LOG_DBG, "xxx2");
        // fd wait accept
        list<struct epoll_event> lWaitAccept;
        list<struct epoll_event> lOtherEv;

        int iEvCnt = iRet;
        for (int i=0; i<iEvCnt; ++i) {
            if ((lEvs[i].data.fd == iListenFd)) {
                if (bHasLock) {
                    if (lEvs[i].events&EPOLLIN) {
                        lWaitAccept.push_back(lEvs[i]);
                    } else { // error occurs 
                        HayLog(LOG_FATAL, "haysvr master find listen fd error. event[%d]",
                                lEvs[i].events);
                    }
                }
            } else { // other ev
                lOtherEv.push_back(lEvs[i]);
            }
        }

        HayLog(LOG_DBG, "xxx3");
        if (bHasLock) { // first deal with accept and unlock
            HayLog(LOG_INFO, "haysvr master get listen ev_cnt[%d]", lWaitAccept.size());
            for (list<struct epoll_event>::iterator iter=lWaitAccept.begin(); iter!=lWaitAccept.end(); ++iter) {
                while (1) {
                    struct sockaddr_in cliaddr;
                    bzero(&cliaddr, sizeof(cliaddr));
                    socklen_t clilen = sizeof(cliaddr);
                    int iCliFd = accept(iter->data.fd, (struct sockaddr *)&cliaddr, &clilen);
                    if (iCliFd == -1) {
                        // maybe fd exhausted 
                        // HayLog(LOG_DBG, "haysvr master listen fd exhausted. err[%s] conn_cnt[%d]", 
                                // strerror(errno), m_iCurConnCnt);
                        break;
                    }
                    
                    m_iCurConnCnt++;

                    // store in map
                    struct ConnData * pData = new ConnData();
                    pData->iLastTime = HayComm::GetNowTimestamp();
                    pData->iFd = iCliFd;
                    pData->tSockAddr = cliaddr; 
                    m_mapConn[iCliFd] = pData;
                    // set nonblocking and add to epoll
                    HayComm::SetNonblocking(iCliFd);
                    // set reuse attr
                    int iReuse = 1;
                    setsockopt(iCliFd, SOL_SOCKET, SO_REUSEADDR, (char *)&iReuse, sizeof(iReuse));
                    AddToEpoll(iMasterEpFd, iCliFd, EPOLLIN|EPOLLRDHUP|EPOLLET, false);

                    HayLog(LOG_INFO, "add clifd[%d] to epoll.", 
                            iCliFd);
                }
            }

            // unlock
            HayLog(LOG_INFO, "haysvr master accept cnt[%d], unlock.", lWaitAccept.size());
            bHasLock = false;
            pthread_mutex_unlock(pMmapLock);
        }

        HayLog(LOG_INFO, "haysvr master get other event cnt[%d]", lOtherEv.size());
        // deal with other events
        for (list<struct epoll_event>::iterator iter=lOtherEv.begin(); iter!=lOtherEv.end(); ++iter) {

            char lsBuf[BUFFER_SIZE];

            // find conndata in map
            int iCliFd = iter->data.fd;
            ConnMap::iterator iterConn = m_mapConn.find(iCliFd);
            ConnData * pData = NULL;
            if (iterConn == m_mapConn.end()) {
                goto RECYCLE_CLIFD;
            }
            pData = iterConn->second;

            // read 
            if (iter->events&EPOLLIN) {
                HayLog(LOG_DBG, "haysvr master epollin. fd[%d]", iter->data.fd);

                while (1) {
                    int iRcnt = recv(iCliFd, lsBuf, BUFFER_SIZE, 0); 
                    if (iRcnt == -1) {
                        if (errno == EAGAIN) {
                            break;
                        } else {
                            HayLog(LOG_ERR, "haysvr master read conn fail. fd[%d] err[%s]", 
                                    iCliFd, strerror(errno));
                            goto RECYCLE_CLIFD;
                        }
                    } else if (iRcnt == 0) {
                        // peer close
                        goto RECYCLE_CLIFD;
                    } else {
                        string sTrunk(lsBuf, iRcnt);
                        pData->sData.append(sTrunk);
                        pData->iCurReadLen += iRcnt;
                        pData->iLastTime = HayComm::GetNowTimestamp();
                    }
                }

                // begin parse 
                // 1. metadata
                if (pData->tMetaData.iCmd == -1) {
                    if (pData->iCurReadLen >= (int)sizeof(MetaData)) {
                        memcpy(&(pData->tMetaData), pData->sData.data(), sizeof(MetaData));
                    } else {
                        continue;
                    }
                } 

                HayLog(LOG_DBG, "cmd[%d] reqtlen[%d] resplen[%d] sdata[%d]",
                        pData->tMetaData.iCmd, pData->tMetaData.iReqtLen, pData->tMetaData.iRespLen, 
                        pData->sData.size());
                // parse data len
                size_t iTotSize = pData->tMetaData.iReqtLen + pData->tMetaData.iRespLen + sizeof(MetaData);
                if (iTotSize <= pData->sData.size()) {
                    // recv complete
                    pData->sData.resize(iTotSize);

                    DelFromEpoll(iMasterEpFd, iCliFd);
                    // int iFileFd = open("/home/panhzh3/svrdata.hayes", O_CREAT|O_RDWR);
                    // write(iFileFd, pData->sData.data(), pData->sData.size());
                    // close(iFileFd);

                    HayLog(LOG_DBG, "pushing to queue. fd[%d]", iCliFd);
                    m_oQueue.push(pData);
                    HayLog(LOG_DBG, "push to queue ok. fd[%d]", iCliFd);

                } else {
                    HayLog(LOG_DBG, "continue recv data. fd[%d]", iCliFd);
                    continue;
                }

            // svr peer error
            } else if (iter->events&EPOLLERR || 
                    iter->events&EPOLLHUP ||
                    iter->events&EPOLLRDHUP) {
                HayLog(LOG_DBG, "haysvr master epollerr|epollup|epollrdhup. fd[%d]", iter->data.fd);
                goto RECYCLE_CLIFD;

            // write
            } else if (iter->events&EPOLLOUT) {

                HayLog(LOG_DBG, "haysvr master epollout. fd[%d] sdatalen[%d]", iter->data.fd, pData->sData.size());
                int iTotSize = sizeof(struct MetaData) + pData->tMetaData.iRespLen + pData->tMetaData.iReqtLen;
                while (1) {
                    if (pData->iCurWriteLen < iTotSize) {
                        int iLPos = pData->iCurWriteLen;
                        int iToWriteSize = iTotSize-iLPos; 
                        if (iToWriteSize > BUFFER_SIZE) {
                            iToWriteSize = BUFFER_SIZE;
                        }
                        int iWcnt = send(iCliFd, pData->sData.data()+iLPos, iToWriteSize, 0);
                        if (iWcnt == -1) {
                            if (errno == EAGAIN) {
                                break;
                            } else {
                                HayLog(LOG_ERR, "haysvr master write err. fd[%d] [%s]", 
                                        iCliFd, strerror(errno));
                            }

                        } else if (iWcnt == 0) {
                            // peer close
                            goto RECYCLE_CLIFD;
                        } else {
                            pData->iCurWriteLen += iWcnt;
                            pData->iLastTime = HayComm::GetNowTimestamp();
                        }
                    } else { // write finish
                        HayLog(LOG_DBG, "haysvr master write fd[%d] write-cnt[%d]", iCliFd, pData->iCurWriteLen);
                        HayLog(LOG_DBG, "normal recycle fd[%d]", iCliFd);
                        goto RECYCLE_CLIFD;
                    }
                } 

            } else {
                HayLog(LOG_INFO, "haysvr master fetch other event. events[%d]", 
                        iter->events);
                // pass
            }

            continue;

RECYCLE_CLIFD:
            HayLog(LOG_DBG, "all recycle fd[%d]", iCliFd);
            // client close fd
            m_iCurConnCnt--;
            close(iCliFd);
            DelFromEpoll(iMasterEpFd, iCliFd);
            delete pData;
            if (iterConn != m_mapConn.end()) {
                m_mapConn.erase(iterConn);
            }
        } 
    }

    HayLog(LOG_INFO, "haysvr master end...");
    MASTER_SUCC_EXIT;
}


int TcpSvr::CreateListenFd() {
    int iListenFd = socket(AF_INET, SOCK_STREAM, 0);    
    int iReuse = 1;
    setsockopt(iListenFd, SOL_SOCKET, SO_REUSEADDR, (char *)&iReuse, sizeof(iReuse));
    struct sockaddr_in svraddr; 
    bzero(&svraddr, sizeof(svraddr));
    svraddr.sin_family = AF_INET;
    svraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    svraddr.sin_port = htons(m_pTcpsvrOption->iPort);

    int iRet = 0;
    iRet = bind(iListenFd, (struct sockaddr *)&svraddr, sizeof(svraddr));
    if (iRet == -1) {
        HayLog(LOG_FATAL, "tcpsvr bind socket fail. port[%d] err[%s]", m_pTcpsvrOption->iPort, strerror(errno));
        MONITOR_ERR_EXIT;
    }

    iRet = listen(iListenFd, SOMAXCONN); // max queue size 
    if (iRet == -1) {
        HayLog(LOG_FATAL, "tcpsvr listen socket fail. err[%s]", strerror(errno));
        MONITOR_ERR_EXIT;
    }

    iRet = HayComm::SetNonblocking(iListenFd);
    if (iRet < 0) {
        HayLog(LOG_FATAL, "tcpsvr listen socket setnonblock fail.");
        MONITOR_ERR_EXIT;
    }

    
    return iListenFd;
}

pthread_mutex_t * TcpSvr::CreateMmapLock() {
    pthread_mutex_t * mtx = NULL;
    mtx = (pthread_mutex_t*)mmap(NULL, sizeof(pthread_mutex_t), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
    if (mtx == MAP_FAILED) {
        HayLog(LOG_FATAL, "tcpsvr create mmap fail. err[%s]",
                strerror(errno));
        MONITOR_ERR_EXIT;
    }
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    pthread_mutexattr_setprotocol(&attr, PTHREAD_PRIO_INHERIT); 
    pthread_mutexattr_setrobust_np(&attr,PTHREAD_MUTEX_ROBUST_NP); 
    pthread_mutex_init(mtx, &attr);
    pthread_mutexattr_destroy(&attr);
    return mtx;
}


void RunWorker(int iMasterEpFd, MyQueue<struct ConnData * > * pQueue, HaysvrDispatcher * pDispatcher) {

    // get conndata from queue
    struct ConnData * pData;
    pQueue->pop(pData); // block until get data
    HayLog(LOG_INFO, "haysvr worker get task. fd[%d]", pData->iFd);
    
    int iCliFd = pData->iFd;
    size_t iMetaDataLen = sizeof(struct MetaData);
    pData->iLastTime = HayComm::GetNowTimestamp(); 

    HayBuf inbuf, outbuf;
    HayLog(LOG_DBG, "hayespan sdatalen[%d], metalen[%d], reqtlen[%d], resplen[%d]",
            pData->sData.size(), iMetaDataLen, pData->tMetaData.iReqtLen, pData->tMetaData.iRespLen);
    inbuf.CopyFrom(pData->sData, iMetaDataLen, pData->tMetaData.iReqtLen);
    outbuf.CopyFrom(pData->sData, iMetaDataLen+pData->tMetaData.iReqtLen, pData->tMetaData.iRespLen);
    HayLog(LOG_DBG, "hayespan inbuf-size[%d] outbuf-size[%d] [%c%c%c]",
            inbuf.m_sBuf.size(), outbuf.m_sBuf.size(), outbuf.m_sBuf[0], outbuf.m_sBuf[1], outbuf.m_sBuf[2]);

    int iRespCode = 0;
    int iSvrErrno = pDispatcher->DoDispatch(
            pData->tMetaData.iCmd,
            inbuf,
            outbuf,
            iRespCode
            );

    if (iSvrErrno < 0) {
        HayLog(LOG_ERR, "haysvr worker DoDispatch ret fail. ret[%d]", 
                iSvrErrno);
    }
    pData->tMetaData.iSvrErrno = iSvrErrno;
    pData->tMetaData.iRespCode = iRespCode;
    pData->tMetaData.iReqtLen = inbuf.m_sBuf.size();
    pData->tMetaData.iRespLen = outbuf.m_sBuf.size();

    // construct sData
    pData->sData = ""; // release old val
    char lsBuf[iMetaDataLen];
    memcpy(lsBuf, (char *)&(pData->tMetaData), iMetaDataLen);
    string sMetaData(lsBuf, iMetaDataLen);
    pData->sData.append(sMetaData);
    pData->sData.append(inbuf.m_sBuf);
    pData->sData.append(outbuf.m_sBuf);
    pData->iCurWriteLen = 0;

    // int iFileFd = open("/home/panhzh3/svrodata.hayes", O_CREAT|O_RDWR);
    // write(iFileFd, pData->sData.data(), pData->sData.size());
    // close(iFileFd);

    HayLog(LOG_INFO, "haysvr worker end processing fd. cmd[%d] fd[%d]",
            pData->tMetaData.iCmd, iCliFd);
    AddToEpoll(iMasterEpFd, iCliFd, EPOLLET|EPOLLOUT, false);
}

// worker logic
void * pThreadFunc(void * pData) {
    HayThreadPool::HayThreadPoolData * pThreadData = (HayThreadPool::HayThreadPoolData *)pData;
    int iMasterEpFd = pThreadData->iMasterEpFd;
    MyQueue<struct ConnData * > * pQueue = pThreadData->pQueue;
    HaysvrDispatcher * pDispatcher = pThreadData->pDispatcher;

    while (1) {
        try {
            RunWorker(iMasterEpFd, pQueue, pDispatcher);
        } catch (exception & e) {
            HayLog(LOG_ERR, "haysvr worker catch exception. exp[%s]",
                    e.what());
        }
    }

    return NULL;
}

// HayThreadPool
HayThreadPool::HayThreadPool() {
    m_iThreadNum = 0;
    m_iMasterEpFd = -1;
    m_pQueue = NULL;
    m_pDispatcher = NULL;
}

HayThreadPool::~HayThreadPool() {
}

void HayThreadPool::SetThreadNum(int iThreadNum) {
    m_iThreadNum = iThreadNum;    
}

void HayThreadPool::SetMasterEpollFd(int iMasterEpFd) {
    m_iMasterEpFd = iMasterEpFd;
}

void HayThreadPool::SetQueue(MyQueue<struct ConnData * > * pQueue) {
    m_pQueue = pQueue;    
}

void HayThreadPool::SetDispatcher(HaysvrDispatcher * pDispatcher) {
    m_pDispatcher = pDispatcher;    
}

void HayThreadPool::Start() {
    int iRet = 0;

    // thread data
    HayThreadPoolData tThreadData;
    tThreadData.iMasterEpFd = m_iMasterEpFd;
    tThreadData.pQueue = m_pQueue;
    tThreadData.pDispatcher = m_pDispatcher;

    for (int i=0; i<m_iThreadNum; ++i) {
        pthread_t iPtId = 0;
        iRet = pthread_create(&iPtId, NULL, pThreadFunc, &tThreadData);
        if (iRet != 0) {
            HayLog(LOG_ERR, "haysvr threadpool create thread fail. ret[%d]", iRet);
            --i;
            continue;
        }

        iRet = pthread_detach(iPtId);
        if (iRet != 0) {
            HayLog(LOG_ERR, "haysvr threadpool detach thread fail. ret[%d]", iRet);
        }

        m_vThreadId.push_back(iPtId);
    }    

    HayLog(LOG_INFO, "haysvr threadpool create threads succ. thread_cnt[%d]", 
            m_vThreadId.size());
}

