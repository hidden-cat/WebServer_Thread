#ifndef _LOCK_H_
#define _LOCK_H_

#include <semaphore.h>
#include <exception>
#include <iostream>
#include <mutex>

class sem{
    public:
        sem() {
            if(sem_init(&m_sem, 0, 0) != 0) {
                throw std::exception();
            }
        }

        sem(unsigned int num) {
            if(sem_init(&m_sem, 0, num) != 0) {
                throw std::exception();
            }
        }

        ~sem() {
            sem_destroy(&m_sem);
        }

        bool wait(void) {
            return (sem_wait(&m_sem) == 0);
        }

        bool post(void) {
            return (sem_post(&m_sem) == 0);
        }
    
    private:
        sem_t m_sem;
};

class locker {
    public:
        locker(){

        }
        ~locker(){

        }

        void lock() {
            mtx.lock();
        }

        void unlock() {
            mtx.unlock();
        }
    private:
        std::mutex mtx;
};

#endif