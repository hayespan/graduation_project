#pragma once

struct HaysvrErrno {

    enum {
        Ok = 0, 
        InitConfigFail = -9001,
        NoProtoOption = -9002,
        SerializeInbuf = -9003,
        SerializeOutbuf = -9004,
        DeSerializeInbuf = -9005,
        DeSerializeOutbuf = -9006,
        InternalErr = -9007,
        CreateSocketFail = -9009,
        ConnectFail = -9010,
        WriteFail = -9011,
        ReadFail = -9012,
    };

};
