#ifndef LOCKER_H
#define LOCKER_H

#include <pthread.h>
#include <exception>
#include <semaphore.h>

// thread synchronization mechanism 
class locker {
public:
    locker() 
    {
        /* if successful, pthread_mutex_init() function shall return 0, 
        otherwise, an error number shall be returned to indicate the error.*/
        if(pthread_mutex_init(&m_mutex, nullptr) != 0) {
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


// conditional variable are used for more advanced synchronization scenarios, 
// where one or more threads need to wait for a conditon to become true before proceed.
class cond
{
public:
    cond() {
        // initialize the condition variable referenced by m_cond
        if(pthread_cond_init(&m_cond, NULL) != 0) {
            throw std::exception();
        }
    }

    ~cond() {
        pthread_cond_destroy(&m_cond);
    }

    // This function is used by a thread to wait for a condition variable to be signaled by another thread.
    // It release the associated mutex and puts the calling thread to sleep until the condition is signaled.
    // When the condition is signaled, the thread wakes up and reacquires the mutex. 
    bool wait(pthread_mutex_t* mutex) {
        return pthread_cond_wait(&m_cond, mutex) == 0;
    }

    // similar to wait. 
    // If the condtion is not signaled within the specific time, the fucntion returns with an error.
    bool timedwait(pthread_mutex_t* mutex, struct timespec t) {
        return pthread_cond_timedwait(&m_cond, mutex, &t) == 0;
    }

    // This function is used to signal one waiting thread that the condition has occurred. 
    // It wakes up one of the threads waiting on the condition variable
    bool signal() {
        return pthread_cond_signal(&m_cond) == 0;
    }


    // This function is used to signal all waiting threads that the condition has occurred. 
    // It wakes up all waiting thread.

    bool broadcast() {
        return pthread_cond_broadcast(&m_cond) == 0;
    } 

private:
    pthread_cond_t m_cond;
};

// semaphore class 
class sem {
public:
    // Constructor for initializing the semaphore with an initial value of 0
    sem() {
        if( sem_init( &m_sem, 0, 0 ) != 0 ) {
            throw std::exception();
        }
    }
    // Constructor for initializing the semaphore with a specified initial value
    sem(int num) {
        if( sem_init( &m_sem, 0, num ) != 0 ) {
            throw std::exception();
        }
    }
    // Destructor for destroying the semaphore
    ~sem() {
        sem_destroy( &m_sem );
    }
    // Wait function to block until the semaphore value is greater than zero
    bool wait() {
        return sem_wait( &m_sem ) == 0;
    }
    // Post function to increment the semaphore value, potentially unblocking waiting threads
    bool post() {
        return sem_post( &m_sem ) == 0;
    }
private:
    sem_t m_sem;  // the underlying POSIX semaphore data structure
};
    

#endif