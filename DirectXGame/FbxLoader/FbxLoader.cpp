#include "FbxLoader.h"

#include <cassert>

/// <summary>
/// Static Member Variable Entity
/// </summary>
const std::string FbxLoader::baseDirectory = "Resources/";

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

void FbxLoader::LoadModelFromFile(const string& modelName)
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
}