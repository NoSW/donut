#include "Unity/IUnityPlatformBase.h"
#include "Unity/IUnityFormat.h"
#include "Unity/IUnityGraphics.h"
#include "Unity/IUnityGraphicsD3D11.h"

#include <donut/unity/UnityApi.h>
#include <donut/core/log.h>
#include <donut/core/math/math.h>

#include <cmath>
#include <vector>
#include <mutex>
#include <algorithm>

namespace donut::unity
{

class UnityD3D11 : public UnityApi
{
public:
	UnityD3D11(IUnityInterfaces* pUnity) : UnityApi(pUnity) {}
    void initialize() override
    {
        mpDevice = mpUnity->Get<IUnityGraphicsD3D11>()->GetDevice();
    }
    
    void shutdown() override
    {
        UnityApi::shutdown();
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
	nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI(void*& pOutUnityDevice) const override
	{
		pOutUnityDevice = nullptr;
		return nvrhi::GraphicsAPI::D3D12;
	}

private:
    ID3D11Device* mpDevice = nullptr;
};

UnityApi* CreateUnityD3D11(IUnityInterfaces* pUnity) { return new UnityD3D11(pUnity); }

} // namespace donut::unity
