#ifndef __GLTRACESIM_ANALYSIS_QUEUE_HH__
#define __GLTRACESIM_ANALYSIS_QUEUE_HH__

#include <queue>
#include <vector>
#include <memory>

#include "pin.H"
#include "util/threads.hh"
#include "generator/buffer.hh"

namespace gltracesim {
namespace pipeline {

/**
 * @brief The AnalysisQueue class
 *
 * Multiple procuders, single consumer queue.
 */
class AnalysisQueue {

public:

    /**
     * @brief AnalysisQueue
     */
    AnalysisQueue();

    /**
     * @brief AnalysisQueue
     */
    ~AnalysisQueue();

    /**
     * @brief push
     * @param tid
     */
    void add_work_thread(int tid);

    /**
     * @brief add_work_item
     * @param aid
     */
    void add_work_item(int aid);

    /**
     * @brief push
     * @param tid
     */
    void push(const packet_t &pkt);

    /**
     * @brief pop
     * @param timeout
     * @return
     */
    int pop(int timeout, int tid);

    /**
     * @brief signal_producer
     * @param tid
     */
    void signal_producer(int tid, int aid);

    /**
     * @brief wait
     * @param tid
     */
    void start();

    /**
     * @brief wait
     * @param tid
     */
    void rotate_buffers();

    /**
     * @brief wait
     * @param tid
     */
    void wait();

    /**
     * @brief front
     */
    buffer_t *fout_buffer;
    buffer_t *analysis_buffer;

private:

    /**
     * @brief mtx
     */
    Mutex producer_mtx;

    /**
     * @brief mtx
     */
    Mutex consumer_mtx;

    /**
     * @brief The work_thread_t struct
     */
    struct work_thread_t {
        /**
         * @brief id
         */
        int tid;

        /**
         * @brief has_work
         */
        Semaphore has_work;

        /**
         * @brief done
         */
        Semaphore done;
    };

    /**
     * @brief signal
     */
    std::vector<work_thread_t> work_threads;

    struct work_item_t {
        /**
         * @brief id
         */
        int aid;
    };

    /**
     * @brief analyzers
     */
    std::vector<int> work_items;

    /**
     * @brief job_queue
     */
    std::deque<int> job_queue;

    /**
     * @brief queue
     */
    buffer_t _data0;
    buffer_t _data1;

private:

    /**
     * @brief operator =
     * @param other
     */
    void operator=(const AnalysisQueue &other) {}

};

/**
 * @brief AnalysisQueuePtr
 */
typedef std::shared_ptr<AnalysisQueue> AnalysisQueuePtr;

} // end namespace pipeline
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYSIS_QUEUE_HH__
