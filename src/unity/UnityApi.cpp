#include "Unity/IUnityPlatformBase.h"
#include "Unity/IUnityFormat.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"
#include "Unity/IUnityGraphicsVulkan.h"

#include <donut/unity/UnityApi.h>
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

namespace donut::unity
{
    void UnityApi::LaunchBakerThread(bool headless)
    {
        ExitBakerThread();
        if (HasValidPluginFolder() && ThreadEntryPtr)
        {
            mpBakeThread = std::make_unique<std::thread>([this, headless]()
                {
                    mShouldExit = false;
                    SetThreadDescription(GetCurrentThread(), L"FBakerThread");
                    ThreadEntryPtr(headless);
                });
            mpBakeThread->detach();
        }
    }

    void UnityApi::SetFolders(char* pluginFolder, char* tempImportFolder, char* tempExportFolder)
    {
        if (pluginFolder) mPluginFolder = pluginFolder;
        if (tempImportFolder) mTempImportFolder = tempImportFolder;
        if (tempExportFolder) mTempExportFolder = tempExportFolder;
    }


static UnityApi* gs_api = nullptr;
UnityApi* UnityApi::GetApiInstance()
{
    return gs_api;
}

} // namespace donut::unity

///////////////////////////////////////////////////
// Exported APIs
///////////////////////////////////////////////////
using namespace donut::unity;

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

#if 1

static PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr_LoadedByUnity = nullptr;

template <typename N>
static void get_combined_ext_names(
    std::vector<const char*>& newExtNames,
    const N& exceptedByPlugin,
    const char* const* pCreateInfoExtNames,
    uint32_t extCount)
{
    for (uint32_t i = 0; i < extCount; ++i)
    {
        newExtNames.push_back(pCreateInfoExtNames[i]);
    }
    for (const char* ext : exceptedByPlugin)
    {
        bool bFound = false;
        for (uint32_t i = 0; i < extCount; ++i)
        {
            if (strcmp(pCreateInfoExtNames[i], ext) == 0)
            {
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            newExtNames.push_back(ext);
        }
    }
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkInstance* pInstance)
{
   /* std::vector<const char*> newInsExtNames;
    get_combined_ext_names(newInsExtNames, kExceptedVulkanInstanceExts, pCreateInfo->ppEnabledExtensionNames, pCreateInfo->enabledExtensionCount);*/
    //(const_cast<VkInstanceCreateInfo*>(pCreateInfo))->ppEnabledExtensionNames = newInsExtNames.data();
    //(const_cast<VkInstanceCreateInfo*>(pCreateInfo))->enabledExtensionCount = uint32_t(newInsExtNames.size());
    auto ptr = (PFN_vkCreateInstance)vkGetInstanceProcAddr_LoadedByUnity(VK_NULL_HANDLE, "vkCreateInstance");
    return ptr(pCreateInfo, pAllocator, pInstance);
}

static VKAPI_ATTR VkResult VKAPI_CALL Hook_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDevice* pDevice)
{
   /* std::vector<const char*> newIDeviceExtNames;
    get_combined_ext_names(newIDeviceExtNames, kExceptedVulkanDeviceExts, pCreateInfo->ppEnabledExtensionNames, pCreateInfo->enabledExtensionCount);
    (const_cast<VkDeviceCreateInfo*>(pCreateInfo))->ppEnabledExtensionNames = newIDeviceExtNames.data();
    (const_cast<VkDeviceCreateInfo*>(pCreateInfo))->enabledExtensionCount = uint32_t(newIDeviceExtNames.size());*/
    auto ptr = (PFN_vkCreateDevice)vkGetInstanceProcAddr_LoadedByUnity(VK_NULL_HANDLE, "vkCreateDevice");
    return ptr(physicalDevice, pCreateInfo, pAllocator, pDevice);
}

static VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL Hook_vkGetInstanceProcAddr(VkInstance device, const char* funcName)
{
    if (!funcName) { return NULL; }

#define VK_INTERCEPT(fn) if (strcmp(funcName, #fn) == 0) { return (PFN_vkVoidFunction)&Hook_##fn; }
     VK_INTERCEPT(vkCreateInstance)
     VK_INTERCEPT(vkCreateDevice)
#undef INTERCEPT

        return NULL;
}

static PFN_vkGetInstanceProcAddr UNITY_INTERFACE_API InterceptVulkanInitialization(PFN_vkGetInstanceProcAddr getInstanceProcAddr, void*)
{
    vkGetInstanceProcAddr_LoadedByUnity = getInstanceProcAddr;
    return Hook_vkGetInstanceProcAddr;
}
#endif

static IUnityInterfaces* spUnityInterface = nullptr;
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {   
    case kUnityGfxDeviceEventInitialize:
    {
        auto* pGfx = spUnityInterface->Get<IUnityGraphics>();
        if (pGfx->GetRenderer() == kUnityGfxRendererD3D11)
        {
            gs_api = CreateUnityD3D11(spUnityInterface);
			gs_api->initialize();
		}
		else if (pGfx->GetRenderer() == kUnityGfxRendererVulkan)
		{
			gs_api = CreateUnityVulkan(spUnityInterface);
			gs_api->initialize();
        }
        else if (pGfx->GetRenderer() == kUnityGfxRendererD3D12)
        {
            gs_api = CreateUnityD3D12(spUnityInterface);
            gs_api->initialize();
        }
        else
        {
			donut::log::warning("Unsupported api %d.\n", pGfx->GetRenderer());
        }
        break;
    }
    case kUnityGfxDeviceEventShutdown:
    {
        if (gs_api)
        {
            gs_api->shutdown();
            delete gs_api;
            gs_api = nullptr;
        }
        break; 
    }
    case kUnityGfxDeviceEventBeforeReset:
        break;
    case kUnityGfxDeviceEventAfterReset:
        break;
    default:
        break;
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginLoad(IUnityInterfaces* unityInterfaces)
{
    spUnityInterface = unityInterfaces;
	auto* pGfx = spUnityInterface->Get<IUnityGraphics>();
	pGfx->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
#if 0
    if (pGfx->GetRenderer() == kUnityGfxRendererNull)
    {
        if (IUnityGraphicsVulkanV2* vulkanInterface = spUnityInterface->Get<IUnityGraphicsVulkanV2>())
            vulkanInterface->AddInterceptInitialization(InterceptVulkanInitialization, NULL, 0);
        else if (IUnityGraphicsVulkan* vulkanInterface = spUnityInterface->Get<IUnityGraphicsVulkan>())
            vulkanInterface->InterceptInitialization(InterceptVulkanInitialization, NULL);
    }
#endif
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    if (gs_api)
    {
        gs_api->shutdown();
        delete gs_api;
        gs_api = nullptr;
    }
}

static void UNITY_INTERFACE_API
OnRenderEvent(int eventID)
{
}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc()
{
	return OnRenderEvent;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_LaunchBakerThread(
    char* pluginFolder,
    char* tempImportFolder,
    char* tempExportFolder,
    bool headless,
    void* pSceneData,
    int bytes,
	void*& pOutSharedTexData,
    bool bUseDebugScene)
{
    pOutSharedTexData = nullptr;
    if (gs_api && pluginFolder && (bUseDebugScene || (pSceneData && bytes)))
    {
        gs_api->SetFolders(pluginFolder, tempImportFolder, tempExportFolder);
        gs_api->LaunchBakerThread(headless);
    }
    else
    {
        donut::log::error("Failed to launch baker thread. State=%p, pluginFolder=%s, tempImportFolder=%s,"
            " tempExportFolder=%s, headless=%d, pSceneData=%p, bytes=%d, bUseDebugScene=%d\n",
            gs_api, pluginFolder, tempImportFolder, tempExportFolder, headless, pSceneData, bytes, bUseDebugScene);
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
