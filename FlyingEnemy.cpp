#include "FlyingEnemy.h"
#include "GameTime.h"

#include <cmath>
#include <numbers>

using namespace KamataEngine;

void FlyingEnemy::Initialize(Model* model, Camera* camera, const Vector3& position) {
	BaseEnemy::Initialize(model, camera, position);
	startPosition_ = position;
	flightTimer_ = 0.0f;
	velocity_.x = -0.04f;
	velocity_.y = 0.0f;
}

void FlyingEnemy::BehaviorWalkUpdate() {
	flightTimer_ += GameTime::GetDeltaSeconds();

	// 巡回範囲の端と地形の壁で折り返す。座標を毎回上書きすると壁を貫通する。
	constexpr float kPatrolRadius = 1.6f;
	const float x = worldTransform_.translation_.x;
	if ((x <= startPosition_.x - kPatrolRadius && velocity_.x < 0.0f) ||
		(x >= startPosition_.x + kPatrolRadius && velocity_.x > 0.0f)) {
		velocity_.x = -velocity_.x;
	}
	const float targetY = startPosition_.y + std::sin(flightTimer_ * 3.4f) * 0.35f;
	const TerrainHit hit = MoveWithMapCollision(
		velocity_.x * GameTime::GetDeltaFrames(), targetY - worldTransform_.translation_.y);
	if (hit.wall) {
		velocity_.x = -velocity_.x;
	}
	worldTransform_.rotation_.y = velocity_.x >= 0.0f
		? std::numbers::pi_v<float> * 0.5f : -std::numbers::pi_v<float> * 0.5f;
	worldTransform_.rotation_.z = std::cos(flightTimer_ * 1.7f) * 0.18f;

	UpdateMatrix();
}
