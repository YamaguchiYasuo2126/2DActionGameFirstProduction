#pragma once

#include <algorithm>
#include <string>
#include <unordered_map>

// 同じlinkIdを持つスイッチと石橋だけで共有する、部屋内の時限状態。
class SwitchLinkState
{
public:
	void Activate(const std::string& linkId, float durationFrames)
	{
		if (!linkId.empty())
		{
			remainingFrames_[linkId] = durationFrames;
		}
	}

	bool IsActive(const std::string& linkId) const
	{
		return GetRemainingFrames(linkId) > 0.0f;
	}

	float GetRemainingFrames(const std::string& linkId) const
	{
		const auto it = remainingFrames_.find(linkId);
		return it != remainingFrames_.end() ? it->second : 0.0f;
	}

	void Update(float deltaFrames)
	{
		for (auto& [id, remaining] : remainingFrames_)
		{
			remaining = (std::max)(0.0f, remaining - deltaFrames);
		}
	}

	void Reset() { remainingFrames_.clear(); }

private:
	std::unordered_map<std::string, float> remainingFrames_;
};
