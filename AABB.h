#pragma once
#include "KamataEngine.h"

// AxisAlignedBoundingBox
struct AABB 
{
	KamataEngine::Vector3 min; // 最小点
	KamataEngine::Vector3 max; // 最大点
};