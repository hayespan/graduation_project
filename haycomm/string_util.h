#pragma once

#include <string>

using std::string;

namespace HayComm {

    unsigned long StrToUInt(const string & sStr, unsigned long ulDefault=0);
    long StrToInt(const string & sStr, long lDefault=0);
    string UIntToStr(unsigned long ulx);
    string IntToStr(long lx);
    unsigned long long StrToULL(const string & sStr, unsigned long long ullDefault=0);
    long long StrToLL(const string & sStr, long long llDefault=0);
    string ULLToStr(unsigned long long ullx);
    string LLToStr(long long llx);

};
