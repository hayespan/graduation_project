#include "haysvr/haysvr.h"
#include "haysvr/tcpsvr.h"

#include "demosvrdispatcher.h"

int main(int argc, char ** argv) {

    const string sSvrConfigPath = "./demosvr.conf";
    DemosvrDispatcher oDispatcher;
    TcpSvr oTcpSvr;

    Haysvr oSvr(sSvrConfigPath, &oTcpSvr, &oDispatcher);
    oSvr.Run();

    return 0;
}
