#ifndef __GLTRACESIM_PACKET_HH__
#define __GLTRACESIM_PACKET_HH__

#include <vector>

#include "device.hh"
#include "resource.hh"

namespace gltracesim {

enum AccessType {
    // CPU
    READ = 0,
    WRITE,
    WRITEBACK,
    SYNC,
    NEW_JOB,
    END_JOB,
    MV_TO_CPU,
    MV_TO_GPU,
    NEW_SCENE,
    END_SCENE,
    NEW_FRAME,
    NEW_RESOURCE,
    END_RESOURCE,
    OPENGL_CALL,
    //
    NUM_ACCESS_TYPES
};

//
struct packet_t
{
    //
    packet_t()
        : inst(0x0),
          vaddr(0x0),
          paddr(0x0),
          length(0),
          cmd(READ),
          tid(0),
          job_id(-1),
          dev_id(dev::CPU)
    {
        // Do nothing
    }

    //
    void* inst;
    //
    uint64_t vaddr;
    //
    uint64_t paddr;
    //
    uint8_t length;
    //
    uint8_t cmd;
    //
    uint8_t tid;
    //
    int job_id;
    //
    int rsc_id;
    //
    uint8_t dev_id;
};

} // end namespace gltracesim

#endif // __GLTRACESIM_PACKET_HH__
