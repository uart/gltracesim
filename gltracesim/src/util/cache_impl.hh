#ifndef __GLTRACESIM_UTIL_CACHE_IMPL_HH__
#define __GLTRACESIM_UTIL_CACHE_IMPL_HH__

#include <cmath>
#include "util/cache.hh"
#include "util/cflags.hh"

namespace gltracesim {

template<class params_t, class entry_t>
Cache<params_t, entry_t>::Cache(params_t *p) : params(*p)
{
    no_sets = params.size / (params.associativity * params.blk_size);
    no_blks = no_sets * params.associativity;
    no_sub_blks = params.blk_size / params.sub_blk_size;

    // Build data structure

    data.reserve(no_blks);
    data.resize(no_blks);
    for (size_t blk_idx = 0; blk_idx < no_blks; ++blk_idx)
    {
        //
        entry_t &ce = data[blk_idx];

        //
        ce.valid = false;
        ce.addr = 0;
    }

    //
    set_idx_mask = (no_sets - 1);
    blk_size_log2 = uint64_t(log2(params.blk_size));
    blk_addr_umask = ~(params.blk_size - 1);
}

template<class params_t, class entry_t>
uint64_t
Cache<params_t, entry_t>::get_set_idx(uint64_t addr) const
{
    return (addr >> blk_size_log2) & set_idx_mask;
}

template<class params_t, class entry_t>
uint64_t
Cache<params_t, entry_t>::get_sub_blk_idx(uint64_t addr) const
{
    return (addr & ~blk_addr_umask) / params.sub_blk_size;
}

template<class params_t, class entry_t>
uint64_t
Cache<params_t, entry_t>::get_no_sub_blks() const
{
    return no_sub_blks;
}

template<class params_t, class entry_t>
uint64_t
Cache<params_t, entry_t>::get_blk_addr(uint64_t addr) const
{
    return (addr & blk_addr_umask);
}

template<class params_t, class entry_t>
void
Cache<params_t, entry_t>::find(uint64_t addr, entry_t* &hit, entry_t* &evict)
{
    uint64_t blk_addr = get_blk_addr(addr);
    uint64_t set_idx = get_set_idx(addr) * params.associativity;

    uint64_t lru_tsc = ~uint64_t(0);
    int lru_idx = set_idx;

    hit = NULL;
    evict = NULL;

    // For each way
    for (size_t w = set_idx; w < (set_idx + params.associativity); ++w) {
        // Hit
        if (_u(data[w].valid && (data[w].addr == blk_addr))) {
            //
            hit = &data[w];

            // Replacement not needed
            return;
        }

        // Check invalid
        if (_u(data[w].valid == false)) {
            evict = &data[w];
        }

        // Check LRU
        if (_u(data[w].last_tsc < lru_tsc)) {
            lru_tsc = data[w].last_tsc;
            lru_idx = w;
        }
    }

    // No invalid entry, use LRU.
    if (evict == NULL) {
        evict = &data[lru_idx];
    }
}

template<class params_t, class entry_t>
typename Cache<params_t, entry_t>::data_t*
Cache<params_t, entry_t>::get_data()
{
    return &data;
}

} // end namespace gltracesim

#endif // __GLTRACESIM_UTIL_CACHE_IMPL_HH__
