#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"
#include "SwitchLinkState.h"
#include <array>
#include <cstddef>
#include <list>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class MapChipField;

// 旧CSVとTiledの両方で使うギミックの種類
enum class GimmickType : uint8_t
{
	kFountainJump = 0,
	kMovingPlatform = 1,
	kWaterWheelLift = 2,
	kUpdraft = 3,
	kBounceLeaf = 4,
	kFallingRock = 5,
	kVine = 6,
	kOreSwitch = 7,
	kStoneBridge = 8,
	kVanishingCloud = 9,
	kThunderCloud = 10,
	kAirRushRing = 11,
	kAirRushSwitch = 12,
};

// TiledのgimmickType文字列を旧ステージと同じ種類へ変換する。
inline std::optional<GimmickType> ParseGimmickType(std::string_view name)
{
	constexpr std::array<std::pair<std::string_view, GimmickType>, 13> names{{
		{"FountainJump", GimmickType::kFountainJump},
		{"MovingPlatform", GimmickType::kMovingPlatform},
		{"WaterWheelLift", GimmickType::kWaterWheelLift},
		{"Updraft", GimmickType::kUpdraft},
		{"BounceLeaf", GimmickType::kBounceLeaf},
		{"FallingRock", GimmickType::kFallingRock},
		{"Vine", GimmickType::kVine},
		{"OreSwitch", GimmickType::kOreSwitch},
		{"StoneBridge", GimmickType::kStoneBridge},
		{"VanishingCloud", GimmickType::kVanishingCloud},
		{"ThunderCloud", GimmickType::kThunderCloud},
		{"AirRushRing", GimmickType::kAirRushRing},
		{"AirRushSwitch", GimmickType::kAirRushSwitch},
	}};
	for (const auto& [typeName, type] : names)
	{
		if (typeName == name)
		{
			return type;
		}
	}
	return std::nullopt;
}

class StageGimmick : public BaseGimmick
{
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* lightningModel,
		KamataEngine::Model* wheelShaftModel,
		KamataEngine::Model* ringBoxEffectModel, KamataEngine::Model* ringBoardEffectModel,
		KamataEngine::Camera* camera,
		MapChipField* mapChipField, SwitchLinkState* switchLinkState,
		const KamataEngine::Vector3& position, GimmickType type, const std::string& linkId);

	void Update() override;
	void Draw() override;
	AABB GetAABB() const override;
	AABB GetDrawAABB() const override;
	void OnPlayerCollision(Player* player) override;
	bool IsActive() const override;
	bool IsSolid() const override;
	bool GetFallingRockShadow(KamataEngine::Vector3& impactPosition, float& strength) const;
	// 全ギミックの更新後に呼び、落石の衝突と予告影の着地点を更新する。
	void ResolveFallingRockCollisions(const std::list<BaseGimmick*>& gimmicks);

private:
	void UpdateMatrix();
	void UpdateAirRushRing();
	void UpdateAirRushRingEffect(float progress);
	void UpdateUpdraftEffect();
	void StartUpdraftContactEffect(const KamataEngine::Vector3& position);
	void StartFountainBurst();
	void UpdateFountainBurst();
	void UpdateWaterWheelGeometry(bool recordMovement);
	AABB GetWaterWheelPlatformAABB(size_t index) const;
	std::vector<AABB> GetFallingRockCollisionBoxes() const;
	void BreakFallingRock();

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* lightningModel_ = nullptr;
	KamataEngine::Model* wheelShaftModel_ = nullptr;
	KamataEngine::Model* ringBoxEffectModel_ = nullptr;
	KamataEngine::Model* ringBoardEffectModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	// 当たり判定用と見た目用の変換を分離する。
	// Blenderで原点が底・上端などに置かれたモデルでも、判定を変えずに位置を補正できる。
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform modelWorldTransform_;
	KamataEngine::WorldTransform lightningWorldTransform_;
	KamataEngine::WorldTransform wheelShaftTransform_;
	KamataEngine::ObjectColor wheelShaftColor_;
	KamataEngine::ObjectColor airRushRingColor_;
	KamataEngine::ObjectColor airRushSwitchColor_;
	KamataEngine::ObjectColor stoneBridgeColor_;
	KamataEngine::ObjectColor vanishingCloudColor_;
	KamataEngine::ObjectColor ringBoxEffectColor_;
	KamataEngine::ObjectColor ringBoardEffectColor_;
	KamataEngine::ObjectColor ringShockwaveColor_;
	KamataEngine::ObjectColor updraftStreakColor_;
	KamataEngine::ObjectColor updraftContactColor_;
	KamataEngine::ObjectColor fountainJetColor_;
	KamataEngine::ObjectColor fountainCoreColor_;
	KamataEngine::ObjectColor fountainDropColor_;
	KamataEngine::WorldTransform fountainJetTransform_;
	KamataEngine::WorldTransform fountainCoreTransform_;
	std::array<KamataEngine::WorldTransform, 12> fountainDropTransforms_;
	std::array<bool, 12> fountainDropVisible_{};
	float fountainBurstSeconds_ = 0.0f;
	float fountainJetHeight_ = 0.0f;
	bool fountainBurstActive_ = false;
	std::array<KamataEngine::WorldTransform, 4> updraftStreakTransforms_;
	std::array<KamataEngine::WorldTransform, 8> updraftContactTransforms_;
	KamataEngine::Vector3 updraftContactPosition_{};
	float updraftVisualSeconds_ = 0.0f;
	float updraftContactSeconds_ = 0.0f;
	bool updraftTouchedThisFrame_ = false;
	bool updraftTouchedLastFrame_ = false;
	std::array<KamataEngine::WorldTransform, 16> ringBoxEffectTransforms_;
	std::array<KamataEngine::WorldTransform, 8> ringBoardEffectTransforms_;
	KamataEngine::WorldTransform ringShockwaveTransform_;
	std::array<KamataEngine::WorldTransform, 4> wheelPlatformTransforms_;
	std::array<KamataEngine::WorldTransform, 4> wheelSpokeTransforms_;
	std::array<KamataEngine::Vector3, 4> wheelPlatformDeltas_{};
	float wheelAngle_ = 0.0f;
	KamataEngine::Vector3 modelOffset_{};
	float modelRotationOffsetZ_ = 0.0f;
	Player* hangingPlayer_ = nullptr;
	KamataEngine::Vector3 startPosition_{};
	KamataEngine::Vector3 movementDelta_{};
	KamataEngine::Vector3 fallingRockImpactPosition_{};
	bool fallingRockHasLanding_ = false;
	KamataEngine::Vector3 fallingRockMapImpactPosition_{};
	bool fallingRockHasMapLanding_ = false;
	KamataEngine::Vector3 fallingRockPreviousPosition_{};
	SwitchLinkState* switchLinkState_ = nullptr;
	std::string linkId_;
	GimmickType type_ = GimmickType::kFountainJump;

	bool isActive_ = true;
	float timer_ = 0.0f;
	float cooldown_ = 0.0f;
	// 消える雲は、上面へ乗った時だけ消滅までのカウントを開始する。
	bool isCloudTriggered_ = false;
	bool isCloudHidden_ = false;
	float cloudTimer_ = 0.0f;
	enum class RingState : uint8_t { kReady, kBursting, kHidden };
	RingState ringState_ = RingState::kReady;
	Player* ringCollector_ = nullptr;
	float ringEffectSeconds_ = 0.0f;
	bool ringLandedAfterPickup_ = false;

	static inline const float kMovingDistance = 2.0f;
	static inline const float kMovingSpeed = 0.03f;
	static inline const float kFallingRockWaitFrames = 45.0f;
	static inline const float kFallingRockWarningFrames = 30.0f;
	static inline const float kFallingRockSpeed = 0.12f;
	static inline const float kWaterWheelRadius = 2.0f;
	static inline const float kWaterWheelAngularSpeed = 1.0f; // rad/s
	static inline const float kWaterWheelPlatformWidth = 1.4f;
	static inline const float kWaterWheelPlatformHeight = 0.35f;
	static inline const float kFountainCooldownFrames = 20.0f;
	static inline const float kFountainBurstDurationSeconds = 0.40f;
	static inline const float kFountainJetMaxHeight = 2.8f;
	static inline const float kFountainNozzleOffsetY = -0.05f;
	static inline const float kUpdraftRiseSpeed = 0.24f;
	static inline const float kUpdraftContactEffectSeconds = 0.35f;
	static inline const float kCloudVisibleFrames = 120.0f;
	static inline const float kCloudHiddenFrames = 90.0f;
	static inline const float kCloudFadeFrames = 45.0f;
	static inline const float kStoneBridgeBlinkFrames = 60.0f;
	static inline const float kStoneBridgeBlinkIntervalFrames = 8.0f;
	static inline const float kThunderWaitFrames = 120.0f;
	// 見てから回避できるよう、落雷中の表示・危険時間を1秒間にする。
	static inline const float kThunderStrikeFrames = 60.0f;
	static inline const float kOreSwitchActiveFrames = 300.0f;
	static inline const float kRingEffectDurationSeconds = 0.45f;

	bool IsThunderStriking() const;
};
