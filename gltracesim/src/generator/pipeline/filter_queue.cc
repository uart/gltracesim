#include "generator/pipeline/filter_queue.hh"

namespace gltracesim {
namespace pipeline {

//
FilterQueue::FilterQueue() :
    gpu(&_data0), fin(&_data1)
{

}

FilterQueue::~FilterQueue()
{

}


void
FilterQueue::push()
{
    //
    consumer_has_work.set();
}

bool
FilterQueue::pop(int timeout)
{

    //
    if (consumer_has_work.wait(timeout) == false) {
        return false;
    }

    consumer_has_work.clear();

    //
    return true;
}

void
FilterQueue::signal_producer()
{
    //
    consumer_done.set();
}

void
FilterQueue::wait() {
    //
    consumer_done.wait();
    //
    consumer_done.clear();
}

void
FilterQueue::rotate_buffers()
{
    // temp
    buffer_t *p = gpu;

    // P <- A
    gpu = fin;
    //
    fin = p;

    // Start from beginning
    gpu->pos = 0;
}

} // end namespace pipeline
} // end namespace gltracesim

