#include "collision/CollisionScene.hpp"
#include "simulation/Simulation.hpp"
#include "simobject/RigidPrimitives.hpp"
#include "simobject/MeshObject.hpp"
#include "simobject/XPBDMeshObject.hpp"
#include "simobject/VirtuosoArm.hpp"
#include "geometry/SphereSDF.hpp"
#include "geometry/BoxSDF.hpp"
#include "geometry/CylinderSDF.hpp"
#include "geometry/MeshSDF.hpp"
#include "geometry/Mesh.hpp"
#include "geometry/VirtuosoArmSDF.hpp"
#include "utils/GeometryUtils.hpp"

#ifdef HAVE_CUDA
#include "gpu/resource/GPUResource.hpp"
#include "gpu/resource/MeshGPUResource.hpp"
#include "gpu/Collision.cuh"
#endif

// namespace Collision
// {

CollisionScene::CollisionScene(const Sim::Simulation* sim)
    : _sim(sim)
{

}

void CollisionScene::addObject(Sim::Object* new_obj, const ObjectConfig* config)
{
    // create a SDF for the new object, if applicable
    // TODO: move this SDF creation process to RigidObject
    std::unique_ptr<Geometry::SDF> sdf = nullptr;
    if (Sim::RigidSphere* sphere = dynamic_cast<Sim::RigidSphere*>(new_obj))
    {
        sdf = std::make_unique<Geometry::SphereSDF>(sphere);
    }
    else if (Sim::RigidBox* box = dynamic_cast<Sim::RigidBox*>(new_obj))
    {
        sdf = std::make_unique<Geometry::BoxSDF>(box);
    }
    else if (Sim::RigidCylinder* cyl = dynamic_cast<Sim::RigidCylinder*>(new_obj))
    {
        sdf = std::make_unique<Geometry::CylinderSDF>(cyl);
    }
    else if (Sim::RigidMeshObject* mesh_obj = dynamic_cast<Sim::RigidMeshObject*>(new_obj))
    {
        if ( const RigidMeshObjectConfig* obj_config = dynamic_cast<const RigidMeshObjectConfig*>(config) )
        {
            sdf = std::make_unique<Geometry::MeshSDF>(mesh_obj, obj_config);
        }
        else
            sdf = std::make_unique<Geometry::MeshSDF>(mesh_obj, nullptr);
        
    }
    else if (Sim::VirtuosoArm* arm = dynamic_cast<Sim::VirtuosoArm*>(new_obj))
    {
        sdf = std::make_unique<Geometry::VirtuosoArmSDF>(arm);
    }

 #ifdef HAVE_CUDA
    if (sdf)
    {
        sdf->createGPUResource();
        sdf->gpuResource()->fullCopyToDevice();
    }

    if (Sim::XPBDMeshObject_Base* mesh_obj = dynamic_cast<Sim::XPBDMeshObject_Base*>(new_obj))
    {
        // create a managed resource for the mesh
        Geometry::Mesh* mesh_ptr = mesh_obj->mesh();
        mesh_ptr->createGPUResource();
        mesh_ptr->gpuResource()->fullCopyToDevice();
    
        // create a block of data of GPUCollision structs that will be populated during collision detection
        // at most, we will have one collision per face in the mesh, so to be safe this is the amount of memory we allocate
        std::vector<Sim::GPUCollision> collisions_vec(mesh_obj->mesh()->numFaces());

        // initialize the time for each collision slot to some negative number so that we can distinguish when there is an active collision
        for (auto& gc : collisions_vec)
        {
            gc.penetration_dist = 100;
        }

        // CHECK_CUDA_ERROR(cudaHostRegister(collisions_vec.data(), collisions_vec.size()*sizeof(Sim::GPUCollision), cudaHostRegisterDefault));

        // create the GPUResource for the array of collision structs
        std::unique_ptr<Sim::WritableArrayGPUResource<Sim::GPUCollision>> arr_resource = 
            std::make_unique<Sim::WritableArrayGPUResource<Sim::GPUCollision>>(collisions_vec.data(), mesh_obj->mesh()->numFaces());
        arr_resource->allocate();

        assert(_gpu_collisions.count(new_obj) == 0);

        // move the vector into the member map
        _gpu_collisions[new_obj] = std::move(collisions_vec);
        _gpu_collision_resources[new_obj] = std::move(arr_resource);
    }
 #endif

    // create a new collision object
    CollisionObject c_obj;
    c_obj.obj = new_obj;
    c_obj.sdf = std::move(sdf);

    _collision_objects.push_back(std::move(c_obj));
}

void CollisionScene::collideObjects()
{
    // collide object pairs
    for (size_t i = 0; i < _collision_objects.size(); i++)
    {
        for (size_t j = i+1; j < _collision_objects.size(); j++)
        {
            _collideObjectPair(_collision_objects[i], _collision_objects[j]);
        }
    }


}

void CollisionScene::_collideObjectPair(CollisionObject& c_obj1, CollisionObject& c_obj2)
{
    // TODO: should we generalize to all MeshObjects? And then each MeshObject have a virtual method createCollisionConstraintForFace()?
    // (in an effort to avoid dynamic_cast)
    
    // std::cout << "Colliding " << c_obj1.obj->name() << " and " << c_obj2.obj->name() << std::endl;
    const Geometry::SDF* sdf;
    const Geometry::Mesh* mesh;
    Sim::XPBDMeshObject_Base* xpbd_obj;
    Sim::RigidObject* rigid_obj;
    if (c_obj1.sdf && c_obj2.sdf)
    {
        // std::cout << "Can't collide 2 SDFs (yet)!" << std::endl;
        return;
    }

    if (!c_obj1.sdf && !c_obj2.sdf)
    {
        // std::cout << "At least one object must have an SDF!" << std::endl;
        return;
    }

    if (c_obj1.sdf)
    {
        sdf = c_obj1.sdf.get();

        Sim::MeshObject* mesh_obj = dynamic_cast<Sim::MeshObject*>(c_obj2.obj);
        assert(mesh_obj);
        mesh = mesh_obj->mesh();
        xpbd_obj = dynamic_cast<Sim::XPBDMeshObject_Base*>(mesh_obj);
        rigid_obj = dynamic_cast<Sim::RigidObject*>(c_obj1.obj);
        assert(xpbd_obj);
    } 
    else
    {
        sdf = c_obj2.sdf.get();

        Sim::MeshObject* mesh_obj = dynamic_cast<Sim::MeshObject*>(c_obj1.obj);
        assert(mesh_obj);
        mesh = mesh_obj->mesh();
        xpbd_obj = dynamic_cast<Sim::XPBDMeshObject_Base*>(mesh_obj);
        rigid_obj = dynamic_cast<Sim::RigidObject*>(c_obj2.obj);
        assert(xpbd_obj);
    }


 #ifdef HAVE_CUDA
    // get GPU resources associated with object
    const Sim::MeshGPUResource* mesh_resource = dynamic_cast<const Sim::MeshGPUResource*>(mesh->gpuResource());
    assert(mesh_resource);
    const Sim::HostReadableGPUResource* sdf_resource = sdf->gpuResource();
    // const std::vector<Sim::GPUCollision>& collisions = _gpu_collisions[xpbd_obj];
    Sim::WritableArrayGPUResource<Sim::GPUCollision>* arr_resource = _gpu_collision_resources[xpbd_obj].get();
    auto t1 = std::chrono::high_resolution_clock::now();
    // copy over memory so that they are up to date
    // TODO: do this asynchronously
    mesh_resource->partialCopyToDevice();
    sdf_resource->partialCopyToDevice();

    launchCollisionKernel(sdf_resource, mesh_resource, mesh->numVertices(), mesh->numFaces(), arr_resource);

    cudaDeviceSynchronize();

    arr_resource->copyFromDevice();

    auto t2 = std::chrono::high_resolution_clock::now();
    // std::cout << "GPU part took " << std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count() << " us\n";

    // Sim::GPUCollision* arr = arr_resource->arr();
    const std::vector<Sim::GPUCollision>& arr = _gpu_collisions[xpbd_obj];

    for (int i = 0; i < mesh->numFaces(); i++)
    {
        // std::cout << arr[i].penetration_dist << "\n";
        if (arr[i].penetration_dist < 0)
        {
            const Vec3i f = mesh->face(i);
            const Vec3r p = mesh->vertex(f[0])*arr[i].bary_coords.x + mesh->vertex(f[1])*arr[i].bary_coords.y + mesh->vertex(f[2])*arr[i].bary_coords.z;
            const Vec3r normal = sdf->gradient(p);
            const Vec3r surface_point = p - normal * arr[i].penetration_dist;
            // const Vec3r surface_point(arr[i].surface_point.x, arr[i].surface_point.y, arr[i].surface_point.z);
            // const Vec3r normal(arr[i].normal.x, arr[i].normal.y, arr[i].normal.z);
            // std::cout << "normal: " << normal[0] << ", " << normal[1] << ", " << normal[2] << std::endl;
            // std::cout << "surface point: " << surface_point[0] << ", " << surface_point[1] << ", " << surface_point[2] << std::endl;
            if (rigid_obj->isFixed())
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_point, normal, i, arr[i].bary_coords.x, arr[i].bary_coords.y, arr[i].bary_coords.z);
            }
            else
            {
                xpbd_obj->addRigidDeformableCollisionConstraint(sdf, rigid_obj, surface_point, normal, i, arr[i].bary_coords.x, arr[i].bary_coords.y, arr[i].bary_coords.z);
            }
        }
    }

 #else
    // iterate through faces of mesh
    const Geometry::Mesh::FacesMat& faces = mesh->faces();
    for (int i = 0; i < faces.cols(); i++)
    {
        const Eigen::Vector3i& f = faces.col(i);
        const Vec3r& p1 = mesh->vertex(f[0]);
        const Vec3r& p2 = mesh->vertex(f[1]);
        const Vec3r& p3 = mesh->vertex(f[2]);
        // std::cout << "v1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << "\tdist: " << sdf->evaluate(p1) << std::endl;
        // std::cout << "v2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << "\tdist: " << sdf->evaluate(p2) << std::endl;
        // std::cout << "v3: " << p3[0] << ", " << p3[1] << ", " << p3[2] << "\tdist: " << sdf->evaluate(p3) << std::endl;
        const Vec3r x = _frankWolfe(sdf, p1, p2, p3);
        const double distance = sdf->evaluate(x);
        if (distance <= 1e-4)
        {// collision occurred, find barycentric coordinates (u,v,w) of x on triangle face
            // from https://ceng2.ktu.edu.tr/~cakir/files/grafikler/Texture_Mapping.pdf
            const auto [u, v, w] = GeometryUtils::barycentricCoords(x, p1, p2, p3);
            const Vec3r grad = sdf->gradient(x);
            const Vec3r surface_x = x - grad*distance;
            // std::cout << "COLLISION!" << std::endl;
            // std::cout << "u: " << u << ", v: " << v << ", w: " << w << std::endl;
            // std::cout << "position: " << x[0] << ", " << x[1] << ", " << x[2] << "\tnormal: " << grad[0] << ", " << grad[1] << ", " << grad[2] << std::endl;
            // std::cout << "surface position: " << surface_x[0] << ", " << surface_x[1] << ", " << surface_x[2] << std::endl;
        
            if (!rigid_obj)
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else if (rigid_obj->isFixed())
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else
            {
                xpbd_obj->addRigidDeformableCollisionConstraint(sdf, rigid_obj, surface_x, grad, i, u, v, w);
            }
            
        }

        // TODO: check each vertex in the mesh separately instead of inside the faces loop
        const double distance_p1 = sdf->evaluate(p1);
        if (distance_p1 <= 1e-4)
        {
            const auto [u, v, w] = GeometryUtils::barycentricCoords(p1, p1, p2, p3);
            const Eigen::Vector3d grad = sdf->gradient(p1);
            const Eigen::Vector3d surface_x = p1 - grad*distance;

            if (!rigid_obj)
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else if (rigid_obj->isFixed())
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else
            {
                xpbd_obj->addRigidDeformableCollisionConstraint(sdf, rigid_obj, surface_x, grad, i, u, v, w);
            }
        }

        const double distance_p2 = sdf->evaluate(p2);
        if (distance_p2 <= 1e-4)
        {
            const auto [u, v, w] = GeometryUtils::barycentricCoords(p2, p1, p2, p3);
            const Eigen::Vector3d grad = sdf->gradient(p2);
            const Eigen::Vector3d surface_x = p2 - grad*distance;

            if (!rigid_obj)
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else if (rigid_obj->isFixed())
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else
            {
                xpbd_obj->addRigidDeformableCollisionConstraint(sdf, rigid_obj, surface_x, grad, i, u, v, w);
            }
        }

        const double distance_p3 = sdf->evaluate(p3);
        if (distance_p3 <= 1e-4)
        {
            const auto [u, v, w] = GeometryUtils::barycentricCoords(p3, p1, p2, p3);
            const Eigen::Vector3d grad = sdf->gradient(p3);
            const Eigen::Vector3d surface_x = p3 - grad*distance;

            if (!rigid_obj)
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else if (rigid_obj->isFixed())
            {
                xpbd_obj->addStaticCollisionConstraint(sdf, surface_x, grad, i, u, v, w);
            }
            else
            {
                xpbd_obj->addRigidDeformableCollisionConstraint(sdf, rigid_obj, surface_x, grad, i, u, v, w);
            }
        }
    }
 #endif
}

Vec3r CollisionScene::_frankWolfe(const Geometry::SDF* sdf, const Vec3r& p1, const Vec3r& p2, const Vec3r& p3) const
{
    // find starting iterate - the triangle vertex with the smallest value of SDF
    const double d_p1 = sdf->evaluate(p1);
    const double d_p2 = sdf->evaluate(p2);
    const double d_p3 = sdf->evaluate(p3);

    // std::cout << "p1: " << p1[0] << ", " << p1[1] << ", " << p1[2] << std::endl;
    // std::cout << "p2: " << p2[0] << ", " << p2[1] << ", " << p2[2] << std::endl;
    // std::cout << "p3: " << p3[0] << ", " << p3[1] << ", " << p3[2] << std::endl;

    // special case: face-face collision, edge-face collision - not sure if this is needed
    // bool e1 = std::abs(d_p1 - d_p2) < 1e-8;
    // bool e2 = std::abs(d_p2 - d_p3) < 1e-8;
    // bool e3 = std::abs(d_p3 - d_p1) < 1e-8;
    // if (e1 && e2)
    // {
    //     return 0.3333333*p1 + 0.3333333*p2 + 0.3333333*p3;
    // }
    // else if (e1)
    // {
    //     return 0.5*p1 + 0.5*p2;
    // }
    // else if (e2)
    // {
    //     return 0.5*p2 + 0.5*p3;
    // }
    // else if (e3)
    // {
    //     return 0.5*p1 + 0.5*p3;
    // }

    Vec3r x;
    if (d_p1 <= d_p2 && d_p1 <= d_p3)       x = p1;
    else if (d_p2 <= d_p1 && d_p2 <= d_p3)  x = p2;
    else                                    x = p3;

    Vec3r s;
    for (int i = 0; i < 32; i++)
    {
        const double alpha = 2.0/(i+3);
        const Vec3r& gradient = sdf->gradient(x);
        // std::cout << "x: " << x[0] << ", " << x[1] << ", " << x[2] << std::endl;
        // std::cout << "gradient: " << gradient[0] << ", " << gradient[1] << ", " << gradient[2] << std::endl;
        const double sg1 = p1.dot(gradient);
        const double sg2 = p2.dot(gradient);
        const double sg3 = p3.dot(gradient);

        if (sg1 < sg2 && sg1 < sg3)       s = p1;
        else if (sg2 < sg1 && sg2 < sg3)  s = p2;
        else                                s = p3;

        x = x + alpha * (s - x);
        
    }

    return x;
}

// } // namespace Collision