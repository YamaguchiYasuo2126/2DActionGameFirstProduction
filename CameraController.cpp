#include "CameraController.h"
#include "Player.h"
#include "MyMathUtility.h"
#define NOMINMAX
#include <windows.h>
#include <cassert>
#include <algorithm>
#include <cmath>

using namespace KamataEngine;

void CameraController::Initialize() {
	// カメラの設定
	camera_.farZ = 6000.0f;

	// カメラの初期化
	camera_.Initialize();
	
}

void CameraController::Update() {
	assert(target_ != nullptr);

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// Playerから現在の速度を取得する
	const KamataEngine::Vector3& targetVelocity = target_->GetVelocity();

	// 速度によるカメラの先行（バイアス）計算
	float velocityBiasY = (std::max)(targetVelocity.y, 0.0f) * kVelocityBias;

	// 追従対象とオフセットと追従対象の速度からカメラの目標座標を計算
	targetCoordinate_.x = isFixedCameraX_ ? fixedCameraX_ : targetWorldTransform.translation_.x + targetOffset_.x;
	targetCoordinate_.y = targetWorldTransform.translation_.y + targetOffset_.y + velocityBiasY;
	targetCoordinate_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// Xは補間しない
	camera_.translation_.x = isFixedCameraX_ ? fixedCameraX_ : targetWorldTransform.translation_.x + targetOffset_.x;
	camera_.translation_.y = MyMathUtility::Lerp(camera_.translation_.y, targetCoordinate_.y, kInterpolationRate);
	camera_.translation_.z = MyMathUtility::Lerp(camera_.translation_.z, targetCoordinate_.z, kInterpolationRate);

	// 追従対象が画面外に出ないように補正
	/*camera_.translation_.x = (std::max)(camera_.translation_.x, targetWorldTransform.translation_.x + margin_.left);
	camera_.translation_.x = (std::min)(camera_.translation_.x, targetWorldTransform.translation_.x + margin_.right);*/
	camera_.translation_.y = (std::max)(camera_.translation_.y, targetWorldTransform.translation_.y + margin_.bottom);
	camera_.translation_.y = (std::min)(camera_.translation_.y, targetWorldTransform.translation_.y + margin_.top);

	// 移動範囲の制限 (クランプ)
	/*camera_.translation_.x = std::clamp(camera_.translation_.x, movableArea_.left, movableArea_.right);*/
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

	// 行列を更新する
	camera_.UpdateMatrix();
}

void CameraController::Reset() {
	assert(target_ != nullptr);

	// 追従対象のワールドトランスフォームを参照
	const WorldTransform& targetWorldTransform = target_->GetWorldTransform();
	// 追従対象とオフセットからカメラの座標を計算
	camera_.translation_.x = isFixedCameraX_ ? fixedCameraX_ : targetWorldTransform.translation_.x + targetOffset_.x;
	camera_.translation_.y = targetWorldTransform.translation_.y + targetOffset_.y;
	camera_.translation_.z = targetWorldTransform.translation_.z + targetOffset_.z;

	// 移動範囲の制限 (クランプ)
	
	camera_.translation_.y = std::clamp(camera_.translation_.y, movableArea_.bottom, movableArea_.top);

}