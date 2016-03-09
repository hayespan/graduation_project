#include "haysvr/haylog.h"
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
            HayLog(LOG_ERR, "%s %s inbuf deserialize fail. ret[%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerializeInbuf;
        }
        iRet = FromBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s %s outbuf deserialize fail. ret[%d]", 
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerializeOutbuf;
        }

        // service
        iResponseCode = oService.Echo(req, resp);

        // tobuf
        iRet = ToBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%s %s outbuf serialize fail. ret[%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::SerializeOutbuf;
        }

    } else {
        
    }
    return HaysvrErrno::Ok;
}
