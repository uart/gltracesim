#ifndef __GLTRACESIM_JOB_HH__
#define __GLTRACESIM_JOB_HH__

#include <map>
#include <chrono>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "job.pb.h"
#include "gltracesim.pb.h"
#include "device.hh"
#include "util/addr_range.hh"
#include "util/timer.hh"

#include "gem5/trace.hh"

#include "stats/distribution.hh"
#include "stats/distribution_impl.hh"

namespace gltracesim {

class GpuJob : public Timer
{

public:

    /**
     * @brief The inst_t union
     */
    union inst_t {
        struct {
            uint32_t opcode;
            uint32_t width;
        };
        uint64_t id;
    };

    /**
     * @brief The basic_blk_t struct
     */
    struct basic_blk_t {
        /**
         * @brief insts
         */
        std::vector<inst_t> insts;
    };

    /**
     * @brief The stats_t struct
     */
    struct stats_t
    {
        //
        stats_t();

        //
        uint64_t reads;
        uint64_t writes;

        //
        std::unordered_map<GpuJob::basic_blk_t*, size_t> basic_blk_count;

        //
        void reset();
    };

public:

    enum Type {
        MISC_JOB = 0,
        DRAW_JOB = 1,
        TILE_JOB = 2,
    };

protected:

    /**
     * @brief GpuJob
     * @param type
     * @param dev
     */
    GpuJob(
        uint64_t id,
        uint16_t frame_id,
        uint16_t scene_id,
        Type type,
        dev::HardwareDevice dev
    );

    /**
     *
     */
    ~GpuJob();

public:

    /**
     * @brief id
     */
    uint64_t id;

    /**
     * @brief scene_id
     */
    uint16_t scene_id;

    /**
     * @brief frame_id
     */
    uint16_t frame_id;

    /**
     * @brief id
     */
    Type type;

    /**
     * @brief id
     */
    uint8_t dev;

public:

    /**
     * @brief core_id
     */
    int core_id;

public:

    /**
     * @brief x
     */
    int x;

    /**
     * @brief y
     */
    int y;

public:

    /**
     * @brief frame_stats
     */
    stats_t stats;

public:

    /**
     * @brief configure_trace_generator
     */
    void configure_trace_generator();

    /**
     * @brief trace
     */
    gem5::TraceGenerator *trace;

public:

    /**
     * @brief load_trace
     */
    void load_trace();

    /**
     * @brief pkts
     */
    std::deque<packet_t> pkts;

public:

    /**
     * @brief dump_info
     * @param job_stats
     */
    virtual void dump_info(gltracesim::proto::JobInfo *job_info);

    /**
     * @brief dump_stats
     * @param job_stats
     */
    virtual void dump_stats(gltracesim::proto::JobStats *job_stats);

    /**
     * @brief reset_stats
     */
    void reset_stats();

};

class GpuMiscJob : public GpuJob {

public:

    /**
     * @brief GpuJob
     * @param dev
     */
    GpuMiscJob(uint64_t id, uint16_t scene_id, uint16_t frame_id, dev::HardwareDevice dev) :
        GpuJob(id, scene_id, frame_id, MISC_JOB, dev) {}
};

class GpuDrawJob : public GpuJob {

public:

    /**
     * @brief GpuJob
     * @param dev
     */
    GpuDrawJob(uint64_t id, uint16_t scene_id, uint16_t frame_id) :
        GpuJob(id, scene_id, frame_id, DRAW_JOB, dev::GPU) {}
};

class GpuTileJob : public GpuJob {

public:

    /**
     * @brief GpuJob
     * @param dev
     */
    GpuTileJob(uint64_t id, uint16_t scene_id, uint16_t frame_id, int x, int y)
        : GpuJob(id, scene_id, frame_id, TILE_JOB, dev::GPU)
    {
        this->x = x;
        this->y = y;
    }
};

//
typedef std::shared_ptr<GpuJob> GpuJobPtr;

} // end namespace gltracesim


#endif // __GLTRACESIM_JOB_HH__
