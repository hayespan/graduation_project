#pragma once

#include <string>

using std::string;

class HayBuf {

public:
    HayBuf();
    virtual ~HayBuf();
    void CopyFrom(const string & s, size_t iPos, int iLen); 

public:
    string m_sBuf;
};

