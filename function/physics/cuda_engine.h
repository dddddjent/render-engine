#pragma once
#include "function/physics/physics_engine.h"
#include <cuda.h>
#include <cuda_runtime.h>
#include <string>
#include <unordered_map>
#ifdef _WIN64
#include <windows.h>
#endif

struct GlobalContext;

class CudaEngine : public PhysicsEngine {
public:
    struct ExtBufferDesc {
#ifdef _WIN64
        HANDLE handle;
#else
        int fd;
#endif
        size_t buffer_size;
        std::string name;
    };

    struct ExtImageDesc {
#ifdef _WIN64
        HANDLE handle;
#else
        int fd;
#endif
        size_t image_size;
        size_t element_size;
        size_t width;
        size_t height;
        size_t depth;
        std::string name;
    };

    struct ExtBuffer {
        cudaExternalMemory_t memory;
        void* ptr;
        size_t size;
        void cleanup()
        {
            cudaDestroyExternalMemory(memory);
        }
    };

    struct ExtImage {
        cudaExternalMemory_t memory;
        cudaMipmappedArray_t mipmapped_array;
        cudaSurfaceObject_t surface_object;
        cudaExtent extent;
        size_t size;
        size_t element_size;
        void cleanup()
        {
            cudaDestroyExternalMemory(memory);
            cudaFreeMipmappedArray(mipmapped_array);
            cudaDestroySurfaceObject(surface_object);
        }
    };

protected:
    GlobalContext* g_ctx;

    std::unordered_map<std::string, ExtBuffer> extBuffers;
    std::unordered_map<std::string, ExtImage> extImages;
    cudaExternalSemaphore_t cuUpdateSemaphore;
    cudaExternalSemaphore_t vkUpdateSemaphore;
    cudaStream_t streamToRun;

    int total_frame;
    int frame_rate;
    int steps_per_frame;
    int current_frame;

    void initSemaphore();
    void importExtBuffer(const ExtBufferDesc& buffer_desc);
    // single channel image only
    void importExtImage(const ExtImageDesc& image_desc);
    void waitOnSemaphore(cudaExternalSemaphore_t& semaphore);
    void signalSemaphore(cudaExternalSemaphore_t& semaphore);

    virtual void initExternalMem();

public:
    virtual void init(Configuration& config, GlobalContext* g_ctx) override;
    virtual void step() override;
    virtual void sync() override;
    virtual void cleanup() override;
};
