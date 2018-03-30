#include "filter_cache.hh"
#include "debug_impl.hh"

namespace gltracesim {

FilterCache::FilterCache(params_t *p) : Base(p), tick(0)
{
    //
    DPRINTF(Init, "FilterCache [size: %lu, a: %lu, blk: %lu, sblk: %lu].\n",
        p->size, p->associativity, p->blk_size, p->sub_blk_size
    );
}

void
FilterCache::dump_stats(gltracesim::proto::CacheStats *cache_stats)
{
    //
    cache_stats->set_id(params.id);

    //
    stats.dump(cache_stats);
}

void
FilterCache::reset_stats()
{
    stats.reset();
}

}
