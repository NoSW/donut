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
#include <donut/unity/FBScene.h>

#include <cmath>
#include <vector>
#include <mutex>
#include <algorithm>


#undef min
#undef max

namespace donut::unity
{


UnityApi::UnityApi(IUnityInterfaces* pUnity) : mpUnity(pUnity)
{
}

UnityApi::~UnityApi()
{
    shutdown();
    mMutex.unlock();
}

void UnityApi::LaunchBakerThread(bool headless)
{
    ExitBakerThread(false);
    mpBakeThread = std::make_unique<std::thread>([this, headless]()
        {
            mRequestExitBakerThread = false;
            auto hr = SetThreadDescription(GetCurrentThread(), L"FBakerThread");
            if (FAILED(hr))
            {
               log::error("Failed to set thread description for baker thread.\n");
            }
            ThreadEntryPtr(headless);
        });
    mpBakeThread->detach();
}

void UnityApi::ExitBakerThread(bool bForce)
{
    if (mpBakeThread)
    {
        if (IsBakerThreadRunning())
        {
            if (bForce)
            {
                if (TerminateThread(mpBakeThread->native_handle(), 1))
                    log::error("Failed to terminate baker thread forcedly.\n");
                else
                    log::warning("Succeed to terminate baker thread forcedly, which does not proper thread clean up\n");
            }
            else
            {
                mRequestExitBakerThread = true;
                WaitForBakerThread(30); // wait for exit
            }
        }
        mpBakeThread.reset();
    }
}

    bool UnityApi::IsBakerThreadRunning() const
    {
        if (mpBakeThread && mpBakeThread->native_handle())
        {
            DWORD exitCode;
            if (GetExitCodeThread(mpBakeThread->native_handle(), &exitCode))
            {
                return exitCode == STILL_ACTIVE;
            }
            else
            {
                log::error("Failed to get exit code of baker thread.\n");
            }
        }
        return false;
    }

void UnityApi::Log(log::Severity severity, const char* message) const
{
    const auto printable = [](char c) { return c >= 32 && c <= 126; };
    const auto extract_ascii  = [&printable](char* dst, const char* src, int maxLen)
    {
        if (!dst || !src) { return; }
        int len = 0;
        while (len < maxLen - 1 && printable(*src))
        {
            *dst = *src;
            dst++;
            src++;
            len++;
        }
        *dst = '\0';
    };
    if (!pUnityLogCallBake || !message) { return; }
    else
    {
        static char buf[1024];
        extract_ascii(buf, message, 1024);
        pUnityLogCallBake((int)severity, buf);
    }
}

bool UnityApi::checkFolderPath(const char* path)
{
   if (path)
   {    
        const char* p = path;
        while(p != '\0')
        {
            if (!(*p >= 32 && *p <= 126)) { return false; }
            p++;
        }
        std::filesystem::path folder(path);
        return std::filesystem::exists(folder) && std::filesystem::is_directory(folder);
    }
    return false;
}
void UnityApi::PushTask(uint32_t type, uint8_t* ptr, uint32_t bytes)
{ 
    if (!IsBakerThreadRunning())
    {
        log::error("Baker thread is not running, task type=%d is ignored.\n", type);
        return;
    }
    mTaskScheduler.PushTask(type, ptr, bytes);
}

void UnityApi::FlushTasks(uint64_t timeout)
{
    if (!IsBakerThreadRunning())
    {
        log::error("Baker thread is not running, failed to flush tasks.\n");
        return;
    }
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mCVar.wait_for(lock, std::chrono::seconds(timeout), [this] { return mTaskScheduler.GetApproxTaskCount() <= 0; });
    }
}

static UnityApi* gs_api = nullptr;
UnityApi* UnityApi::Ins()
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
    if (width == 0 || height == 0 || format == 0 || mipCount == 0 || (gs_api && !gs_api->checkUnityFormat(format)))
    {
        DONUT_LOG_ERROR("Invalid texture parameters: width=%d, height=%d, format=%d, mipCount=%d\n", width, height, format, mipCount);
        return;
    }
    outHandle = nullptr;
    UnityTexDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = format;
    desc.mipCount = mipCount;
    desc.bReadable = true;
    desc.bWriteable = true;
    desc.pHandle = &outHandle;
    pTexHandleUsedByUnity = (gs_api && width && format && mipCount) ? gs_api->createSharedTexSrc(desc) : nullptr;
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
CreateSharedTextureFromHandle(uint32_t width, uint32_t height, uint32_t format, uint32_t mipCount, void*& pTexHandleUsedByUnity, void* handle)
{
    if (width == 0 || height == 0 || format == 0 || mipCount == 0 || (gs_api && !gs_api->checkUnityFormat(format)))
    {
        DONUT_LOG_ERROR("Invalid texture parameters: width=%d, height=%d, format=%d, mipCount=%d\n", width, height, format, mipCount);
        return;
    }
    UnityTexDesc desc;
    desc.width = width;
    desc.height = height;
    desc.format = format;
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

static IUnityInterfaces* gsUnityInterface = nullptr;
static void UNITY_INTERFACE_API OnGraphicsDeviceEvent(UnityGfxDeviceEventType eventType)
{
    switch (eventType)
    {   
    case kUnityGfxDeviceEventInitialize:
    {
        auto* pGfx = gsUnityInterface->Get<IUnityGraphics>();
        if (pGfx->GetRenderer() == kUnityGfxRendererD3D11)
        {
            gs_api = CreateUnityD3D11(gsUnityInterface);
			gs_api->initialize();
		}
		else if (pGfx->GetRenderer() == kUnityGfxRendererVulkan)
		{
			gs_api = CreateUnityVulkan(gsUnityInterface);
			gs_api->initialize();
        }
        else if (pGfx->GetRenderer() == kUnityGfxRendererD3D12)
        {
            gs_api = CreateUnityD3D12(gsUnityInterface);
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
    gsUnityInterface = unityInterfaces;
	auto* pGfx = gsUnityInterface->Get<IUnityGraphics>();
	pGfx->RegisterDeviceEventCallback(OnGraphicsDeviceEvent);
    OnGraphicsDeviceEvent(kUnityGfxDeviceEventInitialize);
#if 0
    if (pGfx->GetRenderer() == kUnityGfxRendererNull)
    {
        if (IUnityGraphicsVulkanV2* vulkanInterface = gsUnityInterface->Get<IUnityGraphicsVulkanV2>())
            vulkanInterface->AddInterceptInitialization(InterceptVulkanInitialization, NULL, 0);
        else if (IUnityGraphicsVulkan* vulkanInterface = gsUnityInterface->Get<IUnityGraphicsVulkan>())
            vulkanInterface->InterceptInitialization(InterceptVulkanInitialization, NULL);
    }
#endif
}


extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
UnityPluginUnload()
{
    if (gs_api)
    {
        delete gs_api;
        gs_api = nullptr;
    }
}

static void UNITY_INTERFACE_API
OnRenderEvent(int eventID) {}

extern "C" UnityRenderingEvent UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
GetRenderEventFunc(){ return OnRenderEvent; }

extern "C" void UNITY_INTERFACE_EXPORT
FBridge_LaunchBakerThread(
    void* pSceneData,
    int bytes,
    uint32_t timeout,
    bool headless,
    bool bUseDebugScene)
{
    if (gs_api && (bUseDebugScene || (pSceneData && bytes)))
    {
        if (gs_api->IsBakerThreadRunning())
        {
            gs_api->Log(donut::log::Severity::Error, "Baker thread is already running, please exit firstly\n");
        }
        else
        {
            gs_api->gSetupData.pSceneData = (void*)pSceneData;
            gs_api->gSetupData.sceneDataBytes = bytes;
            gs_api->LaunchBakerThread(headless);
            gs_api->WaitForBakerThread(timeout);
        }
    }
    else
    {
        donut::log::error("Failed to launch baker thread. State=%p, headless=%d, pSceneData=%p, bytes=%d, bUseDebugScene=%d\n",
            gs_api, headless, pSceneData, bytes, bUseDebugScene);
    }
}

extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_ExitBakerThread(bool bForce) { if (gs_api) { gs_api->ExitBakerThread(bForce); } }
extern "C" bool UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_IsBakerThreadRunning(bool& bRunning) { return gs_api ? gs_api->IsBakerThreadRunning() : false; }
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_DoTask(uint32_t type, uint8_t* ptr, uint32_t bytes) { if (gs_api) { gs_api->PushTask((FBridgeTaskType)type, ptr, bytes); } }
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_FlushTasks(uint64_t timeout) { if (gs_api) { gs_api->FlushTasks(timeout); } }
extern "C" void UNITY_INTERFACE_EXPORT UNITY_INTERFACE_API
FBridge_SetLogCallback(void* ptr) { if (gs_api) { gs_api->SetLogCallback(ptr); } }

