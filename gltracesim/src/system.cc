#include "system.hh"

namespace gltracesim {

//
SystemPtr system;

System::System(const Json::Value &config) :
    config(config),
    frame_nbr(0),
    scene_nbr(0),
    global_scene_idx(0),
    job_nbr(0),
    blk_size(64),
    tsc(0)
{
    // Do nothing
}

System::~System()
{
    // Do nothing
}

} // end namespace gltracesim

