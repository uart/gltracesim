#ifndef __GLTRACESIM_STATS_DISTRIBUTION_HH__
#define __GLTRACESIM_STATS_DISTRIBUTION_HH__

#include <vector>
#include "stats/distribution.pb.h"

namespace gltracesim {
namespace stats {

class Distribution {

public:

    /**
     * @brief Analyzer
     */
    Distribution();

    /**
     * @brief Analyzer
     */
    Distribution(size_t min, size_t max, size_t bucket_size);

    /**
     * @brief ~Analyzer
     */
    virtual ~Distribution();

    /**
     * @brief Analyzer
     */
    void init(size_t min, size_t max, size_t bucket_size);

    /**
     * @brief reset
     */
    void reset();

    /**
     * @brief dump
     * @param cache_stats
     */
    void dump(gltracesim::proto::Distribution *dist);

    /**
     * @brief sample
     * @param idx
     * @param count
     */
    void sample(size_t idx, int count = 1);

private:

    /**
     * @brief min
     */
    size_t min;

    /**
     * @brief max
     */
    size_t max;

    /**
     * @brief bucket_size
     */
    size_t bucket_size;

    /**
     * @brief no_buckets
     */
    size_t no_buckets;

private:

    /**
     * @brief samples
     */
    uint64_t no_samples;

    /**
     * @brief samples
     */
    uint64_t no_underflows;

    /**
     * @brief samples
     */
    uint64_t no_overflows;

    /**
     * @brief dist
     */
    std::vector<uint64_t> data;

};

} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_DISTRIBUTION_HH__
