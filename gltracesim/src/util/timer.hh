#ifndef __GLTRACESIM_TIMER_HH__
#define __GLTRACESIM_TIMER_HH__

#include <chrono>

namespace gltracesim {

class Timer {

public:

    Timer();

public:

    /**
     * @brief start
     */
    void start();

    /**
     * @brief stop
     */
    void stop();

    /**
     * @brief duration
     * @return
     */
    double duration();

protected:

    /**
     * @brief time_point_t
     */
    typedef std::chrono::time_point<std::chrono::system_clock> time_point_t;

    /**
     * @brief start
     */
    time_point_t start_time;

    /**
     * @brief stop
     */
    time_point_t stop_time;

};

} // end namespace gltracesim

#endif // __GLTRACESIM_TIMER_HH__
