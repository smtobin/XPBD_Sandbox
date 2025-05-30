#include "simulation/BeamSimulation.hpp"
#include "utils/MeshUtils.hpp"

#include "simobject/XPBDMeshObject.hpp"
#include "simobject/FirstOrderXPBDMeshObject.hpp"
#include "solver/xpbd_solver/XPBDSolver.hpp"

#include <regex>

namespace Sim
{

BeamSimulation::BeamSimulation(const BeamSimulationConfig* config)
    : OutputSimulation(config)
{
    _out_file << "Cantilever Beam Simulation\n";
}

void BeamSimulation::setup()
{
    // MeshUtils::createBeamObj("../resource/10x1x1_subdivided16_beam.obj", 10, 1, 1, 16);

    Simulation::setup();

    _out_file << toString(0) << std::endl;

    for (auto& obj : _objects) {
        if (XPBDMeshObject_Base* xpbd_mo = dynamic_cast<XPBDMeshObject_Base*>(obj.get()))
        {
            Geometry::AABB aabb = xpbd_mo->boundingBox();
            std::vector<int> vertices_to_fix = xpbd_mo->mesh()->getVerticesWithY(aabb.min[1]);
            for (const auto& v : vertices_to_fix)
                xpbd_mo->fixVertex(v);
            
            _out_file << xpbd_mo->toString(1) << std::endl;

            const Vec3r bbox_center = aabb.center();
            unsigned tip_vertex = xpbd_mo->mesh()->getClosestVertex( {bbox_center[0], aabb.max[1], bbox_center[2]} );
            _beams_tip_vertex.push_back(tip_vertex);
            _beams_tip_start.push_back(xpbd_mo->mesh()->vertex(tip_vertex));
        }
    }

    // write appropriate CSV column headers
    _out_file << "\nTime(s)";
    for (auto& obj : _objects)
    {
        if (XPBDMeshObject_Base* xpbd_mo = dynamic_cast<XPBDMeshObject_Base*>(obj.get()))
        {
            std::regex r("\\s+");
            const std::string& name = std::regex_replace(xpbd_mo->name(), r, "");
            _out_file << " "+name+"DeflectionX(m)" << " "+name+"DeflectionZ(m)" << " "+name+"DynamicsResidual" << " "+name+"PrimaryResidual" << " "+name+"ConstraintResidual" << " "+name+"VolumeRatio";
        }
    }
    _out_file << std::endl;
    
    if (_graphics_scene)
        _graphics_scene->setCameraOrthographic();
        
    _last_print_sim_time = _time;
}

void BeamSimulation::printInfo() const
{
    _out_file << _time;
    for (size_t i = 0; i < _objects.size(); i++) {

        if (XPBDMeshObject_Base* xpbd = dynamic_cast<XPBDMeshObject_Base*>(_objects[i].get()))
        {
            const Vec3r& beam_deflection = _beams_tip_start[i] - xpbd->mesh()->vertex(_beams_tip_vertex[i]);

            Real dynamics_residual = 0;
            Real primary_residual = 0;
            Real constraint_residual = 0;
            Real volume_ratio = 1;
        
            // TODO: get residuals from solver somehow
            
            // VecXr pres_vec = xpbd->solver()->primaryResidual();
            // primary_residual = std::sqrt(pres_vec.squaredNorm() / pres_vec.rows());
            // VecXr cres_vec = xpbd->solver()->constraintResidual();
            // constraint_residual = std::sqrt(cres_vec.squaredNorm() / cres_vec.rows());

            _out_file << " " << beam_deflection[0] << " " << beam_deflection[2] << " " << dynamics_residual << " " << primary_residual << " " << constraint_residual << " " << volume_ratio;
        }

        
        
    }
    _out_file << std::endl;
}

} // namespace Sim