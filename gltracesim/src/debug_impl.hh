#ifndef __GLTRACESIM_TRACE_IMPL_HH__
#define __GLTRACESIM_TRACE_IMPL_HH__

#include <cstdio>
#include "debug.hh"
#include "system.hh"

namespace gltracesim {

inline bool
Debug::is_enabled()
{
    return enabled;
}

inline Debug::DebugLevel
Debug::get_level()
{
    return level;
}

inline bool
Debug::debug(DebugFlag flag)
{
    return flags[flag];
}

inline bool
Debug::debug(int flag)
{
    //
    int id_flag = NUM_DEBUG_FLAGS + flag;

    //
    return (id_flag < int(id_flags.size()) && id_flags[id_flag]);
}

template <typename ...Args>
void
Debug::printf(
    DebugFlag event, const std::string &name,
    const char *fmt, const Args &...args)
{
    uint64_t tsc = 0;

    if (gltracesim::system) {
        tsc = gltracesim::system->get_tsc();
    }

    mtx.lock();
    ::printf("%14lu: <%s> ", tsc, Debug::debug_flag_names[event]);
    ::printf(fmt, args...);
    mtx.unlock();
}

template <typename ...Args>
void
Debug::printf(
    int id, const std::string &name,
    const char *fmt, const Args &...args)
{
    uint64_t tsc = 0;

    if (gltracesim::system) {
        tsc = gltracesim::system->get_tsc();
    }

    mtx.lock();
    ::printf("%14lu: <id:%i> ", tsc, id);
    ::printf(fmt, args...);
    mtx.unlock();
}

} // end namespace gltracesim

#endif // __GLTRACESIM_TRACE_IMPL_HH__
