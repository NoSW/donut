#pragma once
#include <donut/core/log.h>
#include <donut/unity/FBScene.h>
#include <nvrhi/nvrhi.h>
#include <cmath>
#include <filesystem>
#include <thread>
#include <mutex>
#include <queue>

namespace donut::unity
{

enum FBridgeTaskType : uint32_t
{
    kBridge_None = 0,
    kBridge_UpdateCamera = 0x1,
    kBridge_UpdateEnvironment = 0x2,
    kBridge_UpdateDenosierOption = 0x4,
    kBridge_UpdateDistanceHTOption = 0x8,
    kBridge_UpdateBounceCountOptions = 0x10,
    kBridge_UpdateObjectEmissionBoost = 0x20,
    kBridge_UpdateMaterialEmissionColorIntensity = 0x40,
    kBridge_UpdateMeshInstanceLightmapResolution = 0x80,
    kBridge_UpdateOrAddOneLight = 0x100,
    kBridge_UpdateOneTransform = 0x200,
    kBridge_DeleteOneNode = 0x400,
    kBridge_DeleteOneLight = 0x800,
    kBridge_RunFullBakeLightmap = 0x1000,
    kBridge_RunFullBakeVLM = 0x2000,
    kBridge_UpdateOrAddMultiLights = 9,
    kBridge_UpdateMultiTransforms = 10,
    kBridge_DeleteMultiNodes = 11,
    kBridge_DeleteMultiLights = 12,
};


inline bool isValidBridgeType(FBridgeTaskType type)
{
    return type != FBridgeTaskType::kBridge_None;
}

struct BridgeTask
{
    void set(FBridgeTaskType type, uint8_t* ptr, uint32_t bytes);
    void reset();
    uint32_t elementCount() const;
    static bool CalcElementCount(FBridgeTaskType type, uint32_t bytes, uint32_t& count);
    
    FBridgeTaskType type = kBridge_None;
    uint32_t bytes = 0;
    uint8_t* pData = 0;
};

class TaskScheduler
{
public:
    TaskScheduler();
    ~TaskScheduler();

    void PushTask(uint32_t type, uint8_t* ptr, uint32_t bytes);
    bool PopTask(BridgeTask& task);
    size_t GetApproxTaskCount() const;

private:
    void* pTaskQueue = nullptr;
    FBScene::FBGlobalSetting mCachedChange;
};

} // namespace donut::unity

