#pragma once

#include "AABB.h"

class Player;

// ステージギミック共通の基底クラス
class BaseGimmick
{
public:
	virtual ~BaseGimmick() = default;

	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual AABB GetAABB() const = 0;
	virtual void OnPlayerCollision(Player* player) = 0;
	virtual bool IsActive() const = 0;
	virtual bool IsSolid() const = 0;
};
