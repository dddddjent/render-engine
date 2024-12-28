#include "core/config/config.h"
#include "core/vulkan/vulkan_context.h"
#include "cuda_engine.h"
#include "function/global_context.h"
#include <cassert>
// #include "shared_data.h"

void CudaEngine::importExtBuffer(const ExtBufferDesc& buffer_desc)
{
    cudaExternalMemoryHandleDesc externalMemoryDesc = {};
    {
#ifdef _WIN64
        externalMemoryDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
        externalMemoryDesc.handle.win32.handle = buffer_desc.handle; // File descriptor from Vulkan
#else
        externalMemoryDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        externalMemoryDesc.handle.fd = buffer_desc.fd; // File descriptor from Vulkan
#endif

        externalMemoryDesc.size = buffer_desc.buffer_size;
    }
    cudaExternalMemory_t ext_mem;
    cudaImportExternalMemory(&ext_mem, &externalMemoryDesc);

    cudaExternalMemoryBufferDesc bufferDesc = {};
    {
        bufferDesc.offset = 0;
        bufferDesc.size = buffer_desc.buffer_size;
    }
    void* dev_ptr;
    cudaExternalMemoryGetMappedBuffer(&dev_ptr, ext_mem, &bufferDesc);

    extBuffers[buffer_desc.name] = {
        ext_mem,
        dev_ptr,
        buffer_desc.buffer_size
    };
}

void CudaEngine::importExtImage(const ExtImageDesc& image_desc)
{
    assert(image_desc.width * image_desc.height * image_desc.depth
        == image_desc.image_size / image_desc.element_size);

    cudaExternalMemoryHandleDesc externalMemoryDesc = {};
    {
#ifdef _WIN64
        externalMemoryDesc.type = cudaExternalMemoryHandleTypeOpaqueWin32;
        externalMemoryDesc.handle.win32.handle = image_desc.handle; // File descriptor from Vulkan
#else
        externalMemoryDesc.type = cudaExternalMemoryHandleTypeOpaqueFd;
        externalMemoryDesc.handle.fd = image_desc.fd; // File descriptor from Vulkan
#endif
    }
    externalMemoryDesc.size = image_desc.image_size;
    cudaExternalMemory_t ext_mem;
    cudaImportExternalMemory(&ext_mem, &externalMemoryDesc);

    cudaExtent extent = make_cudaExtent(image_desc.width, image_desc.height, image_desc.depth);
    cudaChannelFormatDesc formatDesc;
    {
        formatDesc.x = image_desc.element_size * 8;
        formatDesc.y = 0;
        formatDesc.z = 0;
        formatDesc.w = 0;
        formatDesc.f = cudaChannelFormatKindFloat;
    }
    cudaExternalMemoryMipmappedArrayDesc ext_mipmapped_arr_desc;
    {
        memset(&ext_mipmapped_arr_desc, 0, sizeof(ext_mipmapped_arr_desc));
        ext_mipmapped_arr_desc.offset = 0;
        ext_mipmapped_arr_desc.formatDesc = formatDesc;
        ext_mipmapped_arr_desc.extent = extent;
        ext_mipmapped_arr_desc.flags = 0;
        ext_mipmapped_arr_desc.numLevels = 1;
    }
    cudaMipmappedArray_t mipmapped_arr;
    cudaExternalMemoryGetMappedMipmappedArray(&mipmapped_arr, ext_mem, &ext_mipmapped_arr_desc);

    cudaArray_t arr_0;
    cudaGetMipmappedArrayLevel(&arr_0, mipmapped_arr, 0);

    cudaResourceDesc res_desc = {};
    {
        res_desc.resType = cudaResourceTypeArray;
        res_desc.res.array.array = arr_0;
    }
    cudaSurfaceObject_t surface_object;
    cudaCreateSurfaceObject(&surface_object, &res_desc);

    extImages[image_desc.name] = {
        ext_mem,
        mipmapped_arr,
        surface_object,
        extent,
        image_desc.image_size,
        image_desc.element_size
    };
}

void CudaEngine::initExternalMem()
{
}

void CudaEngine::initSemaphore()
{
    cudaExternalSemaphoreHandleDesc externalSemaphoreHandleDesc;
    memset(&externalSemaphoreHandleDesc, 0, sizeof(externalSemaphoreHandleDesc));
#ifdef _WIN64
    externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
    externalSemaphoreHandleDesc.handle.win32.handle = g_ctx->vk.cuUpdateSemaphoreHandle;
#else
    externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
    externalSemaphoreHandleDesc.handle.fd = g_ctx->vk.cuUpdateSemaphoreFd;
#endif
    externalSemaphoreHandleDesc.flags = 0;
    cudaImportExternalSemaphore(&cuUpdateSemaphore, &externalSemaphoreHandleDesc);

    memset(&externalSemaphoreHandleDesc, 0, sizeof(externalSemaphoreHandleDesc));
#ifdef _WIN64
    externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueWin32;
    externalSemaphoreHandleDesc.handle.win32.handle = g_ctx->vk.vkUpdateSemaphoreHandle;
#else
    externalSemaphoreHandleDesc.type = cudaExternalSemaphoreHandleTypeOpaqueFd;
    externalSemaphoreHandleDesc.handle.fd = g_ctx->vk.vkUpdateSemaphoreFd;
#endif
    externalSemaphoreHandleDesc.flags = 0;
    cudaImportExternalSemaphore(&vkUpdateSemaphore, &externalSemaphoreHandleDesc);
}

void CudaEngine::init(Configuration& cfg, GlobalContext* g_ctx)
{
    this->g_ctx = g_ctx;

    cudaStreamCreate(&streamToRun);
    initSemaphore();
    initExternalMem();
    JSON_GET(DriverConfiguration, driver_cfg, cfg, "driver")
    total_frame = driver_cfg.total_frame;
    frame_rate = driver_cfg.frame_rate;
    steps_per_frame = driver_cfg.steps_per_frame;
    current_frame = 0;
}

void CudaEngine::step()
{
    waitOnSemaphore(vkUpdateSemaphore);

    // TODO

    signalSemaphore(cuUpdateSemaphore);
}

void CudaEngine::sync()
{
    cudaDeviceSynchronize();
}

void CudaEngine::cleanup()
{
    for (auto& p : extBuffers) {
        p.second.cleanup();
    }
    for (auto& p : extImages) {
        p.second.cleanup();
    }
    cudaDestroyExternalSemaphore(vkUpdateSemaphore);
    cudaDestroyExternalSemaphore(cuUpdateSemaphore);
}

void CudaEngine::waitOnSemaphore(cudaExternalSemaphore_t& semaphore)
{
    cudaExternalSemaphoreWaitParams extSemaphoreWaitParams;
    memset(&extSemaphoreWaitParams, 0, sizeof(extSemaphoreWaitParams));
    extSemaphoreWaitParams.params.fence.value = 0;
    extSemaphoreWaitParams.flags = 0;

    cudaWaitExternalSemaphoresAsync(
        &semaphore, &extSemaphoreWaitParams, 1, streamToRun);
}

void CudaEngine::signalSemaphore(cudaExternalSemaphore_t& semaphore)
{
    cudaExternalSemaphoreSignalParams extSemaphoreSignalParams;
    memset(&extSemaphoreSignalParams, 0, sizeof(extSemaphoreSignalParams));
    extSemaphoreSignalParams.params.fence.value = 0;
    extSemaphoreSignalParams.flags = 0;

    cudaSignalExternalSemaphoresAsync(
        &semaphore, &extSemaphoreSignalParams, 1, streamToRun);
}
