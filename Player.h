#pragma once
#include "KamataEngine.h"
#include "AABB.h"
#include "GameTime.h"
#include <array>
#include <cstdint>
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
	void Initialize(KamataEngine::Model* model, KamataEngine::Model* rushBoxEffectModel,
		KamataEngine::Model* rushBoardEffectModel, KamataEngine::Camera* camera,
		const KamataEngine::Vector3& position, KamataEngine::Model* modelFlash = nullptr);

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
	int32_t GetHp() const { return hp_; }
	int32_t GetMaxHp() const { return kMaxHp; }
	uint64_t GetDamageSequence() const { return damageSequence_; }
	uint64_t GetRingPickupSequence() const { return ringPickupSequence_; }

	// 方向選択突進中だけ攻撃判定を持つ。
	bool IsAttack() const { return isJumpRushing_; }

	// タイトル画面等で向きを強制変更するためのセッターを追加
	void SetRotationY(float rotationY)
	{
		worldTransform_.rotation_.y = rotationY;
		UpdateMatrix(); // 行列も一緒に更新する
	}

	// AABBを取得
	AABB GetAABB();
	AABB GetPreviousAABB() const;

	// 攻撃用のAABBを取得
	AABB GetAttackAABB();
	
	// 衝突応答
	void OnCollision(BaseEnemy* enemy);
	void OnHazardCollision(float sourceX);
	void TakeDamage(int32_t damage, float sourceX);
	void Respawn(const KamataEngine::Vector3& position, bool restoreHp = true);

	// ツルへつかまる・ツルから離れるための処理
	bool WantsToGrabVine() const;
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

	// ダメージ源や盾の位置を基準に、その反対側へノックバックする。
	void RequestKnockback(float sourceX);

	// 敵に攻撃を当てたとき、空中行動を回復する
	void OnAttackHit();
	// 空中で触れたリングは常に取得できる。突進の保持数は1回が上限。
	bool TryCollectAirRushRing();
	bool IsOnGround() const { return onGround_; }
	uint64_t GetLandingSequence() const { return landingSequence_; }
	
	bool IsRushAiming() const { return isRushAiming_; }

	float GetRushAngle() const { return rushAngle_; }
	bool IsJumpRushing() const { return isJumpRushing_; }
	bool IsJumpRushEnding() const { return isJumpRushEnding_; }
	bool IsAttachedToTraceVertex() const { return traceAimActive_; }
	bool IsTraceAimDirectionValid() const;
	uint64_t GetRushSequence() const { return rushSequence_; }
	const KamataEngine::Vector3& GetRushStartPosition() const { return rushStartPosition_; }
	const KamataEngine::Vector3& GetRushDirection() const { return rushDirection_; }
	const KamataEngine::Vector3& GetPreviousPosition() const { return previousPosition_; }
	bool CanStartRush() const;
	void SnapToTraceVertex(const KamataEngine::Vector3& vertex,
		const std::array<KamataEngine::Vector3, 2>& directions, int directionCount);
	void FinishRushAtVertex(const KamataEngine::Vector3& vertex,
		const std::array<KamataEngine::Vector3, 2>& nextDirections, int nextDirectionCount);
	void DetachFromTraceVertex();

	// ギミックからプレイヤーへ加える外力
	void AddExternalVelocity(const KamataEngine::Vector3& velocity)
	{
		const float deltaFrames = GameTime::GetDeltaFrames();
		velocity_.x += velocity.x * deltaFrames;
		velocity_.y += velocity.y * deltaFrames;
		velocity_.z += velocity.z * deltaFrames;
	}
	// 上昇気流内では進入時の縦速度に関係なく、一定速度で上昇する。
	void ApplyUpdraft(float upwardSpeed)
	{
		velocity_.y = upwardSpeed;
		onGround_ = false;
	}

	// 移動足場・崩れる足場への着地処理
	void LandOnGimmick(float topY, float horizontalMove);

	void BounceOnGimmick(float topY, float bounceVelocity);

	// 移動前後の位置から、固体ギミックとの衝突面を解決する
	GimmickHitSide ResolveSolidGimmickCollision(const AABB& gimmickAABB, float horizontalMove, float verticalMove = 0.0f);
	// 水車など、下面・側面を通過できる移動足場への上面着地
	bool TryLandOnOneWayGimmick(const AABB& gimmickAABB, float horizontalMove, float verticalMove);

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

	// ノックバック行動初期化・更新の関数宣言
	void BehaviorKnockbackInitialize();
	void BehaviorKnockbackUpdate();
	void UpdateRushVisuals(float deltaSeconds);
	void EmitRushVisuals();
	void SpawnRushGrain(bool board, float spread);
	void CaptureRushAfterimage();
	void ClearRushVisuals();

private:

	// ノックバックの要求を記憶するフラグ
	bool isKnockbackRequested_ = false;
	float knockbackDirection_ = 0.0f;

	// ノックバックの経過時間カウンター
	float knockbackParameter_ = 0.0f;

	// ノックバックの各フェーズの時間
	static inline const float kKnockbackBlowTime = 10.0f;    // 弾き飛ばされる時間
	static inline const float kKnockbackRecoverTime = 20.0f; // 体勢を立て直す時間
	static inline const float kKnockbackHorizontalSpeed = 0.22f;
	static inline const float kKnockbackVerticalSpeed = 0.12f;

	LRDirection lrDirection_ = LRDirection::kRight;

	// 角
	enum Corner 
	{
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
		kKnockback, // ノックバック状態
	};

	// 振るまい
	Behavior behavior_ = Behavior::kRoot;

	Behavior behaviorRequest_ = Behavior::kUnknown;

	// 指定した角の座標を得る関数
	KamataEngine::Vector3 CornerPosition(const KamataEngine::Vector3& center, Corner corner);

	// ワールド変換データ
	KamataEngine::WorldTransform worldTransform_;
	KamataEngine::Vector3 previousPosition_{};

	// モデル
	KamataEngine::Model* model_ = nullptr;
	// 白テクスチャ差し替え専用。通常表示のモデルの材質を変更しない。
	KamataEngine::Model* modelFlash_ = nullptr;
	KamataEngine::ObjectColor damageVisualColor_;
	uint32_t whiteTextureHandle_ = 0;
	KamataEngine::Vector3 velocity_ = {};

	// 突進の粒とプレイヤーの残像。モデルの所有権は GameScene にある。
	KamataEngine::Model* rushBoxEffectModel_ = nullptr;
	KamataEngine::Model* rushBoardEffectModel_ = nullptr;
	struct RushGrain {
		KamataEngine::WorldTransform transform;
		KamataEngine::Vector3 velocity{};
		float remainingSeconds = 0.0f;
		float durationSeconds = 0.0f;
		float size = 0.0f;
		bool board = false;
	};
	struct RushAfterimage {
		KamataEngine::WorldTransform transform;
		KamataEngine::ObjectColor color;
		float remainingSeconds = 0.0f;
	};
	std::array<RushGrain, 24> rushGrains_{};
	std::array<RushAfterimage, 8> rushAfterimages_{};
	KamataEngine::ObjectColor rushBoxColor_;
	KamataEngine::ObjectColor rushBoardColor_;
	float rushVisualIntervalSeconds_ = 0.0f;
	size_t nextRushGrain_ = 0;
	size_t nextRushAfterimage_ = 0;

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
	uint64_t landingSequence_ = 0;

	// 重力加速度(下方向)
	static inline const float kGravityAcceleration = 0.01f;
	// 最大落下速度(下方向)
	static inline const float kLimitFallSpeed = 0.2f;
	// W/↑またはゲームパッドXで即時に出す通常ジャンプの初速
	static inline const float kNormalJumpSpeed = 0.24f;

	// マップチップによるフィールド
	MapChipField* mapChipField_ = nullptr;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	static inline const float kBlank = 0.01f;

	static inline const float kShiftDown = 0.08f;

	// デスフラグ
	bool isDead_ = false;
	static inline const int32_t kMaxHp = 5;
	int32_t hp_ = kMaxHp;
	float damageInvincibleTimer_ = 0.0f;
	static inline const float kDamageInvincibleFrames = 60.0f;
	bool damageFlashActive_ = false;
	float damageRedTimer_ = 0.0f;
	static inline const float kDamageRedFrames = 8.0f;
	static inline const float kDamageFlashIntervalFrames = 5.0f;
	uint64_t damageSequence_ = 0;
	uint64_t ringPickupSequence_ = 0;

	// ツルにつかまっている間の状態
	bool isHanging_ = false;
	bool detachedFromTraceThisFrame_ = false;
	float vineDetachCooldown_ = 0.0f;
	KamataEngine::Vector3 vineAnchorPosition_{};
	float vineAngle_ = 0.0f;
	float vineAngularVelocity_ = 0.0f;
	static inline const float kVineLength = 2.0f;
	static inline const float kVineSwingAcceleration = 0.003f;
	static inline const float kVineGravity = 0.004f;
	static inline const float kVineMaxAngularVelocity = 0.11f;

	// 空中で方向選択突進を使えるか。着地または攻撃成功で回復する。
	bool canAirRush_ = true;

	// チャージジャンプ用の変数群

	// ジャンプをチャージしているか
	bool isRushAiming_ = false;
	// 図形の頂点に吸い付いている間は、次の突進まで位置を保持する。
	bool traceAimActive_ = false;
	bool traceRushActive_ = false;
	bool traceDirectionSelected_ = false;
	KamataEngine::Vector3 traceAimAnchor_{};
	std::array<KamataEngine::Vector3, 2> traceAimDirections_{};
	int traceAimDirectionCount_ = 0;

	// 方向選択を開始してからの実時間
	float rushAimElapsedSeconds_ = 0.0f;

	// Spaceを押し続けられる最大時間。経過後は現在方向へ自動発動する。
	static inline const float kRushAimMaxSeconds = 0.50f;

	// 突進終了後に残す速度の割合
	static inline const float kRushExitHorizontalRetention = 0.65f;
	static inline const float kRushExitVerticalRetention = 0.35f;

	// 通常走行速度を超えた慣性の、60FPS基準の1フレーム減衰率
	static inline const float kRushMomentumDamping = 0.96f;


	// Spaceを離した後の全周攻撃付き突進ジャンプ
	bool isJumpRushing_ = false;
	bool isJumpRushEnding_ = false;
	float jumpRushFrame_ = 0.0f;
	uint64_t rushSequence_ = 0;
	KamataEngine::Vector3 rushStartPosition_{};
	KamataEngine::Vector3 rushDirection_{};
	static inline const float kJumpRushSpeed = 0.5f;
	static inline const float kJumpRushFrame = 9.0f;
	// 突進中に角をかすめたときだけ許す位置補正（マップ1マス=1.0）。
	static inline const float kRushCornerCorrectionMax = 0.15f;
	static inline const float kRushCornerCorrectionStep = 0.025f;
	// 地上突進の発動時に、床との接触を切るため上へずらす量。
	static inline const float kGroundRushLift = 0.15f;


	// ラジアンで保持する。初期値は真上（90度）
	float rushAngle_ = std::numbers::pi_v<float> / 2.0f;

	// 突進中はプレイヤーの周囲全体を攻撃範囲にする。
	static inline const float kRushAttackSize = 1.6f;

};
