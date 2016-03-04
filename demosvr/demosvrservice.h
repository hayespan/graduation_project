#include "demosvrclass.pb.h"

using namespace DemosvrClasses;

class DemosvrService {

public:
    int Echo(const EchoRequest & req, EchoResponse & resp);

};
