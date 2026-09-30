#include "StageSelectScene.h"
#include "GamePadInput.h"
#include "SoundManager.h"

using namespace KamataEngine;

namespace
{
constexpr float kCardWidth = 300.0f;
constexpr float kCardHeight = 240.0f;
constexpr float kCardStartX = 150.0f;
constexpr float kCardY = 240.0f;
constexpr float kCardInterval = 340.0f;
}

StageSelectScene::~StageSelectScene()
{
	delete backgroundSprite_;
	delete selectorSprite_;
	for (Sprite* stageCard : stageCards_)
	{
		delete stageCard;
	}
	delete fade_;
}

void StageSelectScene::Initialize(StageManager* stageManager)
{
	stageManager_ = stageManager;
	selectedStageIndex_ = stageManager_->GetCurrentStageIndex();
	finished_ = false;
	phase_ = Phase::kFadeIn;

	const uint32_t textureHandle = TextureManager::Load("white1x1.png");

	backgroundSprite_ = Sprite::Create(textureHandle, Vector2(0.0f, 0.0f));
	backgroundSprite_->SetSize(Vector2(1280.0f, 720.0f));
	backgroundSprite_->SetColor(Vector4(0.06f, 0.10f, 0.22f, 1.0f));

	// 各ステージを表すカードを生成する。カードの色で Stage 1～3 を区別する。
	const Vector4 cardColors[] = {
		Vector4(0.25f, 0.55f, 0.95f, 1.0f),
		Vector4(0.25f, 0.80f, 0.48f, 1.0f),
		Vector4(0.95f, 0.45f, 0.30f, 1.0f),
	};

	for (int32_t i = 0; i < stageManager_->GetStageCount(); ++i)
	{
		Sprite* stageCard = Sprite::Create(textureHandle, Vector2(kCardStartX + kCardInterval * i, kCardY));
		stageCard->SetSize(Vector2(kCardWidth, kCardHeight));
		// クリア済みのステージは金色にして、進行状況を選択画面でも確認できるようにする。
		stageCard->SetColor(stageManager_->IsStageCleared(i) ?
			Vector4(1.0f, 0.78f, 0.12f, 1.0f) : cardColors[i % std::size(cardColors)]);
		stageCards_.push_back(stageCard);
	}

	// 黄色い枠を、現在選択中のカードの周囲に表示する。
	selectorSprite_ = Sprite::Create(textureHandle, Vector2());
	selectorSprite_->SetSize(Vector2(kCardWidth + 16.0f, kCardHeight + 16.0f));
	selectorSprite_->SetColor(Vector4(1.0f, 0.90f, 0.15f, 1.0f));

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);
}

void StageSelectScene::Update()
{
	fade_->Update();

	switch (phase_)
	{
	case Phase::kFadeIn:
		if (fade_->IsFinished())
		{
			phase_ = Phase::kMain;
		}
		break;

	case Phase::kMain:
	{
		bool isSelectionMoved = false;
		if (Input::GetInstance()->TriggerKey(DIK_LEFT) || GamePadInput::IsLeftTriggered())
		{
			selectedStageIndex_ = (selectedStageIndex_ - 1 + stageManager_->GetStageCount()) % stageManager_->GetStageCount();
			isSelectionMoved = true;
		}
		if (Input::GetInstance()->TriggerKey(DIK_RIGHT) || GamePadInput::IsRightTriggered())
		{
			selectedStageIndex_ = (selectedStageIndex_ + 1) % stageManager_->GetStageCount();
			isSelectionMoved = true;
		}
		if (isSelectionMoved)
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kStageSelect);
		}

		// Space で選択を確定し、ゲームシーンへ遷移する。
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsConfirmTriggered())
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kDecision);
			stageManager_->SetCurrentStageIndex(selectedStageIndex_);
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}
		break;
	}

	case Phase::kFadeOut:
		if (fade_->IsFinished())
		{
			finished_ = true;
		}
		break;
	}

	selectorSprite_->SetPosition(Vector2(
		kCardStartX + kCardInterval * selectedStageIndex_ - 8.0f,
		kCardY - 8.0f));

#ifdef _DEBUG
	ImGui::Begin("Stage Select");
	ImGui::Text("LEFT / RIGHT: Select    SPACE: Start");
	for (int32_t i = 0; i < stageManager_->GetStageCount(); ++i)
	{
		const StageData& stageData = stageManager_->GetStageData(i);
		ImGui::Text("%s Stage %d : %s  [%s]", i == selectedStageIndex_ ? ">" : " ", i + 1,
			stageData.name.c_str(), stageManager_->IsStageCleared(i) ? "CLEAR" : "NOT CLEAR");
	}
	ImGui::End();
#endif
}

void StageSelectScene::Draw()
{
	Sprite::PreDraw();
	backgroundSprite_->Draw();
	for (Sprite* stageCard : stageCards_)
	{
		stageCard->Draw();
	}
	selectorSprite_->Draw();
	Sprite::PostDraw();

	fade_->Draw();
}
