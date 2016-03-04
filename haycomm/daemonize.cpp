#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "daemonize.h"
#include "file.h"

namespace HayComm {

const char * pszRedirectPath = "/dev/null";

int Daemonize(bool bChDir/*true*/, bool bInOutErrNull/*true*/) {
    switch (fork()) {
        case -1: 
            return -1; 
        case 0:
            break;
        default:
            _exit(0);
    }
    umask(0);
    int iRet = setsid();
    if (iRet < 0) {
        return -2;
    }
    switch (fork()) {
        case -1:
            return -3;
        case 0:
            break;
        default:
            _exit(0);
    }
    if (bChDir) {
        if ((iRet=chdir("/")) == -1) {
            return -4;
        }
    }
    if (bInOutErrNull) {
        RedirectFd(STDIN_FILENO, pszRedirectPath);
        RedirectFd(STDOUT_FILENO, pszRedirectPath);
        RedirectFd(STDERR_FILENO, pszRedirectPath);
    }
    return 0;
}

int RedirectFd(int iFd, const string & sRediectPath) {
    if (access(sRediectPath.c_str(), F_OK) == -1) {
        return -1;
    }
    int iRedirectFd = open(sRediectPath.c_str(), 
            S_IRUSR|S_IWUSR|
            S_IRGRP|S_IWGRP|
            S_IROTH|S_IWOTH
            );
    if (iRedirectFd == -1) {
        return -2;
    }
    if (dup2(iRedirectFd, iFd) == -1) {
        return -3;
    }
    return 0;
}

int Singleton(const string & sPidPath, int iPid) {
    int iFd = open(sPidPath.c_str(), O_RDWR|O_CREAT,
            S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH); 
    if (iFd < 0) {
        return -1;
    }
    if (HayComm::LockFile(iFd) < 0) {
        if (errno == EACCES || errno == EAGAIN) {
            // already lock
            close(iFd);
            return -2;
        }
        // other error
        close(iFd);
        return -3;
    }
    ftruncate(iFd, 0);
    char lsBuf[16] = {0};
    sprintf(lsBuf, "%ld", (long)iPid);
    write(iFd, lsBuf, strlen(lsBuf)+1);
    fsync(iFd);
    // here cannot close fd
    return 0;
}

}
