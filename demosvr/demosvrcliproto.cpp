#include "haysvr/haylog.h"
#include "haysvr/haysvrerrno.h"

#include "demosvrcliproto.h"
#include "demosvrmeta.h"

using namespace DemosvrClasses;

DemosvrCliProto::DemosvrCliProto() {

}

DemosvrCliProto::~DemosvrCliProto() {

}

int DemosvrCliProto::Echo(const EchoRequest & req, EchoResponse & resp) {
    int iRet = 0;
    HayBuf inbuf, outbuf;
    // tobuf
    iRet = ToBuf(req, inbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s inbuf serialize fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::SerializeInbuf;
    }
    HayLog(LOG_DBG, "hayespan cliproto inbuf-size[%d]", inbuf.m_sBuf.size());
    iRet = ToBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s outbuf serialize fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::SerializeOutbuf;
    }
    HayLog(LOG_DBG, "hayespan cliproto outbuf-size[%d] outbuf[%c%c%c]", outbuf.m_sBuf.size(),
            outbuf.m_sBuf[0], outbuf.m_sBuf[1], outbuf.m_sBuf[3]);
    // do call
    iRet = DoProtoCall(DemosvrMethodCmd::Echo, inbuf, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s DoProtoCall fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::InternalErr;
    }
    // frombuf
    iRet = FromBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%s %s outbuf deserialize fail. ret[%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::DeSerializeOutbuf;
    }
    return HaysvrErrno::Ok;
}

