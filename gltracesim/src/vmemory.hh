#ifndef __GLTRACESIM_VMEMORY_HH__
#define __GLTRACESIM_VMEMORY_HH__

#include <map>
#include <memory>
#include <string>
#include <vector>
#include <bitset>
#include <random>
#include <json/json.h>

#include "util/addr_range.hh"
#include "util/addr_range_map.hh"

namespace gltracesim {

class VirtualMemoryManager;

/**
 * @brief
 */
typedef std::unique_ptr<VirtualMemoryManager> VirtualMemoryManagerPtr;

/**
 * @brief The VirtualMemoryManager class
 *
 * Reverse memory allocator, given a virtual address, allocate a fake physical
 * address.
 *
 */
class VirtualMemoryManager
{

public:

    /**
     * @brief VirtualMemoryManager
     * @param name
     */
    VirtualMemoryManager(const Json::Value &params);

    /**
     * @brief ~VirtualMemoryManager
     */
    ~VirtualMemoryManager();

public:

    /**
     * @brief get_name
     * @return
     */
    uint64_t translate(uint64_t vaddr);

    /**
     * @brief get_name
     * @return
     */
    void alloc(const AddrRange &vaddr_range);

    /**
     * @brief get_name
     * @return
     */
    void free(const AddrRange &vaddr_range);

private:

    /**
     * @brief get_free_page
     * @param i
     * @param j
     */
    void find_free_page(size_t &i, size_t &j);

    /**
     * @brief get_num_free_pages
     * @return
     */
    size_t get_num_free_pages() const;

    /**
     * @brief get_page_addr
     * @param addr
     * @return
     */
    uint64_t get_page_addr(uint64_t addr) const
    {
        return (addr & page_addr_umask);
    }

    /**
     * @brief get_page_offset
     * @param addr
     * @return
     */
    uint64_t get_page_offset(uint64_t addr) const
    {
        return (addr & ~page_addr_umask);
    }

private:

    /**
     * @brief base_addr
     */
    uint64_t base_addr;

    /**
     * @brief page_size
     */
    uint64_t page_size;

    /**
     * @brief page_addr_umask
     */
    uint64_t page_addr_umask;

    /**
     * @brief fragmented
     */
    bool fragmented;

    /**
     * @brief rand_engine
     */
    std::mt19937 rand_engine;

    /**
     * @brief translation_t
     */
    typedef AddrRangeMap<uint64_t> translation_t;

    /**
     * @brief translation
     */
    translation_t translation;

#define STATE_T_SIZE 256

    /**
     * @brief state_t
     *
     * 256 * 4kB page == 1MB state;
     *
     */
    typedef std::bitset<STATE_T_SIZE> state_t;

    /**
     * @brief free_list
     */
    std::vector<state_t> free_list;

    /**
     * @brief allocated
     */
    size_t allocated;

};

} // end namespace gltracesim

#endif // __GLTRACESIM_VMEMORY_HH__
