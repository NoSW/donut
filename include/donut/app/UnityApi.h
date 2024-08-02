#pragma once
#include <cmath>
#include <filesystem>
#include <thread>
#undef min
#undef max

struct IUnityInterfaces;
struct IUnityGraphics;
namespace donut::app
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
    virtual void init(IUnityInterfaces* pUnity) = 0;
    virtual void destroy() = 0;
    virtual void* createSharedTexSrc(UnityTexDesc desc) = 0;
    virtual void* createSharedTexDst(UnityTexDesc desc) = 0;
    virtual void* createSharedBufSrc(UnityBufDesc desc) = 0;
    virtual void* createSharedBufDst(UnityBufDesc desc) = 0;
    virtual uint32_t getFormat(uint32_t format) = 0;
    virtual void getSharedBufferHandleWin32(void* nativeBuf, HANDLE& handle) = 0;
    virtual void getSharedTextureHandleWin32(void* nativeTex, HANDLE& handle) = 0;

    static UnityApi* GetApiInstance();
public:

    void LaunchBakerThread(bool headless)
    {
        ExitBakerThread();
        if (HasValidPluginFolder())
        {
            mpBakeThread = std::make_unique<std::thread>([this, headless]()
            {
                mShouldExit = false;
                SetThreadDescription(GetCurrentThread(), L"FBakerThread");
                launchThreadPtr(headless);
            });
            mpBakeThread->detach();
        }
    }

    std::filesystem::path GetPluginFolder() { return mPluginFolder; }
    bool HasValidPluginFolder() { return std::filesystem::exists(mPluginFolder) && std::filesystem::is_directory(mPluginFolder); }
    void SetPluginFolder(char* pluginFolder) { mPluginFolder = pluginFolder; }
    void ExitBakerThread() { if (mpBakeThread) { mShouldExit = true; } }
    bool ShouldExit() { return mShouldExit; }
    static int (*launchThreadPtr)(bool);
protected:
    std::vector<void*> mOwnedTexPtrs;
    std::vector<void*> mOwnedBufPtrs;

protected:
    IUnityInterfaces* mpUnity = nullptr;
    IUnityGraphics* mpUnityGraphics = nullptr;
    std::unique_ptr<std::thread> mpBakeThread;
    bool mShouldExit = false;
    std::filesystem::path mPluginFolder;

};

} // namespace donut::app
