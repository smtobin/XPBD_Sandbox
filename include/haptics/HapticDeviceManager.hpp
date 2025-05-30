#ifndef __HAPTIC_DEVICE_MANAGER_HPP
#define __HAPTIC_DEVICE_MANAGER_HPP

#include <HD/hd.h>
#include <HDU/hduError.h>
#include <HDU/hduVector.h>

#include "common/colors.hpp"
#include <iostream>
#include <cassert>
#include <mutex>
#include <optional>
#include <map>

#include "common/types.hpp"

struct HapticDeviceData
{
    HDboolean button1_state;
    HDboolean button2_state;
    hduVector3Dd device_position;
    HDdouble device_transform[16];
    HDErrorInfo error;

    HapticDeviceData()
        : button1_state(false), button2_state(false), device_position(), error()
    {}
};

struct CopiedHapticDeviceData
{
    Vec3r position;
    Mat3r orientation;
    bool button1_pressed;
    bool button2_pressed;
    bool stale;

    CopiedHapticDeviceData()
        : position(Vec3r::Zero()), orientation(Mat3r::Identity()), button1_pressed(false), button2_pressed(false), stale(false)
    {}
};

class HapticDeviceManager
{
    public:
    explicit HapticDeviceManager();
    explicit HapticDeviceManager(const std::string& device_name);
    explicit HapticDeviceManager(const std::string& device_name1, const std::string& device_name2);
    

    ~HapticDeviceManager();

    Vec3r position(HHD handle);
    Mat3r orientation(HHD handle);
    bool button1Pressed(HHD handle);
    bool button2Pressed(HHD handle);

    const std::vector<HHD>& deviceHandles() const { return _device_handles; }

    void setForce(HHD handle, const Eigen::Vector3d& force);
    const Vec3r& force(HHD handle) const { return _device_forces.at(handle); }

    private:
    static HDCallbackCode HDCALLBACK _updateCallback(void *data);

    void copyState(HHD handle);
    void setStale(const bool stale) { _stale = stale; }
    inline void setDeviceData(HHD handle, const HDboolean& b1_state, const HDboolean& b2_state, const hduVector3Dd& position, const HDdouble* transform);

    void _initDeviceWithName(std::optional<std::string> device_name);

    std::vector<HHD> _device_handles;
    std::map<HHD, HapticDeviceData> _device_data;
    std::map<HHD, CopiedHapticDeviceData> _copied_device_data;

    bool _stale;

    std::map<HHD, Vec3r> _device_forces;

    std::mutex _state_mtx;
};

#endif // __HAPTIC_DEVICE_MANAGER_HPP