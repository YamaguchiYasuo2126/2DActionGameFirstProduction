#include "HitEffect.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "MyMathUtility.h"
#include <cassert>
#include <random> 
#include <numbers>
#include "EasingUtility.h"
#include <algorithm>

using namespace KamataEngine;

// 静的メンバ変数の実体
KamataEngine::Model* HitEffect::model_ = nullptr;
KamataEngine::Camera* HitEffect::camera_ = nullptr;

HitEffect* HitEffect::Create(const Vector3& position)
{
	// インスタンス生成
	HitEffect* instance = new HitEffect();
	// newの失敗を検出
	assert(instance);
	// インスタンスの初期化
	instance->Initialize(position);
	// 初期化したインスタンスを返す
	return instance;

}

void HitEffect::Initialize(const Vector3& position) 
{
	// 円形エフェクト
	circleWorldTransform_.Initialize();
	circleWorldTransform_.translation_ = position;

	// 乱数生成エンジン
	std::random_device seedGenerator;
	std::mt19937_64 randomEngine;
	randomEngine.seed(seedGenerator());

	// 指定範囲の乱数生成器（-π から +π まで）
	std::uniform_real_distribution<float> rotationDistribution(-std::numbers::pi_v<float>, std::numbers::pi_v<float>);

	// 楕円エフェクト
	for (WorldTransform& worldTransform : ellipseWorldTransforms_)
	{
		worldTransform.scale_ = {kEllipseWidth, kEllipseLength, 1.0f};
		worldTransform.rotation_ = {0.0f, 0.0f, rotationDistribution(randomEngine)};
		worldTransform.translation_ = position;

		worldTransform.Initialize();
	}

}

void HitEffect::Update() 
{
	// 1フレーム分の時間をカウントアップ
	counter_ += 1.0f / 60.0f;

	// フェーズごとの処理
	switch (phase_) 
	{
	case Phase::kSpread:
	{
		// 0.0f ～ 1.0f の割合を計算
		float t = std::clamp(counter_ / kSpreadDuration, 0.0f, 1.0f);

		// イージングで円のスケールを大きくする（0.0f から 0.6f へ）
		float circleScale = EasingUtility::EaseOut(0.0f, 0.6f, t);
		circleWorldTransform_.scale_ = {circleScale, circleScale, circleScale};

		// 楕円の長さもイージングで伸ばす
		for (WorldTransform& worldTransform : ellipseWorldTransforms_)
		{
			float ellipseLength = EasingUtility::EaseOut(0.0f, kEllipseLength, t);
			worldTransform.scale_ = {kEllipseWidth, ellipseLength, 1.0f};
		}

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

	// 楕円エフェクトの更新
	for (WorldTransform& worldTransform : ellipseWorldTransforms_) 
	{
		// アフィン変換行列の作成
		worldTransform.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransform.scale_, worldTransform.rotation_, worldTransform.translation_);

		// 行列を定数バッファに転送
		worldTransform.TransferMatrix();
	}
}

void HitEffect::Draw() 
{
	// 3Dモデル描画前処理
	Model::PreDraw();

	// 描画の直前にSetAlphaを使って透明度をセット
	model_->SetAlpha(alpha_);

	// 円形エフェクト
	model_->Draw(circleWorldTransform_, *camera_);

	// 楕円エフェクトの描画
	for (WorldTransform& worldTransform : ellipseWorldTransforms_)
	{
		model_->Draw(worldTransform, *camera_);
	}

	// 3Dモデル描画後処理
	Model::PostDraw();
}