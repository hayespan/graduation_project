#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <cstring>
#include <errno.h>
#include <sys/wait.h>
#include <cstdlib>

#include "haycomm/file.h"

#include "haylog.h"
#include "tcpsvr.h"

using std::vector;

// TcpSvrOption
TcpSvrOption::TcpSvrOption() {
    iPort = -1;
    // from http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c
    int iCpuCnt = sysconf(_SC_NPROCESSORS_ONLN); 
    iMasterCnt = iCpuCnt;
    iWorkerCnt = iCpuCnt * 2;
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
        exit(EXIT_FAILURE);
    }
}

int TcpSvr::AddToEpoll(int iEpFd, int iFd, int iEv) {
    struct epoll_event tEv;
    tEv.data.fd = iFd;
    tEv.events = iEv;
    if (HayComm::SetNonblocking(iFd) == -1) {
        HayLog(LOG_ERR, "haysvr setnonblocking fail. fd[%d] err[%s]",
                iFd, strerror(errno));
        return -1;
    }
    if (epoll_ctl(iEpFd, EPOLL_CTL_ADD, iFd, &tEv) == -1) {
        HayLog(LOG_ERR, "haysvr epoll_add fail. fd[%s] err[%s]",
                iFd, strerror(errno));
        return -2;
    }
    return 0;
} 

int TcpSvr::DelFromEpoll(int iEpFd, int iFd) {
    return epoll_ctl(iEpFd, EPOLL_CTL_DEL, iFd, NULL);
}

int TcpSvr::ForkAndRunMaster(int iMonitorEpFd) {
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
                        oMasterInfo.lPWPipeFd[0], // read cmd
                        oMasterInfo.lPRPipeFd[1] 
                        );
            } catch (...) {
                HayLog(LOG_FATAL, "haysvr catch master exception!");
            }
            // master must exit itself,
            // otherwise consider it to be fail
            HayLog(LOG_ERR, "haysvr master didn't exit correctly itself. master_pid[%d]", getpid());
            _exit(EXIT_FAILURE);

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
        exit(EXIT_FAILURE);
    }
    
    // add g_lpipe[0] to epoll
    AddToEpoll(iMonitorEpFd, g_lPipeFd[0], EPOLLIN|EPOLLET);

    // begin fork
    int iMasterIndex = 0;
    while (iMasterIndex < m_pTcpsvrOption->iMasterCnt) {

        if (ForkAndRunMaster(iMonitorEpFd) < 0) {
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
            exit(EXIT_FAILURE);
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
                    exit(EXIT_FAILURE);
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
                        if (ForkAndRunMaster(iMonitorEpFd) < 0) {
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

void TcpSvr::RunMaster(int iRdFd, int iWrFd) {
    while (1) {
        sleep(1); 
        HayLog(LOG_INFO, "master pid[%d] ...", getpid());
    }
    HayLog(LOG_INFO, "haysvr master exit");
    // exit(EXIT_SUCCESS);
}

