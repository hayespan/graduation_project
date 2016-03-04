#include "haycomm/haylog.h"
#include "haysvr/haysvrerrno.h"

#include "demosvrcliproto.h"
#include "demosvrmeta.h"

using namespace DemosvrClasses;

DemosvrCliProto::DemosvrCliProto() {

}

DemosvrCliProto::~DemosvrCliProto() {

}

int DemosvrCliProto::Echo(TcpChannel & oTcpChannel, const EchoRequest & req, EchoResponse & resp) {
    int iRet = 0;
    HayBuf inbuf, outbuf;
    // tobuf
    iRet = ToBuf(req, inbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s inbuf serilize fail. ret[%d]",
                __FILE__, __func__, iRet);
        return HaysvrErrno::SerilizeInbuf;
    }
    iRet = ToBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s outbuf serilize fail. ret[%d]",
                __FILE__, __func__, iRet);
        return HaysvrErrno::SerilizeOutbuf;
    }
    // do call
    iRet = DoProtoCall(oTcpChannel, DemosvrMethodCmd::Echo, inbuf, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s DoProtoCall fail. ret[%d]",
                __FILE__, __func__, iRet);
        return HaysvrErrno::ClientCall;
    }
    // frombuf
    iRet = FromBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s outbuf deserilize fail. ret[%d]",
                __FILE__, __func__, iRet);
        return HaysvrErrno::DeSerilizeOutbuf;
    }
    return HaysvrErrno::Ok;
}

