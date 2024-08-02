#include "Unity/IUnityPlatformBase.h"
#include "Unity/IUnityFormat.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityGraphicsVulkan.h"

#include <donut/app/UnityApi.h>
#include <donut/app/Camera.h>
#include <donut/app/ApplicationBase.h>
#include <donut/core/log.h>
#include <donut/core/math/math.h>
#include <donut/app/DeviceManager.h>
#include <donut/engine/Scene.h>

#include <cmath>
#include <vector>
#include <mutex>
#include <algorithm>

#undef min
#undef max

namespace donut::app
{

class ApiD3D11 : public UnityApi
{
public:
    void init(IUnityInterfaces* pUnity) override
    {
        mpUnity = pUnity;
        mpUnityGraphics = pUnity->Get<IUnityGraphics>();
        mpDevice = pUnity->Get<IUnityGraphicsD3D11>()->GetDevice();
    }
    
    void destroy() override
    {
    }

    uint32_t getFormat(uint32_t format) override { return 0; }// get_dxgi_format((TextureFormat)format); }

    void* createSharedTexSrc(UnityTexDesc inDesc) override
    {
        D3D11_TEXTURE2D_DESC desc = {};
        uint32_t bindFlags = 0;
        if (inDesc.bReadable) { bindFlags |= D3D11_BIND_SHADER_RESOURCE; }
        if (inDesc.bWriteable) { bindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS; }
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = 0;
        desc.Format = (DXGI_FORMAT)(inDesc.format);
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED; // D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
	    desc.MipLevels = inDesc.mipCount;
	    desc.ArraySize = 1; 
	    desc.Width = inDesc.width;
	    desc.Height = inDesc.height;
	    desc.Usage = D3D11_USAGE_DEFAULT;
	    desc.SampleDesc.Count = 1;
	    desc.SampleDesc.Quality = 0;
        ID3D11Texture2D* pOutSharedTex = nullptr;
        if (FAILED(mpDevice->CreateTexture2D(&desc, nullptr, &pOutSharedTex)))
	    {
	    	logError("Failed to create texture for sharing.\n");
	    }
	    else 
	    {
            IDXGIResource1* res;
            HANDLE outHandle = 0;
            if(!FAILED(pOutSharedTex->QueryInterface(__uuidof(IDXGIResource), (void**)&res)) &&
                #if 0 // D3D11_RESOURCE_MISC_SHARED_NTHANDLE | D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX
	           !FAILED(res->CreateSharedHandle(nullptr, DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE, nullptr, &handle)))
                #else // D3D11_RESOURCE_MISC_SHARED
                !FAILED(res->GetSharedHandle(&outHandle)))
                #endif
            {
                *inDesc.pHandle = outHandle;
                return (void*)pOutSharedTex;
	        }
            else
            {
	    	    logError("Failed to create shared handle.\n");
            }
	    }
        return nullptr;
    }

    void* createSharedTexDst(UnityTexDesc desc) override
    {
        if (desc.pHandle == nullptr || *desc.pHandle == 0) { return nullptr; }

        ID3D11Resource* resource = nullptr;
        if (FAILED(mpDevice->OpenSharedResource(*(desc.pHandle), IID_PPV_ARGS(&resource))))
        {
            logError("Failed to create texture from shared handle.\n");
        }
        else
        {
            ID3D11Texture2D* retTex = nullptr;
            if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&retTex))))
            {
                logError("Failed to query D3D11Texture.\n");
            }
            else
            {
                return retTex;
            }
        }
        return nullptr;
    }

    void* createSharedBufSrc(UnityBufDesc desc) override { return nullptr; }
    void* createSharedBufDst(UnityBufDesc desc) override { return nullptr; }
    void getSharedBufferHandleWin32(void* nativeBuf, HANDLE& handle) override { handle = nullptr; }
    void getSharedTextureHandleWin32(void* nativeTex, HANDLE& handle) override { handle = nullptr; }

private:
    ID3D11Device* mpDevice = nullptr;
};

#define VK_FUNC_LIST_APPLY_(apply) \
    apply(vkAllocateMemory) \
    apply(vkGetPhysicalDeviceMemoryProperties) \
    apply(vkGetMemoryWin32HandleKHR) \
    apply(vkCreateImage) \
    apply(vkGetImageMemoryRequirements) \
    apply(vkBindImageMemory) \
    apply(vkDestroyImage) \
    apply(vkCmdCopyImage) \
    apply(vkCreateBuffer) \
    apply(vkGetBufferMemoryRequirements) \
    apply(vkBindBufferMemory) \
    apply(vkDestroyBuffer) \
    apply(vkCmdCopyBuffer) \
    apply(vkMapMemory) \
    apply(vkUnmapMemory) \
    apply(vkFlushMappedMemoryRanges)

#define VK_OP_DECL(func) static PFN_##func _##func = nullptr;
#define VK_OP_LOAD(func) _##func = (PFN_##func)ins.getInstanceProcAddr(ins.instance, #func);
VK_FUNC_LIST_APPLY_(VK_OP_DECL)

class ApiVulkan : public UnityApi
{
public:
    void init(IUnityInterfaces* pUnity) override
    {
        mpUnity = pUnity;
        mpUnityGraphics = pUnity->Get<IUnityGraphics>();
        mpUnityVulkan = pUnity->Get<IUnityGraphicsVulkan>();
        UnityVulkanInstance ins = mpUnityVulkan->Instance();
        mpDevice = ins.device;
        mpPhysicalDevice = ins.physicalDevice;
        VK_FUNC_LIST_APPLY_(VK_OP_LOAD)
        _vkGetPhysicalDeviceMemoryProperties(mpPhysicalDevice, &mDeviceMemoryProperties);

        UnityVulkanPluginEventConfig config_1;
        config_1.graphicsQueueAccess = kUnityVulkanGraphicsQueueAccess_DontCare;
        config_1.renderPassPrecondition = kUnityVulkanRenderPass_EnsureInside;
        config_1.flags = kUnityVulkanEventConfigFlag_EnsurePreviousFrameSubmission | kUnityVulkanEventConfigFlag_ModifiesCommandBuffersState;
        mpUnityVulkan->ConfigureEvent(1, &config_1);
    }

    void destroy() override
    {
    }

    uint32_t getFormat(uint32_t format) override { return 0; }// get_vk_format((TextureFormat)format); }
    void* createSharedTexSrc(UnityTexDesc inDesc) override{ return inDesc.pHandle ? createSharedTexture(inDesc, true) : nullptr; }
    void* createSharedTexDst(UnityTexDesc inDesc) override { return (inDesc.pHandle && *(inDesc.pHandle)) ? createSharedTexture(inDesc, false) : nullptr; }
    void* createSharedBufSrc(UnityBufDesc inDesc) override { return inDesc.pHandle ? createSharedBuffer(inDesc, true) : nullptr; }
    void* createSharedBufDst(UnityBufDesc inDesc) override { return (inDesc.pHandle && *(inDesc.pHandle)) ? createSharedBuffer(inDesc, false) : nullptr; }

    void getSharedBufferHandleWin32(void* nativeBuf, HANDLE& handle) override
    {
        UnityVulkanBuffer unityBuffer;
        mpUnityVulkan->AccessBuffer(nativeBuf, 0, 0, kUnityVulkanResourceAccess_ObserveOnly, &unityBuffer);
        handle = (unityBuffer.memory.memory != VK_NULL_HANDLE) ? unityBuffer.memory.reserved[0] : nullptr;
    }

    void getSharedTextureHandleWin32(void* nativeTex, HANDLE& handle) override
    {
        UnityVulkanImage unityImage;
        mpUnityVulkan->AccessTexture(nativeTex, NULL, VK_IMAGE_LAYOUT_GENERAL, VK_PIPELINE_STAGE_NONE, VK_ACCESS_NONE, kUnityVulkanResourceAccess_ObserveOnly, &unityImage);
        handle = (unityImage.memory.memory != VK_NULL_HANDLE) ? unityImage.memory.reserved[0] : nullptr;
    }

private:

    void* createSharedTexture(UnityTexDesc inDesc, bool bSrc)
    {
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.extent = VkExtent3D{ inDesc.width, inDesc.height, 1 };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.mipLevels = inDesc.mipCount;
        imageInfo.arrayLayers = 1;
        imageInfo.format = (VkFormat)(inDesc.format);
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // TODO
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
        VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO };
        externalMemoryImageCreateInfo.pNext = nullptr;
        externalMemoryImageCreateInfo.handleTypes = kExternalMemoryHandleType;
        imageInfo.pNext = bSrc ? &externalMemoryImageCreateInfo : nullptr;  // TODO?

        VkImage image = VK_NULL_HANDLE;
        if (_vkCreateImage(mpDevice, &imageInfo, nullptr, &image) != VK_SUCCESS)
        {
            logError("Error:Failed to vkCreateImage.\n");
            return nullptr;
        }
        mOwnedTexPtrs.push_back(image);
    
        VkMemoryRequirements memRequirements;
        _vkGetImageMemoryRequirements(mpDevice, image, &memRequirements);
    
        // Allocate the memory for src texture
        DWORD dwAccess = 0;
        if (inDesc.bReadable) { dwAccess |= DXGI_SHARED_RESOURCE_READ; }
        if (inDesc.bWriteable) { dwAccess |= DXGI_SHARED_RESOURCE_WRITE; }
        VkExportMemoryWin32HandleInfoKHR exportMemoryWin32HandleInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
        exportMemoryWin32HandleInfo.pNext = nullptr;
        exportMemoryWin32HandleInfo.pAttributes = nullptr;
        exportMemoryWin32HandleInfo.dwAccess = dwAccess;
        exportMemoryWin32HandleInfo.name = NULL;
    
        VkExportMemoryAllocateInfoKHR exportMemoryAllocateInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR };
        exportMemoryAllocateInfo.pNext = &exportMemoryWin32HandleInfo;
        exportMemoryAllocateInfo.handleTypes = kExternalMemoryHandleType;
        
        // Allocate the memory for dst texture
        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
        memoryDedicatedAllocateInfo.image = image;
        VkImportMemoryWin32HandleInfoKHR importMemoryWin32HandleInfo = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
        importMemoryWin32HandleInfo.pNext = &memoryDedicatedAllocateInfo;
        importMemoryWin32HandleInfo.handleType = kExternalMemoryHandleType;
        importMemoryWin32HandleInfo.handle = *(inDesc.pHandle);

        // find memory type index
        int memoryTypeIndex = find_memory_type_index(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryTypeIndex < 0)
        {
            logError("Error:Failed to find memory type index.\n");
            return nullptr;
        }

        // Allocate the memory
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.pNext = bSrc ? (void*)(&exportMemoryAllocateInfo) : (void*)(&importMemoryWin32HandleInfo);
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
    
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        if(_vkAllocateMemory(mpDevice, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS)
        {
            logError("Error:Failed to vkAllocateMemory.\n");
            return nullptr;
        }
        _vkBindImageMemory(mpDevice, image, imageMemory, 0);
    
        if (bSrc) { *inDesc.pHandle = getSharedHandleWin32(imageMemory); }
        return (*inDesc.pHandle) ? &mOwnedTexPtrs[mOwnedTexPtrs.size() - 1] : nullptr;
    }

    void* createSharedBuffer(UnityBufDesc inDesc, bool bSrc)
    {
        UnityVulkanRecordingState recordingState;
        if (!mpUnityVulkan->CommandRecordingState(&recordingState, kUnityVulkanGraphicsQueueAccess_DontCare) ||
            recordingState.commandBuffer == VK_NULL_HANDLE)
        {
            logError("[createSharedBuffer] Error:Failed to CommandRecordingState.\n");
        }
        VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
        bufferInfo.size = inDesc.size;
        bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkExternalMemoryBufferCreateInfo externalMemoryBufferCreateInfo = { VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_BUFFER_CREATE_INFO };
        externalMemoryBufferCreateInfo.pNext = nullptr;
        externalMemoryBufferCreateInfo.handleTypes = kExternalMemoryHandleType;
        bufferInfo.pNext = bSrc ? &externalMemoryBufferCreateInfo : nullptr;

        VkBuffer buffer = VK_NULL_HANDLE;
        if (_vkCreateBuffer(mpDevice, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        {
            logError("Error:Failed to vkCreateBuffer.\n");
            return nullptr;
        }
        mOwnedBufPtrs.push_back(buffer);

        VkMemoryRequirements memRequirements;
        _vkGetBufferMemoryRequirements(mpDevice, buffer, &memRequirements);

        // find memory type index
        int memoryTypeIndex = find_memory_type_index(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryTypeIndex < 0)
        {
            logError("Error:Failed to find memory type index.\n");
            return nullptr;
        }
        // allocate memory for src buffer
        VkExportMemoryWin32HandleInfoKHR exportMemoryWin32HandleInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
        exportMemoryWin32HandleInfo.pNext = nullptr;
        exportMemoryWin32HandleInfo.pAttributes = nullptr;
        exportMemoryWin32HandleInfo.dwAccess = DXGI_SHARED_RESOURCE_READ | DXGI_SHARED_RESOURCE_WRITE;
        exportMemoryWin32HandleInfo.name = NULL;

        VkExportMemoryAllocateInfoKHR exportMemoryAllocateInfo = { VK_STRUCTURE_TYPE_EXPORT_MEMORY_ALLOCATE_INFO_KHR };
        exportMemoryAllocateInfo.pNext = &exportMemoryWin32HandleInfo;
        exportMemoryAllocateInfo.handleTypes = kExternalMemoryHandleType;

        // allocate memory for dst buffer
        VkMemoryDedicatedAllocateInfo memoryDedicatedAllocateInfo = { VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO };
        memoryDedicatedAllocateInfo.buffer = buffer;
    
        VkImportMemoryWin32HandleInfoKHR importMemoryWin32HandleInfo = { VK_STRUCTURE_TYPE_IMPORT_MEMORY_WIN32_HANDLE_INFO_KHR };
        importMemoryWin32HandleInfo.pNext = &memoryDedicatedAllocateInfo;
        importMemoryWin32HandleInfo.handleType = kExternalMemoryHandleType;
        importMemoryWin32HandleInfo.handle = *(inDesc.pHandle);

        // Allocate the memory
        VkMemoryAllocateInfo allocInfo = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memoryTypeIndex;
        allocInfo.pNext = bSrc ? (void*)&exportMemoryAllocateInfo : (void*)&importMemoryWin32HandleInfo;

        VkDeviceMemory bufferMemory = VK_NULL_HANDLE;
        if (_vkAllocateMemory(mpDevice, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS)
        {
            logError("Error:Failed to vkAllocateMemory.\n");
            return nullptr;
        }
        _vkBindBufferMemory(mpDevice, buffer, bufferMemory, 0);

        if (inDesc.pInitData)
        {
        }

        if (bSrc) { *inDesc.pHandle = getSharedHandleWin32(bufferMemory); }
        return (*inDesc.pHandle) ? &mOwnedBufPtrs[mOwnedBufPtrs.size() - 1] : nullptr;
    }


    HANDLE getSharedHandleWin32(VkDeviceMemory memory)
    {
        VkMemoryGetWin32HandleInfoKHR getWin32HandleInfo = { VK_STRUCTURE_TYPE_MEMORY_GET_WIN32_HANDLE_INFO_KHR };
        getWin32HandleInfo.pNext = nullptr;
        getWin32HandleInfo.memory = memory;
        getWin32HandleInfo.handleType = (VkExternalMemoryHandleTypeFlagBits)kExternalMemoryHandleType;
        HANDLE outWin32Handle = 0;
        if (_vkGetMemoryWin32HandleKHR(mpDevice, &getWin32HandleInfo, &outWin32Handle) ==  VK_SUCCESS)
        {
            return outWin32Handle;
        }
        else
        {
            logError("Error:Failed to vkGetMemoryWin32HandleKHR.\n");
            return nullptr;
        }
    }

    int find_memory_type_index(uint32_t typeBits, VkMemoryPropertyFlags properties)
    {
        const int numMemoryTypes = int(mDeviceMemoryProperties.memoryTypeCount);
        uint32_t bit = 1;
        for (int i = 0;  i < numMemoryTypes; ++i, bit += bit)
        {
            auto const& memoryType = mDeviceMemoryProperties.memoryTypes[i];
            if ((typeBits & bit) && (memoryType.propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        return -1;
    }

private:
    static constexpr auto kExternalMemoryHandleType = VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_WIN32_BIT;
    VkDevice mpDevice = nullptr;
    VkPhysicalDevice mpPhysicalDevice = nullptr;
    IUnityGraphicsVulkan* mpUnityVulkan = nullptr;  
    VkPhysicalDeviceMemoryProperties mDeviceMemoryProperties {};
};

static UnityApi* gs_api = nullptr;
UnityApi* UnityApi::GetApiInstance()
{
    return gs_api;
}

} // namespace donut::app

///////////////////////////////////////////////////
// Exported APIs
///////////////////////////////////////////////////
using namespace donut::app;

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
CreateSharedTexture(uint32_t width, uint32_t height, uint32_t format, uint32_t mipCount, void*& pTexHandleUsedByUnity, void*& outHandle)
{
    outHandle = nullptr;
    UnityTexDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = gs_api->getFormat(format);
    desc.mipCount = mipCount;
    desc.bReadable = true;
    desc.bWriteable = true;
    desc.pHandle = &outHandle;
    pTexHandleUsedByUnity = (gs_api && width && format && mipCount) ? gs_api->createSharedTexSrc(desc) : nullptr;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
CreateSharedTextureFromHandle(uint32_t width, uint32_t height, uint32_t format, uint32_t mipCount, void*& pTexHandleUsedByUnity, void* handle)
{
    UnityTexDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = gs_api->getFormat(format);
    desc.mipCount = mipCount;
    desc.bReadable = true;
    desc.bWriteable = true;
    desc.pHandle = &handle;
    pTexHandleUsedByUnity = (gs_api && width && format && mipCount && handle) ? gs_api->createSharedTexDst(desc) : nullptr;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
CreateSharedBuffer(uint32_t size, void* pInitData, void*& pBufferHandleUsedByUnity, void*& outHandle)
{
    outHandle = nullptr;
    UnityBufDesc desc;
    desc.size = size;
    desc.pInitData = pInitData;
    desc.pHandle = &outHandle;
    pBufferHandleUsedByUnity = (gs_api && size) ? gs_api->createSharedBufSrc(desc) : nullptr;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
CreateSharedBufferFromHandle(uint32_t size, void* pInitData, void*& pBufferHandleUsedByUnity, void* handle)
{
    UnityBufDesc desc;
    desc.size = size;
    desc.pInitData = pInitData;
    desc.pHandle = &handle;
    pBufferHandleUsedByUnity = (gs_api && size && handle) ? gs_api->createSharedBufDst(desc) : nullptr;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetSharedBufferHandleWin32(void* nativeBuf, void*& handle)
{
    handle = nullptr;
    if (gs_api && nativeBuf)
    {
        gs_api->getSharedBufferHandleWin32(nativeBuf, (HANDLE&)handle);
    }
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetSharedTextureHandleWin32(void* nativeTex, void*& handle)
{
    handle = nullptr;
    if (gs_api && nativeTex)
    {
        gs_api->getSharedTextureHandleWin32(nativeTex, (HANDLE&)handle);
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    auto* pGfx = unityInterfaces->Get<IUnityGraphics>();
    if (pGfx->GetRenderer() == kUnityGfxRendererVulkan)
    {
        gs_api = new ApiVulkan();
    }
    else if (pGfx->GetRenderer() == kUnityGfxRendererD3D11)
    {
        gs_api = new ApiD3D11();
    }

    if (gs_api)
    {
        gs_api->init(unityInterfaces);
    }
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    if (gs_api)
    {
        gs_api->destroy();
        delete gs_api;
        gs_api = nullptr;
    }
}

static void UNITY_INTERFACE_API
OnRenderEvent(int eventID)
{
    ApiVulkan *api = dynamic_cast<ApiVulkan*>(gs_api);
    if (api)
    {
    }
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc()
{
	return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_LaunchBakerThread(char* pluginFolder, bool headless, void* pSceneData, int bytes)
{
    if (gs_api && pluginFolder)
    {
        gs_api->SetPluginFolder(pluginFolder);
        gs_api->LaunchBakerThread(headless);
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_UpdateOrAddLights(void* pLights, int numLights)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_UpdateTransforms(void* pTransforms, int numTransforms)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_UpdateMaterials(void* pMaterials, int numMaterials)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_UpdateObjects(void* pObjects, int numObjects)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_UpdateGlobalSetting(void* pSetting)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_DeleteLights(int* pLightIds, int numLights)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_DeleteObjects(int* pObjectIds, int numObjects)
{
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_ExitBakerThread()
{
    if (gs_api)
    {
        gs_api->ExitBakerThread();
    }
}
