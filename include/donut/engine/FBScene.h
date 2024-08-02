#pragma once
#include "Scene.h"

namespace donut::engine
{
using float2 = dm::float2;
using float3 = dm::float3;
using float4 = dm::float4;
class SceneTypeFactory;
class FBScene
{
public:

    static constexpr int32_t kInvalidUnityID = 0;
    static constexpr int kExpectedIndexBytes = 4;
    static constexpr int kMaxSceneNameBytes = 512;
    static constexpr int kEndMagicNumber = 0x20240716;

    enum class LightType
    {
        Spot = 0,
        Directional = 1,
        Point = 2,
        Area = 3,
        Rectangle = 3,
        Disc = 4
    };


    enum FBGlobalSettingFlags : uint32_t
    {
        // single-op populated into FBGlobalSetting
        NoneChanged = 0x0,
        EmptyChanged = 0x1,
        CameraChanged = 0x2,
        EnvChanged = 0x4,
        DenosierScaleChanged = 0x8,
        FullBakeSampleCountChanged = 0x10,
        CameraDistanceHTChanged = 0x20,
        BounceCountChanged = 0x40,
        StartFullBake = 0x80,
        DeleteNode = 0x100,
        ObjectEmissionBoostChanged = 0x200,
        MaterialEmissionBoostChanged = 0x400,
        LightmapResolutionChanged = 0x800,

        // mutil-ops
        TransformChanged = 0x10000000,
        LightChanged = 0x20000000,
        DeleteNodes = 0x30000000,
    };

    static bool IsOneChangedFlag(uint32_t flag, uint32_t& changedFlag, uint32_t& changedCount)
    {
        bool bIsOneChanegd = ((flag >> 28) == 0);
        if (bIsOneChanegd)
        {
            changedFlag = flag;
            changedCount = 1;
            return true;
        }
        else
        {
            changedFlag = flag & 0xF0000000;
            changedCount = flag & 0x0FFFFFFF;
            return false;
        }
    }

    enum class FBSettingFlags : int32_t
    {
        None = 0x0,
        ExportTexToDisk = 0x1,
        ExportBufToScene = 0x2,
        IsExportSceneToDisk = 0x4,
        Use32BitIndices = 0x8,
        use16BitIndices = 0x10,
        HasTerrainData = 0x20,
        HasSplitVertexData = 0x40,
    };

    struct FBTransform
    {
        int32_t unityID;
        float3 position;
        float4 rotation;
        float3 scale;
        int32_t padding;
    };

    struct FBGlobalSetting
    {
        // config
        float4 envTintIntensity;
        float4 cameraRotation;
        float3 cameraPosition;
        float nearPlane;

        float farPlane;
        float aspectRatio;
        float fov;
        int32_t frameWidth;
        
        int32_t frameHeight;
        float denosierScale;
        int32_t fullBakeSampleCount;
        int32_t cameraDistanceHT;

        int32_t bounceCount;
        float3 minBoundsIV;
        float3 maxBoundsIV;
        // setup
        uint32_t lightCount;
    
        uint32_t meshCount;
        uint32_t textureCount;
        uint32_t materialCount;
        uint32_t objectCount;

        int32_t unityPID;
        int32_t vertexCount;
        int32_t indexCount;
        int32_t flags;

        float3 terrainMinPoint;
        float padding1;
        float3 terrainSize;
        uint32_t terrainMeshCount;

        char immeFolderPath[FBScene::kMaxSceneNameBytes];

        bool use32BitIndices() const { return (flags & (int32_t)FBSettingFlags::Use32BitIndices) == (int32_t)FBSettingFlags::Use32BitIndices; }
        bool use16BitIndices() const { return (flags & (int32_t)FBSettingFlags::use16BitIndices) == (int32_t)FBSettingFlags::use16BitIndices; }
        bool isExportSceneToDisk() const { return (flags & (int32_t)FBSettingFlags::IsExportSceneToDisk) == (int32_t)FBSettingFlags::IsExportSceneToDisk; }
        bool isExportTexToDisk() const { return (flags & (int32_t)FBSettingFlags::ExportTexToDisk) == (int32_t)FBSettingFlags::ExportTexToDisk; }
        bool isExportBufToScene() const { return (flags & (int32_t)FBSettingFlags::ExportBufToScene) == (int32_t)FBSettingFlags::ExportBufToScene; }
        bool hasTerrainData() { return (flags & (int32_t)FBSettingFlags::HasTerrainData) == (int32_t)FBSettingFlags::HasTerrainData; }
        bool hasSplitVertexData() { return (flags & (int32_t)FBSettingFlags::HasSplitVertexData) == (int32_t)FBSettingFlags::HasSplitVertexData; }

        int32_t GetDeletedNodeUnityID() const { return unityPID; }
        void GetMaterialEmissionBoost(int32_t& unityID, float4& newBoost) const  { unityID = materialCount; newBoost = float4(minBoundsIV, padding1); }
        void GetObjectEmissionBoost(int32_t& unityID, float& newBoost) const { unityID = objectCount; newBoost = padding1; }
        void GetLightmapResolution(int32_t& unityID, int32_t& newResolution) const { unityID = vertexCount; newResolution = indexCount; }
        
    };

    enum FBLightFlags
    {
        IsActive                    = 0x1,
        CastShadow                  = 0x2,
        OverwriteTemperature        = 0x4,
        OverwriteIndirectMultiplier = 0x8,
        BakeDirectLighting          = 0x10,
        UseCookie                   = 0x20,
    };

    struct FBLight
    {
        int32_t unityID  = kInvalidUnityID;
        int32_t type;
        int32_t flags;
        float attenuationRadius;
        float3 color;
        float intensity;
        float4 extra;
        float indirectMultiplier;
        float colorTemperature;

        bool isActive() const
        {
            return (flags & IsActive) != 0;
        }

        bool isCastShadow() const
        {
            return (flags & CastShadow) != 0;
        }

        bool isOverwriteTemperature() const
        {
            return (flags & OverwriteTemperature) != 0;
        }

        bool isOverwriteIndirectMultiplier() const
        {
            return (flags & OverwriteIndirectMultiplier) != 0;
        }

        bool isBakeDirectLighting() const
        {
            return (flags & BakeDirectLighting) != 0;
        }

        bool isUseCookie() const
        {
            return (flags & UseCookie) != 0;
        }
    };

    enum FBMeshFlags
    {
        HasNone = 0x0,
        UseIndices16Bit = 0x1, // else 32 bit
        HasPosition = 0x2,
        HasNormal = 0x4,
        HasTangent = 0x8,
        HasUV = 0x10,
        HasUV2 = 0x20,
    };
    
    struct FBMesh
    {
        int32_t unityID  = kInvalidUnityID;
        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;
        uint32_t flags = 0;

        bool hasPosition() const { return (flags & HasPosition) != 0; }
        bool hasNormal() const { return (flags & HasNormal) != 0; }
        bool hasTangent() const { return (flags & HasTangent) != 0; }
        bool hasUV() const { return (flags & HasUV) != 0; }
        bool hasUV2() const { return (flags & HasUV2) != 0; }
        bool useIndices16Bit() const { return (flags & UseIndices16Bit) != 0; }
    };

    enum FBMaterialType
    {
        UnknownMaterial = 0,
        PanguScenePBR,
        TreeLeaf,
        Terrain,
        MeshTilling,
    };

    enum FBTextureFlags : uint32_t
    {
        TF_MaterialTexture = 0x1,
        TF_ImmediateData = 0x2,
        TF_sRGB = 0x4,
    };

    struct FBTexture
    {
        int32_t unityID = kInvalidUnityID;
        int32_t width = 0;
        int32_t height = 0;
        int32_t mipCount = 0;
        int32_t format = 0;
        int32_t mipmapStart = 0;
        int32_t flags = 0;
        int32_t handle = 0;

        bool IsMaterialTexture() const { return (flags & TF_MaterialTexture) != 0; }
        bool IsImmediateData() const { return (flags & TF_ImmediateData) != 0; }
        bool IsSRGB() const { return (flags & TF_sRGB) != 0; }
    };

    bool IsMaterialTexture(int index) const { return index >= Scene::kSharedTextureName_Count; }

    struct FBMaterial
    {
        int32_t unityID = kInvalidUnityID;
        int32_t type = UnknownMaterial;
        int32_t textureIds[6] { };
        float parameters[16] { };

    };

    enum class FBObjectFlags
    {
        None = 0,
        Mesh = 0x1,
        Light = 0x2,
        Terrain = 0x4,
    };

    struct FBObject
    {
        int32_t unityID  = kInvalidUnityID;
        int32_t parentUnityID = kInvalidUnityID;
        int32_t meshUnityID = kInvalidUnityID;
        int32_t materialUnityID = kInvalidUnityID;
        float4 rotation;
        float3 position;
        int32_t lightmapResolution = 0;
        float3 scale;
        float emissionBoost = 0.0f;
        uint32_t flags = (uint32_t)(FBObjectFlags::None);

        bool hasTerrain() const
        {
            return (flags & (uint32_t)FBObjectFlags::Terrain) == (uint32_t)FBObjectFlags::Terrain;
        }

        bool hasMesh() const
        {
            return (flags & (uint32_t)FBObjectFlags::Mesh) == (uint32_t)FBObjectFlags::Mesh;
        }

        bool hasLight() const
        {
            return (flags & (uint32_t)FBObjectFlags::Light) == (uint32_t)FBObjectFlags::Light;
        }
    };

    // deserialize begin
    FBGlobalSetting setting;
    std::vector<FBLight> lights;
    std::vector<FBMesh> meshes;
    std::vector<FBTexture> textures;
    std::vector<FBMaterial> materials;
    std::vector<FBObject> objects;

    std::vector<float3> positions;
    std::vector<uint32_t> normals;
    std::vector<uint32_t> tangents;
    std::vector<float2> uv0;
    std::vector<float2> uv2;
    std::vector<uint32_t> indexData;
    // deserialize end

    bool isLoadMaterialTextureFromDisk() const { return setting.isExportTexToDisk(); }

public:
    FBScene(const uint8_t* pData, uint32_t size);
    bool GetSceneData(
        std::shared_ptr<SceneTypeFactory> pSTF,
        TextureCache& textureCache,
        SceneLoadingStats& stats,
        tf::Executor* executor,
        SceneImportResult& result,
        std::vector<void*>& outSharedHandles);
};

}
