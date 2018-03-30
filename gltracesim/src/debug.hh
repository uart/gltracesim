 #ifndef __GLTRACESIM_DEBUG_HH__
#define __GLTRACESIM_DEBUG_HH__

#include <string>
#include <cstdint>
#include <cstdio>

#include <json/json.h>

#include "util/cflags.hh"
#include "util/threads.hh"

namespace gltracesim {

class Debug
{

public:

    /**
     * @brief The DebugFlag enum
     */
    enum DebugFlag
    {
        Init = 0,
        Info,
        Warn,
        Error,
        OpenGL,
        AutoPrune,
        PinMemOp,
        ScheduleEvent,
        SyncEvent,
        GpuCoreMemOp,
        GpuOffCoreMemOp,
        GpuFlushEvent,
        GpuDrawVboEvent,
        GpuTileEvent,
        GpuSceneEvent,
        GpuFrameEvent,
        GpuResourceEvent,
        GpuThreadEvent,
        Prefetcher,
        VirtualMemoryManager,
        NUM_DEBUG_FLAGS
    };

    /**
     * @brief The DebugFlag enum
     */
    enum DebugLevel
    {
        Normal = 0,
        Verbose,
        SuperVerbose,
        NUM_DEBUG_LEVELS
    };

    /**
     * @brief init
     */
    static void init(const Json::Value &params);

    /**
     * @brief is_enabled
     * @return
     */
    static bool is_enabled();

    /**
     * @brief is_enabled
     * @return
     */
    static DebugLevel get_level();

    /**
     * @brief is_enabled
     * @return
     */
    static void turn_on();

    /**
     * @brief debug
     * @param flag
     * @return
     */
    static bool debug(DebugFlag flag);

    /**
     * @brief debug
     * @param flag
     * @return
     */
    static bool debug(int flag);

    /**
     * @brief set_flag
     * @param flag
     */
    static void set_flag(DebugFlag flag);

    /**
     * @brief get_event_name
     * @param flag
     * @return
     */
    static const char* get_event_name(int event);

    /**
     *
     */
    template <typename ...Args>
    static void printf(
        DebugFlag event, const std::string &name,
        const char *fmt, const Args &...args
    );

    /**
     *
     */
    template <typename ...Args>
    void printf(
        int id, const std::string &name,
        const char *fmt, const Args &...args
    );

private:

    /**
     * @brief mtx
     */
    static Mutex mtx;

    /**
     * @brief enabled
     */
    static bool enabled;

    /**
     * @brief enabled
     */
    static DebugLevel level;

    /**
     * @brief Debug
     */
    static bool flags[NUM_DEBUG_FLAGS];

    /**
     * @brief Debug
     */
    static std::vector<bool> id_flags;

    /**
     * @brief debug_flag_names
     */
    static const char* debug_flag_names[NUM_DEBUG_FLAGS];

};

#ifdef __GLTRACESIM_DEBUG_ON__

#define DPRINTF(x, ...) do {                                                   \
    if (_u(Debug::debug(Debug::x) && Debug::is_enabled())) {              \
        Debug::printf(Debug::x, __FILE__, __VA_ARGS__);                        \
    }                                                                          \
} while (0)

#define LDPRINTF(l, x, ...) do {                                               \
    if (_u(Debug::debug(Debug::x) && l <= Debug::get_level() && Debug::is_enabled())) { \
        Debug::printf(Debug::x, __FILE__, __VA_ARGS__);                        \
    }                                                                          \
} while (0)

#define IDPRINTF(id, ...) do {                                                 \
    if (_u(Debug::debug(id) && Debug::is_enabled())) {                             \
        Debug::printf(id, __FILE__, __VA_ARGS__);                              \
    }                                                                          \
} while (0)

#define LIDPRINTF(l, id, ...) do {                                             \
    if (_u(Debug::debug(id)  && l <= Debug::get_level() && Debug::is_enabled())) {      \
        Debug::printf(id, __FILE__, __VA_ARGS__);                              \
    }                                                                          \
} while (0)


#else

#define DPRINTF(x, ...) do {} while (0)

#endif

} // end namespace gltracesim

#endif // __GLTRACESIM_DEBUG_HH__
