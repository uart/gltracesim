#ifndef __GLTRACESIM_STATS_VECTOR_IMPL_HH__
#define __GLTRACESIM_STATS_VECTOR_IMPL_HH__

#include "stats/vector.hh"

namespace gltracesim {
namespace stats {

template<typename T, std::size_t N>
Vector<T, N>::Vector() : BaseClass()
{

}

template<typename T, std::size_t N>
Vector<T, N>&
Vector<T, N>::operator+=(const Vector<T, N> &other)
{
    for (size_t i = 0; i < N; ++i) {
        (*this)[i] += other[i];
    }
    return *this;
}

template<typename T, std::size_t N>
void
Vector<T, N>::reset()
{
    for (size_t i = 0; i < N; ++i) {
        (*this)[i] = 0;
    }
}

} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_VECTOR_IMPL_HH__
