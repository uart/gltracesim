#include <cmath>
#include <sstream>
#include <iomanip>

#include "resource.hh"
#include "resource_impl.hh"

#include "debug.hh"
#include "debug_impl.hh"

#include <png++/png.hpp>

#define HAVE___BUILTIN_FFSLL
#define HAVE___BUILTIN_FFS
#include "gallium/auxiliary/util/u_pack_color.h"

namespace gltracesim {

const char*
GpuResource::target_names[] = {
    "PIPE_BUFFER",
    "PIPE_TEXTURE_1D",
    "PIPE_TEXTURE_2D",
    "PIPE_TEXTURE_3D",
    "PIPE_TEXTURE_CUBE",
    "PIPE_TEXTURE_RECT",
    "PIPE_TEXTURE_1D_ARRAY",
    "PIPE_TEXTURE_2D_ARRAY",
    "PIPE_TEXTURE_CUBE_ARRAY",
};


GpuResource::stats_t::stats_t()
{
    reset();
}

void
GpuResource::stats_t::reset()
{
    used = 0;

    gpu_read_blks = 0;
    gpu_write_blks = 0;

    gpu_core_read_blks = 0;
    gpu_core_write_blks = 0;

    mipmap_utilization.reset();
}

GpuResource::state_t::state_t()
{
    reset();
}

void
GpuResource::state_t::reset()
{
    read = false;
    written = false;
}

GpuResource::blk_state_t::blk_state_t()
{
    reset();
}

void
GpuResource::blk_state_t::reset()
{
    //
    gpu_read_touched = 0;
    gpu_write_touched = 0;
    //
    gpu_core_read_touched = 0;
    gpu_core_write_touched = 0;
}

//
uint64_t GpuResource::id_counter = 0;

GpuResource::GpuResource(struct llvmpipe_resource *_lpr, int dev)
    : id(id_counter++), blk_size(gltracesim::system->get_blk_size()),
      sample_ctr(0), dead(false), zombie(false), dev(dev), sampled(false),
      lpr(*_lpr)
{
    //
    addr_range = AddrRange(
        get_addr(&lpr),
        get_addr(&lpr) + get_size(&lpr) - 1
    );

    //
    size_t no_blks = std::ceil(
        double(addr_range.size()) / gltracesim::system->get_blk_size()
    );

    //
    blk_state.resize(no_blks);
    last_frame_blk_state.resize(no_blks);

    //
    frame_stats.mipmap_utilization.init(0, lpr.base.last_level, 1);
}

size_t
GpuResource::get_mipmap_level(uint64_t addr) const
{
    //
    size_t offset = (addr - addr_range.start);

    // [ 0....0, 1...1, 2...2, ...]
    for (unsigned level = 0; level < lpr.base.last_level; ++level) {
        if (offset < lpr.mip_offsets[level + 1]) {
            return level;
        }
    }

    // First level, or no mipmap
    return 0;
}

void
GpuResource::dump_info(gltracesim::proto::ResourceInfo *ri)
{
    ri->set_id(id);
    ri->set_type(proto::ResourceType(lpr.base.target));
    ri->set_format(proto::ResourceFormat(lpr.base.format));
    ri->set_display_target(lpr.dt != NULL);
    ri->set_start_addr(addr_range.start);
    ri->set_end_addr(addr_range.end);
    ri->set_mipmap_levels(lpr.base.last_level);

    for (unsigned level = 0; level < lpr.base.last_level; ++level) {
        ri->add_mipmap_sizes(
            lpr.mip_offsets[level + 1] - lpr.mip_offsets[level]
        );
    }

    ri->set_msaa_samples(lpr.base.nr_samples);
    ri->set_width(lpr.base.width0);
    ri->set_height(lpr.base.height0);
    ri->set_depth(lpr.base.depth0);
    ri->set_array_size(lpr.base.array_size);
}

void
GpuResource::dump_stats(gltracesim::proto::ResourceStats *rs)
{
    rs->set_id(id);

    if (zombie) {
        rs->set_state(proto::ResourceState::ZOMBIE);
    } else if (dead) {
        rs->set_state(proto::ResourceState::DEAD);
    } else {
        rs->set_state(proto::ResourceState::ALIVE);
    }

    rs->set_gpu_read_blks(frame_stats.gpu_read_blks);
    rs->set_gpu_write_blks(frame_stats.gpu_write_blks);

    struct _blk_state_t {
        size_t gpu_touched;
        size_t gpu_read_touched;
        size_t gpu_write_touched;
        size_t gpu_core_touched;
        size_t gpu_core_read_touched;
        size_t gpu_core_write_touched;
    };

    _blk_state_t total = {0};

    for (auto &blk: blk_state)
    {
        total.gpu_read_touched += blk.gpu_read_touched;
        total.gpu_write_touched += blk.gpu_write_touched;
        total.gpu_touched += (blk.gpu_write_touched || blk.gpu_read_touched);
        total.gpu_core_read_touched += blk.gpu_core_read_touched;
        total.gpu_core_write_touched += blk.gpu_core_write_touched;
        total.gpu_core_touched += (
            blk.gpu_core_write_touched || blk.gpu_core_read_touched
        );
    }

    rs->set_gpu_touched_blks(total.gpu_touched);
    rs->set_gpu_read_touched_blks(total.gpu_read_touched);
    rs->set_gpu_write_touched_blks(total.gpu_write_touched);

    rs->set_gpu_core_read_blks(frame_stats.gpu_core_read_blks);
    rs->set_gpu_core_write_blks(frame_stats.gpu_core_write_blks);

    rs->set_gpu_touched_blks(total.gpu_core_touched);
    rs->set_gpu_core_read_touched_blks(total.gpu_core_read_touched);
    rs->set_gpu_core_write_touched_blks(total.gpu_core_write_touched);

    //
    frame_stats.mipmap_utilization.dump(
        rs->mutable_mipmap_utilization()
    );

    // Resample written texture
    if(total.gpu_write_touched) {
        sample_ctr = 0;
    }
}

void
GpuResource::reset_stats()
{
    //
    frame_stats.reset();

    //
    for (size_t i = 0; i < blk_state.size(); ++i) {
        // Save old state
        last_frame_blk_state[i] = blk_state[i];
        // Reset new state
        blk_state[i].reset();
    }
}

bool
mesa_can_unpack_format(int format)
{
    switch(format) {
    case PIPE_FORMAT_RGTC2_SNORM:
        return false;
    default:
        return true;
    }
    return true;
}

bool
GpuResource::mesa_can_unpack()
{

    auto fmt = lpr.base.format;

    if (mesa_can_unpack_format(fmt) == false) {
        return false;
    }

    if (util_format_is_depth_or_stencil(fmt))
    {
        if (fmt == PIPE_FORMAT_Z16_UNORM) {
        } else if (fmt == PIPE_FORMAT_Z32_UNORM) {
        } else if (fmt == PIPE_FORMAT_Z32_FLOAT) {
        } else if (fmt == PIPE_FORMAT_Z24_UNORM_S8_UINT){
        } else if (fmt == PIPE_FORMAT_Z24X8_UNORM) {
        } else if (fmt == PIPE_FORMAT_S8_UINT_Z24_UNORM) {
        } else if (fmt == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT) {
        } else {
            return false;
        }
    }

    return true;
}

bool
GpuResource::mesa_get_color(
    enum pipe_format format,
    void *addr,
    uint8_t &r,
    uint8_t &g,
    uint8_t &b,
    uint8_t &a)
{
    if (util_format_is_depth_or_stencil(format))
    {
        if (format == PIPE_FORMAT_Z16_UNORM)
        {
            uint16_t z = *((uint16_t*) addr);
            r = g = b = uint8_t((double(z) / 0xFFFF) * 0xFF);
        }
        else if (format == PIPE_FORMAT_Z32_UNORM)
        {
            uint32_t z = *((uint32_t*) addr);
            r = g = b = uint8_t((double(z) / 0xFFFFFFFF) * 0xFF);
        }
        else if (format == PIPE_FORMAT_Z32_FLOAT)
        {
            float z = *((float*) addr);
            r = g = b = uint8_t(z * 0xFF);
        }
        else if (format == PIPE_FORMAT_Z24_UNORM_S8_UINT ||
                 format == PIPE_FORMAT_Z24X8_UNORM)
        {
            uint32_t z = *((uint32_t*) addr);
            r = g = b = uint8_t((double(z) / ~uint32_t(0)) * 0xFF);
        }
        else if (format == PIPE_FORMAT_S8_UINT_Z24_UNORM)
        {
            uint32_t z = *((uint32_t*) addr);
            r = g = b = uint8_t((double(z) / ~uint32_t(0)) * 0xFF);
        }
        else if (format == PIPE_FORMAT_Z32_FLOAT_S8X24_UINT)
        {
            uint64_t z = *((uint64_t*) addr);
            r = g = b = uint8_t((double(z) / ~uint64_t(0)) * 0xFF);
        } else {
            return false;
        }
    } else {
        //
        util_unpack_color_ub(
            format,
            (union util_color*) addr,
            &r, &g, &b, &a
        );
    }

    return true;
}

void
GpuResource::dump_png_image()
{

    if (mesa_can_unpack() == false) {
        //
        DPRINTF(GpuResourceEvent,
            "Resource: %lu, dump_png_image -> Unsupported Resource Format [%s].\n",
            id, util_format_name(lpr.base.format)
        );

        return;
    }

    //
    auto &config = gltracesim::system->get_config();

    if (get_width() < config.get("min-picture-width", 16).asUInt64() ||
        get_height() < config.get("min-picture-height", 16).asUInt64()) {
        //
        DPRINTF(GpuResourceEvent,
            "Resource: %lu, dump_png_image -> Unsupported diminsions [%lux%lu].\n",
            id, get_width(), get_height()
        );

        return;
    }

    std::stringstream outputfile;
    outputfile << gltracesim::system->get_output_dir() << "/"
               << "f" << gltracesim::system->get_frame_nbr() << "/"
               << "s" << gltracesim::system->get_scene_nbr() << "/"
               << "r" << id << ".png";

    //
    png::image< png::rgba_pixel > image(get_width(), get_height());

    for (png::uint_32 y = 0; y < image.get_height(); ++y)
    {
        for (png::uint_32 x = 0; x < image.get_width(); ++x)
        {
            // Pixel row addr
            char* addr = (char*) (addr_range.start + y * lpr.row_stride[0]);
            // pixel col addr
            addr += x * util_format_get_blocksize(lpr.base.format);
            //
            uint8_t r, g, b, a = 0;
            //
            mesa_get_color(lpr.base.format, addr, r, g, b, a);
            //
            image[y][x] = png::rgba_pixel(r, g, b, a);
        }
    }

    //
    DPRINTF(GpuResourceEvent,
        "Resource: %lu, png [w: %d, h: %d] -> %s.\n",
        id, get_width(), get_height(), outputfile.str().c_str()
    );

    image.write(outputfile.str().c_str());
}

void
dump_jpeg_image_wrapper(
    const char *filename, uint8_t *data, size_t width, size_t height
);

void
GpuResource::dump_jpeg_image()
{

    if (mesa_can_unpack() == false) {
        //
        DPRINTF(GpuResourceEvent,
            "Resource: %lu, dump_jpeg_image -> Unsupported Resource Format [%s].\n",
            id, util_format_name(lpr.base.format)
        );

        return;
    }

    //
    auto &config = gltracesim::system->get_config();

    if (get_width() < config.get("min-picture-width", 16).asUInt64() ||
        get_height() < config.get("min-picture-height", 16).asUInt64()) {
        //
        DPRINTF(GpuResourceEvent,
            "Resource: %lu, dump_jpeg_image -> Unsupported diminsions [%lux%lu].\n",
            id, get_width(), get_height()
        );

        return;
    }

    std::stringstream outputfile;
    outputfile << gltracesim::system->get_output_dir() << "/"
               << "f" << gltracesim::system->get_frame_nbr() << "/"
               << "s" << gltracesim::system->get_scene_nbr() << "/"
               << "r" << id << ".jpeg";

    //
    png::rgb_pixel *data = new png::rgb_pixel[get_width() * get_height()];

    // In memory copy
    for (size_t y = 0; y < get_height(); ++y)
    {
        for (size_t x = 0; x < get_width(); ++x)
        {
            // Pixel row addr
            char* addr = (char*) (addr_range.start + y * lpr.row_stride[0]);
            // pixel col addr
            addr += x * util_format_get_blocksize(lpr.base.format);
            //
            uint8_t r, g, b, a = 0;
            //
            mesa_get_color(lpr.base.format, addr, r, g, b, a);
            //
            data[y * get_width() + x].red = r;
            data[y * get_width() + x].green = g;
            data[y * get_width() + x].blue = b;
        }
    }

    //
    dump_jpeg_image_wrapper(
        outputfile.str().c_str(), (uint8_t*) data, get_width(), get_height()
    );

    //
    DPRINTF(GpuResourceEvent,
        "Resource: %lu, jpeg [w: %d, h: %d] -> %s.\n",
        id, get_width(), get_height(), outputfile.str().c_str()
    );

    //
    delete data;
}

}
