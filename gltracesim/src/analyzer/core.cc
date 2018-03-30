#include "analyzer/core.hh"

#include "debug_impl.hh"

#include "gltracesim_analyzer.hh"

namespace gltracesim {
namespace analyzer {

Core::Core(const Json::Value &params,
    GlTraceSimAnalyzer *simulator, schedular::SchedularPtr schedular) :
    simulator(simulator), schedular(schedular), state(RUNNING)
{
    id = params["id"].asInt();
    dev = params["dev"].asInt();
}

void
Core::tick()
{
    //
    state = RUNNING;

    // Done with previous job
    if (_u(job == NULL || job->pkts.empty())) {

        // Try to get a new job from the schedular
        job = schedular->get_next_job(id, dev);

        // Did not get any job, continue
        if (job == NULL) {
            //
            state = IDLE;
            //
            return;
        }

        // Load pakcets
        job->load_trace();

        //
        stats.no_jobs++;
    }

    if (_u(job == NULL || job->pkts.empty())) {
        return;
    }

    //
    auto &pkt = job->pkts.front();
    // Core id
    pkt.tid = id;
    // Handle packet
    simulator->send_packet(pkt);
    // Remove from queue
    job->pkts.pop_front();
    //
    stats.no_pkts++;

    //
    state = RUNNING;
}

} // end namespace analyzer
} // end namespace gltracesim
