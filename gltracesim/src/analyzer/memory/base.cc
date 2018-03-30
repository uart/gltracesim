#include <sstream>

#include "analyzer/memory/base.hh"
#include "gem5/packet.pb.h"
#include "debug_impl.hh"

namespace gltracesim {
namespace analyzer {
namespace memory {

BaseModel::BaseModel(const Json::Value &p) :
    Analyzer(p, p["id"].asInt())
{
    //
    ProtoMessage::PacketHeader hdr;

    //
    hdr.set_obj_id("gltracesim-stats");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    std::stringstream basename;
    basename << params["output-dir"].asCString() << "/" << id;

    //
    pb.stats = new ProtoOutputStream(
        basename.str() + ".stats.pb.gz"
    );
    //
    pb.job_stats = new ProtoOutputStream(
        basename.str() + ".job_stats.pb.gz"
    );
    //
    pb.core_stats = new ProtoOutputStream(
        basename.str() + ".core_stats.pb.gz"
    );
    //
    pb.rsc_stats = new ProtoOutputStream(
        basename.str() +  ".rsc_stats.pb.gz"
    );

    //
    pb.stats->write(hdr);
    pb.job_stats->write(hdr);
    pb.core_stats->write(hdr);
    pb.rsc_stats->write(hdr);

    DPRINTF(Init, "BaseAnalyzer [id: %i].\n", id);

}

BaseModel::~BaseModel()
{
    delete pb.stats;
    delete pb.job_stats;
    delete pb.core_stats;
    delete pb.rsc_stats;

    DPRINTF(Init, "-BaseAnalyzer [id: %i].\n", id);
}

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim



