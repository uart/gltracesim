#include "analyzer/cpu.hh"
#include "analyzer/gpu.hh"

#include "debug_impl.hh"

#include "gltracesim_analyzer.hh"

namespace gltracesim {
namespace analyzer {

GPU::GPU(const Json::Value &params,
    GlTraceSimAnalyzer *simulator, schedular::SchedularPtr schedular) :
    simulator(simulator), schedular(schedular), barrier_id(0),
    core_state(Core::RuntimeState::IDLE), state(PROCESS_CMD)
{
    //
    ProtoMessage::PacketHeader hdr;

    pb.gpu = new ProtoInputStream(
        params["input-dir"].asString() + "/gpu.pb.gz"
    );

    //
    pb.gpu->read(hdr);

    DPRINTF(Init, "GPU CMD File Header [id:%s, ver:%i, tick_freq:%lu].\n",
        hdr.obj_id().c_str(), hdr.ver(), hdr.tick_freq()
    );

    //
    for (int core_id = 0;
         core_id < params["num-gpu-cores"].asInt();
         ++core_id)
    {
        Json::Value p;
        p["id"] = core_id + 1; // 0 == CPU
        p["dev"] = dev::GPU;

        //
        cores.push_back(CorePtr(new Core(p, simulator, schedular)));
    }
}

GPU::~GPU()
{
    delete pb.gpu;
}


void
GPU::tick_cores()
{
    //
    bool is_running = false;

    //
    for (size_t core_id = 0; core_id < cores.size(); ++core_id) {
        //
        cores[core_id]->tick();
        //
        is_running |=
            (cores[core_id]->get_state() == Core::RuntimeState::RUNNING);
    }

    //
    if (_l(is_running)) {
        core_state = Core::RuntimeState::RUNNING;
    } else {
        core_state = Core::RuntimeState::IDLE;
    }
}

void
GPU::tick()
{

    // Current state
    switch (state)
    {
    case D_SYNC:
    case D_RSC_SYNC:
    {
        //
        schedular->set_gpu_ready(++barrier_id);

        // Current state
        switch (state)
        {
        case D_SYNC:     { state = W_SYNC; break; }
        case D_RSC_SYNC: { state = W_RSC_SYNC; break; }
        default: assert(0);
        }

        //
        return;
    }
    case D_NEW_SCENE_SYNC:
    case D_END_SCENE_SYNC:
    case D_FRAME_SYNC:
    {
        // Advance cores
        tick_cores();

        // Wait for all cores
        if (_l(core_state == Core::RuntimeState::RUNNING)) {
            return;
        }

        //
        schedular->set_gpu_ready(++barrier_id);

        // Current state
        switch (state)
        {
        case D_NEW_SCENE_SYNC: { state = W_NEW_SCENE_SYNC; break; }
        case D_END_SCENE_SYNC: { state = W_END_SCENE_SYNC; break; }
        case D_FRAME_SYNC:     { state = W_FRAME_SYNC; break; }
        default: assert(0);
        }

        return;
    }
    case W_SYNC:
    case W_RSC_SYNC:
    case W_NEW_SCENE_SYNC:
    case W_END_SCENE_SYNC:
    case W_FRAME_SYNC:
    {
        // Wait for CPU barrier
        if (_l(schedular->is_cpu_ready(barrier_id) == false)) {
            return;
        }

        //
        state = PROCESS_CMD;

        //
        return;
    }
    case PROCESS_CMD:
    {
        break;
    }
    default: assert(0);
    }

    //
    ProtoMessage::Packet pkt;

    // Next Packet
    if (_u(pb.gpu->read(pkt) == false)) {
        //
        return;
    }

    // Next state
    switch (pkt.cmd())
    {
    case gem5::NewSceneCMD:
    {
        //
        state = D_NEW_SCENE_SYNC;
        //
        return;
    }
    case gem5::EndSceneCMD:
    {
        //
        state = D_NEW_SCENE_SYNC;
        //
        return;
    }
    case gem5::NewJobCMD:
    case gem5::EndJobCMD:
    {
        //
        return;
    }
    case gem5::NewResourceCMD:
    case gem5::EndResourceCMD:
    {
        //
        state = D_RSC_SYNC;
        //
        return;
    }
    case gem5::SyncProvidesCMD:
    case gem5::SyncRequiresCMD:
    case gem5::SyncCMD:
    {
        //
        state = D_SYNC;
        //
        return;
    }
    case gem5::NewFrameCMD:
    {
        //
        state = D_FRAME_SYNC;
        //
        return;
    }
    default:
        assert(0);
    }
}

} // end namespace analyzer
} // end namespace gltracesim
