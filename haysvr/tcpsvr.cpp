#include <unistd.h>

#include "tcpsvr.h"

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

void TcpSvr::Run() {
    
}

