#ifndef __RIGID_MESH_OBJECT_CONFIG_HPP
#define __RIGID_MESH_OBJECT_CONFIG_HPP

#include "config/RigidObjectConfig.hpp"
#include "config/MeshObjectConfig.hpp"

class RigidMeshObjectConfig : public RigidObjectConfig, public MeshObjectConfig
{
    public:

    explicit RigidMeshObjectConfig(const YAML::Node& node)
        : RigidObjectConfig(node), MeshObjectConfig(node)
    {
        _extractParameter("sdf-filename", node, _sdf_filename);
    }

    explicit RigidMeshObjectConfig( const std::string& name, const Vec3r& initial_position, const Vec3r& initial_rotation,
                                    const Vec3r& initial_velocity, const Vec3r& initial_angular_velocity, Real density,
                                    bool collisions, bool graphics_only, bool fixed,
                                    const std::string& filename, const std::optional<Real>& max_size, const std::optional<Vec3r>& size,
                                    bool draw_points, bool draw_edges, bool draw_faces, const Vec4r& color,
                                    const std::optional<std::string>& sdf_filename )
        : RigidObjectConfig(name, initial_position, initial_rotation, initial_velocity, initial_angular_velocity, density, collisions, graphics_only, fixed),
          MeshObjectConfig(filename, max_size, size, draw_points, draw_edges, draw_faces, color)
    {
        _sdf_filename.value = sdf_filename;
    }

    std::optional<std::string> sdfFilename() const { return _sdf_filename.value; }

    protected:
    ConfigParameter<std::string> _sdf_filename;
};

#endif // __RIGID_MESH_OBJECT_CONFIG