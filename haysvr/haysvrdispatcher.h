#pragma once

#include "haybuf.h"

class HaysvrDispatcher {

public:
    HaysvrDispatcher();
    virtual ~HaysvrDispatcher();

    virtual int DoDispatch(int iCmd, const HayBuf & inbuf, HayBuf & outbuf, int & iResponseCode) = 0;

private:
    HaysvrDispatcher(const HaysvrDispatcher &); 
    HaysvrDispatcher & operator=(const HaysvrDispatcher &);

};

