
#include "Unity/IUnityPlatformBase.h"
#include "Unity/IUnityFormat.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsVulkan.h"

#include <donut/unity/UnityApi.h>
#include <donut/core/log.h>
#include <donut/app/DeviceManager.h>
#include <donut/core/math/math.h>

#include <cmath>
#include <vector>
#include <mutex>
#include <algorithm>

#include <dxgi1_3.h>

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_win32.h>

#define VK_FUNC_LIST_APPLY_(apply) \
    apply(vkAllocateMemory) \
    apply(vkGetPhysicalDeviceMemoryProperties) \
    apply(vkEnumerateDeviceExtensionProperties) \
    apply(vkGetPhysicalDeviceProperties) \
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

namespace donut::unity
{


inline uint32_t get_vk_format(TextureFormat fmt)
{
    switch (fmt)
    {
    case kTexFormatAlpha8: return VK_FORMAT_R8_UNORM;
    case kTexFormatARGB4444: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
    case kTexFormatRGB24: return VK_FORMAT_R8G8B8_UNORM;
    case kTexFormatRGBA32: return VK_FORMAT_R8G8B8A8_UNORM;
    case kTexFormatARGB32: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
    case kTexFormatARGBFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case kTexFormatRGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;
    case kTexFormatBGR24: return VK_FORMAT_B8G8R8_UNORM;
    case kTexFormatR16: return VK_FORMAT_R16_UNORM;
    case kTexFormatDXT1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case kTexFormatDXT3: return VK_FORMAT_BC2_UNORM_BLOCK;
    case kTexFormatDXT5: return VK_FORMAT_BC3_UNORM_BLOCK;
    case kTexFormatRGBA4444: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
    case kTexFormatBGRA32: return VK_FORMAT_B8G8R8A8_UNORM;
    case kTexFormatRHalf: return VK_FORMAT_R16_SFLOAT;
    case kTexFormatRGHalf: return VK_FORMAT_R16G16_SFLOAT;
    case kTexFormatRGBAHalf: return VK_FORMAT_R16G16B16A16_SFLOAT;
    case kTexFormatRFloat: return VK_FORMAT_R32_SFLOAT;
    case kTexFormatRGFloat: return VK_FORMAT_R32G32_SFLOAT;
    case kTexFormatRGBAFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
    case kTexFormatYUY2: return VK_FORMAT_R8G8_UNORM;
    case kTexFormatRGB9e5Float: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
    case kTexFormatRGBFloat: return VK_FORMAT_R32G32B32_SFLOAT;
    case kTexFormatBC6H: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
    case kTexFormatBC7: return VK_FORMAT_BC7_UNORM_BLOCK;
    case kTexFormatBC4: return VK_FORMAT_BC4_UNORM_BLOCK;
    case kTexFormatBC5: return VK_FORMAT_BC5_UNORM_BLOCK;
    case kTexFormatDXT1Crunched: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    case kTexFormatDXT5Crunched: return VK_FORMAT_BC3_UNORM_BLOCK;
    case kTexFormatPVRTC_RGB2: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
    case kTexFormatPVRTC_RGBA2: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
    case kTexFormatPVRTC_RGB4: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
    case kTexFormatPVRTC_RGBA4: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
    case kTexFormatETC_RGB4: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case kTexFormatEAC_R: return VK_FORMAT_EAC_R11_UNORM_BLOCK;
    case kTexFormatEAC_R_SIGNED: return VK_FORMAT_EAC_R11_SNORM_BLOCK;
    case kTexFormatEAC_RG: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
    case kTexFormatEAC_RG_SIGNED: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
    case kTexFormatETC2_RGB: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case kTexFormatETC2_RGBA1: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
    case kTexFormatETC2_RGBA8: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case kTexFormatASTC_4x4: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
    case kTexFormatASTC_5x5: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
    case kTexFormatASTC_6x6: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
    case kTexFormatASTC_8x8: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
    case kTexFormatASTC_10x10: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
    case kTexFormatASTC_12x12: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
    case kTexFormatETC_RGB4_3DS: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case kTexFormatETC_RGBA8_3DS: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case kTexFormatRG16: return VK_FORMAT_R16G16_UNORM;
    case kTexFormatR8: return VK_FORMAT_R8_UNORM;
    case kTexFormatETC_RGB4Crunched: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
    case kTexFormatETC2_RGBA8Crunched: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
    case kTexFormatASTC_HDR_4x4: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
    case kTexFormatASTC_HDR_5x5: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
    case kTexFormatASTC_HDR_6x6: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
    case kTexFormatASTC_HDR_8x8: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
    case kTexFormatASTC_HDR_10x10: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
    case kTexFormatASTC_HDR_12x12: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
    case kTexFormatRG32: return VK_FORMAT_R16G16_UNORM;
    case kTexFormatRGB48: return VK_FORMAT_R16G16B16_UNORM;
    case kTexFormatRGBA64: return VK_FORMAT_R16G16B16A16_UNORM;
    default: return VK_FORMAT_UNDEFINED;
    }
}

class UnityVulkan : public UnityApi
{
public:
	UnityVulkan(IUnityInterfaces* pUnity) : UnityApi(pUnity) {}
    void initialize() override
    {
        UnityApi::initialize();
        mpUnityVulkan = mpUnity->Get<IUnityGraphicsVulkan>();
        mUnityVulkanInstance = mpUnityVulkan->Instance();
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

        VkPhysicalDeviceProperties physicalDeviceProperties;
        _vkGetPhysicalDeviceProperties(mpPhysicalDevice, &physicalDeviceProperties);
        if (physicalDeviceProperties.vendorID != 0x10DE)
        {
            DONUT_LOG_WARNING("Unity isn't using a NVIDIA card, some functions maybe disabled\n");
        }
    }

    void shutdown() override
    {
        UnityApi::shutdown();
    }

    uint32_t getNativeFormat(uint32_t format) override { return get_vk_format((TextureFormat)format); }
    void* createSharedTexSrc(UnityTexDesc inDesc) override{ return inDesc.pHandle ? createSharedTexture(inDesc, true) : nullptr; }
    void* createSharedTexDst(UnityTexDesc inDesc) override { return (inDesc.pHandle && *(inDesc.pHandle)) ? createSharedTexture(inDesc, false) : nullptr; }
    void* createSharedBufSrc(UnityBufDesc inDesc) override { return inDesc.pHandle ? createSharedBuffer(inDesc, true) : nullptr; }
    void* createSharedBufDst(UnityBufDesc inDesc) override { return (inDesc.pHandle && *(inDesc.pHandle)) ? createSharedBuffer(inDesc, false) : nullptr; }

    nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI() const override { return nvrhi::GraphicsAPI::VULKAN; }

private:

    void* createSharedTexture(UnityTexDesc inDesc, bool bSrc)
    {
        VkImageCreateInfo imageInfo = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
        imageInfo.extent = VkExtent3D{ inDesc.width, inDesc.height, 1 };
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.mipLevels = inDesc.mipCount;
        imageInfo.arrayLayers = 1;
        imageInfo.format = (VkFormat)getNativeFormat(inDesc.format);
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
            DONUT_LOG_ERROR("Error:Failed to vkCreateImage: width=%d, height%d, unityFormat=%d, mip=%d, bSrc=%d\n", inDesc.width, inDesc.height, inDesc.format, inDesc.mipCount, bSrc);
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
            DONUT_LOG_ERROR("Error:Failed to find memory type index. width=%d, height%d, unityFormat=%d, mip=%d, bSrc=%d\n", inDesc.width, inDesc.height, inDesc.format, inDesc.mipCount, bSrc);
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
            DONUT_LOG_ERROR("Error:Failed to vkAllocateMemory. width=%d, height%d, unityFormat=%d, mip=%d, bSrc=%d\n", inDesc.width, inDesc.height, inDesc.format, inDesc.mipCount, bSrc);
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
            DONUT_LOG_ERROR("[createSharedBuffer] Error:Failed to CommandRecordingState.\n");
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
            DONUT_LOG_ERROR("Error:Failed to vkCreateBuffer.\n");
            return nullptr;
        }
        mOwnedBufPtrs.push_back(buffer);

        VkMemoryRequirements memRequirements;
        _vkGetBufferMemoryRequirements(mpDevice, buffer, &memRequirements);

        // find memory type index
        int memoryTypeIndex = find_memory_type_index(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        if (memoryTypeIndex < 0)
        {
            DONUT_LOG_ERROR("Error:Failed to find memory type index.\n");
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
            DONUT_LOG_ERROR("Error:Failed to vkAllocateMemory.\n");
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
            DONUT_LOG_ERROR("Error:Failed to vkGetMemoryWin32HandleKHR.\n");
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
    UnityVulkanInstance mUnityVulkanInstance{};
    VkPhysicalDeviceMemoryProperties mDeviceMemoryProperties {};
};
UnityApi* CreateUnityVulkan(IUnityInterfaces* pUnity) { return new UnityVulkan(pUnity); }

} // namespace donut::unity