#ifndef __GLTRACESIM_RESOURCE_IMPL_HH__
#define __GLTRACESIM_RESOURCE_IMPL_HH__

#include "resource.hh"

namespace gltracesim {

inline size_t
GpuResource::size() const
{
    return addr_range.size();
}

inline size_t
GpuResource::get_blk_idx(uint64_t addr) const
{
    return (addr - addr_range.start) / blk_size;
}

inline size_t
GpuResource::get_width() const
{
    return lpr.base.width0;
}

inline size_t
GpuResource::get_height() const
{
    return lpr.base.height0;
}

inline uint64_t
GpuResource::get_addr(struct llvmpipe_resource *lpr)
{
    if (llvmpipe_resource_is_texture(&lpr->base)) {
        return uint64_t(lpr->tex_data);
    } else {
        return uint64_t(lpr->data);
    }
}

inline uint64_t
GpuResource::get_size(struct llvmpipe_resource *lpr)
{
    if (llvmpipe_resource_is_texture(&lpr->base)) {
        return lpr->total_alloc_size;
    } else {
        return lpr->base.width0;
    }
}

inline const char*
GpuResource::get_target_name() const {
    return target_names[lpr.base.target];
}

} // end namespace gltracesim

#endif // __GLTRACESIM_RESOURCE_IMPL_HH__
