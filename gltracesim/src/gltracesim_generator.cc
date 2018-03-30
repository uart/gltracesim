#include "debug.hh"
#include "debug_impl.hh"
#include "gltracesim_generator.hh"
#include "util/addr_range.hh"
#include "util/addr_range_map.hh"
#include "util/cflags.hh"
#include "util/mkdir.hh"
#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"
#include "gem5/trace.hh"
#include "opengl.pb.h"
#include "resource_impl.hh"
#include "resource_tracker_impl.hh"
#include "generator/pipeline/pipeline.hh"

#include <sys/stat.h>

#include "gltracesim.pb.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

namespace gltracesim {

//
typedef std::shared_ptr<GlTraceSim> GlTraceSimPtr;

//
GlTraceSimPtr simulator;

static VOID _analysis_thread_work_loop_wrapper(VOID *_aid) {
    gltracesim::simulator->analysis_thread_work_loop(uint64_t(_aid));
}
static VOID _filter_thread_work_loop_wrapper(VOID *_fid) {
    gltracesim::simulator->filter_thread_work_loop(uint64_t(_fid));
}

GlTraceSim::thread_state_t::thread_state_t() :
    line_buffer(0), line_buffer_dirty(false), job(NULL)
{
    // Do nothing
}

GlTraceSim::mem_inst_t::mem_inst_t() :
    addr(0), rsc_misses(0), filter(false)
{
    // Do nothing
}

GlTraceSim::stats_t::stats_t()
{
    for (size_t i = 0; i < dev::NumHardwareDevices; ++i)
    {
        no_tot_mops[i] = 0;
        no_rsc_mops[i] = 0;
        no_rsc_offcore_mops[i] = 0;
    }
    no_opengl_calls = 0;
}

GlTraceSim::GlTraceSim(const std::string &output_dir)
{
    // Open Config File
    std::ifstream config_file(output_dir + "/config.json");

    // Load JSON config
    config_file >> config;

    //
    config["output-dir"] = output_dir;

    //
    Debug::init(config["debug"]);

    //
    system = SystemPtr(new System(config));

    //
    system->set_output_dir(output_dir);

    // System blk size
    system->set_blk_size(
        config["filter-cache"].get("blk-size", 64).asInt()
    );
    //
    system->rt = ResourceTrackerPtr(
        new ResourceTracker()
    );
    //
    system->vmem_manager = VirtualMemoryManagerPtr(
        new VirtualMemoryManager(config["virtual-memory"])
    );

    // Set fast forward frames
    sim_ctrl.start = config.get("start-frame", 0).asInt();
    // Set stop
    sim_ctrl.stop = config.get("stop-frame", INT_MAX).asInt();

    //
    auto_prune_mem_insts = config.get("auto-prune-mem-insts", false).asBool();
    //
    has_new_code = false;

    DPRINTF(Init, "Simulating frames (%i-%i).\n",
        sim_ctrl.start, sim_ctrl.stop
    );

    //
    ProtoMessage::PacketHeader hdr;
    hdr.set_obj_id("gltracesim");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    //
    pb.resources = new ProtoOutputStream(
        output_dir + "/resources.pb.gz"
    );
    pb.resources->write(hdr);

    //
    pb.frames = new ProtoOutputStream(
        output_dir + "/frames.pb.gz"
    );
    pb.frames->write(hdr);

    //
    pb.scenes = new ProtoOutputStream(
        output_dir + "/scenes.pb.gz"
    );
    pb.scenes->write(hdr);

    //
    pb.jobs = new ProtoOutputStream(
        output_dir + "/jobs.pb.gz"
    );
    pb.jobs->write(hdr);

    //
    pb.job_stats = new ProtoOutputStream(
        output_dir + "/jobs.stats.pb.gz"
    );
    pb.job_stats->write(hdr);

    //
    pb.opengl = new ProtoOutputStream(
        output_dir + "/opengl.pb.gz"
    );
    pb.opengl->write(hdr);

    //
    pb.sim_stats = new ProtoOutputStream(
        output_dir + "/stats.pb.gz"
    );
    pb.sim_stats->write(hdr);

    //
    pipe = new pipeline::Pipeline();

    if (config["stop-time"].asInt()) {
        stop_timer = new StopTimer(config["stop-time"].asInt());
    } else {
        stop_timer = NULL;
    }

    //
    if (config.get("dump-trace", false).asBool()) {
        //
        config["output-cpu-file"] =
            config["output-dir"].asString() + "/cpu.pb.gz";

        config["output-gpu-file"] =
            config["output-dir"].asString() + "/gpu.pb.gz";

        //
        AnalyzerPtr tracer = AnalyzerPtr(
            new gem5::CmdTraceGenerator(config, analyzers.size())
        );

        //
        assert(tracer);

        // Create work thread storage
        pipe->add_analyzer(tracer->get_id());

        //
        analyzers.push_back(tracer);
    }

    //
    mkdir("%s/f%u64/s%u64/",
        system->get_output_dir().c_str(),
        system->get_frame_nbr(),
        system->get_scene_nbr()
    );

    //
    current_frame = FramePtr(new Frame(
        system->get_frame_nbr()
    ));

    //
    handle_sync(0, NEW_FRAME);

    //
    current_scene = ScenePtr(new Scene(
        system->get_scene_nbr(),
        system->get_frame_nbr()
    ));
    //
    current_frame->add_scene(current_scene->id);

    //
    handle_sync(0, NEW_SCENE);

    // Flush so we can see progress on cluster log files.
    fflush(stdout);

    current_frame->start();
    current_scene->start();
}

GlTraceSim::~GlTraceSim()
{
    // Resources dead.
    if (config.get("dump-resources", false).asBool()) {
        for (auto &it: system->rt->get_alive()) {
            //
            GpuResourcePtr &gpu_resource = it.second;
            //
            gpu_resource->dump_jpeg_image();
        }
    }

    //
    delete pb.resources;
    //
    delete pb.frames;
    //
    delete pb.scenes;
    //
    delete pb.jobs;
    //
    delete pb.job_stats;
    //
    delete pb.opengl;
    //
    delete pb.sim_stats;
}


void
GlTraceSim::start()
{

    for (int i = 0; i < config.get("num-analysis-threads", 1).asInt(); ++i) {

        //
        int aid = pipe->add_analysis_thread();

        //
        unsigned tid = PIN_SpawnInternalThread(
            _analysis_thread_work_loop_wrapper, (void*) uint64_t(aid), 0, NULL
        );

        //
        if (tid == INVALID_THREADID) {
            fprintf(stderr, "Faild to create filter thread.");
            exit(EXIT_FAILURE);
        }

        //
        pipe->map_analysis_thread(aid, tid);

        //
        DPRINTF(Init, "Analysis Thread: +%i [aid: %i]\n", tid, aid);
    }

    pipe->analysis_queue->start();
}

void
GlTraceSim::pause_and_drain_buffers()
{
    // Drain Gpu and FilterIn buffer
    for (int fid = 0; fid < pipe->num_filter_threads(); ++fid) {
        pipe->filter_queue[fid]->wait(); // back buffer
        pipe->filter_queue[fid]->rotate_buffers();
        pipe->filter_queue[fid]->push();
        pipe->filter_queue[fid]->wait(); // front buffer
        pipe->filter_queue[fid]->rotate_buffers();

        flush_filter_cache(fid, dev::CPU);
        flush_filter_cache(fid, dev::GPU);
    }

    // Drain FilterOut and Analysis buffers
    pipe->analysis_queue->wait(); // back buffer
    pipe->analysis_queue->rotate_buffers();
    pipe->analysis_queue->start(); // front buffer
    pipe->analysis_queue->wait();
    pipe->analysis_queue->rotate_buffers();
}

void
GlTraceSim::resume()
{
    // Start work threads again
    for (int gid = 0; gid < pipe->num_gpu_threads(); ++gid) {
        pipe->filter_queue[gid]->push();
    }
    pipe->analysis_queue->start();
}

void
GlTraceSim::process_buffer(int gid)
{
    // Wait on filter and analasys stage
    pipe->filter_queue[gid]->wait();

    // Swap buffers
    pipe->filter_queue[gid]->rotate_buffers();

    // Resume filter and analasys stage
    pipe->filter_queue[gid]->push();
}

void
GlTraceSim::handle_sync(int gid, int cmd)
{
    pause_and_drain_buffers();

    // Record
    packet_t apkt;
    apkt.dev_id = dev::CPU;
    apkt.tid = gid;
    apkt.cmd = cmd;
    apkt.length = 0;

    //
    pipe->analysis_queue->push(apkt);

    resume();
}

void
GlTraceSim::handle_new_job(int gid, GpuJobPtr job)
{
    //
    assert(job);
    assert(ts[gid].job == NULL);

    //
    ts[gid].job = job;

    // Record
    auto &pkt = pipe->filter_queue[gid]->gpu->data[
        pipe->filter_queue[gid]->gpu->pos
    ];

    //
    pkt.cmd = NEW_JOB;
    pkt.vaddr = 0x0;
    pkt.paddr = 0x0;
    pkt.length = 0;
    pkt.job_id = job->id;
    pkt.dev_id = job->dev;
    pkt.rsc_id = 0;

    //
    pipe->filter_queue[gid]->gpu->pos++;

    //
    rw_mtx.lock();
    //
    gltracesim::proto::JobInfo job_info;
    //
    job->dump_info(&job_info);
    //
    pb.jobs->write(job_info);
    //
    rw_mtx.unlock();

    // Check buffer
    if (_l(pipe->filter_queue[gid]->gpu->pos <
           pipe->filter_queue[gid]->gpu->data.size()))
    {
        return;
    }

    //
    process_buffer(gid);

}

void
GlTraceSim::handle_end_job(int gid)
{
    //
    assert(ts[gid].job);

    // Record
    auto &pkt = pipe->filter_queue[gid]->gpu->data[
        pipe->filter_queue[gid]->gpu->pos
    ];

    //
    pkt.cmd = END_JOB;
    pkt.vaddr = 0x0;
    pkt.paddr = 0x0;
    pkt.length = 0;
    pkt.job_id = ts[gid].job->id;
    pkt.dev_id = ts[gid].job->dev;
    pkt.rsc_id = 0;

    //
    pipe->filter_queue[gid]->gpu->pos++;

    // Drain buffers
    pipe->filter_queue[gid]->wait(); // back buffer
    pipe->filter_queue[gid]->rotate_buffers();
    pipe->filter_queue[gid]->push();
    pipe->filter_queue[gid]->wait(); // front buffer
    pipe->filter_queue[gid]->rotate_buffers();

    flush_filter_cache(gid, dev::CPU);
    flush_filter_cache(gid, dev::GPU);

    pipe->filter_queue[gid]->push();

    //
    rw_mtx.lock();
    //
    gltracesim::proto::JobStats job_stats;
    //
    ts[gid].job->dump_stats(&job_stats);
    //
    pb.job_stats->write(job_stats);
    // Terminate job
    ts[gid].job = NULL;

    //
    rw_mtx.unlock();
}

inline void
GlTraceSim::handle_bbl(int tid, GpuJob::basic_blk_t* basic_blk)
{
    // In FF mode
    if (_u(sim_ctrl.start)) {
        return;
    }
    //
    assert(basic_blk);
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);
    //
    thread_state_t &thread = ts[gid];
    //
    assert(thread.job);
    //
    thread.job->stats.basic_blk_count[basic_blk]++;
}

inline void
GlTraceSim::handle_mem_access(
    int tid, GlTraceSim::mem_inst_t *mem_inst,
    uint64_t vaddr, uint8_t len, uint8_t cmd)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);
    //
    thread_state_t &thread = ts[gid];
    //
    assert(thread.job);

    //
    stats[gid].no_tot_mops[thread.job->dev]++;

    // In FF mode
    if (_u(sim_ctrl.start)) {
        return;
    }

    //
    #define BLK_ADDR_UMARK ~63UL
//    #define BLK_ADDR_UMARK ~31UL

    // Check line buffer
    if ((_l(thread.line_buffer == (vaddr & BLK_ADDR_UMARK))) &&
        (_l(thread.line_buffer == ((vaddr + len - 1) & BLK_ADDR_UMARK)))) {

        if (_l((cmd == READ  && thread.line_buffer_dirty == false) ||
               (cmd == WRITE && thread.line_buffer_dirty))) {
            return;
        }
    }

    //
    thread.line_buffer = (vaddr & BLK_ADDR_UMARK);
    thread.line_buffer_dirty = (cmd == WRITE);

    thread.job->stats.reads += (cmd == READ);
    thread.job->stats.writes += (cmd == WRITE);

    // Approximate TSC
    system->inc_tsc();

    // Buffer the analysis
    auto &pkt = pipe->filter_queue[gid]->gpu->data[
        pipe->filter_queue[gid]->gpu->pos
    ];

    //
    pkt.cmd = cmd;
    pkt.inst = mem_inst;
    pkt.vaddr = vaddr;
    pkt.length = len;
    pkt.job_id = thread.job->id;
    pkt.dev_id = thread.job->dev;

    //
    pipe->filter_queue[gid]->gpu->pos++;

    // Check buffer
    if (_l(pipe->filter_queue[gid]->gpu->pos <
           pipe->filter_queue[gid]->gpu->data.size()))
    {
        return;
    }

    //
    process_buffer(gid);

}

void
GlTraceSim::handle_opengl_call(int tid, uint32_t call_id)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    rw_mtx.lock();
    //
    DPRINTF(OpenGL, "OpenGL: %id [tid: %i, gid: %i].\n",
            call_id, tid, gid
    );
    //
    stats[gid].no_opengl_calls++;
    //
    current_scene->add_opengl_call(call_id);

    // Record
    packet_t pkt;
    pkt.cmd = OPENGL_CALL;
    pkt.tid = gid;
    pkt.paddr = call_id;
    pkt.length = 0;
    pkt.rsc_id = 0;
    pkt.dev_id = dev::CPU;
    //
    pipe->analysis_queue->push(pkt);

    //
    rw_mtx.unlock();
}

void
GlTraceSim::handle_gpu_resource_create(int tid, struct llvmpipe_resource *lpr)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    pause_and_drain_buffers();

    //
    rw_mtx.lock();

    //
    GpuResourcePtr gpu_resource(new GpuResource(lpr, dev::CPU));

    //
    DPRINTF(GpuResourceEvent,
        "Resource: +%lu [tid: %i, gid: %i, addr: 0x%x, size: %fMB, type: %s].\n",
        gpu_resource->id,
        tid, pipe->get_gid(tid),
        gpu_resource->addr_range.start,
        gpu_resource->addr_range.size() / 1048576.0,
        gpu_resource->get_target_name()
    );

    //
    system->rt->add(gpu_resource);
    //
    system->vmem_manager->alloc(gpu_resource->addr_range);

    //
    gltracesim::proto::ResourceInfo resource_info;

    //
    gpu_resource->dump_info(&resource_info);

    //
    pb.resources->write(resource_info);

    // Record
    packet_t pkt;
    pkt.cmd = NEW_RESOURCE;
    pkt.tid = gid;
    pkt.length = 0;
    pkt.rsc_id = gpu_resource->id;
    pkt.dev_id = dev::CPU;

    //
    pipe->analysis_queue->push(pkt);

    //
    rw_mtx.unlock();

    //
    resume();
}

void
GlTraceSim::handle_gpu_resource_destroy(int tid, struct llvmpipe_resource *lpr)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    pause_and_drain_buffers();

    //
    rw_mtx.lock();

    //
    uint64_t addr = GpuResource::get_addr(lpr);
    //
    GpuResourcePtr gpu_resource = system->rt->find_addr(addr);

    //
    if (gpu_resource) {

        //
        DPRINTF(GpuResourceEvent, "Resource: -%lu [tid: %i, gid: %i, addr: 0x%x].\n",
            gpu_resource->id,
            tid, pipe->get_gid(tid),
            gpu_resource->addr_range.start
        );

        if (config.get("dump-resources", false).asBool()) {
            gpu_resource->dump_jpeg_image();
        }

        // Check line buffer
        for (auto &thread: ts) {
            if (thread.last_gpu_resource == gpu_resource) {
                thread.last_gpu_resource = NULL;
            }
        }

        // Move from dead map to dead vector
        system->rt->destroy(gpu_resource);
        //
        system->vmem_manager->free(gpu_resource->addr_range);

        // Record
        packet_t pkt;
        pkt.cmd = END_RESOURCE;
        pkt.length = 0;
        pkt.tid = gid;
        pkt.rsc_id = gpu_resource->id;
        pkt.dev_id = dev::CPU;

        //
        pipe->analysis_queue->push(pkt);

    } else {
        printf("handle_gpu_resource_destroy error %p.\n", (void*)addr);
    }

    //
    rw_mtx.unlock();

    //
    resume();
}


void
GlTraceSim::handle_gpu_draw_vbo_begin(int tid)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    drain_buffers();

    //
    DPRINTF(GpuDrawVboEvent, "Draw: +[tid: %i, gid: %i, state: %s->GPU].\n",
            tid, gid, dev::get_dev_name(ts[gid].job->dev));

    //
    handle_end_job(gid);

    //
    rw_mtx.lock();
    //
    GpuJobPtr new_job = GpuJobPtr(new GpuDrawJob(
        system->get_job_nbr(),
        system->get_scene_nbr(),
        system->get_frame_nbr()
    ));
    //
    new_job->configure_trace_generator();
    //
    system->inc_job_nbr();
    //
    current_scene->add_job(new_job->id);
    //
    rw_mtx.unlock();

    //
    handle_new_job(gid, new_job);
}

void
GlTraceSim::handle_gpu_draw_vbo_end(int tid)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    drain_buffers();

    //
    DPRINTF(GpuDrawVboEvent, "Draw: -[tid: %i, gid: %i, state: %s->CPU].\n",
            tid, gid, dev::get_dev_name(ts[gid].job->dev));

    //
    handle_end_job(gid);

    //
    rw_mtx.lock();
    //
    GpuJobPtr new_job = GpuJobPtr(new GpuMiscJob(
        system->get_job_nbr(),
        system->get_scene_nbr(),
        system->get_frame_nbr(),
        dev::CPU
    ));
    //
    new_job->configure_trace_generator();
    //
    system->inc_job_nbr();
    //
    current_scene->add_job(new_job->id);
    //
    rw_mtx.unlock();

    //
    handle_new_job(gid, new_job);
}

void
GlTraceSim::handle_gpu_flush(int tid, const char *reason)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);
    //
    DPRINTF(GpuFlushEvent, "Flush: [reason: %s, tid: %i, gid: %i].\n",
            reason, tid, gid);

    //
    handle_sync(tid, SYNC);
}

void
GlTraceSim::handle_gpu_scene_begin(int tid)
{
    //
    DPRINTF(GpuSceneEvent, "Scene: +%lu [tid: %i, gid: %i].\n",
        system->get_scene_nbr(),
        tid, pipe->get_gid(tid)
    );
}

void
GlTraceSim::handle_gpu_scene_end(int tid)
{
    //
    current_scene->stop();

    //
    DPRINTF(GpuSceneEvent, "Scene: -%lu [tid: %i, gid: %i].\n",
        system->get_scene_nbr(),
        tid, pipe->get_gid(tid)
    );
    //
    handle_sync(tid, END_SCENE);

    // Resources dead.
    for (auto &it: system->rt->get_alive()) {
        //
        GpuResourcePtr &gpu_resource = it.second;
        //
        if (gpu_resource->scene_state.written) {
            //
            if (config.get("dump-scene-targets", false).asBool()) {
                gpu_resource->dump_png_image();
            }
        }
        //
        gpu_resource->scene_state.reset();
    }

    //
    gltracesim::proto::SceneInfo scene_info;
    //
    current_scene->dump_info(&scene_info);
    //
    pb.scenes->write(scene_info);

    //
    system->inc_scene_nbr();
    //
    current_scene = ScenePtr(new Scene(
        system->get_scene_nbr(),
        system->get_frame_nbr()
    ));
    //
    current_frame->add_scene(current_scene->id);

    //
    handle_sync(tid, NEW_SCENE);

    //
    mkdir("%s/f%u64/s%u64/",
        system->get_output_dir().c_str(),
        system->get_frame_nbr(),
        system->get_scene_nbr()
    );

    //
    current_scene->start();
}

void
GlTraceSim::handle_gpu_tile_begin(int tid, int x, int y)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    assert(ts[gid].job);

    //
    DPRINTF(GpuTileEvent,
        "Tile: +[tid: %i, gid: %i, x:%i, y:%i, state: %s->GPU].\n",
        tid, gid, x, y, dev::get_dev_name(ts[gid].job->dev)
    );

    //
    handle_end_job(gid);

    //
    rw_mtx.lock();
    //
    GpuJobPtr new_job = GpuJobPtr(new GpuTileJob(
        system->get_job_nbr(),
        system->get_scene_nbr(),
        system->get_frame_nbr(),
        x, y
    ));
    //
    new_job->configure_trace_generator();
    //
    system->inc_job_nbr();
    //
    current_scene->add_job(new_job->id);
    current_scene->inc_width(x + 1);
    current_scene->inc_height(y + 1);
    //
    rw_mtx.unlock();

    //
    handle_new_job(gid, new_job);
}

void
GlTraceSim::handle_gpu_tile_end(int tid)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    assert(ts[gid].job);

    //
    DPRINTF(GpuTileEvent,
        "Tile: -[tid: %i, gid: %i, state: %s->GPU].\n",
        tid, gid, dev::get_dev_name(ts[gid].job->dev)
    );

    //
    handle_end_job(gid);

    //
    rw_mtx.lock();
    //
    GpuJobPtr new_job = GpuJobPtr(new GpuMiscJob(
        system->get_job_nbr(),
        system->get_scene_nbr(),
        system->get_frame_nbr(),
        dev::CPU
    ));
    //
    new_job->configure_trace_generator();
    //
    system->inc_job_nbr();
    //
    current_scene->add_job(new_job->id);
    //
    rw_mtx.unlock();

    //
    handle_new_job(gid, new_job);
}

void
GlTraceSim::handle_gpu_swap_buffers(int tid)
{
    // TID -> GPU_TID
    int gid = pipe->get_gid(tid);

    //
    pause_and_drain_buffers();
    //
    rw_mtx.lock();

    //
    gltracesim::proto::Frame frame;

    //
    current_frame->stop();
    current_scene->stop();

    //
    frame.set_id(current_frame->id);
    frame.set_fast_forwarded(sim_ctrl.start > 0);

    //
    frame.mutable_sim_stats()->set_duration(current_frame->duration());

    // Time scaling factor
    double tsf = 1.0 / (current_frame->duration() * 1048576);

    size_t tot_no_tot_mops[] = { 0, 0 };
    size_t tot_no_rsc_mops[] = { 0, 0 };
    size_t tot_no_rsc_offcore_mops[] = { 0, 0 };
    size_t tot_no_opengl_calls = 0;

    for (int gid = 0; gid < pipe->num_gpu_threads(); ++gid)
    {
        tot_no_opengl_calls += stats[gid].no_opengl_calls;
    }

    DPRINTF(GpuFrameEvent,
        "frame: %lu [tid: %i, gid: %i, duration: %fs, opengl: %lu, passes: %lu]\n",
        current_frame->id, tid, pipe->get_gid(tid), current_frame->duration(),
        tot_no_opengl_calls, system->get_scene_nbr()
    );

    for (size_t d = 0; d < dev::NumHardwareDevices; ++d)
    {       
        for (int gid = 0; gid < pipe->num_gpu_threads(); ++gid)
        {
            tot_no_tot_mops[d] += stats[gid].no_tot_mops[d];
            tot_no_rsc_mops[d] += stats[gid].no_rsc_mops[d];
            tot_no_rsc_offcore_mops[d] += stats[gid].no_rsc_offcore_mops[d];
        }

        //
        DPRINTF(GpuFrameEvent,
            "  dev: %s [mops: %lu, %.2fM/s, rsc-mops: %lu, %.2fM/s,"
            " offcore mops: %lu, %.2fM/s]\n",
            dev::get_dev_name(d),
            tot_no_tot_mops[d], tot_no_tot_mops[d] * tsf,
            tot_no_rsc_mops[d], tot_no_rsc_mops[d] * tsf,
            tot_no_rsc_offcore_mops[d], tot_no_rsc_offcore_mops[d] * tsf
        );

        for (int gid = 0; gid < pipe->num_gpu_threads(); ++gid)
        {
            //
            DPRINTF(GpuFrameEvent,
                "    tid: %u [mops: %lu, %.2fM/s, rsc-mops: %lu, %.2fM/s,"
                " offcore mops: %lu, %.2fM/s]\n",
                gid,
                stats[gid].no_tot_mops[d], stats[gid].no_tot_mops[d] * tsf,
                stats[gid].no_rsc_mops[d], stats[gid].no_rsc_mops[d] * tsf,
                stats[gid].no_rsc_offcore_mops[d], stats[gid].no_rsc_offcore_mops[d] * tsf
            );
        }
    }

    frame.mutable_sim_stats()->set_tot_mops(
        tot_no_tot_mops[dev::CPU] +
        tot_no_tot_mops[dev::GPU]
    );
    frame.mutable_sim_stats()->set_dev_mops(
        tot_no_rsc_mops[dev::CPU] +
        tot_no_rsc_mops[dev::GPU]
    );
    frame.mutable_sim_stats()->set_offcore_mops(
        tot_no_rsc_offcore_mops[dev::CPU] +
        tot_no_rsc_offcore_mops[dev::GPU]
    );
    frame.mutable_sim_stats()->set_opengl_calls(
        tot_no_opengl_calls
    );

    for (int gid = 0; gid < pipe->num_gpu_threads(); ++gid)
    {
        stats[gid] = stats_t();
    }

    // Flush so we can see progress on cluster log files.
    fflush(stdout);

    // Resources dead.
    for (auto &it: system->rt->get_alive()) {
        //
        GpuResourcePtr &gpu_resource = it.second;
        //
        gpu_resource->dump_stats(frame.add_resource_stats());
        //
        gpu_resource->reset_stats();
    }

    // Resources deallocated during the frame rendering.
    for (auto &gpu_resource: system->rt->get_dead()) {
        //
        gpu_resource->dump_stats(frame.add_resource_stats());
        //
        gpu_resource->reset_stats();
    }

    // Resources used after being dead.
    for (auto &gpu_resource: system->rt->get_zombies()) {
        //
        gpu_resource->dump_stats(frame.add_resource_stats());
        //
        gpu_resource->reset_stats();
    }

    //
    system->rt->clear_dead();
    system->rt->clear_zombies();

    // Record
    packet_t nf_pkt;
    nf_pkt.cmd = NEW_FRAME;
    nf_pkt.tid = gid;
    nf_pkt.length = 0;
    nf_pkt.dev_id = dev::CPU;

    packet_t ns_pkt;
    ns_pkt.cmd = NEW_SCENE;
    ns_pkt.tid = gid;
    ns_pkt.length = 0;
    ns_pkt.dev_id = dev::CPU;

    //
    for (auto &analyzer: analyzers) {
        //
        analyzer->dump_stats();
        //
        analyzer->reset_stats();
        //
        analyzer->process(nf_pkt);
        //
        analyzer->process(ns_pkt);
    }

    //
    pb.sim_stats->write(frame);

    //
    gltracesim::proto::FrameInfo frame_info;
    //
    current_frame->dump_info(&frame_info);
    //
    pb.frames->write(frame_info);

    // Inc frame count
    system->inc_frame_nbr();
    //
    current_frame = FramePtr(new Frame(
        system->get_frame_nbr()
    ));

    // Resources .
    for (auto &it: system->rt->get_alive()) {
        //
        GpuResourcePtr &gpu_resource = it.second;
        //
        if (gpu_resource->scene_state.written) {
            //
            if (config.get("dump-scene-targets", false).asBool()) {
                gpu_resource->dump_png_image();
            }
        }
        //
        gpu_resource->scene_state.reset();
    }

    //
    gltracesim::proto::SceneInfo scene_info;
    //
    current_scene->dump_info(&scene_info);
    //
    pb.scenes->write(scene_info);

    // Reset intra frame markers
    system->set_scene_nbr(0);
    //
    current_scene = ScenePtr(new Scene(
        system->get_scene_nbr(),
        system->get_frame_nbr()
    ));
    //
    current_frame->add_scene(current_scene->id);

    //
    sim_ctrl.start = std::max(0, sim_ctrl.start - 1);

    // All done, exit
    if (sim_ctrl.stop == 0) {
        //
        DPRINTF(GpuFrameEvent, "%i: All done, exiting...\n", tid);
        //
        rw_mtx.unlock();
        // Exit
        PIN_ExitApplication(EXIT_SUCCESS);
        //
        exit(EXIT_SUCCESS);
    }

    //
    --sim_ctrl.stop;

    //
    if (auto_prune_mem_insts) {
        //
        size_t auto_prune_mem_insts_threshold =
            config.get(
                "auto-prune-mem-insts-threshold",
                1024
            ).asUInt64();

        //
        for (auto &mem_inst: mem_inst_filter) {

            //
            if (has_new_code) {
                mem_inst->filter = false;
                mem_inst->rsc_misses = 0;
                continue;
            }

            //
            if (mem_inst->filter) {
                continue;
            }
            //
            if (mem_inst->accesses_gpu_resource) {
                //
                continue;
            }

            //
            if (mem_inst->rsc_misses > auto_prune_mem_insts_threshold) {
                mem_inst->filter = true;

                //
                DPRINTF(AutoPrune,
                    "prune %p [tid: %i, misses: %lu].\n",
                    mem_inst->addr, tid, mem_inst->rsc_misses
                );
            }
        }

        has_new_code = false;
    }

    //
    mkdir("%s/f%u64/",
        system->get_output_dir().c_str(),
        system->get_frame_nbr()
    );

    //
    current_frame->start();
    current_scene->start();

    // Start work threads again
    resume();

    //
    rw_mtx.unlock();
}

inline void
GlTraceSim::handle_gpu_thread_start(int tid)
{
    //
    rw_mtx.lock();

    //
    int gid = pipe->add_gpu_thread();
    //
    int fid = pipe->add_filter_thread();

    //
    DPRINTF(GpuThreadEvent, "GPU Thread: +%i [gid: %i]\n", tid, gid);

    //
    pipe->map_gpu_thread(gid, tid);

    //
    stats.push_back(stats_t());

    // Create filter thread
    {
        //
        Json::Value &fcc = config["filter-cache"];

        //
        FilterCache::params_t params;

        params.filter_cache = true;
        params.size = fcc["size"].asInt();
        params.associativity = fcc.get("associativity", 8).asInt();
        params.blk_size = fcc.get("blk-size", 64).asInt();
        params.sub_blk_size = params.blk_size;
        params.fetch_on_wr_miss = fcc.get("fetch-on-wr-miss", true).asBool();

        //
        ts[fid].cache[dev::CPU] = FilterCachePtr(new FilterCache(&params));
        ts[fid].cache[dev::GPU] = FilterCachePtr(new FilterCache(&params));
    }

    // Spawn new filter work thread
    int filter_tid = PIN_SpawnInternalThread(
        _filter_thread_work_loop_wrapper, (void*) uint64_t(fid), 0, NULL
    );

    //
    if (filter_tid == int(INVALID_THREADID)) {
        exit(EXIT_FAILURE);
    }

    //
    DPRINTF(Init, "Filter Thread: +%i [fid: %i]\n", filter_tid, fid);

    //
    pipe->map_filter_thread(fid, filter_tid);

    //
    pipe->filter_queue[gid]->push();

    //
    GpuJobPtr new_job = GpuJobPtr(new GpuMiscJob(
        system->get_job_nbr(),
        system->get_scene_nbr(),
        system->get_frame_nbr(),
        dev::CPU
    ));
    //
    new_job->configure_trace_generator();
    //
    system->inc_job_nbr();
    //
    current_scene->add_job(new_job->id);

    //
    rw_mtx.unlock();

    //
    handle_new_job(gid, new_job);
}

// This routine is executed every time a thread is dead.
inline void
GlTraceSim::handle_gpu_thread_stop(int tid)
{
    //
    rw_mtx.lock();
    //
    DPRINTF(GpuThreadEvent, "GPU Thread -%i [gid: %i]\n", tid, pipe->get_gid(tid));
    //
    rw_mtx.unlock();
}


void
GlTraceSim::flush_filter_cache(int fid, int dev)
{
    // Get filter cache from device info
    FilterCachePtr &filter_cache = ts[fid].cache[dev];

    // Get raw data
    FilterCache::Base::data_t *data = filter_cache->get_data();

    for (size_t i = 0; i < data->size(); ++i)
    {
        FilterCache::entry_t *fce = &((*data)[i]);

        // Evict blk
        if (fce->valid && fce->dirty) {
            //
            GpuResourcePtr fce_gpu_resource =
                system->rt->find_id(fce->rsc_id);

            assert(fce_gpu_resource);
            assert(fce_gpu_resource->id == uint64_t(fce->rsc_id));

            // Record in buffer
            packet_t apkt;
            apkt.cmd = WRITE;
            apkt.vaddr = fce->addr;
            apkt.paddr = fce->paddr;
            apkt.length = filter_cache->params.blk_size;
            apkt.tid = fid;
            apkt.rsc_id = fce->rsc_id;
            apkt.job_id = fce->job_id;
            apkt.dev_id = dev;

            //
            ts[fid].job->trace->process(apkt);

            //
            stats[fid].no_rsc_offcore_mops[dev]++;

            // Calc block index into GPU resource
            uint64_t blk_idx = fce_gpu_resource->get_blk_idx(fce->addr);

            // Not thread safe, so approximate
            fce_gpu_resource->frame_stats.gpu_write_blks++;
            fce_gpu_resource->blk_state[blk_idx].gpu_write_touched = 1;

            // Dead but not yet a zombie, resurect it.
            if (fce_gpu_resource->dead && fce_gpu_resource->zombie == false) {
                system->rt->resurrect(fce_gpu_resource);
            }
        }

        fce->valid = false;
    }
}

void
GlTraceSim::process_filter_buffer_item(int fid, packet_t &pkt)
{
    // Check what to do
    if (_u(pkt.cmd != READ && pkt.cmd != WRITE))
    {
        // Handle special commands
        switch (pkt.cmd) {
            case END_JOB:
                // Remove dirty data
                flush_filter_cache(fid, pkt.dev_id);
            default:
                ;
        }

        // Pass through
        pipe->analysis_queue->push(pkt);
        // Done
        return;
    }

    //
    GpuResourcePtr &gpu_resource = ts[fid].last_gpu_resource;

    //
    mem_inst_t *mem_inst = (mem_inst_t *) pkt.inst;
    //
    assert(mem_inst);

    // Check resource line buffer
    if (_l(gpu_resource && gpu_resource->addr_range.contains(pkt.vaddr))) {

    } else {

        // Check if GPU resource
        GpuResourcePtr t_resource = system->rt->find_addr(pkt.vaddr);

        // Not a texture, skip
        if (t_resource == NULL) {
            //
            mem_inst->rsc_misses++;

            //
            return;
        }

        // Address is to a GPU resource
        gpu_resource = t_resource;

        // Record
        ts[fid].last_gpu_resource = gpu_resource;
    }

    //
    mem_inst->accesses_gpu_resource = true;

    //
    stats[fid].no_rsc_mops[pkt.dev_id]++;

    // Check if we need to move resoruce
    if (_u(gpu_resource->dev != pkt.dev_id)) {
        //
        DPRINTF(GpuResourceEvent,
            "Resource: %lu [fid: %i, mv: %s->%s].\n",
            gpu_resource->id, fid,
            dev::get_dev_name(gpu_resource->dev),
            dev::get_dev_name(pkt.dev_id)
        );

        // Record move
        packet_t apkt;
        apkt.cmd = (pkt.dev_id == dev::CPU) ? MV_TO_CPU : MV_TO_GPU;
        apkt.tid = fid;
        apkt.rsc_id = gpu_resource->id;
        apkt.length = gpu_resource->size();
        apkt.dev_id = gpu_resource->dev;

        //
        pipe->analysis_queue->push(apkt);

        // Move resources
        gpu_resource->dev = pkt.dev_id;
    }

    //
    gpu_resource->frame_stats.used = true;

    // Get filter cache from device info
    FilterCachePtr &filter_cache = ts[fid].cache[pkt.dev_id];

    //
    filter_cache->tick++;

    //
    FilterCache::entry_t *fce, *fcre;
    //
    filter_cache->find(pkt.vaddr, fce, fcre);

    // Calc block index into GPU resource
    uint64_t blk_idx = gpu_resource->get_blk_idx(pkt.vaddr);

    if (_l(pkt.cmd == READ)) {
        gpu_resource->scene_state.read = true;
        gpu_resource->frame_stats.gpu_core_read_blks++;
        gpu_resource->blk_state[blk_idx].gpu_core_read_touched = 1;
    } else {
        gpu_resource->scene_state.written = true;
        gpu_resource->frame_stats.gpu_core_write_blks++;
        gpu_resource->blk_state[blk_idx].gpu_core_write_touched = 1;
    }

    // Sample mipmap level.
    gpu_resource->frame_stats.mipmap_utilization.sample(
        gpu_resource->get_mipmap_level(pkt.vaddr)
    );

    if (_l(fce)) {
        assert(fce->valid);

        // Set LRU
        fce->last_tsc = filter_cache->tick;
        // Update state
        fce->dirty |= (pkt.cmd == WRITE);

        // Hit, nothing else to do.
        return;
    }

    // Miss

    // Evict blk
    if (_u(fcre->valid && fcre->dirty)) {
        //
        GpuResourcePtr fcre_gpu_resource =
            system->rt->find_id(fcre->rsc_id);

        assert(fcre_gpu_resource);
        assert(fcre_gpu_resource->id == uint64_t(fcre->rsc_id));

        // Record in buffer
        packet_t apkt;
        apkt.dev_id = pkt.dev_id;
        apkt.vaddr = fcre->addr;
        apkt.paddr = fcre->paddr;
        apkt.tid = fid;
        apkt.cmd = WRITE;
        apkt.rsc_id = fcre->rsc_id;
        apkt.job_id = fcre->job_id;
        apkt.length = filter_cache->params.blk_size;

        //
        ts[fid].job->trace->process(apkt);

        //
        stats[fid].no_rsc_offcore_mops[pkt.dev_id]++;

        // Calc block index into GPU resource
        uint64_t blk_idx = fcre_gpu_resource->get_blk_idx(fcre->addr);

        // Not thread safe, so approximate
        fcre_gpu_resource->frame_stats.gpu_write_blks++;
        fcre_gpu_resource->blk_state[blk_idx].gpu_write_touched = 1;

        // Dead but not yet a zombie, resurect it.
        if (fcre_gpu_resource->dead && fcre_gpu_resource->zombie == false) {
            system->rt->resurrect(fcre_gpu_resource);
        }
    }

    // Insert blk
    fcre->valid = true;
    fcre->dirty = (pkt.cmd == WRITE);
    fcre->addr = pkt.vaddr;
    fcre->paddr = system->vmem_manager->translate(pkt.vaddr);
    fcre->last_tsc = filter_cache->tick;
    fcre->rsc_id = gpu_resource->id;
    fcre->job_id = pkt.job_id;

    // Assume we write whole cacheline
    if (_u(pkt.cmd == WRITE &&
           filter_cache->params.fetch_on_wr_miss == false)) {
        // Do nothing, only install, no fetch
    } else {
        // Miss, record in buffer
        packet_t apkt;
        apkt.cmd = READ;
        apkt.vaddr = pkt.vaddr;
        apkt.paddr = fcre->paddr;
        apkt.length = filter_cache->params.blk_size;
        apkt.tid = fid;
        apkt.rsc_id = gpu_resource->id;
        apkt.job_id = pkt.job_id;
        apkt.dev_id = pkt.dev_id;

        //
        ts[fid].job->trace->process(apkt);

        //
        stats[fid].no_rsc_offcore_mops[pkt.dev_id]++;

        // Not thread safe, so approximate
        gpu_resource->frame_stats.gpu_read_blks++;
        gpu_resource->blk_state[blk_idx].gpu_read_touched = 1;
    }
}

void
GlTraceSim::process_filter_buffer(int fid, buffer_t *in_buffer)
{
    // Get CPU cache to set params
    FilterCachePtr &filter_cache = ts[fid].cache[dev::CPU];

    // Do work
    for (size_t i = 0; i < in_buffer->pos; ++i) {

        //
        packet_t &pkt = in_buffer->data[i];

        // Cache align
        pkt.vaddr = filter_cache->get_blk_addr(pkt.vaddr);

        //
        uint64_t end_addr = pkt.vaddr + pkt.length;

        //
        pkt.length = filter_cache->params.blk_size;

        //
        while (true) {
            //
            process_filter_buffer_item(fid, pkt);

            //
            pkt.vaddr += pkt.length;

            //
            if (_u(pkt.vaddr >= end_addr)) {
                break;
            }
        }
    }

    // Reset buffer
    in_buffer->pos = 0;
}

void
GlTraceSim::filter_thread_work_loop(int fid)
{

    // Drain buffers
    while (true) {

        //
        if (PIN_IsProcessExiting()) {
            //
            DPRINTF(Init, "Filter Thread: -%i [fid: %i]\n",
               pipe->get_gtid(fid), fid);
            //
            return;
        }

        // Timeout to see if we need to exit
        bool has_work = pipe->filter_queue[fid]->pop(1000);

        // Check PIN
        if (has_work == false) {
            continue;
        }

        // Need to lock for seaching GPU Resource Map.
        rw_mtx.rlock();

        //
        process_filter_buffer(fid, pipe->filter_queue[fid]->fin);

        //
        rw_mtx.unlock();

        // Signal done
        pipe->filter_queue[fid]->signal_producer();
    }
}

void
GlTraceSim::analysis_thread_work_loop(int aid)
{
    // Drain buffers
    while (true) {

        // Check if we should exit
        if (PIN_IsProcessExiting()) {
            //
            DPRINTF(Init, "Analysis Thread: -%i [aid: %i]\n",
                pipe->get_atid(aid), aid);
            //
            return;
        }

        // Get work from analysis queue
        int analyzer_id = pipe->analysis_queue->pop(1000, aid);

        // Timeout
        if (analyzer_id < 0) {
            continue;
        }

        for (size_t i = 0;
             i < pipe->analysis_queue->analysis_buffer->pos;
             ++i) {
            //
            analyzers[analyzer_id]->process(
                pipe->analysis_queue->analysis_buffer->data[i]
            );
        }

        // Signal GPU thread that we are done
        pipe->analysis_queue->signal_producer(aid, analyzer_id);
    }
}

void
GlTraceSim::register_mem_inst(mem_inst_t* mem_inst)
{
    //
    assert(mem_inst);
    //
    mem_inst_filter.push_back(mem_inst);
}

void
GlTraceSim::register_new_code(GpuJob::basic_blk_t* basic_blk)
{
    // We detected new code
    if (has_new_code == false) {
        //
        for (auto &mem_inst: mem_inst_filter) {
            mem_inst->filter = false;
            mem_inst->rsc_misses = 0;
        }
    }

    has_new_code = true;

    //
    basic_blks.push_back(basic_blk);
}

uint32_t
GlTraceSim::register_opengl_call(const std::string &call)
{
    assert(call.size());

    //
    uint32_t call_id = opengl_calls.size();

    //
    opengl_calls.push_back(call);
    //
    gltracesim::proto::OpenglCallInfo call_info;
    //
    call_info.set_id(call_id);
    call_info.set_name(call);
    //
    pb.opengl->write(call_info);

    // ID
    return call_id;
}

} // end namespace gltracesim


/* ===================================================================== */

static VOID
handle_mem_access(
    THREADID tid, gltracesim::GlTraceSim::mem_inst_t* mem_inst,
    ADDRINT addr, UINT32 len, bool write)
{
    // Auto prune
    if (_l(mem_inst->filter)) {
        return;
    }

    //
    gltracesim::simulator->handle_mem_access(tid, mem_inst, addr, len, write);
}

//
VOID
handle_pin_instruction(INS ins, VOID *v)
{
    //
    int num_mem_operands = INS_MemoryOperandCount(ins);

    // No memory ops
    if (num_mem_operands == 0) {
        return;
    }

    // Ignore all memory ops
    if (gltracesim::simulator->config.get("ignore-mem-ops", false).asBool())
    {
        return;
    }

    // Ignore stack ops
    if ((INS_IsStackRead(ins) || INS_IsStackWrite(ins)) &&
        (gltracesim::simulator->config.get("ignore-stack-ops", false).asBool()))
    {
        return;
    }

    //
    gltracesim::GlTraceSim::mem_inst_t* mem_inst =
        new gltracesim::GlTraceSim::mem_inst_t();

    //
    mem_inst->addr = (void*) INS_Address(ins);
    //
    gltracesim::simulator->register_mem_inst(mem_inst);

    // Instrument ops
    for (int mop_id = 0; mop_id < num_mem_operands; mop_id++)
    {
        if (INS_MemoryOperandIsRead(ins, mop_id))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR) handle_mem_access,
                IARG_THREAD_ID,
                IARG_PTR, AFUNPTR(mem_inst),
                IARG_MEMORYOP_EA, mop_id,
                IARG_MEMORYREAD_SIZE,
                IARG_BOOL, BOOL(false),
                IARG_END
            );
        }

        if (INS_MemoryOperandIsWritten(ins, mop_id))
        {
            INS_InsertPredicatedCall(
                ins, IPOINT_BEFORE,
                (AFUNPTR) handle_mem_access,
                IARG_THREAD_ID,
                IARG_PTR, AFUNPTR(mem_inst),
                IARG_MEMORYOP_EA, mop_id,
                IARG_MEMORYWRITE_SIZE,
                IARG_BOOL, BOOL(true),
                IARG_END
            );
        }
    }
}

static VOID
handle_opengl_call(THREADID tid, uint32_t id)
{
    gltracesim::simulator->handle_opengl_call(tid, id);
}

typedef void (*drawvbo_fcn_t)(void*, void*);

static VOID
handle_gpu_draw_vbo(THREADID tid, CONTEXT *ctx,
    drawvbo_fcn_t f, void* a0, void *a1)
{
    //
    gltracesim::simulator->handle_gpu_draw_vbo_begin(tid);
    //
    PIN_CallApplicationFunction(
        ctx,
        tid,
        CALLINGSTD_DEFAULT,
        AFUNPTR(f),
        NULL,
        PIN_PARG(void),
        PIN_PARG(void*), a0,
        PIN_PARG(void*), a1,
        PIN_PARG_END()
    );

    //
    gltracesim::simulator->handle_gpu_draw_vbo_end(tid);
}

typedef void (*flush_fcn_t)(void*, void*, const char*);

static VOID
handle_gpu_flush(THREADID tid, CONTEXT *ctx,
    flush_fcn_t f, void *a0, void *a1, const char *reason)
{
    //
    PIN_CallApplicationFunction(
        ctx,
        tid,
        CALLINGSTD_DEFAULT,
        AFUNPTR(f),
        NULL,
        PIN_PARG(void),
        PIN_PARG(void*), a0,
        PIN_PARG(void*), a1,
        PIN_PARG(const char*), reason,
        PIN_PARG_END()
    );

    f(a0, a1, reason);
    //
    gltracesim::simulator->handle_gpu_flush(tid, reason);
}

typedef struct pipe_resource* (*resource_create_fcn_t)(void*, void*);

static struct pipe_resource*
handle_gpu_resource_create(THREADID tid, CONTEXT *ctx,
    resource_create_fcn_t f, void* a0, void* a1)
{
    //
    struct pipe_resource *pt = NULL;
    //
    PIN_CallApplicationFunction(
        ctx,
        tid,
        CALLINGSTD_DEFAULT,
        AFUNPTR(f),
        NULL,
        PIN_PARG(void*), &pt,
        PIN_PARG(void*), a0,
        PIN_PARG(void*), a1,
        PIN_PARG_END()
    );
    //
    gltracesim::simulator->handle_gpu_resource_create(
        tid, llvmpipe_resource(pt)
    );
    //
    return pt;
}

typedef void (*resource_destroy_fcn_t)(void*, struct pipe_resource*);

static VOID
handle_gpu_resource_destroy(THREADID tid, CONTEXT *ctx,
    resource_destroy_fcn_t f, void* a0, struct pipe_resource* pt)
{
    //
    gltracesim::simulator->handle_gpu_resource_destroy(
        tid, llvmpipe_resource(pt)
    );

    //
    PIN_CallApplicationFunction(
        ctx,
        tid,
        CALLINGSTD_DEFAULT,
        AFUNPTR(f),
        NULL,
        PIN_PARG(void),
        PIN_PARG(void*), a0,
        PIN_PARG(void*), pt,
        PIN_PARG_END()
    );
}

static VOID
handle_gpu_scene_begin(THREADID tid)
{
    gltracesim::simulator->handle_gpu_scene_begin(tid);
}

static VOID
handle_gpu_scene_end(THREADID tid)
{
    gltracesim::simulator->handle_gpu_scene_end(tid);
}

static VOID
handle_gpu_tile_begin(THREADID tid, int x, int y)
{
    gltracesim::simulator->handle_gpu_tile_begin(tid, x, y);
}

static VOID
handle_gpu_tile_end(THREADID tid)
{
    gltracesim::simulator->handle_gpu_tile_end(tid);
}

typedef void (*swapbuffers_fcn_t)(void*, uint64_t);

static VOID
handle_gpu_swap_buffers(
    THREADID tid, CONTEXT *ctx, swapbuffers_fcn_t f, void* a0, uint64_t a1)
{
    // Call original
    PIN_CallApplicationFunction(
        ctx,
        tid,
        CALLINGSTD_DEFAULT,
        AFUNPTR(f),
        NULL,
        PIN_PARG(void),
        PIN_PARG(void*), a0,
        PIN_PARG(uint64_t), a1,
        PIN_PARG_END()
    );

    //
    gltracesim::simulator->handle_gpu_swap_buffers(tid);
}

static VOID
handle_pin_image(IMG img, VOID *v)
{
    RTN rtn;

    for (SEC sec = IMG_SecHead(img); sec != SEC_Invalid(); sec = SEC_Next(sec))
    {
        for (rtn = SEC_RtnHead(sec); rtn != RTN_Invalid(); rtn = RTN_Next(rtn))
        {
            std::string name = RTN_Name(rtn);

            if (strncmp(name.c_str(), "gl", 2) == 0)
            {
                if (name.size() > 3 && isupper(name[2]))
                {
                    printf("Instrumenting -> %s\n", name.c_str());

                    //
                    uint32_t id =
                        gltracesim::simulator->register_opengl_call(name);

                    //
                    RTN_Open(rtn);
                    RTN_InsertCall(
                        rtn, IPOINT_BEFORE,
                        (AFUNPTR) handle_opengl_call,
                        IARG_THREAD_ID,
                        IARG_UINT32, id,
                        IARG_END
                    );
                    RTN_Close(rtn);
                }
            }
        }
    }

    rtn = RTN_FindByName(img, "llvmpipe_draw_vbo");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "llvmpipe_draw_vbo")) {
        printf("Instrumenting -> lp_draw_vbo\n");

        //
        PROTO proto_llvmpipe_draw_vbo = PROTO_Allocate(
            PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "llvmpipe_draw_vbo",
            PIN_PARG(uint64_t*),
            PIN_PARG(uint64_t*),
            PIN_PARG_END()
        );

        //
        RTN_ReplaceSignature(
            rtn,
            AFUNPTR(handle_gpu_draw_vbo),
            IARG_PROTOTYPE, proto_llvmpipe_draw_vbo,
            IARG_THREAD_ID,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END
        );
    }

    rtn = RTN_FindByName(img, "llvmpipe_flush");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "llvmpipe_flush")) {
        printf("Instrumenting -> lp_flush\n");

        //
        PROTO proto_llvmpipe_flush = PROTO_Allocate(
            PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "llvmpipe_flush",
            PIN_PARG(uint64_t*),
            PIN_PARG(uint64_t),
            PIN_PARG(const char*),
            PIN_PARG_END()
        );

        //
        RTN_ReplaceSignature(
            rtn,
            AFUNPTR(handle_gpu_flush),
            IARG_PROTOTYPE, proto_llvmpipe_flush,
            IARG_THREAD_ID,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 2,
            IARG_END
        );
    }

    rtn = RTN_FindByName(img, "llvmpipe_resource_create");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "llvmpipe_resource_create")) {
        printf("Instrumenting -> lp_resource_create\n");

        //
        PROTO proto_llvmpipe_resource_create = PROTO_Allocate(
            PIN_PARG(struct pipe_resource*),
            CALLINGSTD_DEFAULT,
            "llvmpipe_resource_create",
            PIN_PARG(uint64_t*),
            PIN_PARG(uint64_t*),
            PIN_PARG_END()
        );

        //
        RTN_ReplaceSignature(
            rtn,
            AFUNPTR(handle_gpu_resource_create),
            IARG_PROTOTYPE, proto_llvmpipe_resource_create,
            IARG_THREAD_ID,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END
        );
    }

    rtn = RTN_FindByName(img, "llvmpipe_resource_destroy");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "llvmpipe_resource_destroy")) {
        printf("Instrumenting -> lp_resource_destroy\n");

        //
        PROTO proto_llvmpipe_resource_destroy = PROTO_Allocate(
            PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "llvmpipe_resource_destroy",
            PIN_PARG(uint64_t*),
            PIN_PARG(uint64_t*),
            PIN_PARG_END()
        );

        //
        RTN_ReplaceSignature(
            rtn,
            AFUNPTR(handle_gpu_resource_destroy),
            IARG_PROTOTYPE, proto_llvmpipe_resource_destroy,
            IARG_THREAD_ID,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END
        );
    }

    rtn = RTN_FindByName(img, "lp_rast_begin");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "lp_rast_begin")) {
        printf("Instrumenting -> lp_rast_begin\n");

        RTN_Open(rtn);
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR) handle_gpu_scene_begin,
            IARG_THREAD_ID,
            IARG_END
        );
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "lp_rast_end");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "lp_rast_end")) {
        printf("Instrumenting -> lp_rast_end\n");

        RTN_Open(rtn);
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR) handle_gpu_scene_end,
            IARG_THREAD_ID,
            IARG_END
        );
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "lp_rast_tile_begin");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "lp_rast_tile_begin")) {
        printf("Instrumenting -> lp_rast_tile_begin\n");

        RTN_Open(rtn);
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR) handle_gpu_tile_begin,
            IARG_THREAD_ID,
            IARG_G_ARG2_CALLEE,
            IARG_G_ARG3_CALLEE,
            IARG_END
        );
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "lp_rast_tile_end");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "lp_rast_tile_end")) {
        printf("Instrumenting -> lp_rast_tile_end\n");

        RTN_Open(rtn);
        RTN_InsertCall(
            rtn, IPOINT_BEFORE,
            (AFUNPTR) handle_gpu_tile_end,
            IARG_THREAD_ID,
            IARG_END
        );
        RTN_Close(rtn);
    }

    rtn = RTN_FindByName(img, "glXSwapBuffers");

    if (RTN_Valid(rtn) && (RTN_Name(rtn) == "glXSwapBuffers")) {
        printf("Instrumenting -> glXSwapBuffers\n");

        //
        PROTO proto_glXSwapBuffers = PROTO_Allocate(
            PIN_PARG(void),
            CALLINGSTD_DEFAULT,
            "glXSwapBuffers",
            PIN_PARG(uint64_t*),
            PIN_PARG(uint64_t),
            PIN_PARG_END()
        );

        //
        RTN_ReplaceSignature(
            rtn,
            AFUNPTR(handle_gpu_swap_buffers),
            IARG_PROTOTYPE, proto_glXSwapBuffers,
            IARG_THREAD_ID,
            IARG_CONTEXT,
            IARG_ORIG_FUNCPTR,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
            IARG_END
        );
    }
}

static void
handle_bbl(int tid, void* basic_blk)
{
    //
    gltracesim::simulator->handle_bbl(
        tid,
        (gltracesim::GpuJob::basic_blk_t*) basic_blk
    );
}

static VOID
handle_pin_trace(TRACE trace, VOID *v)
{
    //
    for (BBL bbl = TRACE_BblHead(trace); BBL_Valid(bbl); bbl = BBL_Next(bbl))
    {
        //
        gltracesim::GpuJob::basic_blk_t *basic_blk =
            new gltracesim::GpuJob::basic_blk_t();

        //
        for (INS ins = BBL_InsHead(bbl); INS_Valid(ins); ins = INS_Next(ins))
        {
            //
            gltracesim::GpuJob::inst_t inst;
            // XED_ICLASS_name
            inst.opcode = INS_Opcode(ins);
            //
            inst.width = 0;

            //
            int num_operands = INS_OperandCount(ins);

            // Instrument ops
            for (int op_id = 0; op_id < num_operands; op_id++)
            {
                if (INS_OperandWritten(ins, op_id))
                {
                    // Bits
                    inst.width = std::max(
                        inst.width, INS_OperandWidth(ins, op_id)
                    );
                }
            }

            //
            basic_blk->insts.push_back(inst);
        }

        //
        BBL_InsertCall(
            bbl, IPOINT_BEFORE,
            (AFUNPTR) handle_bbl,
            IARG_THREAD_ID,
            IARG_PTR, basic_blk,
            IARG_END
        );

        //
        gltracesim::simulator->register_new_code(basic_blk);
    }
}

static VOID
handle_gpu_thread_start(THREADID tid, CONTEXT *ctxt, INT32 flags, VOID *v)
{
    gltracesim::simulator->handle_gpu_thread_start(tid);
}

static VOID
handle_gpu_thread_stop(THREADID tid, const CONTEXT *ctxt, INT32 flags, VOID *v)
{
    gltracesim::simulator->handle_gpu_thread_stop(tid);
}

static VOID
handle_app_finish(INT32 code, VOID *v)
{
    gltracesim::simulator = NULL;
}

// Output dir
KNOB<string> KnobOutputDir(KNOB_MODE_WRITEONCE, "pintool",
    "output_dir", "out", "specify trace file name");

int
main(int argc, char *argv[])
{
    //
    PIN_InitSymbols();

    //
    printf("Init...\n");

    //
    if (PIN_Init(argc, argv))
    {
        printf("GLTraceSim GPU Memory Analyzer.\n");
        printf("%s\n", KNOB_BASE::StringKnobSummary().c_str());
        return EXIT_FAILURE;
    }

    //
    gltracesim::simulator = gltracesim::GlTraceSimPtr(
        new gltracesim::GlTraceSim(KnobOutputDir.Value())
    );

    //
    INS_AddInstrumentFunction(handle_pin_instruction, 0);
    IMG_AddInstrumentFunction(handle_pin_image, 0);

    // Count compute demand
    if (gltracesim::simulator->config.get(
        "intrument-basic-blocks", false
        ).asBool())
    {
        TRACE_AddInstrumentFunction(handle_pin_trace, 0);
    }

    PIN_AddThreadStartFunction(handle_gpu_thread_start, 0);
    PIN_AddThreadFiniFunction(handle_gpu_thread_stop, 0);

    //
    PIN_AddFiniFunction(handle_app_finish, 0);

    //
    printf("Init done, starting application...\n");

    //
    gltracesim::simulator->start();

    // Never returns
    PIN_StartProgram();

    return 0;
}

/* ===================================================================== */
