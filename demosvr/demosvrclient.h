#pragma once

#include "haysvr/haysvrclient.h"

class DemosvrClient: public HaysvrClient {

public:
    DemosvrClient(const string & sClientConfigPath="./demosvrcli.conf");
    virtual ~DemosvrClient();

    int Echo(const string & sMsgIn, string & sMsgOut, int & iVar);

};
