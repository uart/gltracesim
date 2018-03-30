#include "util/threads.hh"

namespace gltracesim {

Mutex::Mutex()
{
#ifdef __USING_PIN__
    assert(PIN_MutexInit(&mtx));
#else

#endif

}

Mutex::~Mutex()
{
#ifdef __USING_PIN__
    PIN_MutexFini(&mtx);
#else

#endif
}


RWRMutex::RWRMutex()
{
#ifdef __USING_PIN__
    assert(PIN_RWMutexInit(&mtx));
#else

#endif

}

RWRMutex::~RWRMutex()
{
#ifdef __USING_PIN__
    PIN_RWMutexFini(&mtx);
#else

#endif
}

Semaphore::Semaphore()
{
#ifdef __USING_PIN__
    assert(PIN_SemaphoreInit(&sem));
#else

#endif
}

Semaphore::~Semaphore()
{
#ifdef __USING_PIN__
    PIN_SemaphoreFini(&sem);
#else

#endif
}

} // end namespace gltracesim

