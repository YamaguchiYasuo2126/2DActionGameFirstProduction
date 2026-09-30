#pragma once
#include "KamataEngine.h"
#include "Fade.h"

/// <summary>
/// クリアシーン
/// </summary>
class ClearScene
{
public:
	ClearScene();
	~ClearScene();

	void Initialize();
	void Update();
	void Draw();

	// フェードアウト完了後、main.cpp がタイトルシーンへ遷移するためのフラグ
	bool IsFinished() const { return finished_; }

private:
	enum class Phase
	{
		kFadeIn,
		kMain,
		kFadeOut,
	};

	Phase phase_ = Phase::kFadeIn;
	bool finished_ = false;

	static inline const float kFadeDuration = 1.0f;

	// クリア画面の背景。後でクリア用画像に差し替え可能。
	KamataEngine::Sprite* backgroundSprite_ = nullptr;
	Fade* fade_ = nullptr;
};
