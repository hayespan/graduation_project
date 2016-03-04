#include "demosvrclient.h"

int main() {
    int iRet = 0;
    DemosvrClient cli;
    string sMsgFromCli = "hello world";
    string sMsgFromSvr;
    iRet = cli.Echo(sMsgFromCli, sMsgFromSvr);
    HayLog(LOG_INFO, "ret[%d] msg_from_cli[%s] msg_from_svr[%s]",
            iRet, sMsgFromCli.c_str(), sMsgFromSvr.c_str());
    return 0;
}
