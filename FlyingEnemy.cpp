#include "FlyingEnemy.h"

#include <cmath>

using namespace KamataEngine;

void FlyingEnemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	BaseEnemy::Initialize(model, camera, position);
	startPosition_ = position;
	flightTimer_ = 0.0f;
}

void FlyingEnemy::BehaviorWalkUpdate() {
	flightTimer_ += 1.0f / 60.0f;

	worldTransform_.translation_ = startPosition_;
	worldTransform_.translation_.x += std::sin(flightTimer_ * 1.7f) * 1.6f;
	worldTransform_.translation_.y += std::sin(flightTimer_ * 3.4f) * 0.35f;
	worldTransform_.rotation_.z = std::cos(flightTimer_ * 1.7f) * 0.18f;

	UpdateMatrix();
}
