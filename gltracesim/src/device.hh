#ifndef __GLTRACESIM_DEV_HH__
#define __GLTRACESIM_DEV_HH__

namespace gltracesim {
namespace dev {

/**
 * @brief The ThreadMode enum
 */
enum HardwareDevice {
    CPU = 0,
    GPU,
    NumHardwareDevices
};

extern const char* hardware_devices_names[];

inline const char*
get_dev_name(int dev) {
    return hardware_devices_names[dev];
}

} // end namespace dev
} // end namespace gltracesim

#endif // __GLTRACESIM_DEV_HH__
