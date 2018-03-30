#ifndef __GLTRACESIM_UTIL_CACHE_HH__
#define __GLTRACESIM_UTIL_CACHE_HH__

#include <vector>
#include <cstdint>

namespace gltracesim {

//
struct cache_entry_t
{
    /**
     * @brief valid
     */
    bool valid;

    /**
     * @brief dirty
     */
    bool dirty;

    /**
     * @brief The Anonymous:1 union
     */
    union
    {
        uint64_t addr;
        uint64_t tag;
    };

    /**
     * @brief tsc
     */
    uint64_t last_tsc;

    /**
     * @brief gpu_resource
     */
    int32_t rsc_id;

    /**
     * @brief gpu_resource
     */
    int32_t job_id;

    /**
     * @brief gpu_resource
     */
    uint8_t dev_id;
};

//
struct cache_params_t
{
    /**
     * @brief size
     */
    uint64_t size;

    /**
     * @brief associativity
     */
    uint64_t associativity;

    /**
     * @brief blk_size
     */
    uint64_t blk_size;

    /**
     * @brief sub_blk_size
     */
    uint64_t sub_blk_size;

};

template<class params_t, class entry_t>
class Cache
{

public:

    /**
     * @brief params
     */
    params_t params;

    /**
     * @brief data_t
     */
    typedef std::vector<entry_t> data_t;

public:

    /**
     * @brief Cache
     * @param p
     */
    Cache(params_t *p);

public:

    /**
     * @brief Set index.
     */
    uint64_t get_set_idx(uint64_t addr) const;

    /**
     * @brief Set index.
     */
    uint64_t get_sub_blk_idx(uint64_t addr) const;

    /**
     * @brief get_no_sub_blk
     */
    uint64_t get_no_sub_blks() const;

    /**
     * @brief get_set_addr
     * @param addr
     * @return
     */
    uint64_t get_blk_addr(uint64_t addr) const;

    /**
     * @brief find
     * @param addr
     * @return
     */
    void find(uint64_t addr, entry_t* &hit, entry_t* &evict);

    /**
     * @brief get_data
     * @return
     */
    data_t* get_data();

public:

    /**
     * @brief data
     */
    data_t data;

    /**
     * @brief addr
     */
    uint64_t no_sets;

    /**
     * @brief addr
     */
    uint64_t no_blks;

    /**
     * @brief addr
     */
    uint64_t no_sub_blks;

    /**
     * @brief addr
     */
    uint64_t set_idx_mask;

    /**
     * @brief addr
     */
    uint64_t blk_size_log2;

    /**
     * @brief addr
     */
    uint64_t blk_addr_umask;

};

} // end namespace gltracesim

#endif // __GLTRACESIM_UTIL_CACHE_HH__
