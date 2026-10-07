#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"

#include <string>

class WorldProgress;

class CheckpointTrigger : public BaseGimmick
{
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera,
		const KamataEngine::Vector3& position, WorldProgress* progress,
		std::string roomPath, std::string checkpointId);

	void Update() override {}
	void Draw() override;
	AABB GetAABB() const override;
	void OnPlayerCollision(Player* player) override;
	bool IsActive() const override { return true; }
	bool IsSolid() const override { return false; }

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	WorldProgress* progress_ = nullptr;
	std::string roomPath_;
	std::string checkpointId_;
};
