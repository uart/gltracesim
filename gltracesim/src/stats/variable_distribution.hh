#ifndef __GLTRACESIM_STATS_VARIABLE_DISTRIBUTION_HH__
#define __GLTRACESIM_STATS_VARIABLE_DISTRIBUTION_HH__

#include <map>
#include <deque>
#include "stats/variable_distribution.pb.h"

namespace gltracesim {
namespace stats {

class VariableDistribution {

public:

    /**
     * @brief Analyzer
     */
    VariableDistribution();

    /**
     * @brief Analyzer
     */
    VariableDistribution(
        size_t min_count, size_t filter_count, size_t filter_size
    );

    /**
     * @brief ~Analyzer
     */
    virtual ~VariableDistribution();

    /**
     * @brief Analyzer
     */
    void init(size_t min_count, size_t filter_count, size_t filter_size);

    /**
     * @brief reset
     */
    void reset();

    /**
     * @brief dump
     * @param cache_stats
     */
    void dump(gltracesim::proto::VariableDistribution *dist);

    /**
     * @brief sample
     * @param idx
     * @param count
     */
    void sample(int64_t idx, int count = 1);

private:

    /**
     * @brief min
     */
    size_t min_count;

    /**
     * @brief min
     */
    size_t filter_count;

    /**
     * @brief max
     */
    size_t filter_size;

private:

    /**
     * @brief samples
     */
    uint64_t no_samples;

    /**
     * @brief samples
     */
    uint64_t no_overflows;

    /**
     * @brief The prefetch_history_t struct
     */
    struct filter_t
    {

        struct count_t {
            uint64_t count;
            int ref_count;
        };

        /**
         * @brief filter
         */
        std::map<int64_t, count_t> patterns;

        /**
         * @brief filter
         */
        std::deque<int64_t> queue;

    } filter;

    /**
     * @brief data
     */
    std::map<int64_t, uint64_t> data;

};

} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_VARIABLE_DISTRIBUTION_HH__
