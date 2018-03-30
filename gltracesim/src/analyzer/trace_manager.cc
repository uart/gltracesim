#include <sstream>
#include <algorithm>

#include "frame.pb.h"
#include "scene.pb.h"
#include "job.pb.h"

#include "debug_impl.hh"

#include "gem5/packet.pb.h"
#include "gem5/protoio.hh"
#include "analyzer/trace_manager.hh"

namespace gltracesim {

//
TraceManagerPtr trace_manager;

TraceManager::TraceManager(const Json::Value &p)
    : fs_prefetcher(0), frame_id(0), scene_id(0), job_frame_id(0),
      resource_id(0)
{

    //
    DPRINTF(Init, "TraceManager.\n");

    //
    pb.frames = new ProtoInputStream(
        p["input-dir"].asString() + "/frames.pb.gz"
    );
    pb.scenes = new ProtoInputStream(
        p["input-dir"].asString() + "/scenes.pb.gz"
    );
    pb.jobs = new ProtoInputStream(
        p["input-dir"].asString() + "/jobs.pb.gz"
    );
    pb.resources = new ProtoInputStream(
        p["input-dir"].asString() + "/resources.pb.gz"
    );

    //
    ProtoMessage::PacketHeader hdr;

    //
    pb.frames->read(hdr);
    pb.scenes->read(hdr);
    pb.jobs->read(hdr);
    pb.resources->read(hdr);

}

TraceManager::~TraceManager()
{
    //
    if (fs_prefetcher) {
        //
        fs_prefetcher->join();
        //
        delete fs_prefetcher;
    }
    //
    delete pb.frames;
    delete pb.scenes;
    delete pb.jobs;
    delete pb.resources;
}

void
TraceManager::prefetch_fs_metadata()
{
    //
    for (auto &it : jobs) {
        //
        GpuJobPtr &job = std::get<1>(it);

        std::stringstream filename;
        filename << system->get_input_dir() << "/"
            << "f" << job->frame_id << "/"
            << "s" << job->scene_id << "/"
            << "j" << job->id << ".trace.pb.gz";

        //
        std::ifstream ifs(
            filename.str().c_str(), std::ios::in | std::ios::binary
        );

        //
        if (!ifs.good()) {
            DPRINTF(Warn, "Error reading %s\n", filename.str().c_str());
        }
    }
}

FramePtr
TraceManager::get_frame(size_t id)
{
    //
    FramePtr frame = NULL;

    //
    if (last_frame && last_frame->id == id) {
        return last_frame;
    }

    // Start from beginning
    if (frame_id < id) {
        //
        frame_id = 0;
        //
        pb.frames->reset();
    }

    while (frame == NULL) {

        //
        gltracesim::proto::FrameInfo info;

        //
        if (pb.frames->read(info) == false) {
            break;
        }

        //
        if (frame_id == id) {
            assert(info.id() == id);

            //
            frame = FramePtr(new Frame(info.id()));

            //
            for (int i = 0; i < info.scene_size(); ++i) {
                frame->add_scene(info.scene(i));
            }
        }

        //
        ++frame_id;
    }

    //
    last_frame = frame;

    //
    return frame;
}

ScenePtr
TraceManager::get_scene(size_t id)
{
    //
    ScenePtr scene = NULL;

    //
    if (last_scene && last_scene->id == id) {
        return last_scene;
    }

    // Start from beginning
    if (scene_id < id) {
        //
        scene_id = 0;
        //
        pb.scenes->reset();
    }

    while (scene == NULL) {

        //
        gltracesim::proto::SceneInfo info;

        //
        if (pb.scenes->read(info) == false) {
            break;
        }

        //
        if (scene_id == id) {
            //
            scene = ScenePtr(new Scene(info.id(), info.frame_id()));

            //
            scene->set_global_id(scene_id);

            //
            for (int i = 0; i < info.job_size(); ++i) {
                scene->add_job(info.job(i));
            }

            //
            for (int i = 0; i < info.opengl_call_size(); ++i) {
                scene->add_opengl_call(info.opengl_call(i));
            }

            //
            scene->width = info.width();
            scene->height = info.height();
        }

        //
        ++scene_id;
    }

    //
    last_scene = scene;

    //
    return scene;
}

GpuJobPtr
TraceManager::get_job(size_t frame_id, size_t id)
{
    // Get job within current scene
    if (jobs.size() && job_frame_id == frame_id) {
        //
        assert(jobs.count(id));
        //
        return jobs[id];
    }

    // Start from beginning
    if (job_frame_id < frame_id) {
        //
        job_frame_id = 0;
        //
        pb.jobs->reset();
    }

    // Wait
    if (fs_prefetcher) {
        //
        fs_prefetcher->join();
        //
        delete fs_prefetcher;
        //
        fs_prefetcher = NULL;
    }

    //
    jobs.clear();

    // Find and load scene
    while (true) {

        //
        gltracesim::proto::JobInfo info;

        //
        if (pb.jobs->read(info) == false) {
            break;
        }

        //
        if (info.frame_id() < frame_id) {
            continue;
        }

        //
        if (info.frame_id() > frame_id) {
            break;
        }

        //
        GpuJobPtr job;

        if (info.type() == proto::TILE_JOB) {
            job = GpuJobPtr(
                new GpuTileJob(
                    info.id(),
                    info.scene_id(),
                    info.frame_id(),
                    info.x(),
                    info.y()
                )
            );
        } else if (info.type() == proto::DRAW_JOB) {
            job = GpuJobPtr(
                new GpuDrawJob(
                    info.id(),
                    info.scene_id(),
                    info.frame_id()
                )
            );
        } else if (info.type() == proto::MISC_JOB) {
            job = GpuJobPtr(
                new GpuMiscJob(
                    info.id(),
                    info.scene_id(),
                    info.frame_id(),
                    (info.dev_id() == 0) ? dev::CPU : dev::GPU
                )
            );
        } else {
            assert(0);
        }

        //
        jobs[job->id] = job;
    }

    //
    DPRINTF(Init, "Loaded %lu jobs.\n", jobs.size());

    //
    assert(jobs.count(id));

    //
    job_frame_id = frame_id;

    //
    return jobs[id];
}


GpuResourcePtr
TraceManager::get_resource(size_t id)
{
    //
    GpuResourcePtr resource = NULL;

    // Start from beginning
    if (resource_id < id) {
        //
        resource_id = 0;
        //
        pb.resources->reset();
    }

    while (resource == NULL) {

        //
        gltracesim::proto::ResourceInfo info;

        //
        if (pb.resources->read(info) == false) {
            break;
        }

        //
        if (resource_id == id) {
            // Allocate a dummy texture
            struct llvmpipe_resource lpr = { 0 };
            lpr.base.target = pipe_texture_target(info.type());
            lpr.base.format = pipe_format(info.format());
            lpr.base.last_level = info.mipmap_levels();
            lpr.base.nr_samples = info.msaa_samples();
            lpr.base.width0 = info.width();
            lpr.base.height0 = info.height();
            lpr.base.depth0 = info.depth();
            lpr.base.array_size = info.array_size();

            lpr.dt = (info.display_target()) ? (sw_displaytarget*) 0x1 : NULL;
            lpr.data = (void*) info.start_addr();
            lpr.tex_data = lpr.data;
            lpr.total_alloc_size = (info.end_addr() - info.start_addr());

            //
            resource = GpuResourcePtr(new GpuResource(&lpr, dev::CPU));

            //
            resource->id = info.id();
            //
            system->rt->map(resource);
        }

        //
        resource_id++;
    }

    //
    return resource;
}


} // end namespace gltracesim

