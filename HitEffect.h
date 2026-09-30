#pragma once
#include "BaseEffect.h"
#include "KamataEngine.h"
#include <array>

/// <summary>
/// ヒット演出用エフェクト
/// </summary>
class HitEffect : public BaseEffect 
{
public:
	void Initialize(const KamataEngine::Vector3& position) override;

	void Update() override;

	void Draw() override;

	// モデルのsetter
	static void SetModel(KamataEngine::Model* model) { model_ = model; }
	// カメラのsetter
	static void SetCamera(KamataEngine::Camera* camera) { camera_ = camera; }

	static HitEffect* Create(const KamataEngine::Vector3& position);

	// エフェクトが終了したかどうかのゲッター
	// フェーズがkDeadかどうかを直接返す
	bool IsDead() const override { return phase_ == Phase::kDead; }

private:
	// コンストラクタをprivateにする
	HitEffect() {}
	
	// フェーズ
	enum class Phase
	{
		kSpread, // 拡大
		kFade,   // フェードアウト
		kDead,   // 消滅
	};

	// 現在のフェーズ
	Phase phase_ = Phase::kSpread;

	// 経過時間カウンター
	float counter_ = 0.0f;

	// 拡大にかける時間（秒）
	static inline const float kSpreadDuration = 0.2f;
	// フェードアウトにかける時間（秒）
	static inline const float kFadeDuration = 0.3f;

	// モデル (借りてくる用)
	static KamataEngine::Model* model_;
	// カメラ (借りてくる用)
	static KamataEngine::Camera* camera_;

	// 円のワールドトランスフォーム
	KamataEngine::WorldTransform circleWorldTransform_;

	// 楕円の個数
	static inline const int kNumEllipse = 2;

	// 楕円の幅
	static inline const float kEllipseWidth = 0.1f;

	// 楕円の長さ
	static inline const float kEllipseLength = 1.5f;

	// 楕円のワールドトランスフォーム
	std::array<KamataEngine::WorldTransform, kNumEllipse> ellipseWorldTransforms_;

	// アルファ値
	float alpha_ = 1.0f;
};
