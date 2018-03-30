#ifndef __GLTRACESIM_ANALYZER_SCHEDULAR_FCFS_HH__
#define __GLTRACESIM_ANALYZER_SCHEDULAR_FCFS_HH__

#include "job.hh"
#include "scene.hh"
#include "frame.hh"

#include "analyzer/schedular/base.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

class FCFSSchedular : public Schedular
{

public:

    /**
     * @brief Schedular
     */
    FCFSSchedular(const Json::Value &params);

    /**
     * @brief ~Schedular
     */
    virtual ~FCFSSchedular();

public:

    /**
     * @brief get_next_cpu_job
     * @param core_id
     * @return
     */
    virtual GpuJobPtr get_next_cpu_job(int core_id);

    /**
     * @brief get_next_gpu_job
     * @param core_id
     * @return
     */
    virtual GpuJobPtr get_next_gpu_job(int core_id);

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id);

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id);

protected:

    /**
     * @brief scene_barrier_id
     */
    uint64_t next_scene_id;

    /**
     * @brief frame_barrier_id
     */
    uint64_t next_frame_id;

    /**
     * @brief current_scene
     */
    ScenePtr current_scene;

    /**
     * @brief current_frame
     */
    FramePtr current_frame;

    /**
     * @brief job_queue
     */
    std::deque<GpuJobPtr> gpu_queue;

    /**
     * @brief cpu_queue
     */
    std::deque<GpuJobPtr> cpu_queue;

};


} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_SCHEDULAR_FCFS_HH__
