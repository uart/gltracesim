#include "vmemory.hh"
#include "debug_impl.hh"

namespace gltracesim {

VirtualMemoryManager::VirtualMemoryManager(const Json::Value &params) :
    base_addr(params.get("base-addr", 4096).asLargestUInt()),
    page_size(params.get("page-size", 4096).asLargestUInt()),
    page_addr_umask(~(page_size - 1)),
    fragmented(params.get("fragmented", true).asBool()),
    allocated(0)
{
    size_t init_size = params.get("init-size", 0x1000000).asLargestUInt();

    DPRINTF(Init, "VirtualMemoryManager [base: 0x%x, init-size: %lu, page-size: %lu].\n",
        base_addr, init_size, page_size
    );

    //
    free_list.resize(
        init_size / (STATE_T_SIZE * page_size)
    );
}

VirtualMemoryManager::~VirtualMemoryManager()
{
    // Do nothing
}

uint64_t
VirtualMemoryManager::translate(uint64_t vaddr)
{
    //
    uint64_t page_vaddr = get_page_addr(vaddr);
    //
    translation_t::iterator it = translation.find(page_vaddr);
    //
    assert(it != translation.end());

    //
    uint64_t paddr = it->second | get_page_offset(vaddr);

    LDPRINTF(Debug::Verbose, VirtualMemoryManager,
        "Translate 0x%x [0x%x -> 0x%x].\n",
        page_vaddr, vaddr, paddr
    );

    //
    return paddr;
}

void
VirtualMemoryManager::find_free_page(size_t &i, size_t &j)
{
    while (true) {
        for (j = 0; j < free_list[i].size(); ++j) {
            if (free_list[i][j] == false) {
                return;
            }
        }
        i = (i + 1) % free_list.size();
    }
}

size_t
VirtualMemoryManager::get_num_free_pages() const {
    return (free_list.size() * STATE_T_SIZE * page_size) - allocated;
}

void
VirtualMemoryManager::alloc(const AddrRange &vaddr_range)
{
    //
    uint64_t start_vpage = get_page_addr(vaddr_range.start);
    uint64_t end_vpage = get_page_addr(vaddr_range.end);

    DPRINTF(VirtualMemoryManager, "Allocating range [0x%x -> 0x%x].\n",
        vaddr_range.start, vaddr_range.end
    );

    //
    for (uint64_t vpage = start_vpage; vpage <= end_vpage; vpage += page_size) {

        // Find free page

        // Grow free list if we run out of memory
        if (get_num_free_pages() == 0) {
            free_list.resize(free_list.size() + 1);

            DPRINTF(VirtualMemoryManager, "Increasing size to %lu.\n",
                allocated
            );
        }

        size_t i = 0;
        size_t j = 0;

        // If system is highly fragmeneted, randomly start searching for free
        // page
        if (fragmented) {
            //
            std::uniform_int_distribution<> rand_dist(0, free_list.size() - 1);
            //
            i = rand_dist(rand_engine);
        }

        // Search free list for unallocated page<
        find_free_page(i, j);

        // Mark as allocated
        free_list[i][j] = 1;

        //
        allocated += page_size;

        //
        uint64_t ppage_addr = base_addr + (i * STATE_T_SIZE + j) * page_size;


        DPRINTF(VirtualMemoryManager, "Allocating page 0x%x [0x%x -> 0x%x].\n",
            ppage_addr, vpage, ppage_addr
        );

        // Update tranlation table
        translation_t::iterator it = translation.find(vpage);
        //
        if (it == translation.end()) {
            //
            translation.insert(AddrRange(vpage), ppage_addr);
        } else {
            // Already allocate, ie. overlapping texture
            it->second = ppage_addr;
        }
    }
}

void
VirtualMemoryManager::free(const AddrRange &vaddr_range)
{
    // Update free list
    //
    uint64_t start_vpage = get_page_addr(vaddr_range.start);
    uint64_t end_vpage = get_page_addr(vaddr_range.end);

    DPRINTF(VirtualMemoryManager, "Freeing range [0x%x -> 0x%x].\n",
        vaddr_range.start, vaddr_range.end
    );

    //
    for (uint64_t vpage = start_vpage; vpage <= end_vpage; vpage += page_size) {
        //
        uint64_t ppage_nbr = (translate(vpage) - base_addr) / page_size ;

        size_t i = ppage_nbr / STATE_T_SIZE;
        size_t j = ppage_nbr % STATE_T_SIZE;

        // Mark as free
        free_list[i][j] = 0;

        DPRINTF(VirtualMemoryManager, "Freeing page 0x%x [0x%x -> 0x%x].\n",
            ppage_nbr * page_size, vpage, ppage_nbr * page_size
        );
    }
    // Leave tlb unmodified in case of overlapping textures
    // Ignore those cases for now
}

} // end namespace gltracesim

