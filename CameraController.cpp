#include "CameraController.h"
#include "GameTime.h"
#include "Player.h"
#include "MyMathUtility.h"
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <algorithm>
#include <cmath>

using namespace KamataEngine;

void CameraController::Initialize()
{
	// カメラの設定
	camera_.farZ = 6000.0f;

	// カメラの初期化
	camera_.Initialize();
	
}

void CameraController::Update()
{
	assert(target_ != nullptr);
	// 前フレームの描画用シェイクを外し、追従補間に揺れを蓄積させない。
	camera_.translation_.x -= shakeOffset_.x;
	camera_.translation_.y -= shakeOffset_.y;
	shakeOffset_ = {};

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// Playerから現在の速度を取得する
	const KamataEngine::Vector3& targetVelocity = target_->GetVelocity();

	// 進行方向へ先行する。突進で速度が上がっても横の先行量は3マスまで。
	const float velocityBiasX = std::clamp(targetVelocity.x * kVelocityBiasX, -kMaxVelocityBiasX, kMaxVelocityBiasX);
	const float velocityBiasY = (std::max)(targetVelocity.y, 0.0f) * kVelocityBiasY;

	// 追従対象とオフセットと追従対象の速度からカメラの目標座標を計算
	targetCoordinate_.x = isFixedCameraX_ ? fixedCameraX_ :
		targetWorldTransform.translation_.x + targetOffset_.x + velocityBiasX;
	targetCoordinate_.y = targetWorldTransform.translation_.y + targetOffset_.y + velocityBiasY;
	targetCoordinate_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// 横長の部屋も追従できるよう、X方向も補間して追従する。
	const float interpolationRate = GameTime::GetLerpFactor(kInterpolationRate);
	camera_.translation_.x = isFixedCameraX_
		? fixedCameraX_
		: MyMathUtility::Lerp(camera_.translation_.x, targetCoordinate_.x, interpolationRate);
	camera_.translation_.y = MyMathUtility::Lerp(camera_.translation_.y, targetCoordinate_.y, interpolationRate);
	camera_.translation_.z = MyMathUtility::Lerp(camera_.translation_.z, targetCoordinate_.z, interpolationRate);

	// 追従対象が画面外に出ないように補正
	camera_.translation_.x = (std::max)(camera_.translation_.x, targetWorldTransform.translation_.x + margin_.left);
	camera_.translation_.x = (std::min)(camera_.translation_.x, targetWorldTransform.translation_.x + margin_.right);
	camera_.translation_.y = (std::max)(camera_.translation_.y, targetWorldTransform.translation_.y + margin_.bottom);
	camera_.translation_.y = (std::min)(camera_.translation_.y, targetWorldTransform.translation_.y + margin_.top);

	// 移動範囲の制限 (クランプ)
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

	if (shakeRemainingSeconds_ > 0.0f)
	{
		const float deltaSeconds = GameTime::GetUnscaledDeltaSeconds();
		shakeElapsedSeconds_ += deltaSeconds;
		shakeRemainingSeconds_ = (std::max)(0.0f, shakeRemainingSeconds_ - deltaSeconds);
		const float fade = shakeRemainingSeconds_ / shakeDurationSeconds_;
		const float baseX = camera_.translation_.x;
		const float baseY = camera_.translation_.y;
		const float offsetX = std::sin(shakeElapsedSeconds_ * 83.0f) * shakeAmplitude_ * fade;
		const float offsetY = std::sin(shakeElapsedSeconds_ * 111.0f + 1.2f) * shakeAmplitude_ * 0.7f * fade;
		// 部屋の端では揺れもクランプし、マップの外を映さない。
		camera_.translation_.x = std::clamp(baseX + offsetX, movableArea_.left, movableArea_.right);
		camera_.translation_.y = std::clamp(baseY + offsetY, movableArea_.bottom, movableArea_.top);
		shakeOffset_ = {camera_.translation_.x - baseX, camera_.translation_.y - baseY, 0.0f};
	}

	// 行列を更新する
	camera_.UpdateMatrix();
}

void CameraController::Reset() 
{
	assert(target_ != nullptr);
	shakeAmplitude_ = 0.0f;
	shakeDurationSeconds_ = 0.0f;
	shakeRemainingSeconds_ = 0.0f;
	shakeElapsedSeconds_ = 0.0f;
	shakeOffset_ = {};

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	camera_.translation_.x = isFixedCameraX_ ? fixedCameraX_ : targetWorldTransform.translation_.x + targetOffset_.x;
	camera_.translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_.translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// 移動範囲の制限 (クランプ)
	camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

}

void CameraController::StartShake(float amplitude, float durationSeconds)
{
	if (amplitude <= 0.0f || durationSeconds <= 0.0f)
	{
		return;
	}
	if (shakeRemainingSeconds_ > 0.0f && amplitude < shakeAmplitude_)
	{
		return;
	}
	shakeAmplitude_ = amplitude;
	shakeDurationSeconds_ = durationSeconds;
	shakeRemainingSeconds_ = durationSeconds;
	shakeElapsedSeconds_ = 0.0f;
}
