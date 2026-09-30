#pragma once
#include "Fade.h"
#include "KamataEngine.h"
#include "StageManager.h"
#include <vector>

class StageSelectScene
{
public:
	StageSelectScene() = default;
	~StageSelectScene();

	void Initialize(StageManager* stageManager);
	void Update();
	void Draw();

	bool IsFinished() const { return finished_; }

private:
	StageManager* stageManager_ = nullptr;
	Fade* fade_ = nullptr;
	KamataEngine::Sprite* backgroundSprite_ = nullptr;
	KamataEngine::Sprite* selectorSprite_ = nullptr;
	std::vector<KamataEngine::Sprite*> stageCards_;

	int32_t selectedStageIndex_ = 0;
	bool finished_ = false;

	enum class Phase { kFadeIn, kMain, kFadeOut };
	Phase phase_ = Phase::kFadeIn;

	static inline const float kFadeDuration = 1.0f;
};
