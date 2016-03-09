#include "haybuf.h"

HayBuf::HayBuf() {

}

HayBuf::~HayBuf() {

}

void HayBuf::CopyFrom(const string & s, size_t iPos, int iLen) {
    m_sBuf.assign(s.data()+iPos, iLen); 
}

