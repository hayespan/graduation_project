#include <iostream>
#include "demosvrclient.h"

using namespace std;

int main() {
    int iTot = 4000;
    int iErr = 0;
    int iErr1 = 0;
    for (int i=0; i<iTot; ++i) {

    int iRet = 0;
    DemosvrClient cli;
    string sMsgFromCli = "hello";
    string sMsgFromSvr;
    int iVar;
    iRet = cli.Echo(sMsgFromCli, sMsgFromSvr, iVar);
    if (iRet < 0) iErr++;
    if (iRet == -1) iErr1++;
    //cout << sMsgFromCli << " " << sMsgFromSvr << " " << iVar << endl;
    //HayLog(LOG_INFO, "ret[%d] msg_from_cli[%s] msg_from_svr[%s] var_from_svr[%d]",
            // iRet, sMsgFromCli.c_str(), sMsgFromSvr.c_str(), iVar);
    }
    cout << iTot << " " << iErr << " " << iErr1 << endl;
    return 0;
}
