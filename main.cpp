#include "GameScene.h"
#include "TitleScene.h"
#include "ClearScene.h"
#include "StageSelectScene.h"
#include "KamataEngine.h"
#include "StageManager.h"
#include "SoundManager.h"
#include <Windows.h>
#include <array>
#include <cstdint>
#include <fstream>
#include <limits>
#include <sstream>

namespace
{
enum class BgmType : uint32_t
{
	kTitle,
	kStage1,
	kStage2,
	kStage3,
	kClear,
	kCount,
};

// Windows.h の max マクロと衝突しないよう、関数名を括弧で囲む。
constexpr uint32_t kInvalidVoiceHandle = (std::numeric_limits<uint32_t>::max)();
constexpr float kBgmVolume = 0.35f;

std::array<uint32_t, static_cast<size_t>(BgmType::kCount)> bgmHandles_{};
uint32_t currentBgmVoiceHandle_ = kInvalidVoiceHandle;
BgmType currentBgmType_ = BgmType::kCount;

size_t ToBgmIndex(BgmType bgmType)
{
	return static_cast<size_t>(bgmType);
}

void InitializeBgm()
{
	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();
	// Audio は Resources フォルダを基準に読み込むため、BGM 以下の相対パスを指定する。
	bgmHandles_[ToBgmIndex(BgmType::kTitle)] = audio->LoadWave("BGM/title.wav");
	bgmHandles_[ToBgmIndex(BgmType::kStage1)] = audio->LoadWave("BGM/stage1.wav");
	bgmHandles_[ToBgmIndex(BgmType::kStage2)] = audio->LoadWave("BGM/stage2.wav");
	bgmHandles_[ToBgmIndex(BgmType::kStage3)] = audio->LoadWave("BGM/stage3.wav");
	bgmHandles_[ToBgmIndex(BgmType::kClear)] = audio->LoadWave("BGM/clear.wav");
}

void StopBgm()
{
	if (currentBgmVoiceHandle_ != kInvalidVoiceHandle)
	{
		KamataEngine::Audio::GetInstance()->StopWave(currentBgmVoiceHandle_);
		currentBgmVoiceHandle_ = kInvalidVoiceHandle;
	}
	currentBgmType_ = BgmType::kCount;
}

void PlayBgm(BgmType bgmType)
{
	// タイトルとステージセレクトの間など、同じ曲を継続する場合は再生し直さない。
	if (currentBgmType_ == bgmType && currentBgmVoiceHandle_ != kInvalidVoiceHandle)
	{
		return;
	}

	StopBgm();
	currentBgmVoiceHandle_ = KamataEngine::Audio::GetInstance()->PlayWave(
		bgmHandles_[ToBgmIndex(bgmType)], true, kBgmVolume);
	currentBgmType_ = bgmType;
}

void PlayCurrentStageBgm(int32_t stageIndex)
{
	switch (stageIndex)
	{
	case 0:
		PlayBgm(BgmType::kStage1);
		break;
	case 1:
		PlayBgm(BgmType::kStage2);
		break;
	case 2:
		PlayBgm(BgmType::kStage3);
		break;
	default:
		// ステージ数が増えた場合でも無音にならないよう、タイトル曲を使う。
		PlayBgm(BgmType::kTitle);
		break;
	}
}
} // namespace

ClearScene* clearScene = nullptr;
GameScene* gameScene = nullptr;
TitleScene* titleScene = nullptr;
StageSelectScene* stageSelectScene = nullptr;
StageManager* stageManager = nullptr;

// シーン
enum class Scene
{

	kUnknown = 0,

	kTitle,
	kStageSelect,
	kGame,
	kClear,
};

// 現在シーン
Scene scene = Scene::kUnknown;

// シーン切り替え
void ChangeScene()
{
	switch (scene) 
	{ 
	case Scene::kTitle:
		if (titleScene->IsFinished())
		{
			// 旧シーンの開放
			delete titleScene;
			titleScene = nullptr;
			// 新シーンの生成と初期化
			scene = Scene::kStageSelect;
			stageSelectScene = new StageSelectScene;
			stageSelectScene->Initialize(stageManager);
			PlayBgm(BgmType::kTitle);
		}
		break;
	case Scene::kStageSelect:
		if (stageSelectScene->IsFinished())
		{
			delete stageSelectScene;
			stageSelectScene = nullptr;

			scene = Scene::kGame;
			gameScene = new GameScene;
			gameScene->Initialize(stageManager);
			PlayCurrentStageBgm(stageManager->GetCurrentStageIndex());
		}
		break;
	case Scene::kGame:
		if (gameScene->IsFinished())
		{
			bool isCleared = gameScene->IsCleared();
			bool isStageSelectRequested = gameScene->IsStageSelectRequested();
			const bool areAllStagesCleared = stageManager->AreAllStagesCleared();

			// 旧シーンの開放
			delete gameScene;
			gameScene = nullptr;

			if (isStageSelectRequested)
			{
				scene = Scene::kStageSelect;
				stageSelectScene = new StageSelectScene;
				stageSelectScene->Initialize(stageManager);
				PlayBgm(BgmType::kTitle);
			}
			else if (isCleared && areAllStagesCleared)
			{
				// 全ステージをクリアした時だけ、最終クリア画面へ移る。
				scene = Scene::kClear;
				clearScene = new ClearScene();
				clearScene->Initialize();
				PlayBgm(BgmType::kClear);
			}
			else if (isCleared)
			{
				// 通常のステージクリア後は、ステージセレクトへ戻る。
				scene = Scene::kStageSelect;
				stageSelectScene = new StageSelectScene;
				stageSelectScene->Initialize(stageManager);
				PlayBgm(BgmType::kTitle);
			}
			else 
			{
				// シーン変更
				scene = Scene::kTitle;
				// 新シーンの生成と初期化
				titleScene = new TitleScene();
				titleScene->Initialize();
				PlayBgm(BgmType::kTitle);
			}
		}
		else if (gameScene->IsReloadRequested()) 
		{
			// シーンリロード
			delete gameScene;
			gameScene = nullptr;
			gameScene = new GameScene;
			gameScene->Initialize(stageManager);
			PlayCurrentStageBgm(stageManager->GetCurrentStageIndex());
		}
		break;

	case Scene::kClear:
		if (clearScene->IsFinished())
		{
			delete clearScene;
			clearScene = nullptr;

			scene = Scene::kTitle;
			titleScene = new TitleScene();
			titleScene->Initialize();
			PlayBgm(BgmType::kTitle);
		}
		break;
	}
}

void UpdateScene()
{
	switch (scene) 
	{
	case Scene::kUnknown:
		break;
	case Scene::kTitle:
		titleScene->Update();
		break;
	case Scene::kStageSelect:
		stageSelectScene->Update();
		break;
	case Scene::kGame:
		gameScene->Update();
		break;
	case Scene::kClear:
		clearScene->Update();
		break;
	}
}

void DrawScene() 
{
	switch (scene)
	{
	case Scene::kUnknown:
		break;
	case Scene::kTitle:
		titleScene->Draw();
		break;
	case Scene::kStageSelect:
		stageSelectScene->Draw();
		break;
	case Scene::kGame:
		gameScene->Draw();
		break;
	case Scene::kClear:
		clearScene->Draw();
		break;
	}
}

void LoadDebugSettings() 
{
	// 設定ファイルを開く（ファイルパスはプロジェクトの構成に合わせて調整してください）
	std::ifstream file("DebugSettings.ini");

	// ファイルが存在しない場合は何もせず終了
	if (!file.is_open()) {
		return;
	}

	std::string line;
	// 1行ずつ読み出す
	while (std::getline(file, line)) {
		// 空行やコメント行（# や ; で始まる行）をスキップ
		if (line.empty() || line[0] == '#' || line[0] == ';') {
			continue;
		}

		std::istringstream lineStream(line);
		std::string key;
		std::string value;

		// スペースやタブで区切ってキーとバリューを取得
		lineStream >> key >> value;

		// ステージ設定の判定
		if (key == "InitialStage") {
			stageManager->SetCurrentStageIndexByName(value);
		}
	}
}

// Windowsアプリでのエントリーポイント(main関数)
int WINAPI WinMain(_In_ HINSTANCE, _In_opt_ HINSTANCE, _In_ LPSTR, _In_ int) {

	using namespace KamataEngine;

	// エンジンの初期化
	KamataEngine::Initialize(L"カエルカエる");

	// DirectXCommonインスタンスの取得
	DirectXCommon* dxCommon = DirectXCommon::GetInstance();

	// ImGuiManagerインスタンスの取得
	ImGuiManager* imguiManager = ImGuiManager::GetInstance();

	// クラスインスタンスの生成
	stageManager = new StageManager;
	// ステージデータファイルの読み込み
	stageManager->LoadStageDataFile();
	// 使用する全BGMを一度読み込む。
	InitializeBgm();
	// 使用する全SEを一度読み込む。
	SoundManager::GetInstance()->Initialize();

	// 最初のシーンの初期化
#ifdef _DEBUG
	// デバッグ設定を読み込んで初期ステージを上書き設定する
	LoadDebugSettings();

	// デバッグビルドでもステージ選択から開始する
	scene = Scene::kStageSelect;
	stageSelectScene = new StageSelectScene;
	stageSelectScene->Initialize(stageManager);
	PlayBgm(BgmType::kTitle);
#else
	// リリースビルド時などでは通常通りタイトルシーンから開始
	scene = Scene::kTitle;
	titleScene = new TitleScene;
	titleScene->Initialize();
	PlayBgm(BgmType::kTitle);
#endif

	// メインループ
	while (true) {
		// エンジンの更新
		if (KamataEngine::Update()) {
			break;
		}

		// ImGui受付開始
		imguiManager->Begin();

		// シーン切り替え
		ChangeScene();
		// 現在シーン更新
		UpdateScene();

		// ImGui受付終了
		imguiManager->End();

		// 描画開始
		dxCommon->PreDraw();

		// ここに描画処理を記述する

		// 現在シーンの描画
		DrawScene();

		// 軸表示の描画
		AxisIndicator::GetInstance()->Draw();

		// ImGui描画
		imguiManager->Draw();

		// 描画終了
		dxCommon->PostDraw();
	}

	// ゲームシーンの解放
	delete titleScene;
	delete stageSelectScene;
	delete gameScene;
	delete clearScene;

	// クラスインスタンスの解放
	delete stageManager;

	// nullptrの代入
	titleScene = nullptr;
	stageSelectScene = nullptr;
	gameScene = nullptr;
	clearScene = nullptr;

	// エンジンを終了する前に、再生中のBGMを止める。
	StopBgm();


	// エンジンの終了処理
	KamataEngine::Finalize();
	return 0;
}
