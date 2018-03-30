#include "util/timer.hh"

namespace gltracesim {

Timer::Timer()
{

}

void
Timer::start()
{
    //
    start_time = std::chrono::system_clock::now();
}

void
Timer::stop()
{
    //
    stop_time = std::chrono::system_clock::now();
}

double
Timer::duration()
{
    //
    std::chrono::duration<double> d = stop_time - start_time;
    //
    return d.count();
}

} // end namespace gltracesim

