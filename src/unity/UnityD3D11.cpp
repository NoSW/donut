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

#include <dxgi1_3.h>

namespace donut::unity
{

uint32_t get_dxgi_format(TextureFormat fmt)
{
    switch (fmt)
    {
    case kTexFormatAlpha8: return DXGI_FORMAT_R8_UNORM;
    case kTexFormatARGB4444: return DXGI_FORMAT_B4G4R4A4_UNORM;
    case kTexFormatRGBA32: return DXGI_FORMAT_R8G8B8A8_UNORM;
    case kTexFormatARGBFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case kTexFormatR16: return DXGI_FORMAT_R16_UNORM;
    case kTexFormatDXT1: return DXGI_FORMAT_BC1_UNORM;
    case kTexFormatDXT3: return DXGI_FORMAT_BC2_UNORM;
    case kTexFormatDXT5: return DXGI_FORMAT_BC3_UNORM;
    case kTexFormatRGBA4444: return DXGI_FORMAT_B4G4R4A4_UNORM;
    case kTexFormatBGRA32: return DXGI_FORMAT_B8G8R8A8_UNORM;
    case kTexFormatRHalf: return DXGI_FORMAT_R16_FLOAT;
    case kTexFormatRGHalf: return DXGI_FORMAT_R16G16_FLOAT;
    case kTexFormatRGBAHalf: return DXGI_FORMAT_R16G16B16A16_FLOAT;
    case kTexFormatRFloat: return DXGI_FORMAT_R32_FLOAT;
    case kTexFormatRGFloat: return DXGI_FORMAT_R32G32_FLOAT;
    case kTexFormatRGBAFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;
    case kTexFormatYUY2: return DXGI_FORMAT_R8G8_UNORM;
    case kTexFormatRGB9e5Float: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
    case kTexFormatRGBFloat: return DXGI_FORMAT_R32G32B32_FLOAT;
    case kTexFormatBC6H: return DXGI_FORMAT_BC6H_UF16;
    case kTexFormatBC7: return DXGI_FORMAT_BC7_UNORM;
    case kTexFormatBC4: return DXGI_FORMAT_BC4_UNORM;
    case kTexFormatBC5: return DXGI_FORMAT_BC5_UNORM;
    case kTexFormatDXT1Crunched: return DXGI_FORMAT_BC1_UNORM;
    case kTexFormatDXT5Crunched: return DXGI_FORMAT_BC3_UNORM;
    case kTexFormatRG16: return DXGI_FORMAT_R16G16_UNORM;
    case kTexFormatR8: return DXGI_FORMAT_R8_UNORM;
    case kTexFormatRG32: return DXGI_FORMAT_R16G16_UNORM;
    case kTexFormatRGBA64: return DXGI_FORMAT_R16G16B16A16_UNORM;
    case kTexFormatRGB24:
    case kTexFormatARGB32:
    case kTexFormatRGB565:
    case kTexFormatBGR24:
    case kTexFormatPVRTC_RGB2:
    case kTexFormatPVRTC_RGBA2:
    case kTexFormatPVRTC_RGB4:
    case kTexFormatPVRTC_RGBA4:
    case kTexFormatETC_RGB4:
    case kTexFormatEAC_R:
    case kTexFormatEAC_R_SIGNED:
    case kTexFormatEAC_RG:
    case kTexFormatEAC_RG_SIGNED:
    case kTexFormatETC2_RGB:
    case kTexFormatETC2_RGBA1:
    case kTexFormatETC2_RGBA8:
    case kTexFormatASTC_4x4: 
    case kTexFormatASTC_5x5: 
    case kTexFormatASTC_6x6: 
    case kTexFormatASTC_8x8: 
    case kTexFormatASTC_10x10:
    case kTexFormatASTC_12x12:
    case kTexFormatETC_RGB4_3DS:
    case kTexFormatETC_RGBA8_3DS:
    case kTexFormatETC_RGB4Crunched:
    case kTexFormatETC2_RGBA8Crunched:
    case kTexFormatASTC_HDR_4x4:
    case kTexFormatASTC_HDR_5x5:
    case kTexFormatASTC_HDR_6x6:
    case kTexFormatASTC_HDR_8x8:
    case kTexFormatASTC_HDR_10x10:
    case kTexFormatASTC_HDR_12x12:
    case kTexFormatRGB48:
    default: return DXGI_FORMAT_UNKNOWN;
    }
}


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

    uint32_t getNativeFormat(uint32_t format) override { return get_dxgi_format((TextureFormat)format); }

    void* createSharedTexSrc(UnityTexDesc inDesc) override
    {
        D3D11_TEXTURE2D_DESC desc = {};
        uint32_t bindFlags = 0;
        if (inDesc.bReadable) { bindFlags |= D3D11_BIND_SHADER_RESOURCE; }
        if (inDesc.bWriteable) { bindFlags |= D3D11_BIND_RENDER_TARGET | D3D11_BIND_UNORDERED_ACCESS; }
        desc.BindFlags = bindFlags;
        desc.CPUAccessFlags = 0;
        desc.Format = (DXGI_FORMAT)getNativeFormat(inDesc.format);
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
	    	DONUT_LOG_ERROR("Failed to create d3d11 texture for sharing.\n");
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
	    	    DONUT_LOG_ERROR("Failed to create d3d11 shared handle.\n");
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
            DONUT_LOG_ERROR("Failed to create texture from shared handle.\n");
        }
        else
        {
            ID3D11Texture2D* retTex = nullptr;
            if (FAILED(resource->QueryInterface(IID_PPV_ARGS(&retTex))))
            {
                DONUT_LOG_ERROR("Failed to query D3D11Texture.\n");
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
	nvrhi::GraphicsAPI GetPreferredAPIByUnityGfxAPI() const override { return nvrhi::GraphicsAPI::D3D12; }

private:
    ID3D11Device* mpDevice = nullptr;
};

UnityApi* CreateUnityD3D11(IUnityInterfaces* pUnity) { return new UnityD3D11(pUnity); }

} // namespace donut::unity
