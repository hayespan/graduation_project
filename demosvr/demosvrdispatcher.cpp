#include "haycomm/haylog.h"
#include "haysvr/haysvrerrno.h"

#include "demosvrmeta.h"
#include "demosvrdispatcher.h"

int DemosvrDispatcher::DoDispatch(int iCmd, const HayBuf & inbuf, HayBuf & outbuf, int & iResponseCode) {
    int iRet = 0;
    if (iCmd == DemosvrMethodCmd::Echo) {
        EchoRequest req;
        EchoResponse resp;
        // frombuf
        iRet = FromBuf(req, inbuf); 
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s %s inbuf deserilize fail. ret[%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerilizeInbuf;
        }
        iRet = FromBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s %s outbuf deserilize fail. ret[%d]", 
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerilizeOutbuf;
        }
        // service
        iResponseCode = oService.Echo(req, resp);
        // tobuf
        iRet = ToBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s %s outbuf serilize fail. ret[%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::SerilizeOutbuf;
        }
    } else {
        
    }
    return HaysvrErrno::Ok;
}
