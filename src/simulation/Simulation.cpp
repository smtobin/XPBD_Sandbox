#include "simulation/Simulation.hpp"
#include "config/RigidMeshObjectConfig.hpp"
#include "config/XPBDMeshObjectConfig.hpp"
#include "config/FirstOrderXPBDMeshObjectConfig.hpp"
#include "config/RigidPrimitiveConfigs.hpp"
#include "config/VirtuosoArmConfig.hpp"
#include "config/VirtuosoRobotConfig.hpp"

#include "graphics/Easy3DGraphicsScene.hpp"

#include "simobject/RigidMeshObject.hpp"
#include "simobject/XPBDMeshObject.hpp"
#include "simobject/FirstOrderXPBDMeshObject.hpp"
#include "simobject/RigidPrimitives.hpp"
#include "simobject/VirtuosoArm.hpp"
#include "simobject/VirtuosoRobot.hpp"

#include "simobject/XPBDObjectFactory.hpp"

#include "utils/MeshUtils.hpp"

#include <gmsh.h>

namespace Sim
{


Simulation::Simulation(const SimulationConfig* config)
    : _setup(false), _config(config)
{
    // initialize gmsh
    gmsh::initialize();

    // set simulation properties based on YAML file
    _name = _config->name();
    _description = _config->description().value();
    _time_step = _config->timeStep().value();
    _end_time = _config->endTime().value();
    _time = 0;
    _g_accel = _config->gAccel().value();
    _viewer_refresh_time = 1/_config->fps().value()*1000;
    _time_between_collision_checks = 1.0/_config->collisionRate().value();

    // set the Simulation mode from the YAML config
    _sim_mode = _config->simMode().value();

    // initialize the graphics scene according to the type specified by the user
    // if "None", don't create a graphics scene
    if (_config->visualization().value() == Visualization::EASY3D)
    {
        _graphics_scene = std::make_unique<Graphics::Easy3DGraphicsScene>("main");
    }

    // initialize the collision scene
    // _collision_scene = std::make_unique<CollisionScene>(1.0/_config->fps().value(), 0.05, 10007);
    _collision_scene = std::make_unique<CollisionScene>(this);
    _last_collision_detection_time = 0;
}

Simulation::Simulation()
{

}

std::string Simulation::toString(const int indent) const
{
    std::string indent_str(indent, '\t');
    std::stringstream ss;
    ss << indent_str << "=====" << type() << " '" << _name << "'=====" << std::endl;
    ss << indent_str << "Time step: " << _time_step << " s" << std::endl;
    ss << indent_str << "End time: " << _end_time << " s" << std::endl;
    ss << indent_str << "Gravity: " << _g_accel << " m/s2" << std::endl;
    return  ss.str();
}

Object* Simulation::_addObjectFromConfig(const ObjectConfig* obj_config)
{
    std::unique_ptr<Object> new_obj;
    // try downcasting
    // TODO: fix this to remove dynamic_cast! maybe move creation to Config class? Idk
    if (const FirstOrderXPBDMeshObjectConfig* xpbd_config = dynamic_cast<const FirstOrderXPBDMeshObjectConfig*>(obj_config))
    {
        // new_obj = std::make_unique<FirstOrderXPBDMeshObject>(this, xpbd_config);
        new_obj = XPBDObjectFactory::createFirstOrderXPBDMeshObject(this, xpbd_config);
    }
    else if (const XPBDMeshObjectConfig* xpbd_config = dynamic_cast<const XPBDMeshObjectConfig*>(obj_config))
    {
        // new_obj = std::make_unique<XPBDMeshObject>(this, xpbd_config);
        new_obj = XPBDObjectFactory::createXPBDMeshObject(this, xpbd_config);
    }
    else if (const RigidMeshObjectConfig* rigid_config = dynamic_cast<const RigidMeshObjectConfig*>(obj_config))
    {
        new_obj = std::make_unique<RigidMeshObject>(this, rigid_config);
    }
    else if (const RigidSphereConfig* rigid_config = dynamic_cast<const RigidSphereConfig*>(obj_config))
    {
        new_obj = std::make_unique<RigidSphere>(this, rigid_config);
    }
    else if (const RigidBoxConfig* rigid_config = dynamic_cast<const RigidBoxConfig*>(obj_config))
    {
        new_obj = std::make_unique<RigidBox>(this, rigid_config);
    }
    else if (const RigidCylinderConfig* rigid_config = dynamic_cast<const RigidCylinderConfig*>(obj_config))
    {
        new_obj = std::make_unique<RigidCylinder>(this, rigid_config);
    }
    else if (const VirtuosoArmConfig* virtuoso_config = dynamic_cast<const VirtuosoArmConfig*>(obj_config))
    {
        new_obj = std::make_unique<VirtuosoArm>(this, virtuoso_config);
    }
    else if (const VirtuosoRobotConfig* virtuoso_config = dynamic_cast<const VirtuosoRobotConfig*>(obj_config))
    {
        new_obj = std::make_unique<VirtuosoRobot>(this, virtuoso_config);
    }
    else
    {
        // downcasting failed for some reason, halt
        std::cerr << "Unknown config type!" << std::endl;
        assert(0);
    }

    // set up the new object
    new_obj->setup();
    
    // add the new object to the collision scene if collisions are enabled
    if (obj_config->collisions() && !obj_config->graphicsOnly())
    {
        _collision_scene->addObject(new_obj.get(), obj_config);
    }
    // add the new object to the graphics scene to be visualized
    if (_graphics_scene)
    {
        _graphics_scene->addObject(new_obj.get(), obj_config);
    }

    // if we get to here, we have successfully created a new MeshObject of some kind
    // so add the new object to the simulation
    if (obj_config->graphicsOnly())
    {
        _graphics_only_objects.push_back(std::move(new_obj));
        return _graphics_only_objects.back().get();
    }
        
    else
    {
        _objects.push_back(std::move(new_obj));
        return _objects.back().get();   
    }
        
    
}

void Simulation::setup()
{   
    // make sure we haven't set up already before
    assert(!_setup);

    _setup = true;


    /** Configure graphics scene... */ 
    if (_graphics_scene)
    {
        _graphics_scene->init();
        _graphics_scene->viewer()->registerSimulation(this);
        // add text that displays the current Sim Time  
        _graphics_scene->viewer()->addText("time", "Sim Time: 0.000 s", 10.0f, 10.0f, 15.0f, Graphics::Viewer::TextAlignment::LEFT, Graphics::Viewer::Font::MAO, std::array<float,3>({0,0,0}), 0.5f, false);
    }

    /** Add simulation objects from Config object... */
    for (const auto& obj_config : _config->objectConfigs())
    {
        _addObjectFromConfig(obj_config.get());
    }
        

    
}

void Simulation::update()
{
    auto start = std::chrono::steady_clock::now();

    // the start time in wall clock time of the simulation
    auto wall_time_start = std::chrono::steady_clock::now();
    // the wall time of the last viewer redraw
    auto last_redraw = std::chrono::steady_clock::now();

    // loop until end time is reached
    while(_time < _end_time)
    {
        // the elapsed seconds in wall time since the simulation has started
        Real wall_time_elapsed_s = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - wall_time_start).count() / 1000000000.0;
        
        // check if any callbacks need to be called
        for (auto& cb : _callbacks)
        {
            if (wall_time_elapsed_s > cb.next_exec_time)
            {
                cb.callback();
                cb.next_exec_time = cb.next_exec_time + cb.interval;
            }
        }

        // if the simulation is ahead of the current elapsed wall time, stall
        if (_sim_mode == SimulationMode::VISUALIZATION && _time > wall_time_elapsed_s)
        {
            continue;
        }

        _timeStep();

        // the time in ms since the viewer was last redrawn
        auto time_since_last_redraw_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - last_redraw).count();
        // we want ~30 fps, so update the viewer every 33 ms
        if (time_since_last_redraw_ms > _viewer_refresh_time)
        {
            // std::cout << _haptic_device_manager->getPosition() << std::endl;
            _updateGraphics();

            last_redraw = std::chrono::steady_clock::now();
        }
        
    }

    // one final redraw of final state
    _updateGraphics();

    auto end = std::chrono::steady_clock::now();
    std::cout << "Simulating " << _end_time << " seconds took " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
}

void Simulation::_timeStep()
{
    // auto t1 = std::chrono::steady_clock::now();

    if (_time - _last_collision_detection_time > _time_between_collision_checks)
    {
        // run collision detection
        auto t1 = std::chrono::steady_clock::now();
        for (auto& obj : _objects)
        {
            if (XPBDMeshObject_Base* xpbd_obj = dynamic_cast<XPBDMeshObject_Base*>(obj.get()))
                xpbd_obj->clearCollisionConstraints();
        }
        _collision_scene->collideObjects();
        auto t2 = std::chrono::steady_clock::now();
        // std::cout << "Collision detection took " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us\n";

        
    }

    for (auto& obj : _objects)
    {
        obj->update();
    }

    // update each object's velocities
    for (auto& obj : _objects)
    {
        obj->velocityUpdate();
    }

    if (_time - _last_collision_detection_time > _time_between_collision_checks)
    {
        // _collision_scene->updatePrevPositions();
        _last_collision_detection_time = _time;
    }
    
    // increment the time by the time step
    _time += _time_step;

    // auto t2 = std::chrono::steady_clock::now();
    // std::cout << "Time step took " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us" << std::endl;
}

void Simulation::_updateGraphics()
{
    if (_graphics_scene)
    {
        _graphics_scene->update();
    }
    // update the sim time text
    if (_graphics_scene)
    {
        _graphics_scene->viewer()->editText("time", "Sim Time: " + std::to_string(_time) + " s");
    }
}

void Simulation::notifyKeyPressed(int /* key */, int action, int /* modifiers */)
{
    // action = 0 ==> key up event
    // action = 1 ==> key down event
    // action = 2 ==> key hold event
    
    // if key is pressed down or held, we want to time step
    if (_sim_mode == SimulationMode::FRAME_BY_FRAME && action > 0)
    {
        _timeStep();
        _updateGraphics();
    }
}

void Simulation::notifyMouseButtonPressed(int /* button */, int /* action */, int /* modifiers */)
{
    // button = 0 ==> left mouse button
    // button = 1 ==> right mouse button
    // action = 0 ==> mouse up
    // action = 1 ==> mouse down
    
    // do nothing
}

void Simulation::notifyMouseMoved(double /* x */, double /* y */)
{
    // do nothing
}

void Simulation::notifyMouseScrolled(double /* dx */, double /* dy */)
{
    // do nothing
}

int Simulation::run()
{
    // first, setup if we haven't done so already
    if (!_setup)
        setup();

    // spwan the update thread
    std::thread update_thread;
    if (_sim_mode != SimulationMode::FRAME_BY_FRAME)
    {
        update_thread = std::thread(&Simulation::update, this);
    }

    // run the Viewer
    // _viewer->fit_screen();
    // return _viewer->run();
    if (_graphics_scene)
    {
        _graphics_scene->run();
        return 0;
    }
    else
    {
        update_thread.join();
        return 0;
    }
    
}

} // namespace Sim