#pragma once
#include "BaseEffect.h"
#include "KamataEngine.h"
#include <array>

/// <summary>
/// ガード演出用エフェクト
/// </summary>
class GuardEffect : public BaseEffect
{
public:
	void Initialize(const KamataEngine::Vector3& position) override;

	void Update() override;

	void Draw() override;

	// モデルのsetter
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	// カメラのsetter
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

	static GuardEffect* Create(const KamataEngine::Vector3& position);

	// エフェクトが終了したかどうかのゲッター
	// フェーズがkDeadかどうかを直接返す
	bool IsDead() const override { return phase_ == Phase::kDead; }

private:
	// コンストラクタをprivateにする
	GuardEffect() {}
	
	// フェーズ
	enum class Phase
	{
		kStop,   // 発生直後に停止
		kSpread, // 拡大
		kFade,   // フェードアウト
		kDead,   // 消滅
	};

	// 現在のフェーズ
	Phase phase_ = Phase::kStop;

	// 経過時間カウンター
	float counter_ = 0.0f;

	// 停止時間
	static inline const float kStopDuration = 0.05f;
	// 拡大にかける時間
	static inline const float kSpreadDuration = 0.2f;
	// フェードアウトにかける時間
	static inline const float kFadeDuration = 0.3f;

	// モデル (借りてくる用)
	static KamataEngine::Model* model_;
	// カメラ (借りてくる用)
	static KamataEngine::Camera* camera_;

	// 円のワールドトランスフォーム
	KamataEngine::WorldTransform circleWorldTransform_;

	// アルファ値
	float alpha_ = 1.0f;
};
