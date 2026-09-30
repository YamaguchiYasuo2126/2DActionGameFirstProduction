#pragma once

#include "BaseEnemy.h"

// 空中を左右に巡回する敵
class FlyingEnemy : public BaseEnemy {
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera,
		const KamataEngine::Vector3& position) override;

protected:
	void BehaviorWalkUpdate() override;

private:
	KamataEngine::Vector3 startPosition_{};
	float flightTimer_ = 0.0f;
};
