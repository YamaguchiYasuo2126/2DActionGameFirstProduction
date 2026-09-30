#include "GuardEffect.h"
#define _USE_MATH_DEFINES
#include "EasingUtility.h"
#include "MyMathUtility.h"
#include <algorithm>
#include <cassert>
#include <math.h>
#include <numbers>
#include <random>

using namespace KamataEngine;

// 静的メンバ変数の実体
KamataEngine::Model* GuardEffect::model_ = nullptr;
KamataEngine::Camera* GuardEffect::camera_ = nullptr;

GuardEffect* GuardEffect::Create(const Vector3& position) {
	// インスタンス生成
	GuardEffect* instance = new GuardEffect();
	// newの失敗を検出
	assert(instance);
	// インスタンスの初期化
	instance->Initialize(position);
	// 初期化したインスタンスを返す
	return instance;
}

void GuardEffect::Initialize(const Vector3& position) {
	// 円形エフェクト
	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = position;

	// 発生直後の初期サイズを設定
	circleWorldTransform_.scale_ = {0.3f, 0.3f, 0.3f};
}

void GuardEffect::Update() {
	// 1フレーム分の時間をカウントアップ
	counter_ += 1.0f / 60.0f;

	// フェーズごとの処理
	switch (phase_)
	{
	case Phase::kStop:
		// 停止フェーズ：指定時間待つだけ
		if (counter_ >= kStopDuration)
		{
			phase_ = Phase::kSpread;
			counter_ = 0.0f;
		}
		break;

	case Phase::kSpread:
	{
		// 0.0f ～ 1.0f の割合を計算
		float t = std::clamp(counter_ / kSpreadDuration, 0.0f, 1.0f);

		// イージングで円のスケールを大きくする（0.3f から 1.5f くらいへ）
		float circleScale = EasingUtility::EaseOut(0.3f, 1.5f, t);
		circleWorldTransform_.scale_ = {circleScale, circleScale, circleScale};

		// 持続時間を超えたら「フェードフェーズ」へ切り替え
		if (counter_ >= kSpreadDuration) 
		{
			phase_ = Phase::kFade;
			counter_ = 0.0f; // カウンターをリセット
		}
	} break;

	case Phase::kFade:
	{
		float t = std::clamp(counter_ / kFadeDuration, 0.0f, 1.0f);

		// 1.0fから 0.0fに向かって減らす
		alpha_ = 1.0f - t;

		// 持続時間を超えたら「消滅フェーズ」へ切り替え
		if (counter_ >= kFadeDuration) 
		{
			phase_ = Phase::kDead;
		}
	} break;

	case Phase::kDead:
		break;
	}

	// 円形エフェクト
	// アフィン変換行列の作成
	circleWorldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(circleWorldTransform_.scale_, circleWorldTransform_.rotation_, circleWorldTransform_.translation_);

	// 行列を定数バッファに転送
	circleWorldTransform_.TransferMatrix();

}

void GuardEffect::Draw() {
	// 3Dモデル描画前処理
	Model::PreDraw();

	// 描画の直前にSetAlphaを使って透明度をセット
	model_->SetAlpha(alpha_);

	// 円形エフェクト
	model_->Draw(circleWorldTransform_, *camera_);

	// 3Dモデル描画後処理
	Model::PostDraw();
}