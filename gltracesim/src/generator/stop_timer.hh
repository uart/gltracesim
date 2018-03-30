#ifndef __GLTRACESIM_STOP_TIMER_HH__
#define __GLTRACESIM_STOP_TIMER_HH__

#include "pin.H"
#include <cstdint>
#include <chrono>

namespace gltracesim {

class StopTimer {

public:

    /**
     * @brief StopTimer
     */
    StopTimer(uint64_t seconds);

    /**
     * @brief ~StopTimer
     */
    virtual ~StopTimer();

private:

    /**
     * @brief thread
     */
    void thread();

    /**
     * @brief _thread
     * @param _this
     */
    static void _thread(void *_this);

    /**
     * @brief tid
     */
    unsigned tid;

    /**
     * @brief seconds
     */
    uint64_t seconds;

    //
    typedef std::chrono::time_point<std::chrono::system_clock> time_point_t;
    //
    time_point_t start;
};

} // end namespace gltracesim

#endif // __GLTRACESIM_STOP_TIMER_HH__
