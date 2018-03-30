#ifndef __GLTRACESIM_TRACE_ANALYZER_HH__
#define __GLTRACESIM_TRACE_ANALYZER_HH__

#include <atomic>
#include <string>
#include <memory>
#include <vector>
#include <deque>
#include <thread>
#include <atomic>
#include <condition_variable>

#include "device.hh"
#include "analyzer.hh"

#include "analyzer/schedular/base.hh"
#include "analyzer/trace_manager.hh"

#include "gem5/trace.hh"
#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"

namespace gltracesim {

namespace analyzer {
class CPU;
class GPU;
}

/**
 *
 */
class GlTraceSimAnalyzer
{

public:

    /**
     * @brief The conf_t struct
     */
    struct conf_t {

        /**
         * @brief use_global_sync
         */
        bool use_global_sync;

        /**
         * @brief use_rsc_sync
         */
        bool use_rsc_sync;

        /**
         * @brief num_gpu_cores
         */
        size_t num_gpu_cores;

        /**
         * @brief batch_size
         */
        size_t batch_size;

    } conf;

public:

    /**
     * @brief VirtualMemoryManager
     * @param name
     */
    GlTraceSimAnalyzer(const std::string &output_dir);

    /**
     * @brief ~VirtualMemoryManager
     */
    ~GlTraceSimAnalyzer();

public:

    /**
     * @brief run
     */
    void run();

public:

    /**
     * @brief handle_new_scene
     */
    void handle_new_scene();

    /**
     * @brief handle_end_scene
     */
    void handle_end_scene();

    /**
     * @brief handle_new_frame
     */
    void handle_new_frame();

    /**
     * @brief handle_end_frame
     */
    void handle_end_frame();

    /**
     * @brief send_packet
     * @param pkt
     */
    void send_packet(packet_t &pkt);

    /**
     * @brief stop_timer_loop
     * @param seconds
     */
    void stop_timer_loop(uint64_t seconds);

private:

    /**
     * @brief stop_timer
     */
    std::thread *stop_timer;

    /**
     * @brief The simulation_ctr_t struct
     */
    struct simulation_ctrl_t {
        // Start epoch
        int start;
        // Stop epoch
        int stop;
    } sim_ctrl;

    /**
     * @brief The proto_t struct
     */
    struct proto_t {
        //
        ProtoOutputStream *stats;
    } pb;

    // Current frame
    FramePtr current_frame;

    /**
     * Analyzers that will be analyzed
     */
    std::vector<AnalyzerPtr> analyzers;

    /**
     * @brief cpu
     */
    analyzer::CPU *cpu;

    /**
     * @brief gpu
     */
    analyzer::GPU *gpu;

    /**
     * @brief schedular
     */
    analyzer::schedular::SchedularPtr schedular;

};


} // end namespace gltracesim

#endif // __GLTRACESIM_TRACE_ANALYZER_HH__
