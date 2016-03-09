#include "demosvrservice.h"

int DemosvrService::Echo(const EchoRequest & req, EchoResponse & resp) {
    resp.set_msg(req.msg()+" hayes!");
    resp.set_var(998);
    return 0;
}
