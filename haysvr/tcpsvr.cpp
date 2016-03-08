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
#include <list>

#include "haycomm/file.h"

#include "haylog.h"
#include "tcpsvr.h"

// different exit way
#define MONITOR_SUCC_EXIT exit(EXIT_SUCCESS);
#define MONITOR_ERR_EXIT exit(EXIT_FAILURE);
#define MASTER_SUCC_EXIT _exit(EXIT_SUCCESS);
#define MASTER_ERR_EXIT _exit(EXIT_FAILURE);

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

void TcpSvr::SetDispatcher(const HaysvrDispatcher * pDispatcher) {
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

int TcpSvr::AddSigHandler(int iSig, sighandler_t pfSigHandler) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = pfSigHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    return sigaction(iSig, &sa, NULL);
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

int TcpSvr::AddToEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock) {
    struct epoll_event tEv;
    tEv.data.fd = iFd;
    tEv.events = iEv;

    if (bNonBlock) {
        if (HayComm::SetNonblocking(iFd) == -1) {
            HayLog(LOG_ERR, "haysvr setnonblocking fail. fd[%d] err[%s]",
                    iFd, strerror(errno));
            return -1;
        }
    }
    if (epoll_ctl(iEpFd, EPOLL_CTL_ADD, iFd, &tEv) == -1) {
        HayLog(LOG_ERR, "haysvr epoll_add fail. fd[%d] err[%s]",
                iFd, strerror(errno));
        return -2;
    }
    return 0;
} 

int TcpSvr::DelFromEpoll(int iEpFd, int iFd) {
    return epoll_ctl(iEpFd, EPOLL_CTL_DEL, iFd, NULL);
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
        int iRet = epoll_wait(iMonitorEpFd, lEvs, iFdCnt, 5000);
        if (iRet < 0) { // epoll error
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

    bool bHasLock = false;
    bool bAcceptPrio = false;

    while (1) {

        if (!bHasLock && (m_iCurConnCnt < (int)(m_pTcpsvrOption->iMaxConnCntPerPro*7/8.))) { // do not own lock
            int iRet = 0;
            if ((iRet=pthread_mutex_trylock(pMmapLock)) == 0) {
                HayLog(LOG_INFO, "pid[%d] get lock succ.", getpid());
                AddToEpoll(iMasterEpFd, iListenFd, EPOLLIN, false);
                bAcceptPrio = true;
                bHasLock = true;
            } else {
                if (iRet == EOWNERDEAD) {
                    pthread_mutex_consistent_np(pMmapLock);
                    HayLog(LOG_INFO, "pid[%d] try consist dead lock.", getpid());
                    pthread_mutex_unlock(pMmapLock);
                } else {
                    bAcceptPrio = false;
                    bHasLock = false;
                    DelFromEpoll(iMasterEpFd, iListenFd);
                    // HayLog(LOG_INFO, "pid[%d] get lock fail.", getpid());
                }
            }
        }

        int iRet = epoll_wait(iMasterEpFd, lEvs, ilEvLen, 500);
        if (iRet < 0) {
            HayLog(LOG_FATAL, "haysvr master epoll_wait fail. ret[%d] err[%s]", iRet, strerror(errno));
            MASTER_ERR_EXIT;
        } else if (iRet == 0) {
            continue;
        }

        // fd wait accept
        list<struct epoll_event> lWaitAccept;
        list<struct epoll_event> lOtherEv;

        int iEvCnt = iRet;
        for (int i=0; i<iEvCnt; ++i) {
            if (lEvs[i].data.fd == iListenFd) {
                if (lEvs[i].events&EPOLLIN) {
                    lWaitAccept.push_back(lEvs[i]);
                } else { // error occurs 
                    HayLog(LOG_FATAL, "haysvr master find listen fd error. event[%d]",
                            lEvs[i].events);
                }
            } else { // other ev
                lOtherEv.push_back(lEvs[i]);
            }
        }

        if (bAcceptPrio) { // first deal with accept and unlock
            for (list<struct epoll_event>::iterator iter=lWaitAccept.begin(); iter!=lWaitAccept.end(); ++iter) {
                struct sockaddr_in cliaddr;
                bzero(&cliaddr, sizeof(cliaddr));
                socklen_t clilen = sizeof(cliaddr);
                int iCliFd = accept(iter->data.fd, (struct sockaddr *)&cliaddr, &clilen);
                if (iCliFd == -1) {
                    // maybe fd exhausted 
                    continue;
                } else {
                    m_iCurConnCnt++;
                }
                HayLog(LOG_INFO, "pid[%d] clifd[%d] err[%s]", 
                        getpid(), iCliFd, strerror(errno));
                // close(iCliFd);
            }
            // unlock
            HayLog(LOG_INFO, "haysvr master accept cnt[%d], unlock.", lWaitAccept.size());
            bHasLock = false;
            pthread_mutex_unlock(pMmapLock);
        }

        // deal with other events
        for (list<struct epoll_event>::iterator iter=lOtherEv.begin(); iter!=lOtherEv.end(); ++iter) {
            
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

