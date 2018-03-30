#ifndef __GLTRACESIM_ANALYZER_CORE_HH__
#define __GLTRACESIM_ANALYZER_CORE_HH__

#include <cstdint>
#include <json/json.h>

#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"

#include "job.hh"

#include "analyzer/schedular/base.hh"

namespace gltracesim {

class GlTraceSimAnalyzer;

namespace analyzer {

class Core {

public:

    /**
     * @brief The state_t enum
     */
    enum RuntimeState {
        IDLE,
        RUNNING
    };

public:

    /**
     * @brief core_t
     */
    Core(
        const Json::Value &params,
        GlTraceSimAnalyzer *simulator,
        schedular::SchedularPtr schedular
    );


    /**
     * @brief tick
     */
    void tick();

    /**
     * @brief has_work
     * @return
     */
    RuntimeState get_state() {
        return state;
    }

private:

    /**
     * @brief id
     */
    int id;

    /**
     * @brief dev
     */
    int dev;

    /**
     * @brief simulator
     */
    GlTraceSimAnalyzer *simulator;

    /**
     * @brief schedular
     */
    schedular::SchedularPtr schedular;

    /**
     * @brief job_id
     */
    GpuJobPtr job;

    /**
     * @brief has_work
     */
    RuntimeState state;

public:

    /**
     * @brief The stats_t struct
     */
    struct stats_t {

        /**
         * @brief stats_t
         */
        stats_t() :
            no_jobs(0),
            no_pkts(0)
        {
            // Do nothing
        }

        /**
         * @brief no_jobs
         */
        size_t no_jobs;

        /**
         * @brief no_pkts
         */
        size_t no_pkts;

    } stats;
};

/**
 * @brief CorePtr
 */
typedef std::unique_ptr<Core> CorePtr;

} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_CORE_HH__
