#pragma once
#include <donut/core/log.h>
#include <donut/unity/TaskScheduler.h>
#include <nvrhi/nvrhi.h>
#include <cmath>
#include <filesystem>
#include <thread>
#include <mutex>
#undef min
#undef max

struct IUnityInterfaces;
struct IUnityGraphics;
namespace donut::unity
{
using HANDLE = void*;

struct BridgeSetupData
{
    void* handleUAVs[5] = { nullptr, nullptr, nullptr, nullptr, nullptr };
    const void* pSceneData = nullptr;
    const char* importFolder = nullptr;
    const char* exportFolder = nullptr;
    uint32_t sceneDataBytes = 0;
    void* pExternalDevice = nullptr;
};

struct UnityTexDesc
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t format = 0;
    uint32_t mipCount = 0;
    bool bReadable = true;
    bool bWriteable = true;
    HANDLE* pHandle = nullptr;
};

struct UnityBufDesc
{
    uint32_t size = 0;
    bool bReadable = true;
    bool bWriteable = true;
    HANDLE* pHandle = 0;
    void* pInitData = nullptr;
};

class UnityApi
{
public:
    UnityApi(IUnityInterfaces* pUnity);
    ~UnityApi();
    virtual void initialize()  {}
    virtual void shutdown() { ExitBakerThread(true); }
    virtual void* createSharedTexSrc(UnityTexDesc desc) = 0;
    virtual void* createSharedTexDst(UnityTexDesc desc) = 0;
    virtual void* createSharedBufSrc(UnityBufDesc desc) = 0;
    virtual void* createSharedBufDst(UnityBufDesc desc) = 0;
    virtual uint32_t getNativeFormat(uint32_t format) = 0;
    virtual bool checkUnityFormat(uint32_t format) { return getNativeFormat(format) != 0; }
    void SetLogCallback(void* p) { pUnityLogCallBake = (void(*)(int, const char*))p; }
    void Log(log::Severity severity, const char* message) const;
    static UnityApi* Ins();

public: // unity thread calls
    void LaunchBakerThread(bool headless);
    void ExitBakerThread(bool bForce);
    void WaitForBakerThread(long long timeout) { std::unique_lock<std::mutex> lock(mMutex); mCVar.wait_for(lock, std::chrono::seconds(timeout), [this] { return !IsBakerThreadRunning(); }); }
    bool IsBakerThreadRunning() const;
    bool RequestExitBakerThread() { return mRequestExitBakerThread; }
    void PushTask(uint32_t type, uint8_t* ptr, uint32_t bytes);
    void FlushTasks(uint64_t timeout);

public: // baker thread calls
    bool PopTask(BridgeTask& task) { return mTaskScheduler.PopTask(task); }
    size_t GetApproxTaskCount() const { return mTaskScheduler.GetApproxTaskCount(); }
    static int (*ThreadEntryPtr)(bool);
    void NotifyUnityMainThread() { mCVar.notify_all(); }
    virtual nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI() const { return nvrhi::GraphicsAPI::D3D12; }
	std::filesystem::path GetTempImportFolder() { return checkFolderPath(gSetupData.importFolder) ? std::filesystem::path(gSetupData.importFolder) : std::filesystem::path{}; }
    std::filesystem::path GetTempExportFolder() { return checkFolderPath(gSetupData.exportFolder) ? std::filesystem::path(gSetupData.exportFolder) : std::filesystem::path{}; }

private:
    static bool checkFolderPath(const char* path);
    
public:
    BridgeSetupData gSetupData;

protected:
    std::vector<void*> mOwnedTexPtrs;
    std::vector<void*> mOwnedBufPtrs;
	std::vector<void*> mDstTexPtrs;

    IUnityInterfaces* mpUnity = nullptr;
    IUnityGraphics* mpUnityGraphics = nullptr;
    TaskScheduler mTaskScheduler;

private:
    std::unique_ptr<std::thread> mpBakeThread;
	std::condition_variable mCVar;
	std::mutex mMutex;
    bool mRequestExitBakerThread = false;
    void (*pUnityLogCallBake)(int, const char*) = nullptr;
};

UnityApi* CreateUnityVulkan(IUnityInterfaces* pUnity);
UnityApi* CreateUnityD3D11(IUnityInterfaces* pUnity);
UnityApi* CreateUnityD3D12(IUnityInterfaces* pUnity);

} // namespace donut::app
