#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdio.h>

#include "log.h"
#include "time_util.h"
#include "lockguard.h"

// from http://stackoverflow.com/a/9098478/3831468
extern char *program_invocation_short_name;

namespace HayComm {

    using HayComm::LockGuard;

    const char * g_ArrLogLevelStr[] = {
        "INFO",
        "FATAL",
        "ERR",
        "WARN",
        "DBG",    
    };

    Log::Log(const string & sLogDirPath) 
        :m_iLogFd(-1), m_iLogLevel(LogLevel::LOG_WARN) {
        m_sLogFileName = GetDefaultLogFileName();
        m_sLogDirPath = sLogDirPath;
        pthread_mutex_init(&m_tLock, NULL);
    }

    int Log::SetLogDirPath(const string & sLogDirPath) {
        LockGuard tLockGuard(m_tLock);
        m_sLogDirPath = sLogDirPath;
        CloseLogFd();
        return 0;
    }
    
    int Log::SetLogLevel(int iLogLevel) {
        switch (iLogLevel) {
            case LogLevel::LOG_INFO:
            case LogLevel::LOG_FATAL:
            case LogLevel::LOG_ERR:
            case LogLevel::LOG_WARN:
            case LogLevel::LOG_DBG:
                break;
            default:
                return -1;
        }
        m_iLogLevel = iLogLevel;
        return 0;
    }

    int Log::log(int iLevel, const char * pszFormat, ...) {
        // level check
        if (iLevel > m_iLogLevel) {
            return -1;
        }
        // check current log file name
        {
            LockGuard tLockGuard(m_tLock);
            string sCurFileName = GetDefaultLogFileName();
            if (sCurFileName != m_sLogFileName) {
                m_sLogFileName = sCurFileName;
                CloseLogFd();
            }
            // check log fd, create if not exists
            string sFullPath = m_sLogDirPath + "/" + m_sLogFileName;
            if (m_iLogFd < 0) {
                m_iLogFd = open(sFullPath.c_str(), 
                        O_CREAT|O_RDWR|O_APPEND,
                        S_IRUSR|S_IWUSR|
                        S_IRGRP|S_IWGRP|
                        S_IROTH|S_IWOTH
                        );
                if (m_iLogFd == -1) {
                    return -2;
                }
            }
        }
        
        // write log
        pid_t iPid = getpid();
        pid_t iPPid = getppid();
        const int iBufLen = 10240;
        char lsBuf[iBufLen];
        int iLen1 = snprintf(lsBuf, iBufLen, 
                "[%s]%s(%d|%d) %s: ",
                program_invocation_short_name,
                Timestamp2Str(GetNowTimestamp()).c_str(),
                iPid,
                iPPid,
                g_ArrLogLevelStr[iLevel]
                );
        if (iLen1 < 0) {
            return -3;
        }
        va_list vl;
        va_start(vl, pszFormat);
        int iLen2 = vsnprintf(lsBuf+iLen1, iBufLen-iLen1, 
                pszFormat, vl);
        va_end(vl);
        if (iLen2 > 0) {
            if (iLen2 < iBufLen-iLen1-1) {
                lsBuf[iLen1+iLen2] = '\n';
                lsBuf[iLen1+iLen2+1] = '\0';
            }
            int iRet = write(m_iLogFd, lsBuf, iLen1+iLen2+1);
            if (iRet == -1) {
                return -3;
            }
        }

        return 0;
    }

    Log::~Log() {
        CloseLogFd();
        pthread_mutex_destroy(&m_tLock);
    }

    string Log::GetDefaultLogFileName() {
        string sNowDateStr = Timestamp2Str(HayComm::GetNowTimestamp(), "%Y%m%d%H");
        return sNowDateStr+".log";
    }

    void Log::CloseLogFd() {
        if (m_iLogFd >= 0) {
            fsync(m_iLogFd);
            close(m_iLogFd);
            m_iLogFd = -1;
        }
    }
}
