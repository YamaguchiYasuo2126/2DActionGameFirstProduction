#pragma once

#include "KamataEngine.h"
#include <array>
#include <cstdint>

/// <summary>
/// 効果音の種類
/// </summary>
enum class SoundEffect : uint32_t
{
	kDecision,
	kEnemyDeath,
	kGoal,
	kJump,
	kPlayerDeath,
	kPlayerRespawn,
	kStageSelect,
	kPlayerAttack,
	kCount,
};

/// <summary>
/// 効果音の読み込み・再生をまとめて管理するクラス
/// </summary>
class SoundManager
{
public:
	static SoundManager* GetInstance();

	/// <summary>
	/// Resources/SE 内の効果音を読み込む
	/// </summary>
	void Initialize();

	/// <summary>
	/// 指定した効果音を一度だけ再生する
	/// </summary>
	void PlaySE(SoundEffect soundEffect, float volume = 0.75f);

private:
	SoundManager() = default;
	~SoundManager() = default;
	SoundManager(const SoundManager&) = delete;
	SoundManager& operator=(const SoundManager&) = delete;

	std::array<uint32_t, static_cast<size_t>(SoundEffect::kCount)> soundHandles_{};
	bool isInitialized_ = false;
};
