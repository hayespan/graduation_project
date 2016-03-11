#include <unistd.h>
#include <iostream>
#include <pthread.h>
#include "demosvrclient.h"

using namespace std;

void * pFunc(void *) {
    int iTot = 1;
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
    // cout << sMsgFromCli << " " << sMsgFromSvr << " " << iVar << endl;
    //HayLog(LOG_INFO, "ret[%d] msg_from_cli[%s] msg_from_svr[%s] var_from_svr[%d]",
            // iRet, sMsgFromCli.c_str(), sMsgFromSvr.c_str(), iVar);
    }
    if (iErr > 0) cout << iErr << endl;
    // cout << iTot << " " << iErr << " " << iErr1 << endl;
    return NULL;
}

int main() {
    int iP = 100;
    int iT = 100;
    for (int i=0; i<iP; ++i) {
        if (fork() == 0) {
            vector<pthread_t> vp;
            for (int j=0; j<iT; ++j) {
                pthread_t ptid;
                if (pthread_create(&ptid, NULL, pFunc, NULL) == 0)
                    vp.push_back(ptid);
                else --j;
            }
            for (int j=0; j<iT; ++j) {
                pthread_join(vp[j], NULL);
            }
            return 0;
        }
    }
    // int iTot = 100;
    // int iErr = 0;
    // int iErr1 = 0;
    // int iRet = 0;
    // DemosvrClient cli;
    // string sMsgFromCli = "hello";
    // string sMsgFromSvr;
    // int iVar;
    // iRet = cli.Echo(sMsgFromCli, sMsgFromSvr, iVar);
    // if (iRet < 0) iErr++;
    // if (iRet == -1) iErr1++;
    // cout << sMsgFromCli << " " << sMsgFromSvr << " " << iVar << endl;
    // //HayLog(LOG_INFO, "ret[%d] msg_from_cli[%s] msg_from_svr[%s] var_from_svr[%d]",
            // // iRet, sMsgFromCli.c_str(), sMsgFromSvr.c_str(), iVar);
    // cout << iTot << " " << iErr << " " << iErr1 << endl;
    return 0;
}
