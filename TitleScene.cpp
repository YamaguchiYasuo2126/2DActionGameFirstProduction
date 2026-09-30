#include "TitleScene.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "MyMathUtility.h"
#include "GamePadInput.h"
#include "SoundManager.h"
#include <numbers>

using namespace KamataEngine;

TitleScene::TitleScene() {}

TitleScene::~TitleScene() 
{

	// 3Dモデルデータの解放
	// 自キャラの3Dモデルデータの解放
	delete modelPlayer_;

	// タイトル文字の3Dモデルデータの解放
	delete modelTitleText_;

	// カメラコントローラの解放
	delete cameraController_;

	// フェードの解放
	delete fade_;
}

void TitleScene::Initialize() 
{
	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// タイトルのワールドトランスフォームの初期化と座標設定
	worldTransformTitle_.Initialize();
	// 画面の中央より少し上に配置
	worldTransformTitle_.translation_ = {0.0f, 6.0f, 20.0f};

	// ファイル名を指定してテクスチャを読み込む

	// 3Dモデルデータの生成
	// プレイヤーのモデルデータ生成
	modelPlayer_ = Model::CreateFromOBJ("player", true);

	// タイトル文字のモデルデータ生成
	modelTitleText_ = Model::CreateFromOBJ("titleFont", true);

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
	player_->Initialize(modelPlayer_, nullptr, cameraController_->GetCamera(), playerPosition);
	
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
	titleFloatingTimer_ += 0.05f;
	// sinf関数にタイマーを渡してY座標を計算
	worldTransformTitle_.translation_.y = 6.0f + sinf(titleFloatingTimer_) * 0.5f;

	// タイトルのワールド行列を更新して定数バッファに転送
	worldTransformTitle_.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransformTitle_.scale_, worldTransformTitle_.rotation_, worldTransformTitle_.translation_);
	worldTransformTitle_.TransferMatrix();

	
}

// 描画処理
void TitleScene::Draw() 
{
	// 3Dモデル描画前処理
	Model::PreDraw();

	// プレイヤーの描画
	player_->Draw();

	// タイトルの描画
	modelTitleText_->Draw(worldTransformTitle_, *cameraController_->GetCamera());

	// フェードの描画
	fade_->Draw();

	// 3Dモデル描画後処理
	Model::PostDraw();
}
