#pragma once

#include <string>
#include <sys/stat.h>
#include <sys/types.h>

using std::string;

namespace HayComm {

    int MakeDirsRecursive(const string & sDirPath, mode_t iMode=S_IRWXU);

    int LockFile(int iFd);
};

