#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include <semaphore.h>

// thread synchronization mechanism 

class locker
{
public:
    locker() 
    {
        /* if successful, pthread_mutex_init() function shall return 0, 
        otherwise, an error number shall be returned to indicate the error.*/

        if(pthread_mutex_init(&m_mutex, nullptr) != 0)
        {
            throw std::exception();
        }
    }

    ~locker()
    {
        pthread_mutex_destroy(&m_mutex);
    }

    bool lock() 
    {
        return pthread_mutex_lock(&m_mutex) == 0;
    }

    bool unlock()
    {
        return pthread_mutex_unlock(&m_mutex) == 0;
    }

    pthread_mutex_t* get()
    {
        return &m_mutex;
    }


private:
    pthread_mutex_t m_mutex;
};


//
class cond
{
public:
    cond()
    {
        if(pthread_cond_init(&m_cond, nullptr) != 0)
        {
            throw std::exception();
        }
    }

    ~cond()
    {
        pthread_cond_destroy(&m_cond);
    }

    bool wait(pthread_mutex_t* mutex)
    {
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timedwait(pthread_mutex_t* mutex, struct timespec t)
    {
        return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
    }

    bool signal()
    {
        return pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast()
    {
        return pthread_cond_broadcast(&m_cond) == 0;
    } 

private:
    pthread_cond_t m_cond;

};

//
class sem
{
public:


private:
    

};

#endif