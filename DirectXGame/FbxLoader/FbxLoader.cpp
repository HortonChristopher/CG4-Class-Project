#include "FbxLoader.h"
#include <cassert>

using namespace DirectX;

// 静的メンバ変数の実体 The entity of a static member variable
const std::string FbxLoader::baseDirectory = "Resources/FbxModel/";

const std::string FbxLoader::defaultTextureFileName = "white1x1.png";

FbxLoader* FbxLoader::GetInstance()
{
    static FbxLoader instance;
    return &instance;
}

void FbxLoader::ConvertMatrixFromFbx( DirectX::XMMATRIX *dst, const FbxAMatrix &src )
{
    // 行 line
    for ( int i = 0; i < 4; i++ )
    {
        // 列 Column
        for ( int j = 0; j < 4; j++ )
        {
            // 1要素コピー 1 element copy
            dst->r[i].m128_f32[j] = (float)src.Get( i, j );
        }
    }
}

void FbxLoader::Initialize( ID3D12Device *device )
{
    // 再初期化チェック Reinitialization check
    assert( fbxManager == nullptr );

    // 引数からメンバ変数に代入 Assign from argument to member variable
    this->device = device;

    // FBXマネージャーの生成 Generate FBX manager
    fbxManager = FbxManager::Create();

    // FBXマネージャーの入出力設定 FBX Manager I / O settings
    FbxIOSettings *ios = FbxIOSettings::Create( fbxManager, IOSROOT );
    fbxManager->SetIOSettings( ios );

    // FBXインポータの生成 FBX importer generation
    fbxImporter = FbxImporter::Create( fbxManager, "" );
}

void FbxLoader:: Finalize()
{
    // 各種FBXインスタンスの破棄 Discard various FBX instances
    fbxImporter->Destroy();
    fbxManager->Destroy();
}

FbxModel* FbxLoader::LoadModelFromFile( const string &modelName )
{
    // モデルと同じ名前のフォルダから読み込む Read from a folder with the same name as the model
    const string directoryPath = baseDirectory + modelName + "/";

    // 拡張子.fbxを付加 Add extension .fbx
    const string fileName = modelName + ".fbx";

    // 連結してフルパスを得る Connect to get a full path
    const string fullpath = directoryPath + fileName;

    // ファイル名を指定してFBXファイルを読み込む Read the FBX file by specifying the file name
    if ( !fbxImporter->Initialize( fullpath.c_str(), -1, fbxManager->GetIOSettings() ) )
    {
        assert( 0 );
    }

    // シーン生成 Scene generation
    FbxScene *fbxScene = FbxScene::Create( fbxManager, "fbxScene" );

    // ファイルからロードしたFBXの情報をシーンにインポート Import FBX information loaded from a file into your scene
    fbxImporter->Import( fbxScene );

    // モデル生成 Model generation
    FbxModel *fbxmodel = new FbxModel();
    fbxmodel->name = modelName;

    // FBXノードの数を取得 Get the number of FBX nodes
    int nodeCount = fbxScene->GetNodeCount();

    // あらかじめ必要数分のメモリを確保することで、アドレスがずれるのを予防 By allocating the required number of memories in advance, it is possible to prevent the address from shifting.
    fbxmodel->nodes.reserve( nodeCount );

    // ルートノードから順に解析してモデルに流し込む Analyze in order from the root node and pour into the model
    ParseNodeRecursive( fbxmodel, fbxScene->GetRootNode() );

    // FBXシーン解放 FBX scene release
    // fbxScene->Destroy();
    fbxmodel->fbxScene = fbxScene;

    // バッファ生成 Buffer generation
    fbxmodel->CreatBuffers( device );

    return fbxmodel;
}

void FbxLoader::ParseNodeRecursive( FbxModel *fbxmodel, FbxNode *fbxNode, Node* parent )
{
    // モデルにノードを追加 Add node to model
    fbxmodel->nodes.emplace_back();
    Node &node = fbxmodel->nodes.back();

    // ノード名を取得 Get node name
    node.name = fbxNode->GetName();

    // FBXノードのローカル移動情報 FBX node local movement information
    FbxDouble3 rotation = fbxNode->LclRotation.Get();
    FbxDouble3 scaling = fbxNode->LclScaling.Get();
    FbxDouble3 translation = fbxNode->LclTranslation.Get();

    // 形式変換して代入 Format conversion and substitution
    node.rotation = { (float)rotation[0], (float)rotation[1],(float)rotation[2],0.0f };
    node.scaling = { (float)scaling[0], (float)scaling[1],(float)scaling[2],0.0f };
    node.translation = { (float)translation[0], (float)translation[1],(float)translation[2],1.0f };

    // スケール、回転、平行移動行列の計算 Calculation of scale, rotation, translation matrix
    XMMATRIX matScaling, matRotation, matTranslation;
    matScaling = XMMatrixScalingFromVector( node.scaling );
    matRotation = XMMatrixRotationRollPitchYawFromVector( node.rotation );
    matTranslation = XMMatrixTranslationFromVector( node.translation );

    // ローカル変形行列 Local transformation matrix
    node.transform = XMMatrixIdentity();
    node.transform *= matScaling;
    node.transform *= matRotation;
    node.transform *= matTranslation;

    // グローバル変形行列の計算 Calculation of global transformation matrix
    node.globalTransform = node.transform;
    if ( parent )
    {
        node.parent = parent;

        // 親の変形を乗算 Multiply the parent transformation
        node.globalTransform *= parent->globalTransform;
    }

    // FBXノードのメッシュ情報を解析 Analyze the mesh information of the FBX node
    FbxNodeAttribute *fbxNodeAttribute = fbxNode->GetNodeAttribute();

    if ( fbxNodeAttribute )
    {
        if ( fbxNodeAttribute->GetAttributeType() == FbxNodeAttribute::eMesh )
        {
            fbxmodel->meshNode = &node;
            ParseMesh( fbxmodel, fbxNode );
        }
    }

    // 子ノードに対して再帰呼び出し Recursive call to child node
    for ( int i = 0; i < fbxNode->GetChildCount(); i++ )
    {
        ParseNodeRecursive( fbxmodel, fbxNode->GetChild( i ), &node );
    }
}

void FbxLoader::ParseMesh( FbxModel *fbxmodel, FbxNode *fbxNode )
{
    // ノードのメッシュを取得 Get the mesh of the node
    FbxMesh *fbxMesh = fbxNode->GetMesh();

    // 頂点座標読み取り Read vertex coordinates
    ParseMeshVertices( fbxmodel, fbxMesh );

    // 面を構成するデータの読み取り Reading the data that makes up the surface
    ParseMeshFaces( fbxmodel, fbxMesh );

    // マテリアルの読み取り Material reading
    ParseMaterial( fbxmodel, fbxNode );

    // スキニング情報読み取り Reading skinning information
    ParseSkin( fbxmodel, fbxMesh );
}

void FbxLoader::ParseMeshVertices( FbxModel *fbxmodel, FbxMesh *fbxMesh )
{
    auto &vertices = fbxmodel->vertices;

    // 頂点座標データの数 Number of vertex coordinate data
    const int controlPointsCount = fbxMesh->GetControlPointsCount();

    // 必要数だけ頂点データ配列を確保 Secure as many vertex data arrays as you need
    FbxModel::VertexPosNormalUvSkin vert{};
    fbxmodel->vertices.resize( controlPointsCount, vert );

    // FBXメッシュの頂点座標配列を取得 Get the vertex coordinate array of the FBX mesh
    FbxVector4 *pCoord = fbxMesh->GetControlPoints();

    // FBXメッシュの全頂点座標をモデル内の配列にコピーする Copy all vertex coordinates of the FBX mesh to an array in the model
    for ( int i = 0; i < controlPointsCount; i++ )
    {
        FbxModel::VertexPosNormalUvSkin &vertex = vertices[i];

        // 座標のコピー Copy of coordinates
        vertex.pos.x = (float)pCoord[i][0];
        vertex.pos.y = (float)pCoord[i][1];
        vertex.pos.z = (float)pCoord[i][2];
    }
}

void FbxLoader::ParseMeshFaces( FbxModel *fbxmodel, FbxMesh *fbxMesh )
{
    auto &vertices = fbxmodel->vertices;
    auto &indices = fbxmodel->indices;

    // 1ファイルに複数メッシュのモデルは非対応 Models with multiple meshes are not supported for one file
    assert( indices.size() == 0 );

    // 面の数 Number of faces
    const int polygonCount = fbxMesh->GetPolygonCount();
    // UVデータの数 Number of UV data
    const int textureUVCount = fbxMesh->GetTextureUVCount();
    // UV名リスト UV name list
    FbxStringList uvNames;
    fbxMesh->GetUVSetNames( uvNames );
    // 面ごとの情報読み取り Information reading for each surface
    for ( int i = 0; i < polygonCount; i++ )
    {
        // 面を構成する頂点の数を取得(3なら三角形ポリゴン) Get the number of vertices that make up a face (3 is a triangular polygon)
        const int polygonSize = fbxMesh->GetPolygonSize( i );
        assert( polygonSize <= 4 );

        // 1頂点ずつ処理 Process one vertex at a time
        for ( int j = 0; j < polygonSize; j++ )
        {
            // FBX頂点配列のインデックス FBX vertex array index
            int index = fbxMesh->GetPolygonVertex( i, j );
            assert( index >= 0 );

            // 頂点法線読み込み Read vertex normals
            FbxModel::VertexPosNormalUvSkin &vertex = vertices[index];
            FbxVector4 normal;
            if ( fbxMesh->GetPolygonVertexNormal( i, j, normal ) )
            {
                vertex.normal.x = (float)normal[0];
                vertex.normal.y = (float)normal[1];
                vertex.normal.z = (float)normal[2];
            }

            // テクスチャUV読み込み Texture UV loading
            if ( textureUVCount > 0 )
            {
                FbxVector2 uvs;
                bool lUnmappedUV;

                // 0番決め打ちで読み込み Read with 0 number fixed
                if ( fbxMesh->GetPolygonVertexUV( i, j, uvNames[0], uvs, lUnmappedUV ) )
                {
                    vertex.uv.x = (float)uvs[0];
                    vertex.uv.y = (float)uvs[1];
                }
            }

            // インデックス配列に頂点インデックス追加 Add vertex index to index array
            // 3頂点までなら Up to 3 vertices
            if ( j < 3 )
            {
                // 1点追加し、他の2点と三角形を構築する
                indices.push_back( index );
            }
            // 4頂点目 4th vertex
            else
            {
                // 3点追加し、 Add 3 points,
                // 四角形の0,1,2,3の内 2,3,0で三角形を構築する Build a triangle with 2,3,0 out of 0,1,2,3 of a quadrangle
                int index2 = indices[indices.size() - 1];
                int index3 = index;
                int index0 = indices[indices.size() - 3];
                indices.push_back( index2 );
                indices.push_back( index3 );
                indices.push_back( index0 );
            }
        }
    }
}

void FbxLoader::ParseMaterial( FbxModel *fbxmodel, FbxNode *fbxNode )
{
    const int materialCount = fbxNode->GetMaterialCount();
    if ( materialCount > 0 )
    {
        // 先頭のマテリアルを取得 Get the first material
        FbxSurfaceMaterial *material = fbxNode->GetMaterial( 0 );
        // テクスチャを読み込んだかどうかを表すフラグ A flag that indicates whether the texture has been loaded
        bool textureLoaded = false;

        if ( material )
        {
            // FbxSurfaceLambertクラスかどうかを調べる Check if it is an FbxSurfaceLambert class
            if ( material->GetClassId().Is( FbxSurfaceLambert::ClassId ) )
            {
                FbxSurfaceLambert *lambert = static_cast<FbxSurfaceLambert *>(material);

                // 環境光係数 Ambient light coefficient
                FbxPropertyT<FbxDouble3> ambient = lambert->Ambient;
                fbxmodel->ambient.x = (float)ambient.Get()[0];
                fbxmodel->ambient.y = (float)ambient.Get()[1];
                fbxmodel->ambient.z = (float)ambient.Get()[2];

                // 拡散反射光係数 Diffuse reflected light coefficient
                FbxPropertyT<FbxDouble3> diffuse = lambert->Diffuse;
                fbxmodel->diffuse.x = (float)diffuse.Get()[0];
                fbxmodel->diffuse.y = (float)diffuse.Get()[1];
                fbxmodel->diffuse.z = (float)diffuse.Get()[2];
            }

            // ディフューズテクスチャを取り出す Extract the diffuse texture
            const FbxProperty diffuseProperty = material->FindProperty( FbxSurfaceMaterial::sDiffuse );
            if ( diffuseProperty.IsValid() )
            {
                const FbxFileTexture *texture = diffuseProperty.GetSrcObject<FbxFileTexture>();
                if ( texture )
                {
                    const char *filepath = texture->GetFileName();

                    // ファイルパスからファイル名抽出 File name extraction from file path
                    string path_str( filepath );
                    string name = ExtractFileName( path_str );

                    // テクスチャ読み込み Texture loading
                    LoadTexture( fbxmodel, baseDirectory + fbxmodel->name + "/" + name );
                    textureLoaded = true;
                }
            }

        }

        // テクスチャがない場合は白テクスチャを貼る If there is no texture, apply a white texture
        if ( !textureLoaded )
        {
            LoadTexture( fbxmodel, baseDirectory + defaultTextureFileName );
        }
    }
}

void FbxLoader::ParseSkin( FbxModel *fbxmodel, FbxMesh *fbxMesh )
{
    // スキニング情報 Skinning information
    FbxSkin *fbxSkin =
        static_cast<FbxSkin *>(fbxMesh->GetDeformer( 0,
            FbxDeformer::eSkin ));
    // スキニング情報がなければ終了 End if there is no skinning information
    if ( fbxSkin == nullptr )
    {
        for ( int i = 0; i < fbxmodel->vertices.size(); i++ )
        {
            fbxmodel->vertices[i].boneIndex[0] = 0;
            fbxmodel->vertices[i].boneWeight[0] = 1.0f;
        }

        return;
    }

    // ボーン配列の参照 Bone array reference
    std::vector<FbxModel::Bone> &bones = fbxmodel->bones;

    // ボーンの数 Number of bones
    int clusterCount = fbxSkin->GetClusterCount();
    bones.reserve( clusterCount );

    // 全てのボーンについて About all bones
    for ( int i = 0; i < clusterCount; i++ )
    {
        // FBXボーン情報 FBX bone information
        FbxCluster *fbxCluster = fbxSkin->GetCluster( i );

        // ボーン自体のノードの名前を取得 Get the name of the node in the bone itself
        const char *boneName = fbxCluster->GetLink()->GetName();

        // 新しくボーンを追加し、追加したボーンの参照を得る Add a new bone and get a reference to the added bone
        bones.emplace_back( FbxModel::Bone( boneName ) );
        FbxModel::Bone &bone = bones.back();
        // 自作ボーンと紐づける Associate with your own bone
        bone.fbxCluster = fbxCluster;

        // FBXから初期姿勢行列を所得する Earn an initial attitude matrix from FBX
        FbxAMatrix fbxMat;
        fbxCluster->GetTransformLinkMatrix( fbxMat );

        // XMMatrix型に変換する Convert to XMMatrix type
        XMMATRIX initialPose;
        ConvertMatrixFromFbx( &initialPose, fbxMat );

        // 初期姿勢行列の逆行列を得る Get the inverse of the initial attitude matrix
        bone.invInitialPose = XMMatrixInverse( nullptr, initialPose );
    }

    // ボーン番号とスキンウェイトのペア Bone number and skin weight pair
    struct WeightSet
    {
        UINT index;
        float weight;
    };

    // 二次元配列 Two-dimensional array
    std::vector<std::list<WeightSet>>
        weightLists( fbxmodel->vertices.size() );

    // 全てのボーンについて About all bones
    for ( int i = 0; i < clusterCount; i++ )
    {
        // FBXボーン情報 FBX bone information
        FbxCluster *fbxCluster = fbxSkin->GetCluster( i );
        // このボーンに影響を与える頂点の数 Number of vertices affecting this bone
        int controlPointIndicesCount = fbxCluster->GetControlPointIndicesCount();
        // このボーンに影響を与える頂点の配列 An array of vertices that affect this bone
        int *controlPointIndices = fbxCluster->GetControlPointIndices();
        double *controlPointWeights = fbxCluster->GetControlPointWeights();

        // 影響を受ける頂点について About affected vertices
        for ( int j = 0; j < controlPointIndicesCount; j++ )
        {
            // 頂点番号 Vertex number
            int vertIndex = controlPointIndices[j];
            // スキンウェイト Skin weight
            float weight = (float)controlPointWeights[j];
            // その頂点の影響を受けるボーンリストに、ボーンとウェイトのペアを追加 Add bone and weight pairs to the bone list affected by that vertex
            weightLists[vertIndex].emplace_back( WeightSet{ (UINT)i, weight } );
        }
    }

    // 頂点配列書き換え用の参照 Reference for rewriting vertex array
    auto &vertices = fbxmodel->vertices;
    // 各頂点について処理 Processing for each vertex
    for ( int i = 0; i < vertices.size(); i++ )
    {
        // 頂点のウェイトから最も大きい4つを選択 Select the 4 largest from the weights of the vertices
        auto &weightList = weightLists[i];
        // 大小比較用のラムダ式を指定して降順にソート Sort in descending order by specifying a lambda expression for size comparison
        weightList.sort(
            []( auto const &lhs, auto const &rhs )
            {
                return lhs.weight > rhs.weight;
            }
        );

        int weightArrayIndex = 0;
        // 降順ソート済みのウェイトリストから From the descending sorted weight list
        for ( auto &WeightSet : weightList )
        {
            // 頂点データに書き込み Write to vertex data
            vertices[i].boneIndex[weightArrayIndex] = WeightSet.index;
            vertices[i].boneWeight[weightArrayIndex] = WeightSet.weight;
            // 4つに達したら終了 Finish when you reach 4
            if ( ++weightArrayIndex >= FbxModel::MAX_BONE_INDICES )
            {
                float weight = 0.0f;
                // 2番目以降のウェイトを合計 Total the second and subsequent weights
                for ( int j = 1; j < FbxModel::MAX_BONE_INDICES; j++ )
                {
                    weight += vertices[i].boneWeight[j];
                }
                // 合計で1.0f(100%)になるように調整 Adjusted to be 1.0f (100%) in total
                vertices[i].boneWeight[0] = 1.0f - weight;
                break;
            }
        }
    }
}

void FbxLoader::LoadTexture( FbxModel *fbxmodel, const std::string &fullpath )
{
    HRESULT result = S_FALSE;

    // WICテクスチャのロード Loading WIC textures
    TexMetadata &metadata = fbxmodel->metadata;
    ScratchImage &scratchImg = fbxmodel->scratchImg;

    // ユニコード文字列に変換 Convert to unicode string
    wchar_t wfilepath[128];
    MultiByteToWideChar( CP_ACP, 0, fullpath.c_str(), -1, wfilepath, _countof( wfilepath ) );
    result = LoadFromWICFile(
        wfilepath, WIC_FLAGS_NONE,
        &metadata, scratchImg );
    if ( FAILED( result ) )
    {
        assert( 0 );
    }
}

std::string FbxLoader::ExtractFileName( const std::string &path )
{
    size_t pos1;

    // 区切り文字 '\\' が出てくる一番最後の部分を検索 Search for the last part where the delimiter'\\' appears
    pos1 = path.rfind( '\\' );
    if ( pos1 != string::npos )
    {
        return path.substr( pos1 + 1, path.size() - pos1 - 1 );
    }

    // 区切り文字 '/' が出てくる一番最後の部分を検索 Search for the last part where the delimiter'/' appears
    pos1 - path.rfind( '/' );
    if ( pos1 != string::npos )
    {
        return path.substr( pos1 + 1, path.size() - pos1 - 1 );
    }

    return path;
}
