#ifndef __GLTRACESIM_ANALYZER_MEMORY_LIMIT_STUDY_CACHE_HH__
#define __GLTRACESIM_ANALYZER_MEMORY_LIMIT_STUDY_CACHE_HH__

#include <memory>
#include <unordered_map>
#include <unordered_set>

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


class LimitStudyCacheModel : public BaseCacheModel
{

public:

    /**
     * @brief Analyzer
     */
    LimitStudyCacheModel(const Json::Value &params);

    /**
     * @brief ~Analyzer
     */
    virtual ~LimitStudyCacheModel();

    /**
     * @brief process
     * @param pkt
     */
    virtual void process(const packet_t &pkt);

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id);

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id);

protected:

    /**
     * @brief filter_inter_scene_sharing
     */
    bool filter_inter_scene_sharing;

    /**
     * @brief filter_inter_scene_sharing
     */
    bool filter_intra_scene_sharing;

    /**
     * @brief filter_inter_scene_sharing
     */
    bool filter_intra_task_sharing;

protected:

    /**
     * @brief load_inter_scene_sharing_data
     * @param frame_id
     */
    void load_inter_scene_sharing_data(
        int frame_id
    );

    /**
     * @brief load_intra_scene_sharing_data
     * @param frame_id
     * @param scene_id
     */
    void load_intra_scene_sharing_data(
        int frame_id,
        int scene_id
    );

protected:

    /**
     * @brief addr_set_t
     */
    typedef std::unordered_set<uint64_t> addr_set_t;

    /**
     * @brief inter_scene_sharing_mask
     */
    addr_set_t inter_scene_sharing_mask;

    /**
     * @brief inter_scene_sharing_mask_asseses
     */
    addr_set_t inter_scene_sharing_mask_accesses;

    /**
     * @brief intra_scene_sharing_mask
     */
    addr_set_t intra_scene_sharing_mask;

    /**
     * @brief inter_scene_sharing_mask_asseses
     */
    addr_set_t intra_scene_sharing_mask_accesses;

    /**
     * @brief intra_task_sharing_mask
     */
    std::unordered_map<uint64_t, addr_set_t> intra_task_sharing_mask;

};

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_MEMORY_LIMIT_STUDY_CACHE_HH__
