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

	// 背景色と、背景透過のクリア文字を別々に描画する。
	KamataEngine::Sprite* backgroundSprite_ = nullptr;
	KamataEngine::Sprite* clearLogoSprite_ = nullptr;
	Fade* fade_ = nullptr;
};
