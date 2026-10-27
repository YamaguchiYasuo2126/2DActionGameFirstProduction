#include "GameScene.h"
#include "2d/ImGuiManager.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include <algorithm>
#include "MyMathUtility.h"
#include "WalkEnemy.h"
#include "ShieldEnemy.h"
#include "FlyingEnemy.h"
#include "HitEffect.h"
#include "GuardEffect.h"
#include "StageManager.h"
#include "Player.h"
#include "StageGimmick.h"
#include "CheckpointGate.h"
#include "CheckpointTrigger.h"
#include "TriangleTraceGimmick.h"
#include "GamePadInput.h"
#include "GameTime.h"
#include "SoundManager.h"

#include <filesystem>
#include <limits>
#include <utility>


using namespace KamataEngine;

GameScene::GameScene() {}

GameScene::~GameScene() {
	GameTime::SetTimeScale(1.0f);
	ClearRoomObjects();
	delete player_;
	player_ = nullptr;

	// 3Dモデルデータの解放
	// 自キャラの3Dモデルデータの解放
	delete modelPlayer_;

	// 攻撃エフェクトの3Dモデルデータの解放
	delete modelAttackEffect_;

	// ヒットエフェクト用3Dモデルデータの解放
	delete modelHitEffect_;

	// ガードエフェクト用3Dモデルデータの解放
	delete modelGuardEffect_;

	// 通常の敵の3Dモデルデータの解放
	delete modelWalkEnemy_;

	// 盾持ちの敵の3Dモデルデータの解放
	delete modelShieldEnemy_;

	// Stage 3の飛行敵の3Dモデルデータの解放
	delete modelFlyingEnemy_;

	// ブロックの3Dモデルデータの解放
	delete modelBlock_;
	delete modelCheckpoint_;
	delete modelCheckpointJewel_;
	delete modelTriangleEdge_;
	delete modelTriangleFill_;
	delete modelFountain_;
	delete modelMovingPlatform_;
	delete modelWaterWheelShaft_;
	delete modelUpdraft_;
	delete modelLotusLeaf_;
	delete modelFallingRock_;
	delete modelVine_;
	delete modelOreSwitch_;
	delete modelStoneBridge_;
	delete modelVanishingCloud_;
	delete modelThunderCloud_;
	delete modelLightning_;
	delete modelRingBoxEffect_;
	delete modelRingBoardEffect_;

	// デスパーティクルのモデルの開放
	delete modelDeathParticles_;
	// デスパーティクルのインスタンスの解放
	if (deathParticles_) 
	{
		delete deathParticles_;
		deathParticles_ = nullptr; // 安全のためnullptrを代入
	}

	// デバッグカメラの解放
	delete debugCamera_;

	// 天球の解放
	delete skydome_;

	// 天球のモデルの解放
	delete modelSkydome_;
	delete lightGroup_;

	// マップチップフィールドの解放
	delete mapChipField_;

	// カメラコントローラの解放
	delete cameraController_;

	// フェードの解放
	delete fade_;

	delete gameOverOverlay_;
	delete retryChoice_;
	delete stageSelectChoice_;
	delete gameOverSelector_;
	delete stageClearOverlay_;
	delete stageClearBanner_;

	delete hudGuideSprite_;
	hudGuideSprite_ = nullptr;

	// ジャンプ表示の解放
	delete jumpDirectionCursor_;
	jumpDirectionCursor_ = nullptr;
}

void GameScene::Initialize(StageManager* stageDataManager) 
{
	GameTime::SetTimeScale(1.0f);
	// ここにインゲームの初期化処理を書く

	// 引数をメンバ変数に記録する
	stageManager_ = stageDataManager;

	// ゲームプレイフェーズから開始
	phase_ = Phase::kPlay;

	// ワールドトランスフォームの初期化
	worldTransform_.Initialize();

	// ファイル名を指定してテクスチャを読み込む

	// 3Dモデルデータの生成
	// プレイヤーのモデルデータ生成
	modelPlayer_ = Model::CreateFromOBJ("player", true);

	// 通常の敵のモデルデータ生成
	modelWalkEnemy_ = Model::CreateFromOBJ("enemy", true);

	// 盾持ちの敵のモデルデータ生成
	modelShieldEnemy_ = Model::CreateFromOBJ("shieldEnemy", true);

	// Stage 3の飛行敵のモデルデータ生成
	modelFlyingEnemy_ = Model::CreateFromOBJ("thunderSpirit", true);

	// 現在選択中のステージに応じて、足場のモデルを切り替える。
	// このモデルは通常ブロック・旧CSV用チェックポイント足場・基本ギミックで使用する。
	switch (stageManager_->GetCurrentStageIndex())
	{
	case 0: // Stage 1: 川
		modelBlock_ = Model::CreateFromOBJ("riverStone", true);
		break;

	case 1: // Stage 2: 山
		modelBlock_ = Model::CreateFromOBJ("mountainStone", true);
		break;

	case 2: // Stage 3: 空
		modelBlock_ = Model::CreateFromOBJ("skyTile", true);
		break;

	default:
		// 想定外のステージでもゲームを続行できるよう、川の足場を使う。
		modelBlock_ = Model::CreateFromOBJ("riverStone", true);
		break;
	}
	modelCheckpoint_ = Model::CreateFromOBJ("checkpoint", true);
	modelCheckpointJewel_ = Model::CreateFromOBJ("checkpointJewel", true);

	// ギミックごとの見た目を読み込む。衝突判定・挙動は従来どおりStageGimmick側で管理する。
	modelTriangleEdge_ = Model::CreateFromOBJ("movingPlatform", true);
	modelTriangleFill_ = Model::CreateFromOBJ("triangleTrampoline", true);
	triangleWhiteTexture_ = TextureManager::Load("white1x1.png");
	modelFountain_ = Model::CreateFromOBJ("fountain", true);
	modelMovingPlatform_ = Model::CreateFromOBJ("movingPlatform", true);
	modelWaterWheelShaft_ = Model::CreateFromOBJ("waterWheelLiftShaft", true);
	modelUpdraft_ = Model::CreateFromOBJ("upDraft", true);
	modelLotusLeaf_ = Model::CreateFromOBJ("lotusLeaf", true);
	modelFallingRock_ = Model::CreateFromOBJ("fallingRock", true);
	modelVine_ = Model::CreateFromOBJ("vine", true);
	modelOreSwitch_ = Model::CreateFromOBJ("oreSwitch", true);
	modelStoneBridge_ = Model::CreateFromOBJ("stoneBridge", true);
	modelVanishingCloud_ = Model::CreateFromOBJ("vanishingCloud", true);
	modelThunderCloud_ = Model::CreateFromOBJ("thunderCloud", true);
	modelLightning_ = Model::CreateFromOBJ("lightning", true);
	modelRingBoxEffect_ = Model::CreateFromOBJ("boxEffectGrain", true);
	modelRingBoardEffect_ = Model::CreateFromOBJ("squareBoardEffectGrain", true);

	// デスパーティクルの3Dモデルデータの生成
	modelDeathParticles_ = Model::CreateFromOBJ("deathParticle", true);

	// プレイヤーの攻撃エフェクトの3Dモデルデータの生成
	modelAttackEffect_ = Model::CreateFromOBJ("attackEffect", true);

	// 敵に対してのヒットエフェクトの3Dモデルデータの生成
	modelHitEffect_ = Model::CreateFromOBJ("hitEffect", true);
	HitEffect::SetModel(modelHitEffect_);
	
	// 盾持ちの敵に対してのガードエフェクトの3Dモデルデータの生成
	modelGuardEffect_ = Model::CreateFromOBJ("ring", true);
	GuardEffect::SetModel(modelGuardEffect_);

	// 天球の3Dモデルの生成
	modelSkydome_ = Model::CreateFromOBJ("Skydome", true);

	// プレイヤー前方のスポットライトと、落石の着地点に投影する丸影。
	lightGroup_ = LightGroup::Create();
	// 【全体の明るさ】環境光。3つの値を上げると、マップやキャラ全体が明るくなる。
	// 参考プロジェクトの {0.74, 0.75, 0.79} から少し明るくした値。
	lightGroup_->SetAmbientColor({0.86f, 0.87f, 0.90f});
	// 使用するライトだけを有効にする。プレイヤーのスポットライトは毎フレーム切り替える。
	for (int i = 0; i < LightGroup::kDirLightNum; ++i)
	{
		lightGroup_->SetDirLightActive(i, false);
	}
	for (int i = 0; i < LightGroup::kPointLightNum; ++i)
	{
		lightGroup_->SetPointLightActive(i, false);
	}
	for (int i = 0; i < LightGroup::kSpotLightNum; ++i)
	{
		lightGroup_->SetSpotLightActive(i, false);
	}
	// 【面の陰影】平行光源。方向で明るい面が変わり、色の値で光の強さを調整できる。
	lightGroup_->SetDirLightActive(0, true);
	lightGroup_->SetDirLightDir(0, {-0.35f, 0.75f, -0.56f});
	lightGroup_->SetDirLightColor(0, {0.68f, 0.69f, 0.73f});
	// 【プレイヤー前方の光】色・強さ・届く距離・端の減衰・広がる角度の順。
	// スポットライトだけを強くしたい場合は Intensity、広げたい場合は Radius や FactorAngle を変更する。
	lightGroup_->SetSpotLightColor(0, {1.0f, 0.95f, 0.75f});
	lightGroup_->SetSpotLightIntensity(0, 8.0f);
	lightGroup_->SetSpotLightRadius(0, 7.0f);
	lightGroup_->SetSpotLightDecay(0, 1.5f);
	lightGroup_->SetSpotLightFactorAngle(0, {0.30f, 0.65f}); // 減衰開始・終了角度（ラジアン）
	// 【落石の予告影】スポットライトとは別。下の値で影の広がりを調整する。
	lightGroup_->SetCircleShadowActive(0, false);
	// 丸影の投影軸は2Dステージ用シェーダーで床方向に固定する。
	lightGroup_->SetCircleShadowDistanceCasterLight(0, 2.5f);
	lightGroup_->SetCircleShadowFactorAngle(0, {0.12f, 0.23f});
	for (Model* model : {modelPlayer_, modelWalkEnemy_, modelShieldEnemy_, modelFlyingEnemy_,
		modelBlock_, modelCheckpoint_, modelCheckpointJewel_, modelTriangleEdge_,
		modelTriangleFill_, modelFountain_,
		modelMovingPlatform_, modelWaterWheelShaft_, modelUpdraft_, modelLotusLeaf_,
		modelFallingRock_, modelVine_, modelOreSwitch_, modelStoneBridge_,
		modelVanishingCloud_, modelThunderCloud_, modelLightning_,
		modelRingBoxEffect_, modelRingBoardEffect_, modelDeathParticles_,
		modelAttackEffect_, modelHitEffect_, modelGuardEffect_})
	{
		if (model)
		{
			model->SetLightGroup(lightGroup_);
		}
	}

	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	jumpDirectionCursor_ = Sprite::Create(textureHandle, Vector2(337.0f, 627.0f));
	jumpDirectionCursor_->SetSize(Vector2(16.0f, 16.0f));
	jumpDirectionCursor_->SetColor(Vector4(1.0f, 0.8f, 0.2f, 1.0f));

	// 操作案内は専用画像1枚をSpriteで表示する。
	const uint32_t hudGuideTexture = TextureManager::Load("controlGuide.png");
	hudGuideSprite_ = Sprite::Create(hudGuideTexture, Vector2(874.0f, 563.0f));
	hudGuideSprite_->SetSize(Vector2(390.0f, 145.0f));

	// ゲームオーバー時は、赤い背景と2つの選択カードを表示する。
	gameOverOverlay_ = Sprite::Create(textureHandle, Vector2(0.0f, 0.0f));
	gameOverOverlay_->SetSize(Vector2(1280.0f, 720.0f));
	gameOverOverlay_->SetColor(Vector4(0.22f, 0.02f, 0.02f, 0.92f));

	retryChoice_ = Sprite::Create(textureHandle, Vector2(180.0f, 300.0f));
	retryChoice_->SetSize(Vector2(380.0f, 150.0f));
	retryChoice_->SetColor(Vector4(0.20f, 0.65f, 0.30f, 1.0f));

	stageSelectChoice_ = Sprite::Create(textureHandle, Vector2(720.0f, 300.0f));
	stageSelectChoice_->SetSize(Vector2(380.0f, 150.0f));
	stageSelectChoice_->SetColor(Vector4(0.20f, 0.42f, 0.82f, 1.0f));

	gameOverSelector_ = Sprite::Create(textureHandle, Vector2());
	gameOverSelector_->SetSize(Vector2(396.0f, 166.0f));
	gameOverSelector_->SetColor(Vector4(1.0f, 0.90f, 0.12f, 1.0f));

	// ステージクリア時は、一度金色の演出を見せてから次の画面へ進める。
	stageClearOverlay_ = Sprite::Create(textureHandle, Vector2(0.0f, 0.0f));
	stageClearOverlay_->SetSize(Vector2(1280.0f, 720.0f));
	stageClearOverlay_->SetColor(Vector4(0.10f, 0.08f, 0.01f, 0.55f));

	stageClearBanner_ = Sprite::Create(textureHandle, Vector2(260.0f, 290.0f));
	stageClearBanner_->SetSize(Vector2(760.0f, 140.0f));
	stageClearBanner_->SetColor(Vector4(1.0f, 0.78f, 0.12f, 0.90f));

	// マップチップの生成
	mapChipField_ = new MapChipField;

	// カメラコントローラの生成
	cameraController_ = new CameraController();

	// カメラコントローラの初期化
	cameraController_->Initialize();

	// エフェクトにカメラをセット
	HitEffect::SetCamera(cameraController_->GetCamera());
	GuardEffect::SetCamera(cameraController_->GetCamera());

	// ゲーム開始時はまだ作らないのでnullptrを入れておく
	deathParticles_ = nullptr;

	// Stage 1をTiledによる地続きマップのテスト入口にする。
	// ファイルがない場合とStage 2/3は従来CSVへフォールバックする。
	const std::string firstTiledRoom = "Resources/maps/rooms/room_01.tmj";
	if (stageManager_->GetCurrentStageIndex() == 0 && std::filesystem::exists(firstTiledRoom))
	{
		isUsingTiledRooms_ = LoadTiledRoom(firstTiledRoom, "start");
	}

	if (!isUsingTiledRooms_)
	{
		const StageData& stageData = stageManager_->GetCurrentStageData();
		const std::string stageFileName = "Resources/fields/" + stageData.name + ".csv";
		mapChipField_->LoadMapChipCsv(stageFileName);
		GenerateFieldObjects();
		ConfigureCameraForCurrentMap();
	}
	UpdateLightGroup();
	
	// 追従対象をセット
	cameraController_->SetTarget(player_);

	// カメラコントローラのリセット (瞬間合わせ)
	cameraController_->Reset();

	// デバッグカメラの生成
	debugCamera_ = new DebugCamera(1280, 720);

	// 天球の生成
	skydome_ = new Skydome();

	// 天球の初期化
	skydome_->Initialize(modelSkydome_, cameraController_->GetCamera());

	// フェードの生成と初期化
	fade_ = new Fade();
	fade_->Initialize();

	// 最初はフェードインを開始する
	fade_->Start(Fade::Status::FadeIn, kFadeDuration);
	phase_ = Phase::kFadeIn; // 初期フェーズをフェードインに設定
}

void GameScene::Update()
{
	// 方向選択中は入力の読み取りを通常どおり保ち、ゲーム内時間だけを20%にする。
	const bool isRushAimInput = Input::GetInstance()->PushKey(DIK_SPACE) || GamePadInput::IsJumpPushed();
	const bool isRushAimTriggered = Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsJumpTriggered();
	const bool isRushAiming = phase_ == Phase::kPlay && player_ && isRushAimInput &&
		(player_->IsRushAiming() || isRushAimTriggered);
	GameTime::SetTimeScale(isRushAiming ? kRushAimTimeScale : 1.0f);
	const float deltaFrames = GameTime::GetDeltaFrames();

	// ここにインゲームの更新処理を書く
	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C))
	{
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	ImGui::Begin("Stage");
	if (isUsingTiledRooms_)
	{
		ImGui::Text("Room: %s", currentRoom_.roomId.c_str());
		ImGui::Text("Tile blocks: %zu", blockTransforms_.size());
	}
	if (!tiledRoomLoadError_.empty())
	{
		ImGui::TextWrapped("Tiled error: %s", tiledRoomLoadError_.c_str());
	}
	// リロードボタン
	if (ImGui::Button("Reload")) 
	{
		// ボタンを押したらリロード要求フラグを立てる
		reloadRequested_ = true;
	}

	if (phase_ == Phase::kGameOver)
	{
		ImGui::Separator();
		ImGui::Text("GAME OVER");
		ImGui::Text("LEFT: Retry this stage    RIGHT: Return to stage select    SPACE: Confirm");
	}
	ImGui::End();

	#endif

	// フェードの更新処理
	fade_->Update();

	// リンクIDごとの石橋タイマーを独立して更新する。
	switchLinkState_.Update(deltaFrames);

	// フェーズ切り替えのチェックを呼び出す
	ChangePhase();

	switch (phase_)
	{
	case Phase::kFadeIn:
	case Phase::kPlay:
		// ゲームプレイフェーズの処理

		// 天球の更新
		skydome_->Update();
	
		// Spaceを押した瞬間、最も近い未完成の頂点へ吸い付く。
		// 既に輪郭へ吸い付いている場合は、その頂点を維持して自分で次の方向を選ぶ。
		if (phase_ == Phase::kPlay && isRushAimTriggered && !player_->IsAttachedToTraceVertex() &&
			player_->CanStartRush())
		{
			TriangleTraceGimmick::SnapCandidate nearest{};
			bool found = false;
			const Vector3 playerPosition = player_->GetWorldPosition();
			for (TriangleTraceGimmick* triangle : triangleTraces_)
			{
				TriangleTraceGimmick::SnapCandidate candidate{};
				if (triangle->FindSnapCandidate(playerPosition, candidate) &&
					(!found || candidate.distanceSquared < nearest.distanceSquared))
				{
					nearest = candidate;
					found = true;
				}
			}

			if (found)
			{
				player_->SnapToTraceVertex(nearest.vertex, nearest.directions, nearest.directionCount);
			}
		}

		// 自キャラの更新
		player_->Update();
		for (TriangleTraceGimmick* triangle : triangleTraces_)
		{
			triangle->ObserveRush(player_);
		}

		// 敵キャラの更新
		for (BaseEnemy* enemy : enemies_)
		{
			enemy->Update();
		}

		UpdateGimmicksAndFallingRocks();

		// 全ての当たり判定
		CheckAllCollisions();
		
		// デスフラグの立った通常の敵を削除
		enemies_.remove_if([](BaseEnemy* enemy)
			{
			if (enemy->IsDead())
			{
				delete enemy;
				return true;
			}
			return false;
		});

		if (isUsingTiledRooms_)
		{
			CheckRoomTransitions();
		}

		break;

	case Phase::kGameOver:
		if (Input::GetInstance()->TriggerKey(DIK_LEFT) || GamePadInput::IsLeftTriggered())
		{
			isRetrySelected_ = true;
		}

		if (Input::GetInstance()->TriggerKey(DIK_RIGHT) || GamePadInput::IsRightTriggered())
		{
			isRetrySelected_ = false;
		}

		if (Input::GetInstance()->TriggerKey(DIK_SPACE) || GamePadInput::IsConfirmTriggered())
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kDecision);
			if (isRetrySelected_)
			{
				reloadRequested_ = true;
			}
			else
			{
				stageSelectRequested_ = true;
				finished_ = true;
			}
		}

		gameOverSelector_->SetPosition(isRetrySelected_ ? Vector2(172.0f, 292.0f) : Vector2(712.0f, 292.0f));
		break;
	case Phase::kDeath:
		// デス演出フェーズの処理
		
		// 天球の更新
		skydome_->Update();

		// 敵キャラの更新
		for (BaseEnemy* enemy : enemies_)
		{
			enemy->Update();
		}

		UpdateGimmicksAndFallingRocks();

		// デスパーティクルが存在するなら更新
		if (deathParticles_)
		{
			deathParticles_->Update();
		}

		break;

	case Phase::kFadeOut:
		// 天球の更新
		skydome_->Update();
		// 敵キャラの更新
		for (BaseEnemy* enemy : enemies_) 
		{
			enemy->Update();
		}

		UpdateGimmicksAndFallingRocks();

		// 全ての当たり判定
		CheckAllCollisions();

		break;
	}

	// エフェクトの更新
	for (BaseEffect* effect : effects_)
	{
		effect->Update();
	}

	// 終了したエフェクトを削除する
	effects_.remove_if([](BaseEffect* effect) 
		{
		if (effect->IsDead())
		{
			delete effect;
			return true;
		}
		return false;
	});

	// カメラコントローラの更新
	cameraController_->Update();

	// デバッグカメラの更新
	if (isDebugCameraActive_)
	{

		debugCamera_->Update();
		// デバッグカメラからビュー行列とプロジェクション行列をコピーする
		cameraController_->GetCamera()->matView = debugCamera_->GetCamera().matView;
		cameraController_->GetCamera()->matProjection = debugCamera_->GetCamera().matProjection;

		// ビュープロジェクション行列の転送
		cameraController_->GetCamera()->TransferMatrix();
	}
	else
	{
		// 通常カメラのビュープロジェクション行列の更新と転送
		cameraController_->GetCamera()->UpdateMatrix();
	}
	UpdateLightGroup();

	// プレイヤーの周囲に8方向選択を表示する
	if (player_ && player_->IsRushAiming())
	{
		Camera* camera = cameraController_->GetCamera();

		// ワールド座標をNDC座標へ変換
		const Matrix4x4 viewProjection = MyMathUtility::Multiply(camera->matView, camera->matProjection);

		const Vector3 playerNdc = MyMathUtility::Transform(player_->GetWorldPosition(), viewProjection);

		constexpr float kScreenWidth = 1280.0f;
		constexpr float kScreenHeight = 720.0f;

		// NDC座標から画面座標へ変換
		const Vector2 playerScreenPosition = {(playerNdc.x + 1.0f) * 0.5f * kScreenWidth, (1.0f - playerNdc.y) * 0.5f * kScreenHeight};

		// 選択方向へカーソルを配置
		const float angle = player_->GetRushAngle();
		constexpr float kCursorRadius = 42.0f;
		constexpr float kCursorHalfSize = 8.0f;

		jumpDirectionCursor_->SetPosition({playerScreenPosition.x + std::cos(angle) * kCursorRadius - kCursorHalfSize,
		                                   playerScreenPosition.y - std::sin(angle) * kCursorRadius - kCursorHalfSize});
		jumpDirectionCursor_->SetColor(player_->IsAttachedToTraceVertex()
			? player_->IsTraceAimDirectionValid() ? Vector4{0.2f, 1.0f, 0.9f, 1.0f}
				: Vector4{1.0f, 0.3f, 0.3f, 1.0f}
			: Vector4{1.0f, 0.8f, 0.2f, 1.0f});
	}

}

void GameScene::UpdateGimmicksAndFallingRocks()
{
	for (BaseGimmick* gimmick : gimmicks_)
	{
		gimmick->Update();
	}
	// 水車や移動足場の更新順に依存しないよう、全ギミックの移動後に落石を判定する。
	for (BaseGimmick* gimmick : gimmicks_)
	{
		if (auto* stageGimmick = dynamic_cast<StageGimmick*>(gimmick))
		{
			stageGimmick->ResolveFallingRockCollisions(gimmicks_);
		}
	}
}

void GameScene::UpdateLightGroup()
{
	if (!lightGroup_)
	{
		return;
	}
	const bool playerActive = player_ && !player_->IsDead();
	lightGroup_->SetSpotLightActive(0, playerActive);
	if (playerActive)
	{
		const Vector3 playerPosition = player_->GetWorldPosition();
		const float facing = player_->GetLRDirection() == Player::LRDirection::kRight ? 1.0f : -1.0f;
		// 【光の出る位置】Xはプレイヤーの少し前、Yは足元付近、Zはモデルの手前。
		// 足元をもっと照らしたいときはY、モデル前面への当たり方はZで調整する。
		const Vector3 lightPosition = {playerPosition.x + facing * 0.15f,
			playerPosition.y + 0.10f, playerPosition.z - 0.65f};
		lightGroup_->SetSpotLightPos(0, lightPosition);
		// LightGroup がGPU転送時に反転するため、見た目の進行方向を渡す。
		lightGroup_->SetSpotLightDir(0, {facing, 0.0f, 0.0f});
	}

	// エンジンの丸影は1枠なので、プレイヤーに近い予告中の落石を表示する。
	float nearestDistanceSquared = (std::numeric_limits<float>::max)();
	Vector3 shadowImpact{};
	float shadowStrength = 0.0f;
	bool hasShadow = false;
	for (BaseGimmick* gimmick : gimmicks_)
	{
		auto* stageGimmick = dynamic_cast<StageGimmick*>(gimmick);
		Vector3 impact{};
		float strength = 0.0f;
		if (!stageGimmick || !stageGimmick->GetFallingRockShadow(impact, strength))
		{
			continue;
		}
		const Vector3 playerPosition = player_ ? player_->GetWorldPosition() : impact;
		const float dx = impact.x - playerPosition.x;
		const float dy = impact.y - playerPosition.y;
		const float distanceSquared = dx * dx + dy * dy;
		if (distanceSquared < nearestDistanceSquared)
		{
			nearestDistanceSquared = distanceSquared;
			shadowImpact = impact;
			shadowStrength = strength;
			hasShadow = true;
		}
	}
	lightGroup_->SetCircleShadowActive(0, hasShadow);
	if (hasShadow)
	{
		lightGroup_->SetCircleShadowCasterPos(0,
			{shadowImpact.x, shadowImpact.y + 0.025f, shadowImpact.z});
		lightGroup_->SetCircleShadowAtten(0, {4.0f - 2.4f * shadowStrength, 0.0f, 0.0f});
	}
	lightGroup_->Update();
}

void GameScene::GenerateEnemy(uint32_t xIndex, uint32_t yIndex, uint8_t subID)
{
	const Vector3 position = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);
	const std::string enemyType = subID == 1 ? "ShieldEnemy" : subID == 4 ? "FlyingEnemy" : "WalkEnemy";
	GenerateEnemyAtPosition(position, enemyType);
}

void GameScene::GenerateEnemyAtPosition(const Vector3& position, const std::string& enemyType)
{
	BaseEnemy* enemy = nullptr;
	Model* model = nullptr;

	if (enemyType == "ShieldEnemy")
	{
		enemy = new ShieldEnemy();
		model = modelShieldEnemy_;
	}
	else if (enemyType == "FlyingEnemy")
	{
		enemy = new FlyingEnemy();
		model = modelFlyingEnemy_;
	}
	else
	{
		enemy = new WalkEnemy();
		model = modelWalkEnemy_;
	}

	enemy->Initialize(model, cameraController_->GetCamera(), position);
	enemy->SetGameScene(this);
	enemies_.push_back(enemy);
}

void GameScene::GenerateGimmick(uint32_t xIndex, uint32_t yIndex, uint8_t subID)
{
	if (subID > static_cast<uint8_t>(GimmickType::kAirRushSwitch))
	{
		return;
	}
	GenerateGimmickAtPosition(mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex),
		static_cast<GimmickType>(subID));
}

void GameScene::GenerateGimmickAtPosition(const Vector3& position, GimmickType type,
	const std::string& linkId)
{
	Model* gimmickModel = modelBlock_;
	switch (type)
	{
	case GimmickType::kFountainJump:
		gimmickModel = modelFountain_;
		break;
	case GimmickType::kMovingPlatform:
	case GimmickType::kWaterWheelLift:
		// 水車の4枚の台にも移動足場モデルを使う。
		gimmickModel = modelMovingPlatform_;
		break;
	case GimmickType::kUpdraft:
		gimmickModel = modelUpdraft_;
		break;
	case GimmickType::kBounceLeaf:
		gimmickModel = modelLotusLeaf_;
		break;
	case GimmickType::kFallingRock:
		gimmickModel = modelFallingRock_;
		break;
	case GimmickType::kVine:
		gimmickModel = modelVine_;
		break;
	case GimmickType::kOreSwitch:
		gimmickModel = modelOreSwitch_;
		break;
	case GimmickType::kAirRushSwitch:
		gimmickModel = modelRingBoxEffect_;
		break;
	case GimmickType::kStoneBridge:
		gimmickModel = modelStoneBridge_;
		break;
	case GimmickType::kVanishingCloud:
		gimmickModel = modelVanishingCloud_;
		break;
	case GimmickType::kThunderCloud:
		gimmickModel = modelThunderCloud_;
		break;
	case GimmickType::kAirRushRing:
		gimmickModel = modelGuardEffect_;
		break;
	}

	StageGimmick* gimmick = new StageGimmick();
	gimmick->Initialize(gimmickModel, modelLightning_, modelWaterWheelShaft_,
		modelRingBoxEffect_, modelRingBoardEffect_,
		cameraController_->GetCamera(), mapChipField_,
		&switchLinkState_, position, type, linkId);
	gimmicks_.push_back(gimmick);
}

void GameScene::GenerateCheckpoint(uint32_t xIndex, uint32_t yIndex, uint8_t subID)
{
	CheckpointGate* checkpoint = new CheckpointGate();
	checkpoint->Initialize(modelBlock_, cameraController_->GetCamera(),
		mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex), subID,
		&respawnPosition_, &activatedCheckpointIndex_);
	gimmicks_.push_back(checkpoint);
}

void GameScene::GenerateFieldObjects() 
{

	// 要素数
	uint32_t numBlockVirtical = mapChipField_->GetNumBlockVirtical();
	uint32_t numBlockHorizontal = mapChipField_->GetNumBlockHorizontal();

	// プレイヤー・敵・ギミックを生成する。ブロック描画は最後にまとめて生成する。
	for (uint32_t i = 0; i < numBlockVirtical; ++i)
	{
		for (uint32_t j = 0; j < numBlockHorizontal; ++j)
		{

			// 該当マスのマップチップ種別を取得
			MapChipType chipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			// 種類に応じてオブジェクトを生成する
			switch (chipType) 
			{
			case MapChipType::kBlock:
				break;
			case MapChipType::kPlayer:
			{
				assert(player_ == nullptr && "自キャラを二重に配置しようとしています");
				// 自キャラの生成
				player_ = new Player();
				// マス目の座標を取得
				Vector3 playerPosition = mapChipField_->GetMapChipPositionByIndex(j, i);
				// プレイヤーの初期化
				player_->Initialize(modelPlayer_, modelAttackEffect_, cameraController_->GetCamera(), playerPosition);
				respawnPosition_ = playerPosition;
				// プレイヤーにマップチップフィールドをセット
				player_->SetMapChipField(mapChipField_);
				break;
			}
			case MapChipType::kEnemy: 
			{
				// サブIDを取得
				uint8_t subID = mapChipField_->GetMapChipSubIDByIndex(j, i);
				// 敵生成関数を呼び出す
				GenerateEnemy(j, i, subID);
				break;
			}
			case MapChipType::kGimmick:
			{
				GenerateGimmick(j, i, mapChipField_->GetMapChipSubIDByIndex(j, i));
				break;
			}
			case MapChipType::kCheckpoint:
			{
				GenerateCheckpoint(j, i, mapChipField_->GetMapChipSubIDByIndex(j, i));
				break;
			}
			default:
				// 空白（kBlank）など、ブロック以外のときは何もしない
				break;
			}
		}
	}

	BuildBlockTransforms();

}

void GameScene::BuildBlockTransforms()
{
	for (WorldTransform* transform : blockTransforms_)
	{
		delete transform;
	}
	blockTransforms_.clear();

	const uint32_t height = mapChipField_->GetNumBlockVirtical();
	const uint32_t width = mapChipField_->GetNumBlockHorizontal();

	for (uint32_t y = 0; y < height; ++y)
	{
		for (uint32_t x = 0; x < width; ++x)
		{
			if (mapChipField_->GetMapChipTypeByIndex(x, y) != MapChipType::kBlock)
			{
				continue;
			}

			WorldTransform* transform = new WorldTransform();
			transform->Initialize();
			transform->translation_ = mapChipField_->GetMapChipPositionByIndex(x, y);
			transform->matWorld_ = MyMathUtility::MakeAffineMatrix(
				transform->scale_, transform->rotation_, transform->translation_);
			transform->TransferMatrix();
			blockTransforms_.push_back(transform);
		}
	}
}

void GameScene::ClearRoomObjects()
{
	triangleTraces_.clear();
	for (BaseEnemy* enemy : enemies_)
	{
		delete enemy;
	}
	enemies_.clear();

	for (BaseGimmick* gimmick : gimmicks_)
	{
		delete gimmick;
	}
	gimmicks_.clear();

	for (WorldTransform* transform : blockTransforms_)
	{
		delete transform;
	}
	blockTransforms_.clear();

	for (BaseEffect* effect : effects_)
	{
		delete effect;
	}
	effects_.clear();

	if (deathParticles_)
	{
		delete deathParticles_;
		deathParticles_ = nullptr;
	}
}

bool GameScene::LoadTiledRoom(const std::string& roomPath, const std::string& spawnId)
{
	TiledRoomData loadedRoom;
	try
	{
		loadedRoom = TiledRoomLoader::Load(roomPath);
	}
	catch (const std::exception& error)
	{
		tiledRoomLoadError_ = error.what();
		return false;
	}

	const TiledSpawnData* requestedSpawn = loadedRoom.FindSpawn(spawnId);
	if (!requestedSpawn)
	{
		const auto firstPlayerSpawn = std::find_if(
			loadedRoom.spawns.begin(), loadedRoom.spawns.end(), [](const TiledSpawnData& spawn) {
				return spawn.className == "PlayerSpawn";
			});
		if (firstPlayerSpawn == loadedRoom.spawns.end())
		{
			tiledRoomLoadError_ = "Tiled room has no PlayerSpawn: " + roomPath;
			return false;
		}
		requestedSpawn = &*firstPlayerSpawn;
	}

	const Vector3 playerPosition = requestedSpawn->position;
	const std::string normalizedPath = std::filesystem::path(roomPath).lexically_normal().generic_string();

	ClearRoomObjects();
	currentRoom_ = std::move(loadedRoom);
	currentRoomPath_ = normalizedPath;
	tiledRoomLoadError_.clear();
	// スイッチと石橋の時限状態は部屋ごとに初期化する。
	switchLinkState_.Reset();

	mapChipField_->ResetMapChipData(currentRoom_.width, currentRoom_.height);
	for (uint32_t y = 0; y < currentRoom_.height; ++y)
	{
		for (uint32_t x = 0; x < currentRoom_.width; ++x)
		{
			if (currentRoom_.IsSolid(x, y))
			{
				mapChipField_->SetMapChipTypeByIndex(x, y, MapChipType::kBlock);
			}
		}
	}
	BuildBlockTransforms();

	if (!player_)
	{
		player_ = new Player();
		player_->Initialize(modelPlayer_, modelAttackEffect_, cameraController_->GetCamera(), playerPosition);
	}
	else
	{
		player_->Respawn(playerPosition, false);
	}
	player_->SetMapChipField(mapChipField_);

	if (!worldProgress_.HasCheckpoint())
	{
		worldProgress_.ActivateCheckpoint(currentRoomPath_, "initial", playerPosition);
	}

	for (const TiledSpawnData& spawn : currentRoom_.spawns)
	{
		if (spawn.className == "EnemySpawn")
		{
			GenerateEnemyAtPosition(spawn.position, spawn.enemyType);
		}
	}
	for (const TiledGimmickData& gimmick : currentRoom_.gimmicks)
	{
		GenerateGimmickAtPosition(gimmick.position, gimmick.type, gimmick.linkId);
	}

	for (const TiledCheckpointData& checkpointData : currentRoom_.checkpoints)
	{
		CheckpointTrigger* checkpoint = new CheckpointTrigger();
		checkpoint->Initialize(modelCheckpoint_, modelCheckpointJewel_,
			cameraController_->GetCamera(), triangleWhiteTexture_, checkpointData.position,
			&worldProgress_, currentRoomPath_, checkpointData.id);
		gimmicks_.push_back(checkpoint);
	}

	for (const TiledTriangleData& triangleData : currentRoom_.triangles)
	{
		TriangleTraceGimmick* triangle = new TriangleTraceGimmick();
		const std::string progressKey = currentRoomPath_ + ":triangle:" + triangleData.id;
		if (triangle->Initialize(modelTriangleEdge_, modelTriangleFill_, triangleWhiteTexture_,
			cameraController_->GetCamera(), triangleData, &worldProgress_, progressKey))
		{
			gimmicks_.push_back(triangle);
			triangleTraces_.push_back(triangle);
		}
		else
		{
			delete triangle;
		}
	}

	ConfigureCameraForCurrentMap();
	// 入ってきたRoomExitへ即座に戻るのを防ぐ。
	// プレイヤーが出口範囲から離れると、再び移動可能になる。
	isRoomExitArmed_ = false;
	return true;
}

void GameScene::ConfigureCameraForCurrentMap()
{
	Camera* camera = cameraController_->GetCamera();
	const float mapWidth = static_cast<float>(mapChipField_->GetNumBlockHorizontal());
	const float mapHeight = static_cast<float>(mapChipField_->GetNumBlockVirtical());
	const float tanHalfFov = std::tan(camera->fovAngleY * 0.5f);

	// 小さな部屋ではカメラを近づけ、画面の縁がマップ外へ出ない画角にする。
	constexpr float kDefaultDistance = 15.0f;
	constexpr float kEdgeInset = 0.05f;
	const float availableHalfWidth = (std::max)(mapWidth * 0.5f - kEdgeInset, 0.05f);
	const float availableHalfHeight = (std::max)(mapHeight * 0.5f - kEdgeInset, 0.05f);
	const float widthFitDistance = availableHalfWidth / (tanHalfFov * camera->aspectRatio);
	const float heightFitDistance = availableHalfHeight / tanHalfFov;
	const float cameraDistance = (std::min)(kDefaultDistance, (std::min)(widthFitDistance, heightFitDistance));
	cameraController_->targetOffset_.z = -cameraDistance;

	const float visibleHalfHeight = cameraDistance * tanHalfFov;
	const float visibleHalfWidth = visibleHalfHeight * camera->aspectRatio;
	const float mapLeft = -0.5f;
	const float mapRight = mapWidth - 0.5f;
	const float mapBottom = -0.5f;
	const float mapTop = mapHeight - 0.5f;
	const float cameraLeft = mapLeft + visibleHalfWidth;
	const float cameraRight = mapRight - visibleHalfWidth;
	const float cameraBottom = mapBottom + visibleHalfHeight;
	const float cameraTop = mapTop - visibleHalfHeight;

	cameraController_->SetMovableArea({cameraLeft, cameraRight, cameraBottom, cameraTop});
	cameraController_->SetFixedCameraX(false);
}

void GameScene::CheckRoomTransitions()
{
	if (phase_ != Phase::kPlay || !player_)
	{
		return;
	}

	const AABB playerBounds = player_->GetAABB();

	// 部屋へ入った直後は、到着地点付近のRoomExitから離れるまで待つ。
	if (!isRoomExitArmed_)
	{
		for (const TiledRoomExitData& exit : currentRoom_.exits)
		{
			if (MyMathUtility::IsCollision(playerBounds, exit.bounds))
			{
				return;
			}
		}

		// すべての出口範囲から離れたので、次フレームから移動可能にする。
		isRoomExitArmed_ = true;
		return;
	}

	// 高速移動でRoomExitを1フレーム中に通り抜けても検出できるよう、
	// 前フレームから現在までを覆うAABBを作成する。
	const AABB previousBounds = player_->GetPreviousAABB();

	AABB sweptBounds{};
	sweptBounds.min = 
	{
	    (std::min)(playerBounds.min.x, previousBounds.min.x),
	    (std::min)(playerBounds.min.y, previousBounds.min.y),
	    (std::min)(playerBounds.min.z, previousBounds.min.z),
	};

	sweptBounds.max = 
	{
	    (std::max)(playerBounds.max.x, previousBounds.max.x),
	    (std::max)(playerBounds.max.y, previousBounds.max.y),
	    (std::max)(playerBounds.max.z, previousBounds.max.z),
	};

	for (const TiledRoomExitData& exit : currentRoom_.exits)
	{
		if (exit.targetRoom.empty() || !MyMathUtility::IsCollision(sweptBounds, exit.bounds))
		{
			continue;
		}

		const std::filesystem::path targetPath = std::filesystem::path(currentRoomPath_).parent_path() / exit.targetRoom;

		if (LoadTiledRoom(targetPath.lexically_normal().generic_string(), exit.targetSpawn))
		{
			cameraController_->SetTarget(player_);
			cameraController_->Reset();
		}

		return;
	}
}

bool GameScene::IsVisible(const AABB& bounds, float margin) const
{
	const Camera* camera = cameraController_->GetCamera();
	const Vector3& cameraPosition = camera->translation_;
	const float distance = std::abs(cameraPosition.z);
	const float visibleHalfHeight = distance * std::tan(camera->fovAngleY * 0.5f);
	const float visibleHalfWidth = visibleHalfHeight * camera->aspectRatio;
	return bounds.max.x >= cameraPosition.x - visibleHalfWidth - margin &&
		bounds.min.x <= cameraPosition.x + visibleHalfWidth + margin &&
		bounds.max.y >= cameraPosition.y - visibleHalfHeight - margin &&
		bounds.min.y <= cameraPosition.y + visibleHalfHeight + margin;
}

// 全ての当たり判定を行う
void GameScene::CheckAllCollisions()
{
	#pragma region 自キャラと敵キャラの当たり判定
	{
		// 判定対象1と2の座標
		AABB aabb1, aabb2;

		// 通常時はプレイヤー本体、突進中はプレイヤー周囲のAABBを使う。
		aabb1 = player_->IsAttack() ? player_->GetAttackAABB() : player_->GetAABB();

		// 自キャラと敵キャラ全ての当たり判定
		for (BaseEnemy* enemy : enemies_)
		{
			// コリジョン無効の敵はスキップする
			if (enemy->IsCollisionDisabled())
			{
				continue;
			}

			// 敵キャラの座標
			aabb2 = enemy->GetAABB();

			// AABB同士の交差判定
			if (MyMathUtility::IsCollision(aabb1, aabb2))
			{
				// 自キャラの衝突時関数を呼び出す
				player_->OnCollision(enemy);
				// 敵の衝突時関数を呼び出す
				enemy->OnCollision(player_);
			}
		}

		
	}
	#pragma endregion

	#pragma region 自キャラとステージギミックの当たり判定
	{
		for (BaseGimmick* gimmick : gimmicks_)
		{
			if (!gimmick->IsActive())
			{
				continue;
			}

			const AABB playerAABB = player_->GetAABB();
			const AABB gimmickAABB = gimmick->GetAABB();
			const bool isCurrentCollision = MyMathUtility::IsCollision(playerAABB, gimmick->GetAABB());
			const bool isPreviousCollision = gimmick->IsSolid() &&
				MyMathUtility::IsCollision(player_->GetPreviousAABB(), gimmick->GetAABB());
			const bool isAttackCollision = player_->IsAttack() &&
				MyMathUtility::IsCollision(player_->GetAttackAABB(), gimmickAABB);

			// 足場の上面に作るわずかな隙間を接地として扱う。
			const bool isStandingOnSolidGimmick =
				gimmick->IsSolid() &&
				player_->GetVelocity().y <= 0.0f &&
				playerAABB.max.x > gimmickAABB.min.x &&
				playerAABB.min.x < gimmickAABB.max.x &&
				playerAABB.min.y >= gimmickAABB.max.y - 0.01f &&
				playerAABB.min.y <= gimmickAABB.max.y + 0.02f;

			if (isCurrentCollision || isPreviousCollision || isStandingOnSolidGimmick || isAttackCollision)
			{
				gimmick->OnPlayerCollision(player_);
			}
		}
	}
	#pragma endregion

	#pragma region 自キャラとアイテムの当たり判定

	#pragma endregion
	
	#pragma region 自弾と敵キャラの当たり判定

	#pragma endregion

}

// フェーズの切り替え
void GameScene::ChangePhase()
{
	switch (phase_)
	{
	case Phase::kFadeIn:
		// フェードインが終わったらゲームプレイフェーズに切り替える
		if (fade_->IsFinished())
		{
			phase_ = Phase::kPlay;
		}
		break;

	case Phase::kPlay:
	{
		// ゲームプレイフェーズの処理
		
		const float clearLineY = static_cast<float>(mapChipField_->GetNumBlockVirtical()) - 0.5f;

		// Tiledの地続きマップでは部屋出口を使うため、従来CSVだけ上端クリアを有効にする。
		if (!isUsingTiledRooms_ && player_->GetWorldPosition().y > clearLineY)
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kGoal);
			isCleared_ = true;
			stageManager_->MarkCurrentStageCleared();
			stageClearTimer_ = 0.0f;
			phase_ = Phase::kStageClear;
			break;
		}


		// HPの残量に関わらず、ダメージ後は一度デス演出を再生する。
		const bool isCheckpointRespawnRequested = player_->ConsumeRespawnRequest();
		if (isCheckpointRespawnRequested || player_->IsDead())
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerDeath);
			// HPが0の時だけ、デス演出後にゲームオーバー選択へ移る。
			isGameOverPending_ = player_->IsDead();
			// フェーズをデス演出フェーズに切り替える
			phase_ = Phase::kDeath;
			// 自キャラの座標を取得
			const Vector3& deathParticlesPosition = player_->GetWorldPosition();

			// 自キャラの座標にデスパーティクルを発生、初期化
			deathParticles_ = new DeathParticles();
			deathParticles_->Initialize(modelDeathParticles_, cameraController_->GetCamera(), deathParticlesPosition);
		}

		break;
	}

	case Phase::kDeath:
		// デス演出フェーズの処理
		if (deathParticles_ && deathParticles_->IsFinished())
		{
			delete deathParticles_;
			deathParticles_ = nullptr;

			if (isGameOverPending_)
			{
				isRetrySelected_ = true;
				phase_ = Phase::kGameOver;
			}
			else
			{
				// HPを保ったまま、最後のチェックポイントへ戻す。
				if (isUsingTiledRooms_ && worldProgress_.HasCheckpoint())
				{
					if (currentRoomPath_ != worldProgress_.GetCheckpointRoomPath())
					{
						LoadTiledRoom(worldProgress_.GetCheckpointRoomPath(), "");
						cameraController_->SetTarget(player_);
					}
					player_->Respawn(worldProgress_.GetRespawnPosition(), false);
				}
				else
				{
					player_->Respawn(respawnPosition_, false);
				}
				SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerRespawn);
				cameraController_->Reset();
				fade_->Start(Fade::Status::FadeIn, kFadeDuration);
				phase_ = Phase::kFadeIn;
			}
		}

		break;

	case Phase::kStageClear:
		// ゲームを止めてクリア演出を表示してからフェードアウトする。
		stageClearTimer_ += GameTime::GetDeltaFrames();
		if (stageClearTimer_ >= kStageClearDuration)
		{
			fade_->Start(Fade::Status::FadeOut, kFadeDuration);
			phase_ = Phase::kFadeOut;
		}
		break;

	case Phase::kFadeOut:
		// フェードアウトが終わったらシーン終了フラグを立てる
		if (fade_->IsFinished()) 
		{
			finished_ = true;
		}
		break;

	}
}

// エフェクトを生成
void GameScene::CreateHitEffect(const KamataEngine::Vector3& position) 
{ 
	HitEffect* newHitEffect = HitEffect::Create(position);
	effects_.push_back(newHitEffect);
}

void GameScene::CreateGuardEffect(const KamataEngine::Vector3& position)
{
	GuardEffect* newGuardEffect = GuardEffect::Create(position);
	effects_.push_back(newGuardEffect);
}

void GameScene::Draw()
{

	// 3Dモデル描画前処理
	Model::PreDraw();
	
	skydome_->Draw();

	for (BaseEnemy* enemy : enemies_)
	{
		if (IsVisible(enemy->GetAABB()))
		{
			enemy->Draw();
		}
	}

	// 1マスごとのブロックのうち、カメラ周辺だけを描画する。
	for (WorldTransform* transform : blockTransforms_)
	{
		const Vector3& position = transform->translation_;
		const Vector3& scale = transform->scale_;
		const AABB bounds = {
			{position.x - scale.x * 0.5f, position.y - scale.y * 0.5f, position.z - 0.5f},
			{position.x + scale.x * 0.5f, position.y + scale.y * 0.5f, position.z + 0.5f}};
		if (IsVisible(bounds))
		{
			modelBlock_->Draw(*transform, *cameraController_->GetCamera());
		}
	}

	for (BaseGimmick* gimmick : gimmicks_)
	{
		if (IsVisible(gimmick->GetDrawAABB()))
		{
			gimmick->Draw();
		}
	}

	if (phase_ == Phase::kFadeIn || phase_ == Phase::kPlay || phase_ == Phase::kStageClear)
	{
		if (player_) 
		{
			player_->Draw();
		}
	} 
	else if (phase_ == Phase::kDeath)
	{
		// デスパーティクルが存在するなら描画
		if (deathParticles_)
		{
			deathParticles_->Draw();
		}
	}
	
	// エフェクトの描画
	for (BaseEffect* effect : effects_) 
	{
		effect->Draw();
	}

	// 3Dモデル描画後処理
	Model::PostDraw();

	if (phase_ == Phase::kFadeIn || phase_ == Phase::kPlay)
	{
		DrawHud();
	}

	if (player_->IsRushAiming()) {
		Sprite::PreDraw();
		jumpDirectionCursor_->Draw();
		Sprite::PostDraw();
	}

	if (phase_ == Phase::kGameOver)
	{
		Sprite::PreDraw();
		gameOverOverlay_->Draw();
		retryChoice_->Draw();
		stageSelectChoice_->Draw();
		gameOverSelector_->Draw();
		Sprite::PostDraw();
	}

	if (phase_ == Phase::kStageClear)
	{
		Sprite::PreDraw();
		stageClearOverlay_->Draw();
		stageClearBanner_->Draw();
		Sprite::PostDraw();
	}

	// フェードの描画
	fade_->Draw();

}

void GameScene::DrawHud()
{
	Sprite::PreDraw();
	hudGuideSprite_->Draw();
	Sprite::PostDraw();
}

