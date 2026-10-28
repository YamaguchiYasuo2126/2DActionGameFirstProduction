#include "ClearScene.h"
#include "GamePadInput.h"
#include "GameTime.h"
#include "SoundManager.h"

using namespace KamataEngine;

ClearScene::ClearScene() = default;

ClearScene::~ClearScene()
{
	delete backgroundSprite_;
	delete clearLogoSprite_;
	delete fade_;
}

void ClearScene::Initialize()
{
	GameTime::SetTimeScale(1.0f);
	// 背景は既存の単色のまま、クリア文字だけを重ねる。
	const uint32_t textureHandle = TextureManager::Load("white1x1.png");
	backgroundSprite_ = Sprite::Create(textureHandle, Vector2(0.0f, 0.0f));
	backgroundSprite_->SetSize(Vector2(1280.0f, 720.0f));
	backgroundSprite_->SetColor(Vector4(0.08f, 0.32f, 0.18f, 1.0f));
	const uint32_t logoTexture = TextureManager::Load("UI/gameClear.png");
	clearLogoSprite_ = Sprite::Create(logoTexture, Vector2(160.0f, 199.0f));
	clearLogoSprite_->SetSize(Vector2(960.0f, 322.0f));

	fade_ = new Fade();
	fade_->Initialize();
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);
	phase_ = Phase::kFadeIn;
	finished_ = false;
}

void ClearScene::Update()
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
		// Space を押したらタイトル画面へ戻るためのフェードアウトを開始する。
		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsConfirmTriggered())
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kDecision);
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		if (fade_->IsFinished())
		{
			finished_ = true;
		}
		break;
	}
}

void ClearScene::Draw()
{
	Sprite::PreDraw();
	backgroundSprite_->Draw();
	clearLogoSprite_->Draw();
	Sprite::PostDraw();

	// 背景の上へ黒フェードを重ねる。
	fade_->Draw();
}
