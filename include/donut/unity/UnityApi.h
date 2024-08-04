#pragma once
#include <donut/core/log.h>
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
protected:
    static uint32_t _clamp(uint32_t w, uint32_t h, uint32_t mipIndex)
    {
        return std::min(mipIndex, (uint32_t)std::log2(std::min(w, h)));
    }

    static void logError(const char* msg)
    {
        OutputDebugStringA(msg);
    }

public:
	UnityApi(IUnityInterfaces* pUnity) : mpUnity(pUnity) { }
    virtual void initialize()  {}
    virtual void shutdown() { ExitBakerThread(); }
    virtual void* createSharedTexSrc(UnityTexDesc desc) = 0;
    virtual void* createSharedTexDst(UnityTexDesc desc) = 0;
    virtual void* createSharedBufSrc(UnityBufDesc desc) = 0;
    virtual void* createSharedBufDst(UnityBufDesc desc) = 0;
    virtual uint32_t getFormat(uint32_t format) = 0;
    virtual void getSharedBufferHandleWin32(void* nativeBuf, HANDLE& handle) = 0;
    virtual void getSharedTextureHandleWin32(void* nativeTex, HANDLE& handle) = 0;

    static UnityApi* GetApiInstance();
public:

    void LaunchBakerThread(bool headless);
	void* GetSrcTexData() { return mOwnedTexPtrs.empty() ? nullptr : mOwnedTexPtrs.data(); }
	void* GetSrcBufData() { return mOwnedBufPtrs.empty() ? nullptr : mOwnedBufPtrs.data(); }
	void* GetDstTexData() { return mDstTexPtrs.empty() ? nullptr : mDstTexPtrs.data(); }
    virtual nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI(void*& pOutUnityDevice) const { return nvrhi::GraphicsAPI::D3D12; pOutUnityDevice = nullptr; }
    std::filesystem::path GetPluginFolder() { return mPluginFolder; }
	std::filesystem::path GetTempImportFolder() { return mTempImportFolder; }
	std::filesystem::path GetTempExportFolder() { return mTempExportFolder; }
    bool HasValidPluginFolder() { return std::filesystem::exists(mPluginFolder) && std::filesystem::is_directory(mPluginFolder); }
	bool HasValidTempImportFolder() { return std::filesystem::exists(mTempImportFolder) && std::filesystem::is_directory(mTempImportFolder); }
	bool HasValidTempExportFolder() { return std::filesystem::exists(mTempExportFolder) && std::filesystem::is_directory(mTempExportFolder); }
    void SetFolders(char* pluginFolder, char* tempImportFolder, char* tempExportFolder);
    void ExitBakerThread() { if (mpBakeThread) { mShouldExit = true; } }
    bool ShouldExit() { return mShouldExit; }
    static int (*ThreadEntryPtr)(bool);
protected:
    std::vector<void*> mOwnedTexPtrs;
    std::vector<void*> mOwnedBufPtrs;
	std::vector<void*> mDstTexPtrs;

public:
    IUnityInterfaces* mpUnity = nullptr;
    IUnityGraphics* mpUnityGraphics = nullptr;
    std::unique_ptr<std::thread> mpBakeThread;
	std::mutex mMutex;
	std::condition_variable mCVar;
    bool mShouldExit = false;
    std::filesystem::path mPluginFolder;
	std::filesystem::path mTempImportFolder;
	std::filesystem::path mTempExportFolder;

};

UnityApi* CreateUnityVulkan(IUnityInterfaces* pUnity);
UnityApi* CreateUnityD3D11(IUnityInterfaces* pUnity);
UnityApi* CreateUnityD3D12(IUnityInterfaces* pUnity);

} // namespace donut::app
