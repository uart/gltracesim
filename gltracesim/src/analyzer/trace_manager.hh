#ifndef __GLTRACESIM_ANALYZER_TRACE_MANAGER_HH__
#define __GLTRACESIM_ANALYZER_TRACE_MANAGER_HH__

#include <thread>
#include <vector>
#include <json/json.h>

#include "frame.hh"
#include "scene.hh"
#include "job.hh"

namespace gltracesim {

class TraceManager {

public:

    /**
     * @brief TraceManager
     * @param params
     */
    TraceManager(const Json::Value &params);

    /**
     *
     */
    ~TraceManager();

private:

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        //
        ProtoInputStream *frames;
        //
        ProtoInputStream *scenes;
        //
        ProtoInputStream *jobs;
        //
        ProtoInputStream *resources;
    } pb;

public:

    /**
     * @brief get_frame
     * @param id
     * @return
     */
    FramePtr get_frame(size_t id);

    /**
     * @brief get_scene
     * @param id
     * @return
     */
    ScenePtr get_scene(size_t id);

    /**
     * @brief get_job
     * @param id
     * @return
     */
    GpuJobPtr get_job(size_t frame_id, size_t id);

    /**
     * @brief get_resource
     * @param id
     * @return
     */
    GpuResourcePtr get_resource(size_t id);

private:

    /**
     * @brief prefetch_fs_metadata
     */
    void prefetch_fs_metadata();

    /**
     * @brief fs_prefetcher
     */
    std::thread *fs_prefetcher;

private:


    /**
     * @brief last_frame
     */
    FramePtr last_frame;

    /**
     * @brief last_frame
     */
    ScenePtr last_scene;

    /**
     * @brief frame_id
     */
    size_t frame_id;

    /**
     * @brief scene_id
     */
    size_t scene_id;

    /**
     * @brief job_scene_id
     */
    size_t job_frame_id;

    /**
     * @brief jobs within last scene
     */
    std::unordered_map<size_t, GpuJobPtr> jobs;

    /**
     * @brief resource_id
     */
    size_t resource_id;

};

/**
 * @brief SystemPtr
 */
typedef std::unique_ptr<TraceManager> TraceManagerPtr;

/**
 * @brief system
 */
extern TraceManagerPtr trace_manager;

} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_TRACE_MANAGER_HH__
