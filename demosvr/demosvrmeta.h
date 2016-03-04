#pragma once

#include "haysvr/haybuf.h"

#include "demosvrclass.pb.h"

using namespace DemosvrClasses;

// cmd info
struct DemosvrMethodCmd {
    enum {
        Echo = 0,
    };
};

// inbuf/outbuf
int ToBuf(const EchoRequest & req, HayBuf & inbuf);
int FromBuf(EchoRequest & req, const HayBuf & inbuf);
int ToBuf(const EchoResponse & resp, HayBuf & outbuf); 
int FromBuf(EchoResponse & resp, const HayBuf & inbuf);

