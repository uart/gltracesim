#ifndef __GLTRACESIM_STATS_VECTOR_HH__
#define __GLTRACESIM_STATS_VECTOR_HH__

#include <array>

namespace gltracesim {
namespace stats {

template<class T, std::size_t N>
class Vector : public std::array<T, N>
{

    //
    typedef std::array<T, N> BaseClass;

public:

    //
    Vector();
    //
    Vector& operator+=(const Vector &other);
    //
    void reset();

};

template<std::size_t N>
class IntVector : public Vector<uint64_t, N>
{

};

template<std::size_t N>
class FloatVector : public Vector<double, N>
{

};


} // end namespace stats
} // end namespace gltracesim

#endif // __GLTRACESIM_STATS_VECTOR_HH__
