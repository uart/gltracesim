#ifndef __GLTRACESIM_ANALYZER_SCHEDULAR_Z_HH__
#define __GLTRACESIM_ANALYZER_SCHEDULAR_Z_HH__

#include "job.hh"
#include "scene.hh"
#include "frame.hh"

#include "analyzer/schedular/base.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

class ZSchedular : public Schedular
{

public:

    /**
     * @brief Schedular
     */
    ZSchedular(const Json::Value &params);

    /**
     * @brief ~Schedular
     */
    virtual ~ZSchedular();

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
     * @brief schedule_tiles
     * @param x
     * @param y
     * @param size
     * @param order
     */
    void schedule_tiles(int x, int y, int size, int order);

protected:

    // A B
    // C D
    enum TileOrder {
        TILE_ACBD,
        TILE_CADB,
    };

    /**
     * @brief z_width
     */
    int z_width;

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
    std::deque<GpuJobPtr> gpu_draw_queue;

    /**
     * @brief job_queue
     */
    std::deque<GpuJobPtr> gpu_tile_queue;

    /**
     * @brief job_queue
     */
    std::deque<GpuJobPtr> gpu_misc_queue;

    /**
     * @brief coord
     */
    typedef std::pair<int, int> coord_t;

    /**
     * @brief job_map
     */
    std::map<coord_t, GpuJobPtr> pending_tile_jobs;

    /**
     * @brief cpu_queue
     */
    std::deque<GpuJobPtr> cpu_queue;

};


} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_SCHEDULAR_Z_HH__
