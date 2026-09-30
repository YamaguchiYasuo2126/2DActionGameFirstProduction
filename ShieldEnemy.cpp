#define NOMINMAX
#include "ShieldEnemy.h"
#include "EasingUtility.h"
#include "Player.h"
#include "GameScene.h"

#include <numbers>
#include <algorithm>
#include <cassert>

// ShieldEnemy専用のUpdate
void ShieldEnemy::Update()
{
	if (behaviorRequest_ != Behavior::kUnknown)
	{
		// 振る舞いを変更する
		behavior_ = behaviorRequest_;
		// 各振る舞いごとの初期化を実行
		switch (behavior_) {
		case Behavior::kWalk:
		default:
			BehaviorWalkInitialize();
			break;
		case Behavior::kDeath:
			BehaviorDeathInitialize();
			break;
		case Behavior::kGuard:
			BehaviorGuardInitialize();
			break;
		}
		// 振る舞いリクエストをリセット
		behaviorRequest_ = Behavior::kUnknown;
	}

	switch (behavior_) 
	{
	case Behavior::kWalk:
	default:
		BehaviorWalkUpdate();
		break;
	case Behavior::kDeath:
		BehaviorDeathUpdate();
		break;
	case Behavior::kGuard:
		BehaviorGuardUpdate();
		break;
	}
}

// ShieldEnemy 固有の歩行更新処理
void ShieldEnemy::BehaviorWalkUpdate()
{
	// クールタイムタイマーを減らす
	if (guardEffectTimer_ > 0.0f) 
	{
		guardEffectTimer_ -= 1.0f / 60.0f;
	}

	// 移動
	worldTransform_.translation_.x += velocity_.x;

	// タイマーを加算
	walkTimer_ += 1.0f / 60.0f;

	// 回転アニメーション
	// 周期 (kWalkMotionTime) に基づいて0 〜 2πの範囲をループするように計算
	float param = std::sin(std::numbers::pi_v<float> * 2.0f * walkTimer_ / kWalkMotionTime);
	// 揺れ幅（度数法）左右に30度ずつ揺れる
	float wobbleDegree = 30.0f * param;

	// 度をラジアンに変換 (通常の公式)
	float wobbleRadian = wobbleDegree * (std::numbers::pi_v<float> / 180.0f);

	// 初期角度（-90度 = -π/2）に、揺れの角度を足してY軸に適用する
	worldTransform_.rotation_.y = (std::numbers::pi_v<float> / -2.0f) + wobbleRadian;

	// 行列更新 (親クラスの関数を呼び出す)
	UpdateMatrix();
}

void ShieldEnemy::OnCollision(Player* player)
{
	// プレイヤーが攻撃中なら
	if (player->IsAttack()) {

		// プレイヤーの向きと敵の速度を取得
		Player::LRDirection playerDir = player->GetLRDirection();
		float enemyVx = velocity_.x;

		// ガード条件(自キャラが右向き かつ 敵が左向き)または(自キャラが左向き かつ 敵が右向き)
		bool isFacingEachOther = (playerDir == Player::LRDirection::kRight && enemyVx < 0.0f) || (playerDir == Player::LRDirection::kLeft && enemyVx > 0.0f);

		if (isFacingEachOther)
		{
			// クールタイムが0以下の時のみエフェクトを生成
			if (guardEffectTimer_ <= 0.0f)
			{
				// ガードエフェクトを発生させる（盾の位置）
				KamataEngine::Vector3 effectPos = worldTransform_.translation_;

				// 敵が左向き(速度マイナス)なら左に、右向きなら右にエフェクトをずらす
				float offset = (enemyVx < 0.0f) ? -0.8f : 0.8f;
				effectPos.x += offset;

				// GameSceneにエフェクト生成を依頼
				if (gameScene_) {
					gameScene_->CreateGuardEffect(effectPos);
				}
				guardEffectTimer_ = kGuardCoolTime;
			}

			// プレイヤーのノックバックを要求する
			player->RequestKnockback();

			// のけぞり演出へ移行する
			BehaviorGuardInitialize();
			behaviorRequest_ = Behavior::kGuard;

			// 早期リターンでデスを回避
			return;
		}

		// 向かい合っていない場合は、親クラスの衝突処理を実行
		BaseEnemy::OnCollision(player);
	}
}

// ガード行動の初期化
void ShieldEnemy::BehaviorGuardInitialize() 
{ 
	guardTimer_ = 0.0f;
}

// ガード行動の更新（のけぞりアニメーション）
void ShieldEnemy::BehaviorGuardUpdate()
{
	// タイマーを加算
	guardTimer_ += 1.0f / 60.0f;

	// 0.0 〜 1.0 の割合（進行度）を計算
	float t = guardTimer_ / kGuardTime;

	// サイン関数でのけぞり角度を計算（1周期 2π）
	// 最初は下向き（マイナスの角度）にしたいので、-sin にする
	float angle = std::sin(std::numbers::pi_v<float> * 2.0f * t);

	// Z軸の回転に適用（π/4 傾かせる）
	worldTransform_.rotation_.z = angle * (std::numbers::pi_v<float> / 4.0f);

	// 演出時間が終わったら歩行状態に戻る
	if (guardTimer_ >= kGuardTime)
	{
		worldTransform_.rotation_.z = 0.0f; // 角度を綺麗にリセット
		behavior_ = Behavior::kWalk;
	}

	// 行列更新
	UpdateMatrix();
}