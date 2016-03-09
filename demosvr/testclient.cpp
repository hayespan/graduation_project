#include "demosvrclient.h"

int main() {
    int iRet = 0;
    DemosvrClient cli;
    string sMsgFromCli = "hello";
    string sMsgFromSvr;
    int iVar;
    iRet = cli.Echo(sMsgFromCli, sMsgFromSvr, iVar);
    HayLog(LOG_INFO, "ret[%d] msg_from_cli[%s] msg_from_svr[%s] var_from_svr[%d]",
            iRet, sMsgFromCli.c_str(), sMsgFromSvr.c_str(), iVar);
    return 0;
}
