#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <pthread.h>
#include <list>
#include <locker.h>
#include <exception>
#include <cstdio>

#include <sys/epoll.h>


/*
    class template
    T - the template parameter, which defines the type of mission
*/ 
template<typename T>
class threadpool
{
public:
    threadpool(int thread_number = 8, int max_request = 10000);
    ~threadpool();
    bool append(T* request);

private:
    static void* worker(void* arg);
    void run();

private:

    int m_thread_number;      // number of threads in the queue

    pthread_t* m_threads;  // an array of threads of size n_thread_number  

    int m_max_requests;    // max number of requests in the queue

    std::list<T*> m_workqueue;  // request queue

    locker m_queuelocker;

    sem m_queuestat;    // ???????????????????

    bool m_stop;     // a stop flag

};

/* 
    Define member functions of a class template 
    [className]<[template parameter]>[member function name](function paremeter)
*/

template<typename T>
threadpool<T>::threadpool(int thread_number, int max_request) : 
m_thread_number(thread_number), m_max_requests(max_request), 
m_stop(false), m_threads(NULL)
{
    if(thread_number <=0 || max_request <= 0)
    {
        throw std::exception();
    }

    m_threads = new pthread_t[m_thread_number];

    if(!m_threads)
    {
        throw std::exception();
    }

    // create (int thread_number) threads, and set them as
    for(int i = 0; i < thread_number; i++)
    {
        printf("create the %dth thread\n", i);
        if( pthread_create(m_threads + i, NULL, worker, this) != 0 )
        {
            delete[] m_threads;
            throw std::exception();
        }

        if(pthread_detach(m_threads[i]))
        {
            delete[] m_threads;
            throw std::exception();
        }
        
    }
}

template<typename T>
threadpool<T>::~threadpool()
{
    delete[] m_threads;
    m_stop = true;
}

template<typename T>
bool threadpool<T>::append(T* request)
{
    m_queuelocker.lock();
    if(m_workqueue.size() > m_max_requests)
    {
        m_max_request.unlock();
        return false;
    }

    m_workqueue.push_back(request);
    m_queuelcoker.unlock();
    m_queuestat.post();
    return true;
}

template<typename T>
void* threadpool<T>::worker(void* arg)
{
    threapool* pool = (threadpool*) arg;
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

        reqeust->process();
    }
}


#endif