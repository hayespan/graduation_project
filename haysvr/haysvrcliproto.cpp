#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <fcntl.h>

#include "haylog.h"
#include "haysvrcliproto.h"
#include "haysvrerrno.h"
#include "metadata.h"

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
    if (m_pCliProtoOption == NULL) {
        return HaysvrErrno::NoProtoOption;
    }

    // TODO
    int iConnFd = socket(AF_INET, SOCK_STREAM, 0);
    if (iConnFd < 0) {
        HayLog(LOG_ERR, "%s create socket fail. err[%s]",
                __func__, strerror(errno));
        return HaysvrErrno::CreateSocketFail;
    }
    struct sockaddr_in svraddr;
    bzero(&svraddr, sizeof(struct sockaddr_in));
    svraddr.sin_family = AF_INET;
    svraddr.sin_port = htons(m_pCliProtoOption->iSvrPort);
    inet_pton(AF_INET, m_pCliProtoOption->sSvrIp.c_str(), &svraddr.sin_addr);

    if (connect(iConnFd, (struct sockaddr *)&svraddr, sizeof(struct sockaddr)) < 0) {
        HayLog(LOG_ERR, "%s connect fail. err[%s]", 
                __func__, strerror(errno));
        close(iConnFd);
        return HaysvrErrno::ConnectFail;
    }
    HayLog(LOG_DBG, "hayespan #1");
    
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
    int iFileFd = open("/home/panhzh3/clidata.hayes", O_CREAT|O_RDWR);
    write(iFileFd, sData.data(), sData.size());
    close(iFileFd);

    // write to svr
    
    int iToWrite = sData.size();
    int iWcnt = 0;
    HayLog(LOG_DBG, "hayespan #2");
    while (iToWrite > 0) {
    HayLog(LOG_DBG, "hayespan #3 [%d] metalen[%d] reqtlen[%d] resplen[%d]", sData.size(), iMetaLen, inbuf.m_sBuf.size(), outbuf.m_sBuf.size());
        int iRet = send(iConnFd, sData.data()+iWcnt, iToWrite, 0);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s client write fail. err[%s]", 
                    __func__, strerror(errno));
            close(iConnFd);
            return HaysvrErrno::WriteFail;
        }
        iWcnt += iRet;
        iToWrite-= iRet;
    }

    // read from svr
    
    sData = "";
    int iRbufLen = 1024;
    char lsRbuf[iRbufLen];
    HayLog(LOG_DBG, "hayespan #4");
    while (1) {
        int iRcnt = recv(iConnFd, lsRbuf, iRbufLen, 0); 
    HayLog(LOG_DBG, "hayespan #5 %d", iRcnt);
        if (iRcnt == -1) {
            HayLog(LOG_ERR, "%s client read socket fail. err[%s]", __func__, strerror(errno));
            close(iConnFd);
            return HaysvrErrno::ReadFail;
        }
        if (iRcnt == 0) {
            break;
        }
        string sTrunk(lsRbuf, iRcnt);
        sData.append(sTrunk);
    }
    HayLog(LOG_DBG, "data from svr. sdatalen[%d]", sData.size());
    memcpy(&tMetaData, sData.data(), iMetaLen);
    if (tMetaData.iSvrErrno != HaysvrErrno::Ok) {
        HayLog(LOG_ERR, "%s client get svr's ret errno. svrerrno[%d]", 
                __func__, tMetaData.iSvrErrno);
        close(iConnFd);
        return tMetaData.iSvrErrno;
    }

    iFileFd = open("/home/panhzh3/cliodata.hayes", O_CREAT|O_RDWR);
    write(iFileFd, sData.data(), sData.size());
    close(iFileFd);

    m_iResponseCode = tMetaData.iRespCode;

    outbuf.m_sBuf = string(sData.data()+iMetaLen+tMetaData.iReqtLen, tMetaData.iRespLen);

    HayLog(LOG_ERR, "hayespan #6");
    return HaysvrErrno::Ok; 
}

int HaysvrCliProto::GetResponseCode() {
    return m_iResponseCode;
}

void HaysvrCliProto::SetCliProtoOption(CliProtoOption * pCliProtoOption) {
    m_pCliProtoOption = pCliProtoOption;
}

HaysvrCliProto::~HaysvrCliProto() {
    
}

