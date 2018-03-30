#include <cmath>
#include <sstream>

#include "frame.hh"

#include "debug.hh"
#include "debug_impl.hh"

namespace gltracesim {

Frame::Frame(uint16_t id) : id(id)
{

}

Frame::~Frame()
{

}

void
Frame::dump_info(gltracesim::proto::FrameInfo *info)
{
    //
    info->set_id(id);
    //
    for (auto scene_id: scenes) {
        info->add_scene(scene_id);
    }
}

}
