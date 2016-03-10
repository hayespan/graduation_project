#pragma once

#include <deque>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

template<class DataType>
class MyQueue {
public:
    MyQueue(int count=1024):_nready(0) {
        this->count = count;
        pthread_mutex_init(&_mutex, NULL);
        pthread_cond_init(&_not_full_cond, NULL);
        pthread_cond_init(&_not_empty_cond, NULL);
    }

    int push(const DataType &d) {
        pthread_mutex_lock(&_mutex);
        while (_nready >= count)
            pthread_cond_wait(&_not_full_cond, &_mutex);

        _queue.push_back(d);
        _nready++;
        pthread_cond_signal(&_not_empty_cond);
        pthread_mutex_unlock(&_mutex);
        return 0;
    }

    int pop(DataType &d) {
        pthread_mutex_lock(&_mutex);
        while (_nready <= 0)
            pthread_cond_wait(&_not_empty_cond, &_mutex);

        d = _queue.front();
        _queue.pop_front();
        _nready--;
        pthread_cond_signal(&_not_full_cond);
        pthread_mutex_unlock(&_mutex);
        return 0;
    }

private:

    int _nready;
    int count;
    pthread_mutex_t _mutex;
    pthread_cond_t _not_full_cond;
    pthread_cond_t _not_empty_cond;

    deque<DataType> _queue;
};

