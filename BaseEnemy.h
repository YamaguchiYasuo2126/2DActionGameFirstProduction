#pragma once
#include "AABB.h"
#include "KamataEngine.h"

class Player;
class GameScene;
class MapChipField;

/// <summary>
/// 敵
/// </summary>
class BaseEnemy {

public:

	// 仮想デストラクタ
	virtual ~BaseEnemy() = default;

	/// <summary>
	/// 初期化
	/// </summary>
	virtual void Initialize(KamataEngine::Model* model, KamataEngine::Camera* camera, const KamataEngine::Vector3& position);

	/// <summary>
	/// 更新
	/// </summary>
	virtual void Update();

	/// <summary>
	/// 描画
	/// </summary>
	virtual void Draw();
	
	// 衝突応答
	virtual void OnCollision(Player* player);

	// ワールド座標を取得
	KamataEngine::Vector3 GetWorldPosition();

	// AABBを取得
	AABB GetAABB();

	// デスフラグのgetterを追加
	bool IsDead() const { return isDead_; }

	// 衝突判定無効フラグのgetter
	bool IsCollisionDisabled() const { return isCollisionDisabled_; }

	// ゲームシーンをセットするsetterを追加
	void SetGameScene(GameScene* gameScene) { gameScene_ = gameScene; }
	void SetMapChipField(MapChipField* mapChipField) { mapChipField_ = mapChipField; }

protected:
	
	// 歩行行動初期化
	virtual void BehaviorWalkInitialize();

	// 歩行行動更新
	virtual void BehaviorWalkUpdate();

	// デス行動初期化
	virtual void BehaviorDeathInitialize();

	// デス行動更新
	virtual void BehaviorDeathUpdate();
	
	/// <summary>
	/// 行列の更新と転送
	/// </summary>
	void UpdateMatrix();
	struct TerrainHit
	{
		bool wall = false;
		bool floor = false;
		bool ceiling = false;
	};
	TerrainHit MoveWithMapCollision(float moveX, float moveY);
	void MoveGroundEnemy(bool moveHorizontally = true);
	bool IsSolidPoint(float x, float y) const;
	bool IsBlockedAt(float x, float y) const;

protected:
	// 振るまい
	enum class Behavior {
		kUnknown, // 変更リクエストがない状態
		kWalk,    // 歩行中
		kDeath,   // デス演出中
		kGuard,   // ガード（のけぞり）状態
	};

	// 振るまい
	Behavior behavior_ = Behavior::kWalk;

	Behavior behaviorRequest_ = Behavior::kUnknown;

	// ワールド変換データ(所有)
	KamataEngine::WorldTransform worldTransform_;

	// モデルのポインタ(借りてくる用)
	KamataEngine::Model* model_ = nullptr;

	// カメラのポインタ(借りてくる用)
	KamataEngine::Camera* camera_ = nullptr;

	// 歩行の速さ
	static inline const float kWalkSpeed = 0.01f;

	// 速度
	KamataEngine::Vector3 velocity_ = {};
	MapChipField* mapChipField_ = nullptr;
	static inline const float kGravityAcceleration = 0.01f;
	static inline const float kMaxFallSpeed = 0.2f;

	// 最初の角度[度]
	static inline const float kWalkMotionAngleStart = 5.0f;
	// 最後の角度[度]
	static inline const float kWalkMotionAngleEnd = 35.0f;
	// アニメーションの周期となる時間[秒]
	static inline const float kWalkMotionTime = 1.0f;

	// 経過時間
	float walkTimer_ = 0.0f;

	// キャラクターの当たり判定サイズ
	static inline const float kWidth = 0.8f;
	static inline const float kHeight = 0.8f;

	// デスフラグ
	bool isDead_ = false;

	// デス演出のタイマー
	float deathTimer_ = 0.0f;
	// デス演出にかかる時間
	static inline const float kDeathAnimationTime = 1.0f;

	// 衝突判定無効フラグ
	bool isCollisionDisabled_ = false;

	// ゲームシーンのポインタを保存する変数を追加
	GameScene* gameScene_ = nullptr;
};
