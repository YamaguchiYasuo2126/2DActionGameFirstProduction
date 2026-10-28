#include "StageGimmick.h"

#include "GameTime.h"
#include "MyMathUtility.h"
#include "MapChipField.h"
#include "Player.h"
#include "SoundManager.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numbers>

using namespace KamataEngine;

void StageGimmick::Initialize(Model* model, Model* lightningModel,
	Model* wheelShaftModel,
	Model* ringBoxEffectModel, Model* ringBoardEffectModel,
	Camera* camera, MapChipField* mapChipField,
	SwitchLinkState* switchLinkState, const Vector3& position, GimmickType type,
	const std::string& linkId)
{
	model_ = model;
	lightningModel_ = lightningModel;
	wheelShaftModel_ = wheelShaftModel;
	ringBoxEffectModel_ = ringBoxEffectModel;
	ringBoardEffectModel_ = ringBoardEffectModel;
	camera_ = camera;
	mapChipField_ = mapChipField;
	switchLinkState_ = switchLinkState;
	linkId_ = linkId;
	type_ = type;
	startPosition_ = position;
	fallingRockPreviousPosition_ = position;

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
		fountainJetColor_.Initialize();
		fountainCoreColor_.Initialize();
		fountainDropColor_.Initialize();
		fountainJetTransform_.Initialize();
		fountainCoreTransform_.Initialize();
		for (auto& transform : fountainDropTransforms_)
		{
			transform.Initialize();
		}
	}
	else if (type_ == GimmickType::kWaterWheelLift)
	{
		// Tiledの配置点を水車の中心とし、4枚の台と支柱を別々に動かす。
		wheelShaftTransform_.Initialize();
		wheelShaftTransform_.translation_ = position;
		// 円柱の径だけを縮め、奥行きのスケールは維持する。
		wheelShaftTransform_.scale_ = {1.1f, 1.1f, 0.65f};
		wheelShaftColor_.Initialize();
		wheelShaftColor_.SetColor({0.52f, 0.29f, 0.13f, 1.0f});
		for (size_t i = 0; i < wheelPlatformTransforms_.size(); ++i)
		{
			wheelPlatformTransforms_[i].Initialize();
			wheelPlatformTransforms_[i].scale_ =
				{kWaterWheelPlatformWidth, kWaterWheelPlatformHeight, 1.0f};
			wheelSpokeTransforms_[i].Initialize();
			// movingPlatform.objの一辺は約0.983。支柱の長さを回転半径に合わせる。
			constexpr float kModelSpan = 0.982598f;
			wheelSpokeTransforms_[i].scale_ =
				{kWaterWheelRadius / kModelSpan, 0.14f / kModelSpan, 0.16f / kModelSpan};
		}
		wheelAngle_ = std::numbers::pi_v<float> / 4.0f;
		UpdateWaterWheelGeometry(false);
	}
	else if (type_ == GimmickType::kUpdraft)
	{
		worldTransform_.scale_ = {1.0f, 2.0f, 1.0f};
		updraftContactSeconds_ = kUpdraftContactEffectSeconds;
		updraftStreakColor_.Initialize();
		updraftContactColor_.Initialize();
		updraftContactColor_.SetColor({0.65f, 1.0f, 1.0f, 1.0f});
		for (auto& transform : updraftStreakTransforms_)
		{
			transform.Initialize();
		}
		for (auto& transform : updraftContactTransforms_)
		{
			transform.Initialize();
		}
	}
	else if (type_ == GimmickType::kFallingRock && mapChipField_)
	{
		// 落下列で最初に当たる床の上面を、予告影の投影先にする。
		const IndexSet startIndex = mapChipField_->GetMapChipIndexSetByPosition(position);
		for (int32_t y = startIndex.yIndex + 1;
			y < static_cast<int32_t>(mapChipField_->GetNumBlockVirtical()); ++y)
		{
			if (mapChipField_->GetMapChipTypeByIndex(startIndex.xIndex, y) == MapChipType::kBlock)
			{
				fallingRockImpactPosition_ = {position.x,
					mapChipField_->GetRectByIndex(startIndex.xIndex, y).top, position.z};
				fallingRockHasLanding_ = true;
				fallingRockMapImpactPosition_ = fallingRockImpactPosition_;
				fallingRockHasMapLanding_ = true;
				break;
			}
		}
	}
	else if (type_ == GimmickType::kVine)
	{
		worldTransform_.scale_ = {0.45f, 4.0f, 1.0f};
	}
	else if (type_ == GimmickType::kOreSwitch)
	{
		worldTransform_.scale_ = {0.8f, 0.3f, 1.0f};
	}
	else if (type_ == GimmickType::kAirRushSwitch)
	{
		// 空中に浮く白い立方体を水色に着色する。
		worldTransform_.scale_ = {0.8f, 0.8f, 0.8f};
		airRushSwitchColor_.Initialize();
		airRushSwitchColor_.SetColor({0.15f, 0.90f, 1.0f, 1.0f});
	}
	else if (type_ == GimmickType::kStoneBridge)
	{
		worldTransform_.scale_ = {2.0f, 0.5f, 1.0f};
		stoneBridgeColor_.Initialize();
	}
	else if (type_ == GimmickType::kVanishingCloud)
	{
		worldTransform_.scale_ = {1.5f, 0.5f, 1.0f};
		vanishingCloudColor_.Initialize();
	}
	else if (type_ == GimmickType::kAirRushRing)
	{
		// 当たり判定は輪の内側を含めた約1タイル。リングモデル自体は2x2の平面。
		worldTransform_.scale_ = {0.9f, 0.9f, 1.0f};
		airRushRingColor_.Initialize();
		airRushRingColor_.SetColor({0.20f, 0.95f, 1.0f, 1.0f});
		ringBoxEffectColor_.Initialize();
		ringBoxEffectColor_.SetColor({0.20f, 0.95f, 1.0f, 1.0f});
		ringBoardEffectColor_.Initialize();
		ringBoardEffectColor_.SetColor({1.0f, 1.0f, 1.0f, 1.0f});
		ringShockwaveColor_.Initialize();
		ringShockwaveColor_.SetColor({0.45f, 1.0f, 1.0f, 1.0f});
		for (auto& transform : ringBoxEffectTransforms_)
		{
			transform.Initialize();
		}
		ringShockwaveTransform_.Initialize();
		for (auto& transform : ringBoardEffectTransforms_)
		{
			transform.Initialize();
		}
	}

	if (type_ == GimmickType::kOreSwitch || type_ == GimmickType::kStoneBridge)
	{
		// タイル中央の配置点から底面を少し下げ、土台ブロックの上面へ揃える。
		// モデルと当たり判定の底面を、直下ブロックの上面へ揃える。
		worldTransform_.translation_.y -= 0.5f - worldTransform_.scale_.y * 0.5f;
		startPosition_ = worldTransform_.translation_;
	}

	// 基本は当たり判定用と同じ変換で描画する。
	modelWorldTransform_.scale_ = worldTransform_.scale_;

	if (type_ == GimmickType::kFountainJump)
	{
		// 噴水はモデル自体に高さがあるため、当たり判定の薄いYスケールを使わない。
		// 配置点の近くにある足場の上面へ、モデルの底を揃える。
		modelWorldTransform_.scale_ = {1.0f, 1.0f, 1.0f};
		modelOffset_.y = -0.268f;
	}
	else if (type_ == GimmickType::kVine)
	{
		// ツルモデルの原点は上端に近いので、見た目の上端とつかまる位置を合わせる。
		modelOffset_.y = worldTransform_.scale_.y * 0.5f + 0.03f;
	}
	else if (type_ == GimmickType::kAirRushRing)
	{
		modelWorldTransform_.scale_ = {0.5f, 0.5f, 0.5f};
		// 地形の前面より手前へ出し、近くのブロックに隠れないようにする。
		modelOffset_.z = -0.65f;
	}
	else if (type_ == GimmickType::kAirRushSwitch)
	{
		// boxEffectGrain.obj は一辺が2なので、見た目を当たり判定の0.8に揃える。
		modelWorldTransform_.scale_ = {0.4f, 0.4f, 0.4f};
	}

	if (type_ == GimmickType::kUpdraft)
	{
		UpdateUpdraftEffect();
	}
	UpdateMatrix();
}

void StageGimmick::Update()
{
	const float deltaFrames = GameTime::GetDeltaFrames();
	movementDelta_ = {};

	if (!isActive_)
	{
		return;
	}
	if (type_ == GimmickType::kAirRushRing)
	{
		UpdateAirRushRing();
		return;
	}
	if (type_ == GimmickType::kFountainJump && fountainBurstActive_)
	{
		fountainBurstSeconds_ += GameTime::GetDeltaSeconds();
		if (fountainBurstSeconds_ >= kFountainBurstDurationSeconds)
		{
			fountainBurstActive_ = false;
			fountainJetHeight_ = 0.0f;
		}
		else
		{
			UpdateFountainBurst();
		}
	}
	if (type_ == GimmickType::kUpdraft)
	{
		updraftTouchedLastFrame_ = updraftTouchedThisFrame_;
		updraftTouchedThisFrame_ = false;
		updraftVisualSeconds_ += GameTime::GetDeltaSeconds();
		updraftContactSeconds_ = (std::min)(
			updraftContactSeconds_ + GameTime::GetDeltaSeconds(),
			kUpdraftContactEffectSeconds);
		UpdateUpdraftEffect();
	}

	if (type_ == GimmickType::kMovingPlatform)
	{
		const float previousX = worldTransform_.translation_.x;
		worldTransform_.translation_.x = startPosition_.x + std::sin(timer_ * kMovingSpeed) * kMovingDistance;
		movementDelta_.x = worldTransform_.translation_.x - previousX;
	}
	else if (type_ == GimmickType::kWaterWheelLift)
	{
		wheelAngle_ = std::fmod(wheelAngle_ + kWaterWheelAngularSpeed * GameTime::GetDeltaSeconds(),
			2.0f * std::numbers::pi_v<float>);
		UpdateWaterWheelGeometry(true);
	}

	if (type_ == GimmickType::kFallingRock)
	{
		// 衝突は全ギミックが動いた後にまとめて解決する。
		fallingRockPreviousPosition_ = worldTransform_.translation_;
		if (timer_ < kFallingRockWaitFrames)
		{
			timer_ += deltaFrames;
		}
		else
		{
			worldTransform_.translation_.y -= kFallingRockSpeed * deltaFrames;
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
		cooldown_ = (std::max)(0.0f, cooldown_ - deltaFrames);
	}

	if (type_ == GimmickType::kVanishingCloud)
	{
		if (isCloudHidden_)
		{
			cloudTimer_ += deltaFrames;
			if (cloudTimer_ >= kCloudHiddenFrames)
			{
				// 復活後は、もう一度乗るまで消滅カウントを開始しない。
				isCloudHidden_ = false;
				isCloudTriggered_ = false;
				cloudTimer_ = 0.0f;
			}
		}
		else if (isCloudTriggered_)
		{
			cloudTimer_ += deltaFrames;
			if (cloudTimer_ >= kCloudVisibleFrames)
			{
				isCloudHidden_ = true;
				cloudTimer_ = 0.0f;
			}
		}
	}
	timer_ += deltaFrames;

	UpdateMatrix();
}

void StageGimmick::UpdateAirRushRing()
{
	if (ringState_ == RingState::kReady)
	{
		return;
	}

	// エフェクト中に着地しても、エフェクトが終わるまでは再表示しない。
	if (ringCollector_ && ringCollector_->GetLandingSequence() != ringPickupLandingSequence_)
	{
		ringLandedAfterPickup_ = true;
	}

	if (ringState_ == RingState::kBursting)
	{
		ringEffectSeconds_ = (std::min)(ringEffectSeconds_ + GameTime::GetDeltaSeconds(),
			kRingEffectDurationSeconds);
		UpdateAirRushRingEffect(ringEffectSeconds_ / kRingEffectDurationSeconds);
		if (ringEffectSeconds_ >= kRingEffectDurationSeconds)
		{
			ringState_ = RingState::kHidden;
		}
	}
	else if (ringLandedAfterPickup_)
	{
		ringState_ = RingState::kReady;
		ringCollector_ = nullptr;
		ringLandedAfterPickup_ = false;
		ringEffectSeconds_ = 0.0f;
	}

	UpdateMatrix();
}

void StageGimmick::UpdateAirRushRingEffect(float progress)
{
	const float remaining = 1.0f - progress;
	const float spread = progress * (2.0f - progress);
	// 取得の瞬間から輪郭が一気に広がり、小片より先に到達する。
	ringShockwaveTransform_.translation_ = {startPosition_.x, startPosition_.y,
		startPosition_.z - 0.15f};
	const float waveScale = 0.18f + 1.18f * spread;
	ringShockwaveTransform_.scale_ = {waveScale, waveScale, 0.5f};
	ringShockwaveTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		ringShockwaveTransform_.scale_, ringShockwaveTransform_.rotation_,
		ringShockwaveTransform_.translation_);
	ringShockwaveTransform_.TransferMatrix();
	ringShockwaveColor_.SetColor({0.45f, 1.0f, 1.0f, remaining});
	for (size_t i = 0; i < ringBoxEffectTransforms_.size(); ++i)
	{
		const float angle = 2.0f * std::numbers::pi_v<float> *
			(static_cast<float>(i) + 0.5f) / static_cast<float>(ringBoxEffectTransforms_.size());
		const bool outer = i % 2 == 0;
		const float distance = 0.12f + (outer ? 1.85f : 1.3f) * spread;
		auto& transform = ringBoxEffectTransforms_[i];
		transform.translation_ = {startPosition_.x + std::cos(angle) * distance,
			startPosition_.y + std::sin(angle) * distance,
			startPosition_.z - (outer ? 0.13f : 0.08f)};
		const float scale = 0.01f + (outer ? 0.11f : 0.15f) * remaining;
		transform.scale_ = {scale, scale, scale};
		transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
			transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}
	for (size_t i = 0; i < ringBoardEffectTransforms_.size(); ++i)
	{
		const float angle = 2.0f * std::numbers::pi_v<float> *
			(static_cast<float>(i) + 0.5f) / static_cast<float>(ringBoardEffectTransforms_.size());
		const float distance = 0.08f + (i % 2 == 0 ? 1.1f : 1.55f) * spread;
		auto& transform = ringBoardEffectTransforms_[i];
		transform.translation_ = {startPosition_.x + std::cos(angle) * distance,
			startPosition_.y + std::sin(angle) * distance, startPosition_.z - 0.05f};
		transform.rotation_.z = angle;
		transform.scale_ = {0.01f + 0.19f * remaining,
			0.01f + 0.08f * remaining, 0.03f};
		transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
			transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}
}

void StageGimmick::StartUpdraftContactEffect(const Vector3& position)
{
	updraftContactPosition_ = position;
	updraftContactPosition_.z = startPosition_.z - 0.22f;
	updraftContactSeconds_ = 0.0f;
	UpdateUpdraftEffect();
}

void StageGimmick::StartFountainBurst()
{
	// 衝突したフレームの描画から水柱が見えるよう、短い初期進行を与える。
	fountainBurstSeconds_ = 0.01f;
	fountainBurstActive_ = true;
	SoundManager::GetInstance()->PlaySE(SoundEffect::kFountain);
	UpdateFountainBurst();
}

void StageGimmick::UpdateFountainBurst()
{
	constexpr float kRiseSeconds = 0.10f;
	constexpr float kHoldSeconds = 0.22f;
	const float rise = std::clamp(fountainBurstSeconds_ / kRiseSeconds, 0.0f, 1.0f);
	const float rapidRise = 1.0f - std::pow(1.0f - rise, 3.0f);
	const float fade = std::clamp((fountainBurstSeconds_ - kHoldSeconds) /
		(kFountainBurstDurationSeconds - kHoldSeconds), 0.0f, 1.0f);
	fountainJetHeight_ = kFountainJetMaxHeight * rapidRise * (1.0f - fade);
	const float nozzleY = startPosition_.y + kFountainNozzleOffsetY;
	const float width = 0.16f * (1.0f - 0.35f * fade);

	// squareBoardEffectGrain.obj は幅・高さがそれぞれ2。下端を吐出口に固定する。
	fountainJetTransform_.translation_ = {startPosition_.x,
		nozzleY + fountainJetHeight_ * 0.5f, startPosition_.z - 0.17f};
	fountainJetTransform_.scale_ = {width, fountainJetHeight_ * 0.5f, 0.02f};
	fountainJetTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		fountainJetTransform_.scale_, fountainJetTransform_.rotation_, fountainJetTransform_.translation_);
	fountainJetTransform_.TransferMatrix();
	fountainJetColor_.SetColor({0.18f, 0.75f, 1.0f, 0.9f * (1.0f - fade)});

	fountainCoreTransform_.translation_ = {startPosition_.x,
		nozzleY + fountainJetHeight_ * 0.5f, startPosition_.z - 0.20f};
	fountainCoreTransform_.scale_ = {width * 0.3f, fountainJetHeight_ * 0.5f, 0.02f};
	fountainCoreTransform_.matWorld_ = MyMathUtility::MakeAffineMatrix(
		fountainCoreTransform_.scale_, fountainCoreTransform_.rotation_, fountainCoreTransform_.translation_);
	fountainCoreTransform_.TransferMatrix();
	fountainCoreColor_.SetColor({0.85f, 1.0f, 1.0f, 0.9f * (1.0f - fade)});

	// 水柱の先端から時間差で水滴を飛ばし、上昇後は重力で落とす。
	for (size_t i = 0; i < fountainDropTransforms_.size(); ++i)
	{
		const float releaseTime = 0.035f + static_cast<float>(i / 4) * 0.045f;
		const float age = fountainBurstSeconds_ - releaseTime;
		fountainDropVisible_[i] = age >= 0.0f && age < 0.27f;
		if (!fountainDropVisible_[i])
		{
			continue;
		}
		const float releaseRise = std::clamp(releaseTime / kRiseSeconds, 0.0f, 1.0f);
		const float releaseHeight = kFountainJetMaxHeight *
			(1.0f - std::pow(1.0f - releaseRise, 3.0f));
		const float side = i % 2 == 0 ? -1.0f : 1.0f;
		auto& transform = fountainDropTransforms_[i];
		transform.translation_ = {
			startPosition_.x + (static_cast<float>(i % 4) - 1.5f) * 0.09f +
				side * (0.7f + static_cast<float>(i / 4) * 0.15f) * age,
			nozzleY + releaseHeight + 1.3f * age - 8.0f * age * age,
			startPosition_.z - 0.23f};
		const float size = 0.018f + 0.035f * (1.0f - age / 0.27f);
		transform.scale_ = {size, size, size};
		transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
			transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}
	fountainDropColor_.SetColor({0.7f, 0.95f, 1.0f, 1.0f - fade});
}

void StageGimmick::UpdateUpdraftEffect()
{
	// 4本の細い筋を時間差で下から上へ流す。判定用のworldTransform_は動かさない。
	for (size_t i = 0; i < updraftStreakTransforms_.size(); ++i)
	{
		const float phase = std::fmod(updraftVisualSeconds_ * 0.9f +
			static_cast<float>(i) / static_cast<float>(updraftStreakTransforms_.size()), 1.0f);
		auto& transform = updraftStreakTransforms_[i];
		transform.translation_ = {
			startPosition_.x - 0.3f + static_cast<float>(i) * 0.2f +
				std::sin(updraftVisualSeconds_ * 3.0f + static_cast<float>(i)) * 0.035f,
			startPosition_.y - 0.9f + phase * 1.8f,
			startPosition_.z - 0.15f};
		transform.scale_ = {0.018f, 0.13f + static_cast<float>(i % 2) * 0.04f, 0.02f};
		transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
			transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}

	if (updraftContactSeconds_ >= kUpdraftContactEffectSeconds)
	{
		return;
	}
	const float progress = updraftContactSeconds_ / kUpdraftContactEffectSeconds;
	for (size_t i = 0; i < updraftContactTransforms_.size(); ++i)
	{
		const float angle = 2.0f * std::numbers::pi_v<float> *
			static_cast<float>(i) / static_cast<float>(updraftContactTransforms_.size());
		auto& transform = updraftContactTransforms_[i];
		transform.translation_ = {
			updraftContactPosition_.x + std::cos(angle) * (0.08f + 0.28f * progress),
			updraftContactPosition_.y + std::sin(angle) * 0.12f + 0.45f * progress,
			updraftContactPosition_.z};
		const float size = 0.045f * (1.0f - progress);
		transform.scale_ = {size, size, size};
		transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
			transform.scale_, transform.rotation_, transform.translation_);
		transform.TransferMatrix();
	}
	updraftContactColor_.SetColor({0.65f, 1.0f, 1.0f, 1.0f - progress});
}

void StageGimmick::UpdateWaterWheelGeometry(bool recordMovement)
{
	for (size_t i = 0; i < wheelPlatformTransforms_.size(); ++i)
	{
		const float angle = wheelAngle_ + static_cast<float>(i) *
			(std::numbers::pi_v<float> / 2.0f);
		const Vector3 previous = wheelPlatformTransforms_[i].translation_;
		const float offsetX = std::cos(angle) * kWaterWheelRadius;
		const float offsetY = std::sin(angle) * kWaterWheelRadius;
		wheelPlatformTransforms_[i].translation_ =
			{startPosition_.x + offsetX, startPosition_.y + offsetY, startPosition_.z - 0.12f};
		wheelPlatformDeltas_[i] = recordMovement
			? Vector3{wheelPlatformTransforms_[i].translation_.x - previous.x,
				wheelPlatformTransforms_[i].translation_.y - previous.y, 0.0f}
			: Vector3{};
		// 支柱は回転するが、プレイヤーが乗る台は水平を保つ。
		wheelSpokeTransforms_[i].translation_ =
			{startPosition_.x + offsetX * 0.5f,
				startPosition_.y + offsetY * 0.5f, startPosition_.z + 0.18f};
		wheelSpokeTransforms_[i].rotation_.z = angle;
	}
	wheelShaftTransform_.rotation_.z = wheelAngle_;
}

void StageGimmick::Draw()
{
	if (type_ == GimmickType::kAirRushRing)
	{
		if (isActive_ && ringState_ == RingState::kReady)
		{
			model_->Draw(modelWorldTransform_, *camera_, &airRushRingColor_);
		}
		else if (isActive_ && ringState_ == RingState::kBursting)
		{
			model_->Draw(ringShockwaveTransform_, *camera_, &ringShockwaveColor_);
			for (auto& transform : ringBoxEffectTransforms_)
			{
				ringBoxEffectModel_->Draw(transform, *camera_, &ringBoxEffectColor_);
			}
			for (auto& transform : ringBoardEffectTransforms_)
			{
				ringBoardEffectModel_->Draw(transform, *camera_, &ringBoardEffectColor_);
			}
		}
		return;
	}
	if (type_ == GimmickType::kAirRushSwitch)
	{
		const bool active = switchLinkState_ && switchLinkState_->IsActive(linkId_);
		airRushSwitchColor_.SetColor(active
			? Vector4{0.95f, 1.0f, 0.25f, 1.0f}
			: Vector4{0.15f, 0.90f, 1.0f, 1.0f});
		model_->Draw(modelWorldTransform_, *camera_, &airRushSwitchColor_);
		return;
	}

	if (IsActive())
	{
		if (type_ == GimmickType::kWaterWheelLift)
		{
			for (auto& spoke : wheelSpokeTransforms_)
			{
				model_->Draw(spoke, *camera_);
			}
			wheelShaftModel_->Draw(wheelShaftTransform_, *camera_, &wheelShaftColor_);
			for (auto& platform : wheelPlatformTransforms_)
			{
				model_->Draw(platform, *camera_);
			}
			return;
		}
		if (type_ == GimmickType::kStoneBridge)
		{
			// リンクごとの残り時間が1秒以下なら点滅し、消滅直前は薄くする。
			const float remaining = switchLinkState_->GetRemainingFrames(linkId_);
			const bool blinking = remaining <= kStoneBridgeBlinkFrames;
			const bool bright = static_cast<int>(std::floor(remaining / kStoneBridgeBlinkIntervalFrames)) % 2 != 0;
			stoneBridgeColor_.SetColor({1.0f, 1.0f, 1.0f, !blinking || bright ? 1.0f : 0.2f});
			model_->Draw(modelWorldTransform_, *camera_, &stoneBridgeColor_);
		}
		else if (type_ == GimmickType::kVanishingCloud)
		{
			// 乗ってからの消滅カウントの最後だけ徐々に透明にする。
			const float alpha = isCloudTriggered_
				? std::clamp((kCloudVisibleFrames - cloudTimer_) / kCloudFadeFrames, 0.0f, 1.0f)
				: 1.0f;
			vanishingCloudColor_.SetColor({1.0f, 1.0f, 1.0f, alpha});
			model_->Draw(modelWorldTransform_, *camera_, &vanishingCloudColor_);
		}
		else
		{
			model_->Draw(modelWorldTransform_, *camera_);
		}
		if (type_ == GimmickType::kFountainJump && fountainBurstActive_)
		{
			ringBoardEffectModel_->Draw(fountainJetTransform_, *camera_, &fountainJetColor_);
			ringBoardEffectModel_->Draw(fountainCoreTransform_, *camera_, &fountainCoreColor_);
			for (size_t i = 0; i < fountainDropTransforms_.size(); ++i)
			{
				if (fountainDropVisible_[i])
				{
					ringBoxEffectModel_->Draw(fountainDropTransforms_[i], *camera_, &fountainDropColor_);
				}
			}
		}
		if (type_ == GimmickType::kUpdraft)
		{
			for (size_t i = 0; i < updraftStreakTransforms_.size(); ++i)
			{
				updraftStreakColor_.SetColor(i % 2 == 0
					? Vector4{0.35f, 0.95f, 1.0f, 0.85f}
					: Vector4{0.9f, 1.0f, 1.0f, 0.85f});
				ringBoardEffectModel_->Draw(updraftStreakTransforms_[i], *camera_, &updraftStreakColor_);
			}
			if (updraftContactSeconds_ < kUpdraftContactEffectSeconds)
			{
				for (auto& transform : updraftContactTransforms_)
				{
					ringBoxEffectModel_->Draw(transform, *camera_, &updraftContactColor_);
				}
			}
		}

		// 雷は危険判定が有効な落雷中だけ描画する。
		if (type_ == GimmickType::kThunderCloud && lightningModel_ && IsThunderStriking())
		{
			lightningModel_->Draw(lightningWorldTransform_, *camera_);
		}
	}
}

AABB StageGimmick::GetDrawAABB() const
{
	if (type_ == GimmickType::kFountainJump && fountainBurstActive_)
	{
		// 土台が画面外でも水柱が見えていれば描画する。衝突判定には使わない。
		return {{startPosition_.x - 0.8f, startPosition_.y - 0.5f, startPosition_.z - 0.5f},
			{startPosition_.x + 0.8f,
				startPosition_.y + kFountainNozzleOffsetY + kFountainJetMaxHeight + 0.2f,
				startPosition_.z + 0.5f}};
	}
	return GetAABB();
}

AABB StageGimmick::GetAABB() const
{
	if (type_ == GimmickType::kAirRushRing && ringState_ == RingState::kBursting)
	{
		// 描画カリングで外側へ飛ぶ小片を途中で消さない。
		constexpr float kBurstExtent = 2.3f;
		return {{startPosition_.x - kBurstExtent, startPosition_.y - kBurstExtent,
			startPosition_.z - 0.5f},
			{startPosition_.x + kBurstExtent, startPosition_.y + kBurstExtent,
			startPosition_.z + 0.5f}};
	}
	if (type_ == GimmickType::kWaterWheelLift)
	{
		// 回転する全足場を含む範囲。詳細な接地は各足場のAABBで行う。
		const float extentX = kWaterWheelRadius + kWaterWheelPlatformWidth * 0.5f;
		const float extentY = kWaterWheelRadius + kWaterWheelPlatformHeight * 0.5f;
		return {{startPosition_.x - extentX, startPosition_.y - extentY, startPosition_.z - 0.62f},
			{startPosition_.x + extentX, startPosition_.y + extentY, startPosition_.z + 0.5f}};
	}
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

AABB StageGimmick::GetWaterWheelPlatformAABB(size_t index) const
{
	const Vector3& position = wheelPlatformTransforms_[index].translation_;
	return {{position.x - kWaterWheelPlatformWidth * 0.5f,
		position.y - kWaterWheelPlatformHeight * 0.5f, position.z - 0.5f},
		{position.x + kWaterWheelPlatformWidth * 0.5f,
		position.y + kWaterWheelPlatformHeight * 0.5f, position.z + 0.5f}};
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
			StartFountainBurst();
			cooldown_ = kFountainCooldownFrames;
		}
		break;

	case GimmickType::kUpdraft:
		// 攻撃判定だけが触れた場合は反応せず、本体が気流内にいる間だけ一定速度で上昇する。
		if (MyMathUtility::IsCollision(player->GetAABB(), GetAABB()))
		{
			if (!updraftTouchedLastFrame_ && !updraftTouchedThisFrame_)
			{
				StartUpdraftContactEffect(player->GetWorldPosition());
			}
			updraftTouchedThisFrame_ = true;
			player->ApplyUpdraft(kUpdraftRiseSpeed);
		}
		break;

	case GimmickType::kFallingRock:
		// 攻撃範囲だけが重なった場合は被弾させず、本体との接触だけを判定する。
		if (MyMathUtility::IsCollision(player->GetAABB(), GetAABB()))
		{
			player->OnHazardCollision(worldTransform_.translation_.x);
		}
		break;

	case GimmickType::kVine:
	{
		// 突進の攻撃範囲だけでは掴まず、本体が触れている時の入力に限る。
		if (!player->WantsToGrabVine() ||
			!MyMathUtility::IsCollision(player->GetAABB(), GetAABB()))
		{
			break;
		}
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
		// 従来どおり攻撃で押せる。リンクした石橋だけを一定時間表示する。
		if (player->IsAttack() && switchLinkState_)
		{
			if (!linkId_.empty() && !switchLinkState_->IsActive(linkId_))
			{
				SoundManager::GetInstance()->PlaySE(SoundEffect::kSwitch);
			}
			switchLinkState_->Activate(linkId_, kOreSwitchActiveFrames);
		}
		break;

	case GimmickType::kAirRushSwitch:
		// 通常接触や旧攻撃では起動しない。突進の攻撃判定が触れた時だけ起動。
		if (player->IsJumpRushing() && switchLinkState_ &&
			MyMathUtility::IsCollision(player->GetAttackAABB(), GetAABB()))
		{
			if (!linkId_.empty() && !switchLinkState_->IsActive(linkId_))
			{
				SoundManager::GetInstance()->PlaySE(SoundEffect::kSwitch);
			}
			switchLinkState_->Activate(linkId_, kOreSwitchActiveFrames);
		}
		break;

	case GimmickType::kThunderCloud:
		if (IsThunderStriking())
		{
			player->OnHazardCollision(worldTransform_.translation_.x);
		}
		break;

	case GimmickType::kAirRushRing:
		// 突進の残数に関係なく、空中で本体が輪に触れたら取得する。
		if (MyMathUtility::IsCollision(player->GetAABB(), GetAABB()) &&
			player->TryCollectAirRushRing())
		{
			ringState_ = RingState::kBursting;
			ringCollector_ = player;
			ringPickupLandingSequence_ = player->GetLandingSequence();
			ringEffectSeconds_ = 0.0f;
			ringLandedAfterPickup_ = false;
			UpdateAirRushRingEffect(0.0f);
		}
		break;

	case GimmickType::kWaterWheelLift:
		for (size_t i = 0; i < wheelPlatformTransforms_.size(); ++i)
		{
			if (player->TryLandOnOneWayGimmick(GetWaterWheelPlatformAABB(i),
				wheelPlatformDeltas_[i].x, wheelPlatformDeltas_[i].y))
			{
				break;
			}
		}
		break;

	case GimmickType::kMovingPlatform:
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

std::vector<AABB> StageGimmick::GetFallingRockCollisionBoxes() const
{
	if (!IsActive())
	{
		return {};
	}
	if (type_ == GimmickType::kWaterWheelLift)
	{
		std::vector<AABB> boxes;
		boxes.reserve(wheelPlatformTransforms_.size() + 1);
		for (size_t i = 0; i < wheelPlatformTransforms_.size(); ++i)
		{
			boxes.push_back(GetWaterWheelPlatformAABB(i));
		}
		const Vector3& shaft = wheelShaftTransform_.translation_;
		const Vector3& scale = wheelShaftTransform_.scale_;
		boxes.push_back({{shaft.x - scale.x * 0.5f, shaft.y - scale.y * 0.5f,
			shaft.z - scale.z * 0.5f},
			{shaft.x + scale.x * 0.5f, shaft.y + scale.y * 0.5f,
			shaft.z + scale.z * 0.5f}});
		return boxes;
	}
	if (type_ == GimmickType::kThunderCloud)
	{
		// GetAABB() は雷柱の危険範囲を含むため、雲本体だけに当てる。
		const Vector3& position = worldTransform_.translation_;
		return {{{position.x - 0.5f, position.y - 0.5f, position.z - 0.5f},
			{position.x + 0.5f, position.y + 0.5f, position.z + 0.5f}}};
	}
	switch (type_)
	{
	case GimmickType::kFountainJump:
	case GimmickType::kMovingPlatform:
	case GimmickType::kBounceLeaf:
	case GimmickType::kVine:
	case GimmickType::kOreSwitch:
	case GimmickType::kStoneBridge:
	case GimmickType::kVanishingCloud:
		return {GetAABB()};
	default:
		// 気流・回復リング・空中スイッチなどは落石が通り抜ける。
		return {};
	}
}

void StageGimmick::BreakFallingRock()
{
	worldTransform_.translation_ = startPosition_;
	fallingRockPreviousPosition_ = startPosition_;
	timer_ = 0.0f;
	UpdateMatrix();
}

void StageGimmick::ResolveFallingRockCollisions(const std::list<BaseGimmick*>& gimmicks)
{
	if (type_ != GimmickType::kFallingRock || !IsActive())
	{
		return;
	}

	fallingRockHasLanding_ = fallingRockHasMapLanding_;
	fallingRockImpactPosition_ = fallingRockMapImpactPosition_;
	float highestLanding = fallingRockHasLanding_
		? fallingRockImpactPosition_.y : (std::numeric_limits<float>::lowest)();
	const AABB currentBox = GetAABB();
	AABB sweptBox = currentBox;
	const float previousBottom = fallingRockPreviousPosition_.y - worldTransform_.scale_.y * 0.5f;
	sweptBox.min.y = (std::min)(previousBottom, currentBox.min.y);
	sweptBox.max.y = (std::max)(fallingRockPreviousPosition_.y +
		worldTransform_.scale_.y * 0.5f, currentBox.max.y);
	const float startBottom = startPosition_.y - worldTransform_.scale_.y * 0.5f;
	bool hitGimmick = false;

	for (BaseGimmick* gimmick : gimmicks)
	{
		if (gimmick == this)
		{
			continue;
		}
		std::vector<AABB> collisionBoxes;
		if (const auto* other = dynamic_cast<const StageGimmick*>(gimmick))
		{
			collisionBoxes = other->GetFallingRockCollisionBoxes();
		}
		else if (gimmick->IsActive() && gimmick->IsSolid())
		{
			collisionBoxes.push_back(gimmick->GetAABB());
		}
		for (const AABB& box : collisionBoxes)
		{
			const bool horizontalOverlap = currentBox.min.x <= box.max.x &&
				currentBox.max.x >= box.min.x && currentBox.min.z <= box.max.z &&
				currentBox.max.z >= box.min.z;
			if (!horizontalOverlap)
			{
				continue;
			}
			if (box.max.y <= startBottom && box.max.y > highestLanding)
			{
				highestLanding = box.max.y;
				fallingRockImpactPosition_ = {startPosition_.x, box.max.y, startPosition_.z};
				fallingRockHasLanding_ = true;
			}
			if (timer_ >= kFallingRockWaitFrames &&
				MyMathUtility::IsCollision(sweptBox, box))
			{
				hitGimmick = true;
			}
		}
	}

	const bool hitMap = fallingRockHasMapLanding_ &&
		timer_ >= kFallingRockWaitFrames &&
		previousBottom >= fallingRockMapImpactPosition_.y &&
		currentBox.min.y <= fallingRockMapImpactPosition_.y;
	if (hitGimmick || hitMap)
	{
		BreakFallingRock();
	}
}

bool StageGimmick::GetFallingRockShadow(Vector3& impactPosition, float& strength) const
{
	if (type_ != GimmickType::kFallingRock || !fallingRockHasLanding_ ||
		timer_ < kFallingRockWaitFrames - kFallingRockWarningFrames)
	{
		return false;
	}
	impactPosition = fallingRockImpactPosition_;
	strength = std::clamp((timer_ - (kFallingRockWaitFrames - kFallingRockWarningFrames)) /
		kFallingRockWarningFrames, 0.0f, 1.0f);
	return true;
}

bool StageGimmick::IsActive() const
{
	if (!isActive_)
	{
		return false;
	}

	if (type_ == GimmickType::kStoneBridge)
	{
		return switchLinkState_ && switchLinkState_->IsActive(linkId_);
	}

	if (type_ == GimmickType::kVanishingCloud)
	{
		return !isCloudHidden_;
	}
	if (type_ == GimmickType::kAirRushRing)
	{
		return ringState_ == RingState::kReady;
	}

	return true;
}

bool StageGimmick::IsThunderStriking() const
{
	const float cycle = kThunderWaitFrames + kThunderStrikeFrames;
	return std::fmod(timer_, cycle) >= kThunderWaitFrames;
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
	if (type_ == GimmickType::kFallingRock &&
		timer_ >= kFallingRockWaitFrames - kFallingRockWarningFrames &&
		timer_ < kFallingRockWaitFrames)
	{
		const float warning = (timer_ - (kFallingRockWaitFrames - kFallingRockWarningFrames)) /
			kFallingRockWarningFrames;
		modelWorldTransform_.translation_.x += std::sin(timer_ * 2.4f) * 0.07f * warning;
	}
	if (type_ == GimmickType::kUpdraft)
	{
		// 見た目だけを軽く揺らす。接触範囲は固定したままにする。
		modelWorldTransform_.translation_.x += std::sin(updraftVisualSeconds_ * 4.0f) * 0.035f;
	}
	modelWorldTransform_.rotation_ = worldTransform_.rotation_;
	modelWorldTransform_.rotation_.z += modelRotationOffsetZ_;
	if (type_ == GimmickType::kUpdraft)
	{
		modelWorldTransform_.rotation_.z += std::sin(updraftVisualSeconds_ * 3.0f) * 0.025f;
	}
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

	if (type_ == GimmickType::kWaterWheelLift)
	{
		auto updateWheelTransform = [](WorldTransform& transform)
		{
			transform.matWorld_ = MyMathUtility::MakeAffineMatrix(
				transform.scale_, transform.rotation_, transform.translation_);
			transform.TransferMatrix();
		};
		updateWheelTransform(wheelShaftTransform_);
		for (size_t i = 0; i < wheelPlatformTransforms_.size(); ++i)
		{
			updateWheelTransform(wheelPlatformTransforms_[i]);
			updateWheelTransform(wheelSpokeTransforms_[i]);
		}
	}
}
