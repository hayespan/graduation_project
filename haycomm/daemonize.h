#pragma once

#include <string>

using std::string;

namespace HayComm {
    int Daemonize(bool bChDir=true, bool bInOutErrNull=true);
    int RedirectFd(int iFd, const string & sRediectPath);
    int Singleton(const string & sPidPath, int iPid);
}
