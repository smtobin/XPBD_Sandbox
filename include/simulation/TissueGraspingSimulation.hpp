#ifndef __TISSUE_GRASPING_SIMULATION_HPP
#define __TISSUE_GRASPING_SIMULATION_HPP

#include "simulation/OutputSimulation.hpp"
#include "haptics/HapticDeviceManager.hpp"
#include "config/TissueGraspingSimulationConfig.hpp"
#include "simobject/XPBDMeshObject.hpp"
#include "simobject/RigidMeshObject.hpp"
#include "common/types.hpp"

namespace Sim
{

class TissueGraspingSimulation : public Simulation
{
    public:

    explicit TissueGraspingSimulation(const std::string& config_filename);

    virtual std::string type() const override { return "TissueGraspingSimulation"; }

    virtual void setup() override;

    // virtual void printInfo() const override;

    virtual void notifyMouseButtonPressed(int button, int action, int modifiers) override;

    virtual void notifyMouseMoved(Real x, Real y) override;

    /** Notifies the simulation that a key has been pressed in the viewer.
         * @param key : the key that was pressed
         * @param action : the action performed on the keyboard
         * @param modifiers : the modifiers (i.e. Shift, Ctrl, Alt)
         */
    virtual void notifyKeyPressed(int key, int action, int modifiers) override;

    protected:

    void _timeStep() override;
    void _updateGraphics() override;
    void _toggleTissueGrasping();
    std::set<unsigned> _getAllVerticesInGraspingArea();

    Vec3r _transformInputPosition(const Vec3r& input_position);

    Real _grasp_size;
    Real _z_scaling;
    SimulationInputDevice _input_device;
    std::optional<std::string> _fixed_faces_filename;

    double _last_mesh_write_time;
    bool _write_mesh;

    bool _grasping;
    Eigen::Vector2d _mouse_pos_2d;
    Vec3r _mouse_pos_3d;
    XPBDMeshObject_Base* _tissue_obj;

    std::shared_ptr<RigidMeshObject> _grasp_tip;
    Vec3r _grasp_tip_position;
    Mat3r _grasp_tip_rotation;

    /** Manages haptic device(s) */
    std::unique_ptr<HapticDeviceManager> _haptic_device_manager;
};

} // namespace Sim

#endif // __TISSUE_GRASPING_SIMULATION_HPP