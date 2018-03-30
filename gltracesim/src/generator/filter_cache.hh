#ifndef __GLTRACESIM_FILTER_CACHE_HH__
#define __GLTRACESIM_FILTER_CACHE_HH__

#include <memory>
#include <cstdint>
#include <cmath>
#include <cassert>

#include "util/cache.hh"
#include "util/cache_impl.hh"
#include "stats/cache.hh"
#include "stats/cache.pb.h"
#include "stats/cache_impl.hh"

#include "gltracesim.pb.h"

namespace gltracesim {

struct filter_cache_params_t : public cache_params_t
{
    /**
     * @brief addr
     */
    uint64_t id;

    /**
     * @brief addr
     */
    bool filter_cache;

    /**
     * @brief fetch_on_wr_miss
     */
    bool fetch_on_wr_miss;

};

//
struct filter_cache_entry_t : public cache_entry_t
{
    /**
     * @brief paddr
     */
    uint64_t paddr;
};

//
typedef Cache<filter_cache_params_t, filter_cache_entry_t> filter_cache_base_t;

/**
 * @brief The FilterCache class
 */
class FilterCache : public filter_cache_base_t
{

public:

    //
    typedef filter_cache_base_t Base;

    //
    typedef filter_cache_params_t params_t;

    //
    typedef filter_cache_entry_t entry_t;

public:

    /**
     * @brief stats
     */
    gltracesim::stats::Cache stats;

public:

    /**
     * @brief Cache
     * @param p
     */
    FilterCache(params_t *p);

public:

    /**
     * @brief dump_stats
     * @param c
     */
    void dump_stats(gltracesim::proto::CacheStats *cache_stats);

    /**
     * @brief reset_stats
     */
    void reset_stats();

public:

    /**
     * @brief tick
     */
    size_t tick;

};

//
typedef std::shared_ptr<FilterCache> FilterCachePtr;

} // end namespace gltracesim

#endif // __GLTRACESIM_FILTER_CACHE_HH__
