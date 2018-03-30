#include "generator/pipeline/analysis_queue.hh"

namespace gltracesim {
namespace pipeline {

AnalysisQueue::AnalysisQueue() :
    fout_buffer(&_data0), analysis_buffer(&_data1)
{

}

AnalysisQueue::~AnalysisQueue()
{

}

//
void
AnalysisQueue::add_work_thread(int tid)
{
    //
    work_threads.push_back(work_thread_t());

    // tid
    work_threads.back().tid = tid;
}

//
void
AnalysisQueue::add_work_item(int aid)
{
    //
    work_items.push_back(aid);
}

//
void
AnalysisQueue::push(const packet_t &pkt)
{
    //
    producer_mtx.lock();

    //
    fout_buffer->data[fout_buffer->pos] = pkt;

    //
    ++fout_buffer->pos;

    // Buffer is full, need to process
    if (fout_buffer->pos >= fout_buffer->data.size()) {      

        // Wait
        for (size_t i = 0; i < work_threads.size(); ++i) {
            //
            work_threads[i].done.wait();
            //
            work_threads[i].done.clear();
        }

        consumer_mtx.lock();

        //
        rotate_buffers();

        //
        job_queue.insert(
            job_queue.begin(), work_items.begin(), work_items.end()
        );

        // Start consumers
        for (size_t i = 0; i < work_threads.size(); ++i) {
            work_threads[i].has_work.set();
        }

        consumer_mtx.unlock();
    }

    //
    producer_mtx.unlock();
}

void
AnalysisQueue::start()
{
    //
    producer_mtx.lock();

    // Push all analazers on work queue
    job_queue.insert(
        job_queue.begin(), work_items.begin(), work_items.end()
    );

    //
    for (size_t i = 0; i < work_threads.size(); ++i) {
        work_threads[i].has_work.set();
    }

    //
    producer_mtx.unlock();
}

void
AnalysisQueue::rotate_buffers()
{
    // temp
    buffer_t *p = fout_buffer;
    // P <- A
    fout_buffer = analysis_buffer;
    //
    analysis_buffer = p;
    //
    fout_buffer->pos = 0;
}

int
AnalysisQueue::pop(int timeout, int tid)
{

    //
    if (work_threads[tid].has_work.wait(timeout) == false) {
        return -1;
    }

    //
    consumer_mtx.lock();

    // No more jobs, clear state
    if (job_queue.empty()) {

        // No more work
        work_threads[tid].has_work.clear();

        // Nothing alse todo
        work_threads[tid].done.set();

        //
        consumer_mtx.unlock();

        //
        return -1;
    }

    //
    int aid = job_queue.front();

    //
    job_queue.pop_front();

    //
    consumer_mtx.unlock();

    //
    return aid;
}

void
AnalysisQueue::signal_producer(int tid, int aid)
{
    //
    consumer_mtx.lock();

    // No more jobs, clear state
    if (job_queue.empty()) {
        // No more work
        work_threads[tid].has_work.clear();
        //
        work_threads[tid].done.set();
    }

    //
    consumer_mtx.unlock();
}

void
AnalysisQueue::wait()
{
    //
    producer_mtx.lock();

    for (size_t i = 0; i < work_threads.size(); ++i) {
        //
        work_threads[i].done.wait();
        //
        work_threads[i].done.clear();
    }

    //
    producer_mtx.unlock();
}

} // end namespace pipeline
} // end namespace gltracesim

