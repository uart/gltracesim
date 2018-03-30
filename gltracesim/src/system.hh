#ifndef __GLTRACESIM_SYSTEM_HH__
#define __GLTRACESIM_SYSTEM_HH__

#include <memory>

#include "resource_tracker.hh"
#include "vmemory.hh"

namespace gltracesim {

/**
 * @brief The VirtualMemoryManager class
 *
 * Reverse memory allocator, given a virtual address, allocate a fake physical
 * address.
 *
 */
class System
{

public:

    /**
     * @brief VirtualMemoryManager
     * @param name
     */
    System(const Json::Value &config);

    /**
     * @brief ~VirtualMemoryManager
     */
    ~System();

public:

    /**
     * @brief is_running
     * @return
     */
    bool is_running() const {
        return running;
    }

    /**
     * @brief stop
     * @return
     */
    void stop() {
        running = false;
    }

    /**
     * @brief get_config
     * @return
     */
    const Json::Value& get_config() {
        return config;
    }

    /**
     * @brief get_input_dir
     * @return
     */
    const std::string& get_input_dir() const {
        return input_dir;
    }

    /**
     * @brief set_input_dir
     * @param new_input_dir
     */
    void set_input_dir(const std::string &new_input_dir) {
        input_dir = new_input_dir;
    }

    /**
     * @brief get_output_dir
     * @return
     */
    const std::string& get_output_dir() const {
        return output_dir;
    }

    /**
     * @brief set_output_dir
     * @param new_output_dir
     */
    void set_output_dir(const std::string &new_output_dir) {
        output_dir = new_output_dir;
    }

    /**
     * @brief get_frame_nbr
     * @return
     */
    uint64_t get_frame_nbr() const {
        return frame_nbr;
    }

    /**
     * @brief inc_frame_nbr
     */
    void inc_frame_nbr() {
        ++frame_nbr;
    }

    /**
     * @brief set_frame_nbr
     * @param new_frame_nbr
     */
    void set_frame_nbr(uint64_t new_frame_nbr) {
        frame_nbr = new_frame_nbr;
    }

    /**
     * @brief get_scene_nbr
     * @return
     */
    uint64_t get_scene_nbr() const {
        return scene_nbr;
    }

    /**
     * @brief get_scene_nbr
     * @return
     */
    uint64_t get_global_scene_idx() const {
        return global_scene_idx;
    }

    /**
     * @brief inc_scene_nbr
     */
    void inc_scene_nbr() {
        ++scene_nbr;
        ++global_scene_idx;
    }

    /**
     * @brief set_scene_nbr
     * @param new_scene_nbr
     */
    void set_scene_nbr(uint64_t new_scene_nbr) {
        scene_nbr = new_scene_nbr;
    }

    /**
     * @brief get_job_nbr
     * @return
     */
    uint64_t get_job_nbr() const {
        return job_nbr;
    }

    /**
     * @brief inc_job_nbr
     */
    void inc_job_nbr() {
        ++job_nbr;
    }

    /**
     * @brief set_job_nbr
     * @param new_job_nbr
     */
    void set_job_nbr(uint64_t new_job_nbr) {
        job_nbr = new_job_nbr;
    }

    /**
     * @brief get_blk_size
     * @return
     */
    uint64_t get_blk_size() const {
        return blk_size;
    }

    /**
     * @brief set_blk_size
     * @param new_blk_size
     */
    void set_blk_size(uint64_t new_blk_size) {
        blk_size = new_blk_size;
    }

    /**
     * @brief get_tsc
     * @return
     */
    uint64_t get_tsc() const {
        return tsc;
    }

    /**
     * @brief inc_tsc
     */
    void inc_tsc() {
        ++tsc;
    }

    /**
     * @brief set_tsc
     * @param new_tsc
     */
    void set_tsc(uint64_t new_tsc) {
        tsc = new_tsc;
    }

public:

    //
    ResourceTrackerPtr rt;
    //
    VirtualMemoryManagerPtr vmem_manager;

private:

    /**
     * @brief running
     */
    bool running;

    /**
     * @brief config
     */
    Json::Value config;

    /**
     * @brief input_dir
     */
    std::string input_dir;

    /**
     * @brief output_dir
     */
    std::string output_dir;

    /**
     * @brief frame_nbr
     */
    uint64_t frame_nbr;

    /**
     * @brief scene_nbr
     */
    uint64_t scene_nbr;

    /**
     * @brief global_scene_idx
     */
    uint64_t global_scene_idx;

    /**
     * @brief job_nbr
     */
    uint64_t job_nbr;

    /**
     * @brief blk_size
     */
    uint64_t blk_size;

    /**
     * @brief tsc
     */
    uint64_t tsc;

};

/**
 * @brief SystemPtr
 */
typedef std::shared_ptr<System> SystemPtr;

/**
 * @brief system
 */
extern SystemPtr system;


} // end namespace gltracesim

#endif // __GLTRACESIM_SYSTEM_HH__
