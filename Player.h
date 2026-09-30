#pragma once
#include "KamataEngine.h"
#include "AABB.h"
#include <numbers>

class MapChipField;

class Enemy;

class ShieldEnemy;

class BaseEnemy;

// マップとの当たり判定情報
struct CollisionMapInfo {
	bool isCeiling = false; // 天井衝突フラグ
	bool isLanding = false;	// 着地フラグ
	bool hitWall = false;	// 壁接触フラグ
	KamataEngine::Vector3 move; // 移動量
};

class Player {

public:
	// 固体ギミックに接触した面
	enum class GimmickHitSide
	{
		kNone,
		kTop,
		kBottom,
		kLeft,
		kRight,
	};

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* modelAttack, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	void Update();

	/// <summary>
	/// 描画
	/// </summary>
	void Draw();

	// ワールドトランスフォームのゲッター
	const KamataEngine::WorldTransform& GetWorldTransform() const { return worldTransform_; }

	// 自キャラの速度を取得するためのgetterを用意する
	const KamataEngine::Vector3& GetVelocity() const { return velocity_; }

	// 外部からポインタをセットするためのsetterを用意する
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

	// ワールド座標を取得
	KamataEngine::Vector3 GetWorldPosition();

	// デスフラグのgetter
	bool IsDead() const { return isDead_; }
	bool ConsumeRespawnRequest();
	int32_t GetHp() const { return hp_; }
	int32_t GetMaxHp() const { return kMaxHp; }

	// 攻撃中かどうかを判定する関数
	bool IsAttack() const { return behavior_ == Behavior::kAttack; }

	// タイトル画面等で向きを強制変更するためのセッターを追加
	void SetRotationY(float rotationY)
	{
		worldTransform_.rotation_.y = rotationY;
		UpdateMatrix(); // 行列も一緒に更新する
	}

	// AABBを取得
	AABB GetAABB();
	AABB GetPreviousAABB() const;

	// 上方向攻撃用のAABBを取得
	AABB GetAttackAABB();
	
	// 衝突応答
	void OnCollision(const BaseEnemy* enemy);
	void OnHazardCollision();
	void TakeDamage(int32_t damage);
	void Respawn(const KamataEngine::Vector3& position, bool restoreHp = true);

	// ツルへつかまる・ツルから離れるための処理
	void AttachToVine(const KamataEngine::Vector3& anchorPosition);
	bool IsHangingFromVine(const KamataEngine::Vector3& anchorPosition) const;
	float GetVineAngle() const { return vineAngle_; }
	void LaunchFromFountain(float launchVelocity);
	
	// 左右
	enum class LRDirection {
		kRight,
		kLeft,
	};

	// プレイヤーの向きを取得するゲッターを追加
	LRDirection GetLRDirection() const { return lrDirection_; }

	// 敵からノックバックを要求する関数
	void RequestKnockback() { isKnockbackRequested_ = true; }

	// 敵に攻撃を当てたとき、空中行動を回復する
	void OnAttackHit();
	
	bool IsJumpCharging() const { return isJumpCharging_; }

	float GetJumpChargeRate() const { return static_cast<float>(jumpChargeFrame_) / kChargeMaxFrame; }


	float GetChargedJumpAngle() const { return chargedJumpAngle_; }

	// ギミックからプレイヤーへ加える外力
	void AddExternalVelocity(const KamataEngine::Vector3& velocity)
	{
		velocity_.x += velocity.x;
		velocity_.y += velocity.y;
		velocity_.z += velocity.z;
	}

	// 移動足場・崩れる足場への着地処理
	void LandOnGimmick(float topY, float horizontalMove);

	void BounceOnGimmick(float topY, float bounceVelocity);

	// 移動前後の位置から、固体ギミックとの衝突面を解決する
	GimmickHitSide ResolveSolidGimmickCollision(const AABB& gimmickAABB, float horizontalMove, float verticalMove = 0.0f);

private:
	/// <summary>
	/// 行列の更新と転送
	/// </summary>
	void UpdateMatrix();

	/// <summary>
	/// 移動入力処理
	/// </summary>
	void MoveInput();
	void UpdateHanging();

	/// <summary>
	/// マップ衝突判定
	/// </summary>
	void CheckMapCollision(CollisionMapInfo& info);

	// マップ衝突判定上下左右
	void CheckMapCollisionUp(CollisionMapInfo& info);
	void CheckMapCollisionDown(CollisionMapInfo& info);
	void CheckMapCollisionRight(CollisionMapInfo& info);
	void CheckMapCollisionLeft(CollisionMapInfo& info);

	// 判定結果を反映して移動させる
	void JudgmentResultReflectionMove(const CollisionMapInfo& info);

	// 天井に接触している場合の処理
	void ContactToCeiling(const CollisionMapInfo& info);

	// 壁に接触している場合の処理
	void ContactToWall(const CollisionMapInfo& info);
	
	// 設置状態の切り替え処理
	void ContactToGroundStateSwitch(const CollisionMapInfo& info);

	// 通常行動初期化
	void BehaviorRootInitialize();

	// 通常行動更新
	void BehaviorRootUpdate();

	// 攻撃行動初期化
	void BehaviorAttackInitialize();

	// 攻撃行動更新
	void BehaviorAttackUpdate();

	// ノックバック行動初期化・更新の関数宣言
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();

private:

	// ノックバックの要求を記憶するフラグ
	bool isKnockbackRequested_ = false;

	// ノックバックの経過時間カウンター
	float knockbackParameter_ = 0.0f;

	// ノックバックの各フェーズの時間
	static inline const float kKnockbackBlowTime = 10.0f;    // 弾き飛ばされる時間
	static inline const float kKnockbackRecoverTime = 20.0f; // 体勢を立て直す時間

	LRDirection lrDirection_ = LRDirection::kRight;

	// 角
	enum Corner {
		kRightBottom,	// 右下
		kLeftBottom,	// 左下
		kRightTop,		// 右上
		kLeftTop,		// 左上

		kNumCorner		// 要素数
	};

	// 振るまい
	enum class Behavior
	{
		kUnknown, // 変更リクエストがない状態
		kRoot, // 通常状態
		kAttack, // 攻撃中
		kKnockback, // ノックバック状態
	};

	// 振るまい
	Behavior behavior_ = Behavior::kRoot;

	Behavior behaviorRequest_ = Behavior::kUnknown;

	// 攻撃フェーズ
	enum class AttackPhase
	{
		kCharge,		// 溜め
		kRush,			// 突進
		kReverberation, // 余韻
	};

	// 現在の攻撃フェーズ
	AttackPhase attackPhase_;

	// 指定した角の座標を得る関数
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 previousPosition_{};

	// モデル
	KamataEngine::Model* model_ = nullptr;
	KamataEngine::Vector3 velocity_ = {};

	// 攻撃エフェクト用のモデルデータ
	KamataEngine::Model* modelAttack_ = nullptr;
	KamataEngine::WorldTransform worldTransformAttack_;

	// 加速度
	static inline const float kAcceleration = 0.01f;
	// 速度減衰率
	static inline const float kAttenuation = 0.1f;
	// 速度制限値
	static inline const float kLimitRunSpeed = 0.1f;

	// 着地時の速度減衰率
	static inline const float kAttenuationLanding = 0.01f;
	static inline const float kAttenuationWall = 0.01f;
	// カメラ
	KamataEngine::Camera* camera_ = nullptr;

	// 旋回開始時の角度
	float turnFirstRotationY_ = 0.0f;
	// 旋回タイマー
	float turnTimer_ = 0.0f;
	// 旋回時間<秒>
	static inline const float kTimeTurn = 0.3f;

	// 設置状態フラグ
	bool onGround_ = true;

	// 重力加速度(下方向)
	static inline const float kGravityAcceleration = 0.01f;
	// 最大落下速度(下方向)
	static inline const float kLimitFallSpeed = 0.2f;
	// ジャンプ初速(上方向)
	static inline const float kJumpAcceleration = 0.24f;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	// 上方向攻撃の当たり判定。プレイヤー本体より上に配置する
	static inline const float kAttackHitOffsetY = 0.7f;
	static inline const float kAttackHitWidth = 0.8f;
	static inline const float kAttackHitHeight = 0.8f;

	static inline const float kBlank = 0.01f;

	static inline const float kShiftDown = 0.08f;

	// デスフラグ
	bool isDead_ = false;
	bool isRespawnRequested_ = false;
	static inline const int32_t kMaxHp = 3;
	int32_t hp_ = kMaxHp;
	uint32_t damageInvincibleTimer_ = 0;
	static inline const uint32_t kDamageInvincibleFrames = 60;

	// ツルにつかまっている間の状態
	bool isHanging_ = false;
	uint32_t vineDetachCooldown_ = 0;
	KamataEngine::Vector3 vineAnchorPosition_{};
	float vineAngle_ = 0.0f;
	float vineAngularVelocity_ = 0.0f;
	static inline const float kVineLength = 2.0f;
	static inline const float kVineSwingAcceleration = 0.003f;
	static inline const float kVineGravity = 0.004f;
	static inline const float kVineMaxAngularVelocity = 0.11f;

	// 攻撃ギミックの経過時間カウンター
	float attackParameter_ = 0.0f;

	// 攻撃の溜め動作時間
	static inline const float kAttackChargeTimer = 6.0f;

	// 突進の時間
	static inline const float kRushTimer = 10.0f;

	// 余韻の時間
	static inline const float kReverberationTimer = 10.0f;

	// 上昇攻撃中の、1フレームあたりの上方向移動量
	static inline const float kAttackRiseSpeed = 0.1f;

	// 攻撃エフェクトの表示フラグ
	bool isAttackEffectVisible_ = false;

	// 空中で攻撃したかどうかのフラグ
	bool isAttackedInAir_ = false;

	// 現在の攻撃で敵にヒットしたか。ヒット後は空中攻撃を再び消費しない。
	bool hasHitEnemyDuringAttack_ = false;

	// 敵に攻撃を当てた後だけ使える空中ジャンプ
	bool canAirJump_ = false;

	// チャージジャンプ用の変数群

	// ジャンプをチャージしているか
	bool isJumpCharging_ = false;

	// チャージ時間用のフレーム
	uint32_t jumpChargeFrame_ = 0;

	// チャージ段階を管理するフレーム
	static inline const uint32_t kChargeLevel1Frame = 15;
	static inline const uint32_t kChargeLevel2Frame = 30;
	static inline const uint32_t kChargeMaxFrame = 45;

	// ジャンプの高さ
	static inline const float kJumpVelocityLow = 0.16f;
	static inline const float kJumpVelocityMiddle = 0.24f;
	static inline const float kJumpVelocityHigh = 0.32f;

	// チャージジャンプ時の横方向初速
	static inline const float kChargedJumpHorizontalSpeed = 0.16f;

	// ラジアンで保持する。初期値は真上（90度）
	float chargedJumpAngle_ = std::numbers::pi_v<float> / 2.0f;

	// 1フレームあたりの方向変更量（2度）
	static inline const float kJumpAngleSpeed = std::numbers::pi_v<float> / 180.0f * 2.0f;

	// 水平すぎる発射を避けるため、20度～160度に制限
	static inline const float kJumpAngleMin = std::numbers::pi_v<float> / 180.0f * 20.0f;
	static inline const float kJumpAngleMax = std::numbers::pi_v<float> / 180.0f * 160.0f;

};
