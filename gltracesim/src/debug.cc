#include <set>
#include "debug.hh"

namespace gltracesim {

const char*
Debug::debug_flag_names[] = {
    "Init",
    "Info",
    "Warn",
    "Error",
    "OpenGL",
    "AutoPrune",
    "PinMemOp",
    "ScheduleEvent",
    "SyncEvent",
    "GpuCoreMemOp",
    "GpuOffCoreMemOp",
    "GpuFlushEvent",
    "GpuDrawVboEvent",
    "GpuTileEvent",
    "GpuSceneEvent",
    "GpuFrameEvent",
    "GpuResourceEvent",
    "GpuThreadEvent",
    "Prefetcher",
    "VirtualMemoryManager",
};

const char*
Debug::get_event_name(int event)
{
    return Debug::debug_flag_names[event];
}

Mutex Debug::mtx;
bool Debug::enabled;
Debug::DebugLevel Debug::level;
bool Debug::flags[Debug::NUM_DEBUG_FLAGS];
std::vector<bool> Debug::id_flags;

int
find_flag(const std::string &flag)
{
    for (int i = 0; i < Debug::NUM_DEBUG_FLAGS; ++i) {
        if (flag == Debug::get_event_name(i)) {
            return i;
        }
    }

    return -1;
}

void
Debug::init(const Json::Value &params)
{
    //
    Debug::enabled = params.get("enable", false).asBool();
    Debug::level = DebugLevel(params.get("level", Normal).asInt());

    //
    std::set<std::string> flags;
    for (unsigned i = 0; i < params["flags"].size(); ++i) {
        char *endptr;

        int id_flag = std::strtol(
            params["flags"][i].asCString(),
            &endptr,
            10
        );

        id_flag += NUM_DEBUG_FLAGS;

        if (*endptr == '\0') {
            id_flags.resize(id_flag + 1, false);
            id_flags[id_flag] = true;
        } else {
            flags.insert(params["flags"][i].asCString());
        }
    }

    //
    bool all = flags.count("All");
    //
    for (size_t i = 0; i < NUM_DEBUG_FLAGS; ++i) {
        Debug::flags[i] = all || flags.count(Debug::get_event_name(i));
    }
}

void
Debug::set_flag(DebugFlag flag)
{
    Debug::flags[flag] = 1;
}

} // end namespace gltracesim

