#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "haycomm/file.h"

#include "haylog.h"
#include "haysvrutil.h"
#include "haysvrcliproto.h"
#include "haysvrerrno.h"
#include "metadata.h"

#define BUFFER_SIZE 512

// CliProtoOption
CliProtoOption::CliProtoOption() {
    iConnTimeout = -1;
    iReadTimeout = -1;
    sSvrIp = "";
    iSvrPort = -1;
}

CliProtoOption::~CliProtoOption() {

}

// HaysvrCliProto
HaysvrCliProto::HaysvrCliProto() {
    m_pCliProtoOption = NULL;
    m_iResponseCode = -1;
}

int HaysvrCliProto::DoProtoCall(int iCmd, const HayBuf & inbuf, HayBuf & outbuf) {

    int iErrno = HaysvrErrno::Ok;
    int iRet = 0;

    int iFdCnt = 5; 
    int iCliEpFd = -1, iConnFd = -1;
    struct epoll_event lEvs[iFdCnt]; 

    struct sockaddr_in svraddr;
    bzero(&svraddr, sizeof(struct sockaddr_in));
    svraddr.sin_family = AF_INET;
    svraddr.sin_port = htons(m_pCliProtoOption->iSvrPort);
    inet_pton(AF_INET, m_pCliProtoOption->sSvrIp.c_str(), &svraddr.sin_addr);

    bool bConnInProg = true;

    int iConnTimeout = m_pCliProtoOption->iConnTimeout;
    int iReadTimeout = m_pCliProtoOption->iReadTimeout;
    int iWaitTime = iConnTimeout;

    // construct metadata
    struct MetaData tMetaData;
    tMetaData.iCmd = iCmd;
    tMetaData.iAttr = 0;
    tMetaData.iReqtLen = inbuf.m_sBuf.size();
    tMetaData.iRespLen = outbuf.m_sBuf.size();
    tMetaData.iSvrErrno = 0;
    tMetaData.iRespCode = 0;

    int iMetaLen = sizeof(tMetaData);
    char lsMetaBuf[iMetaLen];
    memcpy(lsMetaBuf, (char *)&tMetaData, iMetaLen);
    string sData(lsMetaBuf, iMetaLen);
    sData.append(inbuf.m_sBuf);
    sData.append(outbuf.m_sBuf);

    int iTotSize = sData.size();
    int iWroteSize = 0;
    bool bWriteFinish = false;
    int iReadCnt = 0;
    bool bParseHeaderFinish = false;

    if (m_pCliProtoOption == NULL) {
        iErrno = HaysvrErrno::NoProtoOption;
        goto CLIPROTO_RECYCLE;
    }

    iCliEpFd = epoll_create(iFdCnt);
    if (iCliEpFd == -1) {
        HayLog(LOG_FATAL, "haysvr client create epoll fail. err[%s]",  
                strerror(errno));
        iErrno = HaysvrErrno::CreateEpollFail;
        goto CLIPROTO_RECYCLE;
    }

    iConnFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iConnFd < 0) {
        HayLog(LOG_ERR, "haysvr cliproto create socket fail. err[%s]",
                __func__, strerror(errno));
        iErrno = HaysvrErrno::CreateSocketFail;
        goto CLIPROTO_RECYCLE;
    }

    if (HayComm::SetNonblocking(iConnFd) < 0) {
        HayLog(LOG_DBG, "haysvr cliprto set nonblock fail.");
        iErrno = HaysvrErrno::SetNonblockFail;
        goto CLIPROTO_RECYCLE;
    }

    AddToEpoll(iCliEpFd, iConnFd, EPOLLET|EPOLLOUT, false);

    if ((iRet=connect(iConnFd, (struct sockaddr *)&svraddr, sizeof(struct sockaddr))) == 0) {
        // connect quickly!
        bConnInProg = false;
    } else if (iRet == -1 && errno == EINPROGRESS) {
        // connect still in progress
    } else {
        HayLog(LOG_ERR, "haysvr cliproto do connect fail. err[%s]", 
                strerror(errno));
        iErrno = HaysvrErrno::ConnectFail;
        goto CLIPROTO_RECYCLE;
    }

    while (1) {

        int iRet = 0;
        do {
            iRet = epoll_wait(iCliEpFd, lEvs, iFdCnt, iWaitTime);
        } while (iRet == -1 && errno == EINTR);
        if (iRet == 0) { // timeout
            if (bConnInProg) {
                HayLog(LOG_INFO, "haysvr cliproto do connect exceed timelimit, connect_timeout[%d]",
                        m_pCliProtoOption->iConnTimeout);
                iErrno = HaysvrErrno::ConnectTimeout;
                goto CLIPROTO_RECYCLE;
            }
        } else if (iRet < 0) {
            HayLog(LOG_ERR, "haysvr cliproto do connect epoll wait fail. err[%s]", strerror(errno));
            iErrno = HaysvrErrno::EpollWaitFail;
            goto CLIPROTO_RECYCLE;
        }  

        for (int i=0; i<iRet; ++i) {

            if (lEvs[i].events&EPOLLOUT) { // write

                if (bConnInProg) { // if now connect, should check error via getsockopt
                    int iConnErr = 0;
                    socklen_t iConnErrSize = sizeof(iConnErr);
                    getsockopt(iConnFd, SOL_SOCKET, SO_ERROR, &iConnErr, &iConnErrSize);
                    if (iConnErr == 0 || iConnErr == EISCONN) { // connect succ
                        bConnInProg = false;
                        iWaitTime = 500; 
                    } else if (iConnErr == ETIMEDOUT) { // client setting connect_timeout too long, this is real timeout
                        iErrno = HaysvrErrno::ConnectTimeout;
                        goto CLIPROTO_RECYCLE;
                    } else { // other fail situation
                        iErrno = HaysvrErrno::ConnectFail;
                        goto CLIPROTO_RECYCLE;
                    }
                }
                
                while (1) {
                    if (iWroteSize < iTotSize) {
                        int iLPos = iWroteSize;
                        int iToWriteSize = iTotSize-iLPos;
                        if (iToWriteSize > BUFFER_SIZE) {
                            iToWriteSize = BUFFER_SIZE;
                        }
                        int iWcnt = send(iConnFd, sData.data()+iWroteSize, iToWriteSize, 0);
                        if (iWcnt == -1) {
                            if (errno == EAGAIN) {
                                break;
                            } else {
                                HayLog(LOG_ERR, "haysvr cliproto write fail. err[%s]",
                                        strerror(errno));
                                iErrno = HaysvrErrno::WriteFail;
                                goto CLIPROTO_RECYCLE;
                            }
                        } else if (iWcnt == 0) {
                            iErrno = HaysvrErrno::WriteFail;
                            goto CLIPROTO_RECYCLE;
                        } else {
                            iWroteSize += iWcnt;
                            iToWriteSize -= iWcnt;
                        }
                    } else {
                        // write complete
                        bWriteFinish = true;
                        break;
                    } 
                }
                
                if (bWriteFinish) {
                    break;
                }

            } else { // other unexpected events
                HayLog(LOG_ERR, "haysvr cliproto catch unexpected events/err. ev[%d]",
                        lEvs[i].events);
                iErrno = HaysvrErrno::UnExpectErr;
                goto CLIPROTO_RECYCLE;
            }

            if (bWriteFinish) {
                break;
            }

        } 

        if (bWriteFinish) {
            break;
        }

    }
    
    ModInEpoll(iCliEpFd, iConnFd, EPOLLET|EPOLLIN);
    sData = "";
    
    while (1) {
        do {
            iRet = epoll_wait(iCliEpFd, lEvs, iConnFd, iReadTimeout);
        } while (iRet == -1 && errno == EAGAIN);
        if (iRet == -1) {
            iErrno = HaysvrErrno::EpollWaitFail;
            goto CLIPROTO_RECYCLE;
        } else if (iRet == 0) {
            HayLog(LOG_INFO, "haysvr cliproto read exceed timelimit. read_timeout[%d]",
                    iReadTimeout);
            iErrno = HaysvrErrno::ReadTimeout;
            goto CLIPROTO_RECYCLE;
        }

        for (int i=0; i<iRet; ++i) {

            if (lEvs[i].events&EPOLLIN) {

                while (1) {
                    char lsBuf[BUFFER_SIZE];
                    int iRcnt = recv(iConnFd, lsBuf, BUFFER_SIZE, 0);
                    if (iRcnt == -1) {
                        if (errno == EAGAIN) {
                            break;
                        } else {
                            HayLog(LOG_ERR, "haysvr cliproto read fail. err[%s]", 
                                    strerror(errno));
                            
                            iErrno = HaysvrErrno::ReadFail;
                            goto CLIPROTO_RECYCLE;
                        }
                    } else if (iRcnt == 0) {
                        // peer close
                        break;
                    } else {
                        string sTrunk(lsBuf, iRcnt);
                        sData.append(sTrunk);
                        iReadCnt += iRcnt;
                    }
                }

                // parse content
                // 1. metadata
                if (!bParseHeaderFinish) {
                    HayLog(LOG_DBG, "haysvr cliproto readcnt[%d]", iReadCnt);
                    if (iReadCnt < (int)sizeof(MetaData)) {
                        continue; 
                    } else {
                        memcpy(&tMetaData, sData.data(), sizeof(tMetaData));
                        bParseHeaderFinish = true;
                    }
                }

                if (tMetaData.iSvrErrno != HaysvrErrno::Ok) {
                    HayLog(LOG_ERR, "haysvr cliproto get svr's ret errno. svrerrno[%d]", 
                            __func__, tMetaData.iSvrErrno);
                    iErrno = tMetaData.iSvrErrno;
                    goto CLIPROTO_RECYCLE;
                }

                m_iResponseCode = tMetaData.iRespCode;

                int iTotLen = iMetaLen+tMetaData.iReqtLen+tMetaData.iRespLen;
                HayLog(LOG_DBG, "haysvr cliproto totlen[%d] metalen[%d] reqtlen[%d] resplen[%d]", 
                        iTotLen, iMetaLen, tMetaData.iReqtLen, tMetaData.iRespLen);
                if (iReadCnt < iTotLen) {
                    continue;
                } else {
                    sData.resize(iTotLen);
                    outbuf.m_sBuf = string(sData.data()+iMetaLen+tMetaData.iReqtLen, tMetaData.iRespLen);
                    iErrno = HaysvrErrno::Ok;
                    goto CLIPROTO_RECYCLE;
                }

            } else {
                iErrno = HaysvrErrno::UnExpectErr;
                goto CLIPROTO_RECYCLE;
            }
        }

    }


CLIPROTO_RECYCLE:
    close(iCliEpFd);
    close(iConnFd);
    return iErrno; 
}

int HaysvrCliProto::GetResponseCode() {
    return m_iResponseCode;
}

void HaysvrCliProto::SetCliProtoOption(CliProtoOption * pCliProtoOption) {
    m_pCliProtoOption = pCliProtoOption;
}

HaysvrCliProto::~HaysvrCliProto() {
    
}

