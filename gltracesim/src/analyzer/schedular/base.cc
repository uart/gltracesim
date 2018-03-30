#include "frame.pb.h"
#include "scene.pb.h"
#include "job.pb.h"

#include "debug_impl.hh"
#include "gem5/packet.pb.h"

#include "analyzer/schedular/base.hh"
#include "analyzer/schedular/schedule.pb.h"

namespace gltracesim {
namespace analyzer {
namespace schedular {

Schedular::Schedular(const Json::Value &params)
{
    //
    system_barrier_id[dev::CPU] = 0;
    system_barrier_id[dev::GPU] = 0;

    //
    ProtoMessage::PacketHeader hdr;

    //
    hdr.set_obj_id("gltracesim");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    //
    pb.output_schedule = new ProtoOutputStream(
        params["output-dir"].asString() + "/output_schedule.pb.gz"
    );

    pb.output_schedule->write(hdr);

}

Schedular::~Schedular()
{
    delete pb.output_schedule;
}

void
Schedular::dump_schedule_descision(GpuJobPtr &job)
{
    //
    gltracesim::proto::JobSchedule schedule;
    //
    schedule.set_id(job->id);
    //
    schedule.set_core_id(job->core_id);
    //
    pb.output_schedule->write(schedule);
}

bool
Schedular::provide(uint64_t id)
{
    assert(0);
    return true;
}

bool
Schedular::require(uint64_t id)
{
    assert(0);
    return true;
}

} // end namespace schedular
} // end namespace analyzer
} // end namespace gltracesim

