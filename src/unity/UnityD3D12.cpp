#include "Unity/IUnityPlatformBase.h"
#include "Unity/IUnityFormat.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D12.h"

#include <donut/unity/UnityApi.h>
#include <donut/core/log.h>
#include <donut/core/math/math.h>

#include <cmath>
#include <vector>
#include <mutex>
#include <algorithm>

namespace donut::unity
{
uint32_t get_dxgi_format(TextureFormat fmt);

    class UnityD3D12 : public UnityApi
    {
    public:
        UnityD3D12(IUnityInterfaces* pUnity) : UnityApi(pUnity) {}
        void initialize() override
        {
            //mpDevice = mpUnity->Get<IUnityGraphicsD3D12>()->GetDevice();
        }

        void shutdown() override
        {
            UnityApi::shutdown();
        }

        uint32_t getNativeFormat(uint32_t format) override { return get_dxgi_format((TextureFormat)format); }

        void* createSharedTexSrc(UnityTexDesc inDesc) override
        {

            return nullptr;
        }

        void* createSharedTexDst(UnityTexDesc desc) override
        {
     
            return nullptr;
        }

        void* createSharedBufSrc(UnityBufDesc desc) override { return nullptr; }
        void* createSharedBufDst(UnityBufDesc desc) override { return nullptr; }
        nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI() const override { return nvrhi::GraphicsAPI::D3D12; }

    private:
        ID3D12Device* mpDevice = nullptr;
    };

    UnityApi* CreateUnityD3D12(IUnityInterfaces* pUnity) { return new UnityD3D12(pUnity); }

} // namespace donut::unity
