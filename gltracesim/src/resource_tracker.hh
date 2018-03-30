#ifndef __GLTRACESIM_RESOURCE_TRACKER_HH__
#define __GLTRACESIM_RESOURCE_TRACKER_HH__

#include <map>
#include <unordered_map>
#include <memory>
#include <vector>
#include "resource.hh"
#include "util/addr_range_map.hh"

namespace gltracesim {

class ResourceTracker {

public:

    //
    typedef AddrRangeMap<GpuResourcePtr> AddrMap;
    //
    typedef std::unordered_map<uint64_t, GpuResourcePtr> IdMap;
    //
    typedef std::vector<GpuResourcePtr> Vector;


public:

    /**
     * @brief Analyzer
     */
    ResourceTracker();

    /**
     * @brief ~Analyzer
     */
    virtual ~ResourceTracker();

    /**
     * @brief add
     * @param gpu_resource
     */
    void add(GpuResourcePtr gpu_resource)
    {
        //
        alive_addr_map.insert(gpu_resource->addr_range, gpu_resource);

        //
        id_map[gpu_resource->id] = gpu_resource;
    }

    /**
     * @brief add_to_map
     * @param gpu_resource
     */
    void map(GpuResourcePtr gpu_resource)
    {
        //
        id_map[gpu_resource->id] = gpu_resource;
    }

    /**
     * @brief add
     * @param gpu_resource
     */
    void destroy(GpuResourcePtr gpu_resource)
    {
        //
        AddrMap::iterator it = alive_addr_map.find(
            gpu_resource->addr_range.start
        );

        assert(it != alive_addr_map.end());

        //
        alive_addr_map.erase(it);

        //
        gpu_resource->dead = true;

        //
        dead_vector.push_back(gpu_resource);
    }

    /**
     * @brief resurrect
     * @param gpu_resource
     */
    void resurrect(GpuResourcePtr gpu_resource) {
        //
        gpu_resource->zombie = true;

        //
        zombie_vector.push_back(gpu_resource);
    }

    /**
     * @brief add
     * @param gpu_resource
     */
    GpuResourcePtr find_addr(uint64_t addr)
    {
        //
        AddrMap::iterator it = alive_addr_map.find(addr);

        if (_u(it == alive_addr_map.end())) {
            return null_ptr;
        } else {
            return it->second;
        }
    }

    /**
     * @brief add
     * @param gpu_resource
     */
    GpuResourcePtr find_id(uint64_t id)
    {
        //
        IdMap::iterator it = id_map.find(id);

        if (_u(it == id_map.end())) {
            return null_ptr;
        } else {
            //
            return it->second;
        }
    }

    /**
     * @brief get_dead
     * @return
     */
    AddrMap& get_alive() {
        return alive_addr_map;
    }

    /**
     * @brief get_dead
     * @return
     */
    Vector& get_dead() {
        return dead_vector;
    }

    /**
     * @brief clear_dead
     * @return
     */
    void clear_dead() {
        dead_vector.clear();
    }

    /**
     * @brief get_dead
     * @return
     */
    Vector& get_zombies() {
        return zombie_vector;
    }

    /**
     * @brief clear_dead
     * @return
     */
    void clear_zombies() {
        // Kill it again for next frame.
        for (auto &gpu_resource: zombie_vector) {
            gpu_resource->zombie = false;
        }

        zombie_vector.clear();
    }

private:
    //
    GpuResourcePtr null_ptr;

     // Addr -> Resource
    AddrMap alive_addr_map;

    // Addr -> Resource
    Vector dead_vector;

    // Addr -> Resource
    Vector zombie_vector;

    // ID -> Resource
    IdMap id_map;

};

//
typedef std::unique_ptr<ResourceTracker> ResourceTrackerPtr;

} // end namespace gltracesim

#endif // __GLTRACESIM_RESOURCE_TRACKER_HH__
