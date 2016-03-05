#include <unistd.h>
#include <fcntl.h>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
#include <sys/types.h>
#include <cstring>

#include "file.h"

namespace HayComm {

    int MakeDirsRecursive(const string & sDirPath, mode_t iMode) {
        char lsBuf[PATH_MAX];
        char *p = NULL;
        size_t iLen;
        snprintf(lsBuf, sizeof(lsBuf), "%s", sDirPath.c_str());
        iLen = strlen(lsBuf);
        if(lsBuf[iLen - 1] == '/')
            lsBuf[iLen - 1] = 0;
        for(p = lsBuf + 1; *p; ++p) {
            if(*p == '/') {
                *p = 0;
                mkdir(lsBuf, iMode); 
                *p = '/';
            }
        }
        mkdir(lsBuf, iMode);
        struct stat st;
        return (stat(sDirPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode))?0:-1;
    }

    int LockFile(int iFd) {
        struct flock fl;
        fl.l_type = F_WRLCK;
        fl.l_start = 0;
        fl.l_whence = SEEK_SET;
        fl.l_len = 0;
        return (fcntl(iFd, F_SETLK, &fl));
    }

    int SetNonblocking(int iFd) {
        int iFl = fcntl(iFd, F_GETFL);
        return fcntl(iFd, F_SETFL, iFl|O_NONBLOCK);
    }

};
