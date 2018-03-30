#ifndef __GLTRACESIM_ANALYZER_SCHEDULAR_BASE_HH__
#define __GLTRACESIM_ANALYZER_SCHEDULAR_BASE_HH__

#include <json/json.h>

#include "util/cflags.hh"
#include "job.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {


class Schedular
{

public:

    /**
     * @brief Schedular
     */
    Schedular(const Json::Value &params);

    /**
     * @brief ~Schedular
     */
    virtual ~Schedular();

protected:

    /**
     * @brief save_schedule_descision
     * @param core_id
     * @param dev
     * @return
     */
    void dump_schedule_descision(GpuJobPtr &job);

    /**
     * @brief get_next_cpu_job
     * @param core_id
     * @return
     */
    virtual GpuJobPtr get_next_cpu_job(int core_id) = 0;

    /**
     * @brief get_next_gpu_job
     * @param core_id
     * @return
     */
    virtual GpuJobPtr get_next_gpu_job(int core_id) = 0;

public:

    /**
     * @brief get_next_gpu_job
     * @param core_id
     * @return
     */
    virtual GpuJobPtr get_next_job(int core_id, int dev) {
        //
        GpuJobPtr job;

        //
        if (dev == dev::CPU) {
            job = get_next_cpu_job(core_id);
        } else {
            job = get_next_gpu_job(core_id);
        }

        //
        if (job) {
            //
            job->core_id = core_id;
            //
            dump_schedule_descision(job);
        }

        //
        return job;
    }

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id) = 0;

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id) = 0;

public:

    /**
     * @brief is_gpu_ready
     * @param id
     * @return
     */
    bool is_gpu_ready(uint64_t id) {
        //
        if (_u(system_barrier_id[dev::GPU] < id)) {
            return false;
        }
        //
        return true;
    }

    /**
     * @brief set_gpu_ready
     * @param id
     */
    void set_gpu_ready(uint64_t id) {
        system_barrier_id[dev::GPU] = id;
    }

    /**
     * @brief is_cpu_ready
     * @param id
     * @return
     */
    bool is_cpu_ready(uint64_t id) {
        //
        if (_u(system_barrier_id[dev::CPU] < id)) {
            return false;
        }
        //
        return true;
    }

    /**
     * @brief set_cpu_ready
     * @param id
     */
    void set_cpu_ready(uint64_t id) {
        system_barrier_id[dev::CPU] = id;
    }

    /**
     * @brief provide
     * @param id
     * @return
     */
    bool provide(uint64_t id);

    /**
     * @brief require
     * @param id
     * @return
     */
    bool require(uint64_t id);

protected:

    /**
     * @brief The pb_t struct
     */
    struct pb_t {
        //
        ProtoOutputStream *output_schedule;
    } pb;

protected:

    /**
     * @brief no_cores
     */
    size_t no_cores;

protected:

    /**
     * @brief barrier_id
     */
    std::array<uint64_t, 2> system_barrier_id;

    /**
     * @brief scoreboard
     */
    std::set<size_t> system_scoreboard;

};

/**
 * @brief SchedularPtr
 */
typedef std::shared_ptr<Schedular> SchedularPtr;

} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_SCHEDULAR_BASE_HH__
