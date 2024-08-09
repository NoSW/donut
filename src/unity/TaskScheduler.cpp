#include <donut/unity/TaskScheduler.h>
#include <donut/unity/FBScene.h>
#include <readerwriterqueue.h>

namespace donut::unity
{
using BridgeTaskQueue = moodycamel::ReaderWriterQueue<BridgeTask>;

void BridgeTask::set(FBridgeTaskType type, uint8_t* ptr, uint32_t bytes)
{
    this->type = type;
    if (bytes > 0)
    {
        this->bytes = bytes;
        this->pData = new uint8_t[bytes];
        memcpy(this->pData, ptr, bytes);
    }
};

void BridgeTask::reset()
{
    if (pData &&
        (type == kBridge_UpdateOrAddMultiLights ||
        type == kBridge_UpdateMultiTransforms ||
        type == kBridge_DeleteMultiNodes ||
        type == kBridge_DeleteMultiLights))
    { delete[] pData; }
    pData = nullptr;
    bytes = 0;
    type = kBridge_None;
}

uint32_t BridgeTask::elementCount() const
{
    uint32_t  count = 0;
    return CalcElementCount(type, bytes, count) ? count : 0;
}


bool BridgeTask::CalcElementCount(FBridgeTaskType type, uint32_t bytes, uint32_t& count)
{
    uint32_t stride = 0;
    switch (type)
    {
        case kBridge_UpdateOrAddMultiLights: stride = sizeof(FBScene::FBLight); break;
        case kBridge_UpdateMultiTransforms: stride = sizeof(FBScene::FBTransform); break;
        case kBridge_DeleteMultiNodes:;
        case kBridge_DeleteMultiLights: stride = sizeof(UnityID); break;
        default: stride = 0; break;
    }

    if (stride == 0)
    {
        count = 0;
        if (bytes != sizeof(FBScene::FBGlobalSetting))
        {
            log::error("Invalid task bytes %d != %d for type %d\n", bytes, sizeof(FBScene::FBGlobalSetting), type);
            return false;
        }
    }
    else
    {
        if (bytes % stride)
        {
            log::error("Invalid task bytes %d for type %d\n", bytes, type);
            count = 0;
            return false;
        }
        else
        {
            count = bytes / stride;
        }
    }
    return true;
}

TaskScheduler::TaskScheduler()
{
    pTaskQueue = new BridgeTaskQueue(1024);
}

TaskScheduler::~TaskScheduler()
{
    auto* pQueue = (BridgeTaskQueue*)pTaskQueue;
    BridgeTask task;
    while(pQueue->try_dequeue(task))
    {
        task.reset();
    }
    delete pQueue;
}

void TaskScheduler::PushTask(uint32_t type, uint8_t* ptr, uint32_t bytes)
{
    const auto checkValidity = [type, ptr, bytes](uint32_t& count)
    {
        count = 0;
        if (!isValidBridgeType((FBridgeTaskType)type))
            return false;

        if (((bytes == 0) != (ptr == nullptr)))
            return false;

        if (bytes)
        {
            if (!BridgeTask::CalcElementCount((FBridgeTaskType)type, bytes, count))
                return false;
        }
        return true;
    };

    uint32_t elementCount = 0;
    if (checkValidity(elementCount))
    {
        if (elementCount == 0)
        {
            memcpy(&mCachedChange, ptr, bytes);
        }
        else
        {
            BridgeTask task;
            task.set((FBridgeTaskType)type, ptr, bytes);
            if (!((BridgeTaskQueue*)pTaskQueue)->try_enqueue(task))
            {
                log::error("Too many task wait to be processes, failed to push.\n");
                task.reset();
            }
        }
    }
    else
    {
        log::error("Invalid task type %d, bytes %d, ptr %p\n", type, bytes, ptr);
    }
}

bool TaskScheduler::PopTask(BridgeTask& task)
{
    const auto hight_bit = [](uint32_t v)
    {
        uint32_t r = 0;
        while (v)
        {
            v >>= 1;
            r++;
        }
        return r - 1;
    };
    
    if (pTaskQueue)
    {
        return ((BridgeTaskQueue*)pTaskQueue)->try_dequeue(task);
    } 
    else if (mCachedChange.flags)
    {
        task.type = (FBridgeTaskType)(1 << hight_bit(mCachedChange.flags));
        task.set(task.type, (uint8_t*)&mCachedChange, sizeof(FBScene::FBGlobalSetting));
        mCachedChange.flags &= ~(task.type);
    }

    return false;
}

size_t TaskScheduler::GetApproxTaskCount() const
{
    return ((BridgeTaskQueue*)pTaskQueue)->size_approx();
}
}

