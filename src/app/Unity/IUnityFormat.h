#include <d3d11.h>
#include <d3d11_1.h>
#include <dxgi1_3.h>

//#include <vulkan/vulkan.h>
//#include <vulkan/vulkan_win32.h>
enum TextureFormat
{
    kTexFormatUnknown = -1,
    kTexFormatNone = 0,

    kTexFormatAlpha8 = 1,           // In memory: A8U
    kTexFormatARGB4444 = 2,         // In memory: A4U,R4U,G4U,B4U; 0xBGRA if viewed as 16 bit word on little-endian, equivalent to VK_FORMAT_R4G4B4A4_UNORM_PACK16, Pixel layout depends on endianness, A4;R4;G4;B4 going from high to low bits.
    kTexFormatRGB24 = 3,            // In memory: R8U,G8U,B8U
    kTexFormatRGBA32 = 4,           // In memory: R8U,G8U,B8U,A8U; 0xAABBGGRR if viewed as 32 bit word on little-endian. Generally preferred for 32 bit uncompressed data.
    kTexFormatARGB32 = 5,           // In memory: A8U,R8U,G8U,B8U; 0xBBGGRRAA if viewed as 32 bit word on little-endian
    kTexFormatARGBFloat = 6,        // only for internal use at runtime
    kTexFormatRGB565 = 7,           // In memory: R5U,G6U,B5U; 0xBGR if viewed as 16 bit word on little-endian, equivalent to VK_FORMAT_R5G6B5_UNORM_PACK16, Pixel layout depends on endianness, R5;G6;B5 going from high to low bits
    kTexFormatBGR24 = 8,            // In memory: B8U,G8U,R8U
    kTexFormatR16 = 9,              // In memory: R16U

    // DXT/S3TC compression
    kTexFormatDXT1 = 10,            // aka BC1
    kTexFormatDXT3 = 11,            // aka BC2
    kTexFormatDXT5 = 12,            // aka BC3

    kTexFormatRGBA4444 = 13,        // In memory: A4U,R4U,G4U,B4U; 0xARGB if viewed as 16 bit word on little-endian, Pixel layout depends on endianness, R4;G4;B4;A4 going from high to low bits

    kTexFormatBGRA32    = 14,       // In memory: B8U,G8U,R8U,A8U; 0xAARRGGBB if viewed as 32 bit word on little-endian. Used by some WebCam implementations.

    // float/half texture formats
    kTexFormatRHalf = 15,           // In memory: R16F
    kTexFormatRGHalf = 16,          // In memory: R16F,G16F
    kTexFormatRGBAHalf = 17,        // In memory: R16F,G16F,B16F,A16F
    kTexFormatRFloat = 18,          // In memory: R32F
    kTexFormatRGFloat = 19,         // In memory: R32F,G32F
    kTexFormatRGBAFloat = 20,       // In memory: R32F,G32F,B32F,A32F

    kTexFormatYUY2 = 21,            // YUV format, can be used for video streams.

    // Three partial-precision floating-point numbers encoded into a single 32-bit value all sharing the same
    // 5-bit exponent (variant of s10e5, which is sign bit, 10-bit mantissa, and 5-bit biased(15) exponent).
    // There is no sign bit, and there is a shared 5-bit biased(15) exponent and a 9-bit mantissa for each channel.
    kTexFormatRGB9e5Float = 22,

    kTexFormatRGBFloat  = 23,       // Editor only format (used for saving HDR)

    // DX10/DX11 (aka BPTC/RGTC) compressed formats
    kTexFormatBC6H  = 24,           // RGB HDR compressed format, unsigned.
    kTexFormatBC7   = 25,           // HQ RGB(A) compressed format.
    kTexFormatBC4   = 26,           // One-component compressed format, 0..1 range.
    kTexFormatBC5   = 27,           // Two-component compressed format, 0..1 range.

    // Crunch compression
    kTexFormatDXT1Crunched = 28,    // DXT1 Crunched
    kTexFormatDXT5Crunched = 29,    // DXT5 Crunched

    // PowerVR / iOS PVRTC compression
    kTexFormatPVRTC_RGB2 = 30,
    kTexFormatPVRTC_RGBA2 = 31,
    kTexFormatPVRTC_RGB4 = 32,
    kTexFormatPVRTC_RGBA4 = 33,

    // OpenGL ES 2.0 ETC
    kTexFormatETC_RGB4 = 34,

    // EAC and ETC2 compressed formats, in OpenGL ES 3.0
    kTexFormatEAC_R = 41,
    kTexFormatEAC_R_SIGNED = 42,
    kTexFormatEAC_RG = 43,
    kTexFormatEAC_RG_SIGNED = 44,
    kTexFormatETC2_RGB = 45,
    kTexFormatETC2_RGBA1 = 46,
    kTexFormatETC2_RGBA8 = 47,

    // ASTC. The RGB and RGBA formats are internally identical.
    // before we had kTexFormatASTC_RGB_NxN and kTexFormatASTC_RGBA_NxN, thats why we have hole here
    kTexFormatASTC_4x4 = 48,
    kTexFormatASTC_5x5 = 49,
    kTexFormatASTC_6x6 = 50,
    kTexFormatASTC_8x8 = 51,
    kTexFormatASTC_10x10 = 52,
    kTexFormatASTC_12x12 = 53,
    // [54..59] were taken by kTexFormatASTC_RGBA_NxN

    // Nintendo 3DS
    kTexFormatETC_RGB4_3DS = 60,
    kTexFormatETC_RGBA8_3DS = 61,

    kTexFormatRG16 = 62,
    kTexFormatR8 = 63,

    // Crunch compression for ETC format
    kTexFormatETC_RGB4Crunched = 64,
    kTexFormatETC2_RGBA8Crunched = 65,

    kTexFormatASTC_HDR_4x4 = 66,
    kTexFormatASTC_HDR_5x5 = 67,
    kTexFormatASTC_HDR_6x6 = 68,
    kTexFormatASTC_HDR_8x8 = 69,
    kTexFormatASTC_HDR_10x10 = 70,
    kTexFormatASTC_HDR_12x12 = 71,

    // 16-bit raw integer formats
    kTexFormatRG32 = 72,
    kTexFormatRGB48 = 73,
    kTexFormatRGBA64 = 74,

    kTexFormatTotalCount,
};
//
//inline uint32_t get_vk_format(TextureFormat fmt)
//{
//    switch (fmt)
//    {
//    case kTexFormatAlpha8: return VK_FORMAT_R8_UNORM;
//    case kTexFormatARGB4444: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
//    case kTexFormatRGB24: return VK_FORMAT_R8G8B8_UNORM;
//    case kTexFormatRGBA32: return VK_FORMAT_R8G8B8A8_UNORM;
//    case kTexFormatARGB32: return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
//    case kTexFormatARGBFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
//    case kTexFormatRGB565: return VK_FORMAT_R5G6B5_UNORM_PACK16;
//    case kTexFormatBGR24: return VK_FORMAT_B8G8R8_UNORM;
//    case kTexFormatR16: return VK_FORMAT_R16_UNORM;
//    case kTexFormatDXT1: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
//    case kTexFormatDXT3: return VK_FORMAT_BC2_UNORM_BLOCK;
//    case kTexFormatDXT5: return VK_FORMAT_BC3_UNORM_BLOCK;
//    case kTexFormatRGBA4444: return VK_FORMAT_B4G4R4A4_UNORM_PACK16;
//    case kTexFormatBGRA32: return VK_FORMAT_B8G8R8A8_UNORM;
//    case kTexFormatRHalf: return VK_FORMAT_R16_SFLOAT;
//    case kTexFormatRGHalf: return VK_FORMAT_R16G16_SFLOAT;
//    case kTexFormatRGBAHalf: return VK_FORMAT_R16G16B16A16_SFLOAT;
//    case kTexFormatRFloat: return VK_FORMAT_R32_SFLOAT;
//    case kTexFormatRGFloat: return VK_FORMAT_R32G32_SFLOAT;
//    case kTexFormatRGBAFloat: return VK_FORMAT_R32G32B32A32_SFLOAT;
//    case kTexFormatYUY2: return VK_FORMAT_R8G8_UNORM;
//    case kTexFormatRGB9e5Float: return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
//    case kTexFormatRGBFloat: return VK_FORMAT_R32G32B32_SFLOAT;
//    case kTexFormatBC6H: return VK_FORMAT_BC6H_UFLOAT_BLOCK;
//    case kTexFormatBC7: return VK_FORMAT_BC7_UNORM_BLOCK;
//    case kTexFormatBC4: return VK_FORMAT_BC4_UNORM_BLOCK;
//    case kTexFormatBC5: return VK_FORMAT_BC5_UNORM_BLOCK;
//    case kTexFormatDXT1Crunched: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
//    case kTexFormatDXT5Crunched: return VK_FORMAT_BC3_UNORM_BLOCK;
//    case kTexFormatPVRTC_RGB2: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
//    case kTexFormatPVRTC_RGBA2: return VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG;
//    case kTexFormatPVRTC_RGB4: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
//    case kTexFormatPVRTC_RGBA4: return VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG;
//    case kTexFormatETC_RGB4: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
//    case kTexFormatEAC_R: return VK_FORMAT_EAC_R11_UNORM_BLOCK;
//    case kTexFormatEAC_R_SIGNED: return VK_FORMAT_EAC_R11_SNORM_BLOCK;
//    case kTexFormatEAC_RG: return VK_FORMAT_EAC_R11G11_UNORM_BLOCK;
//    case kTexFormatEAC_RG_SIGNED: return VK_FORMAT_EAC_R11G11_SNORM_BLOCK;
//    case kTexFormatETC2_RGB: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
//    case kTexFormatETC2_RGBA1: return VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK;
//    case kTexFormatETC2_RGBA8: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
//    case kTexFormatASTC_4x4: return VK_FORMAT_ASTC_4x4_UNORM_BLOCK;
//    case kTexFormatASTC_5x5: return VK_FORMAT_ASTC_5x5_UNORM_BLOCK;
//    case kTexFormatASTC_6x6: return VK_FORMAT_ASTC_6x6_UNORM_BLOCK;
//    case kTexFormatASTC_8x8: return VK_FORMAT_ASTC_8x8_UNORM_BLOCK;
//    case kTexFormatASTC_10x10: return VK_FORMAT_ASTC_10x10_UNORM_BLOCK;
//    case kTexFormatASTC_12x12: return VK_FORMAT_ASTC_12x12_UNORM_BLOCK;
//    case kTexFormatETC_RGB4_3DS: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
//    case kTexFormatETC_RGBA8_3DS: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
//    case kTexFormatRG16: return VK_FORMAT_R16G16_UNORM;
//    case kTexFormatR8: return VK_FORMAT_R8_UNORM;
//    case kTexFormatETC_RGB4Crunched: return VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK;
//    case kTexFormatETC2_RGBA8Crunched: return VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK;
//    case kTexFormatASTC_HDR_4x4: return VK_FORMAT_ASTC_4x4_SRGB_BLOCK;
//    case kTexFormatASTC_HDR_5x5: return VK_FORMAT_ASTC_5x5_SRGB_BLOCK;
//    case kTexFormatASTC_HDR_6x6: return VK_FORMAT_ASTC_6x6_SRGB_BLOCK;
//    case kTexFormatASTC_HDR_8x8: return VK_FORMAT_ASTC_8x8_SRGB_BLOCK;
//    case kTexFormatASTC_HDR_10x10: return VK_FORMAT_ASTC_10x10_SRGB_BLOCK;
//    case kTexFormatASTC_HDR_12x12: return VK_FORMAT_ASTC_12x12_SRGB_BLOCK;
//    case kTexFormatRG32: return VK_FORMAT_R16G16_UNORM;
//    case kTexFormatRGB48: return VK_FORMAT_R16G16B16_UNORM;
//    case kTexFormatRGBA64: return VK_FORMAT_R16G16B16A16_UNORM;
//    default: return VK_FORMAT_UNDEFINED;
//    }
//}
//
//inline uint32_t get_dxgi_format(TextureFormat fmt)
//{
//    switch (fmt)
//    {
//    case kTexFormatAlpha8: return DXGI_FORMAT_R8_UNORM;
//    case kTexFormatARGB4444: return DXGI_FORMAT_B4G4R4A4_UNORM;
//    case kTexFormatRGBA32: return DXGI_FORMAT_R8G8B8A8_UNORM;
//    case kTexFormatARGBFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;
//    case kTexFormatR16: return DXGI_FORMAT_R16_UNORM;
//    case kTexFormatDXT1: return DXGI_FORMAT_BC1_UNORM;
//    case kTexFormatDXT3: return DXGI_FORMAT_BC2_UNORM;
//    case kTexFormatDXT5: return DXGI_FORMAT_BC3_UNORM;
//    case kTexFormatRGBA4444: return DXGI_FORMAT_B4G4R4A4_UNORM;
//    case kTexFormatBGRA32: return DXGI_FORMAT_B8G8R8A8_UNORM;
//    case kTexFormatRHalf: return DXGI_FORMAT_R16_FLOAT;
//    case kTexFormatRGHalf: return DXGI_FORMAT_R16G16_FLOAT;
//    case kTexFormatRGBAHalf: return DXGI_FORMAT_R16G16B16A16_FLOAT;
//    case kTexFormatRFloat: return DXGI_FORMAT_R32_FLOAT;
//    case kTexFormatRGFloat: return DXGI_FORMAT_R32G32_FLOAT;
//    case kTexFormatRGBAFloat: return DXGI_FORMAT_R32G32B32A32_FLOAT;
//    case kTexFormatYUY2: return DXGI_FORMAT_R8G8_UNORM;
//    case kTexFormatRGB9e5Float: return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
//    case kTexFormatRGBFloat: return DXGI_FORMAT_R32G32B32_FLOAT;
//    case kTexFormatBC6H: return DXGI_FORMAT_BC6H_UF16;
//    case kTexFormatBC7: return DXGI_FORMAT_BC7_UNORM;
//    case kTexFormatBC4: return DXGI_FORMAT_BC4_UNORM;
//    case kTexFormatBC5: return DXGI_FORMAT_BC5_UNORM;
//    case kTexFormatDXT1Crunched: return DXGI_FORMAT_BC1_UNORM;
//    case kTexFormatDXT5Crunched: return DXGI_FORMAT_BC3_UNORM;
//    case kTexFormatRG16: return DXGI_FORMAT_R16G16_UNORM;
//    case kTexFormatR8: return DXGI_FORMAT_R8_UNORM;
//    case kTexFormatRG32: return DXGI_FORMAT_R16G16_UNORM;
//    case kTexFormatRGBA64: return DXGI_FORMAT_R16G16B16A16_UNORM;
//    case kTexFormatRGB24:
//    case kTexFormatARGB32:
//    case kTexFormatRGB565:
//    case kTexFormatBGR24:
//    case kTexFormatPVRTC_RGB2:
//    case kTexFormatPVRTC_RGBA2:
//    case kTexFormatPVRTC_RGB4:
//    case kTexFormatPVRTC_RGBA4:
//    case kTexFormatETC_RGB4:
//    case kTexFormatEAC_R:
//    case kTexFormatEAC_R_SIGNED:
//    case kTexFormatEAC_RG:
//    case kTexFormatEAC_RG_SIGNED:
//    case kTexFormatETC2_RGB:
//    case kTexFormatETC2_RGBA1:
//    case kTexFormatETC2_RGBA8:
//    case kTexFormatASTC_4x4: 
//    case kTexFormatASTC_5x5: 
//    case kTexFormatASTC_6x6: 
//    case kTexFormatASTC_8x8: 
//    case kTexFormatASTC_10x10:
//    case kTexFormatASTC_12x12:
//    case kTexFormatETC_RGB4_3DS:
//    case kTexFormatETC_RGBA8_3DS:
//    case kTexFormatETC_RGB4Crunched:
//    case kTexFormatETC2_RGBA8Crunched:
//    case kTexFormatASTC_HDR_4x4:
//    case kTexFormatASTC_HDR_5x5:
//    case kTexFormatASTC_HDR_6x6:
//    case kTexFormatASTC_HDR_8x8:
//    case kTexFormatASTC_HDR_10x10:
//    case kTexFormatASTC_HDR_12x12:
//    case kTexFormatRGB48:
//    default: return DXGI_FORMAT_UNKNOWN;
//    }
//}
