#include "TitleScene.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "GamePadInput.h"
#include "GameTime.h"
#include "SoundManager.h"
#include <numbers>

using namespace KamataEngine;

TitleScene::TitleScene() {}

TitleScene::~TitleScene() 
{

	// 3Dモデルデータの解放
	// 自キャラの3Dモデルデータの解放
	delete modelPlayer_;

	delete titleLogoSprite_;

	// カメラコントローラの解放
	delete cameraController_;

	// フェードの解放
	delete fade_;
}

void TitleScene::Initialize() 
{
	GameTime::SetTimeScale(1.0f);
	// 3Dモデルデータの生成
	// プレイヤーのモデルデータ生成
	modelPlayer_ = Model::CreateFromOBJ("player", true);

	const uint32_t titleTexture = TextureManager::Load("UI/Title.png");
	titleLogoSprite_ = Sprite::Create(titleTexture, Vector2(240.0f, 35.0f));
	titleLogoSprite_->SetSize(Vector2(800.0f, 267.0f));

	// 自キャラの生成
	player_ = new Player();

	// カメラコントローラの生成
	cameraController_ = new CameraController();
	
	// 追従対象（ターゲット）をセット
	cameraController_->SetTarget(player_);

	// カメラコントローラの初期化
	cameraController_->Initialize();

	// 座標を指定
	Vector3 playerPosition = {0.0f, -2.0f, 0.0f};

	// 自キャラの初期化
	player_->Initialize(modelPlayer_, nullptr, nullptr, cameraController_->GetCamera(), playerPosition);
	
	// セッターを使って正面を向かせる
	player_->SetRotationY(std::numbers::pi_v<float>);

	// カメラの座標をターゲット（プレイヤー）に瞬間合わせする
	cameraController_->Reset();
	
	// カメラの位置を少し下に強制的にずらす
	cameraController_->GetCamera()->translation_.y -= 0.5f;

	// カメラをプレイヤーに近づけて、プレイヤーを画面の手前に大きく映す
	cameraController_->GetCamera()->translation_.z += 9.0f;

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);


}

// 更新処理
void TitleScene::Update()
{	
	// フェードの更新
	fade_->Update();

	// シーンのフェーズごとの処理
	switch (phase_)
	{
	case Phase::kFadeIn:
		// フェードインが終わったらメインフェーズに切り替える
		if (fade_->IsFinished())
		{
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
		// メイン処理(スペースキーが押されたらフェードアウトを開始
		if (Input::GetInstance()->PushKey(DIK_SPACE) || GamePadInput::IsConfirmTriggered())
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kDecision);
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}

		break;
	case Phase::kFadeOut:
		// フェードアウトが終わったらシーン終了フラグを立てる
		if (fade_->IsFinished()) 
		{
			finished_ = true;
		}
		break;
	}

	// カメラの行列転送
	cameraController_->GetCamera()->UpdateMatrix();

	// 毎フレームタイマーを加算して時間を進める
	titleFloatingTimer_ += 0.05f * GameTime::GetDeltaFrames();
	// タイトルロゴを画面上部で少しだけ上下させる。
	titleLogoSprite_->SetPosition(Vector2(240.0f, 35.0f + sinf(titleFloatingTimer_) * 8.0f));

	
}

// 描画処理
void TitleScene::Draw() 
{
	// 3Dモデル描画前処理
	Model::PreDraw();

	// プレイヤーの描画
	player_->Draw();

	// 3Dモデル描画後処理
	Model::PostDraw();
	Sprite::PreDraw();
	titleLogoSprite_->Draw();
	Sprite::PostDraw();

	// フェードの描画
	fade_->Draw();
}
