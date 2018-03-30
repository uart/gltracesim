#include "generator/pipeline/pipeline.hh"

namespace gltracesim {
namespace pipeline {

Pipeline::Pipeline() :
    _num_gpu_threads(0), _num_filter_threads(0), _num_analysis_threads(0)
{
    //
    analysis_queue = AnalysisQueuePtr(new AnalysisQueue());
}

int
Pipeline::add_gpu_thread()
{
    // Allocate internal id
    int internal_tid = _num_gpu_threads++;

    //
    filter_queue[internal_tid] = FilterQueuePtr(new FilterQueue());

    // GPU thread id
    return internal_tid;
}

int
Pipeline::add_filter_thread()
{
    // Allocate internal id
    int internal_tid = _num_filter_threads++;

    // GPU thread id
    return internal_tid;
}


void
Pipeline::add_analyzer(int aid)
{
    //
    analysis_queue->add_work_item(aid);
}

int
Pipeline::add_analysis_thread()
{
    // Allocate internal id
    int internal_tid = _num_analysis_threads++;

    //
    analysis_queue->add_work_thread(internal_tid);

    // GPU thread id
    return internal_tid;
}

void
Pipeline::map_gpu_thread(int internal_tid, int external_tid)
{
    //
    eid2gid_map[external_tid] = internal_tid;
    gid2eid_map[internal_tid] = external_tid;
}

void
Pipeline::map_filter_thread(int internal_tid, int external_tid)
{
    //
    eid2fid_map[external_tid] = internal_tid;
    fid2eid_map[internal_tid] = external_tid;
}

void
Pipeline::map_analysis_thread(int internal_tid, int external_tid)
{
    //
    eid2aid_map[external_tid] = internal_tid;
    aid2eid_map[internal_tid] = external_tid;
}


} // end namespace pipeline
} // end namespace gltracesim

