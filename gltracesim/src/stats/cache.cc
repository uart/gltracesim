
#include "stats/cache.hh"
#include "stats/cache_impl.hh"

namespace gltracesim {
namespace stats {

Cache::Cache()
{
    reset();
}

Cache::~Cache()
{

}

Cache&
Cache::operator +=(const Cache &other)
{
    gpuside += other.gpuside;
    memside += other.memside;
    hits += other.hits;
    misses += other.misses;

    intra_frame_hits += other.intra_frame_hits;
    intra_scene_hits += other.intra_scene_hits;
    intra_job_hits += other.intra_job_hits;
    intra_rsc_hits += other.intra_rsc_hits;

    writebacks += other.writebacks;
    evictions += other.evictions;

    return *this;
}

void
Cache::reset()
{
    gpuside.reset();
    memside.reset();
    hits.reset();
    misses.reset();

    intra_frame_hits.reset();
    intra_scene_hits.reset();
    intra_job_hits.reset();
    intra_rsc_hits.reset();

    writebacks = 0;
    evictions = 0;
}

void
Cache::dump(gltracesim::proto::CacheStats *cache_stats)
{
    cache_stats->mutable_gpuside()->set_read(gpuside[READ]);
    cache_stats->mutable_gpuside()->set_write(gpuside[WRITE]);

    cache_stats->mutable_memside()->set_read(memside[READ]);
    cache_stats->mutable_memside()->set_write(memside[WRITE]);

    cache_stats->mutable_hits()->set_read(hits[READ]);
    cache_stats->mutable_hits()->set_write(hits[WRITE]);

    cache_stats->mutable_misses()->set_read(misses[READ]);
    cache_stats->mutable_misses()->set_write(misses[WRITE]);

    cache_stats->set_writebacks(writebacks);
    cache_stats->set_evictions(evictions);

    cache_stats->mutable_intra_frame_hits()->set_read(intra_frame_hits[READ]);
    cache_stats->mutable_intra_frame_hits()->set_write(intra_frame_hits[WRITE]);

    cache_stats->mutable_intra_scene_hits()->set_read(intra_scene_hits[READ]);
    cache_stats->mutable_intra_scene_hits()->set_write(intra_scene_hits[WRITE]);

    cache_stats->mutable_intra_job_hits()->set_read(intra_job_hits[READ]);
    cache_stats->mutable_intra_job_hits()->set_write(intra_job_hits[WRITE]);

    cache_stats->mutable_intra_rsc_hits()->set_read(intra_rsc_hits[READ]);
    cache_stats->mutable_intra_rsc_hits()->set_write(intra_rsc_hits[WRITE]);
}

} // end namespace stats
} // end namespace gltracesim

