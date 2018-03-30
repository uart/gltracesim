#include <cmath>
#include <sstream>
#include <sys/stat.h>

#include "job.hh"

#include "debug.hh"
#include "debug_impl.hh"

#include "util/mkdir.hh"

#include <json/json.h>

namespace gltracesim {


GpuJob::stats_t::stats_t()
{
    reset();
}

void
GpuJob::stats_t::reset()
{
    reads = 0;
    writes = 0;
    basic_blk_count.clear();
}



GpuJob::GpuJob(
    uint64_t id,
    uint16_t scene_id,
    uint16_t frame_id,
    Type type,
    dev::HardwareDevice dev)
    : id(id),
      scene_id(scene_id),
      frame_id(frame_id),
      type(type),
      dev(dev),
      core_id(-1),
      x(-1), y(-1),
      trace(NULL)
{

}

GpuJob::~GpuJob()
{
    if (trace) {
        delete trace;
    }
    assert(pkts.empty());
}

void
GpuJob::configure_trace_generator()
{
    //
    mkdir("%s/f%u64/s%u64/",
        system->get_output_dir().c_str(),
        frame_id,
        scene_id
    );

    //
    std::stringstream output_file;
    output_file << system->get_output_dir() << "/"
               << "f" << frame_id << "/"
               << "s" << scene_id << "/"
               << "j" << id << ".trace.pb.gz";

    Json::Value params;
    params["output-file"] = output_file.str();

    //
    trace = new gem5::AddrTraceGenerator(params, id);
}


void
GpuJob::load_trace()
{
    assert(pkts.empty());

    //
    std::stringstream input_file;

    //
    input_file << system->get_input_dir() << "/"
             << "f" << frame_id << "/"
             << "s" << scene_id << "/"
             << "j" << id << ".trace.pb.gz";


    //
    struct stat sb;

    //
    if (stat(input_file.str().c_str(), &sb) == -1) {
        return;
    }

    // No data, ignore.
    if (sb.st_size < 64) {
        return;
    }

    //
    ProtoInputStream *pkt_stream = new ProtoInputStream(input_file.str());

//    //
//    if (pkt_stream->good() == false) {
//        //
//        DPRINTF(Warn, "Failed to open stream to %s, retrying...\n",
//                input_file.str().c_str());
//        //
//        sleep(1);
//        //
//        delete pkt_stream;
//        // Retry
//        load_trace();
//        //
//        return;
//    }

    //
    ProtoMessage::PacketHeader hdr;
    //
    pkt_stream->read(hdr);

    //
    while (true) {
        //
        ProtoMessage::Packet pb_pkt;

        //
        if (pkt_stream->read(pb_pkt) == false) {
            break;
        }

        assert(pb_pkt.cmd() == gem5::MemCmd_ReadReq ||
               pb_pkt.cmd() == gem5::MemCmd_WriteReq);
        assert(pb_pkt.has_rsc_id());
        assert(pb_pkt.has_job_id());

        //
        packet_t pkt;
        pkt.cmd = (pb_pkt.cmd() == gem5::MemCmd_ReadReq) ? READ : WRITE;
        pkt.paddr = pb_pkt.addr();
        pkt.rsc_id = pb_pkt.rsc_id();
        pkt.job_id = pb_pkt.job_id();
        pkt.dev_id = pb_pkt.dev_id();

        //
        pkts.push_back(pkt);
    }

    //
    delete pkt_stream;
}

void
GpuJob::dump_info(gltracesim::proto::JobInfo *ji)
{
    //
    ji->set_frame_id(frame_id);
    ji->set_scene_id(scene_id);
    ji->set_id(id);

    ji->set_dev_id(dev);
    ji->set_type(proto::JobType(type));

    //
    if (type == TILE_JOB) {
        ji->set_x(x);
        ji->set_y(y);
    }
}


void
GpuJob::dump_stats(gltracesim::proto::JobStats *js)
{
    //
    js->set_id(id);
    js->set_reads(stats.reads);
    js->set_writes(stats.writes);

    //
    size_t tot_inst_count = 0;

    //
    std::map<uint64_t, size_t> inst_count;

    //
    for (auto &it : stats.basic_blk_count) {
        //
        for (auto &inst : it.first->insts) {
            //
            tot_inst_count += it.second;
            //
            inst_count[inst.id] += it.second;
        }
    }

    //
    js->set_insts(tot_inst_count);

    //
    for (auto &it : inst_count) {
        //
        auto cs = js->add_computation_stats();

        //
        inst_t inst;
        //
        inst.id = it.first;

        //
        cs->set_opcode(inst.opcode);
        //
        cs->set_width(inst.width);
        //
        cs->set_count(it.second);
    }
}

void
GpuJob::reset_stats()
{
    //
    stats.reset();
}

}
