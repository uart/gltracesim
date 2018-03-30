#ifndef __GLTRACESIM_BUFFER_HH__
#define __GLTRACESIM_BUFFER_HH__

#include <array>
#include "packet.hh"

namespace gltracesim {

// Buffer size
#define BUFFER_SIZE 4096

//
struct buffer_t
{
    //
    buffer_t();
    //
    uint64_t pos;
    //
    std::array<packet_t, BUFFER_SIZE> data;
};

} // end namespace gltracesim

#endif // __GLTRACESIM_BUFFER_HH__
