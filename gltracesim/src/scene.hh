#ifndef __GLTRACESIM_SCENE_HH__
#define __GLTRACESIM_SCENE_HH__

#include <chrono>
#include <memory>
#include <cstdint>

#include "job.pb.h"
#include "scene.pb.h"
#include "device.hh"

#include "util/timer.hh"

namespace gltracesim {

class Scene : public Timer
{

public:

    /**
     * @brief GpuJob
     * @param type
     * @param dev
     * @param x
     * @param y
     */
    Scene(uint16_t id, uint16_t frame_id);

    /**
     *
     */
    ~Scene();

public:

    /**
     * @brief set_global_id
     * @param id
     */
    void set_global_id(uint32_t id) {
        global_id = id;
    }

    /**
     * @brief inc_width
     * @param new_width
     */
    void inc_width(uint16_t new_width) {
        width = std::max(width, new_width);
    }

    /**
     * @brief inc_height
     * @param new_height
     */
    void inc_height(uint16_t new_height) {
        height = std::max(height, new_height);
    }

    /**
     * @brief add_job
     * @param job_id
     */
    void add_job(size_t job_id) {
        jobs.push_back(job_id);
    }

    /**
     * @brief add_opengl_call
     * @param call
     */
    void add_opengl_call(uint32_t call_id) {
        opengl_calls.push_back(call_id);
    }

public:

    /**
     * @brief id within frame
     */
    uint16_t id;

    /**
     * @brief frame_id
     */
    uint16_t frame_id;

    /**
     * @brief id from start
     */
    uint32_t global_id;

    /**
     * @brief width
     */
    uint16_t width;

    /**
     * @brief height
     */
    uint16_t height;

    /**
     * @brief jobs
     */
    std::vector<uint64_t> jobs;

    /**
     * @brief opengl_calls
     */
    std::vector<uint32_t> opengl_calls;

public:

    /**
     * @brief dump_info
     * @param info
     */
    virtual void dump_info(gltracesim::proto::SceneInfo *info);

};

//
typedef std::shared_ptr<Scene> ScenePtr;

} // end namespace gltracesim


#endif // __GLTRACESIM_SCENE_HH__
