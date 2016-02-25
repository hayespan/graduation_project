#pragma once

#include <pthread.h>

namespace HayComm {

    class LockGuard {
    public:
        LockGuard(pthread_mutex_t & tOthLock);
        ~LockGuard();
    private:
        LockGuard(const LockGuard &);
        LockGuard & operator=(const LockGuard &);
        
        pthread_mutex_t & m_tLock;
    };

};
