#pragma once
#include "KamataEngine.h"
#include <vector>
#include "Player.h"
#include "CameraController.h"
#include "Fade.h"

/// <summary>
/// タイトルシーン
/// </summary>
class TitleScene
{
public:
	// プレイヤー
	Player* player_ = nullptr;

	// カメラコントローラ
	CameraController* cameraController_ = nullptr;

	// フェード
	Fade* fade_ = nullptr;

	TitleScene();

	~TitleScene();

public:
	void Initialize();

	void Update();

	void Draw();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }

private:
	
	// シーンのフェーズ
	enum class Phase
	{
		kFadeIn,  // フェードイン
		kMain,	  // メイン部
		kFadeOut, // フェードアウト
	};

	// 現在のフェーズ
	Phase phase_ = Phase::kFadeIn;

	// 終了フラグ
	bool finished_ = false;

	// フェードにかける時間
	static inline const float kFadeDuration = 2.0f;

	// 3Dモデルデータ
	// プレイヤーの3Dモデルデータ
	KamataEngine::Model* modelPlayer_ = nullptr;

	// 背景透過のタイトルロゴ
	KamataEngine::Sprite* titleLogoSprite_ = nullptr;

	// タイトルを浮遊させるためのタイマー変数
	float titleFloatingTimer_ = 0.0f;

};
