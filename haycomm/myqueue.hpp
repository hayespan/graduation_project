/*
 * Queue.h
 *
 *  Created on: 2013-4-9
 *      Author: sunshaolei
 */

#ifndef MYQUEUE_H_
#define MYQUEUE_H_

#include <deque>
#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>

using namespace std;

template<class DataType>

class MyQueue {
public:
    MyQueue(int count=1024):_nready(0) {
        this->count = count;

        pthread_mutex_init(&_not_full_mutex, NULL);
        pthread_mutex_init(&_not_empty_mutex, NULL);
        pthread_cond_init(&_not_full_cond, NULL);
        pthread_cond_init(&_not_empty_cond, NULL);
    }

    int push(DataType &d) {

        pthread_mutex_lock(&_not_full_mutex);
        while (_nready >= count)
            pthread_cond_wait(&_not_full_cond, &_not_full_mutex);

        _queue.push_back(d);
        _nready++;
        pthread_cond_signal(&_not_empty_cond);
        pthread_mutex_unlock(&_not_full_mutex);

        return 0;
    }

    int pop(DataType &d) {
        pthread_mutex_lock(&_not_empty_mutex);
        while (_nready <= 0)
            pthread_cond_wait(&_not_empty_cond, &_not_empty_mutex);

        d = _queue.front();
        _queue.pop_front();
        _nready--;
        pthread_cond_signal(&_not_full_cond);
        pthread_mutex_unlock(&_not_empty_mutex);

        return 0;

    }

private:
    int _nready;
    int count;
    pthread_mutex_t _not_full_mutex;
    pthread_mutex_t _not_empty_mutex;
    pthread_cond_t _not_full_cond;
    pthread_cond_t _not_empty_cond;

    deque<DataType> _queue;
};

#endif /* QUEUE_H_ */

