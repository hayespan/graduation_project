
#include "demosvrmeta.h"

int ToBuf(const EchoRequest & req, HayBuf & inbuf) {
    inbuf.m_sBuf = "";
    return req.SerializeToString(&inbuf.m_sBuf)-1;
}

int FromBuf(EchoRequest & req, const HayBuf & inbuf) {
    return req.ParseFromString(inbuf.m_sBuf)-1;
}

int ToBuf(const EchoResponse & resp, HayBuf & outbuf) {
    outbuf.m_sBuf = "";
    return resp.SerializeToString(&outbuf.m_sBuf)-1;
}

int FromBuf(EchoResponse & resp, const HayBuf & outbuf) {
    return resp.ParseFromString(outbuf.m_sBuf)-1;
}

