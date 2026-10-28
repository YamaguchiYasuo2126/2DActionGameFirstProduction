#include "CheckpointTrigger.h"

#include "MyMathUtility.h"
#include "Player.h"
#include "SoundManager.h"
#include "WorldProgress.h"

using namespace KamataEngine;

void CheckpointTrigger::Initialize(Model* model, Model* jewelModel, Camera* camera,
	uint32_t whiteTextureHandle, const Vector3& position,
	WorldProgress* progress, std::string roomPath, std::string checkpointId)
{
	model_ = model;
	jewelModel_ = jewelModel;
	camera_ = camera;
	whiteTextureHandle_ = whiteTextureHandle;
	progress_ = progress;
	roomPath_ = std::move(roomPath);
	checkpointId_ = std::move(checkpointId);
	progressFlag_ = roomPath_ + ":checkpoint:" + checkpointId_;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// 制作したモデルは約1マスの大きさ。仮ブロック用の縮小を外す。
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	// OBJの台座底面は原点から約0.31下。床面は配置点から0.5下なので、見た目だけ下げる。
	constexpr float kModelOffsetY = -0.19f;
	modelWorldTransform_.Initialize();
	modelWorldTransform_.translation_ = position;
	modelWorldTransform_.translation_.y += kModelOffsetY;
	modelWorldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		modelWorldTransform_.scale_, modelWorldTransform_.rotation_, modelWorldTransform_.translation_);
	modelWorldTransform_.TransferMatrix();

	goldJewelTransform_.Initialize();
	goldJewelTransform_.translation_ = modelWorldTransform_.translation_;
	// 元の結晶の表面より少し手前に描き、重なりによるちらつきを防ぐ。
	goldJewelTransform_.scale_ = {1.035f, 1.035f, 1.035f};
	goldJewelTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		goldJewelTransform_.scale_, goldJewelTransform_.rotation_, goldJewelTransform_.translation_);
	goldJewelTransform_.TransferMatrix();
	goldJewelColor_.Initialize();
	goldJewelColor_.SetColor({1.0f, 0.78f, 0.16f, 1.0f});
}

void CheckpointTrigger::Draw()
{
	if (model_ && camera_)
	{
		model_->Draw(modelWorldTransform_, *camera_);
		if (progress_ && progress_->HasFlag(progressFlag_) && jewelModel_)
		{
			// 白テクスチャへの差し替えで、水色の絵柄に影響されず金色を出す。
			jewelModel_->Draw(goldJewelTransform_, *camera_, whiteTextureHandle_, &goldJewelColor_);
		}
	}
}

AABB CheckpointTrigger::GetAABB() const
{
	const Vector3& position = worldTransform_.translation_;
	return {{position.x - 0.5f, position.y - 0.5f, position.z - 0.5f},
		{position.x + 0.5f, position.y + 0.5f, position.z + 0.5f}};
}

void CheckpointTrigger::OnPlayerCollision(Player* player)
{
	if (!progress_ || !player)
	{
		return;
	}

	// 一度起動した地点へ戻った場合も、現在の復活地点として選び直せる。
	if (!progress_->IsCurrentCheckpoint(roomPath_, checkpointId_))
	{
		progress_->ActivateCheckpoint(roomPath_, checkpointId_, worldTransform_.translation_);
		SoundManager::GetInstance()->PlaySE(SoundEffect::kCheckpoint);
	}
}
