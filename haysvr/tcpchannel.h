#pragma once

#include <string>
#include <cstdlib>

using std::string;

class TcpChannel {

public:
    TcpChannel();
    virtual ~TcpChannel();

private:
    TcpChannel(const TcpChannel &);
    TcpChannel & operator=(const TcpChannel &);
    void SetConnTimeout(int iConnectTimeout);
    void SetReadTimeout(int iReadTimeout);
    int Connect(const string & sIp, int iPort);
    int Detach();
    int Close();
    ssize_t Read(char * pReadBuf, ssize_t iBufSize);
    ssize_t Write(const char * pWriteBuf, ssize_t iBufSize);

private:
    int m_iConnectTimeout; 
    int m_iReadTimeout;

    int m_iErrno;
    int m_iSock;
};

