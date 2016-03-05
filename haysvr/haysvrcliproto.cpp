#include "haylog.h"
#include "haysvrcliproto.h"
#include "haysvrerrno.h"

// CliProtoOption
CliProtoOption::CliProtoOption() {
    iConnTimeout = -1;
    iReadTimeout = -1;
    sSvrIp = "";
    iSvrPort = -1;
}

CliProtoOption::~CliProtoOption() {

}

// HaysvrCliProto
HaysvrCliProto::HaysvrCliProto() {
    m_pCliProtoOption = NULL;
    m_iResponseCode = -1;
}

int HaysvrCliProto::DoProtoCall(TcpChannel & oTcpChannel, int iCmd, const HayBuf & inbuf, HayBuf & outbuf) {
    if (m_pCliProtoOption == NULL) {
        return HaysvrErrno::NoProtoOption;
    }

    // TODO

    return HaysvrErrno::Ok; 
}

int HaysvrCliProto::GetResponseCode() {
    return m_iResponseCode;
}

void HaysvrCliProto::SetCliProtoOption(CliProtoOption * pCliProtoOption) {
    m_pCliProtoOption = pCliProtoOption;
}

HaysvrCliProto::~HaysvrCliProto() {
    
}

