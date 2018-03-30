#include <cmath>
#include "debug_impl.hh"

#include "analyzer/trace_manager.hh"
#include "analyzer/schedular/z.hh"

namespace gltracesim {
namespace analyzer {
namespace schedular {

ZSchedular::ZSchedular(const Json::Value &params)
    : Schedular(params), next_scene_id(0), next_frame_id(0)
{
    //
    z_width = params.get("z-width", -1).asInt();
}

ZSchedular::~ZSchedular()
{

}

GpuJobPtr
ZSchedular::get_next_cpu_job(int core_id)
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
ZSchedular::get_next_gpu_job(int core_id)
{
    if (gpu_draw_queue.size()) {
        //
        auto job = gpu_draw_queue.front();
        //
        DPRINTF(ScheduleEvent, "GPU Draw Job: %d [x: %d, y: %d].\n",
            job->id, job->x, job->y
        );
        //
        gpu_draw_queue.pop_front();
        //
        return job;
    }

    if (gpu_tile_queue.size()) {
        //
        auto job = gpu_tile_queue.front();
        //
        DPRINTF(ScheduleEvent, "GPU Tile Job: %d [x: %d, y: %d].\n",
            job->id, job->x, job->y
        );
        //
        gpu_tile_queue.pop_front();
        //
        return job;
    }

    if (gpu_misc_queue.size()) {
        //
        auto job = gpu_misc_queue.front();
        //
        DPRINTF(ScheduleEvent, "GPU Misc Job: %d [x: %d, y: %d].\n",
            job->id, job->x, job->y
        );
        //
        gpu_misc_queue.pop_front();
        //
        return job;
    }

    return NULL;
}

void
ZSchedular::start_new_frame(int frame_id)
{
    assert(gpu_draw_queue.empty());
    assert(gpu_tile_queue.empty());
    assert(gpu_misc_queue.empty());
    //
    current_frame = trace_manager->get_frame(next_frame_id);

    DPRINTF(ScheduleEvent, "New Frame: %d [scenes: %lu].\n",
        current_frame->id,
        current_frame->scenes.size()
    );

    //
    assert(current_frame->id == next_frame_id);
    //
    next_frame_id++;
}

void
ZSchedular::schedule_tiles(int x, int y, int size, int order)
{
    // Cant partition furhter
    if (size == 1) {
        // Find job
        coord_t coord;
        //
        coord.first = x;
        coord.second = y;

        //
        auto it = pending_tile_jobs.find(coord);

        // Coord is outside scene
        if (it == pending_tile_jobs.end()) {
            return;
        }

        DPRINTF(ScheduleEvent, "Assign Job: %d [x: %d, y: %d].\n",
            it->second->id,
            x, y
        );

        //
        gpu_tile_queue.push_back(it->second);

        //
        return;
    }

    int sub_size = size / 2;

    if (order == TILE_ACBD) {
        // Top Left
        schedule_tiles(x, y, sub_size, TILE_ACBD);
        // Bottom Left
        schedule_tiles(x, y + sub_size, sub_size, TILE_ACBD);
        // Top Right
        schedule_tiles(x + sub_size, y, sub_size, TILE_ACBD);
        // Bottom Right
        schedule_tiles(x + sub_size, y + sub_size, sub_size, TILE_ACBD);
    } else if (order == TILE_CADB) {
        // Bottom Left
        schedule_tiles(x, y + sub_size, sub_size, TILE_CADB);
        // Top Left
        schedule_tiles(x, y, sub_size, TILE_CADB);
        // Bottom Right
        schedule_tiles(x + sub_size, y + sub_size, sub_size, TILE_CADB);
        // Top Right
        schedule_tiles(x + sub_size, y, sub_size, TILE_CADB);
    }
}

void
ZSchedular::start_new_scene(int frame_id, int scene_id)
{
    assert(cpu_queue.empty());
    assert(gpu_draw_queue.empty());
    assert(gpu_tile_queue.empty());
    assert(gpu_misc_queue.empty());

    //
    current_scene = trace_manager->get_scene(next_scene_id);

    DPRINTF(ScheduleEvent, "New Scene: %d [frame: %d, width: %d, height: %d, jobs: %lu].\n",
        current_scene->id,
        current_scene->frame_id,
        current_scene->width,
        current_scene->height,
        current_scene->jobs.size()
    );

    // Fill job queues
    for (auto job_id: current_scene->jobs) {
        //
        GpuJobPtr job = trace_manager->get_job(current_scene->frame_id, job_id);
        //
        assert(job->id == job_id);

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
            gpu_draw_queue.push_back(job);
        }

        if (job->type == GpuJob::TILE_JOB) {
            // Find job
            coord_t coord;
            //
            coord.first = job->x;
            coord.second = job->y;

            pending_tile_jobs[coord] = job;
        }

        if (job->type == GpuJob::MISC_JOB) {
            gpu_misc_queue.push_back(job);
        }
    }

    //
    unsigned size = std::max(current_scene->width, current_scene->height);

    // Convert to power of 2
    size = unsigned(pow(2, ceil(log2(size))));

    //
    int real_z_width = (z_width > 0) ? z_width : size;

    // Check if we have any tiles
    if (pending_tile_jobs.size()) {
        for (size_t y = 0; y < size; y += real_z_width) {
            for (size_t x = 0; x < size; x += real_z_width) {
                //
                schedule_tiles(
                    x, // x
                    y, // y
                    real_z_width, // size
                    TILE_ACBD
                );
            }
        }
    }

    //
    pending_tile_jobs.clear();

    DPRINTF(ScheduleEvent, "Loaded %lu CPU jobs.\n", cpu_queue.size());
    DPRINTF(ScheduleEvent, "Loaded %lu GPU draw jobs.\n", gpu_draw_queue.size());
    DPRINTF(ScheduleEvent, "Loaded %lu GPU tile jobs.\n", gpu_tile_queue.size());
    DPRINTF(ScheduleEvent, "Loaded %lu GPU misc jobs.\n", gpu_misc_queue.size());

    // Check that everything is scheduled
    assert((cpu_queue.size() +
            gpu_draw_queue.size() +
            gpu_tile_queue.size() +
            gpu_misc_queue.size()) == current_scene->jobs.size());

    //
    next_scene_id++;
}

} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

