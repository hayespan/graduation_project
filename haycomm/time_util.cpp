#include "time_util.h"

namespace HayComm {

    time_t GetNowTimestamp() {
        time_t tNow;
        time(&tNow);
        return tNow;
    }

    string Timestamp2Str(time_t tTime, const string & sFormat) {
        char lsBuf[64];
        struct tm tTm;
        localtime_r(&tTime, &tTm);
        strftime(lsBuf, sizeof(lsBuf), sFormat.c_str(), &tTm);
        return lsBuf;
    }

    time_t Str2Timestamp(const string & sTimeStr, const string & sFormat) {
        struct tm tTm;
        strptime(sTimeStr.c_str(), sFormat.c_str(), &tTm);
        return mktime(&tTm);
    }


}
