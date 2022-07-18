#include "FbxLoader.h"

#include <cassert>

using namespace DirectX;

/// <summary>
/// Static Member Variable Entity
/// </summary>
const std::string FbxLoader::baseDirectory = "Resources/";

const std::string FbxLoader::defaultTextureFileName = "white1x1.png";

FbxLoader* FbxLoader::GetInstance()
{
    static FbxLoader instance;
    return &instance;
}

void FbxLoader::Initialize(ID3D12Device* device)
{
    // Reinitialization check
    assert(fbxManager == nullptr);

    // Assign member variables from pulling
    this->device = device;

    // Fbx manager generation
    fbxManager = FbxManager::Create();

    // Fbx manager input/output settings
    FbxIOSettings* ios = FbxIOSettings::Create(fbxManager, IOSROOT);
    fbxManager->SetIOSettings(ios);

    // Fbx importer generation
    fbxImporter = FbxImporter::Create(fbxManager, "");
}

void FbxLoader::Finalize()
{
    // Destroy various FBX instances
    fbxImporter->Destroy();
    fbxManager->Destroy();
}

Model* FbxLoader::LoadModelFromFile(const string& modelName)
{
    // Continue from the folder with same name as the model
    const string directoryPath = baseDirectory + modelName + "/";

    // Extention, add FBX
    const string fileName = modelName + ".fbx";

    // Connect to get full bus
    const string fullpath = directoryPath + fileName;

    // Specify each file and read the FBX file
    if (!fbxImporter->Initialize(fullpath.c_str(), -1, fbxManager->GetIOSettings()))
    {
        assert(0);
    }

    // Scene generation
    FbxScene* fbxScene = FbxScene::Create(fbxManager, "fbxScene");

    // Import FBX informaiton loaded from file into scene
    fbxImporter->Import(fbxScene);

    // Model Generation
    Model* model = new Model();
    model->name = modelName;

    // Get number of nodes
    int nodeCount = fbxScene->GetNodeCount();

    // By allocation the required number of memories in advance, it is possible to prevent he address from shifting
    model->nodes.reserve(nodeCount);

    // Analyze root node request and pour into model
    ParseNodeRecursive(model, fbxScene->GetRootNode());

    // Create Buffer
    model->CreateBuffers(device);

    // FBX scene release
    fbxScene->Destroy();

    return model;
}

void FbxLoader::ParseNodeRecursive(Model* model, FbxNode* fbxNode, Node* parent)
{
    // Add node ot model
    model->nodes.emplace_back();
    Node& node = model->nodes.back();

    // Get node name
    node.name = fbxNode->GetName();

    // FBX node local move information
    FbxDouble3 rotation = fbxNode->LclRotation.Get();
    FbxDouble3 scaling = fbxNode->LclScaling.Get();
    FbxDouble3 translation = fbxNode->LclTranslation.Get();

    // Format conversion and assignment
    node.rotation = { (float)rotation[0], (float)rotation[1], (float)rotation[2], 0.0f };
    node.scaling = { (float)scaling[0], (float)scaling[1], (float)scaling[2], 0.0f };
    node.translation = { (float)translation[0], (float)translation[1], (float)translation[2], 1.0f };

    // Convert rotation from degrees to radians
    node.rotation.m128_f32[0] = XMConvertToRadians(node.rotation.m128_f32[0]);
    node.rotation.m128_f32[1] = XMConvertToRadians(node.rotation.m128_f32[1]);
    node.rotation.m128_f32[2] = XMConvertToRadians(node.rotation.m128_f32[2]);

    // Scale, rotation, translation matrix calculation
    XMMATRIX matScaling, matRotation, matTranslation;
    matScaling = XMMatrixScalingFromVector(node.scaling);
    matRotation = XMMatrixRotationRollPitchYawFromVector(node.rotation);
    matTranslation = XMMatrixTranslationFromVector(node.translation);

    // Calculation of local transformation matrix
    node.transform = XMMatrixIdentity();
    node.transform *= matScaling;
    node.transform *= matRotation;
    node.transform *= matTranslation;

    // Calculation of global transformation matrix
    node.globalTransform = node.transform;
    if (parent)
    {
        node.parent = parent;

        // Multiply the parent transformation
        node.globalTransform *= parent->globalTransform;
    }

    // Analyze the mesh information of FBX node
    FbxNodeAttribute* fbxNodeAttribute = fbxNode->GetNodeAttribute();

    if (fbxNodeAttribute)
    {
        if (fbxNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh)
        {
            model->meshNode = &node;
            ParseMesh(model, fbxNode);
        }
    }

    // Recursive call to child nodes
    for (int i = 0; i < fbxNode->GetChildCount(); i++)
    {
        ParseNodeRecursive(model, fbxNode->GetChild(i), &node);
    }
}

void FbxLoader::ParseMesh(Model* model, FbxNode* fbxNode)
{
    // Get mesh of node
    FbxMesh* fbxMesh = fbxNode->GetMesh();

    // Vertex coordinate reading
    ParseMeshVertices(model, fbxMesh);

    // Surface data reading
    ParseMeshFaces(model, fbxMesh);

    // Material reading
    ParseMaterial(model, fbxNode);
}

void FbxLoader::ParseMeshVertices(Model* model, FbxMesh* fbxMesh)
{
    auto& vertices = model->vertices;

    // Number of vertex coordinates
    const int controlPointsCount = fbxMesh->GetControlPointsCount();

    // Secure as many vertex data arrays as needed
    Model::VertexPosNormalUv vert{};
    model->vertices.resize(controlPointsCount, vert);

    // Get the vertex coordinate array of FBX mesh
    FbxVector4* pCoord = fbxMesh->GetControlPoints();

    // Copy all vertex coordinates of FBX mesh to the array in the model
    for (int i = 0; i < controlPointsCount; i++)
    {
        Model::VertexPosNormalUv& vertex = vertices[i];
        
        // Copy of coordinates
        vertex.pos.x = (float)pCoord[i][0];
        vertex.pos.y = (float)pCoord[i][1];
        vertex.pos.z = (float)pCoord[i][2];
    }
}

void FbxLoader::ParseMeshFaces(Model* model, FbxMesh* fbxMesh)
{
    auto& vertices = model->vertices;
    auto& indices = model->indices;

    // 1 files does not support multiple mesh models
    assert(indices.size() == 0);

    // Number of faces
    const int polygonCount = fbxMesh->GetPolygonCount();

    // Number of UV data
    const int textureUVCount = fbxMesh->GetTextureUVCount();

    // UV name list
    FbxStringList uvNames;
    fbxMesh->GetUVSetNames(uvNames);

    // Reading information for each surface
    for (int i = 0; i < polygonCount; i++)
    {
        // Get number of vertices that match the face (3 is a triangle polygon)
        const int polygonSize = fbxMesh->GetPolygonSize(i);
        assert(polygonSize <= 4);

        // Process one vertex at a time
        for (int j = 0; j < polygonSize; j++)
        {
            // FBX vertex array index
            int index = fbxMesh->GetPolygonVertex(i, j);
            assert(index >= 0);

            // Read vertex normals
            Model::VertexPosNormalUv& vertex = vertices[index];
            FbxVector4 normal;
            if (fbxMesh->GetPolygonVertexNormal(i, j, normal))
            {
                vertex.normal.x = (float)normal[0];
                vertex.normal.y = (float)normal[1];
                vertex.normal.z = (float)normal[2];
            }

            // Texture UV reading
            if (textureUVCount > 0)
            {
                FbxVector2 uvs;
                bool lUnmappedUV;

                // Read by number 0 (?)
                if (fbxMesh->GetPolygonVertexUV(i, j, uvNames[0], uvs, lUnmappedUV))
                {
                    vertex.uv.x = (float)uvs[0];
                    vertex.uv.y = (float)uvs[1];
                }
            }

            // Add vertex index to index distribution column
            // Up to 3rd vertex
            if (j < 3)
            {
                // Add one point and build a triangle with the other two points
                indices.push_back(index);
            }
            // 4th point
            else
            {
                // Add 3 points
                // Build a riangle with 2, 3, 0 of 0, 1, 2, 3 of quadrilateral
                int index2 = indices[indices.size() - 1];
                int index3 = index;
                int index0 = indices[indices.size() - 3];
                indices.push_back(index2);
                indices.push_back(index3);
                indices.push_back(index0);
            }
        }
    }
}

void FbxLoader::ParseMaterial(Model* model, FbxNode* fbxNode)
{
    const int materialCount = fbxNode->GetMaterialCount();
    if (materialCount > 0)
    {
        // Get front line material
        FbxSurfaceMaterial* material = fbxNode->GetMaterial(0);

        // Flag indicating whether the texture has been loaded
        bool textureLoaded = false;

        if (material)
        {
            // Material name (Debug)
            string name = material->GetName();

            // Base color
            const FbxProperty propBaseColor = FbxSurfaceMaterialUtils::GetProperty("baseColor", material);
            if (propBaseColor.IsValid())
            {
                // Read value as FbxDouble3
                FbxDouble3 baseColor = propBaseColor.Get<FbxDouble3>();

                // Write the read value to model
                model->baseColor.x = (float)baseColor.Buffer()[0];
                model->baseColor.y = (float)baseColor.Buffer()[1];
                model->baseColor.z = (float)baseColor.Buffer()[2];

                const FbxFileTexture* texture = propBaseColor.GetSrcObject<FbxFileTexture>();
                if (texture)
                {
                    const char* filepath = texture->GetFileName();

                    string path_str(filepath);
                    string name = ExtractFileName(path_str);

                    LoadTexture(&model->baseTexture, baseDirectory + model->name + "/" + name);
                    model->baseColor = { 0,0,0 };
                    textureLoaded = true;
                }
            }

            // Metallic Degree
            const FbxProperty propMetalness = FbxSurfaceMaterialUtils::GetProperty("metalness", material);
            if (propMetalness.IsValid())
            {
                // Write the read value to the model
                model->metalness = propMetalness.Get<float>();

                const FbxFileTexture* texture = propMetalness.GetSrcObject<FbxFileTexture>();
                if (texture)
                {
                    const char* filepath = texture->GetFileName();

                    string path_str(filepath);
                    string name = ExtractFileName(path_str);

                    LoadTexture(&model->metalnessTexture, baseDirectory + model->name + "/" + name);
                    model->metalness = 0.0f;
                }
            }

            // Specular (Gamma?)
            const FbxProperty propSpecular = FbxSurfaceMaterialUtils::GetProperty("specular", material);
            if (propSpecular.IsValid())
            {
                // write the read value to the model
                model->specular = propSpecular.Get<float>();
            }

            // Roughness
            const FbxProperty propSpecularRoughness = FbxSurfaceMaterialUtils::GetProperty("specularRoughness", material);
            if (propSpecularRoughness.IsValid())
            {
                // write the read value to the model
                model->roughness = propSpecularRoughness.Get<float>();

                const FbxFileTexture* texture = propSpecularRoughness.GetSrcObject<FbxFileTexture>();
                if (texture)
                {
                    const char* filepath = texture->GetFileName();

                    string path_str(filepath);
                    string name = ExtractFileName(path_str);

                    LoadTexture(&model->roughnessTexture, baseDirectory + model->name + "/" + name);
                    model->roughness = 0.0f;
                }
            }

            // Subsurface
            const FbxProperty propSubsurface = FbxSurfaceMaterialUtils::GetProperty("subsurface", material);
            if (propSubsurface.IsValid())
            {
                // write the read value to the model
                float subsurface = propSubsurface.Get<float>();
            }

            // Specular Color
            const FbxProperty propSpecularColor = FbxSurfaceMaterialUtils::GetProperty("specularColor", material);
            if (propSpecularColor.IsValid())
            {
                // write the read value to the model
                FbxDouble3 specularColor = propSpecularColor.Get<FbxDouble3>();
            }

            // Specular Anisotropy
            const FbxProperty propSpecularAnisotropy = FbxSurfaceMaterialUtils::GetProperty("spcularAnisotropy", material);
            if (propSpecularAnisotropy.IsValid())
            {
                // write the read value to the model
                float specularanisotropy = propSubsurface.Get<float>();
            }

            // Sheen Color
            const FbxProperty propSheenColor = FbxSurfaceMaterialUtils::GetProperty("sheenColor", material);
            if (propSheenColor.IsValid())
            {
                // write the read value to the model
                FbxDouble3 sheenColor = propSheenColor.Get<FbxDouble3>();
            }

            // Sheen
            const FbxProperty propSheen = FbxSurfaceMaterialUtils::GetProperty("sheen", material);
            if (propSheen.IsValid())
            {
                // write the read value to the model
                float sheen = propSheen.Get<float>();
            }

            // Coat Roughness
            const FbxProperty propCoatRoughness = FbxSurfaceMaterialUtils::GetProperty("coatRoughness", material);
            if (propCoatRoughness.IsValid())
            {
                // write the read value to the model
                float coatRoughness = propCoatRoughness.Get<float>();
            }

            // Coat
            const FbxProperty propCoat = FbxSurfaceMaterialUtils::GetProperty("coat", material);
            if (propCoat.IsValid())
            {
                // write the read value to the model
                float coat = propCoat.Get<float>();
            }

            // Normal map
            const FbxProperty propNormalCamera = FbxSurfaceMaterialUtils::GetProperty("normalCamera", material);
            if (propNormalCamera.IsValid())
            {
                const FbxFileTexture* texture = propNormalCamera.GetSrcObject<FbxFileTexture>();
                if (texture)
                {
                    const char* filepath = texture->GetFileName();

                    string path_str(filepath);
                    string name = ExtractFileName(path_str);

                    LoadTexture(&model->normalTexture, baseDirectory + model->name + "/" + name);
                }
            }
        }

        // If there is no texture, paste a white texture
        if (!textureLoaded)
        {
            LoadTexture(&model->baseTexture, baseDirectory + defaultTextureFileName);
        }
    }
}

void FbxLoader::LoadTexture(TextureData* texdata, const std::string& fullpath)
{
    HRESULT result = S_FALSE;

    // load WIC texture
    TexMetadata& metadata = texdata->metadata;
    ScratchImage& scratchImg = texdata->scratchImg;

    // Convert to unicode string
    wchar_t wfilepath[128];
    MultiByteToWideChar(CP_ACP, 0, fullpath.c_str(), -1, wfilepath, _countof(wfilepath));
    result = LoadFromWICFile(
        wfilepath, WIC_FLAGS_NONE,
        &metadata, scratchImg);
    if (FAILED(result))
    {
        assert(0);
    }
}

std::string FbxLoader::ExtractFileName(const std::string& path)
{
    size_t pos1;

    // Search for the last part where limiter "\\" appears
    pos1 = path.rfind('\\');
    if (pos1 != string::npos)
    {
        return path.substr(pos1 + 1, path.size() - pos1 - 1);
    }

    // Search for the last part where the limiter "/" appears
    pos1 = path.rfind('/');
    if (pos1 != string::npos)
    {
        return path.substr(pos1 + 1, path.size() - pos1 - 1);
    }

    return path;
}