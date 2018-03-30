#ifndef __GLTRACESIM_TRACE_HH__
#define __GLTRACESIM_TRACE_HH__

#include <array>

#include "analyzer.hh"
#include "gem5/protoio.hh"
#include "gem5/packet.pb.h"

#include "util/cflags.hh"

#include <json/json.h>

namespace gltracesim {
namespace gem5 {

enum {
    // gem5: src/mem/packet.hh
    MemCmd_ReadReq    = (0x1),
    MemCmd_WriteReq   = (0x4),
    //
    NewFrameCMD       = (0x1 << 10),
    SyncCMD           = (0x2 << 10),
    SyncProvidesCMD   = (0x3 << 10),
    SyncRequiresCMD   = (0x4 << 10),
    SyncInsnCMD       = (0x5 << 10),
    NewJobCMD         = (0x6 << 10),
    EndJobCMD         = (0x7 << 10),
    NewResourceCMD    = (0x8 << 10),
    EndResourceCMD    = (0x9 << 10),
    NewSceneCMD       = (0xA << 10),
    EndSceneCMD       = (0xB << 10),
    OpenGlCMD         = (0xC << 10)
};

class TraceGenerator : public Analyzer
{

public:

    /**
     * @brief Analyzer
     */
    TraceGenerator(const Json::Value &params, int id);

    /**
     * @brief ~Analyzer
     */
    virtual ~TraceGenerator();

    /**
     * @brief process
     * @param pkt
     */
    virtual void process(const packet_t &pkt) = 0;

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id) { /* do nothing */ }

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id) { /* do nothing */ }

protected:

    /**
     * @brief tick
     */
    static size_t tick;

};

class AddrTraceGenerator : public TraceGenerator
{

public:

    /**
     * @brief Analyzer
     */
    AddrTraceGenerator(const Json::Value &params, int id);

    /**
     * @brief ~Analyzer
     */
    virtual ~AddrTraceGenerator();

    /**
     * @brief process
     * @param pkt
     */
    virtual void process(const packet_t &pkt);

private:

    /**
     * @brief trace_file
     */
    ProtoOutputStream* trace_file;

};

class CmdTraceGenerator : public TraceGenerator
{

public:

    /**
     * @brief Analyzer
     */
    CmdTraceGenerator(const Json::Value &params, int id);

    /**
     * @brief ~Analyzer
     */
    virtual ~CmdTraceGenerator();

    /**
     * @brief process
     * @param pkt
     */
    virtual void process(const packet_t &pkt);

private:

    /**
     * @brief trace_file [ CPU | GPU ]
     */
    std::array<ProtoOutputStream*, 2> trace_file;
};

} // end namespace gem5
} // end namespace gltracesim

#endif // __GLTRACESIM_TRACE_HH__
