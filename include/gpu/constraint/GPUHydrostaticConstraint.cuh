#ifndef __GPU_HYDROSTATIC_CONSTRAINT_CUH
#define __GPU_HYDROSTATIC_CONSTRAINT_CUH

#include "gpu/common/gcc_hostdevice.hpp"
#include "gpu/common/helper.cuh"
#include "gpu/constraint/GPUPositionReference.cuh"

#include "utils/CudaHelperMath.h"

struct GPUHydrostaticConstraint
{
    GPUPositionReference positions[4];
    float Q[9];
    float alpha;
    float gamma;

    // __host__ GPUHydrostaticConstraint(const Sim::XPBDMeshObject_Base* xpbd_obj, int v0, int v1, int v2, int v3)
    // {
    //     // float* gpu_vertices = xpbd_obj->gpuResource()->meshGpuResource().gpuVertices();
    //     // positions[0].ptr = gpu_vertices + v0*3;
    //     positions[0].inv_mass = 1 / xpbd_obj->vertexMass(v0);
    //     positions[0].index = v0;

    //     // positions[1].ptr = gpu_vertices + v1*3;
    //     positions[1].inv_mass = 1 / xpbd_obj->vertexMass(v1);
    //     positions[1].index = v1;

    //     // positions[2].ptr = gpu_vertices + v2*3;
    //     positions[2].inv_mass = 1 / xpbd_obj->vertexMass(v2);
    //     positions[2].index = v2;

    //     // positions[3].ptr = gpu_vertices + v3*3;
    //     positions[3].inv_mass = 1 / xpbd_obj->vertexMass(v3);
    //     positions[3].index = v3;

    //     // compute Q and rest volume
    //     const Geometry::Mesh* mesh = xpbd_obj->mesh();
    //     float rest_volume;
    //     computeQandVolume(mesh->vertex(v0), mesh->vertex(v1), mesh->vertex(v2), mesh->vertex(v3), Q, &rest_volume);

    //     alpha = 1/(xpbd_obj->material().lambda() * rest_volume);            // set alpha after the ElementConstraint constructor because we need the element volume
    //     gamma = xpbd_obj->material().mu() / xpbd_obj->material().lambda(); 
    // }

    __host__ GPUHydrostaticConstraint(int v0_ind, float inv_m0,
                                      int v1_ind, float inv_m1,
                                      int v2_ind, float inv_m2,
                                      int v3_ind, float inv_m3,
                                      const Eigen::Matrix3f& Q_,
                                      float alpha_, float gamma_)
    {
        positions[0].inv_mass = inv_m0;
        positions[0].index = v0_ind;

        positions[1].inv_mass = inv_m1;
        positions[1].index = v1_ind;

        positions[2].inv_mass = inv_m2;
        positions[2].index = v2_ind;

        positions[3].inv_mass = inv_m3;
        positions[3].index = v3_ind;

        for (int i = 0; i < 9; i++) { Q[i] = Q_.data()[i]; }

        alpha = alpha_;
        gamma = gamma_;
    }

    __device__ GPUHydrostaticConstraint() {}

    constexpr __host__ __device__ static int numPositions() { return 4; }

    constexpr __host__ __device__ static bool isInequality() { return false; }

    // __device__ void _loadVertices(float* x)
    // {
    //     x[0] = positions[0].ptr[0];  x[1] = positions[0].ptr[1];  x[2] = positions[0].ptr[2];
    //     x[3] = positions[1].ptr[0];  x[4] = positions[1].ptr[1];  x[5] = positions[1].ptr[2];
    //     x[6] = positions[2].ptr[0];  x[7] = positions[2].ptr[1];  x[8] = positions[2].ptr[2];
    //     x[9] = positions[3].ptr[0];  x[10] = positions[3].ptr[1]; x[11] = positions[3].ptr[2];
    // }
    __device__ void _loadVertices(const float* vertices, float* x)
    {
        *reinterpret_cast<float3*>(x)   = *reinterpret_cast<const float3*>(vertices + 3*positions[0].index);
        *reinterpret_cast<float3*>(x+3) = *reinterpret_cast<const float3*>(vertices + 3*positions[1].index);
        *reinterpret_cast<float3*>(x+6) = *reinterpret_cast<const float3*>(vertices + 3*positions[2].index);
        *reinterpret_cast<float3*>(x+9) = *reinterpret_cast<const float3*>(vertices + 3*positions[3].index);
    }

    __device__ void evaluate(const float* vertices, float* C)
    {
        float x[12];
        _loadVertices(vertices, x);

        float F[9];
        computeF(x, Q, F);

        _evaluate(C, F);
        
    }

    __device__ void _evaluate(float* C, float* F)
    {
        *C = F[0]*F[4]*F[8] - F[0]*F[7]*F[5] - F[3]*F[1]*F[8] + F[3]*F[7]*F[2] + F[6]*F[1]*F[5] - F[6]*F[4]*F[2] - (1+gamma);
    }

    __device__ void gradient(const float* vertices, float* delC)
    {
        float x[12];
        _loadVertices(vertices, x);

        float F[9];
        computeF(x, Q, F);

        _gradient(delC, F);
    }

    __device__ void _gradient(float* delC, float* F)
    {   
        float F_cross[9];
        Vec3Cross(F+3, F+6, F_cross);   // 2nd column of F crossed with 3rd column
        Vec3Cross(F+6, F, F_cross+3);   // 3rd column of F crossed with 1st column
        Vec3Cross(F, F+3, F_cross+6);   // 1st column of F crossed with 2nd column

        Mat3MulTranspose(F_cross, Q, delC);
        delC[9]  = -delC[0] - delC[3] - delC[6];
        delC[10] = -delC[1] - delC[4] - delC[7];
        delC[11] = -delC[2] - delC[5] - delC[8];

        // printf("delC: %f, %f, %f\n\t%f, %f, %f\n\t%f, %f, %f\n", delC[0], delC[1], delC[2], delC[3], delC[4], delC[5], delC[6], delC[7], delC[8]);
    }

    __device__ void evaluateWithGradient(const float* vertices, float* C, float* delC)
    {
        float x[12];
        _loadVertices(vertices, x);

        // printf("x0: %f, %f, %f\n", x[0], x[1], x[2]);
        // printf("x1: %f, %f, %f\n", x[3], x[4], x[5]);
        // printf("x2: %f, %f, %f\n", x[6], x[7], x[8]);
        // printf("x3: %f, %f, %f\n", x[9], x[10], x[11]);

        float F[9];
        computeF(x, Q, F);

        _evaluate(C, F);
        _gradient(delC, F);
    }
};

#endif // __GPU_HYDROSTATIC_CONSTRAINT_CUH