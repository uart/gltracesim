#include "debug_impl.hh"
#include "generator/stop_timer.hh"

namespace gltracesim {

VOID
StopTimer::_thread(VOID *_this) {
    //
    ((StopTimer*) _this)->thread();
}

StopTimer::StopTimer(uint64_t seconds) :
    seconds(seconds), start(std::chrono::system_clock::now())
{
    //
    tid = PIN_SpawnInternalThread(
        _thread, (void*) this, 0, NULL
    );

    //
    if (tid == INVALID_THREADID) {
        fprintf(stderr, "Faild to create filter thread.");
        exit(EXIT_FAILURE);
    }

    //
    DPRINTF(Init, "StopTimer [tid: %u, time: %lus].\n", tid, seconds);
}

StopTimer::~StopTimer()
{

}

void
StopTimer::thread()
{

    //
    DPRINTF(Init, "StopTimer will trigger in %lus.\n", seconds);

    while (true) {
        //
        PIN_Sleep(1000);

        //
        if (PIN_IsProcessExiting()) {
            return;
        }

        //
        std::chrono::time_point<std::chrono::system_clock> now =
                std::chrono::system_clock::now();

        //
        std::chrono::duration<double> duration = now - start;

        if (duration.count() > seconds) {
            //
            DPRINTF(Init, "StopTimer triggered. Exiting...\n");
            // Exit
            PIN_ExitApplication(EXIT_SUCCESS);
            //
            exit(EXIT_SUCCESS);
        }
    }

}

} // end namespace gltracesim

