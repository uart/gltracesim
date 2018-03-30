#include <cmath>

#include "stats/variable_distribution.hh"
#include "stats/variable_distribution_impl.hh"

namespace gltracesim {
namespace stats {

VariableDistribution::VariableDistribution()
{
    // Do nothing
}

VariableDistribution::VariableDistribution(
    size_t min_count, size_t filter_count, size_t filter_size)
{
    init(min_count, filter_count, filter_size);
}

VariableDistribution::~VariableDistribution()
{

}

void
VariableDistribution::init(
    size_t min_count, size_t filter_count, size_t filter_size)
{
    this->min_count = min_count;
    this->filter_count =  filter_count;
    this->filter_size =  filter_size;
    //
    reset();
}

void
VariableDistribution::reset()
{
    no_samples = 0;
    no_overflows = 0;

    // Clear count
    for (auto &e: filter.patterns) {
        e.second.count = 0;
    }

    //
    data.clear();
}

void
VariableDistribution::dump(gltracesim::proto::VariableDistribution *dist)
{
    dist->set_samples(no_samples);

    for (auto &e: data) {
        auto d = dist->add_dist();
        if (e.second >= min_count) {
            d->set_entry(e.first);
            d->set_samples(e.second);
        } else {
            no_overflows += e.second;
        }
    }

    dist->set_overflows(no_overflows);
}

void
VariableDistribution::sample(int64_t pattern, int count)
{
    auto it = data.find(pattern);

    if (it == data.end()) {

        // Insert new prefetch into history filter.
        filter.queue.push_back(pattern);

        // Update potentially important strides
        auto filter_it = filter.patterns.find(pattern);
        if (filter_it == filter.patterns.end()) {
            //
            filter.patterns[pattern].count = count;
            filter.patterns[pattern].ref_count = 1;
        } else {
            //
            filter_it->second.count += count;
            filter_it->second.ref_count++;

            // mv filter to data
            if (filter_it->second.count >= filter_count) {
                data[pattern] = filter_it->second.count;
            }
        }

        // Remove oldest stride
        if (filter.queue.size() == filter_size) {
            //
            int64_t old_pattern = filter.queue.front();

            //
            auto old_filter_it = filter.patterns.find(old_pattern);

            //
            assert(old_filter_it != filter.patterns.end());

            //
            old_filter_it->second.ref_count--;

            // No entry in list matches that pattern, filter it.
            if (old_filter_it->second.ref_count == 0) {
                 //
                if (data.count(old_pattern) == 0) {
                    no_overflows += old_filter_it->second.count;
                }
                //
                filter.patterns.erase(old_filter_it);
            }

            //
            filter.queue.pop_front();
        }
    } else {
        it->second += count;
    }

    //
    assert(filter.patterns.size() <= filter.queue.size());

    ++no_samples;
}

} // end namespace stats
} // end namespace gltracesim
