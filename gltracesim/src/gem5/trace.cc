#include <sstream>
#include <iostream>

#include "device.hh"
#include "packet.hh"
#include "gem5/trace.hh"
#include "debug_impl.hh"

namespace gltracesim {
namespace gem5 {

size_t TraceGenerator::tick = 0;

TraceGenerator::TraceGenerator(const Json::Value &p, int id) :
    Analyzer(p, id)
{

}

TraceGenerator::~TraceGenerator()
{

}

AddrTraceGenerator::AddrTraceGenerator(const Json::Value &p, int id) :
    TraceGenerator(p, id)
{
    trace_file = new ProtoOutputStream(params["output-file"].asString());

    //
    ProtoMessage::PacketHeader hdr;

    //
    hdr.set_obj_id("gltracesim");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    //
    trace_file->write(hdr);
}

AddrTraceGenerator::~AddrTraceGenerator()
{
    delete trace_file;
}

void
AddrTraceGenerator::process(const packet_t &pkt)
{
    //
    ProtoMessage::Packet trace_data;

    //
    trace_data.set_tick(tick++);
    trace_data.set_addr(pkt.paddr);
    trace_data.set_size(pkt.length);
    trace_data.set_rsc_id(pkt.rsc_id);

    //
    if (pkt.job_id >= 0) {
        trace_data.set_job_id(pkt.job_id);
    }

    //
    switch (pkt.cmd) {
    case READ:
    {
        //
        assert(pkt.paddr);
        assert(trace_data.has_job_id());
        //
        trace_data.set_cmd(MemCmd_ReadReq);
        //
        trace_file->write(trace_data);
        //
        return;
    }
    case WRITE:
    {
        //
        assert(pkt.paddr);
        assert(trace_data.has_job_id());
        //
        trace_data.set_cmd(MemCmd_WriteReq);
        //
        trace_file->write(trace_data);
        //
        return;
    }
    default:
        assert(0);
    }
}

CmdTraceGenerator::CmdTraceGenerator(const Json::Value &p, int id) :
    TraceGenerator(p, id)
{
    trace_file[dev::CPU] = new ProtoOutputStream(
        params["output-cpu-file"].asString()
    );
    trace_file[dev::GPU] = new ProtoOutputStream(
        params["output-gpu-file"].asString()
    );

    //
    ProtoMessage::PacketHeader hdr;

    //
    hdr.set_obj_id("gltracesim");
    hdr.set_ver(0);
    hdr.set_tick_freq(1);

    //
    trace_file[dev::CPU]->write(hdr);
    trace_file[dev::GPU]->write(hdr);
}

CmdTraceGenerator::~CmdTraceGenerator()
{
    //
    for (auto &f: trace_file) {
        delete f;
    }
}

void
CmdTraceGenerator::process(const packet_t &pkt)
{
    //
    ProtoMessage::Packet trace_data;

    //
    trace_data.set_tick(tick++);
    trace_data.set_addr(pkt.paddr);
    trace_data.set_size(pkt.length);

    //
    if (pkt.rsc_id >= 0) {
        trace_data.set_rsc_id(pkt.rsc_id);
    }

    //
    if (pkt.job_id >= 0) {
        trace_data.set_job_id(pkt.job_id);
    }

    //
    switch (pkt.cmd) {
    case OPENGL_CALL:
    {
        //
        std::cout << trace_data.job_id() << std::endl;
        assert(trace_data.has_job_id());
        //
        trace_data.set_cmd(OpenGlCMD);
        trace_data.set_dev_id(pkt.dev_id);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case NEW_JOB:
    {
        //
        assert(trace_data.has_job_id());
        //
        trace_data.set_cmd(NewJobCMD);
        trace_data.set_dev_id(pkt.dev_id);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case END_JOB:
    {
        //
        assert(trace_data.has_job_id());
        //
        trace_data.set_cmd(EndJobCMD);
        trace_data.set_dev_id(pkt.dev_id);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case NEW_RESOURCE:
    {
        //
        assert(trace_data.has_rsc_id());
        //
        trace_data.set_cmd(NewResourceCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case END_RESOURCE:
    {
        //
        assert(trace_data.has_rsc_id());
        //
        trace_data.set_cmd(EndResourceCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case MV_TO_CPU:
    {
        // rsc_id
        assert(trace_data.has_rsc_id());
        //
        trace_data.set_cmd(SyncProvidesCMD);
        trace_file[dev::GPU]->write(trace_data);
        //
        trace_data.set_cmd(SyncRequiresCMD);
        trace_file[dev::CPU]->write(trace_data);
        //
        return;
    }
    case MV_TO_GPU:
    {
        // rsc_id
        assert(trace_data.has_rsc_id());
        //
        trace_data.set_cmd(SyncProvidesCMD);
        trace_file[dev::CPU]->write(trace_data);
        //
        trace_data.set_cmd(SyncRequiresCMD);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case SYNC:
    {
        //
        trace_data.set_cmd(SyncCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case NEW_SCENE:
    {
        //
        trace_data.set_cmd(NewSceneCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case END_SCENE:
    {
        //
        trace_data.set_cmd(EndSceneCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    case NEW_FRAME:
    {
        //
        trace_data.set_cmd(NewFrameCMD);
        //
        trace_file[dev::CPU]->write(trace_data);
        trace_file[dev::GPU]->write(trace_data);
        //
        return;
    }
    default:
        assert(0);
    }

}

} // end namespace gem5
} // end namespace gltracesim

