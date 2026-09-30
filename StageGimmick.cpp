#include "StageGimmick.h"

#include "MyMathUtility.h"
#include "MapChipField.h"
#include "Player.h"

#include <cmath>

using namespace KamataEngine;

void StageGimmick::Initialize(Model* model, Model* lightningModel, Camera* camera, MapChipField* mapChipField,
	bool* isOreSwitchActive, uint32_t* oreSwitchTimer, const Vector3& position, GimmickType type)
{
	model_ = model;
	lightningModel_ = lightningModel;
	camera_ = camera;
	mapChipField_ = mapChipField;
	isOreSwitchActive_ = isOreSwitchActive;
	oreSwitchTimer_ = oreSwitchTimer;
	type_ = type;
	startPosition_ = position;

	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	modelWorldTransform_.Initialize();
	lightningWorldTransform_.Initialize();
	lightningWorldTransform_.translation_ = position;
	if (type_ == GimmickType::kThunderCloud)
	{
		// 雷モデルは元データが縦に短く、原点も上端ではない。
		// 危険判定（雲の下から6マス）と同じ長さになるよう補正する。
		// 更新後の雷モデルの大きさに合わせ、見た目を横約1.6マス・縦約5.7マスへ調整する。
		lightningWorldTransform_.scale_ = {1.1f, 2.5f, 1.0f};
	}

	// 同じブロックモデルを利用しつつ、ギミックごとに大きさを変える。
	if (type_ == GimmickType::kFountainJump)
	{
		worldTransform_.scale_ = {1.0f, 0.25f, 1.0f};
	}
	else if (type_ == GimmickType::kWaterWheelLift)
	{
		worldTransform_.scale_ = {1.0f, 0.35f, 1.0f};
	}
	else if (type_ == GimmickType::kUpdraft)
	{
		worldTransform_.scale_ = {1.0f, 2.0f, 1.0f};
	}
	else if (type_ == GimmickType::kVine)
	{
		worldTransform_.scale_ = {0.45f, 4.0f, 1.0f};
	}
	else if (type_ == GimmickType::kOreSwitch)
	{
		worldTransform_.scale_ = {0.8f, 0.3f, 1.0f};
	}
	else if (type_ == GimmickType::kStoneBridge)
	{
		worldTransform_.scale_ = {2.0f, 0.5f, 1.0f};
	}
	else if (type_ == GimmickType::kVanishingCloud)
	{
		worldTransform_.scale_ = {1.5f, 0.5f, 1.0f};
	}

	if (type_ == GimmickType::kOreSwitch || type_ == GimmickType::kStoneBridge)
	{
		// CSVでは土台ブロックの1マス上にギミックを置く。
		// モデルと当たり判定の底面を、直下ブロックの上面へ揃える。
		worldTransform_.translation_.y -= 0.5f - worldTransform_.scale_.y * 0.5f;
		startPosition_ = worldTransform_.translation_;
	}

	// 基本は当たり判定用と同じ変換で描画する。
	modelWorldTransform_.scale_ = worldTransform_.scale_;

	if (type_ == GimmickType::kFountainJump)
	{
		// 噴水はモデル自体に高さがあるため、当たり判定の薄いYスケールを使わない。
		// CSVで1マス下に置かれた足場の上面へ、モデルの底を揃える。
		modelWorldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		modelOffset_.y = -0.268f;
	}
	else if (type_ == GimmickType::kVine)
	{
		// ツルモデルの原点は上端に近いので、見た目の上端とつかまる位置を合わせる。
		modelOffset_.y = worldTransform_.scale_.y * 0.5f + 0.03f;
	}

	UpdateMatrix();
}

void StageGimmick::Update()
{
	movementDelta_ = {};

	if (!isActive_)
	{
		return;
	}

	if (type_ == GimmickType::kMovingPlatform)
	{
		const float previousX = worldTransform_.translation_.x;
		worldTransform_.translation_.x = startPosition_.x + std::sin(static_cast<float>(timer_) * kMovingSpeed) * kMovingDistance;
		movementDelta_.x = worldTransform_.translation_.x - previousX;
	}
	else if (type_ == GimmickType::kWaterWheelLift)
	{
		const Vector3 previousPosition = worldTransform_.translation_;
		const float angle = static_cast<float>(timer_) * kWaterWheelSpeed;
		worldTransform_.translation_.x = startPosition_.x + std::cos(angle) * kWaterWheelRadius;
		worldTransform_.translation_.y = startPosition_.y + std::sin(angle) * kWaterWheelRadius;
		movementDelta_.x = worldTransform_.translation_.x - previousPosition.x;
		movementDelta_.y = worldTransform_.translation_.y - previousPosition.y;
	}

	if (type_ == GimmickType::kFallingRock)
	{
		// 一定時間待ってから落下する。ブロックへ到達したら開始位置へ戻る。
		if (timer_ < kFallingRockWaitFrames)
		{
			++timer_;
		}
		else
		{
			worldTransform_.translation_.y -= kFallingRockSpeed;

			const IndexSet indexSet = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_);
			if (mapChipField_->GetMapChipTypeByIndex(indexSet.xIndex, indexSet.yIndex) == MapChipType::kBlock)
			{
				worldTransform_.translation_ = startPosition_;
				timer_ = 0;
			}
		}

		UpdateMatrix();
		return;
	}

	if (type_ == GimmickType::kVine)
	{
		const Vector3 anchorPosition = {worldTransform_.translation_.x,
			worldTransform_.translation_.y + worldTransform_.scale_.y * 0.5f,
			worldTransform_.translation_.z};

		if (hangingPlayer_ && hangingPlayer_->IsHangingFromVine(anchorPosition))
		{
			// ツルモデルの原点（上端）を支点として、プレイヤーの振り角度へ同期する。
			modelRotationOffsetZ_ = hangingPlayer_->GetVineAngle();
		}
		else
		{
			hangingPlayer_ = nullptr;
			modelRotationOffsetZ_ = 0.0f;
		}
	}

	if (cooldown_ > 0)
	{
		--cooldown_;
	}

	if (type_ == GimmickType::kVanishingCloud)
	{
		if (isCloudHidden_)
		{
			if (++cloudTimer_ >= kCloudHiddenFrames)
			{
				// 復活後は、もう一度乗るまで消滅カウントを開始しない。
				isCloudHidden_ = false;
				isCloudTriggered_ = false;
				cloudTimer_ = 0;
			}
		}
		else if (isCloudTriggered_ && ++cloudTimer_ >= kCloudVisibleFrames)
		{
			isCloudHidden_ = true;
			cloudTimer_ = 0;
		}
	}
	++timer_;

	UpdateMatrix();
}

void StageGimmick::Draw()
{
	if (IsActive())
	{
		model_->Draw(modelWorldTransform_, *camera_);

		// 雷は危険判定が有効な落雷中だけ描画する。
		if (type_ == GimmickType::kThunderCloud && lightningModel_ && IsThunderStriking())
		{
			lightningModel_->Draw(lightningWorldTransform_, *camera_);
		}
	}
}

AABB StageGimmick::GetAABB() const
{
	const Vector3& position = worldTransform_.translation_;
	const Vector3& scale = worldTransform_.scale_;

	// 雷雲は雲の真下に落ちる雷柱を当たり判定にする。
	if (type_ == GimmickType::kThunderCloud)
	{
		return {{position.x - 0.35f, position.y - 6.0f, position.z - 0.5f},
			{position.x + 0.35f, position.y + 0.5f, position.z + 0.5f}};
	}

	AABB aabb;
	aabb.min = {position.x - 0.5f * scale.x, position.y - 0.5f * scale.y, position.z - 0.5f * scale.z};
	aabb.max = {position.x + 0.5f * scale.x, position.y + 0.5f * scale.y, position.z + 0.5f * scale.z};
	return aabb;
}

void StageGimmick::OnPlayerCollision(Player* player)
{
	if (!IsActive())
	{
		return;
	}

	switch (type_)
	{
	case GimmickType::kFountainJump:
		// 上面に触れると、短い間隔を空けて上へ打ち上げる。
		if (cooldown_ == 0)
		{
			player->LaunchFromFountain(0.42f);
			cooldown_ = kFountainCooldownFrames;
		}
		break;

	case GimmickType::kUpdraft:
		// 天空の上昇気流。範囲内では上向きの力を加える。
		player->AddExternalVelocity({0.0f, 0.025f, 0.0f});
		break;

	case GimmickType::kFallingRock:
		// 落石は攻撃では防げない危険物として扱う。
		player->OnHazardCollision();
		break;

	case GimmickType::kVine:
	{
		const Vector3 anchorPosition = {worldTransform_.translation_.x,
			worldTransform_.translation_.y + worldTransform_.scale_.y * 0.5f,
			worldTransform_.translation_.z};
		player->AttachToVine(anchorPosition);
		if (player->IsHangingFromVine(anchorPosition))
		{
			hangingPlayer_ = player;
		}
		break;
	}

	case GimmickType::kOreSwitch:
		// 攻撃でのみ押せるスイッチ。石橋の表示時間を更新する。
		if (player->IsAttack() && isOreSwitchActive_ && oreSwitchTimer_)
		{
			*isOreSwitchActive_ = true;
			*oreSwitchTimer_ = kOreSwitchActiveFrames;
		}
		break;

	case GimmickType::kThunderCloud:
		if (IsThunderStriking())
		{
			player->OnHazardCollision();
		}
		break;

	case GimmickType::kMovingPlatform:
	case GimmickType::kWaterWheelLift:
	case GimmickType::kBounceLeaf:
	case GimmickType::kStoneBridge:
	case GimmickType::kVanishingCloud:
	{
		const Player::GimmickHitSide hitSide = player->ResolveSolidGimmickCollision(
			GetAABB(), movementDelta_.x, movementDelta_.y);

		// 蓮の葉は、上面に乗った時だけ跳ねる
		if (type_ == GimmickType::kBounceLeaf && hitSide == Player::GimmickHitSide::kTop)
		{
			constexpr float kBounceVelocity = 0.38f;
			player->BounceOnGimmick(GetAABB().max.y, kBounceVelocity);
		}

		if (type_ == GimmickType::kVanishingCloud && hitSide == Player::GimmickHitSide::kTop)
		{
			isCloudTriggered_ = true;
		}
		break;
	}
	}
}

bool StageGimmick::IsSolid() const
{
	return type_ == GimmickType::kMovingPlatform ||
		type_ == GimmickType::kWaterWheelLift ||
		type_ == GimmickType::kBounceLeaf ||
		type_ == GimmickType::kStoneBridge ||
		type_ == GimmickType::kVanishingCloud;
}

bool StageGimmick::IsActive() const
{
	if (!isActive_)
	{
		return false;
	}

	if (type_ == GimmickType::kStoneBridge)
	{
		return isOreSwitchActive_ && *isOreSwitchActive_;
	}

	if (type_ == GimmickType::kVanishingCloud)
	{
		return !isCloudHidden_;
	}

	return true;
}

bool StageGimmick::IsThunderStriking() const
{
	const uint32_t cycle = kThunderWaitFrames + kThunderStrikeFrames;
	return timer_ % cycle >= kThunderWaitFrames;
}

void StageGimmick::UpdateMatrix()
{
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);
	worldTransform_.TransferMatrix();

	modelWorldTransform_.translation_ = {
		worldTransform_.translation_.x + modelOffset_.x,
		worldTransform_.translation_.y + modelOffset_.y,
		worldTransform_.translation_.z + modelOffset_.z};
	modelWorldTransform_.rotation_ = worldTransform_.rotation_;
	modelWorldTransform_.rotation_.z += modelRotationOffsetZ_;
	modelWorldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		modelWorldTransform_.scale_, modelWorldTransform_.rotation_, modelWorldTransform_.translation_);
	modelWorldTransform_.TransferMatrix();

	if (type_ == GimmickType::kThunderCloud)
	{
		// 元モデルの最上端を雲の下へ重ね、最下端を雷の当たり判定下端へ揃える。
		// Zを少し手前へ出して、雲と同一平面でのちらつきも防ぐ。
		lightningWorldTransform_.translation_ = {
			worldTransform_.translation_.x,
			worldTransform_.translation_.y + 0.22f,
			worldTransform_.translation_.z - 0.05f};
	}
	else
	{
		lightningWorldTransform_.translation_ = worldTransform_.translation_;
	}
	lightningWorldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		lightningWorldTransform_.scale_, lightningWorldTransform_.rotation_, lightningWorldTransform_.translation_);
	lightningWorldTransform_.TransferMatrix();
}
