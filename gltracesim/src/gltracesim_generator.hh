#ifndef __GLTRACESIM_HH__
#define __GLTRACESIM_HH__

#include <memory>
#include <array>
#include <vector>
#include <json/json.h>

#include "pin.H"

#include "util/addr_range.hh"
#include "util/addr_range_map.hh"
#include "util/threads.hh"
#include "device.hh"
#include "analyzer.hh"
#include "vmemory.hh"
#include "resource_tracker.hh"

#include "job.hh"
#include "scene.hh"
#include "frame.hh"

#include "generator/filter_cache.hh"
#include "generator/stop_timer.hh"
#include "generator/pipeline/pipeline.hh"

#include "gem5/protoio.hh"

namespace gltracesim {

class GlTraceSim {

public:

    // Configuration
    Json::Value config;

    /**
     * @brief The mem_inst_t struct
     */
    struct mem_inst_t {

        /**
         * @brief mem_inst_t
         */
        mem_inst_t();

        /**
         * @brief addr
         */
        void* addr;

        /**
         * @brief accesses_gpu_resource
         */
        bool accesses_gpu_resource;

        /**
         * @brief rsc_misses
         */
        size_t rsc_misses;

        /**
         * @brief filter
         */
        bool filter;
    };

public:

    /**
     * @brief Analyzer
     */
    GlTraceSim(const std::string &output_dir);

    /**
     * @brief ~Analyzer
     */
    virtual ~GlTraceSim();

    /**
     * @brief start
     */
    void start();

public:

    /* Pin/GPU */

    /**
     * @brief pause_and_drain_buffers
     */
    void pause_and_drain_buffers();

    /**
     * @brief resume
     */
    void resume();

    /**
     * @brief drain_buffers
     */
    void drain_buffers() {
        //
        pause_and_drain_buffers();
        //
        resume();
    }

    /**
     * @brief process_buffer
     * @param gpu_tid
     */
    void process_buffer(int gid);

    /**
     * @brief handle_sync
     * @param gid
     * @param cmd
     */
    void handle_sync(int gid, int cmd);

    /**
     * @brief handle_new_job
     * @param gid
     * @param job
     */
    void handle_new_job(int gid, GpuJobPtr job);

    /**
     * @brief handle_end_job
     * @param gid
     */
    void handle_end_job(int gid);

    /**
     * @brief handle_bbl
     * @param tid
     * @param basic_blk
     */
    void handle_bbl(int tid, GpuJob::basic_blk_t* basic_blk);

    /**
     * @brief handle_mem_access
     * @param tid Thread ID
     * @param mem_op Memory inststuction
     * @param addr Address
     * @param len Length
     * @param type Type of operation.
     */
    void handle_mem_access(
        int tid, mem_inst_t *mem_inst, uint64_t addr, uint8_t len, uint8_t type
    );

    /**
     * @brief handle_opengl_call
     * @param tid
     * @param call_id
     */
    void handle_opengl_call(int tid, uint32_t call_id);

    /**
     * @brief handle_gpu_resource_create
     * @param tid
     * @param lpr
     */
    void handle_gpu_resource_create(int tid, struct llvmpipe_resource *lpr);

    /**
     * @brief handle_gpu_resource_destroy
     * @param tid
     * @param lpr
     */
    void handle_gpu_resource_destroy(int tid, struct llvmpipe_resource *lpr);

    /**
     * @brief handle_gpu_draw_vbo_begin
     * @param tid
     */
    void handle_gpu_draw_vbo_begin(int tid);

    /**
     * @brief handle_gpu_draw_vbo_end
     * @param tid
     */
    void handle_gpu_draw_vbo_end(int tid);

    /**
     * @brief handle_gpu_flush
     * @param tid
     * @param reason
     */
    void handle_gpu_flush(int tid, const char *reason);

    /**
     * @brief handle_gpu_scene_begin
     * @param tid
     * @param scene
     */
    void handle_gpu_scene_begin(int tid);

    /**
     * @brief handle_gpu_scene_end
     * @param tid
     * @param scene
     */
    void handle_gpu_scene_end(int tid);

    /**
     * @brief handle_gpu_tile_begin
     * @param tid
     * @param task
     */
    void handle_gpu_tile_begin(int tid, int x, int y);

    /**
     * @brief handle_gpu_tile_end
     * @param tid
     * @param task
     */
    void handle_gpu_tile_end(int tid);

    /**
     * @brief handle_gpu_swap_buffers
     * @param tsc
     * @param tid
     */
    void handle_gpu_swap_buffers(int tid);

    /**
     * @brief handle_gpu_thread_start
     * @param tid
     */
    void handle_gpu_thread_start(int tid);

    /**
     * @brief handle_gpu_thread_stop
     * @param tid
     */
    void handle_gpu_thread_stop(int tid);

    /* Filtering */

    /**
     * @brief flush_filter_cache
     * @param tid
     * @param dev
     */
    void flush_filter_cache(int fid, int dev);

    /**
     * @brief process_filter_buffer_item
     * @param fid
     * @param tbi
     * @param out_buffer
     */
    void process_filter_buffer_item(int fid, packet_t &pkt);

    /**
     * @brief process_filter_buffer
     * @param fid
     * @param in_buffer
     * @param out_buffer
     */
    void process_filter_buffer(
        int fid,
        buffer_t *in_buffer
    );

    /**
     * @brief filter_thread_work
     * @param fid
     */
    void filter_thread_work_loop(int fid);

    /* Analysis */

    /**
     * @brief analysis_thread_work
     * @param aid
     */
    void analysis_thread_work_loop(int aid);

    /**
     * @brief register_mem_inst
     * @param mem_inst
     */
    void register_mem_inst(mem_inst_t* mem_inst);

    /**
     * @brief register_new_code
     */
    void register_new_code(GpuJob::basic_blk_t* basic_blk);

    /**
     * @brief register_opengl_call
     * @param call
     */
    uint32_t register_opengl_call(const std::string &call);

private:

    //
    RWRMutex rw_mtx;

    /**
     * @brief The simulation_ctr_t struct
     */
    struct simulation_ctrl_t {
        // Start frame
        int start;
        // Stop frame
        int stop;
    } sim_ctrl;

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        //
        ProtoOutputStream *resources;
        //
        ProtoOutputStream *frames;
        //
        ProtoOutputStream *scenes;
        //
        ProtoOutputStream *jobs;
        //
        ProtoOutputStream *job_stats;
        //
        ProtoOutputStream *opengl;
        //
        ProtoOutputStream *sim_stats;
    } pb;

    /**
     * @brief The thread_state_t struct
     */
    struct thread_state_t {

        /**
         * @brief thread_state_t
         */
        thread_state_t();

        /**
         * Filter through line buffer
         */
        uint64_t line_buffer;

        /**
         *
         */
        bool line_buffer_dirty;

        /**
         * Filter through a filter cache
         */
        FilterCachePtr cache[2];

        /**
         * Last accessed resource
         */
        GpuResourcePtr last_gpu_resource;

        /**
         * @brief current_job
         */
        GpuJobPtr job;
    };

    /**
     * @brief thread_state
     */
    std::array<thread_state_t, MAX_THREADS> ts;

    /**
     * @brief pipe
     */
    pipeline::Pipeline *pipe;

    /**
     * @brief stop_timer
     */
    StopTimer *stop_timer;

    // Analyzers that will be analyzed
    std::vector<AnalyzerPtr> analyzers;

    /**
     * @brief auto_prune_mem_insts
     */
    bool auto_prune_mem_insts;

    /**
     * @brief has_new_code
     */
    bool has_new_code;

    /**
     * @brief mem_inst_filter
     */
    std::vector<mem_inst_t*> mem_inst_filter;

    /**
     * @brief mem_inst_filter
     */
    std::vector<GpuJob::basic_blk_t*> basic_blks;

    /**
     * @brief opengl_calls
     */
    std::vector<std::string> opengl_calls;

    /**
     * @brief current_scene
     */
    ScenePtr current_scene;

    /**
     * @brief current_frame
     */
    FramePtr current_frame;

    //
    struct stats_t {
        //
        stats_t();
        //
        size_t no_tot_mops[2];
        //
        size_t no_rsc_mops[2];
        //
        size_t no_rsc_offcore_mops[2];
        //
        size_t no_opengl_calls;
    };

    std::vector<stats_t> stats;
};

} // end namespace gltracesim

#endif // __GLTRACESIM_HH__
