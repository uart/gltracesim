#include <cmath>

#include "stats/distribution.hh"
#include "stats/distribution_impl.hh"

namespace gltracesim {
namespace stats {

Distribution::Distribution()
{
    // Do nothing
}

Distribution::Distribution(size_t min, size_t max, size_t bucket_size)
{
    init(min, max, bucket_size);
}

Distribution::~Distribution()
{

}

void
Distribution::init(size_t min, size_t max, size_t bucket_size)
{
    this->min = min;
    this->max = max;
    this->bucket_size =  bucket_size;
    //
    no_buckets = std::ceil((max - min + 1.0) / bucket_size);
    //
    data.resize(no_buckets);
    //
    reset();
}

void
Distribution::reset()
{
    no_samples = 0;
    no_underflows = 0;
    no_overflows = 0;

    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = 0;
    }
}

void
Distribution::dump(gltracesim::proto::Distribution *dist)
{
    dist->set_samples(no_samples);
    dist->set_underflows(no_underflows);
    dist->set_overflows(no_overflows);

    for (size_t i = 0; i < data.size(); ++i) {
        dist->add_dist(data[i]);
    }
}

} // end namespace stats
} // end namespace gltracesim
