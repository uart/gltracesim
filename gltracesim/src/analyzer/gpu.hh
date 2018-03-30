#ifndef __GLTRACESIM_ANALYZER_GPU_HH__
#define __GLTRACESIM_ANALYZER_GPU_HH__

#include <cstdint>
#include <json/json.h>

#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"

#include "job.hh"

#include "analyzer/core.hh"
#include "analyzer/schedular/base.hh"

namespace gltracesim {

class GlTraceSimAnalyzer;

namespace analyzer {

class GPU {

public:

    /**
     * @brief GPU
     */
    GPU(
        const Json::Value &params,
        GlTraceSimAnalyzer *simulator,
        schedular::SchedularPtr schedular
    );

    /**
     *
     */
    ~GPU();

    /**
     * @brief tick
     */
    void tick();

    /**
     * @brief get_core
     * @param core_id
     * @return
     */
    Core* get_core(int core_id) {
        return cores[core_id].get();
    }

    /**
     * @brief num_cores
     * @return
     */
    size_t num_cores() const {
        return cores.size();
    }

private:

    /**
     * @brief sync
     * @return
     */
    bool sync();

    /**
     * @brief tick
     */
    void tick_cores();

private:

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        ProtoInputStream *gpu;
    } pb;

    /**
     * @brief simulator
     */
    GlTraceSimAnalyzer *simulator;

    /**
     * @brief schedular
     */
    schedular::SchedularPtr schedular;

    /**
     * @brief barrier_id
     */
    uint64_t barrier_id;

    /**
     * @brief state
     */
    Core::RuntimeState core_state;

    enum RuntimeState {
        PROCESS_CMD,
        D_SYNC,
        W_SYNC,
        D_RSC_SYNC,
        W_RSC_SYNC,
        D_NEW_SCENE_SYNC,
        D_END_SCENE_SYNC,
        W_NEW_SCENE_SYNC,
        W_END_SCENE_SYNC,
        D_FRAME_SYNC,
        W_FRAME_SYNC
    };

    RuntimeState state;

    /**
     * @brief gpu
     */
    std::vector<CorePtr> cores;

};

} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_GPU_HH__
