#define NOMINMAX
#include "Player.h"
#include "BaseEnemy.h"
#include "EasingUtility.h"
#include "MapChipField.h"
#include "MyMathUtility.h"
#include "GamePadInput.h"
#include "SoundManager.h"
#include "GameTime.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <numbers>

using namespace KamataEngine;

// Vector3演算子オーバーロード
Vector3 operator+(const Vector3& v1, const Vector3& v2) { return Vector3{v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
Vector3 operator-(const Vector3& v) { return {-v.x, -v.y, -v.z}; }
Vector3 operator+(const Vector3& v) { return v; }

namespace {
constexpr float kRushAfterimageSeconds = 0.18f;
constexpr float kRushGrainSeconds = 0.20f;
constexpr float kRushVisualStepSeconds = 0.028f;

void TransferEffectTransform(WorldTransform& transform)
{
	transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
		transform.scale_, transform.rotation_, transform.translation_);
	transform.TransferMatrix();
}
}

void Player::Initialize(Model* model, Model* rushBoxEffectModel, Model* rushBoardEffectModel, Camera* camera,
	const Vector3& position, Model* modelFlash)
{

	// NULLポインタチェック
	assert(model);

	model_ = model;
	modelFlash_ = modelFlash;
	rushBoxEffectModel_ = rushBoxEffectModel;
	rushBoardEffectModel_ = rushBoardEffectModel;
	camera_ = camera;
	if (modelFlash_)
	{
		whiteTextureHandle_ = TextureManager::Load("white1x1.png");
	}
	damageVisualColor_.Initialize();
	rushBoxColor_.Initialize();
	rushBoxColor_.SetColor({0.25f, 0.90f, 1.0f, 0.85f});
	rushBoardColor_.Initialize();
	rushBoardColor_.SetColor({0.9f, 1.0f, 1.0f, 0.8f});

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	for (auto& grain : rushGrains_)
	{
		grain.transform.Initialize();
	}
	for (auto& afterimage : rushAfterimages_)
	{
		afterimage.transform.Initialize();
		afterimage.color.Initialize();
	}
	ClearRushVisuals();

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
	UpdateRushVisuals(GameTime::GetDeltaSeconds());
	detachedFromTraceThisFrame_ = false;
	if (vineDetachCooldown_ > 0)
	{
		vineDetachCooldown_ = (std::max)(0.0f, vineDetachCooldown_ - GameTime::GetDeltaFrames());
	}

	if (damageInvincibleTimer_ > 0)
	{
		damageInvincibleTimer_ = (std::max)(0.0f, damageInvincibleTimer_ - GameTime::GetDeltaFrames());
		if (damageInvincibleTimer_ == 0.0f)
		{
			damageFlashActive_ = false;
		}
	}
	if (damageRedTimer_ > 0)
	{
		damageRedTimer_ = (std::max)(0.0f, damageRedTimer_ - GameTime::GetDeltaFrames());
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

	case Behavior::kKnockback:
		// ノックバック行動開始
		BehaviorKnockbackUpdate();
		break;
	}
}

void Player::ClearRushVisuals()
{
	rushVisualIntervalSeconds_ = 0.0f;
	nextRushGrain_ = 0;
	nextRushAfterimage_ = 0;
	for (auto& grain : rushGrains_)
	{
		grain.remainingSeconds = 0.0f;
	}
	for (auto& afterimage : rushAfterimages_)
	{
		afterimage.remainingSeconds = 0.0f;
	}
}

void Player::UpdateRushVisuals(float deltaSeconds)
{
	for (auto& grain : rushGrains_)
	{
		if (grain.remainingSeconds <= 0.0f)
		{
			continue;
		}
		grain.remainingSeconds = (std::max)(0.0f, grain.remainingSeconds - deltaSeconds);
		if (grain.remainingSeconds <= 0.0f)
		{
			continue;
		}
		grain.transform.translation_.x += grain.velocity.x * deltaSeconds;
		grain.transform.translation_.y += grain.velocity.y * deltaSeconds;
		const float fade = grain.remainingSeconds / grain.durationSeconds;
		grain.transform.scale_ = grain.board
			? Vector3{grain.size * fade, grain.size * 0.32f * fade, 0.025f}
			: Vector3{grain.size * fade, grain.size * fade, grain.size * fade};
		TransferEffectTransform(grain.transform);
	}
	for (auto& afterimage : rushAfterimages_)
	{
		if (afterimage.remainingSeconds <= 0.0f)
		{
			continue;
		}
		afterimage.remainingSeconds = (std::max)(0.0f, afterimage.remainingSeconds - deltaSeconds);
		const float fade = afterimage.remainingSeconds / kRushAfterimageSeconds;
		afterimage.color.SetColor({0.18f, 0.9f, 1.0f, 0.42f * fade});
	}
}

void Player::SpawnRushGrain(bool board, float spread)
{
	auto& grain = rushGrains_[nextRushGrain_];
	nextRushGrain_ = (nextRushGrain_ + 1) % rushGrains_.size();
	grain.board = board;
	grain.durationSeconds = kRushGrainSeconds;
	grain.remainingSeconds = kRushGrainSeconds;
	grain.size = board ? 0.18f : 0.10f;
	const float sideX = -rushDirection_.y;
	const float sideY = rushDirection_.x;
	grain.transform.translation_ = {
		worldTransform_.translation_.x - rushDirection_.x * 0.32f + sideX * spread * 0.17f,
		worldTransform_.translation_.y - rushDirection_.y * 0.32f + sideY * spread * 0.17f,
		worldTransform_.translation_.z - 0.10f};
	grain.transform.rotation_ = {0.0f, 0.0f, std::atan2(rushDirection_.y, rushDirection_.x)};
	grain.velocity = {-rushDirection_.x * 1.8f + sideX * spread * 1.1f,
		-rushDirection_.y * 1.8f + sideY * spread * 1.1f, 0.0f};
	grain.transform.scale_ = board
		? Vector3{grain.size, grain.size * 0.32f, 0.025f}
		: Vector3{grain.size, grain.size, grain.size};
	TransferEffectTransform(grain.transform);
}

void Player::CaptureRushAfterimage()
{
	if (!modelFlash_)
	{
		return;
	}
	auto& afterimage = rushAfterimages_[nextRushAfterimage_];
	nextRushAfterimage_ = (nextRushAfterimage_ + 1) % rushAfterimages_.size();
	afterimage.remainingSeconds = kRushAfterimageSeconds;
	afterimage.transform.translation_ = worldTransform_.translation_;
	// 本体より少し奥に置き、本体の手前を残像が覆わないようにする。
	afterimage.transform.translation_.z += 0.10f;
	afterimage.transform.rotation_ = worldTransform_.rotation_;
	afterimage.transform.scale_ = worldTransform_.scale_;
	afterimage.color.SetColor({0.18f, 0.9f, 1.0f, 0.42f});
	TransferEffectTransform(afterimage.transform);
}

void Player::EmitRushVisuals()
{
	rushVisualIntervalSeconds_ -= GameTime::GetDeltaSeconds();
	// 大きなフレーム間隔でも粒の生成数を制限し、残像プールを使い切らない。
	for (int emitted = 0; rushVisualIntervalSeconds_ <= 0.0f && emitted < 3; ++emitted)
	{
		CaptureRushAfterimage();
		SpawnRushGrain(false, emitted % 2 == 0 ? -1.0f : 1.0f);
		SpawnRushGrain(true, emitted % 2 == 0 ? 1.0f : -1.0f);
		rushVisualIntervalSeconds_ += kRushVisualStepSeconds;
	}
	if (rushVisualIntervalSeconds_ < 0.0f)
	{
		rushVisualIntervalSeconds_ = 0.0f;
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
		traceRushActive_ = false;
	}

	Input* input = Input::GetInstance();
	const bool isRightPushed = input->PushKey(DIK_RIGHT) || input->PushKey(DIK_D) || GamePadInput::IsRightPushed();
	const bool isLeftPushed = input->PushKey(DIK_LEFT) || input->PushKey(DIK_A) || GamePadInput::IsLeftPushed();
	const bool isRushPushed = input->PushKey(DIK_SPACE) || GamePadInput::IsRushPushed();
	const bool isRushTriggered = input->TriggerKey(DIK_SPACE) || GamePadInput::IsRushTriggered();
	const bool isNormalJumpTriggered = input->TriggerKey(DIK_W) || input->TriggerKey(DIK_UP) ||
		GamePadInput::IsNormalJumpTriggered();
	const bool detachFromTrace = (traceAimActive_ || traceRushActive_) &&
		(input->TriggerKey(DIK_E) || GamePadInput::IsGrabTriggered());
	if (detachFromTrace)
	{
		DetachFromTraceVertex();
	}
	else if (traceAimActive_)
	{
		// 一辺をなぞった後も次のSpace入力まで頂点に吸い付いたまま待つ。
		worldTransform_.translation_ = traceAimAnchor_;
		velocity_ = {};
		onGround_ = false;
	}
	const float deltaFrames = GameTime::GetDeltaFrames();
	const float moveDamping = GameTime::GetDampingFactor(1.0f - kAttenuation);

	auto beginTurn = [this](LRDirection direction)
		{
		if (lrDirection_ == direction)
		{
			return;
		}
		lrDirection_ = direction;
		turnFirstRotationY_ = worldTransform_.rotation_.y;
		turnTimer_ = kTimeTurn;
	};


	// 方向選択と突進中は方向入力を通常移動に使わない。
	if (!isRushAiming_ && !isJumpRushing_ && !isRushTriggered && !traceAimActive_)
	{
		if (isRightPushed != isLeftPushed)
		{
			const float momentumDamping = GameTime::GetDampingFactor(kRushMomentumDamping);

			if (isRightPushed)
			{
				if (velocity_.x > kLimitRunSpeed)
				{
					// 右向きの超過速度は急停止させず、徐々に通常速度へ戻す。
					velocity_.x *= momentumDamping;
				}
				else
				{
					// 左向きの速度が残っている場合は先に減速する。
					if (velocity_.x < 0.0f) {
						velocity_.x *= moveDamping;
					}

					velocity_.x += kAcceleration * deltaFrames;
					velocity_.x = (std::min)(velocity_.x, kLimitRunSpeed);
				}

				beginTurn(LRDirection::kRight);
			} 
			else
			{
				if (velocity_.x < -kLimitRunSpeed)
				{
					// 左向きの超過速度を徐々に通常速度へ戻す。
					velocity_.x *= momentumDamping;
				}
				else
				{
					// 右向きの速度が残っている場合は先に減速する。
					if (velocity_.x > 0.0f)
					{
						velocity_.x *= moveDamping;
					}

					velocity_.x -= kAcceleration * deltaFrames;
					velocity_.x = (std::max)(velocity_.x, -kLimitRunSpeed);
				}

				beginTurn(LRDirection::kLeft);
			}
		} 
		else 
		{
			velocity_.x *= moveDamping;
		}
	}

	if (onGround_ && velocity_.y > 0.0f)
	{
		onGround_ = false;
	}

	// W/↑またはゲームパッドXは、方向選択のない即時ジャンプとする。
	if (!isRushPushed && !isRushAiming_ && !isJumpRushing_ && onGround_ && isNormalJumpTriggered)
	{
		velocity_.y = kNormalJumpSpeed;
		onGround_ = false;
		SoundManager::GetInstance()->PlaySE(SoundEffect::kJump);
	}

	// Spaceを押した瞬間に方向選択を開始する。輪郭上でも方向はプレイヤーが選ぶ。
	if (!detachFromTrace && !isRushAiming_ && !isJumpRushing_ &&
		(onGround_ || canAirRush_) && isRushTriggered)
	{

		// 空中から方向選択を開始した場合は、この時点で使用回数を消費する。
		// 地上からの場合は、実際に突進を発動した時点で消費する。
		if (!onGround_)
		{
			canAirRush_ = false;
		}
		else
		{
			// 地上での方向選択だけは、狙っている間に歩き続けないよう横速度を止める。
			velocity_.x = 0.0f;
		}
		isRushAiming_ = true;
		rushAimElapsedSeconds_ = 0.0f;
		traceDirectionSelected_ = false;
		rushAngle_ = std::numbers::pi_v<float> / 2.0f;
	}

	if (isRushAiming_)
	{
		// 輪郭の頂点では上の入力処理が選択中も座標を固定する。
		// スローモーションの影響を受けない実時間で数える。
		rushAimElapsedSeconds_ += GameTime::GetUnscaledDeltaSeconds();

		const bool isRushAimTimeExpired = rushAimElapsedSeconds_ >= kRushAimMaxSeconds;

		// Spaceを離すか、制限時間が経過したら発動する。
		const bool shouldLaunchRush = !isRushPushed || isRushAimTimeExpired;

		if (!shouldLaunchRush) 
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
				GamePadInput::GetAimDirection(directionX, directionY);
			}

			if (directionX != 0 || directionY != 0)
			{
				// 図形をなぞる方向も自動補正せず、通常突進と同じ8方向から選ぶ。
				rushAngle_ = std::atan2(static_cast<float>(directionY), static_cast<float>(directionX));
				if (traceAimActive_)
				{
					traceDirectionSelected_ = true;
				}
				const float selectedX = std::cos(rushAngle_);
				if (selectedX > 0.01f)
				{
					beginTurn(LRDirection::kRight);
				}
				else if (selectedX < -0.01f)
				{
					beginTurn(LRDirection::kLeft);
				}
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
			if (traceAimActive_ && !IsTraceAimDirectionValid())
			{
				// 辺と異なる方向なら突進せず、頂点に留まって選び直せる。
				isRushAiming_ = false;
				rushAimElapsedSeconds_ = 0.0f;
				canAirRush_ = true;
			}
			else
			{
				const bool isGroundRush = onGround_ && !traceAimActive_;
				velocity_.x = directionX * kJumpRushSpeed;
				velocity_.y = directionY * kJumpRushSpeed;
				if (isGroundRush)
				{
					// 水平方向の突進でも床へ接触したままにならないよう、発動時だけ少し浮かせる。
					worldTransform_.translation_.y += kGroundRushLift;
				}
				onGround_ = false;
				// 地上発動も空中分を消費し、着地または攻撃成功まで連続発動を禁止する。
				canAirRush_ = false;

				isRushAiming_ = false;
				traceRushActive_ = traceAimActive_;
				traceAimActive_ = false;
				rushAimElapsedSeconds_ = 0.0f;
				isJumpRushing_ = true;
				isJumpRushEnding_ = false;
				rushVisualIntervalSeconds_ = 0.0f;
				jumpRushFrame_ = 0;
				rushStartPosition_ = worldTransform_.translation_;
				rushDirection_ = {directionX, directionY, 0.0f};
				++rushSequence_;
				SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerAttack);
			}
		}
	}

	// 方向選択中も、開始前の慣性と重力に従ってスローモーションで動き続ける。
	// 突進中だけは全方向の移動量を揃えるため重力を加えない。
	if (!onGround_ && !isJumpRushing_ && !traceAimActive_)
	{
		velocity_.y -= kGravityAcceleration * deltaFrames;
		velocity_.y = (std::max)(velocity_.y, -kLimitFallSpeed);
	}
}

bool Player::CanStartRush() const
{
	return !isDead_ && behavior_ == Behavior::kRoot && behaviorRequest_ == Behavior::kUnknown &&
		!isHanging_ && !isRushAiming_ && !isJumpRushing_ && (onGround_ || canAirRush_);
}

bool Player::IsTraceAimDirectionValid() const
{
	if (!traceAimActive_ || !traceDirectionSelected_)
	{
		return false;
	}
	const float x = std::cos(rushAngle_);
	const float y = std::sin(rushAngle_);
	for (int i = 0; i < traceAimDirectionCount_; ++i)
	{
		if (x * traceAimDirections_[i].x + y * traceAimDirections_[i].y >= 0.96f)
		{
			return true;
		}
	}
	return false;
}

void Player::SnapToTraceVertex(const Vector3& vertex,
	const std::array<Vector3, 2>& directions, int directionCount)
{
	if (!CanStartRush() || directionCount <= 0)
	{
		return;
	}
	traceAimActive_ = true;
	traceRushActive_ = false;
	traceAimAnchor_ = vertex;
	traceAimDirections_ = directions;
	traceAimDirectionCount_ = (std::min)(directionCount, 2);
	// 地上から吸い付いた場合も、次のMoveInputで空中突進を開始できるようにする。
	if (onGround_)
	{
		canAirRush_ = true;
	}
	worldTransform_.translation_ = vertex;
	previousPosition_ = vertex;
	velocity_ = {};
	onGround_ = false;
	UpdateMatrix();
}

void Player::FinishRushAtVertex(const Vector3& vertex,
	const std::array<Vector3, 2>& nextDirections, int nextDirectionCount)
{
	worldTransform_.translation_ = vertex;
	previousPosition_ = vertex;
	velocity_ = {};
	onGround_ = false;
	canAirRush_ = true;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	traceAimActive_ = nextDirectionCount > 0;
	traceRushActive_ = false;
	traceDirectionSelected_ = false;
	traceAimAnchor_ = vertex;
	traceAimDirections_ = nextDirections;
	traceAimDirectionCount_ = (std::clamp)(nextDirectionCount, 0, 2);
	UpdateMatrix();
}

void Player::DetachFromTraceVertex()
{
	detachedFromTraceThisFrame_ = true;
	traceAimActive_ = false;
	traceRushActive_ = false;
	traceDirectionSelected_ = false;
	traceAimDirectionCount_ = 0;
	isRushAiming_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	rushAimElapsedSeconds_ = 0.0f;
	velocity_ = {};
	onGround_ = false;
}

bool Player::WantsToGrabVine() const
{
	return !isDead_ && !isHanging_ && vineDetachCooldown_ <= 0.0f &&
		!traceAimActive_ && !traceRushActive_ && !detachedFromTraceThisFrame_ &&
		(Input::GetInstance()->TriggerKey(DIK_E) || GamePadInput::IsGrabTriggered());
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
	traceAimActive_ = false;
	traceRushActive_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
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
	traceAimActive_ = false;
	traceRushActive_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
}

void Player::UpdateHanging()
{
	const float deltaFrames = GameTime::GetDeltaFrames();
	const bool isLeftPushed = Input::GetInstance()->PushKey(DIK_LEFT) || Input::GetInstance()->PushKey(DIK_A) || GamePadInput::IsLeftPushed();
	const bool isRightPushed = Input::GetInstance()->PushKey(DIK_RIGHT) || Input::GetInstance()->PushKey(DIK_D) || GamePadInput::IsRightPushed();
	const bool isRushTriggered = Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsRushTriggered();

	if (isLeftPushed)
	{
		vineAngularVelocity_ -= kVineSwingAcceleration * deltaFrames;
	}
	if (isRightPushed)
	{
		vineAngularVelocity_ += kVineSwingAcceleration * deltaFrames;
	}

	// 振り子の重力と空気抵抗
	vineAngularVelocity_ -= std::sin(vineAngle_) * kVineGravity * deltaFrames;
	vineAngularVelocity_ *= GameTime::GetDampingFactor(0.99f);
	vineAngularVelocity_ = std::clamp(vineAngularVelocity_, -kVineMaxAngularVelocity, kVineMaxAngularVelocity);
	vineAngle_ += vineAngularVelocity_ * deltaFrames;
	vineAngle_ = std::clamp(vineAngle_, -1.2f, 1.2f);

	worldTransform_.translation_.x = vineAnchorPosition_.x + std::sin(vineAngle_) * kVineLength;
	worldTransform_.translation_.y = vineAnchorPosition_.y - std::cos(vineAngle_) * kVineLength;
	velocity_ = {};

	if (isRushTriggered)
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

	// 突進の角補正候補が、移動途中も含めてブロックを通らないか調べる。
	auto isSweptAreaClear = [&](float left, float right, float bottom, float top) {
		const TileRange range = makeTileRange(left, right, bottom, top);
		for (int32_t y = range.minY; y <= range.maxY; ++y)
		{
			for (int32_t x = range.minX; x <= range.maxX; ++x)
			{
				if (!isBlock(x, y))
				{
					continue;
				}
				const MapChipField::Rect rect = mapChipField_->GetRectByIndex(x, y);
				if (right > rect.left + epsilon && left < rect.right - epsilon &&
					top > rect.bottom + epsilon && bottom < rect.top - epsilon)
				{
					return false;
				}
			}
		}
		return true;
	};
	auto isHorizontalSweepClear = [&](float centerX, float centerY, float moveX) {
		return isSweptAreaClear((std::min)(centerX, centerX + moveX) - halfWidth,
			(std::max)(centerX, centerX + moveX) + halfWidth,
			centerY - halfHeight, centerY + halfHeight);
	};
	auto isVerticalSweepClear = [&](float centerX, float centerY, float moveY) {
		return isSweptAreaClear(centerX - halfWidth, centerX + halfWidth,
			(std::min)(centerY, centerY + moveY) - halfHeight,
			(std::max)(centerY, centerY + moveY) + halfHeight);
	};
	const float startX = worldTransform_.translation_.x;
	const float startY = worldTransform_.translation_.y;
	float cornerCorrectionY = 0.0f;
	if (isJumpRushing_ && std::abs(info.move.x) > epsilon &&
		!isHorizontalSweepClear(startX, startY, info.move.x))
	{
		// 横突進で上端・下端を少しかすめた場合、空いている側へ最小限ずらす。
		const float preferredSign = info.move.y < 0.0f ? -1.0f : 1.0f;
		for (float distance = kRushCornerCorrectionStep;
			distance <= kRushCornerCorrectionMax + epsilon; distance += kRushCornerCorrectionStep)
		{
			for (float sign : {preferredSign, -preferredSign})
			{
				const float correction = sign * distance;
				if (isVerticalSweepClear(startX, startY, correction) &&
					isHorizontalSweepClear(startX, startY + correction, info.move.x))
				{
					cornerCorrectionY = correction;
					break;
				}
			}
			if (cornerCorrectionY != 0.0f)
			{
				break;
			}
		}
	}
	const float collisionY = startY + cornerCorrectionY;

	// X 軸方向の衝突を先に解決する。
	if (info.move.x > 0.0f) 
	{
		const float currentRight = worldTransform_.translation_.x + halfWidth;
		const float targetRight = currentRight + info.move.x;
		const float playerBottom = collisionY - halfHeight;
		const float playerTop = collisionY + halfHeight;
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
	}
	else if (info.move.x < 0.0f)
	{
		const float currentLeft = worldTransform_.translation_.x - halfWidth;
		const float targetLeft = currentLeft + info.move.x;
		const float playerBottom = collisionY - halfHeight;
		const float playerTop = collisionY + halfHeight;
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

	// 上向き突進で天井の左右端をかすめたときも、空いた側へ小さく逃がす。
	float cornerCorrectionX = 0.0f;
	const float collisionX = startX + info.move.x;
	if (isJumpRushing_ && info.move.y > epsilon &&
		!isVerticalSweepClear(collisionX, collisionY, info.move.y))
	{
		const float preferredSign = info.move.x < 0.0f ? -1.0f : 1.0f;
		for (float distance = kRushCornerCorrectionStep;
			distance <= kRushCornerCorrectionMax + epsilon; distance += kRushCornerCorrectionStep)
		{
			for (float sign : {preferredSign, -preferredSign})
			{
				const float correction = sign * distance;
				if (isHorizontalSweepClear(collisionX, collisionY, correction) &&
					isVerticalSweepClear(collisionX + correction, collisionY, info.move.y))
				{
					cornerCorrectionX = correction;
					break;
				}
			}
			if (cornerCorrectionX != 0.0f)
			{
				break;
			}
		}
	}

	// X 軸の解決後の位置を用いて Y 軸方向の衝突を解決する。
	const float playerLeft = collisionX + cornerCorrectionX - halfWidth;
	const float playerRight = collisionX + cornerCorrectionX + halfWidth;
	if (info.move.y > 0.0f)
	{
		const float currentTop = collisionY + halfHeight;
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
					info.move.y = std::min(info.move.y, rect.bottom - collisionY - (halfHeight + kBlank));
					info.isCeiling = true;
				}
			}
		}
	}
	else if (info.move.y < 0.0f)
	{
		const float currentBottom = collisionY - halfHeight;
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
					info.move.y = (std::max)(info.move.y, rect.top - collisionY + (halfHeight + kBlank));
					info.isLanding = true;
				}
			}
		}
	}
	info.move.x += cornerCorrectionX;
	info.move.y += cornerCorrectionY;
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
	if (hit)
	{

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
	if (hit)
	{
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
	if (info.isCeiling)
	{
		velocity_.y = 0.0f;
	}
}

// 壁に接触している場合の処理
void Player::ContactToWall(const CollisionMapInfo& info)
{
	// 壁接触による減速
	if (info.hitWall)
	{
		velocity_.x *= GameTime::GetDampingFactor(1.0f - kAttenuationWall);
	}
}

// 接地状態の切り替え処理
void Player::ContactToGroundStateSwitch(const CollisionMapInfo& info)
{
	// 接地している状態のとき（足場から外れて落下するかどうかのチェック）
	if (onGround_)
	{
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
		velocity_.x *= GameTime::GetDampingFactor(1.0f - kAttenuation);
		// 下方向速度をリセット
		velocity_.y = 0.0f;
		// 接地状態に移行
		onGround_ = true;
		++landingSequence_;
		canAirRush_ = true;
	}
}

void Player::UpdateMatrix()
{
	// アフィン変換行列の作成
	worldTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransform_.scale_, worldTransform_.rotation_, worldTransform_.translation_);

	// 行列を定数バッファに転送
	worldTransform_.TransferMatrix();

	// 攻撃エフェクトの行列計算と転送
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
	if (!onGround_)
	{
		++landingSequence_;
	}
	// 足場の移動量を座標へ一度だけ反映する。
	// 速度にも同じ値を入れると、次フレームに二重に運ばれてしまう。
	worldTransform_.translation_.x += horizontalMove;
	worldTransform_.translation_.y = topY + kHeight / 2.0f + kBlank;

	velocity_.y = 0.0f;
	onGround_ = true;
	canAirRush_ = true;

	UpdateMatrix();
}

void Player::BounceOnGimmick(float topY, float bounceVelocity) {
	// 足場の上へ正確に配置する
	worldTransform_.translation_.y = topY + kHeight / 2.0f + kBlank;

	// 上向きに跳ね返す
	velocity_.y = bounceVelocity;

	// 空中状態へ切り替える
	onGround_ = false;
	// 跳ねる足場へ触れた場合も、着地として空中突進を回復する。
	canAirRush_ = true;

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

bool Player::TryLandOnOneWayGimmick(const AABB& gimmickAABB, float horizontalMove, float verticalMove)
{
	const AABB previous = GetPreviousAABB();
	const AABB current = GetAABB();
	const float previousTop = gimmickAABB.max.y - verticalMove;
	const float currentTop = gimmickAABB.max.y;
	const bool wasOverPlatform =
		previous.max.x > gimmickAABB.min.x - horizontalMove &&
		previous.min.x < gimmickAABB.max.x - horizontalMove;
	const bool willOverlapAfterCarry =
		current.max.x + horizontalMove > gimmickAABB.min.x &&
		current.min.x + horizontalMove < gimmickAABB.max.x;
	const bool overlapsPlatformNow =
		current.max.x > gimmickAABB.min.x && current.min.x < gimmickAABB.max.x;
	constexpr float kFootTolerance = 0.03f;
	const float rideGap = (std::max)(0.08f, std::abs(verticalMove) + 0.04f);
	const bool wasRiding = wasOverPlatform && willOverlapAfterCarry &&
		previous.min.y >= previousTop - kFootTolerance &&
		previous.min.y <= previousTop + 0.06f &&
		current.min.y <= currentTop + rideGap && velocity_.y <= 0.0f;
	const bool crossedTopFromAbove = overlapsPlatformNow &&
		previous.min.y >= previousTop - kFootTolerance &&
		current.min.y <= currentTop + kFootTolerance &&
		current.min.y - previous.min.y <= verticalMove + kFootTolerance;
	if (!wasRiding && !crossedTopFromAbove)
	{
		return false;
	}

	LandOnGimmick(currentTop, horizontalMove);
	return true;
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

	if (isJumpRushing_)
	{
		// 石橋・雲などの足場も、突進で端を少しかすめた時だけ逃がす。
		// 下向きの着地は優先し、足場を通り抜ける補正は行わない。
		auto canOccupyMap = [this](float x, float y) {
			if (!mapChipField_)
			{
				return true;
			}
			const float left = x - kWidth * 0.5f;
			const float right = x + kWidth * 0.5f;
			const float bottom = y - kHeight * 0.5f;
			const float top = y + kHeight * 0.5f;
			const IndexSet minIndex = mapChipField_->GetMapChipIndexSetByPosition({left, top, 0.0f});
			const IndexSet maxIndex = mapChipField_->GetMapChipIndexSetByPosition({right, bottom, 0.0f});
			for (int32_t tileY = minIndex.yIndex; tileY <= maxIndex.yIndex; ++tileY)
			{
				for (int32_t tileX = minIndex.xIndex; tileX <= maxIndex.xIndex; ++tileX)
				{
					if (mapChipField_->GetMapChipTypeByIndex(tileX, tileY) != MapChipType::kBlock)
					{
						continue;
					}
					const MapChipField::Rect rect = mapChipField_->GetRectByIndex(tileX, tileY);
					if (right > rect.left + kEpsilon && left < rect.right - kEpsilon &&
						top > rect.bottom + kEpsilon && bottom < rect.top - kEpsilon)
					{
						return false;
					}
				}
			}
			return true;
		};
		auto tryShift = [&](float shiftX, float shiftY) {
			if (std::abs(shiftX) > kRushCornerCorrectionMax ||
				std::abs(shiftY) > kRushCornerCorrectionMax)
			{
				return false;
			}
			const float targetX = worldTransform_.translation_.x + shiftX;
			const float targetY = worldTransform_.translation_.y + shiftY;
			if (!canOccupyMap(targetX, targetY))
			{
				return false;
			}
			worldTransform_.translation_.x = targetX;
			worldTransform_.translation_.y = targetY;
			UpdateMatrix();
			return true;
		};
		if (previousAABB.max.y <= gimmickAABB.min.y + kEpsilon && velocity_.y > 0.0f)
		{
			const float shiftLeft = gimmickAABB.min.x - currentAABB.max.x - kBlank;
			const float shiftRight = gimmickAABB.max.x - currentAABB.min.x + kBlank;
			const bool leftFirst = std::abs(shiftLeft) <= std::abs(shiftRight);
			if (leftFirst ? (tryShift(shiftLeft, 0.0f) || tryShift(shiftRight, 0.0f))
				: (tryShift(shiftRight, 0.0f) || tryShift(shiftLeft, 0.0f)))
			{
				return GimmickHitSide::kNone;
			}
		}
		else if (previousAABB.min.y < gimmickAABB.max.y - kEpsilon)
		{
			const bool fromLeft = previousAABB.max.x <= gimmickAABB.min.x + kEpsilon && velocity_.x > 0.0f;
			const bool fromRight = previousAABB.min.x >= gimmickAABB.max.x - kEpsilon && velocity_.x < 0.0f;
			if (fromLeft || fromRight)
			{
				const float shiftUp = gimmickAABB.max.y - currentAABB.min.y + kBlank;
				const float shiftDown = gimmickAABB.min.y - currentAABB.max.y - kBlank;
				const bool upFirst = std::abs(shiftUp) <= std::abs(shiftDown);
				if (upFirst ? (tryShift(0.0f, shiftUp) || tryShift(0.0f, shiftDown))
					: (tryShift(0.0f, shiftDown) || tryShift(0.0f, shiftUp)))
				{
					return GimmickHitSide::kNone;
				}
			}
		}
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
	const Vector3 attackCenter = GetWorldPosition();
	// 突進中はプレイヤーの周囲全体を攻撃範囲にする。
	const float halfSize = kRushAttackSize * 0.5f;

	AABB aabb;
	aabb.min = {attackCenter.x - halfSize, attackCenter.y - halfSize, attackCenter.z - halfSize};
	aabb.max = {attackCenter.x + halfSize, attackCenter.y + halfSize, attackCenter.z + halfSize};

	return aabb;
}

void Player::OnCollision(BaseEnemy* enemy)
{
	if (!enemy || IsAttack())
	{
		return; // 攻撃中はダメージ無効
	}

	TakeDamage(1, enemy->GetWorldPosition().x);
}

void Player::OnHazardCollision(float sourceX)
{
	TakeDamage(1, sourceX);
}

void Player::TakeDamage(int32_t damage, float sourceX)
{
	if (damage <= 0 || damageInvincibleTimer_ > 0 || isDead_)
	{
		return;
	}

	hp_ = (std::max)(0, hp_ - damage);
	++damageSequence_;
	damageInvincibleTimer_ = kDamageInvincibleFrames;
	damageFlashActive_ = true;
	damageRedTimer_ = kDamageRedFrames;
	if (hp_ == 0)
	{
		isDead_ = true;
	}
	else
	{
		SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerDamage);
		RequestKnockback(sourceX);
	}
}

void Player::RequestKnockback(float sourceX)
{
	if (isDead_ || isKnockbackRequested_ || behavior_ == Behavior::kKnockback)
	{
		return;
	}
	const float playerX = GetWorldPosition().x;
	constexpr float kSideEpsilon = 0.01f;
	if (playerX < sourceX - kSideEpsilon)
	{
		knockbackDirection_ = -1.0f;
	}
	else if (playerX > sourceX + kSideEpsilon)
	{
		knockbackDirection_ = 1.0f;
	}
	else if (previousPosition_.x < sourceX - kSideEpsilon)
	{
		knockbackDirection_ = -1.0f;
	}
	else if (previousPosition_.x > sourceX + kSideEpsilon)
	{
		knockbackDirection_ = 1.0f;
	}
	else if (std::abs(velocity_.x) > kSideEpsilon)
	{
		knockbackDirection_ = velocity_.x > 0.0f ? -1.0f : 1.0f;
	}
	else
	{
		knockbackDirection_ = lrDirection_ == LRDirection::kRight ? -1.0f : 1.0f;
	}
	// ツルにつかまっていても次の更新でノックバックへ移れるようにする。
	isHanging_ = false;
	isKnockbackRequested_ = true;
}

void Player::Respawn(const Vector3& position, bool restoreHp)
{
	// 取得済みのリングは、死亡後の復活でも再出現できるようにする。
	++landingSequence_;
	worldTransform_.translation_ = position;
	previousPosition_ = position;
	velocity_ = {};

	if (restoreHp)
	{
		hp_ = kMaxHp;
	}

	isDead_ = false;
	isKnockbackRequested_ = false;
	knockbackDirection_ = 0.0f;
	knockbackParameter_ = 0.0f;
	damageInvincibleTimer_ = kDamageInvincibleFrames;
	// 部屋移動やチェックポイント復活にも無敵時間はあるが、被弾演出は出さない。
	damageFlashActive_ = false;
	damageRedTimer_ = 0.0f;
	onGround_ = false;
	canAirRush_ = true;
	isHanging_ = false;
	vineDetachCooldown_ = 0;
	isRushAiming_ = false;
	traceAimActive_ = false;
	traceRushActive_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;
	jumpRushFrame_ = 0;
	behavior_ = Behavior::kRoot;
	behaviorRequest_ = Behavior::kUnknown;
	ClearRushVisuals();
	UpdateMatrix();
}

// 通常行動初期化
void Player::BehaviorRootInitialize() {}

// 通常行動更新
void Player::BehaviorRootUpdate()
{

	if (onGround_) {
		canAirRush_ = true;
	}

	// 移動入力の処理を呼び出す
	MoveInput();

	// 衝突情報を初期化
	CollisionMapInfo collisionMapInfo;
	// 移動量に速度の値をコピー
	collisionMapInfo.move = MyMathUtility::Multiply(GameTime::GetDeltaFrames(), velocity_);

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
		jumpRushFrame_ += GameTime::GetDeltaFrames();
		const bool rushFinished = jumpRushFrame_ >= kJumpRushFrame;
		const bool hitSurface = collisionMapInfo.hitWall || collisionMapInfo.isCeiling || collisionMapInfo.isLanding;
		if (rushFinished || hitSurface)
		{
			// 突進終了後も一部の速度を残し、通常移動へ滑らかにつなげる
			velocity_.x *= kRushExitHorizontalRetention;
			velocity_.y *= kRushExitVerticalRetention;

			// 衝突した方向には慣性を残さない
			if (collisionMapInfo.hitWall) {
				velocity_.x = 0.0f;
			}

			if (collisionMapInfo.isCeiling || collisionMapInfo.isLanding) {
				velocity_.y = 0.0f;
			}

			// 敵との衝突判定が終わるまで攻撃判定を残す。
			isJumpRushEnding_ = true;
		}

	}

	// 旋回制御
	if (turnTimer_ > 0.0f)
	{
		turnTimer_ -= GameTime::GetDeltaSeconds();

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
	if (isJumpRushing_ && !isJumpRushEnding_)
	{
		EmitRushVisuals();
	}
}

void Player::OnAttackHit()
{
	// 突進が敵に当たったら、空中での方向選択突進を1回分回復する。
	canAirRush_ = true;
}

bool Player::TryCollectAirRushRing()
{
	if (isDead_ || onGround_)
	{
		return false;
	}
	// 既に使用可能でも2回分には増やさず、1回分を満たすだけにする。
	canAirRush_ = true;
	++ringPickupSequence_;
	SoundManager::GetInstance()->PlaySE(SoundEffect::kRingGet);
	return true;
}

// ノックバック行動初期化
void Player::BehaviorKnockbackInitialize()
{
	knockbackParameter_ = 0.0f;
	isRushAiming_ = false;
	traceAimActive_ = false;
	traceRushActive_ = false;
	isJumpRushing_ = false;
	isJumpRushEnding_ = false;

	// ノックバック時の表示スケールをデフォルトに戻す。
	worldTransform_.scale_ = {1.0f, 1.0f, 1.0f};

	// 向きではなく、衝突した敵・ギミックと反対側へ押し出す。
	velocity_.x = knockbackDirection_ * kKnockbackHorizontalSpeed;
	velocity_.y = kKnockbackVerticalSpeed;
	onGround_ = false;
}

// ノックバック行動更新
void Player::BehaviorKnockbackUpdate()
{
	knockbackParameter_ += GameTime::GetDeltaFrames();

	// 強い初速で弾き飛ばされるフェーズ
	if (knockbackParameter_ < kKnockbackBlowTime)
	{
		// 空気抵抗などで徐々に減速させる
		velocity_.x *= GameTime::GetDampingFactor(0.9f);
	} else if (knockbackParameter_ < kKnockbackBlowTime + kKnockbackRecoverTime) {
		// 移動が停止し、体勢を立て直すフェーズ
		velocity_.x = 0.0f; // 完全に停止して硬直
	}

	else {
		// 終了して通常状態へ
		behaviorRequest_ = Behavior::kRoot;
	}
	if (!onGround_)
	{
		velocity_.y = (std::max)(-kLimitFallSpeed,
			velocity_.y - kGravityAcceleration * GameTime::GetDeltaFrames());
	}

	// 共通の移動・衝突判定処理
	CollisionMapInfo collisionMapInfo;
	collisionMapInfo.move = MyMathUtility::Multiply(GameTime::GetDeltaFrames(), velocity_);
	CheckMapCollision(collisionMapInfo);
	JudgmentResultReflectionMove(collisionMapInfo);
	ContactToCeiling(collisionMapInfo);
	ContactToWall(collisionMapInfo);
	ContactToGroundStateSwitch(collisionMapInfo);
	if (collisionMapInfo.hitWall)
	{
		velocity_.x = 0.0f;
	}

	UpdateMatrix();
}

void Player::Draw() 
{
	// 残像と粒を本体より先に描く。攻撃判定は IsAttack() / GetAttackAABB() が担当する。
	if (modelFlash_)
	{
		for (auto& afterimage : rushAfterimages_)
		{
			if (afterimage.remainingSeconds > 0.0f)
			{
				modelFlash_->Draw(afterimage.transform, *camera_, whiteTextureHandle_, &afterimage.color);
			}
		}
	}
	for (auto& grain : rushGrains_)
	{
		if (grain.remainingSeconds <= 0.0f)
		{
			continue;
		}
		Model* grainModel = grain.board ? rushBoardEffectModel_ : rushBoxEffectModel_;
		if (grainModel)
		{
			grainModel->Draw(grain.transform, *camera_, grain.board ? &rushBoardColor_ : &rushBoxColor_);
		}
	}
	const float flashElapsed = (std::max)(0.0f,
		kDamageInvincibleFrames - damageInvincibleTimer_ - kDamageRedFrames);
	const bool flashWhite = damageFlashActive_ && damageInvincibleTimer_ > 0.0f &&
		static_cast<int32_t>(flashElapsed / kDamageFlashIntervalFrames) % 2 == 0;
	if (modelFlash_ && damageFlashActive_ && damageRedTimer_ > 0.0f)
	{
		// 被弾直後は元テクスチャを白に差し替え、輪郭全体を赤くする。
		damageVisualColor_.SetColor({1.0f, 0.08f, 0.08f, 1.0f});
		modelFlash_->Draw(worldTransform_, *camera_, whiteTextureHandle_, &damageVisualColor_);
	}
	else if (modelFlash_ && flashWhite)
	{
		// 無敵中は白と通常のテクスチャを交互に描き、透明にはしない。
		damageVisualColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		modelFlash_->Draw(worldTransform_, *camera_, whiteTextureHandle_, &damageVisualColor_);
	}
	else
	{
		model_->Draw(worldTransform_, *camera_);
	}
}
