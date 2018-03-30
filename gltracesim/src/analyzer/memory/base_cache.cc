#include <sstream>

#include "analyzer/memory/base_cache.pb.h"
#include "analyzer/memory/base_cache.hh"

#include "debug_impl.hh"
#include "system.hh"

namespace gltracesim {
namespace analyzer {
namespace memory {

/**
 * @brief The VanillaCacheAnalyzerBuilder struct
 */
struct BaseCacheModelBuilder : public gltracesim::AnalyzerBuilder
{
    BaseCacheModelBuilder() : AnalyzerBuilder("BaseCache") {
        // Do nothing
    }

    AnalyzerPtr create(const Json::Value &params) {
        return AnalyzerPtr(new BaseCacheModel(params));
    }
};

//
BaseCacheModelBuilder vanilla_cache_analyzer_builder;

BaseCacheModel::Cache::Cache(params_t *p) : Base(p)
{
    for (size_t blk_idx = 0; blk_idx < no_blks; ++blk_idx) {
        //
        entry_t &ce = data[blk_idx];
        //
        ce.last_frame_nbr = 0;
        //
        ce.sub_blk_ctrs.resize(no_sub_blks, sat_counter_t(0, 100));
    }
}

BaseCacheModel::BaseCacheModel(const Json::Value &p) :
    BaseModel(p), tick(0)
{
    //
    fetch_on_wr_miss = p.get("fetch-on-wr-miss", true).asBool();

    //
    Cache::params_t params;

    //
    params.size = p["size"].asInt();
    params.associativity = p.get("associativity", 8).asInt();
    params.blk_size = p.get("blk-size", 64).asInt();
    params.sub_blk_size = p.get("sub-blk-size", 64).asInt();

    DPRINTF(Init, "BaseCacheAnalyzer [id: %i, size: %lu, a: %lu, blk: %lu, sblk: %lu].\n",
        id, params.size, params.associativity, params.blk_size, params.sub_blk_size
    );

    //
    cache = CachePtr(new Cache(&params));

    //
    blk_utilization.init(0, params.blk_size / params.sub_blk_size, 1);
    blk_reutilization.init(0, params.blk_size / params.sub_blk_size, 1);

    reset_stats();
}

BaseCacheModel::~BaseCacheModel()
{
    DPRINTF(Init, "-BaseCacheAnalyzer [id: %i].\n", id);
}

void
BaseCacheModel::process(const packet_t &pkt)
{
    // Skip other commands
    if (_u(pkt.cmd != READ && pkt.cmd != WRITE)) {
        return;
    }

    // Update time
    ++tick;

    //
    Cache::entry_t *ce, *re;
    //
    cache->find(pkt.paddr, ce, re);

    //
    if (_u(ce)) {
        assert(ce->valid);

        //
        ce->last_tsc = tick;
        //
        ce->dirty |= (pkt.cmd == WRITE);

        //
        ce->sub_blk_ctrs[cache->get_sub_blk_idx(pkt.paddr)]++;

        // Intra-frame reuse
        if (_l(ce->last_frame_nbr == system->get_frame_nbr())) {
            //
            cache_stats.intra_frame_hits[pkt.cmd]++;
        } else {
            //
            ce->last_frame_nbr = system->get_frame_nbr();
        }

        // Intra-scene reuse
        if (_l(ce->last_scene_nbr == system->get_scene_nbr())) {
            //
            cache_stats.intra_scene_hits[pkt.cmd]++;
        } else {
            //
            ce->last_scene_nbr = system->get_scene_nbr();
        }

        // Intra-job reuse
        if (_l(ce->last_job_nbr == pkt.job_id)) {
            //
            cache_stats.intra_job_hits[pkt.cmd]++;
        } else {
            //
            ce->last_job_nbr = pkt.job_id;
        }

        // Intra-rsc reuse
        if (_l(ce->last_rsc_nbr == pkt.rsc_id)) {
            //
            cache_stats.intra_rsc_hits[pkt.cmd]++;
        } else {
            //
            ce->last_rsc_nbr = pkt.rsc_id;
        }

        //
        cache_stats.hits[pkt.cmd]++;
        cache_stats.gpuside[pkt.cmd] += cache->params.sub_blk_size;

        job_stats[pkt.job_id].hits[pkt.cmd]++;
        job_stats[pkt.job_id].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        core_stats[pkt.tid].hits[pkt.cmd]++;
        core_stats[pkt.tid].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        rsc_stats[pkt.rsc_id].hits[pkt.cmd]++;
        rsc_stats[pkt.rsc_id].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        // Nothing else to do
        return;
    }

    //
    cache_stats.misses[pkt.cmd]++;
    job_stats[pkt.job_id].misses[pkt.cmd]++;
    rsc_stats[pkt.rsc_id].misses[pkt.cmd]++;

    if (_u((pkt.cmd == WRITE) && fetch_on_wr_miss == false)) {
        // Do nothing, only install, no fetch
    } else {
        cache_stats.memside[pkt.cmd] += cache->params.blk_size;
        job_stats[pkt.job_id].memside[pkt.cmd] += cache->params.blk_size;
        core_stats[pkt.tid].memside[pkt.cmd] += cache->params.blk_size;
        rsc_stats[pkt.rsc_id].memside[pkt.cmd] += cache->params.blk_size;

        // Create a mutible copy
        packet_t apkt = pkt;
        apkt.cmd = READ;

        // Forward packet
        send_packet(apkt);
    }

    // Check if we should bypass
    if (_u(bypass(pkt))) {
        return;
    }

    //
    assert(re);

    // Evict and make room
    if (_l(re->valid)) {
        cache_stats.evictions++;
        job_stats[pkt.job_id].evictions++;
        core_stats[pkt.tid].evictions++;
        rsc_stats[pkt.rsc_id].evictions++;

        uint8_t touched_sub_blks = 0;
        uint8_t reused_sub_blks = 0;
        uint8_t sub_blk_idx = 0;
        do
        {
            //
            if (_l(re->sub_blk_ctrs[sub_blk_idx].value() > 0))
            {
                ++touched_sub_blks;
                if (_u(re->sub_blk_ctrs[sub_blk_idx].value() > 1)) {
                    ++reused_sub_blks;
                }
                // Clear it for replacement
                re->sub_blk_ctrs[sub_blk_idx].reset();
            }
            //
            ++sub_blk_idx;

        } while (_u(sub_blk_idx < re->sub_blk_ctrs.size()));
        //
        blk_utilization.sample(touched_sub_blks);
        blk_reutilization.sample(reused_sub_blks);

        //
        if (_u(re->dirty)) {
            //
            cache_stats.writebacks++;
            job_stats[pkt.job_id].writebacks++;
            core_stats[pkt.tid].writebacks++;
            rsc_stats[pkt.rsc_id].writebacks++;

            cache_stats.memside[pkt.cmd] += cache->params.blk_size;
            job_stats[pkt.job_id].memside[pkt.cmd] += cache->params.blk_size;
            core_stats[pkt.tid].memside[pkt.cmd] += cache->params.blk_size;
            rsc_stats[pkt.rsc_id].memside[pkt.cmd] += cache->params.blk_size;

            // Send eviction to memside
            packet_t apkt;
            apkt.vaddr = re->addr;
            apkt.paddr = re->addr;
            apkt.tid = pkt.tid;
            apkt.cmd = WRITE;
            apkt.rsc_id = re->rsc_id;
            apkt.job_id = re->job_id;
            apkt.dev_id = re->dev_id;
            apkt.length = cache->params.blk_size;

            //
            send_packet(apkt);
        }
    }

    // Insert blk
    re->valid = true;
    re->dirty = (pkt.cmd == WRITE);
    re->addr = cache->get_blk_addr(pkt.paddr);
    re->rsc_id = pkt.rsc_id;
    re->job_id = pkt.job_id;
    re->dev_id = pkt.dev_id;
    re->last_tsc = tick;
    re->last_frame_nbr = system->get_frame_nbr();
    re->last_scene_nbr = system->get_scene_nbr();
    re->last_job_nbr = pkt.job_id;
    re->last_rsc_nbr = pkt.rsc_id;

    re->sub_blk_ctrs[cache->get_sub_blk_idx(pkt.paddr)].set(1);
}

void
BaseCacheModel::dump_stats()
{
    // Global Cache stats
    {
        gltracesim::proto::BaseCacheStats stats;
        //
        stats.set_frame_id(system->get_frame_nbr());
        stats.set_scene_id(system->get_scene_nbr());
        //
        cache_stats.dump(stats.mutable_cache_stats());
        //
        blk_utilization.dump(stats.mutable_blk_utilization());
        blk_reutilization.dump(stats.mutable_blk_reutilization());
        //
        pb.stats->write(stats);
    }

    // Per job stats
    for (auto &job : job_stats) {
        //
        gltracesim::proto::BaseCacheJobStats stats;
        //
        stats.set_id(std::get<0>(job));
        //
        std::get<1>(job).dump(stats.mutable_cache_stats());
        //
        pb.job_stats->write(stats);
    }

    // Per job stats
    for (auto &core : core_stats) {
        //
        gltracesim::proto::BaseCacheCoreStats stats;
        //
        stats.set_id(std::get<0>(core));
        //
        std::get<1>(core).dump(stats.mutable_cache_stats());
        //
        pb.core_stats->write(stats);
    }

    // Per resource stats
    for (auto &rsc : rsc_stats) {
        //
        gltracesim::proto::BaseCacheRscStats stats;
        //
        stats.set_id(std::get<0>(rsc));
        //
        stats.set_frame_id(system->get_frame_nbr());
        //
        stats.set_scene_id(system->get_scene_nbr());
        //
        std::get<1>(rsc).dump(stats.mutable_cache_stats());
        //
        pb.rsc_stats->write(stats);
    }
}

void
BaseCacheModel::reset_stats()
{
    //
    cache_stats.reset();
    blk_utilization.reset();
    blk_reutilization.reset();

    job_stats.clear();
    core_stats.clear();
    rsc_stats.clear();
}

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

