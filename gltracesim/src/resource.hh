#ifndef __GLTRACESIM_RESOURCE_HH__
#define __GLTRACESIM_RESOURCE_HH__

#include <memory>
#include <cstdint>

#include "gltracesim.pb.h"
#include "util/addr_range.hh"

#include "stats/distribution.hh"
#include "stats/distribution_impl.hh"

#ifdef UNUSED
    #undef UNUSED
    #include "gallium/drivers/llvmpipe/lp_texture.h"
    #define UNUSED
#else
    #include "gallium/drivers/llvmpipe/lp_texture.h"
    #undef UNUSED
#endif

namespace gltracesim {

class GpuResource
{

public:

    /**
     * @brief The stats_t struct
     */
    struct stats_t
    {
        //
        stats_t();

        //
        void reset();

        bool used;

        uint64_t gpu_read_blks;
        uint64_t gpu_write_blks;

        uint64_t gpu_core_read_blks;
        uint64_t gpu_core_write_blks;

        /**
         * @brief mipmap_utilization
         */
        stats::Distribution mipmap_utilization;

    };

    /**
     * @brief The state_t struct
     */
    struct state_t {

        //
        state_t();

        //
        void reset();

        bool read;
        bool written;
    };

    /**
     * @brief The blk_state_t struct
     */
    struct blk_state_t
    {
        //
        blk_state_t();

        //
        void reset();

        // Touched on a miss or writeback
        uint8_t gpu_read_touched;
        uint8_t gpu_write_touched;

        // Touched on a gpu access
        uint8_t gpu_core_read_touched;
        uint8_t gpu_core_write_touched;
    };

public:

    /**
     * @brief GpuResource
     * @param lpr
     * @param dev
     */
    GpuResource(struct llvmpipe_resource *lpr, int dev);

public:

    /**
     * @brief id
     */
    uint64_t id;

    /**
     * @brief blk_size
     */
    uint64_t blk_size;

    /**
     * @brief sample_ctr
     */
    uint64_t sample_ctr;

    /**
     * @brief deleted
     */
    bool dead;

    /**
     * @brief zombies
     */
    bool zombie;

    /**
     * @brief dev
     */
    bool dev;

    /**
     * @brief addr_range
     */
    AddrRange addr_range;

    /**
     * @brief frame_stats
     */
    stats_t frame_stats;

    /**
     * @brief scene_state
     */
    state_t scene_state;

    /**
     * @brief resource_usage_vector_t
     */
    typedef std::vector<blk_state_t> blk_state_vector_t;

    /**
     * @brief blk_state
     */
    blk_state_vector_t blk_state;

    /**
     * @brief blk_state
     */
    blk_state_vector_t last_frame_blk_state;

    /**
     * @brief sampled
     */
    bool sampled;

public:

    /**
     * @brief get_blk_idx
     * @param addr
     * @return
     */
    size_t get_blk_idx(uint64_t addr) const;

    /**
     * @brief size
     * @return
     */
    size_t size() const;

    /**
     * @brief size
     * @return
     */
    size_t get_mipmap_level(uint64_t addr) const;

    /**
     * @brief get_width
     * @return
     */
    size_t get_width() const;

    /**
     * @brief get_height
     * @return
     */
    size_t get_height() const;

    /**
     * @brief get_target_name
     * @return
     */
    const char* get_target_name() const;

    /**
     * @brief get_addr
     * @param lpr
     * @return
     */
    static uint64_t get_addr(struct llvmpipe_resource *lpr);

    /**
     * @brief get_addr
     * @param lpr
     * @return
     */
    static uint64_t get_size(struct llvmpipe_resource *lpr);

    /**
     * @brief dump_info
     * @param resource_info
     */
    void dump_info(gltracesim::proto::ResourceInfo *resource_info);

    /**
     * @brief dump_stats
     * @param resource_stats
     */
    void dump_stats(gltracesim::proto::ResourceStats *resource_stats);

    /**
     * @brief dump_png_image
     */
    void dump_png_image();

    /**
     * @brief dump_jpeg_image
     */
    void dump_jpeg_image();


    /**
     * @brief reset_stats
     */
    void reset_stats();

private:

    /**
     * @brief target_names
     */
    static const char* target_names[];

    /**
     * @brief lpr
     */
    struct llvmpipe_resource lpr;

    /**
     * @brief id_counter
     */
    static uint64_t id_counter;

private:

    /**
     * @brief mesa_can_unpack
     * @return
     */
    bool mesa_can_unpack();

    /**
     * @brief mesa_get_color
     * @param format
     * @param addr
     * @param r
     * @param g
     * @param b
     * @param a
     * @return
     */
    bool mesa_get_color(
        enum pipe_format format,
        void *addr,
        uint8_t &r,
        uint8_t &g,
        uint8_t &b,
        uint8_t &a
    );

};

//
typedef std::shared_ptr<GpuResource> GpuResourcePtr;

} // end namespace gltracesim

#include "resource_impl.hh"

#endif // __GLTRACESIM_RESOURCE_HH__
