#include <tuple>
#include <sstream>
#include <algorithm>

#include "analyzer/memory/base_cache.pb.h"
#include "analyzer/memory/ls_cache.hh"
#include "analyzer/memory/ls_cache.pb.h"

#include "analyzer/trace_manager.hh"

#include "debug_impl.hh"
#include "system.hh"

namespace gltracesim {
namespace analyzer {
namespace memory {

/**
 * @brief The VanillaCacheAnalyzerBuilder struct
 */
struct LimitStudyCacheModelBuilder : public gltracesim::AnalyzerBuilder
{
    LimitStudyCacheModelBuilder() : AnalyzerBuilder("LimitStudyCache") {
        // Do nothing
    }

    AnalyzerPtr create(const Json::Value &params) {
        return AnalyzerPtr(new LimitStudyCacheModel(params));
    }
};

//
LimitStudyCacheModelBuilder limit_study_cache_analyzer_builder;

void
LimitStudyCacheModel::load_inter_scene_sharing_data(int frame_id)
{
    //
    std::stringstream filename;
    //
    filename << params["classification-input-dir"].asString() << "/"
             << params["benchmark-name"].asString() << "/"
             << params["benchmark-name"].asString() << "_"
             << "fr_" << frame_id << "_"
             << "cacheline_classification.pb.gz";

    {
        //
        std::ifstream infile(filename.str());
        //
        if (infile.good() == false) {
            //
            DPRINTF(Warn, "%s does not exist.\n", filename.str().c_str());
            //
            if (params.get("exit-on-missing-fclassification", true).asBool()) {
                //
                DPRINTF(Warn, "exiting...\n", filename.str().c_str());
                //
                system->stop();
            }
            //
            return;
        }
    }

    //
    ProtoInputStream* pis = new ProtoInputStream(filename.str());

    //
    while (true) {

        //
        gltracesim::proto::FrameCachelineUsers fcu;

        //
        if (pis->read(fcu) == false) {
            break;
        }

        assert(fcu.frame_id() == uint32_t(frame_id));

        //
        if (fcu.scene_users() > 1) {
            //
            uint64_t blk_id = fcu.addr() >> cache->blk_size_log2;
            //
            inter_scene_sharing_mask.insert(blk_id);
        }
    }

//    printf("%s: %lu.\n", filename.c_str(), inter_scene_sharing_mask.size());

    //
    delete pis;
}

void
LimitStudyCacheModel::load_intra_scene_sharing_data(int frame_id, int scene_id)
{
    //
    std::stringstream filename;
    //
    filename << params["classification-input-dir"].asString() << "/"
             << params["benchmark-name"].asString() << "/"
             << params["benchmark-name"].asString() << "_"
             << "fr_" << frame_id << "_"
             << "sc_" << scene_id << "_"
             << "cacheline_classification.pb.gz";

    {
        //
        std::ifstream infile(filename.str());
        //
        if (infile.good() == false) {
            //
            DPRINTF(Warn, "%s does not exist.\n", filename.str().c_str());
            //
            return;
        }
    }

    //
    ProtoInputStream* pis = new ProtoInputStream(filename.str());

    //
    while (true) {

        //
        gltracesim::proto::SceneCachelineUsers scu;

        //
        if (pis->read(scu) == false) {
            break;
        }

        assert(scu.frame_id() == uint32_t(frame_id));
//        assert(scu.scene_id() == uint32_t(scene_id));

        //
        if (scu.task_users() > 1) {
            //
            uint64_t blk_id = scu.addr() >> cache->blk_size_log2;
            //
            intra_scene_sharing_mask.insert(blk_id);
        }
    }

//    printf("%s: %lu.\n", filename.c_str(), intra_scene_sharing_mask.size());

    //
    delete pis;
}


LimitStudyCacheModel::LimitStudyCacheModel(const Json::Value &p) :
    BaseCacheModel(p)
{
    //
    filter_inter_scene_sharing =
        p.get("filter-inter-scene-sharing", false).asBool();
    //
    filter_intra_scene_sharing =
        p.get("filter-intra-scene-sharing", false).asBool();
    //
    filter_intra_task_sharing =
        p.get("filter-intra-task-sharing", false).asBool();

    DPRINTF(Init, "LimitStudyCacheAnalyzer [id: %i, filter: %i, %i, %i].\n",
        id, filter_inter_scene_sharing, filter_intra_scene_sharing,
        filter_intra_task_sharing
    );

}

LimitStudyCacheModel::~LimitStudyCacheModel()
{
    DPRINTF(Init, "-LimitStudyCacheAnalyzer [id: %i].\n", id);
}

void
LimitStudyCacheModel::process(const packet_t &pkt)
{
    // Skip other commands
    if (_u(pkt.cmd != READ && pkt.cmd != WRITE)) {
        return;
    }

    // Update time
    ++tick;


    // If we hit in filter
    bool filter_hit = false;

    //
    uint64_t blk_id = pkt.paddr >> cache->blk_size_log2;

    //
    if (filter_inter_scene_sharing) {
        //
        auto it = inter_scene_sharing_mask.find(blk_id);

        //
        if (it != inter_scene_sharing_mask.end()) {

            //
            auto jt = inter_scene_sharing_mask_accesses.find(blk_id);

            // New
            if (jt == inter_scene_sharing_mask_accesses.end()) {
                //
                inter_scene_sharing_mask_accesses.insert(blk_id);
            } else {
                //
                filter_hit = true;
            }
        }
    }

    //
    if (filter_intra_scene_sharing) {
        //
        auto it = intra_scene_sharing_mask.find(blk_id);

        //
        if (it != intra_scene_sharing_mask.end()) {

            //
            auto jt = intra_scene_sharing_mask_accesses.find(blk_id);

            // New
            if (jt == intra_scene_sharing_mask_accesses.end()) {
                //
                intra_scene_sharing_mask_accesses.insert(blk_id);
            } else {
                //
                filter_hit = true;
            }
        }
    }

    //
    if (filter_intra_task_sharing) {
        //
        auto it = intra_task_sharing_mask.find(pkt.job_id);

        //
        if (it == intra_task_sharing_mask.end()) {
            auto _it = intra_task_sharing_mask.insert(
                { pkt.job_id, addr_set_t() }
            );

            //
            assert(std::get<1>(_it));

            //
            it = std::get<0>(_it);
        }

        //
        assert(it != intra_task_sharing_mask.end());

        //
        auto jt = std::get<1>(*it).find(blk_id);

        // New
        if (jt == std::get<1>(*it).end()) {
            //
            std::get<1>(*it).insert(blk_id);
        } else {
            //
            filter_hit = true;
        }
    }

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
    if (filter_hit) {
        cache_stats.hits[pkt.cmd]++;
        cache_stats.gpuside[pkt.cmd] += cache->params.sub_blk_size;

        job_stats[pkt.job_id].hits[pkt.cmd]++;
        job_stats[pkt.job_id].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        core_stats[pkt.tid].hits[pkt.cmd]++;
        core_stats[pkt.tid].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        rsc_stats[pkt.rsc_id].hits[pkt.cmd]++;
        rsc_stats[pkt.rsc_id].gpuside[pkt.cmd] += cache->params.sub_blk_size;

        //
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
LimitStudyCacheModel::start_new_frame(int frame_id)
{
    //
    if (filter_inter_scene_sharing) {
        //
        assert(inter_scene_sharing_mask.size() ==
               inter_scene_sharing_mask_accesses.size());
        //
        inter_scene_sharing_mask.clear();
        //
        inter_scene_sharing_mask_accesses.clear();
        //
        load_inter_scene_sharing_data(frame_id);
    }
}

void
LimitStudyCacheModel::start_new_scene(int frame_id, int scene_id)
{
    //
    if (filter_intra_scene_sharing) {
        //
        assert(intra_scene_sharing_mask.size() ==
               intra_scene_sharing_mask_accesses.size());
        //
        intra_scene_sharing_mask.clear();
        //
        intra_scene_sharing_mask_accesses.clear();
        //
        load_intra_scene_sharing_data(frame_id, scene_id);
    }

    //
    if (filter_intra_task_sharing) {
        //
        intra_task_sharing_mask.clear();
    }

}

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

