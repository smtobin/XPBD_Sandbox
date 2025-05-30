#ifndef __ARRAY_GPU_RESOURCE_HPP
#define __ARRAY_GPU_RESOURCE_HPP

#include "gpu/resource/GPUResource.hpp"

#include <iostream>

namespace Sim
{

template <typename T>
class ArrayGPUResource : public HostReadableGPUResource
{
    public:
    explicit ArrayGPUResource(const T* arr, int num_elements)
        : _arr(arr), _num_elements(num_elements)
    {
    }

    ArrayGPUResource()
        : _arr(nullptr), _num_elements(0)
    {
    }

    virtual ~ArrayGPUResource()
    {
        cudaFree(_d_arr);
    }

    virtual void allocate() override
    {
        _arr_size = _num_elements * sizeof(T);
        // CHECK_CUDA_ERROR(cudaHostRegister(const_cast<T*>(_arr), _arr_size, cudaHostRegisterMapped));
        // CHECK_CUDA_ERROR(cudaHostGetDevicePointer((void**)&_d_arr, const_cast<T*>(_arr), 0));
        cudaMalloc((void**)&_d_arr, _arr_size);
    }

    virtual void fullCopyToDevice() const override
    {
        cudaMemcpy(_d_arr, _arr, _arr_size, cudaMemcpyHostToDevice);
    }

    virtual void partialCopyToDevice() const override
    {
        fullCopyToDevice();
    }

    T* gpuArr() const { return _d_arr; }

    private:
    const T* _arr;
    int _num_elements;
    size_t _arr_size;

    T* _d_arr;

};

} // namespace Sim

#endif // __ARRAY_GPU_RESOURCE_HPP