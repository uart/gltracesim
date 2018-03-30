#ifndef __GLTRACESIM_SATURATION_COUNTER_IMPL_HH__
#define __GLTRACESIM_SATURATION_COUNTER_IMPL_HH__

namespace gltracesim {

template<typename T>
SatCounter<T>::SatCounter(T min, T max) :
    min_val(min), max_val(max), val(min)
{

}

template<typename T>
T
SatCounter<T>::min() const
{
    return min_val;
}

template<typename T>
void
SatCounter<T>::set_min(T min)
{
    min_val = min;
}

template<typename T>
T
SatCounter<T>::max() const
{
    return max_val;
}

template<typename T>
void
SatCounter<T>::set_max(T max)
{
    max_val = max;
}

template<typename T>
bool
SatCounter<T>::is_min() const
{
    return (val == min_val);
}

template<typename T>
bool
SatCounter<T>::is_max() const
{
    return (val == max_val);
}


template<typename T>
T
SatCounter<T>::value() const
{
    return val;
}

template<typename T>
void
SatCounter<T>::set(T value)
{
    if (value < min_val) {
        val = min_val;
    } else if (value > max_val) {
        val = max_val;
    } else {
        val = value;
    }
}

template<typename T>
void
SatCounter<T>::reset()
{
    val = min_val;
}

template<typename T>
bool
SatCounter<T>::operator<(const SatCounter<T>& r) const
{
    return (val < r.val);
}

template<typename T>
bool
SatCounter<T>::operator==(const SatCounter<T>& r) const
{
    return (val == r.val);
}

template<typename T>
void
SatCounter<T>::operator--(int)
{
    auto t = val - 1;
    if (t >= min_val)
        val = t;
}

template<typename T>
void
SatCounter<T>::operator++(int)
{
    auto t = val + 1;
    if (t <= max_val)
        val = t;
}

template<typename T>
SatCounter<T>&
SatCounter<T>::operator-=(const SatCounter<T>& r)
{
    auto t = val - r.val;
    if (t > min_val)
        val = t;
    return *this;
}

template<typename T>
SatCounter<T>&
SatCounter<T>::operator+=(const SatCounter<T>& r)
{
    auto t = val + r.val;
    if (t < max_val)
        val = t;

    return *this;
}

} // end namespace gltracesim

#endif // __GLTRACESIM_SATURATION_COUNTER_IMPL_HH__
