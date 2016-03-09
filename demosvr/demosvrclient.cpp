#include "haysvr/haysvrclient.h"

#include "demosvrclient.h"
#include "demosvrcliproto.h"

using namespace DemosvrClasses;

DemosvrClient::DemosvrClient(const string & sClientConfigPath)  
    :HaysvrClient(sClientConfigPath, new DemosvrCliProto()) {
    
}

DemosvrClient::~DemosvrClient() {
}

int DemosvrClient::Echo(const string & sMsgIn, string & sMsgOut, int & iVar) {
    int iRet = 0;
    EchoRequest req;
    EchoResponse resp;
    req.set_msg(sMsgIn);
    resp.set_msg(sMsgIn);
    resp.set_var(999);
    iRet = ((DemosvrCliProto &)GetCliProto()).Echo(req, resp);
    if (iRet < 0) { // haysvr err
        HayLog(LOG_ERR, "%s %s cliproto fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return -1;
    } else if ((iRet=GetCliProto().GetResponseCode()) < 0) { // user err
        HayLog(LOG_ERR, "%s %s fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return -2;
    }
    // succ
    sMsgOut = resp.msg();
    iVar = resp.var();
    return 0;
}

