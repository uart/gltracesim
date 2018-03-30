#ifndef __GLTRACESIM_STATS_DISTRIBUTION_IMPL_HH__
#define __GLTRACESIM_STATS_DISTRIBUTION_IMPL_HH__

#include "stats/distribution.hh"

namespace gltracesim {
namespace stats {

inline void
Distribution::sample(size_t idx, int count)
{
    if (idx < min) {
        ++no_underflows;
    } else if (idx > max) {
        ++no_overflows;
    } else {
        //
        data[(double(idx - min) / (max - min)) * (no_buckets - 1)] += count;
    }
    ++no_samples;
}

} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_DISTRIBUTION_IMPL_HH__
