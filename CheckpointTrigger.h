#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"

#include <string>

class WorldProgress;

class CheckpointTrigger : public BaseGimmick
{
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* jewelModel,
		KamataEngine::Camera* camera, uint32_t whiteTextureHandle,
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
	KamataEngine::Model* jewelModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform modelWorldTransform_;
	KamataEngine::WorldTransform goldJewelTransform_;
	KamataEngine::ObjectColor goldJewelColor_;
	uint32_t whiteTextureHandle_ = 0;
	WorldProgress* progress_ = nullptr;
	std::string roomPath_;
	std::string checkpointId_;
	std::string progressFlag_;
};
