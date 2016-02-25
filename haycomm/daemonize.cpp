#include <cstdio>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "daemonize.h"

namespace HayUtils {

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
        int iFd = open(pszRedirectPath, 
                S_IRUSR|S_IWUSR|
                S_IRGRP|S_IWGRP|
                S_IROTH|S_IWOTH
                );
        if (iFd == -1) {
            return -5;
        }
        dup2(iFd, STDIN_FILENO);
        dup2(iFd, STDOUT_FILENO);
        dup2(iFd, STDERR_FILENO);
    }
    return 0;
}

}
