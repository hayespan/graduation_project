#pragma once

#include "haysvr/haysvrdispatcher.h"

#include "demosvrservice.h"

class DemosvrDispatcher: public HaysvrDispatcher {
    
public:

    virtual int DoDispatch(int iCmd, const HayBuf & inbuf, HayBuf & outbuf, int & iResponseCode);

private:
    DemosvrService oService;
};
