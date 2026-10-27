#pragma once

#include "BaseGimmick.h"
#include "KamataEngine.h"
#include "TiledRoom.h"
#include "WorldProgress.h"

#include <array>
#include <cstdint>
#include <string>

class Player;

// Tiledの輪郭を突進で三辺なぞると、斜辺で跳ねられる三角形になる。
class TriangleTraceGimmick final : public BaseGimmick
{
public:
	struct SnapCandidate
	{
		KamataEngine::Vector3 vertex{};
		std::array<KamataEngine::Vector3, 2> directions{};
		int directionCount = 0;
		float distanceSquared = 0.0f;
	};

	bool Initialize(KamataEngine::Model* edgeModel, KamataEngine::Model* fillModel,
		uint32_t whiteTexture, KamataEngine::Camera* camera, const TiledTriangleData& data,
		WorldProgress* progress, const std::string& progressKey);
	bool FindSnapCandidate(const KamataEngine::Vector3& playerPosition, SnapCandidate& candidate) const;
	void ObserveRush(Player* player);

	void Update() override;
	void Draw() override;
	AABB GetAABB() const override;
	void OnPlayerCollision(Player* player) override;
	bool IsActive() const override { return true; }
	bool IsSolid() const override { return false; }

private:
	std::array<KamataEngine::Vector3, 3> vertices_{};
	std::array<bool, 3> traced_{};
	std::array<KamataEngine::WorldTransform, 3> edgeTransforms_{};
	std::array<KamataEngine::ObjectColor, 3> edgeColors_{};
	KamataEngine::WorldTransform fillTransform_;
	KamataEngine::ObjectColor fillColor_;
	KamataEngine::Model* edgeModel_ = nullptr;
	KamataEngine::Model* fillModel_ = nullptr;
	KamataEngine::Camera* camera_ = nullptr;
	WorldProgress* progress_ = nullptr;
	std::string progressKey_;
	uint32_t whiteTexture_ = 0;
	uint64_t observedRushSequence_ = 0;
	int activeEdge_ = -1;
	int edgeEndVertex_ = -1;
	int leftBaseIndex_ = -1;
	int rightBaseIndex_ = -1;
	int apexIndex_ = -1;
	bool completed_ = false;
	bool completedThisFrame_ = false;
};
