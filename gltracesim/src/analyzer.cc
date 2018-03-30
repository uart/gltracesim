#include <sstream>

#include "analyzer.hh"
#include "debug_impl.hh"

namespace gltracesim {

AnalyzerBuilder::AnalyzerBuilder(const std::string &name) : name(name)
{
    Analyzer::register_builder(name, this);
}

AnalyzerBuilder::~AnalyzerBuilder()
{
    // Do nothing
}

const std::string&
AnalyzerBuilder::get_name() const
{
    return name;
}

bool
AnalyzerBuilder::can_create(const std::string &name) const
{
    return (this->name == name);
}

Analyzer::builder_map_t&
Analyzer::builders()
{
    // Global map of all builders.
    static builder_map_t builders;

    //
    return builders;
}

AnalyzerBuilder*
Analyzer::find_builder(const std::string &name)
{
    if (Analyzer::builders().count(name) == 0) {
        return NULL;
    }

    return Analyzer::builders()[name];
}

void
Analyzer::register_builder(const std::string &name, AnalyzerBuilder* builder)
{
    builders()[name] = builder;
}

void
Analyzer::dprint_builders()
{
    for (auto &it: Analyzer::builders()) {
        DPRINTF(Init, "AnalyzerBuilder: %s -> %p.\n",
            it.first.c_str(), (void*) it.second
        );
    }
}


Analyzer::Analyzer(const Json::Value &p, int id) : id(id), params(p)
{

}

Analyzer::~Analyzer()
{

}

void
Analyzer::dump_stats()
{
    // Do nothing
}

void
Analyzer::reset_stats()
{
    // Do nothing
}

void
Analyzer::add_child_analyzer(Analyzer* analyzer)
{
    //
    child_analyzers.push_back(analyzer);
}

} // end namespace gltracesim

