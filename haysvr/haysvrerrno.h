#pragma once

struct HaysvrErrno {

    enum {
        Ok = 0, 
        InitConfigFail = -9001,
        NoProtoOption = -9002,
        SerilizeInbuf = -9003,
        SerilizeOutbuf = -9004,
        DeSerilizeInbuf = -9005,
        DeSerilizeOutbuf = -9006,
        ClientCall = -9007,
    };

};
