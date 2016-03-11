#include <sys/epoll.h>
#include <sys/types.h>
#include <cstring>
#include "haycomm/file.h"

#include "haysvrutil.h"

int AddSigHandler(int iSig, sighandler_t pfSigHandler) {
    struct sigaction sa;
    memset(&sa, '\0', sizeof(sa));
    sa.sa_handler = pfSigHandler;
    sa.sa_flags |= SA_RESTART;
    sigfillset(&sa.sa_mask);
    return sigaction(iSig, &sa, NULL);
}

int AddToEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock) {
    struct epoll_event tEv;
    tEv.data.fd = iFd;
    tEv.events = iEv;

    if (bNonBlock) {
        if (HayComm::SetNonblocking(iFd) == -1) {
            return -1;
        }
    }
    if (epoll_ctl(iEpFd, EPOLL_CTL_ADD, iFd, &tEv) == -1) {
        return -2;
    }
    return 0;
} 

int DelFromEpoll(int iEpFd, int iFd) {
    return epoll_ctl(iEpFd, EPOLL_CTL_DEL, iFd, NULL);
}


int ModInEpoll(int iEpFd, int iFd, int iEv, bool bNonBlock) {
    struct epoll_event tEv;
    tEv.data.fd = iFd;
    tEv.events = iEv;

    if (bNonBlock) {
        if (HayComm::SetNonblocking(iFd) == -1) {
            return -1;
        }
    }
    if (epoll_ctl(iEpFd, EPOLL_CTL_MOD, iFd, &tEv) == -1) {
        return -2;
    }
    return 0;
}

