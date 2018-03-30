
#include "analyzer/cpu.hh"
#include "analyzer/gpu.hh"
#include "debug_impl.hh"

#include "gltracesim_analyzer.hh"

namespace gltracesim {
namespace analyzer {

CPU::CPU(const Json::Value &params,
    GlTraceSimAnalyzer *simulator, schedular::SchedularPtr schedular) :
    simulator(simulator), schedular(schedular), barrier_id(0),
    state(PROCESS_CMD)
{
    //
    ProtoMessage::PacketHeader hdr;

    pb.cpu = new ProtoInputStream(
        params["input-dir"].asString() + "/cpu.pb.gz"
    );

    //
    pb.cpu->read(hdr);

    DPRINTF(Init, "CPU CMD File Header [id:%s, ver:%i, tick_freq:%lu].\n",
        hdr.obj_id().c_str(), hdr.ver(), hdr.tick_freq()
    );

    // Create CPU core
    {
        Json::Value p;
        p["id"] = 0;
        p["dev"] = dev::CPU;

        //
        core = CorePtr(new Core(p, simulator, schedular));
    }
}

CPU::~CPU()
{
    delete pb.cpu;
}

void
CPU::tick()
{

    // Current state
    switch (state)
    {
    case D_SYNC:
    case D_RSC_SYNC:
    {
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
        //
        core->tick();
        //
        if (core->get_state() == Core::RuntimeState::RUNNING) {
            return;
        }

        // Current state
        switch (state)
        {
        case D_NEW_SCENE_SYNC: { state = W_NEW_SCENE_SYNC; break; }
        case D_END_SCENE_SYNC: { state = W_END_SCENE_SYNC; break; }
        case D_FRAME_SYNC:     { state = W_FRAME_SYNC; break; }
        default: ;
        }

        return;
    }
    case W_SYNC:
    case W_RSC_SYNC:
    case W_NEW_SCENE_SYNC:
    case W_END_SCENE_SYNC:
    case W_FRAME_SYNC:
    {
        //
        if (_l(schedular->is_gpu_ready(barrier_id + 1) == false)) {
            return;
        }

        schedular->set_cpu_ready(++barrier_id);

        // Current state
        switch (state)
        {
        case W_NEW_SCENE_SYNC:
        {
            simulator->handle_new_scene();
            //
            break;
        }
        case W_END_SCENE_SYNC:
        {
            simulator->handle_end_scene();
            //
            break;
        }
        case W_FRAME_SYNC:
        {
            //
            simulator->handle_end_frame();

            //
            if (system->is_running() == false) {
                return;
            }

            //
            simulator->handle_new_frame();
            //
            break;
        }
        default: ;
        }

        //
        state = PROCESS_CMD;

        //
        return;
    }
    case PROCESS_CMD:
    default:;
    }


    //
    ProtoMessage::Packet pkt;

    //
    if (_u(pb.cpu->read(pkt) == false)) {
        //
        DPRINTF(Info, "No more packets, exiting...\n");
        //
        return;
    }

//    printf("cmd: %lu\n", pkt.cmd());

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
        state = D_END_SCENE_SYNC;
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
    {
        //
        assert(pkt.has_rsc_id());

        //
        GpuResourcePtr gpu_resource =
            trace_manager->get_resource(pkt.rsc_id());

        assert(gpu_resource);
        assert(gpu_resource->id == pkt.rsc_id());

        //
        DPRINTF(GpuResourceEvent,
            "Resource: +%lu [addr: 0x%x, size: %fMB, type: %s].\n",
            gpu_resource->id,
            gpu_resource->addr_range.start,
            gpu_resource->addr_range.size() / 1048576.0,
            gpu_resource->get_target_name()
        );

        //
        system->rt->add(gpu_resource);
        //
        system->vmem_manager->alloc(gpu_resource->addr_range);

        //
        state = D_RSC_SYNC;
        //
        return;
    }
    case gem5::EndResourceCMD:
    {
        //
        assert(pkt.has_rsc_id());

        //
        GpuResourcePtr gpu_resource =
            system->rt->find_id(pkt.rsc_id());

        assert(gpu_resource);
        assert(gpu_resource->id == pkt.rsc_id());

        //
        DPRINTF(GpuResourceEvent, "Resource: -%lu [addr: 0x%x].\n",
            gpu_resource->id,
            gpu_resource->addr_range.start
        );

        // Move from dead map to dead vector
        system->rt->destroy(gpu_resource);
        //
        system->vmem_manager->free(gpu_resource->addr_range);

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
