#include <chrono>

#include "debug.hh"
#include "debug_impl.hh"
#include "gltracesim_analyzer.hh"
#include "util/addr_range.hh"
#include "util/addr_range_map.hh"
#include "resource_impl.hh"
#include "resource_tracker_impl.hh"

#include "analyzer/cpu.hh"
#include "analyzer/gpu.hh"

#include "analyzer/schedular/fcfs.hh"
#include "analyzer/schedular/z.hh"
#include "analyzer/schedular/random.hh"

#include "gltracesim.pb.h"

namespace gltracesim {

void
GlTraceSimAnalyzer::stop_timer_loop(uint64_t seconds)
{
    //
    std::this_thread::sleep_for(std::chrono::seconds(seconds));
    //
    DPRINTF(Init, "Stop timer triggered. Exiting...\n");
    //
    system->stop();
}

GlTraceSimAnalyzer::GlTraceSimAnalyzer(const std::string &output_dir) :
    cpu(NULL), gpu(NULL)
{
    //
    Json::Value config;

    // Open Config File
    std::ifstream config_file(output_dir + "/config.json");

    // Load JSON config
    config_file >> config;

    //
    config["output-dir"] = output_dir.c_str();

    //
    Debug::init(config["debug"]);

    // Sync
    conf.use_rsc_sync = config.get("use-rsc-sync", false).asBool();
    conf.use_global_sync = config.get("use-global-sync", false).asBool();
    //
    conf.num_gpu_cores = config.get("num-gpu-cores", 1).asInt();
    conf.batch_size = config.get("batch-size", 4096).asUInt();

    // Set fast forward frames
    sim_ctrl.start = config.get("start-frame", 0).asInt();
    // Set stop
    sim_ctrl.stop = config.get("stop-frame", INT_MAX).asInt();

    // Enable timer
    uint64_t seconds = config.get("stop-time", 0).asUInt64();
    if (seconds)
    {
        DPRINTF(Init, "Stop timer will trigger in %lus.\n",
            seconds
        );
        //
        stop_timer = new std::thread([=] { stop_timer_loop(seconds); });
    }

    //
    system = SystemPtr(new System(config));

    // System blk size
    system->set_blk_size(
        config["filter-cache"].get("blk-size", 64).asInt()
    );
    //
    system->set_input_dir(config["input-dir"].asString());
    //
    system->rt = ResourceTrackerPtr(
        new ResourceTracker()
    );
    //
    system->vmem_manager = VirtualMemoryManagerPtr(
        new VirtualMemoryManager(config["virtual-memory"])
    );

    //
    trace_manager = TraceManagerPtr(new TraceManager(config));

    //
    ProtoMessage::PacketHeader hdr;

    pb.stats = new ProtoOutputStream(
        output_dir + "/stats.pb.gz"
    );

    //
    hdr.set_obj_id("gltracesim-stats");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    //
    pb.stats->write(hdr);

    //
    for (unsigned i = 0; i < config["models"].size(); ++i) {
        //
        Json::Value &params = config["models"][i];
        //
        params["output-dir"] = config["output-dir"];
        params["benchmark-name"] = config["benchmark-name"];

        //
        AnalyzerBuilder* builder = gltracesim::Analyzer::find_builder(
            params["type"].asCString()
        );

        //
        if (builder == NULL) {
            //
            DPRINTF(Init, "No such analyzer (%s).\n",
                params["type"].asCString()
            );
            //
            exit(EXIT_FAILURE);
        }

        DPRINTF(Init, "Loading Analyzer (%s).\n", builder->get_name().c_str());

        //
        AnalyzerPtr analyzer = builder->create(params);

        //
        assert(analyzer);

        //
        analyzers.push_back(analyzer);
    }

    //
    auto schedular_type = config["schedular"].get("type", "fcfs").asString();
    //
    config["schedular"]["output-dir"] = config["output-dir"];

    if (schedular_type == "fcfs") {
        schedular = analyzer::schedular::SchedularPtr(
            new analyzer::schedular::FCFSSchedular(
                config["schedular"]
            )
        );
    } else if (schedular_type == "z") {
        schedular = analyzer::schedular::SchedularPtr(
            new analyzer::schedular::ZSchedular(
                config["schedular"]
            )
        );
    } else if (schedular_type == "r") {
        schedular = analyzer::schedular::SchedularPtr(
            new analyzer::schedular::RandomSchedular(
                config["schedular"]
            )
        );
    } else {
        assert(0);
    }

    //
    cpu = new analyzer::CPU(config, this, schedular);
    gpu = new analyzer::GPU(config, this, schedular);

    //
    handle_new_frame();
}

GlTraceSimAnalyzer::~GlTraceSimAnalyzer()
{
    delete cpu;
    delete gpu;
    delete pb.stats;
}

void
GlTraceSimAnalyzer::handle_end_scene()
{
    //
    for (auto &analyzer: analyzers) {
        //
        analyzer->dump_stats();
        //
        analyzer->reset_stats();
    }

    //
    system->inc_scene_nbr();
}


void
GlTraceSimAnalyzer::handle_new_scene()
{
    //
    for (auto &analyzer: analyzers) {
        //
        analyzer->start_new_scene(
            system->get_frame_nbr(),
            system->get_scene_nbr()
        );
    }

    //
    schedular->start_new_scene(
        system->get_frame_nbr(),
        system->get_scene_nbr()
    );
}

void
GlTraceSimAnalyzer::handle_end_frame()
{
    //
    gltracesim::proto::Frame frame;

    //
    frame.set_id(current_frame->id);
    frame.set_fast_forwarded(sim_ctrl.start > 0);

    //
    current_frame->stop();

    //
    frame.mutable_sim_stats()->set_duration(current_frame->duration());

    // Time scaling factor
    double tsf = 1.0 / (current_frame->duration() * 1048576);

    size_t tot_pkts = cpu->get_core()->stats.no_pkts;
    size_t tot_jobs = cpu->get_core()->stats.no_jobs;;

    for (size_t core_id = 0; core_id < conf.num_gpu_cores; ++core_id) {
        tot_jobs += gpu->get_core(core_id)->stats.no_jobs;
        tot_pkts += gpu->get_core(core_id)->stats.no_pkts;
    }

    //
    DPRINTF(GpuFrameEvent,
        "Frame: %lu [jobs: %lu, pkts: %lu, %.2fM/s, duration: %fs]\n",
        current_frame->id,
        tot_jobs, tot_pkts, tot_pkts * tsf,
        current_frame->duration()
    );

    //
    DPRINTF(GpuFrameEvent, "  CPU: [jobs: %lu, pkts: %lu, %.2fM/s]\n",
        cpu->get_core()->stats.no_jobs,
        cpu->get_core()->stats.no_pkts,
        cpu->get_core()->stats.no_pkts * tsf
    );

    //
    cpu->get_core()->stats.no_jobs = 0;
    cpu->get_core()->stats.no_pkts = 0;

    //
    for (size_t core_id = 0; core_id < conf.num_gpu_cores; ++core_id) {
        //
        DPRINTF(GpuFrameEvent,
            "  GPU core: %3i [jobs: %lu, pkts: %lu, %.2fM/s]\n",
            core_id,
            gpu->get_core(core_id)->stats.no_jobs,
            gpu->get_core(core_id)->stats.no_pkts,
            gpu->get_core(core_id)->stats.no_pkts * tsf
        );

        gpu->get_core(core_id)->stats.no_jobs = 0;
        gpu->get_core(core_id)->stats.no_pkts = 0;
    }

    //
    pb.stats->write(frame);

    //
    system->inc_frame_nbr();
    //
    system->set_scene_nbr(0);

    // Fast-forwardning
    sim_ctrl.start = std::max(0, sim_ctrl.start - 1);

    // All done, exit
    if (sim_ctrl.stop == 0) {
        //
        DPRINTF(GpuFrameEvent, "All done, exiting...\n");
        // Stop sim loop
        system->stop();
        //
        return;
    }

    //
    --sim_ctrl.stop;

    // Flush so we can see progress on cluster log files.
    fflush(stdout);
}

void
GlTraceSimAnalyzer::handle_new_frame()
{
    //
    current_frame = trace_manager->get_frame(system->get_frame_nbr());

    //
    if (current_frame == NULL) {
        //
        DPRINTF(GpuFrameEvent, "All done, exiting...\n");
        // Stop sim loop
        system->stop();
        //
        return;
    }

    //
    for (auto &analyzer: analyzers) {
        //
        analyzer->start_new_frame(
            system->get_frame_nbr()
        );
    }

    //
    schedular->start_new_frame(
        system->get_frame_nbr()
    );

    //
    current_frame->start();
}

void
GlTraceSimAnalyzer::send_packet(packet_t &pkt)
{
    //
    system->inc_tsc();
    //
    for (auto &analyzer: analyzers) {
        //
        analyzer->process(pkt);
    }
}

void
GlTraceSimAnalyzer::run()
{
    //
    while (system->is_running()) {
        //
        cpu->tick();
        //
        gpu->tick();
    }

    // Flush so we can see progress on cluster log files.
    fflush(stdout);
}

} // end namespace gltracesim

int
main(int argc, char *argv[])
{
    //
    std::string outdir(argv[1]);
    //
    gltracesim::GlTraceSimAnalyzer analyzer(outdir);
    //
    analyzer.run();
    //
    return EXIT_SUCCESS;
}

/* ===================================================================== */
