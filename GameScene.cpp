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
#include "GamePadInput.h"
#include "SoundManager.h"


using namespace KamataEngine;

GameScene::GameScene() {}

GameScene::~GameScene() {
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

	// 敵キャラクターのインスタンスを解放する
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

	// ブロックの3Dモデルデータの解放
	delete modelBlock_;
	delete modelFountain_;
	delete modelMovingPlatform_;
	delete modelUpdraft_;
	delete modelLotusLeaf_;
	delete modelFallingRock_;
	delete modelVine_;
	delete modelOreSwitch_;
	delete modelStoneBridge_;
	delete modelVanishingCloud_;
	delete modelThunderCloud_;
	delete modelLightning_;

	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
			delete worldTransformBlock;
		}
	}

	worldTransformBlocks_.clear();

	// デスパーティクルのモデルの開放
	delete modelDeathParticles_;
	// デスパーティクルのインスタンスの解放
	if (deathParticles_) 
	{
		delete deathParticles_;
		deathParticles_ = nullptr; // 安全のためnullptrを代入
	}

	// エフェクトのインスタンスを解放する
	for (BaseEffect* effect : effects_)
	{
		delete effect;
	}
	effects_.clear();

	// デバッグカメラの解放
	delete debugCamera_;

	// 天球の解放
	delete skydome_;

	// 天球のモデルの解放
	delete modelSkydome_;

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
	delete hudGuide_;
}

void GameScene::Initialize(StageManager* stageDataManager) 
{
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
	// このモデルは通常ブロック・チェックポイント・基本ギミックで共通して使用する。
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

	// ギミックごとの見た目を読み込む。衝突判定・挙動は従来どおりStageGimmick側で管理する。
	modelFountain_ = Model::CreateFromOBJ("fountain", true);
	modelMovingPlatform_ = Model::CreateFromOBJ("movingPlatform", true);
	modelUpdraft_ = Model::CreateFromOBJ("upDraft", true);
	modelLotusLeaf_ = Model::CreateFromOBJ("lotusLeaf", true);
	modelFallingRock_ = Model::CreateFromOBJ("fallingRock", true);
	modelVine_ = Model::CreateFromOBJ("vine", true);
	modelOreSwitch_ = Model::CreateFromOBJ("oreSwitch", true);
	modelStoneBridge_ = Model::CreateFromOBJ("stoneBridge", true);
	modelVanishingCloud_ = Model::CreateFromOBJ("vanishingCloud", true);
	modelThunderCloud_ = Model::CreateFromOBJ("thunderCloud", true);
	modelLightning_ = Model::CreateFromOBJ("lightning", true);

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

	uint32_t textureHandle = TextureManager::Load("white1x1.png");

	jumpGaugeBack_ = Sprite::Create(textureHandle, Vector2(40.0f, 650.0f));
	jumpGaugeBack_->SetSize(Vector2(240.0f, 24.0f));
	jumpGaugeBack_->SetColor(Vector4(0.1f, 0.1f, 0.1f, 0.8f));

	jumpGaugeFill_ = Sprite::Create(textureHandle, Vector2(44.0f, 654.0f));
	jumpGaugeFill_->SetColor(Vector4(0.2f, 0.8f, 1.0f, 1.0f));

	jumpDirectionBack_ = Sprite::Create(textureHandle, Vector2(40.0f, 685.0f));
	jumpDirectionBack_->SetSize(Vector2(180.0f, 24.0f));
	jumpDirectionBack_->SetColor(Vector4(0.1f, 0.1f, 0.1f, 0.8f));

	jumpDirectionCursor_ = Sprite::Create(textureHandle, Vector2(44.0f, 689.0f));
	jumpDirectionCursor_->SetSize(Vector2(52.0f, 16.0f));
	jumpDirectionCursor_->SetColor(Vector4(1.0f, 0.8f, 0.2f, 1.0f));

	// FireAlpacaで作成した透過HUDを画面全体に重ねる。
	const uint32_t hudGuideTextureHandle = TextureManager::Load("UI/hudGuide.png");
	hudGuide_ = Sprite::Create(hudGuideTextureHandle, Vector2(0.0f, 0.0f));
	hudGuide_->SetSize(Vector2(1280.0f, 720.0f));

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

	// 現在のステージデータを取得する
	const StageData& stageData = stageManager_->GetCurrentStageData();

	// ステージファイルパスの設定
	std::string stageFileName = "Resources/fields/" + stageData.name + ".csv";
	mapChipField_->LoadMapChipCsv(stageFileName);

	// カメラコントローラの生成
	cameraController_ = new CameraController();

	// カメラコントローラの初期化
	cameraController_->Initialize();

	// エフェクトにカメラをセット
	HitEffect::SetCamera(cameraController_->GetCamera());
	GuardEffect::SetCamera(cameraController_->GetCamera());

	// カメラコントローラーに移動範囲をセット
	Rect cameraArea = {11.0f, 11.0f, 6.3f, 93.2f};
	cameraController_->SetMovableArea(cameraArea);

	cameraController_->SetFixedCameraX(true, 11.0f);

	// ゲーム開始時はまだ作らないのでnullptrを入れておく
	deathParticles_ = nullptr;

	// カメラのリセットを行う前に、マップチップからブロックとプレイヤーを生成・配置する
	GenerateFieldObjects();
	
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
	// ここにインゲームの更新処理を書く
	#ifdef _DEBUG
	if (Input::GetInstance()->TriggerKey(DIK_C))
	{
		isDebugCameraActive_ = !isDebugCameraActive_;
	}

	ImGui::Begin("Stage");
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

	// 鉱石スイッチで有効になる石橋の残り時間を更新する。
	if (oreSwitchTimer_ > 0)
	{
		--oreSwitchTimer_;
		isOreSwitchActive_ = oreSwitchTimer_ > 0;
	}

	// フェーズ切り替えのチェックを呼び出す
	ChangePhase();

	switch (phase_)
	{
	case Phase::kFadeIn:
	case Phase::kPlay:
		// ゲームプレイフェーズの処理

		// 天球の更新
		skydome_->Update();
	
		// 自キャラの更新
		player_->Update();
		if (player_->IsJumpCharging())
		{
			float rate = std::clamp(player_->GetJumpChargeRate(), 0.0f, 1.0f);
			jumpGaugeFill_->SetSize(Vector2(232.0f * rate, 16.0f));

			float angle = player_->GetChargedJumpAngle();

			const Vector2 center = {130.0f, 765.0f};
			const float radius = 80.0f;

			float x = center.x + std::cos(angle) * radius;
			float y = center.y - std::sin(angle) * radius;

			jumpDirectionCursor_->SetPosition(Vector2(x, y));
		}

		


		// 敵キャラの更新
		for (BaseEnemy* enemy : enemies_)
		{
			enemy->Update();
		}

		for (BaseGimmick* gimmick : gimmicks_)
		{
			gimmick->Update();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) 
		{
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine)
			{
				if (!worldTransformBlock)
				{
					continue;
				}
				worldTransformBlock->matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

				// 定数バッファに転送する
				worldTransformBlock->TransferMatrix();
			}
		}

		// 全ての当たり判定
		CheckAllCollisions();
		
		// デスフラグの立った通常の敵を削除
		enemies_.remove_if([](BaseEnemy* enemy) {
			if (enemy->IsDead())
			{
				delete enemy;
				return true;
			}
			return false;
		});

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

		for (BaseGimmick* gimmick : gimmicks_)
		{
			gimmick->Update();
		}

		// デスパーティクルが存在するなら更新
		if (deathParticles_)
		{
			deathParticles_->Update();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_)
		{
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine)
			{
				if (!worldTransformBlock)
				{
					continue;
				}
				worldTransformBlock->matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

				// 定数バッファに転送する
				worldTransformBlock->TransferMatrix();
			}
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

		for (BaseGimmick* gimmick : gimmicks_)
		{
			gimmick->Update();
		}

		// ブロックの更新
		for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_) {
			for (WorldTransform* worldTransformBlock : worldTransformBlockLine) {
				if (!worldTransformBlock) {
					continue;
				}
				worldTransformBlock->matWorld_ = MyMathUtility::MakeAffineMatrix(worldTransformBlock->scale_, worldTransformBlock->rotation_, worldTransformBlock->translation_);

				// 定数バッファに転送する
				worldTransformBlock->TransferMatrix();
			}
		}

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
		if (effect->IsDead()) {
			delete effect;
			return true;
		}
		return false;
	});

	// カメラコントローラの更新
	cameraController_->Update();

	// デバッグカメラの更新
	if (isDebugCameraActive_) {

		debugCamera_->Update();
		// デバッグカメラからビュー行列とプロジェクション行列をコピーする
		cameraController_->GetCamera()->matView = debugCamera_->GetCamera().matView;
		cameraController_->GetCamera()->matProjection = debugCamera_->GetCamera().matProjection;

		// ビュープロジェクション行列の転送
		cameraController_->GetCamera()->TransferMatrix();
	} else {
		// 通常カメラのビュープロジェクション行列の更新と転送
		cameraController_->GetCamera()->UpdateMatrix();
	}

}

void GameScene::GenerateEnemy(uint32_t xIndex, uint32_t yIndex, uint8_t subID)
{
	// マス目の座標を取得
	Vector3 enemyPosition = mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex);

	switch (subID) 
	{
	case 0: {
		// 歩行敵の生成
		WalkEnemy* newWalkEnemy = new WalkEnemy();
		// 通常の敵キャラの初期化
		newWalkEnemy->Initialize(modelWalkEnemy_, cameraController_->GetCamera(), enemyPosition);
		// 通常の敵にゲームシーンのポインタをセットする(自分自身を渡す)
		newWalkEnemy->SetGameScene(this);
		// リストに追加
		enemies_.push_back(newWalkEnemy);
		break;
	}
	case 1: {
		// 盾敵の生成
		ShieldEnemy* newShieldEnemy = new ShieldEnemy();
		// 盾持ちの敵キャラの初期化
		newShieldEnemy->Initialize(modelShieldEnemy_, cameraController_->GetCamera(), enemyPosition);
		// 盾持ちの敵にゲームシーンのポインタをセットする(自分自身を渡す)
		newShieldEnemy->SetGameScene(this);
		// リストに追加
		enemies_.push_back(newShieldEnemy);
		break;
	}
	case 4: {
		// 飛行敵の生成
		FlyingEnemy* newFlyingEnemy = new FlyingEnemy();
		newFlyingEnemy->Initialize(modelFlyingEnemy_, cameraController_->GetCamera(), enemyPosition);
		newFlyingEnemy->SetGameScene(this);
		enemies_.push_back(newFlyingEnemy);
		break;
	}
	default:
		break;
	}
}

void GameScene::GenerateGimmick(uint32_t xIndex, uint32_t yIndex, uint8_t subID)
{
	
	if (subID > static_cast<uint8_t>(GimmickType::kThunderCloud)) {
		return;
	}

	Model* gimmickModel = modelBlock_;
	switch (static_cast<GimmickType>(subID))
	{
	case GimmickType::kFountainJump:
		gimmickModel = modelFountain_;
		break;
	case GimmickType::kMovingPlatform:
	case GimmickType::kWaterWheelLift:
		// 水車リフト用モデルが追加されるまでは、移動足場モデルを使用する。
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
	case GimmickType::kStoneBridge:
		gimmickModel = modelStoneBridge_;
		break;
	case GimmickType::kVanishingCloud:
		gimmickModel = modelVanishingCloud_;
		break;
	case GimmickType::kThunderCloud:
		gimmickModel = modelThunderCloud_;
		break;
	}

	StageGimmick* gimmick = new StageGimmick();
	gimmick->Initialize(gimmickModel, modelLightning_, cameraController_->GetCamera(), mapChipField_,
		&isOreSwitchActive_, &oreSwitchTimer_,
		mapChipField_->GetMapChipPositionByIndex(xIndex, yIndex),
		static_cast<GimmickType>(subID));
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

	// 要素数を変更する
	// 列数を設定(縦方向のブロック数)
	worldTransformBlocks_.resize(numBlockVirtical);
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		// 1列の要素数を設定(横方向のブロック数)
		worldTransformBlocks_[i].resize(numBlockHorizontal);
	}

	// キューブの生成
	for (uint32_t i = 0; i < numBlockVirtical; ++i) {
		for (uint32_t j = 0; j < numBlockHorizontal; ++j) {

			// 該当マスのマップチップ種別を取得
			MapChipType chipType = mapChipField_->GetMapChipTypeByIndex(j, i);

			// 種類に応じてオブジェクトを生成する
			switch (chipType) 
			{
			case MapChipType::kBlock:
			{
				WorldTransform* worldTransform = new WorldTransform();
				worldTransform->Initialize();
				worldTransformBlocks_[i][j] = worldTransform;
				worldTransformBlocks_[i][j]->translation_ = mapChipField_->GetMapChipPositionByIndex(j, i);
				break;
			}
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

}

// 全ての当たり判定を行う
void GameScene::CheckAllCollisions()
{
	#pragma region 自キャラと敵キャラの当たり判定
	{
		// 判定対象1と2の座標
		AABB aabb1, aabb2;

		// 通常時はプレイヤー本体、攻撃中は上方向攻撃のAABBを使う
		aabb1 = player_->IsAttack() ? player_->GetAttackAABB() : player_->GetAABB();

		// 自キャラと敵キャラ全ての当たり判定
		for (BaseEnemy* enemy : enemies_)
		{
			// コリジョン無効の敵はスキップする
			if (enemy->IsCollisionDisabled()) {
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
		
		const float clearLineY = static_cast<float>(MapChipField::kNumBlockVirtical) - 0.5f;

		// プレイヤー中心がステージ最上端を越えたらクリア
		if (player_->GetWorldPosition().y > clearLineY)
		{
			SoundManager::GetInstance()->PlaySE(SoundEffect::kGoal);
			isCleared_ = true;
			stageManager_->MarkCurrentStageCleared();
			stageClearTimer_ = 0;
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
				player_->Respawn(respawnPosition_, false);
				SoundManager::GetInstance()->PlaySE(SoundEffect::kPlayerRespawn);
				cameraController_->Reset();
				fade_->Start(Fade::Status::FadeIn, kFadeDuration);
				phase_ = Phase::kFadeIn;
			}
		}

		break;

	case Phase::kStageClear:
		// ゲームを止めてクリア演出を表示してからフェードアウトする。
		++stageClearTimer_;
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
		enemy->Draw();
	}

	// ブロックの描画
	for (std::vector<WorldTransform*>& worldTransformBlockLine : worldTransformBlocks_)
	{
		for (WorldTransform* worldTransformBlock : worldTransformBlockLine) 
		{
			if (!worldTransformBlock)
			{
				continue;
			}
			modelBlock_->Draw(*worldTransformBlock, *cameraController_->GetCamera());
		}
	}

	for (BaseGimmick* gimmick : gimmicks_)
	{
		gimmick->Draw();
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

	if (player_->IsJumpCharging()) {
		Sprite::PreDraw();
		jumpGaugeBack_->Draw();
		jumpGaugeFill_->Draw();
		jumpDirectionBack_->Draw();
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
	hudGuide_->Draw();
	Sprite::PostDraw();
}

