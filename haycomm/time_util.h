#pragma once

#include <string>
#include <time.h>

using std::string;

namespace HayComm {

    time_t GetNowTimestamp();
    
    string Timestamp2Str(time_t tTime, const string & sFormat="%Y-%m-%d %H:%M:%S");

    time_t Str2Timestamp(const string & sTimeStr, const string & sFormat);

}
