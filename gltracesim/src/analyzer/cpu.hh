#ifndef __GLTRACESIM_ANALYZER_CPU_HH__
#define __GLTRACESIM_ANALYZER_CPU_HH__

#include <cstdint>
#include <json/json.h>

#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"

#include "analyzer/core.hh"
#include "analyzer/schedular/base.hh"

namespace gltracesim {

class GlTraceSimAnalyzer;

namespace analyzer {

class CPU {

public:

    /**
     * @brief
     */
    CPU(
        const Json::Value &params,
        GlTraceSimAnalyzer *simulator,
        schedular::SchedularPtr schedular
    );

    /**
     *
     */
    ~CPU();

    /**
     * @brief tick
     */
    void tick();

    /**
     * @brief get_core
     * @param core_id
     * @return
     */
    Core* get_core() {
        return core.get();
    }

private:

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        ProtoInputStream *cpu;
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
     * @brief core
     */
    CorePtr core;

};

} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_GPU_HH__
