#ifndef __GLTRACESIM_SATURATION_COUNTER_HH__
#define __GLTRACESIM_SATURATION_COUNTER_HH__

namespace gltracesim {

template<typename T>
class SatCounter {

public:

    /**
     * @brief SatCounter
     * @param min
     * @param max
     */
    SatCounter(T min = 0, T max = 1);

    /**
     * @brief min
     * @return
     */
    T min() const;

    /**
     * @brief set_min
     * @return
     */
    void set_min(T min);

    /**
     * @brief max
     * @return
     */
    T max() const;

    /**
     * @brief set_max
     * @return
     */
    void set_max(T max);

    /**
     * @brief is_max
     * @return
     */
    bool is_max() const;

    /**
     * @brief is_min
     * @return
     */
    bool is_min() const;

    /**
     * @brief value
     * @return
     */
    T value() const;

    /**
     * @brief reset
     */
    void set(T value);

    /**
     * @brief reset
     */
    void reset();

    /**
     * @brief operator <
     * @param r
     * @return
     */
    bool operator<(const SatCounter<T>& r) const;

    /**
     * @brief operator ==
     * @param r
     * @return
     */
    bool operator==(const SatCounter<T>& r) const;

    /**
     * @brief operator --
     */
    void operator--(int);

    /**
     * @brief operator ++
     */
    void operator++(int);

    /**
     * @brief operator -=
     */
    SatCounter<T>& operator-=(const SatCounter<T>& r);

    /**
     * @brief operator +=
     */
    SatCounter<T>& operator+=(const SatCounter<T>& r);

private:

    /**
     * @brief min_val
     */
    T min_val;

    /**
     * @brief max_val
     */
    T max_val;

    /**
     * @brief val
     */
    T val;

};

} // end namespace gltracesim

#include "util/sat_counter_impl.hh"

#endif // __GLTRACESIM_SATURATION_COUNTER_HH__
