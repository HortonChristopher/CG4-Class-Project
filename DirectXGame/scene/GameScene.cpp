#include "GameScene.h"
#include "Object3d.h"
#include "FbxLoader/FbxLoader.h"

#include <cassert>
#include <sstream>
#include <iomanip>

using namespace DirectX;

static float baseColor[3];
static float metalness;
static float specular;
static float roughness;

GameScene::GameScene()
{
}

GameScene::~GameScene()
{
	safe_delete(spriteBG);
	safe_delete(lightGroup);
	safe_delete(object1);
	safe_delete(model1);
}

void GameScene::Initialize(DirectXCommon* dxCommon, Input* input, Audio * audio)
{
	// nullptrチェック
	assert(dxCommon);
	assert(input);
	assert(audio);

	this->dxCommon = dxCommon;
	this->input = input;
	this->audio = audio;

	// カメラ生成
	camera = new DebugCamera(WinApp::window_width, WinApp::window_height, input);

	// ライト生成
	lightGroup = LightGroup::Create();

	// Device set
	Object3d::SetDevice(dxCommon->GetDevice());
	// Camera set
	Object3d::SetCamera(camera);
	// Graphics Pipeline set
	//Object3d::CreateGraphicsPipeline();
	// LightGroup Set
	Object3d::SetLightGroup(lightGroup);

	// デバッグテキスト用テクスチャ読み込み
	if (!Sprite::LoadTexture(debugTextTexNumber, L"Resources/debugfont.png")) {
		assert(0);
		return ;
	}
	// デバッグテキスト初期化
	debugText = DebugText::GetInstance();
	debugText->Initialize(debugTextTexNumber);

	// テクスチャ読み込み
	if (!Sprite::LoadTexture(1, L"Resources/background.png")) {
		assert(0);
		return;
	}
	// 背景スプライト生成
	spriteBG = Sprite::Create(1, { 0.0f,0.0f });
	// パーティクルマネージャ生成
	particleMan = ParticleManager::GetInstance();
	particleMan->SetCamera(camera);

	// Specify the FBX model and read the file
	//FbxLoader::GetInstance()->LoadModelFromFile("cube");
	model1 = FbxLoader::GetInstance()->LoadModelFromFile("spherePBR");

	object1 = new Object3d;
	object1->Initialize();
	object1->SetModel(model1);

	// Get initial value of the material parameter
	baseColor[0] = model1->GetBaseColor().x;
	baseColor[1] = model1->GetBaseColor().y;
	baseColor[2] = model1->GetBaseColor().z;
	metalness = model1->GetMetalness();
	specular = model1->GetSpecular();
	roughness = model1->GetRoughness();

	// テクスチャ2番に読み込み
	Sprite::LoadTexture(2, L"Resources/tex1.png");

	// カメラ注視点をセット
	camera->SetTarget({0, 0, 0});
	camera->SetDistance(3.0f);
}

void GameScene::Update()
{
	lightGroup->Update();
	camera->Update();
	particleMan->Update();

	// Reflect material parameters in the model
	model1->SetBaseColor(XMFLOAT3(baseColor));
	model1->SetMetalness(metalness);
	model1->SetSpecular(specular);
	model1->SetRoughness(roughness);
	model1->TransferMaterial();

	object1->Update();
}

void GameScene::Draw()
{
	// コマンドリストの取得
	ID3D12GraphicsCommandList* cmdList = dxCommon->GetCommandList();

#pragma region 背景スプライト描画
	// 背景スプライト描画前処理
	Sprite::PreDraw(cmdList);
	// 背景スプライト描画
	spriteBG->Draw();

	/// <summary>
	/// ここに背景スプライトの描画処理を追加できる
	/// </summary>

	// スプライト描画後処理
	Sprite::PostDraw();
	// 深度バッファクリア
	dxCommon->ClearDepthBuffer();
#pragma endregion

#pragma region 3D描画

	// 3D Object Drawing
	object1->Draw(cmdList);

	// パーティクルの描画
	//particleMan->Draw(cmdList);
#pragma endregion

#pragma region 前景スプライト描画
	// 前景スプライト描画前処理
	Sprite::PreDraw(cmdList);

	/// <summary>
	/// ここに前景スプライトの描画処理を追加できる
	/// </summary>

	// デバッグテキストの描画
	debugText->DrawAll(cmdList);

	// スプライト描画後処理
	Sprite::PostDraw();
#pragma endregion

#pragma region ImGui
	// ImGui
	ImGui::Begin("Material");
	ImGui::SetWindowPos(ImVec2(0, 0));
	ImGui::SetWindowSize(ImVec2(300, 130));
	ImGui::ColorEdit3("baseColor", baseColor, ImGuiColorEditFlags_Float);
	ImGui::SliderFloat("metalness", &metalness, 0, 1);
	ImGui::SliderFloat("specular", &specular, 0, 1);
	ImGui::SliderFloat("roughness", &roughness, 0, 1);
	ImGui::End();
#pragma endregion
}
