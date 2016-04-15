#!/usr/bin/env python

import sys
import os
import json
import copy

gener_task_list = []

def _extract_gener_postfix(f):
    postfix = f.__name__[6:].replace('_', '.')
    return postfix

def _get_svr_name_prefix(svr_name):
    return svr_name[0].upper() + svr_name[1:]

def gener_file(f):
    gener_task_list.append(f)
    return f

@gener_file
def gener_class_proto(svr_name, methods):
    class_proto = svr_name + _extract_gener_postfix(gener_class_proto)
    if os.access(class_proto, os.R_OK):
        return
    tmpl = """package %sClasses;

message EchoRequest {
    required string msg = 1;
}

message EchoResponse {
    required string msg = 1;
    required int32 var = 2;
}
""" % _get_svr_name_prefix(svr_name)
    with open(class_proto, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_cliproto_h(svr_name, methods):
    cliproto_h = svr_name + _extract_gener_postfix(gener_cliproto_h)
    svr_prefix = _get_svr_name_prefix(svr_name)
    method_tmpl = ''
    methods_str = ''
    method_tmpl = """
    int %s(const %s & req, %s & resp);
"""
    for i in methods:
        methods_str += method_tmpl % (
                i[0], i[1], i[2],
                )
    tmpl = """#pragma once

#include "haysvr/haysvrcliproto.h"

#include "%sclass.pb.h"

using namespace %sClasses;

class %sCliProto: public HaysvrCliProto {

public:
    %sCliProto();
    virtual ~%sCliProto();
    %s
};
""" % (
            svr_name,
            svr_prefix,
            svr_prefix,
            svr_prefix,
            svr_prefix,
            methods_str,
            )
    with open(cliproto_h, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_cliproto_cpp(svr_name, methods):
    cliproto_cpp = svr_name + _extract_gener_postfix(gener_cliproto_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)
    method_tmpl = """
int %(svr_prefix)sCliProto::%(method_name)s(const %(request_name)s & req, %(response_name)s & resp) {
    int iRet = 0;
    HayBuf inbuf, outbuf;
    // tobuf
    iRet = ToBuf(req, inbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%%s %%s inbuf serialize fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::SerializeInbuf;
    }
    HayLog(LOG_DBG, "hayespan cliproto inbuf-size[%%d]", inbuf.m_sBuf.size());
    iRet = ToBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%%s %%s outbuf serialize fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::SerializeOutbuf;
    }
    HayLog(LOG_DBG, "hayespan cliproto outbuf-size[%%d]", outbuf.m_sBuf.size());
    // do call
    iRet = DoProtoCall(%(svr_prefix)sMethodCmd::%(method_name)s, inbuf, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%%s %%s DoProtoCall fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::InternalErr;
    }
    // frombuf
    iRet = FromBuf(resp, outbuf);
    if (iRet < 0) {
        HayLog(LOG_ERR, "%%s %%s outbuf deserialize fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return HaysvrErrno::DeSerializeOutbuf;
    }
    return HaysvrErrno::Ok;
}
"""
    methods_str = ''
    for i in methods:
        methods_str += method_tmpl % {
                'svr_name': svr_name,
                'svr_prefix': svr_prefix,
                'method_name': i[0],
                'request_name': i[1],
                'response_name': i[2],
                }
    tmpl = """#include "haysvr/haylog.h"
#include "haysvr/haysvrerrno.h"

#include "%(svr_name)scliproto.h"
#include "%(svr_name)smeta.h"

using namespace %(svr_prefix)sClasses;

%(svr_prefix)sCliProto::%(svr_prefix)sCliProto() {

}

%(svr_prefix)sCliProto::~%(svr_prefix)sCliProto() {

}
%(methods_str)s
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        'methods_str': methods_str,
        }
    with open(cliproto_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_client_h(svr_name, methods):
    client_h = svr_name + _extract_gener_postfix(gener_client_h)
    svr_prefix = _get_svr_name_prefix(svr_name)
    tmpl = """#pragma once

#include "haysvr/haysvrclient.h"

class %(svr_prefix)sClient: public HaysvrClient {

public:
    %(svr_prefix)sClient(const string & sClientConfigPath="/home/panhzh3/file/graduation_project/%(svr_name)s/%(svr_name)scli.conf");
    virtual ~%(svr_prefix)sClient();

    int Echo(const string & sMsgIn, string & sMsgOut, int & iVar);

};
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(client_h, os.F_OK):
        return
    with open(client_h, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_client_cpp(svr_name, methods):
    client_cpp = svr_name + _extract_gener_postfix(gener_client_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)
    tmpl = """#include "haysvr/haysvrclient.h"

#include "%(svr_name)sclient.h"
#include "%(svr_name)scliproto.h"

using namespace %(svr_prefix)sClasses;

%(svr_prefix)sClient::%(svr_prefix)sClient(const string & sClientConfigPath)  
    :HaysvrClient(sClientConfigPath, new %(svr_prefix)sCliProto()) {
    
}

%(svr_prefix)sClient::~%(svr_prefix)sClient() {
}

int %(svr_prefix)sClient::Echo(const string & sMsgIn, string & sMsgOut, int & iVar) {
    int iRet = 0;
    EchoRequest req;
    EchoResponse resp;
    req.set_msg(sMsgIn);
    resp.set_msg(sMsgIn);
    resp.set_var(999);
    iRet = ((%(svr_prefix)sCliProto &)GetCliProto()).Echo(req, resp);
    if (iRet < 0) { // haysvr err
        HayLog(LOG_ERR, "%%s %%s cliproto fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return -1;
    } else if ((iRet=GetCliProto().GetResponseCode()) < 0) { // user err
        HayLog(LOG_ERR, "%%s %%s fail. ret[%%d]",
                __FILE__, __PRETTY_FUNCTION__, iRet);
        return -2;
    }
    // succ
    sMsgOut = resp.msg();
    iVar = resp.var();
    return 0;
}

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(client_cpp, os.F_OK):
        return
    with open(client_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_meta_h(svr_name, methods):
    meta_h = svr_name + _extract_gener_postfix(gener_meta_h)
    svr_prefix = _get_svr_name_prefix(svr_name)

    cmd_enum_field_tmpl = """
        %(method_name)s = %(cmd_num)d,
"""
    method_req_tmpl = """
int ToBuf(const %(request_name)s & req, HayBuf & inbuf);
int FromBuf(%(request_name)s& req, const HayBuf & inbuf);
"""
    method_resp_tmpl = """
int ToBuf(const %(response_name)s & resp, HayBuf & outbuf);
int FromBuf(%(response_name)s& resp, const HayBuf & outbuf);
"""
    cmds_str = ''
    methods_str = ''
    for index, i in enumerate(methods):
        cmds_str += cmd_enum_field_tmpl % {
                'method_name': i[0],
                'cmd_num': index,
                }
        methods_str += method_req_tmpl % {
                'request_name': i[1],
                }
        methods_str += method_resp_tmpl % {
                'response_name': i[2],
                }

    tmpl = """#pragma once

#include "haysvr/haybuf.h"

#include "%(svr_name)sclass.pb.h"

using namespace %(svr_prefix)sClasses;

// cmd info
struct %(svr_prefix)sMethodCmd {
    enum {
%(cmds_str)s
    };
};

// inbuf/outbuf
%(methods_str)s
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        'cmds_str': cmds_str,
        'methods_str': methods_str,
        }
    with open(meta_h, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_meta_cpp(svr_name, methods):
    meta_cpp = svr_name + _extract_gener_postfix(gener_meta_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)

    method_req_tmpl = """
int ToBuf(const %(request_name)s & req, HayBuf & inbuf) {
    inbuf.m_sBuf = "";
    return req.SerializeToString(&inbuf.m_sBuf)-1;
}

int FromBuf(%(request_name)s & req, const HayBuf & inbuf) {
    return req.ParseFromString(inbuf.m_sBuf)-1;
}
"""
    method_resp_tmpl = """
int ToBuf(const %(response_name)s & resp, HayBuf & outbuf) {
    outbuf.m_sBuf = "";
    return resp.SerializeToString(&outbuf.m_sBuf)-1;
}

int FromBuf(%(response_name)s & resp, const HayBuf & outbuf) {
    return resp.ParseFromString(outbuf.m_sBuf)-1;
}
"""
    methods_str = ''
    for i in methods:
        methods_str += method_req_tmpl % {
                'request_name': i[1],
                }
        methods_str += method_resp_tmpl % {
                'response_name': i[2],
                }

    tmpl = """#include "%(svr_name)smeta.h"
%(methods_str)s
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        'methods_str': methods_str,
        }
    with open(meta_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_dispatcher_h(svr_name, methods):
    dispatcher_h = svr_name + _extract_gener_postfix(gener_dispatcher_h)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """#pragma once

#include "haysvr/haysvrdispatcher.h"

#include "%(svr_name)sservice.h"

class %(svr_prefix)sDispatcher: public HaysvrDispatcher {
    
public:

    virtual int DoDispatch(int iCmd, const HayBuf & inbuf, HayBuf & outbuf, int & iResponseCode);

private:
    %(svr_prefix)sService oService;
};

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    with open(dispatcher_h, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_dispatcher_cpp(svr_name, methods):
    dispatcher_cpp = svr_name + _extract_gener_postfix(gener_dispatcher_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)

    cmd_tmpl = """
        EchoRequest req;
        EchoResponse resp;

        // frombuf
        iRet = FromBuf(req, inbuf); 
        if (iRet < 0) {
            HayLog(LOG_ERR, "%%s %%s inbuf deserialize fail. ret[%%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerializeInbuf;
        }
        iRet = FromBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%%s %%s outbuf deserialize fail. ret[%%d]", 
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::DeSerializeOutbuf;
        }

        // service
        iResponseCode = oService.%(method_name)s(req, resp);

        // tobuf
        iRet = ToBuf(resp, outbuf);
        if (iRet < 0) {
            HayLog(LOG_ERR, "%%s %%s outbuf serialize fail. ret[%%d]",
                    __FILE__, __PRETTY_FUNCTION__, iRet);
            return HaysvrErrno::SerializeOutbuf;
        }
    """
    if_tmpl = """
    if (iCmd == %(svr_prefix)sMethodCmd::%(method_name)s) {
"""
    elif_tmpl = """
    } else if (iCmd == %(svr_prefix)sMethodCmd::%(method_name)s) {
"""
    else_tmpl = """
    } else {

    }
"""
    if_elif_else_str = ''
    for index, i in enumerate(methods):
        if index == 0:
            if_elif_else_str += if_tmpl % {
                    'svr_prefix': svr_prefix,
                    'method_name': i[0],
                    }
        else:
            if_elif_else_str += elif_tmpl % {
                    'svr_prefix': svr_prefix,
                    'method_name': i[0],
                    }
        if_elif_else_str += cmd_tmpl % {
                'method_name': i[0],
                }
    if_elif_else_str += else_tmpl
    method_tmpl = """
int %(svr_prefix)sDispatcher::DoDispatch(int iCmd, const HayBuf & inbuf, HayBuf & outbuf, int & iResponseCode) {
    int iRet = 0;
%(if_elif_else_str)s
    return HaysvrErrno::Ok;
}
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        'if_elif_else_str': if_elif_else_str,
        }
    tmpl = """#include "haysvr/haylog.h"
#include "haysvr/haysvrerrno.h"

#include "%(svr_name)smeta.h"
#include "%(svr_name)sdispatcher.h"
%(method_tmpl)s
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        'method_tmpl': method_tmpl,
        }
    with open(dispatcher_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_service_h(svr_name, methods):
    service_h = svr_name + _extract_gener_postfix(gener_service_h)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """#include "%(svr_name)sclass.pb.h"

using namespace %(svr_prefix)sClasses;

class %(svr_prefix)sService {

public:

    int Echo(const EchoRequest & req, EchoResponse & resp);

};

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(service_h, os.F_OK):
        return
    with open(service_h, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_service_cpp(svr_name, methods):
    service_cpp = svr_name + _extract_gener_postfix(gener_service_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """#include "%(svr_name)sservice.h"

int %(svr_prefix)sService::Echo(const EchoRequest & req, EchoResponse & resp) {
    resp.set_msg(req.msg()+" hayes!");
    resp.set_var(998);
    return 0;
}

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(service_cpp, os.F_OK):
        return
    with open(service_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_main_cpp(svr_name, methods):
    main_cpp = svr_name + _extract_gener_postfix(gener_main_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """#include "haysvr/haysvr.h"
#include "haysvr/tcpsvr.h"

#include "%(svr_name)sdispatcher.h"

int main(int argc, char ** argv) {

    const string sSvrConfigPath = "./%(svr_name)s.conf";
    %(svr_prefix)sDispatcher oDispatcher;
    TcpSvr oTcpSvr;

    Haysvr oSvr(sSvrConfigPath, &oTcpSvr, &oDispatcher);
    oSvr.Run();

    return 0;
}
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(main_cpp, os.F_OK):
        return
    with open(main_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_cli_conf(svr_name, methods):
    cli_conf = svr_name + _extract_gener_postfix(gener_cli_conf)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """# config for client

[client]
connect_timeout=5000
read_timeout=60000
svr_ip=127.0.0.1
svr_port=34567

[log]
debug_mode=0

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(cli_conf, os.F_OK):
        return
    with open(cli_conf, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener__conf(svr_name, methods):
    _conf = svr_name + _extract_gener_postfix(gener__conf)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """# config for server

[server]
port=34567
mastercnt=4
workercnt=8
max_conn_cnt_per_process=1000
req_queue_size=1000

[log]
debug_mode=0

""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(_conf, os.F_OK):
        return
    with open(_conf, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_clienttest_cpp(svr_name, methods):
    clienttest_cpp = svr_name + _extract_gener_postfix(gener_clienttest_cpp)
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """#include "%(svr_name)sclient.h"
#include <iostream>

using namespace std;

int TestEcho() {
    int iRet = 0;
    %(svr_prefix)sClient cli;
    string sMsgFromCli = "hello";
    string sMsgFromSvr;
    int iVar;
    iRet = cli.Echo(sMsgFromCli, sMsgFromSvr, iVar);
    cout << "ret = " << iRet << endl
        << sMsgFromCli << endl
        << sMsgFromSvr << endl
        << iVar << endl;
    return iRet;
}

int main(int argc, char ** argv) {
    TestEcho();
    return 0;
}
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(clienttest_cpp, os.F_OK):
        return
    with open(clienttest_cpp, 'w+') as f:
        f.write(tmpl)

@gener_file
def gener_Makefile(svr_name, methods):
    Makefile = 'Makefile'
    svr_prefix = _get_svr_name_prefix(svr_name)

    tmpl = """include ../Makefile.comm
COMM_OBJS = %(svr_name)smeta.o %(svr_name)sclass.pb.o
CLI_OBJS = %(svr_name)sclient.o %(svr_name)scliproto.o $(COMM_OBJS)
SVR_OBJS = %(svr_name)smain.o %(svr_name)sdispatcher.o %(svr_name)sservice.o $(COMM_OBJS)
TEST_OBJS = %(svr_name)sclienttest.o

all: CXXFLAGS += -I$(ROOT_DIR) 
all: %(svr_name)sclient %(svr_name)s %(svr_name)sclienttest
all: INCLS += -I$(ROOT_DIR)
all: DEPS += -lprotobuf -L$(DYNAMIC_DIR) -lhaysvr -lhaycomm

%(svr_name)ssrc:
	protoc %(svr_name)sclass.proto --cpp_out=.

.PHONY: %(svr_name)sclient
%(svr_name)sclient: %(svr_name)ssrc lib%(svr_name)sclient.so lib%(svr_name)sclient.a

lib%(svr_name)sclient.so: $(CLI_OBJS)
	$(CXX_DYNAMIC)
lib%(svr_name)sclient.a: $(CLI_OBJS)
	$(AR_STATIC)

%(svr_name)s: CXXFLAGS += -static
%(svr_name)s: INCLS = 
%(svr_name)s: DEPS = -lprotobuf -L$(STATIC_DIR) -lhaysvr -lhaycomm -lpthread
%(svr_name)s: $(SVR_OBJS) 
	$(CXX_EXE)

%(svr_name)sclienttest: DEPS += -L$(DYNAMIC_DIR) -l%(svr_name)sclient -lhaysvrcli -lhaycomm
%(svr_name)sclienttest: $(TEST_OBJS)
	$(CXX_EXE)

.PHONY: clean
clean:
	rm -f *.o;
	rm -f *.so;
	rm -f *.a;
	rm -f %(svr_name)s;
	rm -f %(svr_name)sclienttest;
""" % {
        'svr_name': svr_name,
        'svr_prefix': svr_prefix,
        }
    if os.access(Makefile, os.F_OK):
        return
    with open(Makefile, 'w+') as f:
        f.write(tmpl)


def run_main():
    if len(sys.argv) < 2:
        print >> sys.stderr, 'usage: %s metafile' % sys.argv[0]
        return -1
    filename = sys.argv[1]
    with open(filename, 'r') as metafile:
        metadata = json.loads(metafile.read())    
        svr_name = metadata.get('svr_name')
        methods = metadata.get('methods', )
        methods_tmp = copy.deepcopy(methods)
        methods_tmp.insert(0, ['Echo', 
            'EchoRequest', 'EchoResponse', ])
        if not svr_name:
            print >> sys.stderr, 'err: no svr_name'
            return -1
        for gener_task in gener_task_list:
            gener_task(svr_name, methods_tmp)

if __name__ == '__main__':
    run_main()
