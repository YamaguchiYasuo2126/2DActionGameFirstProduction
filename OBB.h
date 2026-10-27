#pragma once
#include "KamataEngine.h"

// OrientedBoundingBox
struct OBB {
	KamataEngine::Vector3 center;         // 中心点
	KamataEngine::Vector3 orientation[3]; // 座標軸。正規化・直交必須
	KamataEngine::Vector3 size;           // 座標軸方向の長さの半分。中心から面までの距離
};