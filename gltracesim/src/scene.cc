#include <cmath>
#include <sstream>

#include "scene.hh"

#include "debug.hh"
#include "debug_impl.hh"

namespace gltracesim {

Scene::Scene(uint16_t id, uint16_t frame_id)
    : id(id), frame_id(frame_id), global_id(0), width(0), height(0)
{

}

Scene::~Scene()
{

}

void
Scene::dump_info(gltracesim::proto::SceneInfo *info)
{
    //
    info->set_frame_id(frame_id);
    info->set_id(id);
    info->set_width(width);
    info->set_height(height);
    //
    for (auto job_id: jobs) {
        info->add_job(job_id);
    }

    //
    for (auto call_id: opengl_calls) {
        info->add_opengl_call(call_id);
    }
}

}
