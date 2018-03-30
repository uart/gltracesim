#include <algorithm>
#include "debug_impl.hh"

#include "analyzer/trace_manager.hh"
#include "analyzer/schedular/fcfs.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

FCFSSchedular::FCFSSchedular(const Json::Value &params)
    : Schedular(params), next_scene_id(0), next_frame_id(0)
{

}

FCFSSchedular::~FCFSSchedular()
{

}

GpuJobPtr
FCFSSchedular::get_next_cpu_job(int core_id)
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
FCFSSchedular::get_next_gpu_job(int core_id)
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
FCFSSchedular::start_new_frame(int frame_id)
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
FCFSSchedular::start_new_scene(int frame_id, int scene_id)
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

    std::vector<GpuJobPtr> pending_draw_jobs;
    std::vector<GpuJobPtr> pending_tile_jobs;
    std::vector<GpuJobPtr> pending_misc_jobs;

    // Fill job queues
    for (auto job_id: current_scene->jobs) {
        //
        GpuJobPtr job = trace_manager->get_job(current_scene->frame_id, job_id);

        //
        if (job->dev == dev::CPU) {
            //
            cpu_queue.push_back(job);
            //
            continue;
        }

        // GPU
        assert(job->dev == dev::GPU);

        if (job->type == GpuJob::DRAW_JOB) {
            pending_draw_jobs.push_back(job);
        }

        if (job->type == GpuJob::TILE_JOB) {
            pending_tile_jobs.push_back(job);
        }

        if (job->type == GpuJob::MISC_JOB) {
            pending_misc_jobs.push_back(job);
        }
    }

    struct cmp_t {
        cmp_t(int width) : width(width) {}
        int width;
        bool operator()(GpuJobPtr a, GpuJobPtr b)
        {
            return (a->y * width + a->x) < (b->y * width + b->x);
        }
    };

    //
    std::sort(
        pending_tile_jobs.begin(),
        pending_tile_jobs.end(),
        cmp_t(current_scene->width)
    );

    gpu_queue.insert(
        gpu_queue.end(),
        pending_draw_jobs.begin(),
        pending_draw_jobs.end()
    );

    gpu_queue.insert(
        gpu_queue.end(),
        pending_tile_jobs.begin(),
        pending_tile_jobs.end()
    );

    gpu_queue.insert(
        gpu_queue.end(),
        pending_misc_jobs.begin(),
        pending_misc_jobs.end()
    );

    //
    DPRINTF(ScheduleEvent, "Loaded %lu CPU jobs.\n", cpu_queue.size());
    DPRINTF(ScheduleEvent, "Loaded %lu GPU jobs.\n", gpu_queue.size());

    next_scene_id++;
}

} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

