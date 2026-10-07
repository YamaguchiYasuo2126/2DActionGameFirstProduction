#pragma once

#include "KamataEngine.h"

#include <string>
#include <unordered_set>

class WorldProgress
{
public:
	void ActivateCheckpoint(const std::string& roomPath, const std::string& checkpointId,
		const KamataEngine::Vector3& respawnPosition)
	{
		checkpointRoomPath_ = roomPath;
		checkpointId_ = checkpointId;
		respawnPosition_ = respawnPosition;
		hasCheckpoint_ = true;
		SetFlag(roomPath + ":checkpoint:" + checkpointId);
	}

	bool HasCheckpoint() const { return hasCheckpoint_; }
	const std::string& GetCheckpointRoomPath() const { return checkpointRoomPath_; }
	const KamataEngine::Vector3& GetRespawnPosition() const { return respawnPosition_; }

	void SetFlag(const std::string& flag) { flags_.insert(flag); }
	bool HasFlag(const std::string& flag) const { return flags_.contains(flag); }

private:
	bool hasCheckpoint_ = false;
	std::string checkpointRoomPath_;
	std::string checkpointId_;
	KamataEngine::Vector3 respawnPosition_{};
	std::unordered_set<std::string> flags_;
};
