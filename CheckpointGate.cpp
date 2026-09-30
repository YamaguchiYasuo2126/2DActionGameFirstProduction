#include "CheckpointGate.h"

#include "MyMathUtility.h"
#include "Player.h"

using namespace KamataEngine;

void CheckpointGate::Initialize(Model* model, Camera* camera, const Vector3& position,
	uint8_t checkpointIndex, Vector3* respawnPosition, int32_t* activatedCheckpointIndex)
{
	model_ = model;
	camera_ = camera;
	checkpointIndex_ = checkpointIndex;
	respawnPosition_ = respawnPosition;
	activatedCheckpointIndex_ = activatedCheckpointIndex;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	// CSV上のチェックポイントマスを基準にすると、幅21マスの足場が右へ1マスずれる。
	// 左へ1マス補正し、左右の外壁（x=0, x=22）の内側へぴったり接続する。
	worldTransform_.translation_.x -= 1.0f;
	worldTransform_.scale_ = {kWidth, kHeight, 1.0f};
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();
}

void CheckpointGate::Draw()
{
	model_->Draw(worldTransform_, *camera_);
}

AABB CheckpointGate::GetAABB() const
{
	const Vector3& position = worldTransform_.translation_;
	return {{position.x - kWidth * 0.5f, position.y - kHeight * 0.5f, position.z - 0.5f},
		{position.x + kWidth * 0.5f, position.y + kHeight * 0.5f, position.z + 0.5f}};
}

void CheckpointGate::OnPlayerCollision(Player* player)
{
	const AABB gateAABB = GetAABB();
	const AABB currentAABB = player->GetAABB();
	const AABB previousAABB = player->GetPreviousAABB();

	const bool overlapsX = currentAABB.max.x > gateAABB.min.x && currentAABB.min.x < gateAABB.max.x;
	const bool isFallingFromAbove =
		overlapsX &&
		player->GetVelocity().y <= 0.0f &&
		previousAABB.min.y >= gateAABB.max.y - 0.01f;

	// 上から落ちてきた場合だけ着地させる。下からは一切押し戻さない。
	if (!isFallingFromAbove)
	{
		return;
	}

	player->LandOnGimmick(gateAABB.max.y, 0.0f);

	if (respawnPosition_ && activatedCheckpointIndex_ &&
		static_cast<int32_t>(checkpointIndex_) > *activatedCheckpointIndex_)
	{
		// プレイヤーの中心を足場より少し上に置く。
		*respawnPosition_ = {worldTransform_.translation_.x,
			gateAABB.max.y + 0.5f,
			worldTransform_.translation_.z};
		*activatedCheckpointIndex_ = checkpointIndex_;
	}
}
