#include <sstream>
#include <climits>
#include <cstdlib>
#include <errno.h>
#include <string>

#include "string_util.h"

using namespace std;

namespace HayComm {

    unsigned long StrToUInt(const string & sStr, unsigned long ulDefault) {
        unsigned long ulx;
        ulx = strtoul(sStr.c_str(), NULL, 10);
        if (ulx == ULONG_MAX && errno == ERANGE) {
            return ulDefault;
        } 
        return ulx;
    }

    long StrToInt(const string & sStr, long lDefault) {
        long lx;
        lx = strtol(sStr.c_str(), NULL, 10);
        if ((lx == LONG_MAX || lx == LONG_MIN) && errno == ERANGE) {
            return lDefault;
        } 
        return lx;
    }

    string UIntToStr(unsigned long ulx) {
        ostringstream ostr;
        ostr << ulx;
        return ostr.str();
    }

    string IntToStr(long lx) {
        ostringstream ostr;
        ostr << lx;
        return ostr.str();
    }

    unsigned long long StrToULL(const string & sStr, unsigned long long ullDefault) {
        unsigned long long ullx;
        ullx = strtoull(sStr.c_str(), NULL, 10);
        if (ullx == ULLONG_MAX && errno == ERANGE) {
            return ullDefault;
        } 
        return ullx;
    }

    long long StrToLL(const string & sStr, long long llDefault) {
        long long llx;
        llx = strtoll(sStr.c_str(), NULL, 10);
        if ((llx == LLONG_MAX || llx == LLONG_MIN) && errno == ERANGE) {
            return llDefault;
        } 
        return llx;
    }

    string ULLToStr(unsigned long long ullx) {
        ostringstream ostr;
        ostr << ullx;
        return ostr.str();
    }

    string LLToStr(long long llx) {
        ostringstream ostr;
        ostr << llx;
        return ostr.str();
    }
};
