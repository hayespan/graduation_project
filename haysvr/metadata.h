#pragma once

struct MetaData {
    int iCmd;
    int iAttr;
    int iReqtLen;
    int iRespLen;
    int iSvrErrno;
    int iRespCode;

    MetaData();
};
