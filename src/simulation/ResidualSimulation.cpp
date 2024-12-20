#include "simulation/ResidualSimulation.hpp"
#include "utils/MeshUtils.hpp"

#include "simobject/XPBDMeshObject.hpp"
#include "solver/XPBDSolver.hpp"

#include <regex>


ResidualSimulation::ResidualSimulation(const std::string& config_filename)
    : OutputSimulation(config_filename)
{
    _out_file << "Residual Simulation\n";
}

void ResidualSimulation::setup()
{
    Simulation::setup();

    _out_file << toString() << std::endl;

    for (auto& mesh_object : _mesh_objects) {
        if (ElasticMeshObject* elastic_mesh_object = dynamic_cast<ElasticMeshObject*>(mesh_object.get()))
        {
            _out_file << "\n" << elastic_mesh_object->toString() << std::endl;
        }
    }

    // write appropriate CSV column headers
    _out_file << "\nTime(s)";
    for (auto& mesh_object : _mesh_objects)
    {
        if (ElasticMeshObject* elastic_mesh_object = dynamic_cast<ElasticMeshObject*>(mesh_object.get()))
        {
            std::regex r("\\s+");
            const std::string& name = std::regex_replace(elastic_mesh_object->name(), r, "");
            _out_file << " "+name+"DynamicsResidual" << " "+name+"PrimaryResidual" << " "+name+"ConstraintResidual" << " "+name+"VolumeRatio";
        }
    }
    _out_file << std::endl;
    
    
    _last_print_sim_time = _time;
}

void ResidualSimulation::printInfo() const
{
    _out_file << _time;
    for (unsigned i = 0; i < _mesh_objects.size(); i++) {

        double dynamics_residual = 0;
        double primary_residual = 0;
        double constraint_residual = 0;
        double volume_ratio = 1;
        if (XPBDMeshObject* xpbd = dynamic_cast<XPBDMeshObject*>(_mesh_objects[i].get()))
        {
            Eigen::VectorXd pres_vec = xpbd->solver()->primaryResidual();
            primary_residual = std::sqrt(pres_vec.squaredNorm() / pres_vec.rows());
            Eigen::VectorXd cres_vec = xpbd->solver()->constraintResidual();
            constraint_residual = std::sqrt(cres_vec.squaredNorm() / cres_vec.rows());
            // constraint_residual = elastic_mesh_object->constraintResidual();
            // dynamics_residual = elastic_mesh_object->dynamicsResidual();
            // volume_ratio = elastic_mesh_object->volumeRatio();
            // std::cout << "Time: " << _time << std::endl;
            // std::cout << "\tDynamics residual: " << elastic_mesh_object->dynamicsResidual() << std::endl;
            // std::cout << "\tPrimary residual: " << elastic_mesh_object->primaryResidual() << std::endl;
            // std::cout << "\tConstraint residual: " << elastic_mesh_object->constraintResidual() << std::endl;
            // std::cout << "\tVolume ratio: " << elastic_mesh_object->volumeRatio() << std::endl;
        }
        else
        {
            continue;
        }
        _out_file << " " << dynamics_residual << " " << primary_residual << " " << constraint_residual << " " << volume_ratio;
        
    }
    _out_file << std::endl;
}