#include <sstream>

#include "analyzer/memory/base_cache.pb.h"
#include "analyzer/memory/intel_cache.hh"

#include "debug_impl.hh"
#include "system.hh"

namespace gltracesim {
namespace analyzer {
namespace memory {

/**
 * @brief The VanillaCacheAnalyzerBuilder struct
 */
struct IntelCacheModelBuilder : public gltracesim::AnalyzerBuilder
{
    IntelCacheModelBuilder() : AnalyzerBuilder("IntelCache") {
        // Do nothing
    }

    AnalyzerPtr create(const Json::Value &params) {
        return AnalyzerPtr(new IntelCacheModel(params));
    }
};

//
IntelCacheModelBuilder intel_cache_analyzer_builder;

IntelCacheModel::IntelCacheModel(const Json::Value &p) :
    BaseCacheModel(p)
{
    DPRINTF(Init, "IntelCacheAnalyzer [id: %i].\n",
        id
    );

    max_rsc_size = p["max-rsc-size"].asUInt64();
}

IntelCacheModel::~IntelCacheModel()
{
    DPRINTF(Init, "-IntelCacheAnalyzer [id: %i].\n", id);
}

bool
IntelCacheModel::bypass(const packet_t &pkt)
{
    //
    GpuResourcePtr gpu_resource = system->rt->find_id(pkt.rsc_id);
    //
    assert(gpu_resource);

    //
    if (gpu_resource->size() > max_rsc_size) {
        return true;
    } else {
        return false;
    }
}

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

