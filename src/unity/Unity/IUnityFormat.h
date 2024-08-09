#pragma once
namespace donut::unity
{

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

} // namespace donut::unity

