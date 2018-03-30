#ifndef __GLTRACESIM_THREADS_HH__
#define __GLTRACESIM_THREADS_HH__

#include <cassert>

#ifdef __USING_PIN__
#include "pin.H"
#else
#include <mutex>
#include <condition_variable>
#endif

namespace gltracesim {

class Mutex {

public:

    /**
     * @brief Mutex
     */
    Mutex();

    /**
     * @brief ~Mutex
     */
    virtual ~Mutex();

    /**
     * @brief lock
     */
    void lock() {
#ifdef __USING_PIN__
    PIN_MutexLock(&mtx);
#else
    mtx.lock();
#endif
    }

    /**
     * @brief unlock
     */
    void unlock() {
#ifdef __USING_PIN__
    PIN_MutexUnlock(&mtx);
#else
    mtx.unlock();
#endif
    }

private:

#ifdef __USING_PIN__
    PIN_MUTEX mtx;
#else
    std::mutex mtx;
#endif

};

class RWRMutex {

public:

    /**
     * @brief Mutex
     */
    RWRMutex();

    /**
     * @brief ~Mutex
     */
    virtual ~RWRMutex();

    /**
     * @brief lock
     */
    void lock() {
#ifdef __USING_PIN__
    PIN_RWMutexWriteLock(&mtx);
#else
    mtx.lock();
#endif
    }

    /**
     * @brief rlock
     */
    void rlock() {
#ifdef __USING_PIN__
    PIN_RWMutexReadLock(&mtx);
#else
    mtx.lock();
#endif
    }

    /**
     * @brief unlock
     */
    void unlock() {
#ifdef __USING_PIN__
    PIN_RWMutexUnlock(&mtx);
#else
    mtx.unlock();
#endif
    }

private:

#ifdef __USING_PIN__
    PIN_RWMUTEX mtx;
#else
    std::mutex mtx;
#endif

};

class Semaphore {

public:

    /**
     * @brief Mutex
     */
    Semaphore();

    /**
     * @brief ~Mutex
     */
    virtual ~Semaphore();

    /**
     * @brief lock
     */
    void set() {
#ifdef __USING_PIN__
    PIN_SemaphoreSet(&sem);
#else
    assert(0);
#endif
    }

    /**
     * @brief unlock
     */
    void clear() {
#ifdef __USING_PIN__
    PIN_SemaphoreClear(&sem);
#else
    assert(0);
#endif
    }

    /**
     * @brief unlock
     */
    void wait() {
#ifdef __USING_PIN__
    PIN_SemaphoreWait(&sem);
#else
    assert(0);
#endif
    }

    /**
     * @brief unlock
     */
    bool wait(int timeout) {
#ifdef __USING_PIN__
    return PIN_SemaphoreTimedWait(&sem, timeout);
#else
    assert(0);
#endif
    }

private:

#ifdef __USING_PIN__
    PIN_SEMAPHORE sem;
#else

#endif

};



} // end namespace gltracesim

#endif // __GLTRACESIM_THREADS_HH__
