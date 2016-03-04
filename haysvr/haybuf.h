#pragma once

#include <string>

using std::string;

class HayBuf {

public:
    HayBuf();
    virtual ~HayBuf();
    
private:
    string m_sBuf;
};

