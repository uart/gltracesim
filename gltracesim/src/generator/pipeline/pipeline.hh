#ifndef __GLTRACESIM_PIPELINE_PIPELINE_HH__
#define __GLTRACESIM_PIPELINE_PIPELINE_HH__

#include <array>
#include <vector>

#include "generator/pipeline/filter_queue.hh"
#include "generator/pipeline/analysis_queue.hh"

namespace gltracesim {
namespace pipeline {

// Max number of threads (for std::array)
#define MAX_THREADS 128

//
class Pipeline
{

public:

    /**
     * @brief Pipeline
     */
    Pipeline();

    // Size: Number of gpu threads
    std::array<FilterQueuePtr, MAX_THREADS> filter_queue;

    // Size: Number of analysis threads (fixed)
    AnalysisQueuePtr analysis_queue;

    //
    int num_gpu_threads() { return _num_gpu_threads; }
    int num_filter_threads() { return _num_filter_threads; }
    int num_anaysis_threads() { return _num_analysis_threads; }

    //
    int get_gid(int external_tid) { return eid2gid_map[external_tid]; }
    int get_fid(int external_tid) { return eid2fid_map[external_tid]; }
    int get_aid(int external_tid) { return eid2aid_map[external_tid]; }

    //
    int get_gtid(int internal_tid) { return gid2eid_map[internal_tid]; }
    int get_ftid(int internal_tid) { return fid2eid_map[internal_tid]; }
    int get_atid(int internal_tid) { return aid2eid_map[internal_tid]; }

    //
    void add_analyzer(int aid);

    //
    int add_gpu_thread();
    int add_filter_thread();
    int add_analysis_thread();

    void map_gpu_thread(int gid, int external_tid);
    void map_filter_thread(int fid, int external_tid);
    void map_analysis_thread(int aid, int external_tid);

private:

    //
    int _num_gpu_threads;
    int _num_filter_threads;
    int _num_analysis_threads;

    // Size: Number of threads
    std::array<int, MAX_THREADS> eid2gid_map;
    std::array<int, MAX_THREADS> eid2fid_map;
    std::array<int, MAX_THREADS> eid2aid_map;

    // Size: Number of gpu threads
    std::array<int, MAX_THREADS> gid2eid_map;
    // Size: Number of gpu threads
    std::array<int, MAX_THREADS> fid2eid_map;
    // Size: Number of analysis threads (fixed)
    std::array<int, MAX_THREADS> aid2eid_map;

};

} // end namespace pipeline
} // end namespace gltracesim

#endif // __GLTRACESIM_PIPELINE_PIPELINE_HH__
