#include "SoundManager.h"

SoundManager* SoundManager::GetInstance()
{
	static SoundManager instance;
	return &instance;
}

void SoundManager::Initialize()
{
	if (isInitialized_)
	{
		return;
	}

	KamataEngine::Audio* audio = KamataEngine::Audio::GetInstance();
	// Audio は Resources フォルダを基準に読み込む。
	soundHandles_[static_cast<size_t>(SoundEffect::kDecision)] = audio->LoadWave("SE/decision.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kEnemyDeath)] = audio->LoadWave("SE/enemyDeath.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kGoal)] = audio->LoadWave("SE/goal.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kJump)] = audio->LoadWave("SE/jump.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kPlayerDeath)] = audio->LoadWave("SE/playerDeath.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kPlayerRespawn)] = audio->LoadWave("SE/playerRespawn.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kStageSelect)] = audio->LoadWave("SE/stageSelect.wav");
	soundHandles_[static_cast<size_t>(SoundEffect::kPlayerAttack)] = audio->LoadWave("SE/playerAttack.wav");

	isInitialized_ = true;
}

void SoundManager::PlaySE(SoundEffect soundEffect, float volume)
{
	if (!isInitialized_)
	{
		return;
	}

	KamataEngine::Audio::GetInstance()->PlayWave(
		soundHandles_[static_cast<size_t>(soundEffect)], false, volume);
}
