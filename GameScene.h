#pragma once
#include "KamataEngine.h"
#include <vector>
#include "Player.h"
#include "BaseEnemy.h"
#include "BaseEffect.h"
#include "Skydome.h"
#include "MapChipField.h"
#include "CameraController.h"
#include "AABB.h"
#include "DeathParticles.h"
#include "Fade.h"
#include "BaseGimmick.h"
#include "TiledRoom.h"
#include "WorldProgress.h"

#include <string>

class StageManager;

class GameScene {

public:
	// メンバ変数
	// プレイヤー
	Player* player_ = nullptr;

	// 敵
	std::list<BaseEnemy*> enemies_;

	// デスパーティクル
	DeathParticles* deathParticles_ = nullptr;

	// ヒットエフェクト
	std::list<BaseEffect*> effects_;

	// 水流・足場などのステージギミック
	std::list<BaseGimmick*> gimmicks_;

	// 天球
	Skydome* skydome_ = nullptr;

	// カメラコントローラ
	CameraController* cameraController_ = nullptr;
	
	// エフェクトを生成
	void CreateHitEffect(const KamataEngine::Vector3& position);
	void CreateGuardEffect(const KamataEngine::Vector3& position);

	GameScene();

	~GameScene();
	
public:

	/// <summary>
	/// 初期化
	/// </summary>
	void Initialize(StageManager* stageDataManager);

	void Update();

	void Draw();

	// 終了フラグのgetter
	bool IsFinished() const { return finished_; }
	
	// リロード要求フラグのgetter
	bool IsReloadRequested() const { return reloadRequested_; }

	bool IsCleared() const { return isCleared_; }
	bool IsStageSelectRequested() const { return stageSelectRequested_; }

private:

	// 終了フラグ
	bool finished_ = false;

	// ゲームのフェーズ (型)
	enum class Phase
	{
		kFadeIn,  // フェードイン
		kPlay,	  // ゲームプレイ
		kDeath,   // デス演出
		kGameOver, // ゲームオーバー時の選択
		kStageClear, // ステージクリア演出
		kFadeOut, // フェードアウト
	};

	// ゲームの現在フェーズ
	Phase phase_;
	
	// フェードにかける時間
	static inline const float kFadeDuration = 1.0f;

	// フェード
	Fade* fade_ = nullptr;

	// 3Dモデルデータ
	// プレイヤーの3Dモデルデータ
	KamataEngine::Model* modelPlayer_ = nullptr;
	
	// プレイヤーの攻撃エフェクトの3Dモデルデータ
	KamataEngine::Model* modelAttackEffect_ = nullptr;

	// 敵に対してのヒットエフェクトの3Dモデルデータ
	KamataEngine::Model* modelHitEffect_ = nullptr;

	// 盾持ちの敵に対してのガードエフェクトの3Dモデルデータ
	KamataEngine::Model* modelGuardEffect_ = nullptr;

	// 通常の敵の3Dモデルデータ
	KamataEngine::Model* modelWalkEnemy_ = nullptr;

	// 盾持ちの敵の3Dモデルデータ
	KamataEngine::Model* modelShieldEnemy_ = nullptr;

	// Stage 3の飛行敵の3Dモデルデータ
	KamataEngine::Model* modelFlyingEnemy_ = nullptr;

	// ブロックの3Dモデルデータ
	KamataEngine::Model* modelBlock_ = nullptr;

	// ギミックごとの3Dモデルデータ
	KamataEngine::Model* modelFountain_ = nullptr;
	KamataEngine::Model* modelMovingPlatform_ = nullptr;
	KamataEngine::Model* modelUpdraft_ = nullptr;
	KamataEngine::Model* modelLotusLeaf_ = nullptr;
	KamataEngine::Model* modelFallingRock_ = nullptr;
	KamataEngine::Model* modelVine_ = nullptr;
	KamataEngine::Model* modelOreSwitch_ = nullptr;
	KamataEngine::Model* modelStoneBridge_ = nullptr;
	KamataEngine::Model* modelVanishingCloud_ = nullptr;
	KamataEngine::Model* modelThunderCloud_ = nullptr;
	KamataEngine::Model* modelLightning_ = nullptr;

	// デスパーティクルの3Dモデルデータ
	KamataEngine::Model* modelDeathParticles_ = nullptr;

	// 天球の3Dモデルデータ
	KamataEngine::Model* modelSkydome_ = nullptr;

	KamataEngine::Sprite* jumpDirectionBack_ = nullptr;
	KamataEngine::Sprite* jumpDirectionCursor_ = nullptr;
	std::vector<KamataEngine::Sprite*> hudGuideCharacters_;

	// ゲームオーバーの選択画面
	KamataEngine::Sprite* gameOverOverlay_ = nullptr;
	KamataEngine::Sprite* retryChoice_ = nullptr;
	KamataEngine::Sprite* stageSelectChoice_ = nullptr;
	KamataEngine::Sprite* gameOverSelector_ = nullptr;
	bool isRetrySelected_ = true;

	// ステージクリア時に表示する演出用スプライト
	KamataEngine::Sprite* stageClearOverlay_ = nullptr;
	KamataEngine::Sprite* stageClearBanner_ = nullptr;
	uint32_t stageClearTimer_ = 0;
	static inline const uint32_t kStageClearDuration = 90;

	// ワールドトランスフォーム
	KamataEngine::WorldTransform worldTransform_;

	// 1マスごとのブロック描画用トランスフォーム
	std::vector<KamataEngine::WorldTransform*> blockTransforms_;

	// デバッグカメラ有効
	bool isDebugCameraActive_ = false;

	KamataEngine::DebugCamera* debugCamera_ = nullptr;

	// マップチップフィールド
	MapChipField* mapChipField_ = nullptr;

	// Tiledの部屋と、部屋をまたいで保持する進行状態
	TiledRoomData currentRoom_;
	WorldProgress worldProgress_;
	std::string currentRoomPath_;
	bool isUsingTiledRooms_ = false;
	uint32_t roomTransitionCooldown_ = 0;
	
	void GenerateFieldObjects();
	void BuildBlockTransforms();
	void ClearRoomObjects();
	bool LoadTiledRoom(const std::string& roomPath, const std::string& spawnId);
	void ConfigureCameraForCurrentMap();
	void CheckRoomTransitions();
	bool IsVisible(const AABB& bounds, float margin = 2.0f) const;

	// 敵生成用の関数を追加
	void GenerateEnemy(uint32_t xIndex, uint32_t yIndex, uint8_t subID);
	void GenerateEnemyAtPosition(const KamataEngine::Vector3& position, const std::string& enemyType);
	void GenerateGimmick(uint32_t xIndex, uint32_t yIndex, uint8_t subID);
	void GenerateCheckpoint(uint32_t xIndex, uint32_t yIndex, uint8_t subID);

	// 全ての当たり判定を行う
	void CheckAllCollisions();

	// フェーズの切り替え
	void ChangePhase();
	void DrawHud();
	
	// リロード要求フラグ
	bool reloadRequested_ = false;
	bool stageSelectRequested_ = false;
	bool isGameOverPending_ = false;

	// ステージマネージャ用の参照用のポインタ
	StageManager* stageManager_ = nullptr;

	bool isCleared_ = false;
	bool isOreSwitchActive_ = false;
	uint32_t oreSwitchTimer_ = 0;

	// 最後に上面へ着地したチェックポイント。未到達時はP0の初期地点。
	KamataEngine::Vector3 respawnPosition_{};
	int32_t activatedCheckpointIndex_ = -1;
	
};
