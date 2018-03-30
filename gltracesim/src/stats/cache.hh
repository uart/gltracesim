#ifndef __GLTRACESIM_STATS_CACHE_HH_
#define __GLTRACESIM_STATS_CACHE_HH_

#include <memory>
#include <array>

#include "packet.hh"
#include "stats/cache.pb.h"
#include "stats/vector_impl.hh"

namespace gltracesim {
namespace stats {

class Cache
{

public:

    /**
     * @brief Cache
     */
    Cache();

    /**
     * @brief Cache
     */
    ~Cache();

    /**
     * @brief operator +
     * @param other
     */
    Cache& operator+=(const Cache &other);

    /**
     * @brief reset
     */
    void reset();

    /**
     * @brief dump
     * @param cache_stats
     */
    void dump(gltracesim::proto::CacheStats *cache_stats);

public:

    typedef stats::IntVector<NUM_ACCESS_TYPES> stats_vector_t;

    stats_vector_t gpuside;
    stats_vector_t memside;
    stats_vector_t hits;
    stats_vector_t misses;

    uint64_t writebacks;
    uint64_t evictions;

    stats_vector_t intra_frame_hits;
    stats_vector_t intra_scene_hits;
    stats_vector_t intra_job_hits;
    stats_vector_t intra_rsc_hits;
};

} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_CACHE_HH_
