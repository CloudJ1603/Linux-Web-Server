#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <memory>
#include <pthread.h>
#include <list>
#include "locker.h"
#include <exception>
#include <cstdio>
#include <sys/epoll.h>

template<typename T>
class threadpool {
public:
    static const int DEFAULT_THREAD_NUMBER = 8;
    static const int DEFAULT_MAX_REQUEST = 10000;

    threadpool(int thread_number = DEFAULT_THREAD_NUMBER, int max_request = DEFAULT_MAX_REQUEST);
    ~threadpool();
    bool append(T* request);
private:
    static void* worker(void* arg);
    void run();

    int m_thread_number;                    // number of threads in the queue
    std::unique_ptr<pthread_t[]> m_threads; // an array of threads of size n_thread_number  
    int m_max_requests;                     // max number of requests in the queue
    std::list<T*> m_workqueue;              // request queue
    locker m_queuelocker;                   // mutex locker
    sem m_queuestat;                        // semaphore status to determine if there is any request to handle
    bool m_stop;                            // a stop flag 
};

/* 
    Define member functions of a class template 
    [className]<[template parameter]>[member function name](function paremeter)
*/

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_request) : 
m_thread_number(thread_number), m_max_requests(max_request), 
m_stop(false), m_threads(NULL) {

    if(thread_number <=0 || max_request <= 0) {
        throw std::exception();
    }

    // m_threads = new pthread_t[m_thread_number]; --> change to unique pointer
    m_threads = std::make_unique<pthread_t[]>(m_thread_number);
    // if fail to creatge the threads array
    if(!m_threads) {
        throw std::exception();
    }

    // create "thread_number" number of threads, and detach them
    for(int i = 0; i < thread_number; i++) {
        printf("create the %dth thread\n", i);
        if( pthread_create(m_threads.get() + i, NULL, worker, this) != 0 ) {
            // delete[] m_threads;
            throw std::exception();
        }
        if(pthread_detach((m_threads.get())[i]) != 0) {
            // delete[] m_threads;
            throw std::exception();
        }
    }
}

template<typename T>
threadpool<T>::~threadpool() {
    // delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request) {
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests) {
        m_queuelocker.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelocker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg) {
    threadpool* pool = (threadpool*) arg;
    pool->run();
    return pool;
}

template<typename T>
void threadpool<T>::run()
{
    while(!m_stop)    // ?????????????????????
    {
        m_queuestat.wait();
        m_queuelocker.lock();
        if(m_workqueue.empty())
        {
            m_queuelocker.unlock();
            continue;
        }

        T* request = m_workqueue.front();
        m_workqueue.pop_front();
        m_queuelocker.unlock();

        if(!request) {
            continue;
        }

        request->process();
    }
}


#endif