#ifndef __GLTRACESIM_MODEL_CACHE_HH__
#define __GLTRACESIM_MODEL_CACHE_HH__

#include <memory>
#include <unordered_map>

#include "util/cache.hh"
#include "util/cache_impl.hh"
#include "util/sat_counter.hh"

#include "stats/cache.hh"
#include "stats/cache_impl.hh"
#include "stats/distribution.hh"
#include "stats/distribution_impl.hh"

#include "analyzer.hh"
#include "analyzer/memory/base.hh"
#include <json/json.h>

namespace gltracesim {
namespace analyzer {
namespace memory {

class BaseCacheModel : public BaseModel
{

protected:

    /**
     * @brief The filter_cache_params_t struct
     */
    struct cache_params_t : public gltracesim::cache_params_t
    {

    };

    /**
     * @brief sat_counter_t
     */
    typedef SatCounter<int8_t> sat_counter_t;

    /**
     * @brief blk_util_vector
     */
    typedef std::vector<sat_counter_t> blk_util_vector;

    /**
     * @brief The cache_entry_t struct
     */
    struct cache_entry_t : public gltracesim::cache_entry_t
    {
        /**
         * @brief frame number
         */
        uint16_t last_frame_nbr;

        /**
         * @brief scene number
         */
        uint16_t last_scene_nbr;

        /**
         * @brief job number
         */
        uint16_t last_job_nbr;

        /**
         * @brief rsc number
         */
        uint16_t last_rsc_nbr;

        /**
         * @brief touched_sub_blks
         */
        blk_util_vector sub_blk_ctrs;
    };

    /**
     * @brief cache_base_t
     */
    typedef gltracesim::Cache<cache_params_t, cache_entry_t> BaseCache;

    /**
     * @brief The BasicCache class
     */
    struct Cache : public BaseCache
    {
        //
        typedef BaseCache Base;
        typedef cache_params_t params_t;
        typedef cache_entry_t entry_t;

        /**
         * @brief BasicCache
         * @param p
         */
        Cache(params_t *p);

    };

    //
    typedef std::shared_ptr<Cache> CachePtr;

public:

    /**
     * @brief Analyzer
     */
    BaseCacheModel(const Json::Value &params);

    /**
     * @brief ~Analyzer
     */
    virtual ~BaseCacheModel();

    /**
     * @brief process
     * @param pkt
     */
    void process(const packet_t &pkt);

    /**
     * @brief bypass
     * @param pkt
     * @return
     */
    virtual bool bypass(const packet_t &pkt) { return false; }

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id) { /* do nothing */ }

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id) { /* do nothing */ }

    /**
     * @brief process
     * @param buffer
     */
    virtual void dump_stats();

    /**
     * @brief process
     * @param buffer
     */
    virtual void reset_stats();

protected:

    /**
     * @brief tick
     */
    size_t tick;

    /**
     * @brief install_wr_in_lru
     */
    bool fetch_on_wr_miss;

    /**
     * @brief cache
     */
    CachePtr cache;

    /**
     * @brief cache_stats
     */
    stats::Cache cache_stats;

    /**
     * @brief job_stats
     */
    std::unordered_map<uint64_t, stats::Cache> job_stats;

    /**
     * @brief core_stats
     */
    std::unordered_map<uint64_t, stats::Cache> core_stats;

    /**
     * @brief rsc_stats
     */
    std::unordered_map<uint64_t, stats::Cache> rsc_stats;

    /**
     * @brief blk_utilization
     */
    stats::Distribution blk_utilization;

    /**
     * @brief blk_reutilization
     */
    stats::Distribution blk_reutilization;

};

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_MODEL_CACHE_HH__
