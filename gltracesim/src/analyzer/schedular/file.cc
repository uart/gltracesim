#include <algorithm>
#include "debug_impl.hh"

#include "analyzer/schedular/schedule.pb.h"

#include "analyzer/trace_manager.hh"
#include "analyzer/schedular/file.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

FileSchedular::FileSchedular(const Json::Value &params)
    : Schedular(params), next_scene_id(0), next_frame_id(0)
{
    //
    DPRINTF(Init, "FileSchedular.\n");

    //
    ProtoMessage::PacketHeader hdr;

    //
    pb.schedule = new ProtoInputStream(
        p["schedule"].asString()
    );

    //
    pb.cluster = new ProtoInputStream(
        p["clusters"].asString()
    );

    //
    pb.schedule->read(hdr);
    pb.cluster->read(hdr);

    //
    gltracesim::proto::Schedule schedule;

    //
    if (pb.schedule->read(schedule) == false) {
        assert(0);
    }

    while (true) {

        //
        gltracesim::proto::Cluster cluster;

        //
        if (pb.cluster->read(cluster) == false) {
            break;
        }

        //
        FramePtr frame = FramePtr(new Frame(info.id()));

        //
        for (int i = 0; i < info.scene_size(); ++i) {
            frame->add_scene(info.scene(i));
        }

        //
        frames.push_back(frame);
    }
}

FileSchedular::~FileSchedular()
{

}

GpuJobPtr
FileSchedular::get_next_cpu_job(int core_id)
{
    if (cpu_queue.empty()) {
        return NULL;
    }

    //
    auto job = cpu_queue.front();
    //
    cpu_queue.pop_front();
    //
    return job;
}

GpuJobPtr
FileSchedular::get_next_gpu_job(int core_id)
{
    if (gpu_queue.empty()) {
        return NULL;
    }

    //
    auto job = gpu_queue.front();
    //
    gpu_queue.pop_front();
    //
    DPRINTF(ScheduleEvent, "GPU Job: %d [x: %d, y: %d].\n",
        job->id, job->x, job->y
    );
    //
    return job;
}

void
FileSchedular::start_new_frame(int frame_id)
{
    assert(cpu_queue.empty());
    assert(gpu_queue.empty());

    //
    current_frame = trace_manager->get_frame(next_frame_id);

    DPRINTF(ScheduleEvent, "New Frame: %d [scenes: %lu].\n",
        current_frame->id,
        current_frame->scenes.size()
    );

    //
    assert(current_frame->id == next_frame_id);

    next_frame_id++;
}

void
FileSchedular::start_new_scene(int frame_id, int scene_id)
{
    assert(cpu_queue.empty());
    assert(gpu_queue.empty());

    //
    current_scene = trace_manager->get_scene(next_scene_id);

    DPRINTF(ScheduleEvent, "New Scene: %d [frame: %d, width: %d, height: %d, jobs: %lu].\n",
        current_scene->id,
        current_scene->frame_id,
        current_scene->width,
        current_scene->height,
        current_scene->jobs.size()
    );

    next_scene_id++;
}

} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

