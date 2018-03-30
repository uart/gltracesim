#ifndef __GLTRACESIM_ANALYZER_MEMORY_BASE_HH__
#define __GLTRACESIM_ANALYZER_MEMORY_BASE_HH__

#include "analyzer.hh"
#include <json/json.h>

namespace gltracesim {
namespace analyzer {
namespace memory {

class BaseModel : public Analyzer
{

public:

    /**
     * @brief Analyzer
     */
    BaseModel(const Json::Value &params);

    /**
     * @brief ~Analyzer
     */
    virtual ~BaseModel();

    /**
     * @brief process
     * @param pkt
     */
    void process(const packet_t &pkt) = 0;

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id) { /* do nothing */ }

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id) { /* do nothing */ }

    /**
     * @brief process
     * @param buffer
     */
    virtual void dump_stats() = 0;

    /**
     * @brief process
     * @param buffer
     */
    virtual void reset_stats() = 0;

protected:

    /**
     * @brief The pb_t struct
     */
    struct pb_t {
        //
        ProtoOutputStream *stats;
        //
        ProtoOutputStream *job_stats;
        //
        ProtoOutputStream *core_stats;
        //
        ProtoOutputStream *rsc_stats;
    } pb;

};

} // end namespace memory
} // end namespace analyzer
} // end namespace gltracesim

#endif // __GLTRACESIM_ANALYZER_MEMORY_BASE_HH__
