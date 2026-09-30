#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"

// 下からは通過でき、上から着地すると復活地点を更新する一方向足場。
class CheckpointGate : public BaseGimmick
{
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera,
		const KamataEngine::Vector3& position, uint8_t checkpointIndex,
		KamataEngine::Vector3* respawnPosition, int32_t* activatedCheckpointIndex);

	void Update() override {}
	void Draw() override;
	AABB GetAABB() const override;
	void OnPlayerCollision(Player* player) override;
	bool IsActive() const override { return true; }
	bool IsSolid() const override { return true; }

private:
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	KamataEngine::WorldTransform worldTransform_;
	uint8_t checkpointIndex_ = 0;
	KamataEngine::Vector3* respawnPosition_ = nullptr;
	int32_t* activatedCheckpointIndex_ = nullptr;

	static inline const float kWidth = 21.0f;
	static inline const float kHeight = 0.20f;
};
