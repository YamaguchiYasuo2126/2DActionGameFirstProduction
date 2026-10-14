#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>

// 60FPS時の既存パラメータを保ったまま、実時間とタイムスケールを共有する。
class GameTime final
{
public:
	static void Initialize()
	{
		lastTime_ = Clock::now();
		deltaSeconds_ = kReferenceDeltaSeconds;
		timeScale_ = 1.0f;
		isFirstFrame_ = true;
	}

	static void BeginFrame()
	{
		const Clock::time_point now = Clock::now();
		if (isFirstFrame_)
		{
			lastTime_ = now;
			deltaSeconds_ = kReferenceDeltaSeconds;
			isFirstFrame_ = false;
			return;
		}
		const float elapsed = std::chrono::duration<float>(now - lastTime_).count();
		lastTime_ = now;
		// デバッガ停止後などに大量の移動を一度に実行しない。
		deltaSeconds_ = std::clamp(elapsed, 0.0f, 0.1f);
	}

	static void SetTimeScale(float timeScale) { timeScale_ = (std::max)(timeScale, 0.0f); }
	static float GetTimeScale() { return timeScale_; }
	static float GetDeltaSeconds() { return deltaSeconds_ * timeScale_; }
	static float GetUnscaledDeltaSeconds() { return deltaSeconds_; }
	static float GetDeltaFrames() { return GetDeltaSeconds() / kReferenceDeltaSeconds; }

	// 60FPS時の毎フレーム補間率を、可変フレームでも同じ応答にする。
	static float GetLerpFactor(float factorAt60Fps)
	{
		return 1.0f - std::pow(1.0f - factorAt60Fps, GetDeltaFrames());
	}

	// 0.9fなどの「1フレームごとの減衰率」を実時間に変換する。
	static float GetDampingFactor(float multiplierAt60Fps)
	{
		return std::pow(multiplierAt60Fps, GetDeltaFrames());
	}

private:
	using Clock = std::chrono::steady_clock;
	static inline constexpr float kReferenceDeltaSeconds = 1.0f / 60.0f;
	static inline Clock::time_point lastTime_ = Clock::now();
	static inline float deltaSeconds_ = kReferenceDeltaSeconds;
	static inline float timeScale_ = 1.0f;
	static inline bool isFirstFrame_ = true;
};
