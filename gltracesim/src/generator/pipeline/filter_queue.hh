#ifndef __GLTRACESIM_FILTER_QUEUE_HH__
#define __GLTRACESIM_FILTER_QUEUE_HH__

#include <memory>

#include "pin.H"
#include "util/threads.hh"
#include "generator/buffer.hh"

namespace gltracesim {
namespace pipeline {

/**
 * @brief The FilterQueue class
 *
 * Single producer, single consumer queue.
 *
 */
class FilterQueue {

public:

    /**
     * @brief FilterQueue
     */
    FilterQueue();

    /**
     * @brief FilterQueue
     */
    ~FilterQueue();

    /**
     * @brief push
     */
    void push();

    /**
     * @brief pop
     * @param timeout
     * @return
     */
    bool pop(int timeout);

    /**
     * @brief signal_producer
     */
    void signal_producer();

    /**
     * @brief wait
     */
    void wait();

    /**
     * @brief rotate_buffers
     *
     * G -> FI -> G
     *
     */
    void rotate_buffers();

    // Pointers to pipeline stage buffer
    buffer_t *gpu;
    buffer_t *fin;

private:

    //
    Semaphore consumer_has_work;

    //
    Semaphore consumer_done;

private:

    // Storage
    buffer_t _data0;
    buffer_t _data1;

private:

    /**
     * @brief operator =
     * @param other
     */
    void operator=(const FilterQueue &other) {}

};

/**
 * @brief AnalysisQueuePtr
 */
typedef std::shared_ptr<FilterQueue> FilterQueuePtr;

} // end namespace pipeline
} // end namespace gltracesim

#endif // __GLTRACESIM_FILTER_QUEUE_HH__
