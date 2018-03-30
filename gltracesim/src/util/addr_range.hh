#ifndef __GLTRACESIM_ADDR_RANGE_HH__
#define __GLTRACESIM_ADDR_RANGE_HH__

namespace gltracesim {

struct AddrRange {
    uint64_t start;
    uint64_t end;

    AddrRange(uint64_t start = 0)
        : start(start), end(start + 1)
    {

    }

    AddrRange(uint64_t start, uint64_t end)
        : start(start), end(end)
    {

    }

    uint64_t size() const {
        return (end - start + 1);
    }

    bool operator<(const AddrRange& r) const {
        return start < r.start;
    }

    bool operator==(const AddrRange& r) const {
        if (start    != r.start)    return false;
        if (end      != r.end)      return false;
        return true;
    }

    bool contains(const uint64_t a) const {
        return (start <= a && a <= end);
    }
};

} // end namespace gltracesim

#endif // __GLTRACESIM_ADDR_RANGE_HH__
