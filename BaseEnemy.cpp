#define NOMINMAX
#include "EasingUtility.h"
#include "BaseEnemy.h"
#include "GameTime.h"
#include "GameScene.h"
#include "MapChipField.h"
#include "MyMathUtility.h"
#include "Player.h"
#include "SoundManager.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

void BaseEnemy::Initialize(Model* model, Camera* camera, const Vector3& position)
{
	// NULLポインタチェック
	assert(model);

	model_ = model;
	camera_ = camera;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();
	worldTransform_.translation_ = position;
	worldTransform_.rotation_.y = std::numbers::pi_v<float> / -2.0f;

	// 速度を設定する
	velocity_ = {-kWalkSpeed, 0.0f, 0.0f};

	walkTimer_ = 0.0f;

	// 初期化時にも行列を確定させておく
	UpdateMatrix();
}

void BaseEnemy::Update()
{
	if (behaviorRequest_ != Behavior::kUnknown) 
	{

		// 振る舞いを変更する
		behavior_ = behaviorRequest_;
		// 各振る舞いごとの初期化を実行
		switch (behavior_)
		{
		case Behavior::kWalk:
		default:
			// 歩行ビヘイビアの初期化
			BehaviorWalkInitialize();

			break;
		case Behavior::kDeath:
			// デス演出ビヘイビアの初期化
			BehaviorDeathInitialize();

			break;
		}

		// 振る舞いリクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;
	}

	switch (behavior_)
	{
	case Behavior::kWalk:
	default:
		// 通常行動更新
		BehaviorWalkUpdate();
		break;

	case Behavior::kDeath:
		// 攻撃行動更新
		BehaviorDeathUpdate();
		break;
	}
}

void BaseEnemy::UpdateMatrix()
{
	// アフィン変換行列の作成
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();
}

bool BaseEnemy::IsSolidPoint(float x, float y) const
{
	if (!mapChipField_)
	{
		return false;
	}
	const IndexSet index = mapChipField_->GetMapChipIndexSetByPosition({x, y, 0.0f});
	return mapChipField_->IsInBounds(index.xIndex, index.yIndex) &&
		mapChipField_->GetMapChipTypeByIndex(index.xIndex, index.yIndex) == MapChipType::kBlock;
}

bool BaseEnemy::IsBlockedAt(float x, float y) const
{
	if (!mapChipField_)
	{
		return false;
	}
	constexpr float kInside = 0.001f;
	const IndexSet topLeft = mapChipField_->GetMapChipIndexSetByPosition(
		{x - kWidth * 0.5f + kInside, y + kHeight * 0.5f - kInside, 0.0f});
	const IndexSet bottomRight = mapChipField_->GetMapChipIndexSetByPosition(
		{x + kWidth * 0.5f - kInside, y - kHeight * 0.5f + kInside, 0.0f});
	for (int32_t tileY = topLeft.yIndex; tileY <= bottomRight.yIndex; ++tileY)
	{
		for (int32_t tileX = topLeft.xIndex; tileX <= bottomRight.xIndex; ++tileX)
		{
			if (!mapChipField_->IsInBounds(tileX, tileY) ||
				mapChipField_->GetMapChipTypeByIndex(tileX, tileY) == MapChipType::kBlock)
			{
				return true;
			}
		}
	}
	return false;
}

BaseEnemy::TerrainHit BaseEnemy::MoveWithMapCollision(float moveX, float moveY)
{
	TerrainHit hit;
	if (!mapChipField_)
	{
		worldTransform_.translation_.x += moveX;
		worldTransform_.translation_.y += moveY;
		return hit;
	}

	// 1フレームの移動を小分けにし、可変FPSでも薄い壁を通り抜けないようにする。
	auto moveAxis = [this](float distance, bool horizontal) {
		if (distance == 0.0f)
		{
			return false;
		}
		constexpr float kMaxStep = 0.2f;
		const int steps = (std::max)(1, static_cast<int>(std::ceil(std::abs(distance) / kMaxStep)));
		const float step = distance / static_cast<float>(steps);
		for (int i = 0; i < steps; ++i)
		{
			const float currentX = worldTransform_.translation_.x;
			const float currentY = worldTransform_.translation_.y;
			const float nextX = currentX + (horizontal ? step : 0.0f);
			const float nextY = currentY + (horizontal ? 0.0f : step);
			if (!IsBlockedAt(nextX, nextY))
			{
				worldTransform_.translation_.x = nextX;
				worldTransform_.translation_.y = nextY;
				continue;
			}
			// 当たる直前まで進め、壁や床から不自然に離れた位置で止まらないようにする。
			float open = 0.0f;
			float blocked = 1.0f;
			for (int attempt = 0; attempt < 9; ++attempt)
			{
				const float midpoint = (open + blocked) * 0.5f;
				const float candidateX = currentX + (horizontal ? step * midpoint : 0.0f);
				const float candidateY = currentY + (horizontal ? 0.0f : step * midpoint);
				if (IsBlockedAt(candidateX, candidateY))
				{
					blocked = midpoint;
				}
				else
				{
					open = midpoint;
				}
			}
			worldTransform_.translation_.x += horizontal ? step * open : 0.0f;
			worldTransform_.translation_.y += horizontal ? 0.0f : step * open;
			return true;
		}
		return false;
	};

	hit.wall = moveAxis(moveX, true);
	const bool hitVertical = moveAxis(moveY, false);
	hit.floor = hitVertical && moveY < 0.0f;
	hit.ceiling = hitVertical && moveY > 0.0f;
	return hit;
}

void BaseEnemy::MoveGroundEnemy(bool moveHorizontally)
{
	const float deltaFrames = GameTime::GetDeltaFrames();
	const float footY = worldTransform_.translation_.y - kHeight * 0.5f;
	const bool hasGround = IsSolidPoint(worldTransform_.translation_.x, footY - 0.08f);
	if (moveHorizontally && hasGround && velocity_.x != 0.0f)
	{
		const float direction = velocity_.x > 0.0f ? 1.0f : -1.0f;
		const float aheadX = worldTransform_.translation_.x + direction * (kWidth * 0.5f + 0.12f);
		if (!IsSolidPoint(aheadX, footY - 0.08f))
		{
			velocity_.x = -velocity_.x;
		}
	}
	velocity_.y = (std::max)(-kMaxFallSpeed,
		velocity_.y - kGravityAcceleration * deltaFrames);
	const TerrainHit hit = MoveWithMapCollision(
		moveHorizontally ? velocity_.x * deltaFrames : 0.0f,
		velocity_.y * deltaFrames);
	if (hit.wall)
	{
		velocity_.x = -velocity_.x;
	}
	if (hit.floor || hit.ceiling)
	{
		velocity_.y = 0.0f;
	}
	worldTransform_.rotation_.y = velocity_.x >= 0.0f
		? std::numbers::pi_v<float> * 0.5f : -std::numbers::pi_v<float> * 0.5f;
}

// ワールド座標を取得
Vector3 BaseEnemy::GetWorldPosition()
{
	// ワールド座標を入れる変数
	Vector3 worldPos;
	// ワールド行列の平行移動成分を取得（ワールド座標）
	worldPos.x = worldTransform_.matWorld_.m[3][0];
	worldPos.y = worldTransform_.matWorld_.m[3][1];
	worldPos.z = worldTransform_.matWorld_.m[3][2];

	return worldPos;
}

// AABBを取得
AABB BaseEnemy::GetAABB()
{
	Vector3 worldPos = GetWorldPosition();

	AABB aabb;

	aabb.min = {worldPos.x - kWidth / 2.0f, worldPos.y - kHeight / 2.0f, worldPos.z - kWidth / 2.0f};
	aabb.max = {worldPos.x + kWidth / 2.0f, worldPos.y + kHeight / 2.0f, worldPos.z + kWidth / 2.0f};

	return aabb;
}

void BaseEnemy::OnCollision(Player* player)
{
	if (behavior_ == Behavior::kDeath)
	{
		// 敵がやられているなら何もしない
		return;
	}

	// プレイヤーが攻撃中なら敵が死ぬ
	if (player->IsAttack())
	{
		// 敵の振る舞いをデス演出に変更
		behaviorRequest_ = Behavior::kDeath;
		SoundManager::GetInstance()->PlaySE(SoundEffect::kEnemyDeath);

		// 攻撃成功：空中ジャンプ・空中攻撃を回復 
		player->OnAttackHit();


		// 敵と自キャラの座標をそれぞれ取得
		Vector3 enemyPos = GetWorldPosition();
		Vector3 playerPos = player->GetWorldPosition();

		// 中間位置を格納する変数を用意
		Vector3 effectPos;

		// X, Y, Z それぞれ個別に足して2で割る
		effectPos.x = (enemyPos.x + playerPos.x) / 2.0f;
		effectPos.y = (enemyPos.y + playerPos.y) / 2.0f;
		effectPos.z = (enemyPos.z + playerPos.z) / 2.0f;

		// 敵のモデルに埋もれないように、Z座標を手前（マイナス方向）にずらす
		effectPos.z -= 0.4f;

		// GameSceneのエフェクト生成関数を呼び出す
		gameScene_->CreateHitEffect(effectPos);
	}
}

// 歩行行動初期化
void BaseEnemy::BehaviorWalkInitialize() {}

// 歩行行動更新
void BaseEnemy::BehaviorWalkUpdate()
{
	MoveGroundEnemy();

	// タイマーを加算
	walkTimer_ += GameTime::GetDeltaSeconds();

	// 回転アニメーション
	// 周期 (kWalkMotionTime) に基づいて0 〜 2πの範囲をループするように計算
	float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer_ / kWalkMotionTime);
	// paramを0.0f 〜 1.0fに変換し、角度を計算
	float degree = kWalkMotionAngleStart + kWalkMotionAngleEnd * (param + 1.0f) / 2.0f;
	// 度をラジアンに変換 (度数 * π / 180)
	float angleAroundAxisX = degree * (std::numbers::pi_v<float> / 180.0f);
	// 計算した角度を X軸の回転に適用する
	worldTransform_.rotation_.x = angleAroundAxisX;

	// 行列更新
	UpdateMatrix();
}

// デス行動初期化
void BaseEnemy::BehaviorDeathInitialize() 
{
	// タイマーをリセット
	deathTimer_ = 0.0f;

	// コリジョン無効フラグを立てる
	isCollisionDisabled_ = true;
}

// デス行動更新
void BaseEnemy::BehaviorDeathUpdate()
{
	// アニメーションのタイマーを加算する
	deathTimer_ += GameTime::GetDeltaSeconds();

	// 進行度を 0.0f ～ 1.0f の範囲で計算
	float t = std::clamp(deathTimer_ / kDeathAnimationTime, 0.0f, 1.0f);

	// Y軸まわりの回転角をイージングで変化させる
	worldTransform_.rotation_.y = EasingUtility::EaseOut(std::numbers::pi_v<float> / -2.0f, std::numbers::pi_v<float> * 6.0f, t);

	// X軸まわりの回転角をイージングで変化させる
	worldTransform_.rotation_.x = EasingUtility::EaseOut(0.0f, std::numbers::pi_v<float> / -3.0f, t);

	// ワールドトランスフォームの行列更新
	UpdateMatrix();

	// if (アニメーションのタイマーが一定時間に達したら)
	if (deathTimer_ >= kDeathAnimationTime)
	{
		// デスフラグを立てる
		isDead_ = true;
	}
}

void BaseEnemy::Draw() 
{ 
	model_->Draw(worldTransform_, *camera_);
}
