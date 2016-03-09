#pragma once

#include "haysvr/haysvrcliproto.h"

#include "demosvrclass.pb.h"

using namespace DemosvrClasses;

class DemosvrCliProto: public HaysvrCliProto {

public:
    DemosvrCliProto();
    virtual ~DemosvrCliProto();
    
    int Echo(const EchoRequest & req, EchoResponse & resp);    

};
