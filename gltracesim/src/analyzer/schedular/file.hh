#ifndef __GLTRACESIM_ANALYZER_SCHEDULAR_FILE_HH__
#define __GLTRACESIM_ANALYZER_SCHEDULAR_FILE_HH__

#include "job.hh"
#include "scene.hh"
#include "frame.hh"

#include "analyzer/schedular/base.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

class FileSchedular : public Schedular
{

public:

    /**
     * @brief Schedular
     */
    FileSchedular(const Json::Value &params);

    /**
     * @brief ~Schedular
     */
    virtual ~FileSchedular();

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

private:

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        //
        ProtoInputStream *schedule;
        //
        ProtoInputStream *cluster;
    } pb;


protected:

    struct Cluster {
        //
        uint32_t id;
        //
        std::vector<uint32> jobs;
    };

    struct Schedule {
        //
        uint32_t id;
        //
        uint32_t frame_id;
        //
        uint32_t scene_id;
        //
        std::vector<Cluster> clusters;
    };

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

#endif // __GLTRACESIM_ANALYZER_SCHEDULAR_FILE_HH__
