#pragma once
#include "BaseEnemy.h"
#include "KamataEngine.h"

class ShieldEnemy : public BaseEnemy {
protected:
	// 必要になったらvirtual overrideを追加する
	// 親クラス（BaseEnemy）の歩行処理を上書きして、独自の動きにする
	void BehaviorWalkUpdate() override;
	
public:
	// 衝突判定をShieldEnemy専用に上書きする
	void OnCollision(class Player* player) override;
	
	// 更新処理をShieldEnemy専用に上書きする
	void Update() override;

private:

	// ガード状態の初期化と更新関数
	void BehaviorGuardInitialize();
	void BehaviorGuardUpdate();

	// クールタイム用の変数
	float guardEffectTimer_ = 0.0f;                  // タイマー変数
	static inline const float kGuardCoolTime = 0.1f; // 連続発生を防ぐ時間

	// のけぞり演出用の変数
	float guardTimer_ = 0.0f;
	static inline const float kGuardTime = 0.5f; // のけぞる時間

};