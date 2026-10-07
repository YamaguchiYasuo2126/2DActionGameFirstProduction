#define NOMINMAX
#include "Player.h"
#include "EasingUtility.h"
#include "MapChipField.h"
#include "MyMathUtility.h"
#include "GamePadInput.h"
#include "SoundManager.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// Vector3演算子オーバーロード
Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Vector3{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
Vector3 operator-(const Vector3& v) { return {-v.x, -v.y, -v.z}; }
Vector3 operator+(const Vector3& v) { return v; }

void Player::Initialize(Model* model, Model* modelAttack, Camera* camera, const Vector3& position)
{

	// NULLポインタチェック
	assert(model);

	model_ = model;
	modelAttack_ = modelAttack;
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// エフェクトのトランスフォームの初期化
	worldTransformAttack_.Initialize();

	worldTransform_.translation_ = position;
	previousPosition_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 初期化時にも行列を確定させておく
	UpdateMatrix();
}

void Player::Update()
{
	// ギミックとの衝突面を判定するため、移動前の位置を保存する。
	previousPosition_ = worldTransform_.translation_;
	if (vineDetachCooldown_ > 0)
	{
		--vineDetachCooldown_;
	}
	if (damageInvincibleTimer_ > 0)
	{
		--damageInvincibleTimer_;
	}

	if (isHanging_)
	{
		UpdateHanging();
		return;
	}

	// 外部からのノックバックリクエストを処理
	if (isKnockbackRequested_) 
	{
		behaviorRequest_ = Behavior::kKnockback;
		isKnockbackRequested_ = false; // フラグをリセット
	}

	if (behaviorRequest_ != Behavior::kUnknown)
	{

		// 振る舞いを変更する
		behavior_ = behaviorRequest_;
		// 各振る舞いごとの初期化を実行
		switch (behavior_) 
		{
		case Behavior::kRoot:
		default:
			// ルートビヘイビアの初期化
			BehaviorRootInitialize();

			break;
		case Behavior::kAttack:
			// 攻撃ビヘイビアの初期化
			BehaviorAttackInitialize();

			break;
		case Behavior::kKnockback:
			// ノックバックビヘイビアの初期化
			BehaviorKnockbackInitialize();

			break;
		}

		// 振る舞いリクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;
	}

	switch (behavior_)
	{
	case Behavior::kRoot:
	default:
		// 通常行動更新
		BehaviorRootUpdate();
		break;

	case Behavior::kAttack:
		// 攻撃行動更新
		BehaviorAttackUpdate();
		break;

	case Behavior::kKnockback:
		// ノックバック行動開始
		BehaviorKnockbackUpdate();
		break;
	}
}

// 移動入力
void Player::MoveInput()
{
	// 衝突フレームの攻撃判定が終わった次フレームから通常操作へ戻す。
	if (isJumpRushing_ && (isJumpRushEnding_ || onGround_))
	{
		isJumpRushing_ = false;
		isJumpRushEnding_ = false;
		isAttackEffectVisible_ = false;
	}

	Input* input = Input::GetInstance();
	const bool isRightPushed = input->PushKey(DIK_RIGHT) || input->PushKey(DIK_D) || GamePadInput::IsRightPushed();
	const bool isLeftPushed = input->PushKey(DIK_LEFT) || input->PushKey(DIK_A) || GamePadInput::IsLeftPushed();
	const bool isJumpPushed = input->PushKey(DIK_SPACE) || GamePadInput::IsJumpPushed();
	const bool isJumpTriggered = input->TriggerKey(DIK_SPACE) || GamePadInput::IsJumpTriggered();

	auto beginTurn = [this](LRDirection direction) {
		if (lrDirection_ == direction)
		{
			return;
		}
		lrDirection_ = direction;
		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = kTimeTurn;
	};

	// 通常移動は矢印キーとA/Dのどちらでも行える。
	// チャージ中と突進中は方向入力を通常移動に使わない。
	if (!isRushAiming_ && !isJumpRushing_)
	{
		if (isRightPushed != isLeftPushed)
		{
			if (isRightPushed)
			{
				if (velocity_.x < 0.0f)
				{
					velocity_.x *= (1.0f - kAttenuation);
				}
				velocity_.x += kAcceleration;
				beginTurn(LRDirection::kRight);
			}
			else
			{
				if (velocity_.x > 0.0f)
				{
					velocity_.x *= (1.0f - kAttenuation);
				}
				velocity_.x -= kAcceleration;
				beginTurn(LRDirection::kLeft);
			}
			velocity_.x = std::clamp(velocity_.x, -kLimitRunSpeed, kLimitRunSpeed);
		}
		else
		{
			velocity_.x *= (1.0f - kAttenuation);
		}
	}

	if (onGround_ && velocity_.y > 0.0f)
	{
		onGround_ = false;
	}

	// Spaceを押した瞬間は真上を既定方向としてチャージを開始する。
	if (!isRushAiming_ && !isJumpRushing_ && (onGround_ || canAirJump_) && isJumpTriggered)
	{
		isRushAiming_ = true;
		rushAngle_ = std::numbers::pi_v<float> / 2.0f;
		velocity_.x = 0.0f;
	}

	if (isRushAiming_)
	{
		if (isJumpPushed)
		{
			

			// WASD（または矢印キー）の縦横各-1/0/1を合成し、8方向に固定する。
			int32_t directionX = 0;
			int32_t directionY = 0;
			const bool keyboardLeft = input->PushKey(DIK_A) || input->PushKey(DIK_LEFT);
			const bool keyboardRight = input->PushKey(DIK_D) || input->PushKey(DIK_RIGHT);
			const bool keyboardUp = input->PushKey(DIK_W) || input->PushKey(DIK_UP);
			const bool keyboardDown = input->PushKey(DIK_S) || input->PushKey(DIK_DOWN);
			const bool hasKeyboardDirection = keyboardLeft || keyboardRight || keyboardUp || keyboardDown;
			if (hasKeyboardDirection)
			{
				directionX = keyboardRight == keyboardLeft ? 0 : keyboardRight ? 1 : -1;
				directionY = keyboardUp == keyboardDown ? 0 : keyboardUp ? 1 : -1;
			}
			else
			{
				Vector2 stickDirection{};
				if (GamePadInput::GetLeftStickDirection(stickDirection))
				{
					directionX = stickDirection.x > 0.0f ? 1 : stickDirection.x < 0.0f ? -1 : 0;
					directionY = stickDirection.y > 0.0f ? 1 : stickDirection.y < 0.0f ? -1 : 0;
				}
			}

			if (directionX != 0 || directionY != 0)
			{
				rushAngle_ = std::atan2(static_cast<float>(directionY), static_cast<float>(directionX));
				if (directionX > 0)
				{
					beginTurn(LRDirection::kRight);
				}
				else if (directionX < 0)
				{
					beginTurn(LRDirection::kLeft);
				}
			}

			// 空中での再チャージ中はわずかに落下させる。
			if (!onGround_)
			{
				velocity_.y = -0.01f;
			}
		}
		else
		{
			

			float directionX = std::cos(rushAngle_);
			float directionY = std::sin(rushAngle_);
			// sin(pi) などの誤差で水平突進が傾かないよう、ほぼ0の成分を固定する。
			if (std::abs(directionX) < 0.001f)
			{
				directionX = 0.0f;
			}
			if (std::abs(directionY) < 0.001f)
			{
				directionY = 0.0f;
			}
			velocity_.x = directionX * kJumpRushSpeed;
			velocity_.y = directionY * kJumpRushSpeed;
			onGround_ = false;

			isRushAiming_ = false;
			isJumpRushing_ = true;
			isJumpRushEnding_ = false;
			isAttackEffectVisible_ = true;
			jumpRushFrame_ = 0;
			SoundManager::GetInstance()->PlaySE(SoundEffect::kJump);
		}
	}

	// 突進中は全方向の移動量を揃えるため重力を加えない。
	if (!onGround_ && !isRushAiming_ && !isJumpRushing_)
	{
		velocity_.y -= kGravityAcceleration;
		velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);
	}
}

void Player::AttachToVine(const Vector3& anchorPosition)
{
	if (isHanging_ || vineDetachCooldown_ > 0)
	{
		return;
	}

	vineAnchorPosition_ = anchorPosition;
	const Vector3 playerPosition = GetWorldPosition();
	vineAngle_ = std::atan2(playerPosition.x - vineAnchorPosition_.x,
		vineAnchorPosition_.y - playerPosition.y);
	vineAngle_ = std::clamp(vineAngle_, -1.2f, 1.2f);
	vineAngularVelocity_ = 0.0f;
	isHanging_ = true;
	isRushAiming_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	isAttackEffectVisible_ = false;
	onGround_ = false;
	velocity_ = {};
}

bool Player::IsHangingFromVine(const Vector3& anchorPosition) const
{
	if (!isHanging_)
	{
		return false;
	}

	constexpr float kAnchorEpsilon = 0.01f;
	return std::abs(vineAnchorPosition_.x - anchorPosition.x) < kAnchorEpsilon &&
		std::abs(vineAnchorPosition_.y - anchorPosition.y) < kAnchorEpsilon &&
		std::abs(vineAnchorPosition_.z - anchorPosition.z) < kAnchorEpsilon;
}

void Player::LaunchFromFountain(float launchVelocity)
{
	velocity_.y = launchVelocity;
	onGround_ = false;
	isRushAiming_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	isAttackEffectVisible_ = false;
}

void Player::UpdateHanging()
{
	const bool isLeftPushed = Input::GetInstance()->PushKey(DIK_LEFT) || Input::GetInstance()->PushKey(DIK_A) || GamePadInput::IsLeftPushed();
	const bool isRightPushed = Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_D) || GamePadInput::IsRightPushed();
	const bool isJumpTriggered = Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsJumpTriggered();

	if (isLeftPushed)
	{
		vineAngularVelocity_ -= kVineSwingAcceleration;
	}
	if (isRightPushed)
	{
		vineAngularVelocity_ += kVineSwingAcceleration;
	}

	// 振り子の重力と空気抵抗
	vineAngularVelocity_ -= std::sin(vineAngle_) * kVineGravity;
	vineAngularVelocity_ *= 0.99f;
	vineAngularVelocity_ = std::clamp(vineAngularVelocity_, -kVineMaxAngularVelocity, kVineMaxAngularVelocity);
	vineAngle_ += vineAngularVelocity_;
	vineAngle_ = std::clamp(vineAngle_, -1.2f, 1.2f);

	worldTransform_.translation_.x = vineAnchorPosition_.x + std::sin(vineAngle_) * kVineLength;
	worldTransform_.translation_.y = vineAnchorPosition_.y - std::cos(vineAngle_) * kVineLength;
	velocity_ = {};

	if (isJumpTriggered)
	{
		// 振り子の接線方向へ勢いを付けてツルから離れる。
		velocity_.x = std::cos(vineAngle_) * kVineLength * vineAngularVelocity_;
		velocity_.y = std::sin(vineAngle_) * kVineLength * vineAngularVelocity_;
		isHanging_ = false;
		vineDetachCooldown_ = 15;
		SoundManager::GetInstance()->PlaySE(SoundEffect::kJump);
	}

	UpdateMatrix();
}

// 指定した角の座標を得る関数
Vector3 Player::CornerPosition(const Vector3& center, Corner corner)
{

	Vector3 offsetTable[kNumCorner] = {
	    {+kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kRightBottom
	    {-kWidth / 2.0f, -kHeight / 2.0f, 0.0f}, // kLeftBottom
	    {+kWidth / 2.0f, +kHeight / 2.0f, 0.0f}, // kRightTop
	    {-kWidth / 2.0f, +kHeight / 2.0f, 0.0f}  // kLeftTop
	};

	return center + offsetTable[static_cast<uint32_t>(corner)];
}

// マップ衝突判定
void Player::CheckMapCollision(CollisionMapInfo& info)
{
	// 斜め移動は X 軸、Y 軸の順で解決する。各軸で移動経路を横切る
	// ブロック面を走査するため、角だけを調べる方式よりすり抜けに強い。
	const float halfWidth = kWidth / 2.0f;
	const float halfHeight = kHeight / 2.0f;
	const float epsilon = 0.0001f;
	const int32_t mapWidth = static_cast<int32_t>(mapChipField_->GetNumBlockHorizontal());
	const int32_t mapHeight = static_cast<int32_t>(mapChipField_->GetNumBlockVirtical());

	auto isBlock = [this](int32_t xIndex, int32_t yIndex) { return mapChipField_->GetMapChipTypeByIndex(xIndex, yIndex) == MapChipType::kBlock; };
	struct TileRange
	{
		int32_t minX;
		int32_t maxX;
		int32_t minY;
		int32_t maxY;
	};

	// 移動経路を囲む矩形だけをマップ座標へ変換する。
	// 1マス分広げることで、ちょうど境界上にいる場合も判定対象へ含める。
	auto makeTileRange = [this, mapWidth, mapHeight](float left, float right, float bottom, float top) {
		if (mapWidth <= 0 || mapHeight <= 0)
		{
			return TileRange{0, -1, 0, -1};
		}

		const IndexSet indices[] = {
			mapChipField_->GetMapChipIndexSetByPosition({left, bottom, 0.0f}),
			mapChipField_->GetMapChipIndexSetByPosition({left, top, 0.0f}),
			mapChipField_->GetMapChipIndexSetByPosition({right, bottom, 0.0f}),
			mapChipField_->GetMapChipIndexSetByPosition({right, top, 0.0f}),
		};

		int32_t minX = indices[0].xIndex;
		int32_t maxX = indices[0].xIndex;
		int32_t minY = indices[0].yIndex;
		int32_t maxY = indices[0].yIndex;
		for (const IndexSet& index : indices)
		{
			minX = (std::min)(minX, index.xIndex);
			maxX = (std::max)(maxX, index.xIndex);
			minY = (std::min)(minY, index.yIndex);
			maxY = (std::max)(maxY, index.yIndex);
		}

		return TileRange{
			std::clamp(minX - 1, 0, mapWidth - 1),
			std::clamp(maxX + 1, 0, mapWidth - 1),
			std::clamp(minY - 1, 0, mapHeight - 1),
			std::clamp(maxY + 1, 0, mapHeight - 1),
		};
	};

	// X 軸方向の衝突を先に解決する。
	if (info.move.x > 0.0f) 
	{
		const float currentRight = worldTransform_.translation_.x + halfWidth;
		const float targetRight = currentRight + info.move.x;
		const float playerBottom = worldTransform_.translation_.y - halfHeight;
		const float playerTop = worldTransform_.translation_.y + halfHeight;
		const TileRange range = makeTileRange(currentRight - halfWidth * 2.0f, targetRight, playerBottom, playerTop);

		for (int32_t y = range.minY; y <= range.maxY; ++y)
		{
			for (int32_t x = range.minX; x <= range.maxX; ++x)
			{
				if (!isBlock(x, y))
				{
					continue;
				}

				const MapChipField::Rect rect = mapChipField_->GetRectByIndex(x, y);
				const bool crossesLeftFace = currentRight <= rect.left + epsilon && targetRight > rect.left;
				const bool overlapsVertically = playerTop > rect.bottom + epsilon && playerBottom < rect.top - epsilon;
				if (crossesLeftFace && overlapsVertically) {
					info.move.x = std::min(info.move.x, rect.left - worldTransform_.translation_.x - (halfWidth + kBlank));
					info.hitWall = true;
				}
			}
		}
	} else if (info.move.x < 0.0f)
	{
		const float currentLeft = worldTransform_.translation_.x - halfWidth;
		const float targetLeft = currentLeft + info.move.x;
		const float playerBottom = worldTransform_.translation_.y - halfHeight;
		const float playerTop = worldTransform_.translation_.y + halfHeight;
		const TileRange range = makeTileRange(targetLeft, currentLeft + halfWidth * 2.0f, playerBottom, playerTop);

		for (int32_t y = range.minY; y <= range.maxY; ++y)
		{
			for (int32_t x = range.minX; x <= range.maxX; ++x)
			{
				if (!isBlock(x, y))
				{
					continue;
				}

				const MapChipField::Rect rect = mapChipField_->GetRectByIndex(x, y);
				const bool crossesRightFace = currentLeft >= rect.right - epsilon && targetLeft < rect.right;
				const bool overlapsVertically = playerTop > rect.bottom + epsilon && playerBottom < rect.top - epsilon;
				if (crossesRightFace && overlapsVertically) {
					info.move.x = (std::max)(info.move.x, rect.right - worldTransform_.translation_.x + (halfWidth + kBlank));
					info.hitWall = true;
				}
			}
		}
	}

	// X 軸の解決後の位置を用いて Y 軸方向の衝突を解決する。
	const float playerLeft = worldTransform_.translation_.x + info.move.x - halfWidth;
	const float playerRight = worldTransform_.translation_.x + info.move.x + halfWidth;
	if (info.move.y > 0.0f) {
		const float currentTop = worldTransform_.translation_.y + halfHeight;
		const float targetTop = currentTop + info.move.y;
		const TileRange range = makeTileRange(playerLeft, playerRight, currentTop - halfHeight * 2.0f, targetTop);

		for (int32_t y = range.minY; y <= range.maxY; ++y)
		{
			for (int32_t x = range.minX; x <= range.maxX; ++x)
			{
				if (!isBlock(x, y))
				{
					continue;
				}

				const MapChipField::Rect rect = mapChipField_->GetRectByIndex(x, y);
				const bool crossesBottomFace = currentTop <= rect.bottom + epsilon && targetTop > rect.bottom;
				const bool overlapsHorizontally = playerRight > rect.left + epsilon && playerLeft < rect.right - epsilon;
				if (crossesBottomFace && overlapsHorizontally)
				{
					info.move.y = std::min(info.move.y, rect.bottom - worldTransform_.translation_.y - (halfHeight + kBlank));
					info.isCeiling = true;
				}
			}
		}
	} else if (info.move.y < 0.0f)
	{
		const float currentBottom = worldTransform_.translation_.y - halfHeight;
		const float targetBottom = currentBottom + info.move.y;
		const TileRange range = makeTileRange(playerLeft, playerRight, targetBottom, currentBottom + halfHeight * 2.0f);

		for (int32_t y = range.minY; y <= range.maxY; ++y)
		{
			for (int32_t x = range.minX; x <= range.maxX; ++x)
			{
				if (!isBlock(x, y))
				{
					continue;
				}

				const MapChipField::Rect rect = mapChipField_->GetRectByIndex(x, y);
				const bool crossesTopFace = currentBottom >= rect.top - epsilon && targetBottom < rect.top;
				const bool overlapsHorizontally = playerRight > rect.left + epsilon && playerLeft < rect.right - epsilon;
				if (crossesTopFace && overlapsHorizontally)
				{
					info.move.y = (std::max)(info.move.y, rect.top - worldTransform_.translation_.y + (halfHeight + kBlank));
					info.isLanding = true;
				}
			}
		}
	}
}

// マップ衝突判定上方向
void Player::CheckMapCollisionUp(CollisionMapInfo& info)
{
	// 上昇あり？
	if (info.move.y <= 0.0f) 
	{
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, kNumCorner> positionNew;

	for (uint32_t i = 0; i < positionNew.size(); ++i) 
	{
		positionNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真上の当たり判定を行う
	bool hit = false;

	// 当たったブロックのインデックスを保存する変数
	IndexSet hitIndexSet{};

	float kMargin = 0.01f;

	// 左上点の判定
	IndexSet indexSetLeft = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftTop] + Vector3{+kMargin, 0.0f, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetLeft; // 左上で当たった情報を保存
	}

	// 右上点の判定
	IndexSet indexSetRight = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightTop] + Vector3{-kMargin, 0.0f, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex + 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetRight; // 右上で当たった情報を保存
	}

	// ブロックにヒット？
	if (hit) {
		// 移動前の自キャラ上端座標（中心の上）のインデックスを取得
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3{0.0f, +kHeight / 2.0f, 0.0f});

		// 移動前と移動後でY方向のセル番号が変わった（境界を跨いだ）場合のみ天井ヒットとする
		if (indexSetNow.yIndex != hitIndexSet.yIndex)
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndexSet.xIndex, hitIndexSet.yIndex);

			// めり込まない限界の移動量（Y方向）を計算
			float limitVelocityY = rect.bottom - worldTransform_.translation_.y - (kHeight / 2.0f + kBlank);

			// 元の移動量と限界の移動量を比較し、小さい方（より手前で止まる方）を採用する
			info.move.y = std::min(info.move.y, limitVelocityY);

			// 天井に当たったことを記録する
			info.isCeiling = true;
		}
	}
}

// マップ衝突判定下方向
void Player::CheckMapCollisionDown(CollisionMapInfo& info)
{
	// 落下していないならこれ以降の処理はしない
	if (info.move.y >= 0.0f) {
		return;
	}

	// 移動後の4つの角の座標
	std::array<Vector3, kNumCorner> positionNew;

	for (uint32_t i = 0; i < positionNew.size(); ++i)
	{
		positionNew[i] = CornerPosition(worldTransform_.translation_ + info.move, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	// 真上の当たり判定を行う
	bool hit = false;

	// ヒットしたブロックのインデックスを保存する変数を用意
	IndexSet hitIndexSet{};

	float kMargin = 0.01f;

	// 左下点の判定
	IndexSet indexSetLeft = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftBottom] + Vector3{+kMargin, 0.0f, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex - 1);
	// 隣接セルがともにブロックであればヒット
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock) 
	{
		hit = true;
		hitIndexSet = indexSetLeft; // 左下で当たった情報を保存
	}

	// 右下点の判定
	IndexSet indexSetRight = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightBottom] + Vector3{-kMargin, 0.0f, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex - 1);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetRight; // 右下で当たった情報を保存
	}

	// ブロックにヒット？
	if (hit)
	{
		// 移動前の自キャラ下端座標（中心の下）のインデックスを取得
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3{0.0f, -kHeight / 2.0f, 0.0f});

		// 移動前と移動後でY方向のセル番号が変わった（境界を跨いだ）場合のみ着地とする
		if (indexSetNow.yIndex != hitIndexSet.yIndex)
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndexSet.xIndex, hitIndexSet.yIndex);

			// めり込まない限界の移動量（Y方向）を計算
			// （ブロックの上端）-（プレイヤーの移動前のY座標）+（プレイヤーの半分の高さ）
			float limitVelocityY = rect.top - worldTransform_.translation_.y + (kHeight / 2.0f + kBlank);

			// 元の移動量と限界の移動量を比較し、大きい方（より手前で止まる方）を採用する
			info.move.y = (std::max)(info.move.y, limitVelocityY);
			// 着地したことを記録する
			info.isLanding = true;
		}
	}
}

// マップ衝突判定右方向
void Player::CheckMapCollisionRight(CollisionMapInfo& info)
{
	// 右移動あり？
	if (info.move.x <= 0.0f)
	{
		return;
	}

	std::array<Vector3, kNumCorner> positionNew;

	// Y方向の移動を0にして、X方向の移動量だけを取り出す
	Vector3 moveAmountX = {info.move.x, 0.0f, 0.0f};
	for (uint32_t i = 0; i < positionNew.size(); ++i)
	{
		positionNew[i] = CornerPosition(worldTransform_.translation_ + moveAmountX, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;
	IndexSet hitIndexSet = {};

	// 誤検知を防ぐためのわずかな隙間
	float kMargin = 0.01f;

	// 右上点の判定
	IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightTop] + Vector3{0.0f, -kMargin, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex - 1, indexSetTop.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetTop;
	}

	// 右下点の判定
	IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kRightBottom] + Vector3{0.0f, +kMargin, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex - 1, indexSetBottom.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetBottom; // 複数当たった場合は下が優先される
	}

	// ブロックにヒット？
	if (hit) {

		// 移動前の自キャラ右端座標（中心の右）のインデックスを取得
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3{+kWidth / 2.0f, 0.0f, 0.0f});

		// 移動前と移動後でX方向のセル番号が変わった（境界を跨いだ）場合のみ壁ヒットとする
		if (indexSetNow.xIndex != hitIndexSet.xIndex)
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndexSet.xIndex, hitIndexSet.yIndex);

			// めり込まない限界の移動量（X方向）を計算
			// （ブロックの左端） - （プレイヤーの現在のX座標） - （プレイヤーの半分の幅 + わずかな隙間）
			float limitVelocityX = rect.left - worldTransform_.translation_.x - (kWidth / 2.0f + kBlank);

			// 元の移動量と限界の移動量を比較し、小さい方（より手前で止まる方）を採用する
			info.move.x = std::min(info.move.x, limitVelocityX);

			// 壁に当たったフラグを立てる
			info.hitWall = true;
		}
	}
}

// マップ衝突判定左方向
void Player::CheckMapCollisionLeft(CollisionMapInfo& info) 
{
	// 左移動あり？（移動量が0以上ならチェックしない）
	if (info.move.x >= 0.0f)
	{
		return;
	}

	std::array<Vector3, kNumCorner> positionNew;
	Vector3 moveAmountX = {info.move.x, 0.0f, 0.0f};
	for (uint32_t i = 0; i < positionNew.size(); ++i)
	{
		positionNew[i] = CornerPosition(worldTransform_.translation_ + moveAmountX, static_cast<Corner>(i));
	}

	MapChipType mapChipType;
	MapChipType mapChipTypeNext;
	bool hit = false;
	IndexSet hitIndexSet = {};

	float kMargin = 0.01f;

	// 左上点の判定
	IndexSet indexSetTop = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftTop] + Vector3{0.0f, -kMargin, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex, indexSetTop.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetTop.xIndex + 1, indexSetTop.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetTop;
	}

	// 左下点の判定
	IndexSet indexSetBottom = mapChipField_->GetMapChipIndexSetByPosition(positionNew[kLeftBottom] + Vector3{0.0f, +kMargin, 0.0f});
	mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex, indexSetBottom.yIndex);
	mapChipTypeNext = mapChipField_->GetMapChipTypeByIndex(indexSetBottom.xIndex + 1, indexSetBottom.yIndex);
	if (mapChipType == MapChipType::kBlock && mapChipTypeNext != MapChipType::kBlock)
	{
		hit = true;
		hitIndexSet = indexSetBottom;
	}

	// ブロックにヒット？
	if (hit) {
		// 移動前の自キャラ左端座標（中心の左）のインデックスを取得
		IndexSet indexSetNow = mapChipField_->GetMapChipIndexSetByPosition(worldTransform_.translation_ + Vector3{-kWidth / 2.0f, 0.0f, 0.0f});

		// 移動前と移動後でX方向のセル番号が変わった（境界を跨いだ）場合のみ壁ヒットとする
		if (indexSetNow.xIndex != hitIndexSet.xIndex)
		{
			// めり込み先ブロックの範囲矩形
			MapChipField::Rect rect = mapChipField_->GetRectByIndex(hitIndexSet.xIndex, hitIndexSet.yIndex);

			// めり込まない限界の移動量（X方向）を計算
			// （ブロックの右端） - （プレイヤーの現在のX座標） + （プレイヤーの半分の幅 + わずかな隙間）
			float limitVelocityX = rect.right - worldTransform_.translation_.x + (kWidth / 2.0f + kBlank);

			// 元の移動量と限界の移動量を比較し、大きい方（より手前で止まる方）を採用する
			info.move.x = std::max(info.move.x, limitVelocityX);

			// 壁に当たったフラグを立てる
			info.hitWall = true;
		}
	}
}

// 判定結果を反映して移動させる
void Player::JudgmentResultReflectionMove(const CollisionMapInfo& info) 
{

	worldTransform_.translation_.x += info.move.x;
	worldTransform_.translation_.y += info.move.y;
}

// 天井に接触している場合の処理
void Player::ContactToCeiling(const CollisionMapInfo& info) 
{
	if (info.isCeiling) {
		velocity_.y = 0.0f;
	}
}

// 壁に接触している場合の処理
void Player::ContactToWall(const CollisionMapInfo& info)
{
	// 壁接触による減速
	if (info.hitWall) {
		velocity_.x *= (1.0f - kAttenuationWall);
	}
}

// 接地状態の切り替え処理
void Player::ContactToGroundStateSwitch(const CollisionMapInfo& info)
{
	// 接地している状態のとき（足場から外れて落下するかどうかのチェック）
	if (onGround_) {
		bool hit = false;
		MapChipType mapChipType;

		// 現在の座標を使って左下と右下の少し下を調べる
		std::array<Vector3, kNumCorner> positions;
		for (uint32_t i = 0; i < positions.size(); ++i) 
		{
			positions[i] = CornerPosition(worldTransform_.translation_, static_cast<Corner>(i));
		}

		// 左下点の少し下の判定
		IndexSet indexSetLeft = mapChipField_->GetMapChipIndexSetByPosition(positions[kLeftBottom] + Vector3{0.0f, -kShiftDown, 0.0f});
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetLeft.xIndex, indexSetLeft.yIndex);
		if (mapChipType == MapChipType::kBlock) 
		{
			hit = true;
		}

		// 右下点の少し下の判定
		IndexSet indexSetRight = mapChipField_->GetMapChipIndexSetByPosition(positions[kRightBottom] + Vector3{0.0f, -kShiftDown, 0.0f});
		mapChipType = mapChipField_->GetMapChipTypeByIndex(indexSetRight.xIndex, indexSetRight.yIndex);
		if (mapChipType == MapChipType::kBlock)
		{
			hit = true;
		}

		// 足元にブロックがなくなったら落下開始
		if (!hit) {
			onGround_ = false; // 空中状態に切り替える
		}
	}
	// 空中から落下してきて着地したとき
	else if (info.isLanding)
	{
		// 摩擦で横方向速度が減衰する
		velocity_.x *= (1.0f - kAttenuation);
		// 下方向速度をリセット
		velocity_.y = 0.0f;
		// 接地状態に移行
		onGround_ = true;
	}
}

void Player::UpdateMatrix()
{
	// アフィン変換行列の作成
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();

	// 攻撃エフェクトの行列計算と転送
	worldTransformAttack_.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransformAttack_.scale_, worldTransformAttack_.rotation_, worldTransformAttack_.translation_);
	worldTransformAttack_.TransferMatrix();
}

// ワールド座標を取得
Vector3 Player::GetWorldPosition()

{
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

void Player::LandOnGimmick(float topY, float horizontalMove)
{
	// 足場の移動量を座標へ一度だけ反映する。
	// 速度にも同じ値を入れると、次フレームに二重に運ばれてしまう。
	worldTransform_.translation_.x += horizontalMove;
	worldTransform_.translation_.y = topY + kHeight / 2.0f + kBlank;

	velocity_.y = 0.0f;
	onGround_ = true;

	UpdateMatrix();
}

void Player::BounceOnGimmick(float topY, float bounceVelocity) {
	// 足場の上へ正確に配置する
	worldTransform_.translation_.y = topY + kHeight / 2.0f + kBlank;

	// 上向きに跳ね返す
	velocity_.y = bounceVelocity;

	// 空中状態へ切り替える
	onGround_ = false;

	UpdateMatrix();
}

// AABBを取得
AABB Player::GetAABB()
{
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

AABB Player::GetPreviousAABB() const
{
	AABB aabb;
	aabb.min = {previousPosition_.x - kWidth / 2.0f, previousPosition_.y - kHeight / 2.0f, previousPosition_.z - kWidth / 2.0f};
	aabb.max = {previousPosition_.x + kWidth / 2.0f, previousPosition_.y + kHeight / 2.0f, previousPosition_.z + kWidth / 2.0f};
	return aabb;
}

Player::GimmickHitSide Player::ResolveSolidGimmickCollision(const AABB& gimmickAABB, float horizontalMove, float verticalMove)
{
	const AABB previousAABB = GetPreviousAABB();
	const AABB currentAABB = GetAABB();
	constexpr float kEpsilon = 0.01f;
	constexpr float kSupportEpsilon = 0.02f;

	const bool overlapsX = currentAABB.max.x > gimmickAABB.min.x && currentAABB.min.x < gimmickAABB.max.x;
	const bool overlapsY = currentAABB.max.y > gimmickAABB.min.y && currentAABB.min.y < gimmickAABB.max.y;

	// LandOnGimmick で作るわずかな隙間も、足場の上に立っている状態として扱う。
	// これがないと、非接触扱い→1フレーム落下→再着地を繰り返して振動する。
	const bool isStandingOnTop =
		overlapsX &&
		velocity_.y <= 0.0f &&
		currentAABB.min.y >= gimmickAABB.max.y - kEpsilon &&
		currentAABB.min.y <= gimmickAABB.max.y + kSupportEpsilon;
	if (isStandingOnTop)
	{
		LandOnGimmick(gimmickAABB.max.y, horizontalMove);
		return GimmickHitSide::kTop;
	}

	// 上下へ動く足場では、前フレームの上面に乗っていたかでも判定する。
	const float previousGimmickTop = gimmickAABB.max.y - verticalMove;
	const bool wasStandingOnMovingTop =
		overlapsX &&
		velocity_.y <= 0.0f &&
		previousAABB.min.y >= previousGimmickTop - kEpsilon &&
		previousAABB.min.y <= previousGimmickTop + kSupportEpsilon;
	if (wasStandingOnMovingTop)
	{
		LandOnGimmick(gimmickAABB.max.y, horizontalMove);
		return GimmickHitSide::kTop;
	}

	if (!overlapsX || !overlapsY)
	{
		return GimmickHitSide::kNone;
	}

	// 前フレームの位置から侵入した面を決める。これにより横・下からの接触で上面へワープしない。
	if (previousAABB.min.y >= gimmickAABB.max.y - kEpsilon && velocity_.y <= 0.0f)
	{
		LandOnGimmick(gimmickAABB.max.y, horizontalMove);
		return GimmickHitSide::kTop;
	}

	if (previousAABB.max.y <= gimmickAABB.min.y + kEpsilon && velocity_.y > 0.0f)
	{
		worldTransform_.translation_.y = gimmickAABB.min.y - kHeight / 2.0f - kBlank;
		velocity_.y = 0.0f;
		UpdateMatrix();
		return GimmickHitSide::kBottom;
	}

	if (previousAABB.max.x <= gimmickAABB.min.x + kEpsilon)
	{
		worldTransform_.translation_.x = gimmickAABB.min.x - kWidth / 2.0f - kBlank;
		velocity_.x = 0.0f;
		UpdateMatrix();
		return GimmickHitSide::kLeft;
	}

	if (previousAABB.min.x >= gimmickAABB.max.x - kEpsilon)
	{
		worldTransform_.translation_.x = gimmickAABB.max.x + kWidth / 2.0f + kBlank;
		velocity_.x = 0.0f;
		UpdateMatrix();
		return GimmickHitSide::kRight;
	}

	// 足場が横移動してプレイヤーへめり込んだ場合も、最も浅い方向へ押し戻す。
	const float overlapLeft = currentAABB.max.x - gimmickAABB.min.x;
	const float overlapRight = gimmickAABB.max.x - currentAABB.min.x;
	const float overlapBottom = currentAABB.max.y - gimmickAABB.min.y;
	const float overlapTop = gimmickAABB.max.y - currentAABB.min.y;
	const float overlapX = (std::min)(overlapLeft, overlapRight);
	const float overlapY = (std::min)(overlapBottom, overlapTop);

	if (overlapX < overlapY)
	{
		if (overlapLeft < overlapRight)
		{
			worldTransform_.translation_.x = gimmickAABB.min.x - kWidth / 2.0f - kBlank;
			velocity_.x = 0.0f;
			UpdateMatrix();
			return GimmickHitSide::kLeft;
		}

		worldTransform_.translation_.x = gimmickAABB.max.x + kWidth / 2.0f + kBlank;
		velocity_.x = 0.0f;
		UpdateMatrix();
		return GimmickHitSide::kRight;
	}

	if (overlapTop < overlapBottom)
	{
		LandOnGimmick(gimmickAABB.max.y, horizontalMove);
		return GimmickHitSide::kTop;
	}

	worldTransform_.translation_.y = gimmickAABB.min.y - kHeight / 2.0f - kBlank;
	velocity_.y = 0.0f;
	UpdateMatrix();
	return GimmickHitSide::kBottom;
}

// 攻撃用のAABBを取得
AABB Player::GetAttackAABB()
{
	Vector3 attackCenter = GetWorldPosition();
	float attackWidth = kRushAttackSize;
	float attackHeight = kRushAttackSize;
	if (!isJumpRushing_)
	{
		// ゲームパッドXで残している従来攻撃は、従来どおり上方向の判定を使う。
		attackCenter = attackCenter + Vector3{0.0f, kAttackHitOffsetY, 0.0f};
		attackWidth = kAttackHitWidth;
		attackHeight = kAttackHitHeight;
	}

	AABB aabb;
	aabb.min = {attackCenter.x - attackWidth / 2.0f, attackCenter.y - attackHeight / 2.0f, attackCenter.z - attackWidth / 2.0f};
	aabb.max = {attackCenter.x + attackWidth / 2.0f, attackCenter.y + attackHeight / 2.0f, attackCenter.z + attackWidth / 2.0f};

	return aabb;
}

void Player::OnCollision(const BaseEnemy* enemy)
{
	(void)enemy;

	if (IsAttack())
	{
		return; // 攻撃中はダメージ無効
	}

	TakeDamage(1);
}

void Player::OnHazardCollision()
{
	TakeDamage(1);
}

void Player::TakeDamage(int32_t damage)
{
	if (damageInvincibleTimer_ > 0 || isDead_)
	{
		return;
	}

	hp_ = (std::max)(0, hp_ - damage);
	damageInvincibleTimer_ = kDamageInvincibleFrames;
	if (hp_ == 0)
	{
		isDead_ = true;
	}
	else
	{
		// 残りHPがある間は、ゲームシーンにチェックポイント復活を依頼する。
		isRespawnRequested_ = true;
	}
}

bool Player::ConsumeRespawnRequest()
{
	const bool requested = isRespawnRequested_;
	isRespawnRequested_ = false;
	return requested;
}

void Player::Respawn(const Vector3& position, bool restoreHp)
{
	worldTransform_.translation_ = position;
	previousPosition_ = position;
	velocity_ = {};
	if (restoreHp)
	{
		hp_ = kMaxHp;
	}
	isDead_ = false;
	isRespawnRequested_ = false;
	damageInvincibleTimer_ = kDamageInvincibleFrames;
	onGround_ = false;
	isHanging_ = false;
	vineDetachCooldown_ = 0;
	isRushAiming_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	jumpRushFrame_ = 0;
	behavior_ = Behavior::kRoot;
	behaviorRequest_ = Behavior::kUnknown;
	isAttackEffectVisible_ = false;
	UpdateMatrix();
}

// 通常行動初期化
void Player::BehaviorRootInitialize() { isAttackEffectVisible_ = false; }

// 通常行動更新
void Player::BehaviorRootUpdate()
{

	if (onGround_) {
		isAttackedInAir_ = false;
		canAirJump_ = false;
	}

	// Spaceは突進ジャンプ専用。ゲームパッドXの従来攻撃だけ互換操作として残す。
	if (!isRushAiming_ && !isJumpRushing_ && GamePadInput::IsAttackTriggered())
	{
		if (!onGround_ && isAttackedInAir_)
		{

		} else {
			// 攻撃ビヘイビアをリクエスト
			behaviorRequest_ = Behavior::kAttack;
			// もし空中で攻撃したならフラグをtrueにする
			if (!onGround_) {
				isAttackedInAir_ = true;
			}
		}
	}

	// 移動入力の処理を呼び出す
	MoveInput();

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.move = velocity_;

	// マップ衝突チェック
	CheckMapCollision(collisionMapInfo);

	// 判定結果を反映して移動させる
	JudgmentResultReflectionMove(collisionMapInfo);

	// 天井に接触している場合の処理
	ContactToCeiling(collisionMapInfo);

	// 壁に接触している場合の処理
	ContactToWall(collisionMapInfo);

	// 接地状態の切り替え
	ContactToGroundStateSwitch(collisionMapInfo);

	if (isJumpRushing_)
	{
		++jumpRushFrame_;
		const bool rushFinished = jumpRushFrame_ >= kJumpRushFrame;
		const bool hitSurface = collisionMapInfo.hitWall || collisionMapInfo.isCeiling || collisionMapInfo.isLanding;
		if (rushFinished || hitSurface)
		{
			// 突進速度を残さず、次フレームから移動入力を反映できるようにする。
			velocity_ = {};
			// GameSceneの敵判定がこの後に走るため、衝突フレーム中は攻撃を有効に保つ。
			isJumpRushEnding_ = true;
		}

		// 突進中の攻撃表示はプレイヤー中心に置く。
		worldTransformAttack_.translation_ = worldTransform_.translation_;
		worldTransformAttack_.rotation_ = {};
		worldTransformAttack_.rotation_.y = std::numbers::pi_v<float> / 2.0f;
		worldTransformAttack_.scale_ = {kRushAttackSize, kRushAttackSize, kRushAttackSize};
	}

	// 旋回制御
	if (turnTimer_ > 0.0f)
	{
		turnTimer_ -= 1.0f / 60.0f;

		// タイマーが0未満にならないようにクランプ
		if (turnTimer_ < 0.0f)
		{
			turnTimer_ = 0.0f;
		}

		// 左右の自キャラ角度テーブル
		float destinationRotationYTable[] = {std::numbers::pi_v<float> / 2.0f, std::numbers::pi_v<float> * 3.0f / 2.0f};

		// 状態に応じた角度を取得する
		float destinationRotationY = destinationRotationYTable[static_cast<uint32_t>(lrDirection_)];

		// イージングに渡すための時間割合 t(0.0f ～ 1.0f)を計算する
		float t = 1.0f - (turnTimer_ / kTimeTurn);
		t = std::clamp(t, 0.0f, 1.0f);

		// 自キャラの角度を設定する
		worldTransform_.rotation_.y = EasingUtility::EaseInOut(turnFirstRotationY_, destinationRotationY, t);
	}

	// 行列更新
	UpdateMatrix();
}

// 攻撃行動初期化
void Player::BehaviorAttackInitialize() 
{
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	worldTransformAttack_.scale_ = {1.0f, 1.0f, 1.0f};
	// カウンター初期化
	attackParameter_ = 0.0f;

	// フェーズを溜めからスタートさせる
	attackPhase_ = AttackPhase::kCharge;

	velocity_ = {};

	isAttackEffectVisible_ = false;
	hasHitEnemyDuringAttack_ = false;
	SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerAttack);
}

// 攻撃行動更新
void Player::BehaviorAttackUpdate()
{
	attackParameter_++;

	// 攻撃動作用の速度
	Vector3 velocity{};

	switch (attackPhase_) 
	{

		// 溜め動作
	case AttackPhase::kCharge:
	default:
	{
		isAttackEffectVisible_ = false;
		float t = static_cast<float>(attackParameter_) / kAttackChargeTimer;
		worldTransform_.scale_.z = EasingUtility::EaseOut(1.0f, 0.3f, t);
		worldTransform_.scale_.y = EasingUtility::EaseOut(1.0f, 1.6f, t);

		velocity.x = 0.0f;

		// 前進動作へ移行
		if (attackParameter_ >= kAttackChargeTimer) 
		{
			attackPhase_ = AttackPhase::kRush;
			attackParameter_ = 0.0f; // カウンターをリセット
		}
		break;
	}

		// 突進動作
	case AttackPhase::kRush: 
	{
		// 突進と余韻中はエフェクトを表示
		isAttackEffectVisible_ = true;

		float t = static_cast<float>(attackParameter_) / kRushTimer;
		worldTransform_.scale_.z = EasingUtility::EaseOut(0.3f, 1.3f, t);
		worldTransform_.scale_.y = EasingUtility::EaseOut(1.6f, 0.7f, t);

		// 横移動はしない
		velocity.x = 0.0f;

		// 上方向へ少し浮きながら攻撃する
		velocity.y = kAttackRiseSpeed;

		// 指定時間が経過したら余韻動作へ移行
		if (attackParameter_ >= kRushTimer)
		{
			attackPhase_ = AttackPhase::kReverberation;
			attackParameter_ = 0.0f; // カウンターをリセット
		}

		break;
	}
		// 余韻動作
	case AttackPhase::kReverberation:
	{
		// 突進と余韻中はエフェクトを表示
		isAttackEffectVisible_ = true;
		float t = static_cast<float>(attackParameter_) / kReverberationTimer;
		worldTransform_.scale_.z = EasingUtility::EaseOut(1.3f, 1.0f, t);
		worldTransform_.scale_.y = EasingUtility::EaseOut(0.7f, 1.0f, t);

		velocity.x = 0.0f;

		// 余韻が終わったら通常状態(Root)に戻す
		if (attackParameter_ >= kReverberationTimer)
		{
			behaviorRequest_ = Behavior::kRoot;
			attackParameter_ = 0.0f;
		}

		break;
	}
	}

	// 既存のマップ衝突判定と移動処理を走らせる
	CollisionMapInfo collisionMapInfo;
	if (velocity.y > 0.0f)
	{
		onGround_ = false;
	}
	collisionMapInfo.move = velocity;

	// マップ衝突チェック
	CheckMapCollision(collisionMapInfo);

	// 判定結果を反映して移動させる
	JudgmentResultReflectionMove(collisionMapInfo);

	// 各種接触・接地状態の切り替え処理
	ContactToCeiling(collisionMapInfo);
	ContactToWall(collisionMapInfo);
	ContactToGroundStateSwitch(collisionMapInfo);

	// 敵にヒットしていない攻撃だけ、空中攻撃を消費した状態にする。
	// ヒット後に isAttackedInAir_ を再び true に戻さないことが重要。
	if (!onGround_ && !hasHitEnemyDuringAttack_) {
		isAttackedInAir_ = true;
	}

	// 移動後のプレイヤー座標に合わせて、攻撃エフェクトを上側へ配置する
	worldTransformAttack_.translation_ = worldTransform_.translation_ + Vector3{0.0f, kAttackHitOffsetY, 0.0f};
	// 攻撃エフェクトは正面向きの平面モデルなので、プレイヤーの左右反転を引き継がない。
	// 左向き時に裏面の法線がライティングされて黒くなることを防ぐ。
	worldTransformAttack_.rotation_ = {};
	worldTransformAttack_.rotation_.y = std::numbers::pi_v<float> / 2.0f;

	// 行列更新
	UpdateMatrix();
}

void Player::OnAttackHit() {
	// 空中攻撃を再び可能にする
	isAttackedInAir_ = false;
	hasHitEnemyDuringAttack_ = true;

	// 空中ジャンプを1回だけ可能にする
	canAirJump_ = true;

	// 攻撃アニメーションは中断せず、kReverberation の終了まで継続する。
	// 通常状態への遷移は BehaviorAttackUpdate() 側に任せる。
}

// ノックバック行動初期化
void Player::BehaviorKnockbackInitialize()
{
	knockbackParameter_ = 0.0f;
	isRushAiming_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	// 攻撃エフェクトを消す
	isAttackEffectVisible_ = false;

	// 攻撃中に中断されたときのためにスケールをデフォルトに戻す
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// 後ろに弾き飛ばす初速を設定
	if (lrDirection_ == LRDirection::kRight) {
		velocity_.x = -0.6f; // 右向きなら左へ
	} else {
		velocity_.x = 0.6f; // 左向きなら右へ
	}
}

// ノックバック行動更新
void Player::BehaviorKnockbackUpdate()
{
	knockbackParameter_++;

	// 強い初速で弾き飛ばされるフェーズ
	if (knockbackParameter_ < kKnockbackBlowTime)
	{
		// 空気抵抗などで徐々に減速させる
		velocity_.x *= 0.9f;
	} else if (knockbackParameter_ < kKnockbackBlowTime + kKnockbackRecoverTime) {
		// 移動が停止し、体勢を立て直すフェーズ
		velocity_.x = 0.0f; // 完全に停止して硬直
	}

	else {
		// 終了して通常状態へ
		behaviorRequest_ = Behavior::kRoot;
	}

	// 共通の移動・衝突判定処理
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = velocity_;
	CheckMapCollision(collisionMapInfo);
	JudgmentResultReflectionMove(collisionMapInfo);
	ContactToCeiling(collisionMapInfo);
	ContactToWall(collisionMapInfo);
	ContactToGroundStateSwitch(collisionMapInfo);

	UpdateMatrix();
}

void Player::Draw() 
{
	model_->Draw(worldTransform_, *camera_);
	// 攻撃エフェクトの描画（フラグがtrueの時のみ）
	if (modelAttack_ != nullptr) {
		if (isAttackEffectVisible_ && modelAttack_)
		{
			// camera_ はポインタなので、*camera_ にして実体として渡す
			modelAttack_->Draw(worldTransformAttack_, *camera_);
		}
	}
}
