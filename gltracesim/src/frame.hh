#ifndef __GLTRACESIM_FRAME_HH__
#define __GLTRACESIM_FRAME_HH__

#include <chrono>
#include <vector>
#include <memory>
#include <cstdint>

#include "frame.pb.h"
#include "device.hh"

#include "util/timer.hh"

namespace gltracesim {

class Frame : public Timer
{

public:

    /**
     * @brief Frame
     */
    Frame(uint16_t id);

    /**
     *
     */
    ~Frame();

public:

    /**
     * @brief add_job
     * @param job_id
     */
    void add_scene(size_t scene_id) {
        scenes.push_back(scene_id);
    }

public:

    /**
     * @brief id
     */
    uint16_t id;

    /**
     * @brief jobs
     */
    std::vector<uint16_t> scenes;

public:

    /**
     * @brief dump_info
     * @param info
     */
    virtual void dump_info(gltracesim::proto::FrameInfo *info);

};

//
typedef std::shared_ptr<Frame> FramePtr;

} // end namespace gltracesim


#endif // __GLTRACESIM_FRAME_HH__
