#ifndef __GLTRACESIM_ADDR_RANGE_MAP_HH__
#define __GLTRACESIM_ADDR_RANGE_MAP_HH__

#include <map>
#include "util/cflags.hh"

namespace gltracesim {

template <typename V>
struct AddrRangeMap
{
    //
    typedef std::map<AddrRange, V> RangeMap;
    //
    typedef typename RangeMap::iterator iterator;
    typedef typename RangeMap::const_iterator const_iterator;

    //
    RangeMap map;

    iterator find(uint64_t addr)
    {
        if (_u(map.empty())) {
            return map.end();
        }

        iterator it = map.upper_bound(AddrRange(addr));

        if (it == map.begin()) {
            if (it->first.contains(addr)) {
                return it;
            } else {
                return map.end();
            }
        }

        --it;

        if (_l(it->first.contains(addr)))
            return it;

        return map.end();
    }

    void insert(const AddrRange &r, const V& d) {
        map.insert(std::make_pair(r, d));
    }

    void erase(iterator p) {
        map.erase(p);
    }

    void clear() {
        map.clear();
    }

    const_iterator begin() const {
        return map.begin();
    }

    iterator begin() {
        return map.begin();
    }

    const_iterator end() const {
        return map.end();
    }

    iterator end() {
        return map.end();
    }

    std::size_t size() const {
        return map.size();
    }

    bool empty() const {
        return map.empty();
    }
};

} // end namespace gltracesim

#endif // __GLTRACESIM_ADDR_RANGE_MAP_HH__
