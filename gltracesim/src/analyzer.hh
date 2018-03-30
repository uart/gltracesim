#ifndef __GLTRACESIM_MODEL_HH__
#define __GLTRACESIM_MODEL_HH__

#include <map>
#include <memory>
#include <string>
#include <json/json.h>

#include "packet.hh"
#include "gltracesim.pb.h"

#include "gem5/protoio.hh"

namespace gltracesim {

class Analyzer;

/**
 * @brief
 */
typedef std::shared_ptr<Analyzer> AnalyzerPtr;

/**
 * @brief The AnalyzerBuilder class
 */
class AnalyzerBuilder
{

public:

    /**
     * @brief AnalyzerBuilder
     * @param name
     */
    AnalyzerBuilder(const std::string &name);

    /**
     * @brief AnalyzerBuilder
     * @param name
     */
    virtual ~AnalyzerBuilder();

public:

    /**
     * @brief get_name
     * @return
     */
    const std::string& get_name() const;

    /**
     * @brief can_create
     * @param params
     */
    virtual bool can_create(const std::string &name) const;

    /**
     * @brief create
     * @param params
     * @return
     */
    virtual AnalyzerPtr create(const Json::Value &params) = 0;

protected:

    /**
     * @brief name
     */
    std::string name;
};

class Analyzer
{

public:

    /**
     * @brief Analyzer
     */
    Analyzer(const Json::Value &p, int id);

    /**
     * @brief ~Analyzer
     */
    virtual ~Analyzer();

    /**
     * @brief process
     * @param pkt
     */
    virtual void process(const packet_t &pkt) = 0;

    /**
     * @brief start_new_frame
     */
    virtual void start_new_frame(int frame_id) = 0;

    /**
     * @brief start_new_scene
     */
    virtual void start_new_scene(int frame_id, int scene_id) = 0;

    /**
     * @brief process
     * @param buffer
     */
    virtual void dump_stats();

    /**
     * @brief process
     * @param buffer
     */
    virtual void reset_stats();

    /**
     * @brief get_aid
     * @return
     */
    int get_id() const {
        return id;
    }

    /**
     * @brief add_child_analyzer
     */
    void add_child_analyzer(Analyzer*);

protected:

    /**
     * @brief send_packet
     * @param pkt
     */
    void send_packet(const packet_t &pkt)
    {
        for (auto &analyzer: child_analyzers)
        {
            //
            analyzer->process(pkt);
        }
    }

protected:

    /**
     * @brief id
     */
    int id;

    /**
     * @brief params
     */
    Json::Value params;

    /**
     * @brief mem_side_analyzers
     */
    std::vector<Analyzer*> child_analyzers;

public:

    /**
     * @brief find_builder
     * @param analyzer
     * @return
     */
    static AnalyzerBuilder* find_builder(const std::string &name);

    /**
     * @brief register_builder
     */
    static void register_builder(
        const std::string &name, AnalyzerBuilder* builder
    );

    /**
     * @brief dprint_builders
     */
    static void dprint_builders();

private:

    /**
     * @brief builder_map_t
     */
    typedef std::map<std::string, AnalyzerBuilder*> builder_map_t;

    /**
     * @brief builders Construct on first use.
     * @return
     */
    static builder_map_t& builders();

};

} // end namespace gltracesim

#endif // __GLTRACESIM_MODEL_HH__
