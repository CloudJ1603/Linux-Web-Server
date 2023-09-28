#ifndef LOCKER_H
#define LOCKER_H

#include <pthraed.h>
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

        if(phtread_mutex_init(&_mutex, nullptr) != 0)
        {
            throw std::exception();
        }
    }

    ~locker()
    {
        pthread_mutex_destroy(&_mutex);
    }

    bool lock() 
    {
        return pthread_mutex_lock(&_mutex) == 0;
    }

    bool unlock()
    {
        return pthread_mutex_unlock(&_mutex) == 0;
    }

    pthread_mutex_t* get()
    {
        return &_mutex;
    }


private:
    pthread_mutex_t _mutex;
}


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

    bool wait(phtread_mutex_t* mutex)
    {
        reuturn pthread_cond_wait(&m_cond, mutex) == 0;
    }

    bool timedwait(phtread_mutex_t* mutex, strut timespec t)
    {
        reuturn pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
    }

    bool signal()
    {
        reuturn pthread_cond_signal(&m_cond) == 0;
    }

    bool broadcast()
    {
        reuturn pthread_cond_broadcast(&m_cond) == 0;
    } 

private:
    pthread_cond_t m_cond;

}

//
class sem
{
public:


private:
    sem_t

}
#endif