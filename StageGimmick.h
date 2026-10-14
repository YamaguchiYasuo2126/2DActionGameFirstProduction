#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"

class MapChipField;

// CSVに対応するギミックの種類
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
};

class StageGimmick : public BaseGimmick
{
public:
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* lightningModel, KamataEngine::Camera* camera,
		MapChipField* mapChipField, bool* isOreSwitchActive, float* oreSwitchTimer,
		const KamataEngine::Vector3& position, GimmickType type);

	void Update() override;
	void Draw() override;
	AABB GetAABB() const override;
	void OnPlayerCollision(Player* player) override;
	bool IsActive() const override;
	bool IsSolid() const override;

private:
	void UpdateMatrix();

	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Model* lightningModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	MapChipField* mapChipField_ = nullptr;
	// 当たり判定用と見た目用の変換を分離する。
	// Blenderで原点が底・上端などに置かれたモデルでも、判定を変えずに位置を補正できる。
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::WorldTransform modelWorldTransform_;
	KamataEngine::WorldTransform lightningWorldTransform_;
	KamataEngine::Vector3 modelOffset_{};
	float modelRotationOffsetZ_ = 0.0f;
	Player* hangingPlayer_ = nullptr;
	KamataEngine::Vector3 startPosition_{};
	KamataEngine::Vector3 movementDelta_{};
	bool* isOreSwitchActive_ = nullptr;
	float* oreSwitchTimer_ = nullptr;
	GimmickType type_ = GimmickType::kFountainJump;

	bool isActive_ = true;
	float timer_ = 0.0f;
	float cooldown_ = 0.0f;
	// 消える雲は、上面へ乗った時だけ消滅までのカウントを開始する。
	bool isCloudTriggered_ = false;
	bool isCloudHidden_ = false;
	float cloudTimer_ = 0.0f;

	static inline const float kMovingDistance = 2.0f;
	static inline const float kMovingSpeed = 0.03f;
	static inline const float kFallingRockWaitFrames = 45.0f;
	static inline const float kFallingRockSpeed = 0.12f;
	static inline const float kWaterWheelRadius = 2.0f;
	static inline const float kWaterWheelSpeed = 0.035f;
	static inline const float kFountainCooldownFrames = 20.0f;
	static inline const float kCloudVisibleFrames = 120.0f;
	static inline const float kCloudHiddenFrames = 90.0f;
	static inline const float kThunderWaitFrames = 120.0f;
	// 見てから回避できるよう、落雷中の表示・危険時間を1秒間にする。
	static inline const float kThunderStrikeFrames = 60.0f;
	static inline const float kOreSwitchActiveFrames = 300.0f;

	bool IsThunderStriking() const;
};
