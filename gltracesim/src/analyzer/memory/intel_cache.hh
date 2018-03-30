#ifndef __GLTRACESIM_ANALYZER_MEMORY_INTEL_CACHE_HH__
#define __GLTRACESIM_ANALYZER_MEMORY_INTEL_CACHE_HH__

#include <memory>

#include "util/cache.hh"
#include "util/cache_impl.hh"
#include "util/sat_counter.hh"

#include "stats/cache.hh"
#include "stats/cache_impl.hh"
#include "stats/distribution.hh"
#include "stats/distribution_impl.hh"

#include "analyzer/memory/base_cache.hh"

namespace gltracesim {
namespace analyzer {
namespace memory {


class IntelCacheModel : public BaseCacheModel
{

public:

    /**
     * @brief Analyzer
     */
    IntelCacheModel(const Json::Value &params);

    /**
     * @brief ~Analyzer
     */
    virtual ~IntelCacheModel();

    /**
     * @brief bypass
     * @param pkt
     * @return
     */
    virtual bool bypass(const packet_t &pkt);

protected:

    /**
     * @brief max_rsc_size
     */
    uint64_t max_rsc_size;

};

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_MEMORY_INTEL_CACHE_HH__
