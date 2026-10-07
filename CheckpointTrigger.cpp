#include "CheckpointTrigger.h"

#include "MyMathUtility.h"
#include "Player.h"
#include "WorldProgress.h"

using namespace KamataEngine;

void CheckpointTrigger::Initialize(Model* model, Camera* camera, const Vector3& position,
	WorldProgress* progress, std::string roomPath, std::string checkpointId)
{
	model_ = model;
	camera_ = camera;
	progress_ = progress;
	roomPath_ = std::move(roomPath);
	checkpointId_ = std::move(checkpointId);

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.scale_ = {0.45f, 0.45f, 0.45f};
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void CheckpointTrigger::Draw()
{
	if (model_ && camera_)
	{
		model_->Draw(worldTransform_, *camera_);
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

	const std::string flag = roomPath_ + ":checkpoint:" + checkpointId_;
	if (!progress_->HasFlag(flag))
	{
		progress_->ActivateCheckpoint(roomPath_, checkpointId_, worldTransform_.translation_);
	}
}
