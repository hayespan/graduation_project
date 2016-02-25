#include "lockguard.h"

namespace HayComm {
    
    LockGuard::LockGuard(pthread_mutex_t & tOthLock) 
        :m_tLock(tOthLock) {
        pthread_mutex_lock(&m_tLock);
    }

    LockGuard::~LockGuard() {
        pthread_mutex_unlock(&m_tLock);
    }

}
