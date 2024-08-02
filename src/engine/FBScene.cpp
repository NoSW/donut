#include <donut/engine/FBScene.h>
#include <donut/engine/SceneGraph.h>
#include <donut/engine/TextureCache.h>
#include <donut/core/log.h>
#include "../app/Unity/IUnityFormat.h"
#include <unordered_map>
namespace donut::engine
{

FBScene::FBScene(const uint8_t* pData, uint32_t size)
{
    std::memcpy(&setting, pData, sizeof(FBGlobalSetting));
    log::info("FBScene {} MB", (double)size/(1024.0 * 1024.0));
    if (size < sizeof(FBGlobalSetting) + setting.lightCount * sizeof(FBLight) + setting.meshCount * sizeof(FBMesh) + setting.textureCount * sizeof(FBTexture) + setting.materialCount * sizeof(FBMaterial) + setting.objectCount * sizeof(FBObject))
    {
        log::error("Invalid: size()=%u, lightCount=%u, meshCount=%u, textureCount=%u, materialCount=%u, objectCount=%u", size, setting.lightCount, setting.meshCount, setting.textureCount, setting.materialCount, setting.objectCount);
    }
    else
    {
        if (!setting.hasSplitVertexData())
        {
            log::error("Invalid: hasSplitVertexData == false");
            return;
        }
        lights.resize(setting.lightCount);
        meshes.resize(setting.meshCount);
        textures.resize(setting.textureCount);
        materials.resize(setting.materialCount);
        objects.resize(setting.objectCount);
        size_t offset = sizeof(FBGlobalSetting);
        std::memcpy(lights.data(), pData + offset, sizeof(FBLight) * setting.lightCount);
        offset += sizeof(FBLight) * setting.lightCount;
        std::memcpy(meshes.data(), pData + offset, sizeof(FBMesh) * setting.meshCount);
        offset += sizeof(FBMesh) * setting.meshCount;
        std::memcpy(textures.data(), pData + offset, sizeof(FBTexture) * setting.textureCount);
        offset += sizeof(FBTexture) * setting.textureCount;
        std::memcpy(materials.data(), pData + offset, sizeof(FBMaterial) * setting.materialCount);
        offset += sizeof(FBMaterial) * setting.materialCount;
        std::memcpy(objects.data(), pData + offset, sizeof(FBObject) * setting.objectCount);
        offset += sizeof(FBObject) * setting.objectCount;

        positions.resize(setting.vertexCount);
        normals.resize(setting.vertexCount);
        tangents.resize(setting.vertexCount);
        uv0.resize(setting.vertexCount);
        uv2.resize(setting.vertexCount);
        indexData.resize(setting.indexCount);

        std::memcpy(positions.data(), pData + offset, sizeof(float3) * setting.vertexCount);
        offset += sizeof(float3) * setting.vertexCount;

        std::memcpy(normals.data(), pData + offset, sizeof(uint32_t) * setting.vertexCount);
        offset += sizeof(uint32_t) * setting.vertexCount;

        std::memcpy(tangents.data(), pData + offset, sizeof(uint32_t) * setting.vertexCount);
        offset += sizeof(uint32_t) * setting.vertexCount;

        std::memcpy(uv0.data(), pData + offset, sizeof(float2) * setting.vertexCount);
        offset += sizeof(float2) * setting.vertexCount;

        std::memcpy(uv2.data(), pData + offset, sizeof(float2) * setting.vertexCount);
        offset += sizeof(float2) * setting.vertexCount;

        std::memcpy(indexData.data(), pData + offset, sizeof(uint32_t) * setting.indexCount);
        offset += sizeof(uint32_t) * setting.indexCount;

        uint32_t endMagicNumber;
        std::memcpy(&endMagicNumber, pData + offset, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        assert(offset == size);
        if (endMagicNumber != kEndMagicNumber)
        {
            log::error("Invalid endMagicNumber: %u != %u", endMagicNumber, kEndMagicNumber);
        }
        else 
        {
            // put directional lights first
            std::sort(lights.begin(), lights.end(), [](const FBLight& a, const FBLight& b) { return a.type == (int32_t)LightType::Directional; });
            if (setting.hasTerrainData() && all(setting.terrainSize < 1e-4f))
            {
                log::error("Scene %s has terrain, but invalid terrain size", setting.immeFolderPath);
            }
        }

        log::info("isExportSceneToDisk=%d", setting.isExportSceneToDisk());
        log::info("isExportTexToDisk=%d", setting.isExportTexToDisk());
        log::info("isExportBufToScene=%d", setting.isExportBufToScene());
        log::info("use16BitIndices%d", setting.use16BitIndices());
        log::info("use32BitIndices=%d", setting.use32BitIndices());
    }
}

bool FBScene::GetSceneData(
    std::shared_ptr<SceneTypeFactory> pSTF,
    TextureCache& textureCache,
    SceneLoadingStats& stats,
    tf::Executor* executor,
    SceneImportResult& result,std::vector<void*>& outSharedHandles)
{
    if (!setting.hasSplitVertexData())
    {
        return false;
    }
    // textures
    std::unordered_map<int32_t, std::shared_ptr<LoadedTexture>> loadedTextures;
    auto load_texture = [this, &loadedTextures, &textureCache](int32_t unityID, bool sRGB)
    {
        if (unityID == kInvalidUnityID)
            return std::shared_ptr<LoadedTexture>(nullptr);

        auto it = loadedTextures.find(unityID);
        if (it != loadedTextures.end())
            return it->second;

        std::shared_ptr<LoadedTexture> loadedTexture;
        std::filesystem::path filePath = "/fbtextures/" + std::to_string(unityID) + ".dds";
        // TODO: add check for fbtextures folder
//#ifdef DONUT_WITH_TASKFLOW
//            if (executor)
//                loadedTexture = textureCache.LoadTextureFromFileAsync(filePath, sRGB, *executor);
//            else
//#endif
                loadedTexture = textureCache.LoadTextureFromFileDeferred(filePath, sRGB);
        loadedTextures[unityID] = loadedTexture;
        return loadedTexture;
    };

    for (const auto& tex : textures)
    {
        if (tex.IsMaterialTexture())
        {
           auto loadTex = load_texture(tex.unityID, tex.IsSRGB());
        }
        else
        {
            // TODO: created shared texture
        }
    }
    
    // material

    auto find_texture = [&loadedTextures, &load_texture](int32_t unityID, bool sRGB = false)
    {
        auto it = loadedTextures.find(unityID);
        if (it != loadedTextures.end())
            return it->second;
        else return load_texture(unityID, sRGB);
    };


    std::unordered_map<int32_t, std::shared_ptr<Material>> loadedMaterials;
    bool useTransmission = false;
    for (const auto& mat : materials)
    {
        std::shared_ptr<Material> matinfo = pSTF->CreateMaterial();
        matinfo->useSpecularGlossModel = false;
        matinfo->unityID = mat.unityID;
    #ifndef NDEBUG
        matinfo->name = std::to_string(mat.unityID);
    #endif
        switch (mat.type)
        {
        case PanguScenePBR:
        {
            matinfo->baseOrDiffuseTexture = find_texture(mat.textureIds[0], true);
            matinfo->normalTexture = find_texture(mat.textureIds[1]);
            matinfo->emissiveTexture = find_texture(mat.textureIds[2]);
            matinfo->emissiveColor = mat.textureIds[2] == kInvalidUnityID ? float3(0, 0, 0) : float3(mat.parameters[0], mat.parameters[1], mat.parameters[2]);
            matinfo->emissiveIntensity = mat.parameters[3];
            matinfo->domain = useTransmission ? MaterialDomain::Transmissive : MaterialDomain::Opaque;
            loadedMaterials[mat.unityID] = matinfo;
            break;
        }
        case TreeLeaf:
        {
            matinfo->baseOrDiffuseTexture = find_texture(mat.textureIds[0], true);
            matinfo->normalTexture = find_texture(mat.textureIds[1]);
            matinfo->domain = useTransmission ? MaterialDomain::TransmissiveAlphaTested : MaterialDomain::AlphaTested;
            matinfo->alphaCutoff = mat.parameters[0];
            loadedMaterials[mat.unityID] = matinfo;
            break;   
        }
        case Terrain:
        {
            matinfo->baseOrDiffuseTexture = find_texture(mat.textureIds[0], true);
            matinfo->normalTexture = find_texture(mat.textureIds[1]);
            loadedMaterials[mat.unityID] = matinfo;
            break;  
        }
        case MeshTilling:
        {
            matinfo->baseOrDiffuseTexture = find_texture(mat.textureIds[0], true);
            matinfo->normalTexture = find_texture(mat.textureIds[3]);
            loadedMaterials[mat.unityID] = matinfo;
        }
        default:
            log::error("Unsupported material type {}", mat.type);
            break;
        }
    }
    auto buffers = std::make_shared<BufferGroup>();
    buffers->indexData = std::move(indexData);
    buffers->positionData = std::move(positions);
    buffers->normalData = std::move(normals);
    buffers->tangentData = std::move(tangents);
    buffers->texcoord1Data = std::move(uv0);
    buffers->texcoord2Data = std::move(uv2);

    uint32_t totalIndices = 0;
    uint32_t totalVertices = 0;
    std::unordered_map<int32_t, std::shared_ptr<MeshInfo>> loadedMeshes;
    for (const auto& mesh : meshes)
    {
        dm::box3 bounds = dm::box3::empty();
        std::shared_ptr<MeshInfo> minfo = pSTF->CreateMesh();
        minfo->buffers = buffers; // TODO: maybe a bufferGroup per mesh?
        minfo->indexOffset = totalIndices;
        minfo->vertexOffset = totalVertices;
        minfo->totalVertices = mesh.vertexCount;
        minfo->totalIndices = mesh.indexCount;
    #ifndef NDEBUG
        minfo->name = std::to_string(mesh.unityID);
    #endif

        for (int i = totalVertices; i < mesh.vertexCount; i++)
        {
            dm::float3 pos(buffers->positionData[i].x, buffers->positionData[i].y, buffers->positionData[i].z);
            bounds |= pos;
        }
        minfo->objectSpaceBounds = bounds;
        totalIndices += mesh.indexCount;
        totalVertices += mesh.vertexCount;
        loadedMeshes[mesh.unityID] = minfo;
    }

    std::unordered_map<int32_t, std::shared_ptr<Light>> loadedLights;
    for (const auto& from : lights)
    {
        std::shared_ptr<Light> to;
        switch ((LightType)from.type)
        {
        case LightType::Directional:
        {
            auto directional = std::make_shared<DirectionalLight>();
            directional->irradiance = from.intensity;
            directional->color = from.color; // multiply by temperature
            to = directional;
        }
        case LightType::Spot:
        {
            auto spot = std::make_shared<SpotLight>();
            spot->intensity = from.intensity;
            spot->color = from.color;
            spot->range = from.attenuationRadius;
            spot->innerAngle = from.extra.x;
            spot->outerAngle = from.extra.y;
            break;
        }
        case LightType::Point:
        {
            auto point = std::make_shared<PointLight>();
            point->intensity = from.intensity;
            point->color = from.color;
            point->range = from.attenuationRadius;
            break;
        }
        case LightType::Rectangle:
        {
            auto rect = std::make_shared<RectangleLight>();
            rect->flux = from.intensity;
            rect->color = from.color;
            rect->range = from.attenuationRadius;
            rect->width = from.extra.x;
            rect->height = from.extra.y;
            break;
        }
        case LightType::Disc:
        {
            auto disc = std::make_shared<DiscLight>();
            disc->flux = from.intensity;
            disc->color = from.color;
            disc->range = from.attenuationRadius;
            disc->radius = from.extra.x;
            break;
        }
        default:
            break;
        }
        if (to)
        {
            to->unityID = from.unityID;
        }
    }

    std::shared_ptr<Material> emptyMaterial;
    for (const auto& obj : objects)
    {
        auto foundMesh = loadedMeshes.find(obj.meshUnityID);
        auto foundMaterial = loadedMaterials.find(obj.materialUnityID);
        if (foundMesh != loadedMeshes.end())
        {
            auto& meshInfo = foundMesh->second;
            if (!meshInfo->geometries.empty())
            {
                continue;
            }
            auto geo = pSTF->CreateMeshGeometry();
            if (foundMaterial != loadedMaterials.end())
            {
                geo->material = foundMaterial->second;
            }
            else
            {
                log::warning("Geometry %d for mesh '%s' doesn't have a material.", uint32_t(meshInfo->geometries.size()), meshInfo->name.c_str());
                if (!emptyMaterial)
                {
                    emptyMaterial = std::make_shared<Material>();
                    emptyMaterial->name = "(empty)";
                }
                geo->material = emptyMaterial;
            }
            geo->indexOffsetInMesh = 0;
            geo->vertexOffsetInMesh = 0;
            geo->numIndices = meshInfo->totalIndices;
            geo->numVertices = meshInfo->totalVertices;
            geo->objectSpaceBounds = meshInfo->objectSpaceBounds;
            meshInfo->geometries.push_back(geo);
        }
    }

    // build the scene graph
    std::shared_ptr<SceneGraph> graph = std::make_shared<SceneGraph>();
    std::shared_ptr<SceneGraphNode> root = std::make_shared<SceneGraphNode>();
#ifndef NDEBUG
    root->SetName(setting.immeFolderPath);
#endif
    for (const auto& obj : objects)
    {
        auto n = std::make_shared<SceneGraphNode>();
        auto foundMesh = loadedMeshes.find(obj.meshUnityID);
        dm::double3 translation(obj.position.x, obj.position.y, obj.position.z);
        dm::double3 scaling(obj.scale.x, obj.scale.y, obj.scale.z);
        dm::dquat rotation(obj.rotation.x, obj.rotation.y, obj.rotation.z, obj.rotation.w);
        n->SetTransform(&translation, &rotation, &scaling);
        n->SetUnityID(obj.unityID);
        graph->Attach(root, n);
        if (foundMesh != loadedMeshes.end())
        {
            auto leaf = pSTF->CreateMeshInstance(foundMesh->second);
            n->SetLeaf(leaf);
        }
        auto foundLight = loadedLights.find(obj.unityID);
        if (foundLight != loadedLights.end())
        {
            if (n->GetLeaf())
            {
                auto lightNode = std::make_shared<SceneGraphNode>();
                lightNode->SetLeaf(foundLight->second);
                graph->Attach(n, lightNode);
            }
            else
            {
                n->SetLeaf(foundLight->second);
            }
        }

#ifndef NDEBUG
        n->SetName(std::to_string(obj.unityID));
#endif
        stats.ObjectsTotal++;
        stats.ObjectsLoaded++;
    }
    root->ReverseChildren();
    result.rootNode = root;
    return true;
}


} // namespace Falcor
