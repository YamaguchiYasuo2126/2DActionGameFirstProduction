#pragma once
#include "KamataEngine.h"

// エフェクトの基底クラス
class BaseEffect 
{
public:
	// 仮想デストラクタ
	virtual ~BaseEffect() = default;

	// 仮想関数
	virtual void Initialize(const KamataEngine::Vector3& position) = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual bool IsDead() const = 0;
};