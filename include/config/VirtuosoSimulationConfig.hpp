#ifndef __VIRTUOSO_SIMULATION_CONFIG_HPP
#define __VIRTUOSO_SIMULATION_CONFIG_HPP

#include "config/SimulationConfig.hpp"

enum SimulationInputDevice
{
    MOUSE,
    KEYBOARD,
    HAPTIC
};

class VirtuosoSimulationConfig : public SimulationConfig
{
    /** Predefined default for input device */
    static std::optional<SimulationInputDevice>& DEFAULT_INPUT_DEVICE() { static std::optional<SimulationInputDevice> input_device(SimulationInputDevice::KEYBOARD); return input_device; }

    static std::map<std::string, SimulationInputDevice>& INPUT_DEVICE_OPTIONS() 
    {
        static std::map<std::string, SimulationInputDevice> input_device_options{{"Mouse", SimulationInputDevice::MOUSE},
                                                                                 {"Keyboard", SimulationInputDevice::KEYBOARD},
                                                                                 {"Haptic", SimulationInputDevice::HAPTIC}};
        return input_device_options;
    }

    public:
    explicit VirtuosoSimulationConfig(const YAML::Node& node)
        : SimulationConfig(node)
    {
        _extractParameterWithOptions("input-device", node, _input_device, INPUT_DEVICE_OPTIONS(), DEFAULT_INPUT_DEVICE());
    }

    SimulationInputDevice inputDevice() const { return _input_device.value.value(); }

    protected:
    ConfigParameter<SimulationInputDevice> _input_device;
};


#endif // __VIRTUOSO_SIMULATION_CONFIG_HPP